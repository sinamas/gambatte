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
#ifndef SOUND_CHANNEL4_H
#define SOUND_CHANNEL4_H

#include "master_disabler.h"
#include "length_counter.h"
#include "envelope_unit.h"

class Lfsr : public SoundUnit {
	uint32_t reg;
	bool highState;
	uint8_t nr3;
	
public:
	Lfsr() : highState(false) {}
	void event();
	bool isHighState() const { return highState; }
	void nr3Change(unsigned newNr3) { nr3 = newNr3; }
	void nr4Init(unsigned cycleCounter) { reg = 0xFF; counter = cycleCounter; }
	void reset(unsigned nr3_data) { nr3 = nr3_data; }
};

class Channel4 {
	MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	EnvelopeUnit envelopeUnit;
	Lfsr lfsr;
	
	SoundUnit *nextEventUnit;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	
	bool master;
	
// 	uint8_t nr1;
	uint8_t nr2;
// 	uint8_t nr3;
	uint8_t nr4;
	
	void setEvent();
	
public:
	Channel4();
	void setNr1(unsigned data);
	void setNr2(unsigned data);
	void setNr3(unsigned data) { lfsr.nr3Change(data); }
	void setNr4(unsigned data);
	
	void setSo(bool so1, bool so2);
	// void deactivate() { disableMaster(); setEvent(); }
	bool isActive() const { return master; }
	
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	
	void reset(unsigned nr1, unsigned nr2, unsigned nr3, unsigned nr4);
};

#endif
