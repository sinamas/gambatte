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
#include "duty_unit.h"

void DutyUnit::frequencyChange(const unsigned nr3, const unsigned nr4) {
	const unsigned base = nr3 | ((nr4 & 0x7) << 8);
	period = (2048 - base) * 2;
}

void DutyUnit::event() {
	unsigned inc = period << duty;
	if (duty == 3)
		inc -= period * 2;
	
	highState ^= 1;
	if (!highState)
		inc = period * 8 - inc;
		
	counter += inc;
}

void DutyUnit::nr4Change(const unsigned nr3, const unsigned newNr4, const unsigned cycleCounter) {
	frequencyChange(nr3, newNr4);
	
	if ((newNr4 & 0x80) != 0 && counter == 0xFFFFFFFF)
		counter = cycleCounter;
}

void DutyUnit::reset(const unsigned nr1, const unsigned nr3, const unsigned nr4) {
	nr1Change(nr1);
	frequencyChange(nr3, nr4);
	counter = 0;
	highState = true;
}
