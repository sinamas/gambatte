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
{
	init(false);
}

void LengthCounter::event() {
	counter = 0xFFFFFFFF;
	lengthCounter = 0;
	disableMaster();
}

void LengthCounter::nr1Change(const unsigned newNr1, const unsigned nr4, const unsigned cycleCounter) {
	lengthCounter = (~newNr1 & lengthMask) + 1;
	counter = (nr4 & 0x40) ? (cycleCounter >> 13) + lengthCounter << 13 : 0xFFFFFFFF;
}

void LengthCounter::nr4Change(const unsigned oldNr4, const unsigned newNr4, const unsigned cycleCounter) {
	if (counter != 0xFFFFFFFF)
		lengthCounter = (counter >> 13) - (cycleCounter >> 13);
	
	{
		unsigned dec = 0;
		
		if (newNr4 & 0x40) {
			dec = ~cycleCounter >> 12 & 1;
			
			if (!(oldNr4 & 0x40) && lengthCounter) {
				if (!(lengthCounter -= dec))
					disableMaster();
			}
		}
		
		if ((newNr4 & 0x80) && !lengthCounter)
			lengthCounter = lengthMask + 1 - dec;
	}
	
	if ((newNr4 & 0x40) && lengthCounter)
		counter = (cycleCounter >> 13) + lengthCounter << 13;
	else
		counter = 0xFFFFFFFF;
}

/*void LengthCounter::reset() {
	counter = 0xFFFFFFFF;
	
	if (cgb)
		lengthCounter = lengthMask + 1;
}*/

void LengthCounter::init(const bool cgb) {
	this->cgb = cgb;
	nr1Change(0, 0, 0);
}
