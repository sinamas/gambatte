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

Channel3::Channel3() :
	disableMaster(master, waveCounter, waveCounter, waveCounter),
	lengthCounter(disableMaster, 0xFF)
{}

void Channel3::setNr0(const unsigned data) {
	nr0 = data;
	
	if (master && !(data & 0x80))
		disableMaster();
}

void Channel3::setNr2(const unsigned data) {
	rShift = ((data >> 5) & 3U) - 1;
	
	if (rShift > 4)
		rShift = 4;
}

void Channel3::setNr4(const unsigned data) {
	lengthCounter.nr4Change(nr4, data, cycleCounter);
		
	nr4 = data;
	
	if (data & 0x80) { //init-bit
		nr4 &= 0x7F;
		
		if (!master && (nr0 & 0x80)) {
			master = true;
			wavePos = 0x1F;
			waveCounter = cycleCounter;
		}
	}
}

void Channel3::setSo(const bool so1, const bool so2) {
	soMask = (so1 ? 0xFFFF0000 : 0) | (so2 ? 0xFFFF : 0);
}

void Channel3::reset(const unsigned nr0_data, const unsigned nr1_data, const unsigned nr2_data, const unsigned nr3_data, const unsigned nr4_data) {
	nr0 = nr0_data;
// 	nr1 = nr1_data;
// 	nr2 = nr2_data;
	nr3 = nr3_data;
	nr4 = nr4_data;
	
	cycleCounter = 0;

	lengthCounter.reset(nr1_data);
	setNr2(nr2_data);
	
	disableMaster();
}

void Channel3::update(uint32_t *buf, const unsigned soBaseVol, unsigned cycles) {
	const unsigned outBase = soBaseVol & soMask;
	
	const unsigned endCycles = cycleCounter + cycles;
	
	while (cycleCounter < endCycles) {
		const unsigned out = master ? outBase * (((waveRam[wavePos / 2] >> ((~wavePos & 1) * 4)) & 0xF) >> rShift) : 0;
		
		unsigned multiplier = endCycles;
		
		if (waveCounter <= multiplier || lengthCounter.counter <= multiplier) {
			if (lengthCounter.counter < waveCounter) {
				multiplier = lengthCounter.counter;
				lengthCounter.event();
			} else {
				multiplier = waveCounter;
				waveCounter += 2048 - (nr3 | ((nr4 & 0x7) << 8));
				++wavePos;
				wavePos &= 0x1F;
			}
		}
		
		multiplier -= cycleCounter;
		
		if (out) {
			unsigned n = multiplier;
			
			while (n--)
				(*buf++) += out;
		} else
			buf += multiplier;
			
		cycleCounter += multiplier;
	}
	
	if (cycleCounter & 0x80000000) {
		if (lengthCounter.counter != 0xFFFFFFFF)
			lengthCounter.counter -= cycleCounter;
		if (waveCounter != 0xFFFFFFFF)
			waveCounter -= cycleCounter;
		
		cycleCounter = 0;
	}
}
