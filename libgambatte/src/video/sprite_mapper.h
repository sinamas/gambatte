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
#include "basic_add_event.h"

class M3ExtraCycles;
class SaveState;

class SpriteMapper : public VideoEvent {
	class OamReader {
		unsigned char buf[80];
		bool szbuf[40];
	
	public:
		const LyCounter &lyCounter;
	
	private:
		const unsigned char *oamram;
		unsigned long lu;
		unsigned char lastChange;
		bool largeSpritesSrc;
		
	public:
		OamReader(const LyCounter &lyCounter, const unsigned char *oamram);
		void change(unsigned long cc);
		void change(const unsigned char *oamram, unsigned long cc) { change(cc); this->oamram = oamram; }
		bool changed() const { return lastChange != 0xFF; }
		bool largeSprites(unsigned spNr) const { return szbuf[spNr]; }
		const unsigned char *oam() const { return oamram; }
		void resetCycleCounter(const unsigned newCc) { lu = newCc; }
		void setLargeSpritesSrc(const bool src) { largeSpritesSrc = src; }
		void update(unsigned long cc);
		const unsigned char *spritePosBuf() const { return buf; }
		void setStatePtrs(SaveState &state);
	};
	
	enum { NEED_SORTING_MASK = 0x80 };

public:
	class SpxLess {
		const unsigned char *const posbuf_plus1;
		
	public:
		SpxLess(const unsigned char *const posbuf) : posbuf_plus1(posbuf + 1) {} 
		
		bool operator()(const unsigned char l, const unsigned char r) const {
			return posbuf_plus1[l] < posbuf_plus1[r];
		}
	};

private:
	mutable unsigned char spritemap[144*10];
	mutable unsigned char num[144];
	
	M3ExtraCycles &m3ExtraCycles;
	OamReader oamReader;
	
	bool cgb;
	
	void clearMap();
	void mapSprites();
	void sortLine(unsigned ly) const;
	
public:
	SpriteMapper(M3ExtraCycles &m3ExtraCycles,
	             const LyCounter &lyCounter,
	             const unsigned char *oamram_in);
	
	void doEvent();
	
	bool isCgb() const {
		return cgb;
	}
	
	bool largeSprites(unsigned spNr) const {
		return oamReader.largeSprites(spNr);
	}
	
	unsigned numSprites(const unsigned ly) const {
		return num[ly] & ~NEED_SORTING_MASK;
	}
	
	void oamChange(const unsigned long cc) {
		oamReader.change(cc);
	}
	
	void oamChange(const unsigned char *oamram, const unsigned long cc) {
		oamReader.change(oamram, cc);
	}
	
	const unsigned char *oamram() const {
		return oamReader.oam();
	}
	
	const unsigned char *posbuf() const {
		return oamReader.spritePosBuf();
	}
	
	void preCounterChange(const unsigned long cc) {
		oamReader.update(cc);
	}
	
	void resetCycleCounter(const unsigned long newCc) {
		oamReader.resetCycleCounter(newCc);
	}
	
	static unsigned long schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
		return lyCounter.nextLineCycle(80, cycleCounter);
	}
	
	void setCgb(const bool cgb_in) {
		cgb = cgb_in;
	}
	
	void setLargeSpritesSource(const bool src) {
		oamReader.setLargeSpritesSrc(src);
	}
	
	const unsigned char* sprites(const unsigned ly) const {
		if (num[ly] & NEED_SORTING_MASK)
			sortLine(ly);
		
		return spritemap + ly * 10;
	}
	
	void setStatePtrs(SaveState &state) { oamReader.setStatePtrs(state); }
};

static inline void addEvent(event_queue<VideoEvent*,VideoEventComparer> &q, SpriteMapper *const e, const unsigned long newTime) {
	addUnconditionalEvent(q, e, newTime);
}

static inline void addFixedtimeEvent(event_queue<VideoEvent*,VideoEventComparer> &q, SpriteMapper *const e, const unsigned long newTime) {
	addUnconditionalFixedtimeEvent(q, e, newTime);
}

#endif
