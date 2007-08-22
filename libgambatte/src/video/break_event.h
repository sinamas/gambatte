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
#ifndef BREAK_EVENT_H
#define BREAK_EVENT_H

#include <stdint.h>
#include "video_event.h"
#include "ly_counter.h"

class BreakEvent : public VideoEvent {
	uint8_t &drawStartCycle;
	uint8_t &scReadOffset;
	
	uint8_t scxSrc;
	uint8_t baseCycle;
	
public:
	BreakEvent(uint8_t &drawStartCycle_in, uint8_t &scReadOffset_in);
	
	void doEvent();
	
	void reset() {
		doEvent();
	}
	
	void schedule(const LyCounter &lyCounter, unsigned) {
		setTime(lyCounter.time());
	}
	
	void setDoubleSpeed(const bool dS) {
		baseCycle = 90 + dS * 4;
	}
	
	void setScxSource(const unsigned scxSrc_in) {
		scxSrc = scxSrc_in;
	}
};

#endif
