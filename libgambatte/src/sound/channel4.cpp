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

static unsigned toPeriod(const unsigned nr3) {
	unsigned s = (nr3 >> 4) + 3;
	unsigned r = nr3 & 7;
	
	if (!r) {
		r = 1;
		--s;
	}
	
	return r << s;
}

void Channel4::Lfsr::updateBackupCounter(const unsigned cc) {
	if (backupCounter <= cc) {
		const unsigned period = toPeriod(nr3);
		backupCounter = cc - (cc - backupCounter) % period + period;
	}
}

void Channel4::Lfsr::event() {
	if (nr3 < 0xE0) {
		const unsigned shifted = reg >> 1;
		const unsigned xored = (reg ^ shifted) & 1;
		
		reg = shifted | xored << 14;
		
		if (nr3 & 8)
			reg = reg & ~0x40 | xored << 6;
	}
	
	counter += toPeriod(nr3);
	backupCounter = counter;
}

void Channel4::Lfsr::nr3Change(const unsigned newNr3, const unsigned cc) {
	updateBackupCounter(cc);
	nr3 = newNr3;
}

void Channel4::Lfsr::nr4Init(unsigned cc) {
	reg = 0xFF;
	updateBackupCounter(cc);
	backupCounter += 4;
	counter = backupCounter;
}

void Channel4::Lfsr::init(const unsigned cc) {
	nr3 = 0;
	reg = 0xFF;
	backupCounter = cc + toPeriod(nr3);
	counter = 0xFFFFFFFF;
}

void Channel4::Lfsr::resetCounters(const unsigned oldCc) {
	updateBackupCounter(oldCc);
	backupCounter -= 0x80000000;
	SoundUnit::resetCounters(oldCc);
}

Channel4::Channel4() :
	disableMaster(master, lfsr),
	lengthCounter(disableMaster, 0x3F)
{}

void Channel4::setEvent() {
	nextEventUnit = &lfsr;
	if (envelopeUnit.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &envelopeUnit;
	if (lengthCounter.getCounter() < nextEventUnit->getCounter())
		nextEventUnit = &lengthCounter;
}

void Channel4::setNr1(const unsigned data) {
	lengthCounter.nr1Change(data, nr4, cycleCounter);
	
	setEvent();
}

void Channel4::setNr2(const unsigned data) {
	if (envelopeUnit.nr2Change(data)) {
		disableMaster();
		setEvent();
	}
}

void Channel4::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		
		master = !envelopeUnit.nr4Init(cycleCounter);
		
		if (master)
			lfsr.nr4Init(cycleCounter);
	}
	
	setEvent();
}

void Channel4::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel4::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.

// 	lengthCounter.reset();
	lfsr.reset(cycleCounter);
	envelopeUnit.reset();
	
	setEvent();
}

void Channel4::init(const unsigned cc, const bool cgb) {
	nr4 = 0;
	
	cycleCounter = 0x1000 | cc & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
	lfsr.init(cycleCounter);
	lengthCounter.init(cgb);
	envelopeUnit.init(false, cycleCounter);
	
	disableMaster();
	
	setEvent();
}

void Channel4::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = envelopeUnit.dacIsOn() ? soBaseVol & soMask : 0;
	
	const unsigned endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned out = outBase * ((master && lfsr.isHighState()) ? envelopeUnit.getVolume() * 2 - 15 : 0 - 15);
		
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
		lengthCounter.resetCounters(cycleCounter);
		lfsr.resetCounters(cycleCounter);
		envelopeUnit.resetCounters(cycleCounter);
		
		cycleCounter -= 0x80000000;
	}
}
