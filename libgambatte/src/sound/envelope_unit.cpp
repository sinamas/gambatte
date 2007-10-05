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
	const unsigned period = nr2 & 7;
	
	if (period) {
		unsigned newVol = volume;
		
		if (nr2 & 8)
			++newVol;
		else
			--newVol;
		
		if (newVol < 0x10U) {
			volume = newVol;
			counter += period << 15;
		} else
			counter = 0xFFFFFFFF;
	} else
		counter += 8 << 15;
}

bool EnvelopeUnit::nr2Change(const unsigned newNr2) {
	if (!(nr2 & 7) && counter != 0xFFFFFFFF)
		++volume;
	else if (!(nr2 & 8))
		volume += 2;
	
	if ((nr2 ^ newNr2) & 8)
		volume = 0x10 - volume;
	
	volume &= 0xF;
	
	nr2 = newNr2;
	
	return !(newNr2 & 0xF8);
}

bool EnvelopeUnit::nr4Init(const unsigned cc) {
	{
		unsigned period = nr2 & 7;
		
		if (!period)
			period = 8;
		
		if (!(cc & 0x7000))
			++period;
		
		counter = cc - (cc - 0x1000 & 0x7FFF) + period * 0x8000;
	}
	
	volume = nr2 >> 4;
	
	return !(nr2 & 0xF8);
}

void EnvelopeUnit::reset() {
	counter = 0xFFFFFFFF;
}

void EnvelopeUnit::init(const bool ch1, const unsigned cc) {
	volume = 0;
	counter = cc - (cc - 0x1000 & 0x7FFF) + 8 * 0x8000;
	nr2 = 0;
	
	if (ch1) {
		nr2 = 0xF3;
		counter = 0xFFFFFFFF;
	}
}
