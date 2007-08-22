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
#include "length_counter.h"
#include "master_disabler.h"

LengthCounter::LengthCounter(MasterDisabler &disabler, const unsigned mask) :
	disableMaster(disabler),
	lengthMask(mask)
{}

void LengthCounter::event() {
	counter = 0xFFFFFFFF;
	lengthCounter = 0;
	disableMaster();
}

void LengthCounter::nr1Change(const unsigned newNr1, const unsigned nr4, const unsigned cycleCounter) {
	lengthCounter = ((~newNr1 & lengthMask) + 1) * 8192;
	counter = (nr4 & 0x40) ? cycleCounter + lengthCounter : 0xFFFFFFFF;
}

void LengthCounter::nr4Change(const unsigned oldNr4, const unsigned newNr4, const unsigned cycleCounter) {
	if ((newNr4 & 0x40) == 0) {
		if (counter != 0xFFFFFFFF) {
			lengthCounter = counter - cycleCounter;
			counter = 0xFFFFFFFF;
		}
	} else {
// 		if (((oldNr4 & 0x40) == 0 || (newNr4 & 0x80)) && lengthCounter == 0) {
// 			lengthCounter = 0x40 * 8192;
// 			counter = cycleCounter + lengthCounter;
// 		} else if ((oldNr4 & 0x40) == 0 && lengthCounter != 0)
// 			counter = cycleCounter + lengthCounter;
	
		switch((((newNr4 & 0x80) | (oldNr4 /*& 0x40*/)) >> 6) | (lengthCounter == 0 ? 4 : 0)) {
		case 0x7:
		case 0x6:
		case 0x4: lengthCounter = (lengthMask + 1) * 8192;
		case 0x2:
		case 0x0: counter = cycleCounter + lengthCounter; break;
		default: break;
		}
	}
}

void LengthCounter::reset(const unsigned nr1) {
	nr1Change(nr1, 0, 0);
}
