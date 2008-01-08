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
#include "wx_reader.h"

#include "../event_queue.h"
#include "m3_extra_cycles.h"

WxReader::WxReader(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue,
                   VideoEvent &weEnableChecker,
                   VideoEvent &weDisableChecker,
                   M3ExtraCycles &m3ExtraCycles) :
VideoEvent(7),
m3EventQueue(m3EventQueue),
weEnableChecker(weEnableChecker),
weDisableChecker(weDisableChecker),
m3ExtraCycles(m3ExtraCycles)
{
	setDoubleSpeed(false);
	setSource(0);
	reset();
}

void WxReader::rescheduleEvent(VideoEvent& event, const unsigned long diff) {
	if (event.time() != DISABLED_TIME) {
		event.setTime(event.time() + diff);
		(diff & 0x200) ? m3EventQueue.dec(&event, &event) : m3EventQueue.inc(&event, &event);
	}
}

void WxReader::doEvent() {
	const unsigned long diff = static_cast<unsigned long>(src_) - static_cast<unsigned long>(wx_) << dS;
	wx_ = src_;
	
	rescheduleEvent(weEnableChecker, diff);
	rescheduleEvent(weDisableChecker, diff);
	
	m3ExtraCycles.invalidateCache();
	
	setTime(DISABLED_TIME);
}

void addEvent(WxReader &event, const unsigned scxAnd7, const LyCounter &lyCounter,
		const unsigned long cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue)
{
	const unsigned long oldTime = event.time();
	
	event.schedule(scxAnd7, lyCounter, cycleCounter);
	
	if (oldTime == VideoEvent::DISABLED_TIME)
		queue.push(&event);
	else if (oldTime != event.time()) {
		if (event.time() > oldTime)
			queue.inc(&event, &event);
		else
			queue.dec(&event, &event);
	}
}
