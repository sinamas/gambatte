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
#include "sprite_mapper.h"
#include "sprite_size_reader.h"
#include "scx_reader.h"
#include "../event_queue.h"

// #include <algorithm>
#include <cstring>

SpriteMapper::SpriteMapper(const SpriteSizeReader &spriteSizeReader_in,
             const ScxReader &scxReader_in,
             const unsigned char *const oamram_in) :
	VideoEvent(2),
	spriteSizeReader(spriteSizeReader_in),
	scxReader(scxReader_in),
	oamram(oamram_in)
{
	setDoubleSpeed(false);
	setCgb(false);
	clearMap();
}

void SpriteMapper::clearMap() {
	std::memset(cycles, CYCLES_INVALID, sizeof(cycles));
	std::memset(num, 0, sizeof(num));
}

void SpriteMapper::mapSprites() {
	clearMap();
	
	const unsigned spriteHeight = 8u << spriteSizeReader.largeSprites();
	
	for (unsigned i = 0x00; i < 0xA0; i += 4) {
		const unsigned bottom_pos = oamram[i] - (17u - spriteHeight);
		
		if (bottom_pos >= 143 + spriteHeight)
			continue;
		
		const unsigned spx = oamram[i + 1];
		const unsigned value = spx << 8 | i;
		
		unsigned short *map = spritemap;
		unsigned char *n = num;
		
		if (bottom_pos >= spriteHeight) {
			const unsigned startly = bottom_pos + 1 - spriteHeight;
			n += startly;
			map += startly * 10;
		}
		
		unsigned char *const end = num + (bottom_pos >= 143 ? 143 : bottom_pos);
		
		do {
			if (*n < 10)
				map[(*n)++] = value;
			
			map += 10;
			++n;
		} while (n <= end);
	}
}

// unsafe if start is at the end of allocated memory, since start+1 could be undefined/of.
static void insertionSort(unsigned short *const start, unsigned short *const end) {
	unsigned short *a = start;
	
	while (++a < end) {
		const unsigned e = *a;
		
		unsigned short *b = a;
		
		while (b != start && *(b - 1) > e) {
			*b = *(b - 1);
			b = b - 1;
		}
		
		*b = e;
	}
}

void SpriteMapper::updateLine(const unsigned ly) const {
	if (num[ly] == 0) {
		cycles[ly] = 0;
		return;
	}
	
	unsigned short sortBuf[10];
	unsigned short *tmp = spritemap + ly * 10;
	
	if (cgb) {
		std::memcpy(sortBuf, tmp, sizeof(sortBuf));
		tmp = sortBuf;
	}
	
	insertionSort(tmp, tmp + num[ly]);
// 	std::sort(tmp, tmp + num[ly]);
	
	unsigned sum = 0;
	
	for (unsigned i = 0; i < num[ly]; ++i) {
		const unsigned spx = tmp[i] >> 8;
		
		if (spx > 167)
			break;
		
		unsigned cycles = 6;
		const unsigned posAnd7 = scxReader.scxAnd7() + spx & 7;
		
		if (posAnd7 < 5) {
			cycles = 11 - posAnd7;
			
			for (unsigned j = i; j--;) {
				const unsigned tmpSpx = tmp[j] >> 8;
				
				if (spx - tmpSpx > 4U)
					break;
				
				if ((scxReader.scxAnd7() + tmpSpx & 7) < 4 || spx == tmpSpx) {
					cycles = 6;
					break;
				}
			}
		}
		
		sum += cycles;
	}
	
	cycles[ly] = sum;
}

void SpriteMapper::doEvent() {
	mapSprites();
	
	if (spriteSizeReader.time() < scxReader.time())
		setTime(spriteSizeReader.time() + timeDiff);
	else
		setTime(scxReader.time());
}

void SpriteMapper::reset() {
	mapSprites();
	setTime(DISABLED_TIME);
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
