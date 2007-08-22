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
#include "envelope_unit.h"

void EnvelopeUnit::event() {
	counter += (nr2 & 7) * 32768;

	unsigned newVol = volume;
	
	if(nr2 & 8)
		++newVol;
	else
		--newVol;
			
	if (newVol > 0xFU)
		counter = 0xFFFFFFFF;
	else
		volume = newVol;
}

bool EnvelopeUnit::nr2Change(const unsigned newNr2, const unsigned cycleCounter) {
	switch (zombie) {
	case 0:
		if ((nr2 ^ newNr2) & 0x7) {
			const unsigned envFreq = (newNr2 & 7) * 32768;
			counter = envFreq ? cycleCounter + envFreq : 0xFFFFFFFF;
			zombie = 1;
		}
		if ((nr2 ^ newNr2) & 0x8)
			volume = 15 - volume;
		else
			++volume;
		break;
	case 1:
		if ((nr2 ^ newNr2) & 0x8)
			volume = 15 - volume;
		/*else*/ if ((newNr2 & 0x8) == 0)
			volume += 2;
		break;
	}
	volume &= 0xF;
	
	nr2 = newNr2;

	return (newNr2 & 0xF8) == 0;
}

bool EnvelopeUnit::nr4Init(const unsigned nr2_in, const unsigned cycleCounter) {
	const unsigned envFreq = (nr2_in & 7) * 32768;
	counter = envFreq ? cycleCounter + envFreq : 0xFFFFFFFF;
	zombie = envFreq != 0;
	
	volume = nr2_in >> 4;
	
	nr2 = nr2_in;
	
	return (nr2_in & 0xF8) == 0;
}

void EnvelopeUnit::reset(const unsigned nr2_in) {
	counter = 0xFFFFFFFF;
	volume = 0;
	zombie = (nr2_in & 7) != 0;
	nr2 = nr2_in;
}
