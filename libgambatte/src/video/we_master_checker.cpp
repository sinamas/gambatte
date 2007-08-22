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
#include "we_master_checker.h"

#include "event_queue.h"
#include "wy.h"
#include "basic_add_event.h"

#include <cstdio>

WeMasterChecker::WeMasterChecker(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
                Wy &wy_in,
                const LyCounter &lyCounter_in) :
	VideoEvent(10),
	m3EventQueue(m3EventQueue_in),
	wy(wy_in),
	lyCounter(lyCounter_in)
{
	reset();
}

void WeMasterChecker::doEvent() {
// 	if (wy.value() >= lyCounter.ly()) {
		if (!weMaster_ /*&& src */&& wy.value() == lyCounter.ly()) {
			wy.weirdAssWeMasterEnableOnWyLineCase();
			addEvent(wy.reader4(), lyCounter, time(), m3EventQueue);
		}
		
		weMaster_ |= true;
// 	}
	
	setTime(time() + (70224U << lyCounter.isDoubleSpeed()));
}

void addEvent(WeMasterChecker &event, const unsigned wy, const bool we, const unsigned cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	const unsigned oldTime = event.time();
	event.schedule(wy, we, cycleCounter);
	
	if (oldTime != event.time()) {
		if (oldTime == uint32_t(-1))
			queue.push(&event);
		else if (event.time() == uint32_t(-1))
			queue.remove(&event);
		else if (event.time() > oldTime)
			queue.inc(&event, &event);
		else
			queue.dec(&event, &event);
	}
}
