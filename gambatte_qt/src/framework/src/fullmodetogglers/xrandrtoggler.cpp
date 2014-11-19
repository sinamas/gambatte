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

#include "xrandrtoggler.h"
#include <QX11Info>

bool XRandRToggler::isUsable() {
	int unused = 0;
	int major = 1;
	int minor = 1;
	return XRRQueryExtension(QX11Info::display(), &unused, &unused)
	    && XRRQueryVersion(QX11Info::display(), &major, &minor)
	    && major == 1 && minor >= 1;
}

struct XRandRToggler::XRRDeleter {
	static void del(XRRScreenConfiguration *c) { if (c) { XRRFreeScreenConfigInfo(c); } }
};

XRandRToggler::XRandRToggler()
: config_(XRRGetScreenInfo(QX11Info::display(), QX11Info::appRootWindow()))
, originalResIndex_(0)
, fullResIndex_(0)
, rotation_(0)
, fullRateIndex_(0)
, originalRate_(0)
, isFull_(false)
{
	fullResIndex_ = originalResIndex_ =
		XRRConfigCurrentConfiguration(config_.get(), &rotation_);
	originalRate_ = XRRConfigCurrentRate(config_.get());

	int nSizes;
	XRRScreenSize const *const randrSizes = XRRConfigSizes(config_.get(), &nSizes);
	for (int i = 0; i < nSizes; ++i) {
		ResInfo info;
		info.w = randrSizes[i].width;
		info.h = randrSizes[i].height;

		int nHz;
		short const *const rates = XRRConfigRates(config_.get(), i, &nHz);
		for (int j = 0; j < nHz; ++j)
			info.rates.push_back(rates[j] * 10);

		infoVector_.push_back(info);
	}

	std::size_t i = 0;
	while (i < infoVector_[fullResIndex_].rates.size()
			&& infoVector_[fullResIndex_].rates[i] != originalRate_ * 10) {
		++i;
	}

	if (i == infoVector_[fullResIndex_].rates.size())
		infoVector_[fullResIndex_].rates.push_back(originalRate_ * 10);

	fullRateIndex_ = i;
}

QRect const XRandRToggler::fullScreenRect(QWidget const *) const {
	int w, h;

	{
		int nSizes = 0;
		XRRScreenSize const *randrSizes = XRRConfigSizes(config_.get(), &nSizes);
		Rotation rot = rotation_;
		SizeID currentID = XRRConfigCurrentConfiguration(config_.get(), &rot);
		if (currentID < nSizes) {
			w = randrSizes[currentID].width;
			h = randrSizes[currentID].height;
		} else {
			w = infoVector_[fullResIndex_].w;
			h = infoVector_[fullResIndex_].h;
		}
	}

	return QRect(0, 0, w, h);
}

void XRandRToggler::setMode(std::size_t /*screen*/, std::size_t resIndex, std::size_t rateIndex) {
	fullResIndex_ = resIndex;
	fullRateIndex_ = rateIndex;
	if (isFullMode())
		setFullMode(true);
}

void XRandRToggler::setFullMode(bool const enable) {
	SizeID const currentID = XRRConfigCurrentConfiguration(config_.get(), &rotation_);
	int const currentRate = XRRConfigCurrentRate(config_.get());

	SizeID newID;
	int newRate;
	if (enable) {
		newID = fullResIndex_;
		newRate = infoVector_[fullResIndex_].rates[fullRateIndex_] / 10;

		if (!isFull_) {
			originalResIndex_ = currentID;
			originalRate_ = currentRate;
		}
	} else {
		newID = originalResIndex_;
		newRate = originalRate_;
	}

	if (newID != currentID || newRate != currentRate) {
		XRRSetScreenConfigAndRate(QX11Info::display(), config_.get(),
		                          QX11Info::appRootWindow(),
		                          newID, rotation_, newRate, CurrentTime);
		if (newRate != currentRate)
			emit rateChange(newRate * 10);
	}

	isFull_ = enable;
}

void XRandRToggler::emitRate() {
	emit rateChange(XRRConfigCurrentRate(config_.get()) * 10);
}
