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
#include "xrandrtoggler.h"

#include <QX11Info>

bool XRandRToggler::isUsable() {
	int unused;
	return XQueryExtension(QX11Info::display(), "RANDR", &unused, &unused, &unused);
}

XRandRToggler::XRandRToggler() :
isFull(false)
{
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
	/*Rotation */rotation = 0;
	fullResIndex = originalResIndex = XRRConfigCurrentConfiguration(config, &rotation);
	const short rate = XRRConfigCurrentRate(config);
	
	int nSizes;
	XRRScreenSize *randrSizes = XRRConfigSizes(config, &nSizes);
	
	for (int i = 0; i < nSizes; ++i) {
		ResInfo info;
		info.w = randrSizes[i].width;
		info.h = randrSizes[i].height;
		
		int nHz;
		short *rates = XRRConfigRates(config, i, &nHz);
		
		for (int j = 0; j < nHz; ++j)
			info.rates.push_back(rates[j]);
		
		infoVector.push_back(info);
	}
	
	{
		unsigned i = 0;
		
		while (infoVector[fullResIndex].rates[i] != rate)
			++i;
		
		fullRateIndex = originalRateIndex = i;
	}
	
	XRRFreeScreenConfigInfo(config);
}

XRandRToggler::~XRandRToggler() {
	setFullRes(false);
// 	XRRFreeScreenConfigInfo(config);
}

bool XRandRToggler::isFullRes() const {
	return isFull;
}

void XRandRToggler::setMode(const unsigned resIndex, const unsigned rateIndex) {
	fullResIndex = resIndex;
	fullRateIndex = rateIndex;
	
	if (isFullRes())
		setFullRes(true);
}

void XRandRToggler::setFullRes(const bool enable) {
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
// 	Rotation rotation=0;
	SizeID currentID = XRRConfigCurrentConfiguration(config, &rotation);
	int currentRate = XRRConfigCurrentRate(config);
// 	std::cout << "current rate: " << currentRate << "\n";
	
	SizeID newID;
	int newRate;
	
	if (enable) {
		newID = fullResIndex;
		newRate = infoVector[fullResIndex].rates[fullRateIndex];
	} else {
		newID = originalResIndex;
		newRate = infoVector[originalResIndex].rates[originalRateIndex];
	}
	
// 	std::cout << "new rate: " << newRate << "\n";
	
	if (newID != currentID || newRate != currentRate) {
		XRRSetScreenConfigAndRate(QX11Info::display(), config, QX11Info::appRootWindow(), newID, rotation, newRate, CurrentTime);
		
		//emit modeChange();
		
		if (newRate != currentRate)
			emit rateChange(newRate);
	}
	
	XRRFreeScreenConfigInfo(config);
	
	isFull = enable;
}

void XRandRToggler::emitRate() {
	XRRScreenConfiguration *config = XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow());
	emit rateChange(XRRConfigCurrentRate(config));
	XRRFreeScreenConfigInfo(config);
}
