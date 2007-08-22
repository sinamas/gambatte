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
#ifndef VIDEO_MODE2_IRQ_H
#define VIDEO_MODE2_IRQ_H

class LycIrq;

#include <stdint.h>
#include "ly_counter.h"

class Mode2Irq : public VideoEvent {
	const LyCounter &lyCounter;
	const LycIrq &lycIrq;
	uint8_t &ifReg;
	
public:
	Mode2Irq(const LyCounter &lyCounter_in, const LycIrq &lycIrq_in,
	         uint8_t &ifReg_in);
	
	void doEvent();
	
	void reset() {
		setTime(uint32_t(-1));
	}
	
	void schedule(const LyCounter &lyCounter, unsigned cycleCounter);
};

#endif
