/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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
#include "xf86vidmodetoggler.h"
#include <QX11Info>
#include <cstring>
#include <algorithm>
#include <functional>
#include <X11/extensions/Xinerama.h>

bool Xf86VidModeToggler::isUsable() {
	int unused;
	
	if (XF86VidModeQueryExtension(QX11Info::display(), &unused, &unused)) {
		XF86VidModeSetClientVersion(QX11Info::display());
		
		if (XF86VidModeQueryVersion(QX11Info::display(), &unused, &unused)) {
			if (XineramaQueryVersion(QX11Info::display(), &unused, &unused)) {
				int number = 0;
				
				if (XineramaScreenInfo *info = XineramaQueryScreens(QX11Info::display(), &number))
					XFree(info);
				
				if (number > 1)
					return false;
			}
			
			return true;
		}
	}
	
	return false;
}

static inline bool operator!=(const ResInfo &l, const ResInfo &r) {
	return l.w != r.w || l.h != r.h;
}

static inline bool operator>(const ResInfo &l, const ResInfo &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static unsigned getRate(const unsigned dotclock, const unsigned htotal, const unsigned vtotal) {
	if (unsigned long pixels = htotal * vtotal)
		return (dotclock * 10000ull + (pixels >> 1)) / pixels;
	
	return 0;
}

static unsigned getRate(const XF86VidModeModeInfo &mode) {
	return getRate(mode.dotclock, mode.htotal, mode.vtotal);
}

static void addMode(XF86VidModeModeInfo *mode, std::vector<ResInfo> &infoVector, unsigned *resIndex, unsigned *rateIndex) {
	ResInfo info;
	short rate;
	
	info.w = mode->hdisplay;
	info.h = mode->vdisplay;
	rate = getRate(*mode);
	
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

Xf86VidModeToggler::Xf86VidModeToggler(WId winId) :
modesinfo(NULL),
modecount(0),
fullResIndex(0),
fullRateIndex(0),
originalvportx(0),
originalvporty(0),
winId(winId),
isFull(false)
{
	XF86VidModeGetAllModeLines(QX11Info::display(), QX11Info::appScreen(), &modecount, &modesinfo);
	
	if (modecount) {
		for (int i = 1; i < modecount; ++i) {
			addMode(modesinfo[i], infoVector, NULL, NULL);
		}
		
		originalMode = *modesinfo[0];
		addMode(modesinfo[0], infoVector, &fullResIndex, &fullRateIndex);
	}
}

Xf86VidModeToggler::~Xf86VidModeToggler() {
	setFullMode(false);
	
	if (modesinfo)
		XFree(modesinfo);
}

const QRect Xf86VidModeToggler::fullScreenRect(const QWidget *wdgt) const {
	int dotclock = 0;
	XF86VidModeModeLine modeline;
	
	if (XF86VidModeGetModeLine(QX11Info::display(), QX11Info::appScreen(), &dotclock, &modeline)) {
		int x = 0;
		int y = 0;
		
// 		XF86VidModeGetViewPort(QX11Info::display(), QX11Info::appScreen(), &x, &y);
		
		return QRect(x, y, modeline.hdisplay, modeline.vdisplay);
	}
	
	return wdgt->geometry();
}

void Xf86VidModeToggler::setMode(unsigned /*screen*/, const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex = resIndex;
	fullRateIndex = rateIndex;
	
	if (isFullMode())
		setFullMode(true);
}

void Xf86VidModeToggler::setFullMode(const bool enable) {
	XF86VidModeModeInfo curMode;
	bool curModeValid;
	
	{
		int dotclock = 0;
		XF86VidModeModeLine modeline;
		curModeValid = XF86VidModeGetModeLine(QX11Info::display(), QX11Info::appScreen(), &dotclock, &modeline);
		curMode.dotclock = dotclock;
		
		std::memcpy(reinterpret_cast<char*>(&curMode) + sizeof(curMode.dotclock),
		            reinterpret_cast<char*>(&modeline),
		            std::min(sizeof(XF86VidModeModeLine), sizeof(XF86VidModeModeInfo) - sizeof(curMode.dotclock)));
	}
	
	XF86VidModeModeInfo *newMode = NULL;
	int vportx = 0;
	int vporty = 0;
	
	if (enable) {
		if (!isFull) {
			if (curModeValid)
				originalMode = curMode;
			
			XF86VidModeGetViewPort(QX11Info::display(), QX11Info::appScreen(), &originalvportx, &originalvporty);
		}
		
		int i = 0;
		
		while (i < modecount &&
				modesinfo[i]->hdisplay != infoVector[fullResIndex].w ||
				modesinfo[i]->vdisplay != infoVector[fullResIndex].h ||
				getRate(*modesinfo[i]) != static_cast<unsigned>(infoVector[fullResIndex].rates[fullRateIndex])) {
			++i;
		}
		
		if (i != modecount) {
			newMode = modesinfo[i];
		}
	} else {
		if (modecount) {
			newMode = &originalMode;
		}
		
		vportx = originalvportx;
		vporty = originalvporty;
	}
	
	if (newMode) {
		if (XF86VidModeSwitchToMode(QX11Info::display(), QX11Info::appScreen(), newMode)) {
			const unsigned newRate = getRate(*newMode);
			
			if (!curModeValid || newRate != getRate(curMode))
				emit rateChange(newRate);
		}
	}
	
	XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), vportx, vporty);
	XF86VidModeLockModeSwitch(QX11Info::display(), QX11Info::appScreen(), enable);
	
	if (enable) {
		XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
				GrabModeAsync, GrabModeAsync,
				winId, None, CurrentTime);
		QCoreApplication::instance()->installEventFilter(this);
	} else {
		XUngrabPointer(QX11Info::display(), CurrentTime);
		QCoreApplication::instance()->removeEventFilter(this);
	}
	
// 	XSync(QX11Info::display(), False);
	
	isFull = enable;
}

bool Xf86VidModeToggler::eventFilter(QObject *obj, QEvent *ev) {
	/*if (obj->isWidgetType() && static_cast<QWidget*>(obj)->winId() == winId) {
		if (ev->type() == 8) {
			XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), 0, 0);
			XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
			             GrabModeAsync, GrabModeAsync,
			             winId, None, CurrentTime);
		} else if (ev->type() == 9)
			XUngrabPointer(QX11Info::display(), CurrentTime);
	}*/
	
	if (ev->type() == QEvent::Show || ev->type() == QEvent::Hide) {
		XGrabPointer(QX11Info::display(), QX11Info::appRootWindow(), True, 0,
		             GrabModeAsync, GrabModeAsync,
		             winId, None, CurrentTime);
	} else if (ev->type() == QEvent::Enter && obj->isWidgetType() && static_cast<QWidget*>(obj)->winId() == winId) {
		XF86VidModeSetViewPort(QX11Info::display(), QX11Info::appScreen(), 0, 0);
	}/* else if (ev->type() == QEvent::Leave && obj->isWidgetType() && static_cast<QWidget*>(obj)->winId() == winId && !qApp->activeWindow()) {
		XUngrabPointer(QX11Info::display(), CurrentTime);
	}*/
	
	return false;
}

void Xf86VidModeToggler::emitRate() {
	int rate = 0;
	int dotclock = 0;
	XF86VidModeModeLine modeline;
	
	if (XF86VidModeGetModeLine(QX11Info::display(), QX11Info::appScreen(), &dotclock, &modeline)) {
		rate = getRate(dotclock, modeline.htotal, modeline.vtotal);
	}
	
	emit rateChange(rate);
}
