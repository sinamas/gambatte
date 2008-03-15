/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
	rate = devmode.dmDisplayFrequency;
	
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

GdiToggler::GdiToggler() :
originalWidth(0),
originalHeight(0),
originalRate(0),
fullResIndex(0),
fullRateIndex(0),
isFull(false)
{
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	originalWidth = devmode.dmPelsWidth;
	originalHeight = devmode.dmPelsHeight;
	originalRate = devmode.dmDisplayFrequency;
	
	const unsigned bpp = devmode.dmBitsPerPel;
	
	int n = 0;
	
	while (EnumDisplaySettings(NULL, n++, &devmode)) {
		if (devmode.dmBitsPerPel == bpp)
			addMode(devmode, infoVector, NULL, NULL);
	}
	
	devmode.dmPelsWidth = originalWidth;
	devmode.dmPelsHeight = originalHeight;
	devmode.dmDisplayFrequency = originalRate;
	
	addMode(devmode, infoVector, &fullResIndex, &fullRateIndex);
}

GdiToggler::~GdiToggler() {
	setFullMode(false);
}

const QRect GdiToggler::fullScreenRect(const QWidget */*wdgt*/) const {
	return QApplication::desktop()->screenGeometry(/*wdgt*/);
}

void GdiToggler::setMode(unsigned /*screen*/, const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex = resIndex;
	fullRateIndex = rateIndex;
	
	if (isFullMode())
		setFullMode(true);
}

void GdiToggler::setFullMode(const bool enable) {
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	const unsigned currentWidth = devmode.dmPelsWidth;
	const unsigned currentHeight = devmode.dmPelsHeight;
	const unsigned currentRate = devmode.dmDisplayFrequency;
	
	if (enable) {
		const ResInfo &info = infoVector[fullResIndex];
		devmode.dmPelsWidth = info.w;
		devmode.dmPelsHeight = info.h;
		devmode.dmDisplayFrequency = info.rates[fullRateIndex];
		
		if (!isFull) {
			originalWidth = currentWidth;
			originalHeight = currentHeight;
			originalRate = currentRate;
		}
	} else {
		devmode.dmPelsWidth = originalWidth;
		devmode.dmPelsHeight = originalHeight;
		devmode.dmDisplayFrequency = originalRate;
	}
	
	if (devmode.dmPelsWidth != currentWidth || devmode.dmPelsHeight != currentHeight || devmode.dmDisplayFrequency != currentRate) {
		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
		
		if (devmode.dmDisplayFrequency != currentRate)
			emit rateChange(devmode.dmDisplayFrequency);
	}
	
	isFull = enable;
}

void GdiToggler::emitRate() {
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	emit rateChange(devmode.dmDisplayFrequency);
}
