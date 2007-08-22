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
#ifndef WE_MASTER_CHECKER_H
#define WE_MASTER_CHECKER_H

template<typename T, class Comparer> class event_queue;
class Wy;

#include <stdint.h>
#include "video_event.h"
#include "video_event_comparer.h"
#include "ly_counter.h"

class WeMasterChecker : public VideoEvent {
	event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue;
	Wy &wy;
	const LyCounter &lyCounter;
	
	bool weMaster_;
	
public:
	WeMasterChecker(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
	                Wy &wy_in,
	                const LyCounter &lyCounter_in);
	
	void doEvent();
	
	void reset() {
		unset();
		setTime(uint32_t(-1));
	}
	
	void schedule(const unsigned wy, const bool we, const unsigned cycleCounter) {
		if (we && wy < 143)
			setTime(lyCounter.nextFrameCycle(wy * 456 + 448 + lyCounter.isDoubleSpeed() * 4, cycleCounter));
		else
			setTime(uint32_t(-1));
	}
	
	void unset() {
		weMaster_ = false;
	}
	
	bool weMaster() const {
		return weMaster_;
	}
};

void addEvent(WeMasterChecker &event, unsigned wy, bool we, unsigned cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
