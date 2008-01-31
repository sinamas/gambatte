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

#include <algorithm>

SpriteMapper::OamReader::OamReader(const LyCounter &lyCounter, const unsigned char *oamram)
: lyCounter(lyCounter), oamram(oamram) {
	setLargeSpritesSrc(false);
	reset();
}

void SpriteMapper::OamReader::reset() {
	lu = 0xFFFFFFFF;
	lastChange = 0xFF;
	std::fill_n(szbuf, 40, largeSpritesSrc);
	
	unsigned pos = 0;
	unsigned distance = 80;
	
	while (distance--) {
		buf[pos] = oamram[pos * 2 & ~3 | pos & 1];
		++pos;
	}
}

static unsigned toPosCycles(const unsigned long cc, const LyCounter &lyCounter) {
	unsigned lc = lyCounter.lineCycles(cc) + 4 - lyCounter.isDoubleSpeed() * 3;
	
	if (lc >= 456)
		lc -= 456;
	
	return lc >> 1;
}

void SpriteMapper::OamReader::update(const unsigned long cc) {
	if (changed()) {
		const unsigned lulc = toPosCycles(lu, lyCounter);
		
		unsigned pos = std::min(lulc, 40u);
		unsigned distance = 40;
		
		if (cc - lu >> lyCounter.isDoubleSpeed() < 456) {
			const unsigned cclc = toPosCycles(cc, lyCounter);
			
			distance = std::min(cclc, 40u) - pos + (cclc < lulc ? 40 : 0);
		}
		
		{
			const unsigned targetDistance = lastChange - pos + (lastChange <= pos ? 40 : 0);
			
			if (targetDistance <= distance) {
				distance = targetDistance;
				lastChange = 0xFF;
			}
		}
		
		while (distance--) {
			if (pos >= 40)
				pos = 0;
			
			szbuf[pos] = largeSpritesSrc;
			buf[pos * 2] = oamram[pos * 4];
			buf[pos * 2 + 1] = oamram[pos * 4 + 1];
			
			++pos;
		}
	}
	
	lu = cc;
}

void SpriteMapper::OamReader::change(const unsigned long cc) {
	update(cc);
	lastChange = std::min(toPosCycles(cc, lyCounter), 40u);
}

SpriteMapper::SpriteMapper(M3ExtraCycles &m3ExtraCycles,
                           const LyCounter &lyCounter,
                           const unsigned char *const oamram) :
	VideoEvent(2),
	m3ExtraCycles(m3ExtraCycles),
	oamReader(lyCounter, oamram)
{
	setCgb(false);
	clearMap();
}

void SpriteMapper::clearMap() {
	std::memset(num, cgb ? 0 : NEED_SORTING_MASK, sizeof(num));
}

void SpriteMapper::mapSprites() {
	clearMap();
	
	for (unsigned i = 0x00; i < 0x50; i += 2) {
		const unsigned spriteHeight = 8u << largeSprites(i >> 1);
		const unsigned bottom_pos = posbuf()[i] - (17u - spriteHeight);
		
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
	insertionSort(spritemap + ly * 10, spritemap + ly * 10 + num[ly], SpxLess(posbuf()));
}

void SpriteMapper::doEvent() {
	oamReader.update(time());
	mapSprites();
	setTime(oamReader.changed() ? time() + oamReader.lyCounter.lineTime() : DISABLED_TIME);
}

void SpriteMapper::reset() {
	oamReader.reset();
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
