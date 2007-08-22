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
#include "mode3_event.h"
#include "mode0_irq.h"
#include "irq_event.h"

#include <stdint.h>

Mode3Event::Mode3Event(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
                       event_queue<VideoEvent*,VideoEventComparer> &vEventQueue_in,
                       Mode0Irq &mode0Irq_in, IrqEvent &irqEvent_in) :
	VideoEvent(1),
	m3EventQueue(m3EventQueue_in),
	vEventQueue(vEventQueue_in),
	mode0Irq(mode0Irq_in),
	irqEvent(irqEvent_in)
{
	reset();
}

void Mode3Event::doEvent() {
	m3EventQueue.top()->doEvent();
	
	if (m3EventQueue.top()->time() == uint32_t(-1))
		m3EventQueue.pop();
	else
		m3EventQueue.modify_root(m3EventQueue.top());
	
	if (mode0Irq.time() != uint32_t(-1)) {
		const unsigned oldTime = mode0Irq.time();
		mode0Irq.mode3CyclesChange();
		if (mode0Irq.time() != oldTime) {
			// position in irqEventQueue should remain the same.
			// The same may be possible for vEventQueue, with some precautions.
			if (irqEvent.time() == oldTime) {
				irqEvent.schedule();
				if (mode0Irq.time() > oldTime)
					vEventQueue.inc(&irqEvent, &irqEvent);
				else
					vEventQueue.dec(&irqEvent, &irqEvent);
			}
			
		}
	}
	
	setTime(m3EventQueue.empty() ? uint32_t(-1) : m3EventQueue.top()->time());
}

void addEvent(Mode3Event &event, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	const unsigned oldTime = event.time();
	event.schedule();
	
	if (oldTime == uint32_t(-1))
		queue.push(&event);
	else if (oldTime != event.time()) {
		if (event.time() > oldTime)
			queue.inc(&event, &event);
		else
			queue.dec(&event, &event);
	}
}
