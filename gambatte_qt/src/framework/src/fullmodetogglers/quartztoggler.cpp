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

#include "quartztoggler.h"
#include <QApplication>
#include <QDesktopWidget>
#include <functional>

static bool operator!=(ResInfo const &l, ResInfo const &r) {
	return l.w != r.w || l.h != r.h;
}

static bool operator>(ResInfo const &l, ResInfo const &r) {
	return l.w > r.w || (l.w == r.w && l.h > r.h);
}

static void addMode(CFDictionaryRef mode,
                    std::vector<ResInfo> &infoVector,
                    std::size_t *resIndex, std::size_t *rateIndex)
{
	ResInfo info;
	short rate;

	{
		int w = 0;
		int h = 0;
		double r = 0.0;
		CFNumberGetValue(CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayWidth)),
		                 kCFNumberIntType, &w);
		CFNumberGetValue(CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayHeight)),
		                 kCFNumberIntType, &h);
		CFNumberGetValue(CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayRefreshRate)),
		                 kCFNumberDoubleType, &r);

		info.w = w;
		info.h = h;
		rate = static_cast<short>(r * 10.0 + 0.5);
	}

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

QuartzToggler::QuartzToggler()
: originalMode()
, widgetScreen(0)
, isFull(false)
{
	CGDisplayCount dspyCnt = 0;
	CGGetActiveDisplayList(0, 0, &dspyCnt);
	activeDspys.reset(dspyCnt);
	CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt);

	infoVector.resize(dspyCnt);
	fullResIndex.resize(dspyCnt);
	fullRateIndex.resize(dspyCnt);

	for (CGDisplayCount i = 0; i < dspyCnt; ++i) {
		CGDirectDisplayID display = activeDspys[i];
		CFDictionaryRef currentMode = CGDisplayCurrentMode(display);
		CFArrayRef modesArray = CGDisplayAvailableModes(display);
		CFIndex numModes = CFArrayGetCount(modesArray);
		CFNumberRef bpp =
			CFNumberRef(CFDictionaryGetValue(currentMode, kCGDisplayBitsPerPixel));
		for (CFIndex j = 0; j < numModes; ++j) {
			CFDictionaryRef mode =
				CFDictionaryRef(CFArrayGetValueAtIndex(modesArray, j));
			CFNumberRef mbpp =
				CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayBitsPerPixel));
			if (CFNumberCompare(bpp, mbpp, 0) == kCFCompareEqualTo)
				addMode(mode, infoVector[i], 0, 0);
		}
	}

	originalMode = CGDisplayCurrentMode(activeDspys[widgetScreen]);

	for (CGDisplayCount i = 0; i < dspyCnt; ++i) {
		std::size_t resIndex = 0;
		std::size_t rateIndex = 0;
		addMode(CGDisplayCurrentMode(activeDspys[i]),
		        infoVector[i], &resIndex, &rateIndex);
		fullResIndex[i] = resIndex;
		fullRateIndex[i] = rateIndex;
	}
}

QRect const QuartzToggler::fullScreenRect(QWidget const *) const {
	CGRect r = CGDisplayBounds(activeDspys[widgetScreen]);
	return QRectF(r.origin.x, r.origin.y, r.size.width, r.size.height).toRect();
}

void QuartzToggler::setScreen(QWidget const *widget) {
	std::size_t n = QApplication::desktop()->screenNumber(widget);
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

void QuartzToggler::setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex) {
	fullResIndex[screen] = resIndex;
	fullRateIndex[screen] = rateIndex;
	if (isFullMode() && screen == widgetScreen)
		setFullMode(true);
}

void QuartzToggler::setFullMode(bool const enable) {
	CGDirectDisplayID display = activeDspys[widgetScreen];
	CFDictionaryRef currentMode = CGDisplayCurrentMode(display);
	CFDictionaryRef mode = currentMode;
	if (enable) {
		int bpp = 0;
		CFNumberGetValue(
			CFNumberRef(CFDictionaryGetValue(currentMode, kCGDisplayBitsPerPixel)),
			kCFNumberIntType, &bpp);
		mode = CGDisplayBestModeForParametersAndRefreshRate(
			display,
			bpp,
			infoVector[widgetScreen][fullResIndex[widgetScreen]].w,
			infoVector[widgetScreen][fullResIndex[widgetScreen]].h,
			infoVector[widgetScreen][fullResIndex[widgetScreen]].rates[fullRateIndex[widgetScreen]] / 10.0,
			0);

		if (!isFull)
			originalMode = currentMode;
	} else if (isFull)
		mode = originalMode;

	if (mode != currentMode) {
		CGDisplaySwitchToMode(display, mode);

		double oldRate = 0.0;
		double newRate = 0.0;
		CFNumberGetValue(
			CFNumberRef(CFDictionaryGetValue(currentMode, kCGDisplayRefreshRate)),
			kCFNumberDoubleType, &oldRate);
		CFNumberGetValue(
			CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayRefreshRate)),
			kCFNumberDoubleType, &newRate);

		if (static_cast<int>(oldRate * 10.0 + 0.5) != static_cast<int>(newRate * 10.0 + 0.5))
			emit rateChange(static_cast<int>(newRate * 10.0 + 0.5));
	}

	isFull = enable;
}

void QuartzToggler::emitRate() {
	double rate = 0.0;
	CFDictionaryRef mode = CGDisplayCurrentMode(activeDspys[widgetScreen]);
	CFNumberGetValue(CFNumberRef(CFDictionaryGetValue(mode, kCGDisplayRefreshRate)),
	                 kCFNumberDoubleType, &rate);

	emit rateChange(static_cast<int>(rate * 10.0 + 0.5));
}
