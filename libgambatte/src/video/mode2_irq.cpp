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
#include "mode2_irq.h"

#include "ly_counter.h"
#include "lyc_irq.h"

Mode2Irq::Mode2Irq(const LyCounter &lyCounter_in, const LycIrq &lycIrq_in,
                   uint8_t &ifReg_in) :
	VideoEvent(0),
	lyCounter(lyCounter_in),
	lycIrq(lycIrq_in),
	ifReg(ifReg_in)
{
	reset();
}

void Mode2Irq::doEvent() {
	const unsigned ly = lyCounter.ly() == 153 ? 0 : (lyCounter.ly() + 1);
	
	if (lycIrq.time() == uint32_t(-1) || (lycIrq.lycReg() != 0 && ly != (lycIrq.lycReg() + 1U)) || (lycIrq.lycReg() == 0 && ly > 1))
		ifReg |= 2;
	
	setTime(time() + lyCounter.lineTime());
	
	if (ly == 0)
		setTime(time() - 4);
	else if (ly == 143)
		setTime(time() + lyCounter.lineTime() * 10 + 4);
}

void Mode2Irq::schedule(const LyCounter &lyCounter, const unsigned cycleCounter) {
	unsigned next = lyCounter.time() - cycleCounter;
	
	if (lyCounter.ly() >= 143 || (lyCounter.ly() == 142 && next <= 5)) {
		next += (153 - lyCounter.ly()) * lyCounter.lineTime();
		if (next <= 1)
			next += lyCounter.lineTime();
		next -= 1;
	} else {
		if (next <= 5)
			next += lyCounter.lineTime();
		next -= 5;
	}
	
	setTime(cycleCounter + next);
}
