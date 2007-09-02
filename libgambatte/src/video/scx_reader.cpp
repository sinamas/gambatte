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
#include "scx_reader.h"

#include "../event_queue.h"

ScxReader::ScxReader(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
//                      VideoEvent &wyReader3_in,
                     VideoEvent &wxReader_in,
                     VideoEvent &weEnableChecker_in,
                     VideoEvent &weDisableChecker_in) :
	VideoEvent(1),
	m3EventQueue(m3EventQueue_in),
// 	wyReader3(wyReader3_in),
	wxReader(wxReader_in),
	weEnableChecker(weEnableChecker_in),
	weDisableChecker(weDisableChecker_in)
{
	setDoubleSpeed(false);
	setSource(0);
	reset();
}

void ScxReader::rescheduleEvent(VideoEvent& event, const unsigned diff) {
	if (event.time() != uint32_t(-1)) {
		event.setTime(event.time() + diff);
		(diff & 0x10) ? m3EventQueue.dec(&event, &event) : m3EventQueue.inc(&event, &event);
	}
}

void ScxReader::doEvent() {
	const unsigned diff = src - scxAnd7_ << dS;
	scxAnd7_ = src;
	
// 	rescheduleEvent(wyReader3, diff);
	rescheduleEvent(wxReader, diff);
	rescheduleEvent(weEnableChecker, diff);
	rescheduleEvent(weDisableChecker, diff);
	
	setTime(uint32_t(-1));
}
