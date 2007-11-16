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

static unsigned long toPeriod(const unsigned nr3) {
	unsigned s = (nr3 >> 4) + 3;
	unsigned r = nr3 & 7;
	
	if (!r) {
		r = 1;
		--s;
	}
	
	return r << s;
}

void Channel4::Lfsr::updateBackupCounter(const unsigned long cc) {
	/*if (backupCounter <= cc) {
		const unsigned long period = toPeriod(nr3);
		backupCounter = cc - (cc - backupCounter) % period + period;
	}*/
	
	if (backupCounter <= cc) {
		const unsigned long period = toPeriod(nr3);
		unsigned long periods = (cc - backupCounter) / period + 1;
		
		backupCounter += periods * period;
		
		if (master && nr3 < 0xE0) {
			if (nr3 & 8) {
				while (periods > 6) {
					const unsigned xored = (reg << 1 ^ reg) & 0x7E;
					reg = reg >> 6 & ~0x7E | xored | xored << 8;
					periods -= 6;
				}
				
				const unsigned xored = (reg ^ reg >> 1) << 7 - periods & 0x7F;
				reg = reg >> periods & ~(0x80 - (0x80 >> periods)) | xored | xored << 8;
			} else {
				while (periods > 15) {
					reg = reg ^ reg >> 1;
					periods -= 15;
				}
				
				reg = reg >> periods | (reg ^ reg >> 1) << 15 - periods & 0x7FFF;
			}
		}
	}
}

void Channel4::Lfsr::reviveCounter(const unsigned long cc) {
	updateBackupCounter(cc);
	counter = backupCounter;
}

/*static const unsigned char nextStateDistance[0x40] = {
	6, 1, 1, 2, 2, 1, 1, 3,
	3, 1, 1, 2, 2, 1, 1, 4,
	4, 1, 1, 2, 2, 1, 1, 3,
	3, 1, 1, 2, 2, 1, 1, 5,
	5, 1, 1, 2, 2, 1, 1, 3,
	3, 1, 1, 2, 2, 1, 1, 4,
	4, 1, 1, 2, 2, 1, 1, 3,
	3, 1, 1, 2, 2, 1, 1, 6,
};*/

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
	
	
	/*if (nr3 < 0xE0) {
		const unsigned periods = nextStateDistance[reg & 0x3F];
		const unsigned xored = (reg ^ reg >> 1) << 7 - periods & 0x7F;
		
		reg = reg >> periods | xored << 8;
		
		if (nr3 & 8)
			reg = reg & ~(0x80 - (0x80 >> periods)) | xored;
	}
	
	const unsigned long period = toPeriod(nr3);
	backupCounter = counter + period;
	counter += period * nextStateDistance[reg & 0x3F];*/
}

void Channel4::Lfsr::nr3Change(const unsigned newNr3, const unsigned long cc) {
	updateBackupCounter(cc);
	nr3 = newNr3;
	
// 	if (counter != COUNTER_DISABLED)
// 		counter = backupCounter + toPeriod(nr3) * (nextStateDistance[reg & 0x3F] - 1);
}

void Channel4::Lfsr::nr4Init(unsigned long cc) {
	disableMaster();
	updateBackupCounter(cc);
	master = true;
	backupCounter += 4;
	counter = backupCounter;
// 	counter = backupCounter + toPeriod(nr3) * (nextStateDistance[reg & 0x3F] - 1);
}

void Channel4::Lfsr::init(const unsigned long cc) {
	nr3 = 0;
	disableMaster();
	backupCounter = cc + toPeriod(nr3);
}

void Channel4::Lfsr::resetCounters(const unsigned long oldCc) {
	updateBackupCounter(oldCc);
	backupCounter -= COUNTER_MAX;
	SoundUnit::resetCounters(oldCc);
}

Channel4::Channel4() :
	staticOutputTest(*this, lfsr),
	disableMaster(master, lfsr),
	lengthCounter(disableMaster, 0x3F),
	envelopeUnit(staticOutputTest)
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
	if (envelopeUnit.nr2Change(data))
		disableMaster();
	else
		staticOutputTest(cycleCounter);
	
	setEvent();
}

void Channel4::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		
		master = !envelopeUnit.nr4Init(cycleCounter);
		
		if (master)
			lfsr.nr4Init(cycleCounter);
		
		staticOutputTest(cycleCounter);
	}
	
	setEvent();
}

void Channel4::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
	staticOutputTest(cycleCounter);
	setEvent();
}

void Channel4::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.

// 	lengthCounter.reset();
	lfsr.reset(cycleCounter);
	envelopeUnit.reset();
	
	setEvent();
}

void Channel4::init(const unsigned long cc, const bool cgb) {
	nr4 = 0;
	
	cycleCounter = 0x1000 | cc & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
	lfsr.init(cycleCounter);
	lengthCounter.init(cgb);
	envelopeUnit.init(false, cycleCounter);
	
	disableMaster();
	
	setEvent();
}

void Channel4::update(uint32_t *buf, const unsigned long soBaseVol, unsigned long cycles) {
	const unsigned long outBase = envelopeUnit.dacIsOn() ? soBaseVol & soMask : 0;
	const unsigned long endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned long out = outBase * ((/*master && */lfsr.isHighState()) ? envelopeUnit.getVolume() * 2 - 15 : 0 - 15);
		
		unsigned long multiplier = nextEventUnit->getCounter();
		
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
	
	if (cycleCounter & SoundUnit::COUNTER_MAX) {
		lengthCounter.resetCounters(cycleCounter);
		lfsr.resetCounters(cycleCounter);
		envelopeUnit.resetCounters(cycleCounter);
		
		cycleCounter -= SoundUnit::COUNTER_MAX;
	}
}
