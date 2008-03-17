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
#include "blitterwidget.h"

struct Time {
	long sec;
	long rsec;
};

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
	
	void get(Time *const time, const long denom) const {
		LARGE_INTEGER li;
		
		if (qpf)
			QueryPerformanceCounter(&li);
		else
			li.QuadPart = timeGetTime();
		
		time->sec = li.QuadPart / freq;
		time->rsec = (li.QuadPart % freq) * denom / freq;
	}
};

static Timer timer;

static void getTime(Time *const time, const long denom) {
	timer.get(time, denom);
}

static void sleep(const long secnum, const long secdenom) {
	Sleep(static_cast<qlonglong>(secnum) * 1000 / secdenom);
}

#else

#include <sys/time.h>

static void getTime(Time *const time, const long denom) {
	timeval t;
	gettimeofday(&t, NULL);
	time->sec = t.tv_sec;
	time->rsec = static_cast<qlonglong>(t.tv_usec) * denom / 1000000;
}

static void sleep(const long secnum, const long secdenom) {
	timespec tspec = { tv_sec: 0,
	                   tv_nsec: static_cast<qlonglong>(secnum) * 1000000000 / secdenom };

	nanosleep(&tspec, NULL);
}

#endif /*Q_WS_WIN*/

class BlitterWidget::Impl {
	Time last;
	long ftnum;
	long ftdenom;
	long late;
	unsigned noSleep;
	
public:
	Impl() : ftnum(Rational().numerator), ftdenom(Rational().denominator), late(0), noSleep(60) { getTime(&last, ftdenom); }
	
	void setFrameTime(const Rational &ft) {
		last.rsec = static_cast<qulonglong>(last.rsec) * ft.denominator / ftdenom;
		ftnum = ft.numerator;
		ftdenom = ft.denominator;
		late = 0;
	}
	
	const Rational frameTime() const {
		return Rational(ftnum, ftdenom);
	}
	
	int sync(const bool turbo) {
		if (turbo)
			return 0;
		
		Time current;
		getTime(&current, ftdenom);
		
		long diff = (current.sec - last.sec) * ftdenom + current.rsec - last.rsec;
		
		if (diff < ftnum) {
			diff = ftnum - diff;
			
			if (diff > late) {
				sleep(diff - late, ftdenom);
				
				getTime(&current, ftdenom);
				late += ((current.sec - last.sec) * ftdenom + current.rsec - last.rsec - ftnum) / 2;
				
				if (late < 0)
					late = 0;
				
				noSleep = 60;
			} else if (noSleep-- == 0) {
				noSleep = 60;
				late = 0;
			}
			
			while ((current.sec - last.sec) * ftdenom + current.rsec - last.rsec < ftnum)
				getTime(&current, ftdenom);
			
			last.rsec += ftnum;
			
			if (last.rsec >= ftdenom) {
				last.rsec -= ftdenom;
				++last.sec;
			}
		} else {
			//quickfix:catches up to current time
			last = current;
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
setPixelBuffer(setPixelBuffer),
nameString(name),
integerOnlyScaler(integerOnlyScaler)
{
	setMouseTracking(true);
}

BlitterWidget::~BlitterWidget() {
	delete impl;
}

void BlitterWidget::setFrameTime(Rational ft) {
	impl->setFrameTime(ft);
}

const BlitterWidget::Rational BlitterWidget::frameTime() const {
	return impl->frameTime();
}

int BlitterWidget::sync(const bool turbo) {
	return impl->sync(turbo);
}
