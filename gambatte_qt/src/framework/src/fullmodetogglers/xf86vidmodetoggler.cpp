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

#include "xf86vidmodetoggler.h"
#include <QX11Info>
#include <X11/extensions/Xinerama.h>
#include <algorithm>
#include <cstring>
#include <functional>

bool Xf86VidModeToggler::isUsable() {
	int unused;
	if (!XF86VidModeQueryExtension(QX11Info::display(), &unused, &unused))
		return false;

	XF86VidModeSetClientVersion(QX11Info::display());
	if (!XF86VidModeQueryVersion(QX11Info::display(), &unused, &unused))
		return false;

	int num = 0;
	if (XineramaQueryVersion(QX11Info::display(), &unused, &unused)) {
		if (XineramaScreenInfo *info =
				XineramaQueryScreens(QX11Info::display(), &num)) {
			XFree(info);
		}
	}

	return num <= 1;
}

static bool operator!=(ResInfo const &l, ResInfo const &r) {
	return l.w != r.w || l.h != r.h;
}

static bool operator>(ResInfo const &l, ResInfo const &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static unsigned getRate(unsigned dotclock, unsigned long htotal, unsigned long vtotal) {
	if (unsigned long pixels = htotal * vtotal)
		return (dotclock * 10000ull + (pixels >> 1)) / pixels;

	return 0;
}

static unsigned getRate(XF86VidModeModeInfo const &mode) {
	return getRate(mode.dotclock, mode.htotal, mode.vtotal);
}

static void addMode(XF86VidModeModeInfo const *mode,
                    std::vector<ResInfo> &infoVector,
                    std::size_t *resIndex, std::size_t *rateIndex)
{
	ResInfo info;
	info.w = mode->hdisplay;
	info.h = mode->vdisplay;
	short const rate = getRate(*mode);

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

Xf86VidModeToggler::Xf86VidModeToggler(WId winId)
: modesinfo_()
, modecount_(0)
, fullResIndex_(0)
, fullRateIndex_(0)
, originalvportx_(0)
, originalvporty_(0)
, winId_(winId)
, isFull_(false)
{
	XF86VidModeGetAllModeLines(QX11Info::display(), QX11Info::appScreen(),
	                           &modecount_, &modesinfo_);
	if (modecount_) {
		for (int i = 1; i < modecount_; ++i)
			addMode(modesinfo_[i], infoVector_, 0, 0);

		originalMode_ = *modesinfo_[0];
		addMode(modesinfo_[0], infoVector_, &fullResIndex_, &fullRateIndex_);
	}
}

Xf86VidModeToggler::~Xf86VidModeToggler() {
	if (modesinfo_)
		XFree(modesinfo_);
}

QRect const Xf86VidModeToggler::fullScreenRect(QWidget const *wdgt) const {
	int dotclock = 0;
	XF86VidModeModeLine modeline;
	if (XF86VidModeGetModeLine(QX11Info::display(),
	                           QX11Info::appScreen(),
	                           &dotclock, &modeline)) {
		int x = 0;
		int y = 0;
// 		XF86VidModeGetViewPort(QX11Info::display(), QX11Info::appScreen(), &x, &y);
		return QRect(x, y, modeline.hdisplay, modeline.vdisplay);
	}

	return wdgt->geometry();
}

void Xf86VidModeToggler::setMode(std::size_t /*screen*/, std::size_t resIndex, std::size_t rateIndex) {
	fullResIndex_ = resIndex;
	fullRateIndex_ = rateIndex;
	if (isFullMode())
		setFullMode(true);
}

void Xf86VidModeToggler::setFullMode(bool const enable) {
	XF86VidModeModeInfo curMode;
	bool curModeValid;

	{
		int dotclock = 0;
		XF86VidModeModeLine modeline;
		curModeValid = XF86VidModeGetModeLine(QX11Info::display(), QX11Info::appScreen(),
		                                      &dotclock, &modeline);
		curMode.dotclock = dotclock;
		std::memcpy(reinterpret_cast<char *>(&curMode) + sizeof curMode.dotclock,
		            reinterpret_cast<char *>(&modeline),
		            std::min(sizeof modeline, sizeof curMode - sizeof curMode.dotclock));
	}

	XF86VidModeModeInfo *newMode = 0;
	int vportx = 0;
	int vporty = 0;
	if (enable) {
		if (!isFull_) {
			if (curModeValid)
				originalMode_ = curMode;

			XF86VidModeGetViewPort(QX11Info::display(), QX11Info::appScreen(),
			                       &originalvportx_, &originalvporty_);
		}

		ResInfo const &resinfo = infoVector_[fullResIndex_];
		for (int i = 0; i < modecount_; ++i) {
			if (modesinfo_[i]->hdisplay == resinfo.w
				&& modesinfo_[i]->vdisplay == resinfo.h
				&& static_cast<int>(getRate(*modesinfo_[i]))
				   == resinfo.rates[fullRateIndex_]) {
				newMode = modesinfo_[i];
				break;
			}
		}
	} else {
		if (modecount_)
			newMode = &originalMode_;

		vportx = originalvportx_;
		vporty = originalvporty_;
	}

	if (newMode) {
		if (XF86VidModeSwitchToMode(QX11Info::display(), QX11Info::appScreen(), newMode)) {
			unsigned newRate = getRate(*newMode);
			if (!curModeValid || newRate != getRate(curMode))
				emit rateChange(newRate);
		}
	}

	XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), vportx, vporty);
	XF86VidModeLockModeSwitch(QX11Info::display(), QX11Info::appScreen(), enable);
	if (enable) {
		XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
		             GrabModeAsync, GrabModeAsync, winId_, None, CurrentTime);
		QCoreApplication::instance()->installEventFilter(this);
	} else {
		XUngrabPointer(QX11Info::display(), CurrentTime);
		QCoreApplication::instance()->removeEventFilter(this);
	}

// 	XSync(QX11Info::display(), False);
	isFull_ = enable;
}

bool Xf86VidModeToggler::eventFilter(QObject *obj, QEvent *ev) {
	/*if (obj->isWidgetType() && static_cast<QWidget*>(obj)->winId() == winId_) {
		if (ev->type() == 8) {
			XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), 0, 0);
			XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
			             GrabModeAsync, GrabModeAsync,
			             winId_, None, CurrentTime);
		} else if (ev->type() == 9)
			XUngrabPointer(QX11Info::display(), CurrentTime);
	}*/

	if (ev->type() == QEvent::Show || ev->type() == QEvent::Hide) {
		XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
		             GrabModeAsync, GrabModeAsync, winId_, None, CurrentTime);
	} else if (ev->type() == QEvent::Enter
			&& obj->isWidgetType()
			&& static_cast<QWidget *>(obj)->winId() == winId_) {
		XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), 0, 0);
	}/* else if (ev->type() == QEvent::Leave && obj->isWidgetType()
			&& static_cast<QWidget *>(obj)->winId() == winId_ && !qApp->activeWindow()) {
		XUngrabPointer(QX11Info::display(), CurrentTime);
	}*/

	return false;
}

void Xf86VidModeToggler::emitRate() {
	int rate = 0;
	int dotclock = 0;
	XF86VidModeModeLine modeline;
	if (XF86VidModeGetModeLine(QX11Info::display(),
	                           QX11Info::appScreen(),
	                           &dotclock, &modeline)) {
		rate = getRate(dotclock, modeline.htotal, modeline.vtotal);
	}

	emit rateChange(rate);
}
