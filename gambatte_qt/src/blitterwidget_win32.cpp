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

#include <windows.h>

struct Time {
	DWORD sec;
	DWORD rsec;
};

class Timer {
	LONGLONG freq;
	bool qpf;
	
public:
	Timer() : freq(0), qpf(false) {
		LARGE_INTEGER li;
		qpf = QueryPerformanceFrequency(&li);
		freq = qpf ? li.QuadPart : 1000;
	}
	
	void get(Time *const time, const BlitterWidget::Rational &ft) const {
		LARGE_INTEGER li;
		
		if (qpf)
			QueryPerformanceCounter(&li);
		else
			li.QuadPart = timeGetTime();
		
		time->sec = li.QuadPart / freq;
		time->rsec = (li.QuadPart % freq) * ft.denominator / freq;
	}
};

static Timer timer;

class BlitterWidget::Impl {
	Rational ft;
	Time last;
	
public:
	Impl() { timer.get(&last, ft); }
	
	void setFrameTime(const Rational &ft) {
		last.rsec = last.rsec * ft.denominator / this->ft.denominator;
		this->ft = ft;
	}
	
	const Rational& frameTime() const {
		return ft;
	}
	
	int sync(const bool turbo) {
		if (turbo)
			return 0;
		
		Time current;
		timer.get(&current, ft);
		
		const DWORD diff = (current.sec - last.sec) * ft.denominator + current.rsec - last.rsec;
		
		if (diff < ft.numerator) {
			{
				const DWORD msdiff = diff * 1000 / ft.denominator;
				
				if (msdiff > 1) {
					Sleep(msdiff - 1);
				}
			}
			
			do {
				timer.get(&current, ft);
			} while ((current.sec - last.sec) * ft.denominator + current.rsec - last.rsec < ft.numerator);
			
			last.rsec += ft.numerator;
			
			if (last.rsec >= ft.denominator) {
				last.rsec -= ft.denominator;
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
