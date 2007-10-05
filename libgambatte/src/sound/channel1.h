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

class Channel1 {
	class SweepUnit : public SoundUnit {
		MasterDisabler &disableMaster;
		DutyUnit &dutyUnit;
		uint16_t shadow;
		uint8_t nr0;
		bool negging;
		
		unsigned calcFreq();
		
	public:
		SweepUnit(MasterDisabler &disabler, DutyUnit &dutyUnit);
		void event();
		void nr0Change(unsigned newNr0);
		void nr4Init(unsigned cycleCounter);
		void init();
		void reset();
	};
	
	MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	DutyUnit dutyUnit;
	EnvelopeUnit envelopeUnit;
	SweepUnit sweepUnit;
	
	SoundUnit *nextEventUnit;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	
	bool master;
	
	uint8_t nr4;
	
	void setEvent();
	
public:
	Channel1();
	void setNr0(unsigned data);
	void setNr1(unsigned data);
	void setNr2(unsigned data);
	void setNr3(unsigned data);
	void setNr4(unsigned data);
	
	void setSo(bool so1, bool so2);
	bool isActive() const { return master; }
	
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	
	void reset();
	void init(unsigned cc, bool cgb);
};

#endif
