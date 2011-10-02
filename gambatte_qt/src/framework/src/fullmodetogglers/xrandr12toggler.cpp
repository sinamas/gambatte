/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "xrandr12toggler.h"
#include "uncopyable.h"
#include <functional>
#include <algorithm>
#include <QWidget>
#include <QX11Info>

static inline bool operator!=(const ResInfo &l, const ResInfo &r) {
	return l.w != r.w || l.h != r.h;
}

static inline bool operator>(const ResInfo &l, const ResInfo &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static unsigned getRate(const unsigned long dotclock, const unsigned htotal, const unsigned vtotal) {
	if (unsigned long pixels = htotal * vtotal)
		return (dotclock * 10ull + (pixels >> 1)) / pixels;
	
	return 0;
}

static unsigned getRate(const XRRModeInfo &mode) {
	return getRate(mode.dotClock, mode.hTotal, mode.vTotal);
}

static void addMode(const XRRModeInfo &mode, std::vector<ResInfo> &infoVector, unsigned *resIndex, unsigned *rateIndex) {
	ResInfo info;
	short rate;
	
	info.w = mode.width;
	info.h = mode.height;
	rate = getRate(mode);
	
	std::vector<ResInfo>::iterator it = std::lower_bound(infoVector.begin(), infoVector.end(), info, std::greater<ResInfo>());
	
	if (it == infoVector.end() || *it != info)
		it = infoVector.insert(it, info);
	
	std::vector<short>::iterator rateIt = std::lower_bound(it->rates.begin(), it->rates.end(), rate, std::greater<short>());
	
	if (rateIt == it->rates.end() || *rateIt != rate)
		rateIt = it->rates.insert(rateIt, rate);
	
	if (resIndex)
		*resIndex = std::distance(infoVector.begin(), it);
	
	if (rateIndex)
		*rateIndex = std::distance(it->rates.begin(), rateIt);
}

static XRRModeInfo* getModeInfo(const XRRScreenResources *const resources, const RRMode mode) {
	for (int m = 0; m < resources->nmode; ++m) {
		if (resources->modes[m].id == mode)
			return resources->modes + m;
	}
	
	return NULL;
}

static bool isGood(const RRMode mode, const std::vector<XRROutputInfo*> &outputInfos) {
	for (std::size_t o = 0; o < outputInfos.size(); ++o) {
		RRMode *const modesEnd = outputInfos[o]->modes + outputInfos[o]->nmode;
		
		if (std::find(outputInfos[o]->modes, modesEnd, mode) == modesEnd)
			return false;
	}

	return true;
}

namespace {

class OutputInfos : Uncopyable {
	std::vector<XRROutputInfo*> v;
public:
	OutputInfos(XRRScreenResources *resources, const XRRCrtcInfo *crtcInfo);
	~OutputInfos();
	const std::vector<XRROutputInfo*>& get() { return v; }
};

OutputInfos::OutputInfos(XRRScreenResources *const resources, const XRRCrtcInfo *const crtcInfo)
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
	for (std::size_t i = 0; i < v.size(); ++i)
		XRRFreeOutputInfo(v[i]);
}

class CrtcInfoPtr : Uncopyable {
	XRRCrtcInfo *const crtcInfo;
	
public:
	CrtcInfoPtr(XRRScreenResources *const resources, const RRCrtc crtc)
	: crtcInfo(XRRGetCrtcInfo(QX11Info::display(), resources, crtc)) {}
	~CrtcInfoPtr() { if (crtcInfo) XRRFreeCrtcInfo(crtcInfo); }
	operator XRRCrtcInfo*() { return crtcInfo; }
	operator const XRRCrtcInfo*() const { return crtcInfo; }
	XRRCrtcInfo* operator->() { return crtcInfo; }
	const XRRCrtcInfo* operator->() const { return crtcInfo; }
};

static RRMode getMode(XRRScreenResources *const resources, const XRRCrtcInfo *const crtcInfo,
			const RRMode preferred, const ResInfo &info, const short rate) {
	if (XRRModeInfo *curMode = getModeInfo(resources, crtcInfo->mode)) {
		if (curMode->width == info.w && curMode->height == info.h && getRate(*curMode) == static_cast<unsigned>(rate))
			return crtcInfo->mode;
	}
	
	if (XRRModeInfo *prefMode = getModeInfo(resources, preferred)) {
		if (prefMode->width == info.w && prefMode->height == info.h && getRate(*prefMode) == static_cast<unsigned>(rate))
			return preferred;
	}
	
	OutputInfos outputInfos(resources, crtcInfo);
	
	for (int m = 0; m < resources->nmode; ++m) {
		if (resources->modes[m].width == info.w && resources->modes[m].height == info.h && getRate(resources->modes[m]) == static_cast<unsigned>(rate)) {
			if (isGood(resources->modes[m].id, outputInfos.get()))
				return resources->modes[m].id;
		}
	}
	
	return crtcInfo->mode;
}

}

bool XRandR12Toggler::isUsable() {
	int unused = 0;
	int major = 1;
	int minor = 1;
	
	return XRRQueryExtension(QX11Info::display(), &unused, &unused) &&
			XRRQueryVersion(QX11Info::display(), &major, &minor) &&
			major == 1 && minor >= 2;
}

XRandR12Toggler::XRandR12Toggler() :
originalMode(0),
resources(NULL),
widgetScreen(0),
isFull(false)
{
	resources = XRRGetScreenResources(QX11Info::display(), QX11Info::appRootWindow());
	
	if (resources) {
		infoVector.resize(resources->ncrtc);
		fullResIndex.resize(resources->ncrtc);
		fullRateIndex.resize(resources->ncrtc);
		
		for (int c = 0; c < resources->ncrtc; ++c) {
			XRRModeInfo *curMode = NULL;
			
			{
				CrtcInfoPtr crtcInfo(resources, resources->crtcs[c]);
				
				if (crtcInfo && (curMode = getModeInfo(resources, crtcInfo->mode))) {
					OutputInfos outputInfos(resources, crtcInfo);
					
					for (int m = 0; m < resources->nmode; ++m) {
						if (resources->modes[m].width <= curMode->width && resources->modes[m].height <= curMode->height) {
							if (isGood(resources->modes[m].id, outputInfos.get()))
								addMode(resources->modes[m], infoVector[c], NULL, NULL);
						}
					}
				}
			}
			
			unsigned resIndex = 0;
			unsigned rateIndex = 0;
			
			if (curMode)
				addMode(*curMode, infoVector[c], &resIndex, &rateIndex);
			
			fullResIndex[c] = resIndex;
			fullRateIndex[c] = rateIndex;
		}
	}
}

XRandR12Toggler::~XRandR12Toggler() {
	if (resources)
		XRRFreeScreenResources(resources);
}

const QRect XRandR12Toggler::fullScreenRect(const QWidget *const w) const {
	if (resources) {
		CrtcInfoPtr crtcInfo(resources, resources->crtcs[widgetScreen]);
		
		if (crtcInfo)
			return QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height);
	}
	
	return FullModeToggler::fullScreenRect(w);
}

void XRandR12Toggler::setScreen(const QWidget *const widget) {
	unsigned n = widgetScreen;
	
	if (resources) {
		int maxIntersect = 0;
		const QRect &wrect = widget->geometry();
		
		for (int c = 0; c < resources->ncrtc; ++c) {
			CrtcInfoPtr crtcInfo(resources, resources->crtcs[c]);
			
			if (crtcInfo) {
				const QRect &r = QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height).intersected(wrect);
				const int area = r.width() * r.height();
				
				if (area > maxIntersect) {
					maxIntersect = area;
					n = c;
					
					if (r == wrect)
						break;
				}
			}
		}
	}
	
	if (n != widgetScreen && n < infoVector.size()) {
		if (isFullMode()) {
			setFullMode(false);
			widgetScreen = n;
			setFullMode(true);
		} else {
			widgetScreen = n;
			emitRate();
		}
	}
}

void XRandR12Toggler::setMode(const unsigned screen, const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex[screen] = resIndex;
	fullRateIndex[screen] = rateIndex;
	
	if (isFullMode() && screen == widgetScreen)
		setFullMode(true);
}

void XRandR12Toggler::setFullMode(const bool enable) {
	if (resources) {
		CrtcInfoPtr crtcInfo(resources, resources->crtcs[widgetScreen]);
		
		if (crtcInfo) {
			RRMode mode = crtcInfo->mode;
			
			if (enable) {
				if (!infoVector[widgetScreen].empty()) {
					mode = getMode(resources, crtcInfo, isFull ? originalMode : crtcInfo->mode,
							infoVector[widgetScreen][fullResIndex[widgetScreen]],
							infoVector[widgetScreen][fullResIndex[widgetScreen]].rates[fullRateIndex[widgetScreen]]);
				}
				
				if (!isFull)
					originalMode = crtcInfo->mode;
			} else if (isFull)
				mode = originalMode;
			
			if (mode != crtcInfo->mode) {
				if (XRRSetCrtcConfig(QX11Info::display(), resources, resources->crtcs[widgetScreen],
						CurrentTime, crtcInfo->x, crtcInfo->y, mode, crtcInfo->rotation,
						crtcInfo->outputs, crtcInfo->noutput) == Success) {
					const short rate = infoVector[widgetScreen][fullResIndex[widgetScreen]].rates[fullRateIndex[widgetScreen]];
					short prevRate = -1;
					
					if (XRRModeInfo *curMode = getModeInfo(resources, crtcInfo->mode))
						prevRate = getRate(*curMode);
					
					if (rate != prevRate)
						emit rateChange(rate);
				}
			}
		}
	}
	
	isFull = enable;
}

void XRandR12Toggler::emitRate() {
	const CrtcInfoPtr crtcInfo(resources, resources->crtcs[widgetScreen]);
	short rate = 0;
	
	if (crtcInfo) {
		if (XRRModeInfo *curMode = getModeInfo(resources, crtcInfo->mode))
			rate = getRate(*curMode);
	}
	
	emit rateChange(rate);
}

const QString XRandR12Toggler::screenName(const unsigned screen) const {
	OutputInfos outputInfos(resources, CrtcInfoPtr(resources, resources->crtcs[screen]));
	
	if (!outputInfos.get().empty())
		return QString(outputInfos.get()[0]->name);
	
	return FullModeToggler::screenName(screen);
}
