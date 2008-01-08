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

#include "video_event.h"
//#include "video_event_comparer.h"
#include "ly_counter.h"

class M3ExtraCycles;

class SpriteMapper : public VideoEvent {
	enum { NEED_SORTING_MASK = 0x80 };
	
	mutable unsigned char spritemap[144*10];
	mutable unsigned char num[144];
	
	M3ExtraCycles &m3ExtraCycles;
	
public:
	const unsigned char *const oamram;

private:
	bool largeSprites_;
	bool largeSpritesSrc_;
	bool cgb;
	
	void clearMap();
	void mapSprites();
	void sortLine(unsigned ly) const;
	
public:
	SpriteMapper(M3ExtraCycles &m3ExtraCycles,
	             const unsigned char *const oamram_in);
	
	void doEvent();
	
	bool isCgb() const {
		return cgb;
	}
	
	bool largeSprites() const {
		return largeSprites_;
	}
	
	unsigned numSprites(const unsigned ly) const {
		return num[ly] & ~NEED_SORTING_MASK;
	}
	
	void reset();
	
	void schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
		setTime(lyCounter.nextLineCycle(16 + lyCounter.isDoubleSpeed() * 4, cycleCounter));
	}
	
	void setCgb(const bool cgb_in) {
		cgb = cgb_in;
	}
	
	void setLargeSpritesSource(const bool src) {
		largeSpritesSrc_ = src;
	}
	
	const unsigned char* sprites(const unsigned ly) const {
		if (num[ly] & NEED_SORTING_MASK)
			sortLine(ly);
		
		return spritemap + ly * 10;
	}
};

//void addEvent(SpriteMapper &event, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
