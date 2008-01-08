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
#include "sprite_mapper.h"
#include "m3_extra_cycles.h"
#include "../insertion_sort.h"
#include <cstring>

// #include <algorithm>

class SpxLess {
	const unsigned char *const oamram_plus1;
	
public:
	SpxLess(const unsigned char *const oamram) : oamram_plus1(oamram + 1) {} 
	
	bool operator()(const unsigned char l, const unsigned char r) const {
		return oamram_plus1[l] < oamram_plus1[r];
	}
};

SpriteMapper::SpriteMapper(M3ExtraCycles &m3ExtraCycles,
                           const unsigned char *const oamram) :
	VideoEvent(2),
	m3ExtraCycles(m3ExtraCycles),
	oamram(oamram)
{
	setCgb(false);
	setLargeSpritesSource(false);
	largeSprites_ = largeSpritesSrc_;
	clearMap();
}

void SpriteMapper::clearMap() {
	std::memset(num, cgb ? 0 : NEED_SORTING_MASK, sizeof(num));
}

void SpriteMapper::mapSprites() {
	clearMap();
	
	const unsigned spriteHeight = 8u << largeSprites();
	
	for (unsigned i = 0x00; i < 0xA0; i += 4) {
		const unsigned bottom_pos = oamram[i] - (17u - spriteHeight);
		
		if (bottom_pos >= 143 + spriteHeight)
			continue;
		
		unsigned char *map = spritemap;
		unsigned char *n = num;
		
		if (bottom_pos >= spriteHeight) {
			const unsigned startly = bottom_pos + 1 - spriteHeight;
			n += startly;
			map += startly * 10;
		}
		
		unsigned char *const end = num + (bottom_pos >= 143 ? 143 : bottom_pos);
		
		do {
			if ((*n & ~NEED_SORTING_MASK) < 10)
				map[(*n)++ & ~NEED_SORTING_MASK] = i;
			
			map += 10;
			++n;
		} while (n <= end);
	}
	
	m3ExtraCycles.invalidateCache();
}

void SpriteMapper::sortLine(const unsigned ly) const {
	num[ly] &= ~NEED_SORTING_MASK;
	insertionSort(spritemap + ly * 10, spritemap + ly * 10 + num[ly], SpxLess(oamram));
}

void SpriteMapper::doEvent() {
	largeSprites_ = largeSpritesSrc_;
	mapSprites();
	setTime(DISABLED_TIME);
}

void SpriteMapper::reset() {
	doEvent();
}

/*void SpriteMapper::schedule() {
	if (spriteSizeReader.time() < scxReader.time())
		setTime(spriteSizeReader.time() + timeDiff);
	else
		setTime(scxReader.time());
}

void addEvent(SpriteMapper &event, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	const unsigned oldTime = event.time();
	event.schedule();
	
	if (oldTime == DISABLED_TIME)
		queue.push(&event);
	else if (oldTime > event.time()) {
		queue.dec(&event, &event);
	}
}*/
