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

static inline bool toOutState(const unsigned duty, const unsigned pos) {
	static const unsigned char duties[4] = { 0x80, 0x81, 0xE1, 0x7E };
	
	return duties[duty] >> pos & 1;
}

void DutyUnit::updatePos(const unsigned long cc) {
	if (cc >= nextPosUpdate) {
		const unsigned long inc = (cc - nextPosUpdate) / period + 1;
		nextPosUpdate += period * inc;
		pos += inc;
		pos &= 7;
	}
}

void DutyUnit::setDuty(const unsigned nr1) {
	duty = nr1 >> 6;
	high = toOutState(duty, pos);
}

void DutyUnit::setCounter() {
	static const unsigned char nextStateDistance[4 * 8] = {
		6, 5, 4, 3, 2, 1, 0, 0,
		0, 5, 4, 3, 2, 1, 0, 1,
		0, 3, 2, 1, 0, 3, 2, 1,
		0, 5, 4, 3, 2, 1, 0, 1
	};
	
	if (enableEvents && nextPosUpdate != COUNTER_DISABLED)
		counter = nextPosUpdate + period * nextStateDistance[duty * 8 | pos];
	else
		counter = COUNTER_DISABLED;
}

void DutyUnit::setFreq(const unsigned newFreq, const unsigned long cc) {
	updatePos(cc);
	period = 2048 - newFreq << 1;
	setCounter();
}

void DutyUnit::event() {
	unsigned inc = period << duty;
	
	if (duty == 3)
		inc -= period * 2;
	
	if (!(high ^= true))
		inc = period * 8 - inc;
	
	counter += inc;
}

void DutyUnit::nr1Change(const unsigned newNr1, const unsigned long cc) {
	updatePos(cc);
	setDuty(newNr1);
	setCounter();
}

void DutyUnit::nr3Change(const unsigned newNr3, const unsigned long cc) {
	setFreq(getFreq() & 0x700 | newNr3, cc);
}

void DutyUnit::nr4Change(const unsigned newNr4, const unsigned long cc) {
	setFreq(newNr4 << 8 & 0x700 | getFreq() & 0xFF, cc);
	
	if (newNr4 & 0x80) {
		nextPosUpdate = (cc & ~1) + period;
		setCounter();
	}
}

void DutyUnit::init(const unsigned long cc) {
	pos = 0;
	setDuty(0);
	period = 2048 << 1;
	nextPosUpdate = (cc & ~1) + period;
	enableEvents = true;
	setCounter();
}

void DutyUnit::reset() {
	pos = 0;
	high = toOutState(duty, pos);
	nextPosUpdate = COUNTER_DISABLED;
	setCounter();
}

void DutyUnit::resetCounters(const unsigned long oldCc) {
	if (nextPosUpdate == COUNTER_DISABLED)
		return;
	
	updatePos(oldCc);
	nextPosUpdate -= COUNTER_MAX;
	SoundUnit::resetCounters(oldCc);
}

void DutyUnit::killCounter() {
	enableEvents = false;
	setCounter();
}

void DutyUnit::reviveCounter(const unsigned long cc) {
	updatePos(cc);
	high = toOutState(duty, pos);
	enableEvents = true;
	setCounter();
}
