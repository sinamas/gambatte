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
	
public:
	Freq() {
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);
		value = li.QuadPart;
	}
	
	LONGLONG get() const { return value; }
};

const BlitterWidget::Rational BlitterWidget::frameTime() const {
	Rational r = { 16743, 1000000 };
	
	return r;
}

int BlitterWidget::sync(const bool turbo) {
	if (turbo)
		return 0;
	
	static Freq freq;
	static const LONGLONG inc = (freq.get() * 16743) / 1000000;
	
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	static LONGLONG time = t.QuadPart;

	if (time > t.QuadPart) {
		{
			const DWORD tmp = ((time - t.QuadPart) * 1000) / freq.get();
			
			if (tmp > 1) {
				Sleep(tmp - 1);
			}
		}

		do {
			QueryPerformanceCounter(&t);
		} while (time > t.QuadPart);
	} else
		time = t.QuadPart; //quickfix:catches up to current time (The GUI tends to pause the run loop, so this function doesn't get called)

	time += inc;
	
	return 0;
}
