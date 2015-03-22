//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#include "frameratecontrol.h"
#include "blitterwidget.h"
#include "mediaworker.h"

FrameRateControl::FrameRateControl(MediaWorker &worker, BlitterWidget *blitter)
: worker_(worker), blitter_(blitter), frameTime_(1, 60), refreshRate_(600), refreshRateSync_(false)
{
	update();
}

void FrameRateControl::setBlitter(BlitterWidget *const blitter) {
	blitter_ = blitter;
	blitter_->rateChange(refreshRate_);
	update();
}

void FrameRateControl::setFrameTime(Rational const frameTime) {
	if (frameTime_ != frameTime) {
		frameTime_ = frameTime;
		update();
	}
}

void FrameRateControl::setRefreshRate(int refreshRate) {
	if (refreshRate < 1)
		refreshRate = 600;

	refreshRate_ = refreshRate;
	blitter_->rateChange(refreshRate);
	update();
}

void FrameRateControl::update() {
	unsigned si = 0;

	if (refreshRateSync_) {
		si = (frameTime_.num * refreshRate_ + (frameTime_.denom * 10 >> 1)) / (frameTime_.denom * 10);
		if (si < 1)
			si = 1;

		if (si > blitter_->maxSwapInterval())
			si = blitter_->maxSwapInterval();
	}

	blitter_->setSwapInterval(si);
	worker_.setFrameTime(si ? Rational(si * 10, refreshRate_) : frameTime_);
	worker_.setFrameTimeEstimate(blitter_->frameTimeEst());
}
