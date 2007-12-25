/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#include "sc_reader.h"

#include "../event_queue.h"

ScReader::ScReader() : VideoEvent(2) {
	setDoubleSpeed(false);
	setScxSource(0);
	setScySource(0);
	reset();
}

void ScReader::doEvent() {
	scy_[0] = scy_[1];
	scy_[1] = scySrc;
	scx_[0] = scx_[1];
	scx_[1] = scxSrc;
	
	if ((scy_[0] ^ scy_[1]) | (scx_[0] ^ scx_[1]))
		setTime(time() + incCycles);
	else
		setTime(DISABLED_TIME);
	
}

void ScReader::reset() {
	scx_[1] = scx_[0] = scxSrc;
	scy_[1] = scy_[0] = scySrc;
	setTime(DISABLED_TIME);
}

void ScReader::schedule(const unsigned long lastUpdate, const unsigned long videoCycles, const unsigned scReadOffset) {
	setTime(lastUpdate + ((8u - ((videoCycles - scReadOffset) & 7)) << dS));
}

void ScReader::setDoubleSpeed(const bool dS_in) {
	dS = dS_in;
	incCycles = 8u << dS_in;
}

void addEvent(ScReader &event, const unsigned long lastUpdate, const unsigned long videoCycles,
              const unsigned scReadOffset, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	const unsigned long oldTime = event.time();
	
	event.schedule(lastUpdate, videoCycles, scReadOffset);
	
	if (oldTime == VideoEvent::DISABLED_TIME)
		queue.push(&event);
	else if (oldTime != event.time()) {
		if (event.time() > oldTime)
			queue.inc(&event, &event);
		else
			queue.dec(&event, &event);
	}
}
