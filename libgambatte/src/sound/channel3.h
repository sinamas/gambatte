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

class Channel3 {
	uint8_t waveRam[0x10];
	
	MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	uint32_t waveCounter;
	
	bool master;
	
	uint8_t nr0;
// 	uint8_t nr1;
// 	uint8_t nr2;
	uint8_t nr3;
	uint8_t nr4;
	uint8_t wavePos;
	uint8_t rShift;
	
public:
	Channel3();
	// void deactivate() { disableMaster(); }
	bool isActive() const { return master; }
	void reset(unsigned nr0, unsigned nr1, unsigned nr2, unsigned nr3, unsigned nr4);
	void setNr0(unsigned data);
	void setNr1(unsigned data) { lengthCounter.nr1Change(data, nr4, cycleCounter); }
	void setNr2(unsigned data);
	void setNr3(unsigned data) { nr3 = data; }
	void setNr4(unsigned data);
	void setSo(bool so1, bool so2);
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	void waveRamWrite(unsigned index, unsigned data) { if (!master) waveRam[index] = data; }
};

#endif
