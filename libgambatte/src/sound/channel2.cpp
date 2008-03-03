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
#include "../savestate.h"

Channel2::Channel2() :
	staticOutputTest(*this, dutyUnit),
	disableMaster(master, dutyUnit),
	lengthCounter(disableMaster, 0x3F),
	envelopeUnit(staticOutputTest),
	cycleCounter(0),
	soMask(0),
	nr4(0),
	master(false)
{
	setEvent();
}

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
	else
		staticOutputTest(cycleCounter);
	
	setEvent();
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
		staticOutputTest(cycleCounter);
	}
	
	dutyUnit.nr4Change(data, cycleCounter);
	
	setEvent();
}

void Channel2::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
	staticOutputTest(cycleCounter);
	setEvent();
}

void Channel2::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
// 	lengthCounter.reset();
	dutyUnit.reset();
	envelopeUnit.reset();
	
	setEvent();
}

void Channel2::init(const bool cgb) {
	lengthCounter.init(cgb);
}

void Channel2::saveState(SaveState &state) {
	dutyUnit.saveState(state.spu.ch2.duty, cycleCounter);
	envelopeUnit.saveState(state.spu.ch2.env);
	lengthCounter.saveState(state.spu.ch2.lcounter);
	
	state.spu.ch2.nr4 = nr4;
	state.spu.ch2.master = master;
}

void Channel2::loadState(const SaveState &state) {
	dutyUnit.loadState(state.spu.ch2.duty, state.mem.ioamhram.get()[0x116], state.spu.ch2.nr4);
	envelopeUnit.loadState(state.spu.ch2.env, state.mem.ioamhram.get()[0x117]);
	lengthCounter.loadState(state.spu.ch2.lcounter);
	
	cycleCounter = state.spu.cycleCounter;
	nr4 = state.spu.ch2.nr4;
	master = state.spu.ch2.master;
}

void Channel2::update(Gambatte::uint_least32_t *buf, const unsigned long soBaseVol, unsigned long cycles) {
	const unsigned long outBase = envelopeUnit.dacIsOn() ? soBaseVol & soMask : 0;
	const unsigned long endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned long out = outBase * ((master && dutyUnit.isHighState()) ? envelopeUnit.getVolume() * 2 - 15ul : 0 - 15ul);
		
		unsigned long multiplier = nextEventUnit->getCounter();
		
		if (multiplier <= endCycles) {
			nextEventUnit->event();
			setEvent();
		} else
			multiplier = endCycles;
		
		multiplier -= cycleCounter;
		cycleCounter += multiplier;
		
		Gambatte::uint_least32_t *const bufend = buf + multiplier;
		
		if (out) {
			while (buf != bufend)
				(*buf++) += out;
		}
		
		buf = bufend;
	}
	
	if (cycleCounter & SoundUnit::COUNTER_MAX) {
		dutyUnit.resetCounters(cycleCounter);
		lengthCounter.resetCounters(cycleCounter);
		envelopeUnit.resetCounters(cycleCounter);
		
		cycleCounter -= SoundUnit::COUNTER_MAX;
	}
}
