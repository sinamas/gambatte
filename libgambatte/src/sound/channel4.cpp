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
#include "channel4.h"

void Lfsr::event() {
	{
		unsigned s = (nr3 >> 4) + 3;
		unsigned r = nr3 & 7;
		
		if (!r) {
			r = 1;
			--s;
		}
		
		counter += r << s; 
	}
	
// 	counter += ((nr3 & 7) + 1) << ((nr3 >> 4) + 2);
	
	const unsigned tmp = (nr3 & 0x8) ? reg : (reg >> 8);
	highState = (tmp & 0x40) == 0;
	reg = (reg << 1) | ((((tmp >> 1) ^ tmp) >> 5) & 1);
}

Channel4::Channel4() :
	disableMaster(master, envelopeUnit.counter, lfsr.counter, lfsr.counter),
	lengthCounter(disableMaster, 0x3F)
{}

void Channel4::setEvent() {
	nextEventUnit = &lfsr;
	if (envelopeUnit.counter < nextEventUnit->counter)
		nextEventUnit = &envelopeUnit;
	if (lengthCounter.counter < nextEventUnit->counter)
		nextEventUnit = &lengthCounter;
}

void Channel4::setNr1(const unsigned data) {
// 	nr1 = data;
	
	lengthCounter.nr1Change(data, nr4, cycleCounter);
	
	setEvent();
}

void Channel4::setNr2(const unsigned data) {
	nr2 = data;
	
	if (master) {
		if (envelopeUnit.nr2Change(data, cycleCounter))
			disableMaster();
			
		setEvent();
	}
}

void Channel4::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		master = envelopeUnit.nr4Init(nr2, cycleCounter) == false;
		if (master)
			lfsr.nr4Init(cycleCounter);
	}
	
	setEvent();
}

void Channel4::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel4::reset(const unsigned nr1_data, const unsigned nr2_data, const unsigned nr3_data, const unsigned nr4_data) {
// 	nr1 = nr1_data;
	nr2 = nr2_data;
// 	nr3 = nr3_data;
	nr4 = nr4_data;
	
	cycleCounter = 0;

	lengthCounter.reset(nr1_data);
	lfsr.reset(nr3_data);
	envelopeUnit.reset(nr2_data);
	
	disableMaster();
	
	setEvent();
}

void Channel4::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = soBaseVol & soMask;
	
	const unsigned endCycles = cycleCounter + cycles;
	while (cycleCounter < endCycles) {
		const unsigned out = (master && lfsr.isHighState()) ? outBase * envelopeUnit.getVolume() : 0;
		
		unsigned multiplier = nextEventUnit->counter;
		if (multiplier <= endCycles) {
			nextEventUnit->event();
			setEvent();
		} else
			multiplier = endCycles;
		multiplier -= cycleCounter;
		
		if (out) {
			const uint32_t *const bufEnd = buf + multiplier;
			while (buf < bufEnd)
				(*buf++) += out;
		} else
			buf += multiplier;
			
		cycleCounter += multiplier;
	}
	
	if (cycleCounter & 0x80000000) {
		if (lengthCounter.counter != 0xFFFFFFFF)
			lengthCounter.counter -= cycleCounter;
		if (lfsr.counter != 0xFFFFFFFF)
			lfsr.counter -= cycleCounter;
		if (envelopeUnit.counter != 0xFFFFFFFF)
			envelopeUnit.counter -= cycleCounter;
		
		cycleCounter = 0;
	}
}
