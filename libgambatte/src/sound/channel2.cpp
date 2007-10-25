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
#include "channel2.h"

Channel2::Channel2() :
	disableMaster(master),
	lengthCounter(disableMaster, 0x3F)
{}

void Channel2::setEvent() {
	nextEventUnit = &dutyUnit;
	if (envelopeUnit.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &envelopeUnit;
	if (lengthCounter.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &lengthCounter;
}

void Channel2::setNr1(const unsigned data) {
	lengthCounter.nr1Change(data, nr4, cycleCounter);
	dutyUnit.nr1Change(data, cycleCounter);
	
	setEvent();
}

void Channel2::setNr2(const unsigned data) {
	if (envelopeUnit.nr2Change(data))
		disableMaster();
}

void Channel2::setNr3(const unsigned data) {
	dutyUnit.nr3Change(data, cycleCounter);
	setEvent();
}

void Channel2::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		master = !envelopeUnit.nr4Init(cycleCounter);
	}
	
	dutyUnit.nr4Change(data, cycleCounter);
	
	setEvent();
}

void Channel2::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel2::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
// 	lengthCounter.reset();
	dutyUnit.reset();
	envelopeUnit.reset();
	
	setEvent();
}

void Channel2::init(const unsigned cc, const bool cgb) {
	nr4 = 0;
	cycleCounter = 0x1000 | cc & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	master = false;
	
	dutyUnit.init(cycleCounter);
	lengthCounter.init(cgb);
	envelopeUnit.init(false, cycleCounter);
	
	setEvent();
}

void Channel2::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = envelopeUnit.dacIsOn() ? soBaseVol & soMask : 0;
	
	const unsigned endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned out = outBase * ((master && dutyUnit.isHighState()) ? envelopeUnit.getVolume() * 2 - 15 : 0 - 15);
		
		unsigned multiplier = nextEventUnit->getCounter();
		
		if (multiplier <= endCycles) {
			nextEventUnit->event();
			setEvent();
		} else
			multiplier = endCycles;
		
		multiplier -= cycleCounter;
		cycleCounter += multiplier;
		
		uint32_t *const bufend = buf + multiplier;
		
		if (out) {
			while (buf != bufend)
				(*buf++) += out;
		}
		
		buf = bufend;
	}
	
	if (cycleCounter & 0x80000000) {
		dutyUnit.resetCounters(cycleCounter);
		lengthCounter.resetCounters(cycleCounter);
		envelopeUnit.resetCounters(cycleCounter);
		
		cycleCounter -= 0x80000000;
	}
}
