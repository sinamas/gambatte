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

class Channel4 {
	class Lfsr : public SoundUnit {
		uint32_t backupCounter;
		uint16_t reg;
		uint8_t nr3;
		
		void updateBackupCounter(unsigned cc);
		
	public:
		Lfsr() {}
		void event();
		bool isHighState() const { return ~reg & 1; }
		void nr3Change(unsigned newNr3, unsigned cc);
		void nr4Init(unsigned cc);
		void init(unsigned cc);
		void reset(unsigned cc) { init(cc); }
		void resetCounters(unsigned oldCc);
		void killCounter() { counter = 0xFFFFFFFF; }
	};
	
	class Ch4MasterDisabler : public MasterDisabler {
		Lfsr &lfsr;
	public:
		Ch4MasterDisabler(bool &m, Lfsr &lfsr) : MasterDisabler(m), lfsr(lfsr) {}
		void operator()() { MasterDisabler::operator()(); lfsr.killCounter(); }
	};
	
	Ch4MasterDisabler disableMaster;
	LengthCounter lengthCounter;
	EnvelopeUnit envelopeUnit;
	Lfsr lfsr;
	
	SoundUnit *nextEventUnit;
	
	uint32_t cycleCounter;
	uint32_t soMask;
	
	uint8_t nr4;
	bool master;
	
	void setEvent();
	
public:
	Channel4();
	void setNr1(unsigned data);
	void setNr2(unsigned data);
	void setNr3(unsigned data) { lfsr.nr3Change(data, cycleCounter); }
	void setNr4(unsigned data);
	
	void setSo(bool so1, bool so2);
	bool isActive() const { return master; }
	
	void update(uint32_t *buf, unsigned soBaseVol, unsigned cycles);
	
	void reset();
	void init(unsigned cc, bool cgb);
};

#endif
