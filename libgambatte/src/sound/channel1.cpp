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
#include "channel1.h"
#include <algorithm>

Channel1::SweepUnit::SweepUnit(MasterDisabler &disabler, DutyUnit &dutyUnit) :
	disableMaster(disabler),
	dutyUnit(dutyUnit)
{}

unsigned Channel1::SweepUnit::calcFreq() {
	unsigned freq = shadow >> (nr0 & 0x07);
	
	if (nr0 & 0x08) {
		freq = shadow - freq;
		negging = true;
	} else
		freq = shadow + freq;
	
	if (freq & 2048)
		disableMaster();
	
	return freq;
}

void Channel1::SweepUnit::event() {
	const unsigned period = nr0 >> 4 & 0x07;
	
	if (period) {
		const unsigned freq = calcFreq();
		
		if (!(freq & 2048) && (nr0 & 0x07)) {
			shadow = freq;
			dutyUnit.setFreq(freq, counter);
			calcFreq();
		}
		
		counter += period << 14;
	} else
		counter += 8 << 14;
}

void Channel1::SweepUnit::nr0Change(const unsigned newNr0) {
	if (negging && !(newNr0 & 0x08))
		disableMaster();
	
	nr0 = newNr0;
}

void Channel1::SweepUnit::nr4Init(const unsigned cc) {
	negging = false;
	shadow = dutyUnit.getFreq();
	
	const unsigned period = nr0 >> 4 & 0x07;
	const unsigned shift = nr0 & 0x07;
	
	if (period | shift)
		counter = (cc >> 14) + (period ? period : 8) << 14;
	else
		counter = 0xFFFFFFFF;
	
	if (shift)
		calcFreq();
}

void Channel1::SweepUnit::init() {
	nr0 = 0;
	shadow = 0;
	negging = false;
	counter = 0xFFFFFFFF;
}

void Channel1::SweepUnit::reset() {
	counter = 0xFFFFFFFF;
}

Channel1::Channel1() :
	disableMaster(master),
	lengthCounter(disableMaster, 0x3F),
	sweepUnit(disableMaster, dutyUnit)
{}

void Channel1::setEvent() {
	nextEventUnit = &dutyUnit;
	if (sweepUnit.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &sweepUnit;
	if (envelopeUnit.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &envelopeUnit;
	if (lengthCounter.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &lengthCounter;
}

void Channel1::setNr0(const unsigned data) {
	sweepUnit.nr0Change(data);
}

void Channel1::setNr1(const unsigned data) {
	lengthCounter.nr1Change(data, nr4, cycleCounter);
	dutyUnit.nr1Change(data, cycleCounter);
	
	setEvent();
}

void Channel1::setNr2(const unsigned data) {
	if (envelopeUnit.nr2Change(data))
		disableMaster();
}

void Channel1::setNr3(const unsigned data) {
	dutyUnit.nr3Change(data, cycleCounter);
	setEvent();
}

void Channel1::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	dutyUnit.nr4Change(data, cycleCounter);
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		master = !envelopeUnit.nr4Init(cycleCounter);
		sweepUnit.nr4Init(cycleCounter);
	}
	
	setEvent();
}

void Channel1::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel1::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.

// 	lengthCounter.reset();
	dutyUnit.reset();
	envelopeUnit.reset();
	sweepUnit.reset();
	
	setEvent();
}

void Channel1::init(const unsigned cc, const bool cgb) {
	nr4 = 0;
	cycleCounter = 0x1000 | cc & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
	dutyUnit.init(cycleCounter);
	dutyUnit.nr1Change(0x80, cycleCounter);
	lengthCounter.init(cgb);
	envelopeUnit.init(true, cycleCounter);
	sweepUnit.init();
	
	master = true;
	
	setEvent();
}

void Channel1::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = envelopeUnit.dacIsOn() ? soBaseVol & soMask : 0;
	
	const unsigned endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned out = 15 * 8 * 4 * 0x00010001 +
		                     outBase * ((master && dutyUnit.isHighState()) ? envelopeUnit.getVolume() * 2 - 15 : 0 - 15);
		
		unsigned multiplier = nextEventUnit->getCounter();
		
		if (multiplier <= endCycles) {
			nextEventUnit->event();
			setEvent();
		} else
			multiplier = endCycles;
		
		multiplier -= cycleCounter;
		cycleCounter += multiplier;
		
		std::fill_n(buf, multiplier, out);
			
		buf += multiplier;
	}
	
	if (cycleCounter & 0x80000000) {
		dutyUnit.resetCounters(cycleCounter);
		lengthCounter.resetCounters(cycleCounter);
		envelopeUnit.resetCounters(cycleCounter);
		sweepUnit.resetCounters(cycleCounter);
		
		cycleCounter -= 0x80000000;
	}
}
