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
#ifndef SPRITE_MAPPER_H
#define SPRITE_MAPPER_H

#include <stdint.h>
#include "video_event.h"
//#include "video_event_comparer.h"
#include "ly_counter.h"

template<typename T, class Comparer> class event_queue;
class SpriteSizeReader;
class ScxReader;

class SpriteMapper : public VideoEvent {
	uint16_t spritemap[144*12];
	
	const SpriteSizeReader &spriteSizeReader;
	const ScxReader &scxReader;
	
public:
	const uint8_t *const oamram;

private:
	uint8_t timeDiff;
	bool cgb;
	
	void clearMap();
	void mapSprites();
	
public:
	SpriteMapper(const SpriteSizeReader &spriteSizeReader_in,
	             const ScxReader &scxReader_in,
	             const uint8_t *const oamram_in);
	
	void doEvent();
	
	void reset();
	
	void schedule(const LyCounter &lyCounter, const unsigned cycleCounter) {
		// synced to scxReader.
		setTime(lyCounter.nextLineCycle(82 + lyCounter.isDoubleSpeed() * 3, cycleCounter));
	}
	
	void setCgb(const bool cgb_in) {
		cgb = cgb_in;
	}
	
	void setDoubleSpeed(const bool dS) {
		timeDiff = dS ? 85 * 2 - 40 : (82 - 16);
	}
	
	const uint16_t * spriteMap() const {
		return spritemap;
	}
};

//void addEvent(SpriteMapper &event, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
