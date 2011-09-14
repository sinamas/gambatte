/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#include "blitterwidget.h"
#include <adaptivesleep.h>
#include <QtGlobal> // for Q_WS_WIN define
#include <cstdlib>

static long limit(long est, const long ref) {
	if (est > ref + (ref >> 5))
		est = ref + (ref >> 5);
	else if (est < ref - (ref >> 5))
		est = ref - (ref >> 5);

	return est;
}

void FtEst::init(const long frameTime) {
	this->frameTime = frameTime;
	ft = 4500000 << 6;
	ftAvg = frameTime << UPSHIFT;
	last = 0;
	count = (ft + (frameTime >> 1)) / (frameTime ? frameTime : 1);
}

void FtEst::update(const usec_t t) {
	if (std::abs(static_cast<long>(t - last) - frameTime) < frameTime >> 3) {
		ft += (t - last) << 6;
		count += 1 << 6;

		long ftAvgNew = static_cast<long>(static_cast<float>(ft) * UP / count + 0.5f);
		ftAvgNew = limit((ftAvg * 31 + ftAvgNew + 16) >> 5, frameTime << UPSHIFT);
		ftAvg = ftAvgNew;

		if (ft > (6000000 << 6)) {
			ft = (ft * 3 + 2) >> 2;
			count = (count * 3 + 2) >> 2;
		}
	}

	last = t;
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

void usecsleep(const usec_t usecs) {
	Sleep((usecs + 999) / 1000);
}

#else

#include <sys/time.h>

usec_t getusecs() {
	timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * usec_t(1000000) + t.tv_usec;
}

void usecsleep(const usec_t usecs) {
	timespec tspec = { tv_sec: 0,
	                   tv_nsec: usecs * 1000 };

	nanosleep(&tspec, NULL);
}

#endif /*Q_WS_WIN*/

BlitterWidget::BlitterWidget(VideoBufferLocker vbl,
                             const QString &name,
                             unsigned maxSwapInterval,
                             QWidget *parent) :
QWidget(parent),
vbl(vbl),
nameString_(name),
maxSwapInterval_(maxSwapInterval),
paused(true)
{
	setMouseTracking(true);
}
