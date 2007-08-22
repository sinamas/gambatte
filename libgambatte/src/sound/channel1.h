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
#ifndef SOUND_CHANNEL1_H
#define SOUND_CHANNEL1_H

#include "master_disabler.h"
#include "length_counter.h"
#include "duty_unit.h"
#include "envelope_unit.h"

class SweepUnit : public SoundUnit {
	MasterDisabler &disableMaster;
	uint32_t &dutyPeriod;
	uint32_t nextFrequency;

	uint8_t nr0;
	
public:
	SweepUnit(MasterDisabler &disabler, uint32_t &dutyPeriod);
	void event();
	bool nr0Change(unsigned newNr0, unsigned nr3, unsigned nr4, unsigned cycleCounter);
	bool nr4Init(unsigned nr0_in, unsigned nr3, unsigned newNr4, unsigned cycleCounter) { nr0 = nr0_in; return nr0Change(nr0_in, nr3, newNr4, cycleCounter); }
	void reset() { counter = 0xFFFFFFFF; }
};

class Channel1 {
	MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	DutyUnit dutyUnit;
	EnvelopeUnit envelopeUnit;
	SweepUnit sweepUnit;
	
	SoundUnit *nextEventUnit;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	
	bool master;
	
	uint8_t nr0;
// 	uint8_t nr1;
	uint8_t nr2;
	uint8_t nr3;
	uint8_t nr4;
	
	void setEvent();
	
public:
	Channel1();
	void setNr0(unsigned data);
	void setNr1(unsigned data);
	void setNr2(unsigned data);
	void setNr3(unsigned data) { nr3 = data; dutyUnit.nr3Change(data, nr4); }
	void setNr4(unsigned data);
	
	void setSo(bool so1, bool so2);
	// void deactivate() { disableMaster(); setEvent(); }
	bool isActive() const { return master; }
	
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	
	void reset(unsigned nr0, unsigned nr1, unsigned nr2, unsigned nr3, unsigned nr4);
};

#endif
