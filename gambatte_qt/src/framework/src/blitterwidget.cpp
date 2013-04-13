/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#include "blitterwidget.h"
#include "adaptivesleep.h"
#include <QtGlobal> // for Q_WS_WIN define
#include <algorithm>
#include <cstdlib>

static long limit(long const est, long const ref) {
	long const maxdiff = ref * 3 >> 6;
	return std::max(std::min(est, ref + maxdiff), ref - maxdiff);
}

enum { ft_shift = 6 };

FtEst::FtEst(long const frameTime)
: frameTime_(frameTime)
, ft_(4500000 << ft_shift)
, ftAvg_(frameTime << ftavg_shift)
, last_(0)
, count_((ft_ + (frameTime >> 1)) / (frameTime ? frameTime : 1))
{
}

void FtEst::update(usec_t const t) {
	if (t - last_ < usec_t(ftAvg_ + (ftAvg_ >> 4)) >> ftavg_shift) {
		ft_ += t - last_ < usec_t(ftAvg_ - (ftAvg_ >> 4)) >> ftavg_shift
		     ? (ftAvg_ - (ftAvg_ >> 7)) << (ft_shift - ftavg_shift)
		     : (t - last_) << ft_shift;
		count_ += 1 << ft_shift;

		long const ftAvgNew = long(float(ft_) * ftavg_scale / count_ + 0.5f);
		ftAvg_ = limit((ftAvg_ * 31 + ftAvgNew + 16) >> 5, frameTime_ << ftavg_shift);

		if (ft_ > 6000000 << ft_shift) {
			ft_ = (ft_ * 3 + 2) >> 2;
			count_ = (count_ * 3 + 2) >> 2;
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
