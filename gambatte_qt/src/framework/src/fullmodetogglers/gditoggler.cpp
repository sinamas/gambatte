//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#include "gditoggler.h"
#include "../gdisettings.h"
#include <QApplication>
#include <QDesktopWidget>
#include <windows.h>
#include <algorithm>
#include <functional>

static bool operator!=(ResInfo const &l, ResInfo const &r) {
	return l.w != r.w || l.h != r.h;
}

static bool operator>(ResInfo const &l, ResInfo const &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static void addMode(DEVMODE const &devmode,
                    std::vector<ResInfo> &infoVector,
                    std::size_t *resIndex, std::size_t *rateIndex)
{
	ResInfo info;
	info.w = devmode.dmPelsWidth;
	info.h = devmode.dmPelsHeight;
	short const rate = devmode.dmDisplayFrequency * 10;

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

class GdiToggler::MultiMon {
public:
	MultiMon() {
		if (!gdiSettings.monitorFromPoint)
			return;

		for (std::size_t i = 0, n = QApplication::desktop()->numScreens(); i < n; ++i) {
			QPoint qpoint = QApplication::desktop()->screenGeometry(i).center();
			POINT point = { qpoint.x(), qpoint.y() };
			HMONITOR m = gdiSettings.monitorFromPoint(
				point, GdiSettings::MON_DEFAULTTONEAREST);
			monitors_.push_back(m);
		}
	}

	std::size_t numScreens() const { return std::max(std::size_t(1), monitors_.size()); }

	BOOL enumDisplaySettings(std::size_t screen, DWORD iModeNum, LPDEVMODE devmode) const {
		return gdiSettings.enumDisplaySettings(monitor(screen), iModeNum, devmode);
	}

	LONG changeDisplaySettings(std::size_t screen, LPDEVMODE devmode, DWORD dwflags) const {
		return gdiSettings.changeDisplaySettings(monitor(screen), devmode, dwflags);
	}

private:
	std::vector<HMONITOR> monitors_;

	HMONITOR monitor(std::size_t screen) const {
		return screen < monitors_.size() ? monitors_[screen] : 0;
	}
};

GdiToggler::GdiToggler()
: mon(new MultiMon)
, widgetScreen(0)
, isFull(false)
{
	infoVector.resize(mon->numScreens());
	fullResIndex.resize(mon->numScreens());
	fullRateIndex.resize(mon->numScreens());

	DEVMODE devmode;
	devmode.dmSize = sizeof devmode;
	devmode.dmDriverExtra = 0;

	for (std::size_t i = 0; i < mon->numScreens(); ++i) {
		mon->enumDisplaySettings(i, ENUM_CURRENT_SETTINGS, &devmode);
		unsigned const bpp = devmode.dmBitsPerPel;
		int n = 0;
		while (mon->enumDisplaySettings(i, n++, &devmode)) {
			if (devmode.dmBitsPerPel == bpp)
				addMode(devmode, infoVector[i], 0, 0);
		}
	}

	for (std::size_t i = 0; i < mon->numScreens(); ++i) {
		mon->enumDisplaySettings(i, ENUM_CURRENT_SETTINGS, &devmode);
		addMode(devmode, infoVector[i], &fullResIndex[i], &fullRateIndex[i]);
	}
}

GdiToggler::~GdiToggler() {
}

void GdiToggler::setScreen(QWidget const *widget) {
	std::size_t n = QApplication::desktop()->screenNumber(widget);
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

void GdiToggler::setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex) {
	fullResIndex[screen] = resIndex;
	fullRateIndex[screen] = rateIndex;

	if (isFullMode() && screen == widgetScreen)
		setFullMode(true);
}

void GdiToggler::setFullMode(bool const enable) {
	DEVMODE devmode;
	devmode.dmSize = sizeof devmode;
	devmode.dmDriverExtra = 0;

	mon->enumDisplaySettings(widgetScreen, ENUM_CURRENT_SETTINGS, &devmode);
	unsigned const currentWidth = devmode.dmPelsWidth;
	unsigned const currentHeight = devmode.dmPelsHeight;
	unsigned const currentRate = devmode.dmDisplayFrequency;

	if (enable) {
		ResInfo const &info = infoVector[widgetScreen][fullResIndex[widgetScreen]];
		devmode.dmPelsWidth = info.w;
		devmode.dmPelsHeight = info.h;
		devmode.dmDisplayFrequency = info.rates[fullRateIndex[widgetScreen]] / 10;
	} else if (isFull) {
		mon->enumDisplaySettings(widgetScreen, ENUM_REGISTRY_SETTINGS, &devmode);
	}

	if (devmode.dmPelsWidth != currentWidth
			|| devmode.dmPelsHeight != currentHeight
			|| devmode.dmDisplayFrequency != currentRate) {
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
