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
#ifndef SOUND_CHANNEL3_H
#define SOUND_CHANNEL3_H

#include "master_disabler.h"
#include "length_counter.h"

#include <cstdio>

class Channel3 {
	class Ch3MasterDisabler : public MasterDisabler {
		uint32_t &waveCounter;
		
	public:
		Ch3MasterDisabler(bool &m, uint32_t &wC) : MasterDisabler(m), waveCounter(wC) {}
		void operator()() { MasterDisabler::operator()(); waveCounter = 0xFFFFFFFF; }
	};
	
	uint8_t waveRam[0x10];
	
	Ch3MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	uint32_t waveCounter;
	uint32_t lastReadTime;
	
	bool master;
	bool cgb;
	
	uint8_t nr0;
	uint8_t nr3;
	uint8_t nr4;
	uint8_t wavePos;
	uint8_t rShift;
	uint8_t sampleBuf;
	
public:
	Channel3();
	bool isActive() const { return master; }
	void reset();
	void init(unsigned cc, bool cgb);
	void setNr0(unsigned data);
	void setNr1(unsigned data) { lengthCounter.nr1Change(data, nr4, cycleCounter); }
	void setNr2(unsigned data);
	void setNr3(unsigned data) { nr3 = data; }
	void setNr4(unsigned data);
	void setSo(bool so1, bool so2);
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	
	uint8_t waveRamRead(unsigned index) const {
		if (master) {
			if (!cgb && cycleCounter != lastReadTime)
				return 0xFF;
			
			index = wavePos >> 1;
		}
		
		return waveRam[index];
	}
	
	void waveRamWrite(unsigned index, unsigned data) {
		if (master) {
			if (!cgb && cycleCounter != lastReadTime)
				return;
			
			index = wavePos >> 1;
		}
		
		waveRam[index] = data;
	}
};

#endif
