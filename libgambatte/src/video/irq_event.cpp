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
#include "irq_event.h"

IrqEvent::IrqEvent(event_queue<VideoEvent*,VideoEventComparer> &irqEventQueue_in) :
	VideoEvent(11),
	irqEventQueue(irqEventQueue_in)
{
	reset();
}

void IrqEvent::doEvent() {
	irqEventQueue.top()->doEvent();
	
	if (irqEventQueue.top()->time() == uint32_t(-1))
		irqEventQueue.pop();
	else
		irqEventQueue.modify_root(irqEventQueue.top());
	
	schedule();
}

void modifyEvent(IrqEvent &event, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	const unsigned oldTime = event.time();
	event.schedule();
	
	if (oldTime != event.time()) {
		if (event.time() > oldTime)
			queue.inc(&event, &event);
		else
			queue.dec(&event, &event);
	}
}
