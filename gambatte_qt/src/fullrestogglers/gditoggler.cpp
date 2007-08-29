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

#include <algorithm>
#include <windows.h>

template<typename T>
struct greater {
	bool operator()(const T &l, const T &r) const {
		return l > r;
	}
};

static inline bool operator!=(const ResInfo &l, const ResInfo &r) {
	return l.w != r.w || l.h != r.h;
}

static inline bool operator>(const ResInfo &l, const ResInfo &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

GdiToggler::GdiToggler() :
isFull(false)
{
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	const unsigned originalWidth = devmode.dmPelsWidth;
	const unsigned originalHeight = devmode.dmPelsHeight;
	const short originalRate = devmode.dmDisplayFrequency;
	const unsigned bpp = devmode.dmBitsPerPel;
	
	int n = 0;
	
	while (EnumDisplaySettings(NULL, n++, &devmode)) {
		if (devmode.dmBitsPerPel != bpp)
			continue;
		
		ResInfo info;
		info.w = devmode.dmPelsWidth;
		info.h = devmode.dmPelsHeight;
		
		std::vector<ResInfo>::iterator it = std::lower_bound(infoVector.begin(), infoVector.end(), info, greater<ResInfo>());
		
		if (it == infoVector.end() || *it != info)
			it = infoVector.insert(it, info);
		
		std::vector<short>::iterator rateIt = std::lower_bound(it->rates.begin(), it->rates.end(), devmode.dmDisplayFrequency, greater<short>());
		
		if (rateIt == it->rates.end() || *rateIt != static_cast<short>(devmode.dmDisplayFrequency))
			it->rates.insert(rateIt, devmode.dmDisplayFrequency);
	}
	
	{
		unsigned i = 0;
		
		while (i < infoVector.size() && (infoVector[i].w != originalWidth || infoVector[i].h != originalHeight))
			++i;
		
		fullResIndex = originalResIndex = i < infoVector.size() ? i : 0;
	}
	
	{
		unsigned i = 0;
		
		while (i < infoVector[fullResIndex].rates.size() && infoVector[fullResIndex].rates[i] != originalRate)
			++i;
		
		fullRateIndex = originalRateIndex = i < infoVector[fullResIndex].rates.size() ? i : 0;
	}
}

GdiToggler::~GdiToggler() {
	setFullRes(false);
}

bool GdiToggler::isFullRes() const {
	return isFull;
}

void GdiToggler::setMode(const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex = resIndex;
	fullRateIndex = rateIndex;
	
	if (isFullRes())
		setFullRes(true);
}

void GdiToggler::setFullRes(const bool enable) {
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	const unsigned currentWidth = devmode.dmPelsWidth;
	const unsigned currentHeight = devmode.dmPelsHeight;
	const unsigned currentRate = devmode.dmDisplayFrequency;
	
	{
		const ResInfo &info = infoVector[enable ? fullResIndex : originalResIndex];
		devmode.dmPelsWidth = info.w;
		devmode.dmPelsHeight = info.h;
		devmode.dmDisplayFrequency = info.rates[enable ? fullRateIndex : originalRateIndex];
	}
	
	if (devmode.dmPelsWidth != currentWidth || devmode.dmPelsHeight != currentHeight || devmode.dmDisplayFrequency != currentRate) {
		devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
		
		//emit modeChange();
		
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
