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
#ifndef VIDEO_IRQ_EVENT_H
#define VIDEO_IRQ_EVENT_H

#include "../event_queue.h"
#include <stdint.h>
#include "video_event.h"
#include "video_event_comparer.h"

class IrqEvent : public VideoEvent {
	event_queue<VideoEvent*,VideoEventComparer> &irqEventQueue;
	
public:
	IrqEvent(event_queue<VideoEvent*,VideoEventComparer> &irqEventQueue_in);
	
	void doEvent();
	
	void reset() {
		setTime(uint32_t(-1));
	}
	
	void schedule() {
		setTime(irqEventQueue.top()->time());
	}
};

void modifyEvent(IrqEvent &event, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
