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

SweepUnit::SweepUnit(MasterDisabler &disabler, uint32_t &period) :
	disableMaster(disabler),
	dutyPeriod(period)
{}

void SweepUnit::event() {
	counter += (nr0 & 0x70) * 1024;
	
	unsigned freq = nextFrequency;
	
	dutyPeriod = (2048 - freq) * 2;
	
	const unsigned inc = freq >> (nr0 & 0x7);
	
	if (inc == 0)
		counter = 0xFFFFFFFF;
	else {
		freq = (nr0 & 0x8) ? freq - inc : freq + inc;
		nextFrequency = freq;
		
		if (freq & 2048)
			disableMaster();
	}
}

inline bool SweepUnit::nr0Change(const unsigned newNr0, const unsigned nr3, const unsigned nr4, const unsigned cycleCounter) {
	unsigned newSweepTime = (newNr0 & 0x70) * 1024;
	
	if (newSweepTime != 0) {
		const unsigned rshift = newNr0 & 0x7;
		if (rshift == 0)
			newSweepTime = 0;
			
		unsigned freq = nr3 | ((nr4 & 0x7) << 8);
		const unsigned inc = freq >> rshift;
		if (newNr0 & 0x8)
			freq -= inc;
		else {
			freq += inc;
			if ((nr0 & 0x8) && counter != 0xFFFFFFFF)
				return true;
		}
		nextFrequency = freq;
		
		if (freq & 2048)
			return true;
	}
		
	counter = newSweepTime ? cycleCounter + newSweepTime : 0xFFFFFFFF;
	nr0 = newNr0;
	
	return false;
}

Channel1::Channel1() :
	disableMaster(master, dutyUnit.counter, envelopeUnit.counter, sweepUnit.counter),
	lengthCounter(disableMaster, 0x3F),
	sweepUnit(disableMaster, dutyUnit.period)
{}

void Channel1::setEvent() {
	nextEventUnit = &dutyUnit;
	if (envelopeUnit.counter < nextEventUnit->counter)
		nextEventUnit = &envelopeUnit;
	if (sweepUnit.counter < nextEventUnit->counter)
		nextEventUnit = &sweepUnit;
	if (lengthCounter.counter < nextEventUnit->counter)
		nextEventUnit = &lengthCounter;
}

void Channel1::setNr0(const unsigned data) {
	nr0 = data;
	
	if (master) {
		if (sweepUnit.nr0Change(data, nr3, nr4, cycleCounter))
			disableMaster();
		
		setEvent();
	}
}

void Channel1::setNr1(const unsigned data) {
// 	nr1 = data;
	
	lengthCounter.nr1Change(data, nr4, cycleCounter);
	dutyUnit.nr1Change(data);
	
	setEvent();
}

void Channel1::setNr2(const unsigned data) {
	nr2 = data;
	
	if (master) {
		if (envelopeUnit.nr2Change(data, cycleCounter))
			disableMaster();
			
		setEvent();
	}
}

void Channel1::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		master = envelopeUnit.nr4Init(nr2, cycleCounter) == false;
		if (master && sweepUnit.nr4Init(nr0, nr3, data, cycleCounter))
			disableMaster();
	}
	
	if (master)
		dutyUnit.nr4Change(nr3, data, cycleCounter);
	
	setEvent();
}

void Channel1::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel1::reset(const unsigned nr0_data, const unsigned nr1_data, const unsigned nr2_data, const unsigned nr3_data, const unsigned nr4_data) {
	nr0 = nr0_data;
// 	nr1 = nr1_data;
	nr2 = nr2_data;
	nr3 = nr3_data;
	nr4 = nr4_data;
	
	cycleCounter = 0;
	master = true;

	lengthCounter.reset(nr1_data);
	dutyUnit.reset(nr1_data, nr3_data, nr4_data);
	envelopeUnit.reset(nr2_data);
	sweepUnit.reset();
	
	if (nr2_data == 0)
		disableMaster();
	
	setEvent();
}

void Channel1::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = soBaseVol & soMask;
	
	const unsigned endCycles = cycleCounter + cycles;
	while (cycleCounter < endCycles) {
		const unsigned out = 16 * soBaseVol + ((master && dutyUnit.isHighState()) ? outBase * envelopeUnit.getVolume() : 0);
		
		unsigned multiplier = nextEventUnit->counter;
		if (multiplier <= endCycles) {
			nextEventUnit->event();
			setEvent();
		} else
			multiplier = endCycles;
		multiplier -= cycleCounter;
		
		if (out)
			std::fill_n(buf, multiplier, out);
			
		buf += multiplier;
		cycleCounter += multiplier;
	}
	
	if (cycleCounter & 0x80000000) {
		if (lengthCounter.counter != 0xFFFFFFFF)
			lengthCounter.counter -= cycleCounter;
		if (dutyUnit.counter != 0xFFFFFFFF)
			dutyUnit.counter -= cycleCounter;
		if (envelopeUnit.counter != 0xFFFFFFFF)
			envelopeUnit.counter -= cycleCounter;
		if (sweepUnit.counter != 0xFFFFFFFF)
			sweepUnit.counter -= cycleCounter;
		
		cycleCounter = 0;
	}
}
