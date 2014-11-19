//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "xrandr12toggler.h"
#include <QWidget>
#include <QX11Info>
#include <algorithm>
#include <functional>
#include <iostream>

static bool operator!=(ResInfo const &l, ResInfo const &r) {
	return l.w != r.w || l.h != r.h;
}

static bool operator>(ResInfo const &l, ResInfo const &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static unsigned getRate(unsigned long dotclock, unsigned long htotal, unsigned long vtotal) {
	if (unsigned long pixels = htotal * vtotal)
		return (dotclock * 10ull + (pixels >> 1)) / pixels;

	return 0;
}

static unsigned getRate(XRRModeInfo const &mode) {
	return getRate(mode.dotClock, mode.hTotal, mode.vTotal);
}

static void addMode(XRRModeInfo const &mode,
                    std::vector<ResInfo> &infoVector,
                    std::size_t *resIndex, std::size_t *rateIndex)
{
	ResInfo info;
	info.w = mode.width;
	info.h = mode.height;
	short const rate = getRate(mode);

	std::vector<ResInfo>::iterator it =
		std::lower_bound(infoVector.begin(), infoVector.end(),
		                 info, std::greater<ResInfo>());
	if (it == infoVector.end() || *it != info)
		it = infoVector.insert(it, info);

	std::vector<short>::iterator rateIt =
		std::lower_bound(it->rates.begin(), it->rates.end(),
		                 rate, std::greater<short>());
	if (rateIt == it->rates.end() || *rateIt != rate)
		rateIt = it->rates.insert(rateIt, rate);

	if (resIndex)
		*resIndex = std::distance(infoVector.begin(), it);
	if (rateIndex)
		*rateIndex = std::distance(it->rates.begin(), rateIt);
}

static XRRModeInfo * getModeInfo(XRRScreenResources const *resources, RRMode const mode) {
	for (int m = 0; m < resources->nmode; ++m) {
		if (resources->modes[m].id == mode)
			return resources->modes + m;
	}

	return 0;
}

static bool isGood(RRMode const mode, std::vector<XRROutputInfo *> const &outputInfos) {
	for (std::size_t o = 0; o < outputInfos.size(); ++o) {
		RRMode *modesEnd = outputInfos[o]->modes + outputInfos[o]->nmode;
		if (std::find(outputInfos[o]->modes, modesEnd, mode) == modesEnd)
			return false;
	}

	return true;
}

namespace {

class OutputInfos : Uncopyable {
	std::vector<XRROutputInfo *> v;
public:
	OutputInfos(XRRScreenResources *resources, XRRCrtcInfo const *crtcInfo);
	~OutputInfos();
	std::vector<XRROutputInfo *> const & get() { return v; }
};

OutputInfos::OutputInfos(XRRScreenResources *const resources, XRRCrtcInfo const *const crtcInfo)
: v(crtcInfo ? crtcInfo->noutput : 0)
{
	for (std::size_t i = 0; i < v.size();) {
		v[i] = XRRGetOutputInfo(QX11Info::display(), resources, crtcInfo->outputs[i]);
		if (v[i])
			++i;
		else
			v.pop_back();
	}
}

OutputInfos::~OutputInfos() {
	std::for_each(v.begin(), v.end(), XRRFreeOutputInfo);
}

static RRMode getMode(XRRScreenResources *const resources,
                      XRRCrtcInfo const *const crtcInfo,
                      RRMode const preferred,
                      ResInfo const &info,
                      unsigned const rate)
{
	if (XRRModeInfo *curMode = getModeInfo(resources, crtcInfo->mode)) {
		if (curMode->width == info.w
				&& curMode->height == info.h
				&& getRate(*curMode) == rate) {
			return crtcInfo->mode;
		}
	}
	if (XRRModeInfo *prefMode = getModeInfo(resources, preferred)) {
		if (prefMode->width == info.w
				&& prefMode->height == info.h
				&& getRate(*prefMode) == rate) {
			return preferred;
		}
	}

	OutputInfos outputInfos(resources, crtcInfo);
	for (int m = 0; m < resources->nmode; ++m) {
		if (resources->modes[m].width == info.w
				&& resources->modes[m].height == info.h
				&& getRate(resources->modes[m]) == rate
				&& isGood(resources->modes[m].id, outputInfos.get())) {
			return resources->modes[m].id;
		}
	}

	return crtcInfo->mode;
}

} // anon ns

struct XRandR12Toggler::XRRDeleter {
	static void del(XRRCrtcInfo *c) { if (c) { XRRFreeCrtcInfo(c); } }
	static void del(XRRScreenResources *r) { if (r) { XRRFreeScreenResources(r); } }
};

bool XRandR12Toggler::isUsable() {
	int unused = 0;
	int major = 1;
	int minor = 1;
	return XRRQueryExtension(QX11Info::display(), &unused, &unused)
	    && XRRQueryVersion(QX11Info::display(), &major, &minor)
	    && major == 1 && minor >= 2;
}

XRandR12Toggler::XRandR12Toggler()
: originalMode_(0)
, resources_(XRRGetScreenResources(QX11Info::display(), QX11Info::appRootWindow()))
, widgetScreen_(0)
, isFull_(false)
{
	if (!resources_)
		return;

	infoVector_.resize(resources_->ncrtc);
	fullResIndex_.resize(resources_->ncrtc);
	fullRateIndex_.resize(resources_->ncrtc);

	for (int c = 0; c < resources_->ncrtc; ++c) {
		XRRModeInfo *curMode = 0;
		scoped_ptr<XRRCrtcInfo, XRRDeleter> const crtcInfo(
			XRRGetCrtcInfo(QX11Info::display(), resources_.get(),
			               resources_->crtcs[c]));
		if (crtcInfo && (curMode = getModeInfo(resources_.get(), crtcInfo->mode))) {
			OutputInfos outputInfos(resources_.get(), crtcInfo.get());
			for (int m = 0; m < resources_->nmode; ++m) {
				if (resources_->modes[m].width <= curMode->width
						&& resources_->modes[m].height <= curMode->height
						&& isGood(resources_->modes[m].id, outputInfos.get())) {
					addMode(resources_->modes[m], infoVector_[c], 0, 0);
				}
			}
		}

		std::size_t resIndex = 0;
		std::size_t rateIndex = 0;
		if (curMode)
			addMode(*curMode, infoVector_[c], &resIndex, &rateIndex);

		fullResIndex_[c] = resIndex;
		fullRateIndex_[c] = rateIndex;
	}
}

QRect const XRandR12Toggler::fullScreenRect(QWidget const *const w) const {
	if (resources_) {
		scoped_ptr<XRRCrtcInfo, XRRDeleter> crtcInfo(
			XRRGetCrtcInfo(QX11Info::display(), resources_.get(),
			               resources_->crtcs[widgetScreen_]));
		if (crtcInfo)
			return QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height);
	}

	return FullModeToggler::fullScreenRect(w);
}

void XRandR12Toggler::setScreen(QWidget const *const widget) {
	std::size_t n = widgetScreen_;
	if (resources_) {
		int maxIntersect = 0;
		QRect const wrect = widget->geometry();
		for (int c = 0; c < resources_->ncrtc; ++c) {
			scoped_ptr<XRRCrtcInfo, XRRDeleter> const crtcInfo(
				XRRGetCrtcInfo(QX11Info::display(), resources_.get(),
				               resources_->crtcs[c]));
			if (crtcInfo) {
				QRect const r =
					QRect(crtcInfo->x, crtcInfo->y,
					      crtcInfo->width, crtcInfo->height).intersected(wrect);
				int const area = r.width() * r.height();
				if (area > maxIntersect) {
					maxIntersect = area;
					n = c;
					if (r == wrect)
						break;
				}
			}
		}
	}

	if (n != widgetScreen_ && n < infoVector_.size()) {
		if (isFullMode()) {
			setFullMode(false);
			widgetScreen_ = n;
			setFullMode(true);
		} else {
			widgetScreen_ = n;
			emitRate();
		}
	}
}

void XRandR12Toggler::setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex) {
	fullResIndex_[screen] = resIndex;
	fullRateIndex_[screen] = rateIndex;
	if (isFullMode() && screen == widgetScreen_)
		setFullMode(true);
}

void XRandR12Toggler::setFullMode(bool const enable) {
	bool const oldIsFull = isFull_;
	isFull_ = enable;
	if (!resources_)
		return;

	scoped_ptr<XRRCrtcInfo, XRRDeleter> const crtcInfo(
		XRRGetCrtcInfo(QX11Info::display(), resources_.get(),
		               resources_->crtcs[widgetScreen_]));
	if (!crtcInfo)
		return;

	RRMode mode = crtcInfo->mode;
	if (enable) {
		if (!infoVector_[widgetScreen_].empty()) {
			ResInfo const &resInfo =
				infoVector_[widgetScreen_][fullResIndex_[widgetScreen_]];
			mode = getMode(resources_.get(), crtcInfo.get(),
			               oldIsFull ? originalMode_ : crtcInfo->mode,
			               resInfo, resInfo.rates[fullRateIndex_[widgetScreen_]]);
		}

		if (!oldIsFull)
			originalMode_ = crtcInfo->mode;
	} else if (oldIsFull)
		mode = originalMode_;

	if (mode != crtcInfo->mode) {
		if (XRRSetCrtcConfig(QX11Info::display(), resources_.get(),
		                     resources_->crtcs[widgetScreen_],
		                     CurrentTime, crtcInfo->x, crtcInfo->y, mode,
		                     crtcInfo->rotation, crtcInfo->outputs,
		                     crtcInfo->noutput) == Success) {
			short prevRate = 0;
			if (XRRModeInfo *info = getModeInfo(resources_.get(), crtcInfo->mode))
				prevRate = getRate(*info);

			short rate = 0;
			if (XRRModeInfo *info = getModeInfo(resources_.get(), mode))
				rate = getRate(*info);

			if (rate != prevRate)
				emit rateChange(rate);
		}
	}
}

void XRandR12Toggler::emitRate() {
	scoped_ptr<XRRCrtcInfo, XRRDeleter> const crtcInfo(
		XRRGetCrtcInfo(QX11Info::display(), resources_.get(),
		               resources_->crtcs[widgetScreen_]));
	short rate = 0;
	if (crtcInfo) {
		if (XRRModeInfo *curMode = getModeInfo(resources_.get(), crtcInfo->mode))
			rate = getRate(*curMode);
	}

	emit rateChange(rate);
}

QString const XRandR12Toggler::screenName(std::size_t const screen) const {
	scoped_ptr<XRRCrtcInfo, XRRDeleter> const crtcInfo(
		XRRGetCrtcInfo(QX11Info::display(), resources_.get(), resources_->crtcs[screen]));
	OutputInfos outputInfos(resources_.get(), crtcInfo.get());
	if (!outputInfos.get().empty())
		return QString(outputInfos.get()[0]->name);

	return FullModeToggler::screenName(screen);
}
