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
#include <cstdlib>

void FtEst::init(const long frameTime) {
	this->frameTime = frameTime;
	ft = 0;
	ftAvg = frameTime << COUNT_LOG2;
	ftVar = ftAvg >> 12;
	last = 0;
	count = COUNT;
}

void FtEst::update(const usec_t t) {
	if (t - last < static_cast<unsigned long>(frameTime + (frameTime >> 2))) {
		ft += t - last;
		
		if (--count == 0) {
			count = COUNT;
			long oldFtAvg = ftAvg;
			ftAvg = (ftAvg * 31 + ft + 16) >> 5;
			
			if (ftAvg > ((frameTime + (frameTime >> 6)) << COUNT_LOG2))
				ftAvg = (frameTime + (frameTime >> 6)) << COUNT_LOG2;
			else if (ftAvg < ((frameTime - (frameTime >> 6)) << COUNT_LOG2))
				ftAvg = (frameTime - (frameTime >> 6)) << COUNT_LOG2;
			
			ftVar = (ftVar * 15 + std::abs(ftAvg - oldFtAvg) + 8) >> 4;
			ft = 0;
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

class BlitterWidget::Impl {
	AdaptiveSleep asleep;
	usec_t last;
	
public:
	Impl() : last(0) {}
	
	long sync(const long ft) {
		if (ft) {
			last += asleep.sleepUntil(last, ft);
			last += ft;
		}
		
		return 0;
	}
};

BlitterWidget::BlitterWidget(PixelBufferSetter setPixelBuffer,
                             const QString &name,
                             bool integerOnlyScaler,
                             QWidget *parent) :
QWidget(parent),
impl(new Impl),
ft(1000000/60),
setPixelBuffer(setPixelBuffer),
nameString(name),
integerOnlyScaler(integerOnlyScaler)
{
	setMouseTracking(true);
}

BlitterWidget::~BlitterWidget() {
	delete impl;
}

long BlitterWidget::sync(const long ft) {
	return impl->sync(ft);
}
