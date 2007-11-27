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
#include "channel3.h"

#include <cstring>

static inline unsigned toPeriod(const unsigned nr3, const unsigned nr4) {
	return 0x800 - (nr4 << 8 & 0x700 | nr3);
}

Channel3::Channel3() :
	disableMaster(master, waveCounter),
	lengthCounter(disableMaster, 0xFF)
{}

void Channel3::setNr0(const unsigned data) {
	nr0 = data & 0x80;
	
	if (!(data & 0x80))
		disableMaster();
}

void Channel3::setNr2(const unsigned data) {
	rShift = (data >> 5 & 3U) - 1;
	
	if (rShift > 3)
		rShift = 4;
}

void Channel3::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data & 0x7F;
	
	if (data & nr0/* & 0x80*/) {
		if (!cgb && waveCounter == cycleCounter + 1) {
			const unsigned pos = (wavePos + 1 & 0x1F) >> 1;
			
			if (pos < 4)
				waveRam[0] = waveRam[pos];
			else
				std::memcpy(waveRam, waveRam + (pos & ~3), 4);
		}
		
		master = true;
		wavePos = 0;
		lastReadTime = waveCounter = cycleCounter + toPeriod(nr3, data) + 3;
	}
}

void Channel3::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel3::reset() {
	cycleCounter = 0x1000 | cycleCounter & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.

// 	lengthCounter.reset();
	sampleBuf = 0;
}

void Channel3::init(const unsigned long cc, const bool cgb) {
	this->cgb = cgb;

	nr0 = 0;
	nr3 = 0;
	nr4 = 0;
	
	sampleBuf = 0;
	
	cycleCounter = 0x1000 | cc & 0xFFF; // cycleCounter >> 12 & 7 represents the frame sequencer position.
	
	setNr2(0);
	
	lengthCounter.init(cgb);
	
	disableMaster();
}

void Channel3::updateWaveCounter(const unsigned long cc) {
	if (cc >= waveCounter) {
		const unsigned period = toPeriod(nr3, nr4);
		const unsigned long periods = (cc - waveCounter) / period;

		lastReadTime = waveCounter + periods * period;
		waveCounter = lastReadTime + period;

		wavePos += periods + 1;
		wavePos &= 0x1F;

		sampleBuf = waveRam[wavePos >> 1];
	}
}

void Channel3::update(uint32_t *buf, const unsigned long soBaseVol, unsigned long cycles) {
	const unsigned long outBase = (nr0/* & 0x80*/) ? soBaseVol & soMask : 0;
	
	if (outBase && rShift != 4) {
		const unsigned long endCycles = cycleCounter + cycles;
		
		while (cycleCounter < endCycles) {
			const unsigned long out = outBase * (master ? ((sampleBuf >> (~wavePos << 2 & 4) & 0xF) >> rShift) * 2 - 15ul : 0 - 15ul);
			
			unsigned long multiplier = endCycles;
			
			if (waveCounter <= multiplier || lengthCounter.getCounter() <= multiplier) {
				if (lengthCounter.getCounter() < waveCounter) {
					multiplier = lengthCounter.getCounter();
					lengthCounter.event();
				} else {
					lastReadTime = multiplier = waveCounter;
					waveCounter += toPeriod(nr3, nr4);
					++wavePos;
					wavePos &= 0x1F;
					sampleBuf = waveRam[wavePos >> 1];
				}
			}
			
			multiplier -= cycleCounter;
			cycleCounter += multiplier;
			
			uint32_t *const bufend = buf + multiplier;
			
			if (out) {
				while (buf != bufend)
					(*buf++) += out;
			}
			
			buf = bufend;
		}
	} else {
		if (outBase) {
			const unsigned long out = outBase * (0 - 15ul);
			uint32_t *const bufend = buf + cycles;
			
			while (buf != bufend)
				(*buf++) += out;
		}
		
		cycleCounter += cycles;
		
		while (lengthCounter.getCounter() <= cycleCounter) {
			updateWaveCounter(lengthCounter.getCounter());
			lengthCounter.event();
		}
		
		updateWaveCounter(cycleCounter);
	}
	
	if (cycleCounter & SoundUnit::COUNTER_MAX) {
		lengthCounter.resetCounters(cycleCounter);
		
		if (waveCounter != SoundUnit::COUNTER_DISABLED)
			waveCounter -= SoundUnit::COUNTER_MAX;
		
		lastReadTime -= SoundUnit::COUNTER_MAX;
		cycleCounter -= SoundUnit::COUNTER_MAX;
	}
}
