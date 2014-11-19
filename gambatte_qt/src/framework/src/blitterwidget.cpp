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

#include "blitterwidget.h"
#include "adaptivesleep.h"
#include <QtGlobal> // for Q_WS_WIN define
#include <algorithm>
#include <cstdlib>

static long limit(long const est, long const ref) {
	long const maxdiff = ref * 3 >> 6;
	return std::max(std::min(est, ref + maxdiff), ref - maxdiff);
}

FtEst::FtEst(long const frameTime)
: frameTime_(frameTime)
, ftAvg_(frameTime << ftavg_shift)
, last_(0)
, t_(2250000)
, c_(t_ / (frameTime ? frameTime : 1))
, tc_(t_ * c_)
, c2_(c_ * c_)
{
}

void FtEst::update(usec_t const t) {
	if (t - last_ < usec_t(ftAvg_ + (ftAvg_ >> 4)) >> ftavg_shift) {
		t_ += t - last_ < usec_t(ftAvg_ - (ftAvg_ >> 4)) >> ftavg_shift
		    ? ftAvg_ * (0.9921875 / ftavg_scale)
		    : t - last_;
		c_ += 1;
		tc_ += t_ * c_;
		c2_ += c_ * c_;

		long const ftAvgNew = long(tc_ * ftavg_scale / c2_ + 0.5);
		ftAvg_ = limit((ftAvg_ * 31 + ftAvgNew + 16) >> 5,
		               frameTime_ << ftavg_shift);
		if (t_ > 3000000) {
			t_ *= 3.0 / 4;
			c_ *= 3.0 / 4;
			tc_ *= 9.0 / 16;
			c2_ *= 9.0 / 16;
		}
	}

	last_ = t;
}

#ifdef Q_WS_WIN

#include <windows.h>

class Timer {
	LONGLONG freq;
	bool qpf;

public:
	Timer() : freq(0), qpf(false) {
		LARGE_INTEGER li;
		qpf = QueryPerformanceFrequency(&li);
		freq = qpf ? li.QuadPart : 1000;
	}

	usec_t get() const {
		LARGE_INTEGER li;

		if (qpf)
			QueryPerformanceCounter(&li);
		else
			li.QuadPart = timeGetTime();

		return static_cast<ULONGLONG>(li.QuadPart) * 1000000 / freq;
	}
};

static Timer timer;

usec_t getusecs() {
	return timer.get();
}

void usecsleep(usec_t usecs) {
	Sleep((usecs + 999) / 1000);
}

#else

#include <sys/time.h>

usec_t getusecs() {
	timeval t;
	gettimeofday(&t, 0);
	return t.tv_sec * usec_t(1000000) + t.tv_usec;
}

void usecsleep(usec_t usecs) {
	timespec tspec = { 0, long(usecs) * 1000 };
	nanosleep(&tspec, 0);
}

#endif /*Q_WS_WIN*/

BlitterWidget::BlitterWidget(VideoBufferLocker vbl,
                             QString const &name,
                             unsigned maxSwapInterval,
                             QWidget *parent)
: QWidget(parent)
, vbl_(vbl)
, nameString_(name)
, maxSwapInterval_(maxSwapInterval)
, paused_(true)
{
	setMouseTracking(true);
}
