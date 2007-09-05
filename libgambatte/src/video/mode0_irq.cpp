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
#include "mode0_irq.h"

#include "ly_counter.h"
#include "lyc_irq.h"
#include "m3_extra_cycles.h"

Mode0Irq::Mode0Irq(const LyCounter &lyCounter_in, const LycIrq &lycIrq_in, 
                   const M3ExtraCycles &m3ExtraCycles_in, uint8_t &ifReg_in) :
	VideoEvent(0),
	lyCounter(lyCounter_in),
	lycIrq(lycIrq_in),
	m3ExtraCycles(m3ExtraCycles_in),
	ifReg(ifReg_in)
{
	reset();
}

void Mode0Irq::doEvent() {
	if (lycIrq.time() == uint32_t(-1) || lyCounter.ly() != lycIrq.lycReg())
		ifReg |= 2;
	
	unsigned m3ec;
	
	if (lyCounter.ly() == 143) {
		m3ec = m3ExtraCycles(0);
		setTime(time() + lyCounter.lineTime() * 11);
	} else {
		m3ec = m3ExtraCycles(lyCounter.ly() + 1);
		setTime(time() + lyCounter.lineTime());
	}
	
	m3ec <<= lyCounter.isDoubleSpeed();
	setTime(time() + m3ec - lastM3ExtraCycles);
	lastM3ExtraCycles = m3ec;
}

void Mode0Irq::mode3CyclesChange() {
	unsigned ly = lyCounter.ly();
	if (time() >= lyCounter.time()) {
		++ly;
		if (ly > 143)
			ly = 0;
	}
	
	const unsigned m3ec = m3ExtraCycles(ly) << lyCounter.isDoubleSpeed();
	
	setTime(time() + m3ec - lastM3ExtraCycles);
	lastM3ExtraCycles = m3ec;
}

void Mode0Irq::schedule(const LyCounter &lyCounter, const unsigned cycleCounter) {
	const unsigned lineCycles = 456 * 2 - ((lyCounter.time() - cycleCounter) << (1 ^ lyCounter.isDoubleSpeed()));
	unsigned line = lyCounter.ly();
	int next = ((169 + lyCounter.isDoubleSpeed() * 3 + 80 + 1 - lyCounter.isDoubleSpeed()) * 2 - lyCounter.isDoubleSpeed()) - lineCycles;
	unsigned m3ExCs;
	
	if (line < 144) {
		m3ExCs = m3ExtraCycles(line) * 2;
		next += m3ExCs;
		if (next <= 0) {
			next += 456 * 2 - m3ExCs;
			++line;
			if (line < 144) {
				m3ExCs = m3ExtraCycles(line) * 2;
				next += m3ExCs;
			}
		}
	}
	
	if (line > 143) {
		m3ExCs = m3ExtraCycles(0) * 2;
		next += (154 - line) * 456 * 2 + m3ExCs;
	}
	
	lastM3ExtraCycles = m3ExCs >> (1 ^ lyCounter.isDoubleSpeed());
	
	setTime(cycleCounter + (next >> (1 ^ lyCounter.isDoubleSpeed())));
}
