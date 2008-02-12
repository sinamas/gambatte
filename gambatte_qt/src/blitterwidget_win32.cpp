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

class Freq {
	LONGLONG value;
	bool valid;
	
public:
	Freq() {
		LARGE_INTEGER li;
		valid = QueryPerformanceFrequency(&li);
		value = valid ? li.QuadPart : 1000;
	}
	
	LONGLONG get() const { return value; }
	bool isValid() const { return valid; }
};

static void getTime(LARGE_INTEGER *const t, const bool qpf) {
	if (qpf)
		QueryPerformanceCounter(t);
	else
		t->QuadPart = timeGetTime();
}

static Freq freq;

class BlitterWidget::Impl {
	struct Time {
		DWORD sec;
		DWORD rsec;
	};
	
	Rational ft;
	Time time;
	
	
public:
	Impl() { time.sec = time.rsec = 0; }
	
	void setFrameTime(Rational ft) {
		this->ft = ft;
	}
	
	const Rational frameTime() const {
		return ft;
	}
	
	int sync(const bool turbo) {
		if (turbo)
			return 0;
		
		const LONGLONG wtime = freq.get() * time.sec + freq.get() * time.rsec / ft.denominator;
		const bool qpf = freq.isValid();
		
		LARGE_INTEGER t;
		
		getTime(&t, qpf);
		
		if (wtime > t.QuadPart && wtime - t.QuadPart <= inc) {
			{
				const DWORD tmp = ((wtime - t.QuadPart) * 1000) / freq.get();
				
				if (tmp > 1) {
					Sleep(tmp - 1);
				}
			}
			
			do {
				getTime(&t, qpf);
			} while (wtime > t.QuadPart);
		} else {
			//quickfix:catches up to current time
			time.sec = t.QuadPart / freq.get();
			time.rsec = (t.QuadPart % freq.get()) * ft.denominator / freq.get();
		}
		
		time.rsec += ft.numerator;
		
		if (time.rsec >= ft.denominator) {
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
{}

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
