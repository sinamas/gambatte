/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#include "gditoggler.h"
#include "../gdisettings.h"
#include "uncopyable.h"
#include <QApplication>
#include <QDesktopWidget>
#include <algorithm>
#include <functional>
#include <windows.h>

static inline bool operator!=(const ResInfo &l, const ResInfo &r) {
	return l.w != r.w || l.h != r.h;
}

static inline bool operator>(const ResInfo &l, const ResInfo &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static void addMode(const DEVMODE &devmode, std::vector<ResInfo> &infoVector, unsigned *resIndex, unsigned *rateIndex) {
	ResInfo info;
	short rate;
	
	info.w = devmode.dmPelsWidth;
	info.h = devmode.dmPelsHeight;
	rate = devmode.dmDisplayFrequency * 10;
	
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

class GdiToggler::MultiMon : Uncopyable {
	HMONITOR *devMonitors;
	unsigned numDevs;
	
	HMONITOR monitor(int screen) const { return devMonitors ? devMonitors[screen] : NULL; }
	
public:
	MultiMon() : devMonitors(NULL), numDevs(1) {
		if (gdiSettings.monitorFromPoint) {
			numDevs = QApplication::desktop()->numScreens();
			devMonitors = new HMONITOR[numDevs];
			
			for (unsigned i = 0; i < numDevs; ++i) {
				const QPoint &qpoint = QApplication::desktop()->screenGeometry(i).center();
				POINT point = { x: qpoint.x(), y: qpoint.y() };
				devMonitors[i] = gdiSettings.monitorFromPoint(point, GdiSettings::MON_DEFAULTTONEAREST);
			}
		}
	}
	
	~MultiMon() {
		delete []devMonitors;
	}
	
	unsigned numScreens() const { return numDevs; }
	
	BOOL enumDisplaySettings(unsigned screen, DWORD iModeNum, LPDEVMODE devmode) const {
		return gdiSettings.enumDisplaySettings(monitor(screen), iModeNum, devmode);
	}
	
	LONG changeDisplaySettings(unsigned screen, LPDEVMODE devmode, DWORD dwflags) const {
		return gdiSettings.changeDisplaySettings(monitor(screen), devmode, dwflags);
	}
};

GdiToggler::GdiToggler() :
mon(new MultiMon),
widgetScreen(0),
isFull(false)
{
	infoVector.resize(mon->numScreens());
	fullResIndex.resize(mon->numScreens());
	fullRateIndex.resize(mon->numScreens());
	
	DEVMODE devmode;
	devmode.dmSize = sizeof devmode;
	devmode.dmDriverExtra = 0;
	
	for (unsigned i = 0; i < mon->numScreens(); ++i) {
		mon->enumDisplaySettings(i, ENUM_CURRENT_SETTINGS, &devmode);
		
		const unsigned bpp = devmode.dmBitsPerPel;
		
		int n = 0;
		
		while (mon->enumDisplaySettings(i, n++, &devmode)) {
			if (devmode.dmBitsPerPel == bpp)
				addMode(devmode, infoVector[i], NULL, NULL);
		}
	}
	
	for (unsigned i = 0; i < mon->numScreens(); ++i) {
		mon->enumDisplaySettings(i, ENUM_CURRENT_SETTINGS, &devmode);
		addMode(devmode, infoVector[i], &fullResIndex[i], &fullRateIndex[i]);
	}
}

GdiToggler::~GdiToggler() {
	setFullMode(false);
	delete mon;
}

/*const QRect GdiToggler::fullScreenRect(const QWidget *wdgt) const {
	return QApplication::desktop()->screenGeometry(wdgt);
}*/

void GdiToggler::setScreen(const QWidget *widget) {
	unsigned n = QApplication::desktop()->screenNumber(widget);
	
	if (n != widgetScreen && n < mon->numScreens()) {
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

void GdiToggler::setMode(const unsigned screen, const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex[screen] = resIndex;
	fullRateIndex[screen] = rateIndex;
	
	if (isFullMode() && screen == widgetScreen)
		setFullMode(true);
}

void GdiToggler::setFullMode(const bool enable) {
	DEVMODE devmode;
	devmode.dmSize = sizeof devmode;
	devmode.dmDriverExtra = 0;
	
	mon->enumDisplaySettings(widgetScreen, ENUM_CURRENT_SETTINGS, &devmode);
	const unsigned currentWidth = devmode.dmPelsWidth;
	const unsigned currentHeight = devmode.dmPelsHeight;
	const unsigned currentRate = devmode.dmDisplayFrequency;
	
	if (enable) {
		const ResInfo &info = infoVector[widgetScreen][fullResIndex[widgetScreen]];
		devmode.dmPelsWidth = info.w;
		devmode.dmPelsHeight = info.h;
		devmode.dmDisplayFrequency = info.rates[fullRateIndex[widgetScreen]] / 10;
	} else if (isFull) {
		mon->enumDisplaySettings(widgetScreen, ENUM_REGISTRY_SETTINGS, &devmode);
	}
	
	if (devmode.dmPelsWidth != currentWidth || devmode.dmPelsHeight != currentHeight || devmode.dmDisplayFrequency != currentRate) {
		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		mon->changeDisplaySettings(widgetScreen, &devmode, enable ? CDS_FULLSCREEN : 0);
		
		if (devmode.dmDisplayFrequency != currentRate)
			emit rateChange(devmode.dmDisplayFrequency * 10);
	}
	
	isFull = enable;
}

void GdiToggler::emitRate() {
	DEVMODE devmode;
	devmode.dmSize = sizeof devmode;
	devmode.dmDriverExtra = 0;
	mon->enumDisplaySettings(widgetScreen, ENUM_CURRENT_SETTINGS, &devmode);
	emit rateChange(devmode.dmDisplayFrequency * 10);
}
