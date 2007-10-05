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

void DutyUnit::updatePos(const unsigned cc) {
	if (cc >= nextPosUpdate) {
		const unsigned inc = 1 + (cc - nextPosUpdate) / period;
		nextPosUpdate += period * inc;
		pos += inc;
		pos &= 7;
	}
}

void DutyUnit::setDuty(const unsigned nr1) {
	static const uint8_t duties[4] = { 0x80, 0x81, 0xE1, 0x7E };
	
	high = duties[(duty = nr1 >> 6)] >> pos & 1;
}

void DutyUnit::setCounter() {
	static const uint8_t nextStateDistance[4 * 8] = {
		6, 5, 4, 3, 2, 1, 0, 0,
		0, 5, 4, 3, 2, 1, 0, 1,
		0, 3, 2, 1, 0, 3, 2, 1,
		0, 5, 4, 3, 2, 1, 0, 1
	};
	
	counter = nextPosUpdate != 0xFFFFFFFF ? nextPosUpdate + period * nextStateDistance[duty * 8 | pos] : nextPosUpdate;
}

void DutyUnit::setFreq(const unsigned newFreq, const unsigned cc) {
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

void DutyUnit::nr1Change(const unsigned newNr1, const unsigned cc) {
	updatePos(cc);
	setDuty(newNr1);
	setCounter();
}

void DutyUnit::nr3Change(const unsigned newNr3, const unsigned cc) {
	setFreq(getFreq() & 0x700 | newNr3, cc);
}

void DutyUnit::nr4Change(const unsigned newNr4, const unsigned cc) {
	setFreq(newNr4 << 8 & 0x700 | getFreq() & 0xFF, cc);
	
	if (newNr4 & 0x80) {
		nextPosUpdate = (cc & ~1) + period;
		setCounter();
	}
}

void DutyUnit::init(const unsigned cc) {
	pos = 0;
	setDuty(0);
	period = 2048 << 1;
	nextPosUpdate = (cc & ~1) + period;
	setCounter();
}

void DutyUnit::reset() {
	pos = 0;
	setDuty(0);
	nextPosUpdate = 0xFFFFFFFF;
	setCounter();
}

void DutyUnit::resetCounters(const unsigned oldCc) {
	if (nextPosUpdate == 0xFFFFFFFF)
		return;
	
	updatePos(oldCc);
	nextPosUpdate -= 0x80000000;
	counter -= 0x80000000;
}
