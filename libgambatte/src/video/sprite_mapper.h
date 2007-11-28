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
	enum { CYCLES_INVALID = 0xFF };
	
	mutable unsigned short spritemap[144*10];
	mutable unsigned char cycles[144];
	unsigned char num[144];
	
	const SpriteSizeReader &spriteSizeReader;
	const ScxReader &scxReader;
	
public:
	const uint8_t *const oamram;

private:
	unsigned char timeDiff;
	bool cgb;
	
	void clearMap();
	void mapSprites();
	void updateLine(unsigned ly) const;
	
public:
	SpriteMapper(const SpriteSizeReader &spriteSizeReader_in,
	             const ScxReader &scxReader_in,
	             const uint8_t *const oamram_in);
	
	void doEvent();
	
	void reset();
	
	void schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
		// synced to scxReader.
		setTime(lyCounter.nextLineCycle(82 + lyCounter.isDoubleSpeed() * 3, cycleCounter));
	}
	
	void setCgb(const bool cgb_in) {
		cgb = cgb_in;
	}
	
	void setDoubleSpeed(const bool dS) {
		timeDiff = dS ? 85 * 2 - 40 : (82 - 16);
	}
	
	unsigned numSprites(const unsigned ly) const {
		return num[ly];
	}
	
	unsigned spriteCycles(const unsigned ly) const {
		if (cycles[ly] == CYCLES_INVALID)
			updateLine(ly);
		
		return cycles[ly];
	}
	
	const unsigned short* sprites(const unsigned ly) const {
		if (cycles[ly] == CYCLES_INVALID)
			updateLine(ly);
		
		return spritemap + ly * 10;
	}
};

//void addEvent(SpriteMapper &event, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
