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

#include <sys/time.h>

class BlitterWidget::Impl {
	struct Time {
		long sec;
		long rsec;
	};
	
	Time time;
	Rational ft;
	long late;
	unsigned noSleep;
	
public:
	Impl() : late(0), noSleep(60) { time.sec = time.rsec = 0; }
	
	void setFrameTime(Rational ft) { this->ft = ft; }
	
	const Rational frameTime() const {
		return ft;
	}
	
	int sync(const bool turbo) {
		if (turbo)
			return 0;
		
		const long time_usec = static_cast<quint64>(time.rsec) * 1000000 / ft.denominator;
		
		timeval t;
		gettimeofday(&t, NULL);
		
		if (time.sec > t.tv_sec || time.sec == t.tv_sec && time_usec > t.tv_usec) {
			timeval tmp = { tv_sec: 0, tv_usec: time_usec - t.tv_usec };
			
			if (time.sec != t.tv_sec)
				tmp.tv_usec += 1000000;
			
			if (tmp.tv_usec > late) {
				tmp.tv_usec -= late;
				
				if (tmp.tv_usec >= 1000000) {
					tmp.tv_usec -= 1000000;
					++tmp.tv_sec;
				}
				
				timespec tspec = { tmp.tv_sec, tmp.tv_usec * 1000 };
				nanosleep(&tspec, NULL);
				
				gettimeofday(&t, NULL);
				late -= (time.sec - t.tv_sec) * 1000000 + time_usec - t.tv_usec >> 1;
				
				if (late < 0)
					late = 0;
				
				noSleep = 60;
			} else if (noSleep-- == 0) {
				noSleep = 60;
				late = 0;
			}
			
			while (time.sec > t.tv_sec || time.sec == t.tv_sec && time_usec > t.tv_usec)
				gettimeofday(&t, NULL);
		} else {
			//quickfix:catches up to current time
			time.sec = t.tv_sec;
			time.rsec = static_cast<quint64>(ft.denominator) * t.tv_usec / 1000000;
		}
		
		time.rsec += ft.numerator;
		
		if (static_cast<unsigned long>(time.rsec) >= ft.denominator) {
			time.rsec -= ft.denominator;
			++time.sec;
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
