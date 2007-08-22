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
#include "sprite_size_reader.h"
#include "scx_reader.h"
#include "../event_queue.h"

#include <algorithm>

SpriteMapper::SpriteMapper(const SpriteSizeReader &spriteSizeReader_in,
             const ScxReader &scxReader_in,
             const uint8_t *const oamram_in) :
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
	for (uint16_t* i = &spritemap[10]; i < &spritemap[144 * 12]; i += 12)
		i[1] = i[0] = 0;
}

void SpriteMapper::mapSprites() {
	clearMap();
	
	const unsigned spriteHeight = 8 << spriteSizeReader.largeSprites();
	
	for (unsigned i = 0x00; i < 0xA0; i += 4) {
		const unsigned bottom_pos = oamram[i] - (17 - spriteHeight);
		if (bottom_pos >= 143 + spriteHeight)
			continue;
		
		const unsigned spx = oamram[i + 1];
		const unsigned value = (spx << 8) | i;
		
		uint16_t *tmp = &spritemap[bottom_pos < spriteHeight ? 0 : ((bottom_pos + 1 - spriteHeight) * 12)];
		uint16_t *const end = &spritemap[bottom_pos >= 143 ? 143 * 12 : (bottom_pos * 12)];
		
		do {
			if (tmp[10] < 10)
				tmp[tmp[10]++] = value;
			tmp += 12;
		} while (tmp <= end);
	}
	
	uint16_t sortBuf[11];
	
	for (uint16_t *mapPos = spritemap; mapPos < spritemap + 144 * 12; mapPos += 12) {
		if (mapPos[10] == 0)
			continue;
		
		uint16_t *tmp = mapPos;
		
		if (cgb) {
			std::memcpy(sortBuf, tmp, sizeof(sortBuf));
			tmp = sortBuf;
		}
		
		std::sort(tmp, tmp + tmp[10]);
		
		unsigned sum = 0;
		
		for (unsigned i = 0; i < tmp[10]; ++i) {
			const unsigned spx = tmp[i] >> 8;
			const unsigned cycles = spx < 168 ? std::max(11U - ((scxReader.scxAnd7() + spx) & 7U), 6U) : 0;
			
			if (cycles > 6) {
				unsigned j = 0;
				for (; j < i; ++j) {
					const unsigned tmpSpx = tmp[j] >> 8;
					if (spx - tmpSpx < 5U && (((scxReader.scxAnd7() + tmpSpx) & 7) < 4 || spx == tmpSpx))
						break;
				}
				
				sum += j < i ? 6 : cycles;
			} else
				sum += cycles;
		}
		
		mapPos[11] = sum;
	}
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
	setTime(uint32_t(-1));
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
	
	if (oldTime == uint32_t(-1))
		queue.push(&event);
	else if (oldTime > event.time()) {
		queue.dec(&event, &event);
	}
}*/
