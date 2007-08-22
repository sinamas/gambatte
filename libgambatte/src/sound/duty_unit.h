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
#ifndef DUTY_UNIT_H
#define DUTY_UNIT_H

#include "sound_unit.h"

class DutyUnit : public SoundUnit {
public:
	uint32_t period;

private:
	bool highState;
	uint8_t duty;

	void frequencyChange(unsigned nr3, unsigned nr4);

public:
	void event();
	bool isHighState() const { return highState; }
	void nr1Change(unsigned newNr1) { duty = newNr1 >> 6; };
	void nr3Change(unsigned newNr3, unsigned nr4) { frequencyChange(newNr3, nr4); }
	void nr4Change(unsigned nr3, unsigned newNr4, unsigned cycleCounter);
	void reset(unsigned nr1, unsigned nr3, unsigned nr4);
};

#endif
