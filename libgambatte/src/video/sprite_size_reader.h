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
#ifndef SPRITE_SIZE_READER_H
#define SPRITE_SIZE_READER_H

template<typename T, class Comparer> class event_queue;
class SpriteMapper;

#include "video_event.h"
#include "ly_counter.h"
#include "video_event_comparer.h"

class SpriteSizeReader : public VideoEvent {
	event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue;
	SpriteMapper &spriteMapper;
	const LyCounter &lyCounter;
	
	bool largeSprites_;
	bool src_;
	
public:
	SpriteSizeReader(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
	                 SpriteMapper &spriteMapper_in, const LyCounter &lyCounter_in);
	
	void doEvent();
	
	bool largeSprites() const {
		return largeSprites_;
	}
	
	void reset() {
		largeSprites_ = src_;
		setTime(uint32_t(-1));
	}
	
	void setSource(const bool src) {
		src_ = src;
	}
	
	void schedule(const LyCounter &lyCounter, const unsigned cycleCounter) {
		setTime(lyCounter.nextLineCycle(16 + lyCounter.isDoubleSpeed() * 4, cycleCounter));
	}
};

#endif
