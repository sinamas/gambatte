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

#include <windows.h>

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
		
		if (infoVector.empty() || infoVector.back().w != info.w || infoVector.back().h != info.h) {
			infoVector.push_back(info);
		}
		
		infoVector.back().rates.push_back(devmode.dmDisplayFrequency);
	}
	
	{
		unsigned i = 0;
		
		while (infoVector[i].w != originalWidth || infoVector[i].h != originalHeight)
			++i;
		
		fullResIndex = originalResIndex = i;
	}
	
	{
		unsigned i = 0;
		
		while (infoVector[fullResIndex].rates[i] != originalRate)
			++i;
		
		fullRateIndex = originalRateIndex = i;
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
