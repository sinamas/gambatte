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
#ifndef VIDEO_H
#define VIDEO_H

class VideoBlitter;
struct FilterInfo;
class Filter;

// #include <sys/types.h>
#include <stdint.h>
#include <vector>
#include "event_queue.h"
#include "videoblitter.h"

#include "video/video_event_comparer.h"
#include "video/ly_counter.h"
#include "video/sprite_size_reader.h"
#include "video/wx_reader.h"
#include "video/wy.h"
#include "video/we.h"
#include "video/we_master_checker.h"
#include "video/scx_reader.h"
#include "video/sprite_mapper.h"
#include "video/sc_reader.h"
#include "video/break_event.h"
#include "video/mode3_event.h"

#include "video/lyc_irq.h"
#include "video/mode0_irq.h"
#include "video/mode1_irq.h"
#include "video/mode2_irq.h"
#include "video/irq_event.h"
#include "video/m3_extra_cycles.h"

class LCD {
	//static const uint8_t xflipt[0x100];
	static const uint32_t dmgColorsRgb32[4];
	static const uint32_t dmgColorsRgb16[4];
	static const uint32_t dmgColorsUyvy[4];

	uint32_t bgPalette[8 * 4];
	uint32_t spPalette[8 * 4];
	
	const uint8_t *const vram;
	const uint8_t *bgTileData;
	const uint8_t *bgTileMap;
	const uint8_t *wdTileMap;
	
	VideoBlitter *vBlitter;
	Filter *filter;
	
	void *dbuffer;
	void (LCD::*draw)(unsigned xpos, unsigned ypos, unsigned endX);
	unsigned (*gbcToFormat)(unsigned bgr15);
	const uint32_t *dmgColors;
	
	uint32_t dpitch;

	uint32_t lastUpdate;
	uint32_t videoCycles;

	uint32_t winYPos;

	uint32_t enableDisplayM0Time;
	
	event_queue<VideoEvent*,VideoEventComparer> m3EventQueue;
	event_queue<VideoEvent*,VideoEventComparer> irqEventQueue;
	event_queue<VideoEvent*,VideoEventComparer> vEventQueue;
	
	const M3ExtraCycles m3ExtraCycles;
	
	LyCounter lyCounter;
	We we;
	WeMasterChecker weMasterChecker;
	Wy wyReg;
	WxReader wxReader;
	SpriteSizeReader spriteSizeReader;
	ScxReader scxReader;
	SpriteMapper spriteMapper;
	ScReader scReader;
	BreakEvent breakEvent;
	Mode3Event mode3Event;
	
	LycIrq lycIrq;
	Mode0Irq mode0Irq;
	Mode1Irq mode1Irq;
	Mode2Irq mode2Irq;
	IrqEvent irqEvent;
	
	PixelBuffer pb;
	
	std::vector<Filter*> filters;
	
	uint8_t drawStartCycle;
	uint8_t scReadOffset;
	uint8_t tileIndexSign;
	uint8_t ifReg;
	
	bool doubleSpeed;
	bool enabled;
	bool cgb;
	bool bgEnable;
	bool spriteEnable;
	
	static void setDmgPalette(uint32_t *palette, const uint32_t *dmgColors, unsigned data);
	static unsigned gbcToRgb32(unsigned bgr15);
	static unsigned gbcToRgb16(unsigned bgr15);
	static unsigned gbcToUyvy(unsigned bgr15);
	
	void setDBuffer();
	void resetVideoState(unsigned statReg, unsigned cycleCounter);
	void rescheduleEvents(unsigned cycleCounter);
	
	void setDoubleSpeed(bool enabled);

	void event();
	
	bool isMode0IrqPeriod(unsigned cycleCounter);
	bool isMode2IrqPeriod(unsigned cycleCounter);
	bool isLycIrqPeriod(unsigned lycReg, unsigned endCycles, unsigned cycleCounter);
	bool isMode1IrqPeriod(unsigned cycleCounter);

	template<typename T> void bg_drawPixels(T *buffer_line, unsigned xpos, unsigned end, unsigned scx, const uint8_t *tilemap, const uint8_t *tiledata);
	template<typename T> void drawSprites(T *buffer_line, unsigned ypos);

	template<typename T> void cgb_bg_drawPixels(T *buffer_line, unsigned xpos, unsigned end, unsigned scx, const uint8_t *tilemap, const uint8_t *tiledata, unsigned tileline);
	template<typename T> void cgb_drawSprites(T *buffer_line, unsigned ypos);
	
	void null_draw(unsigned xpos, unsigned ypos, unsigned endX);
	template<typename T> void dmg_draw(unsigned xpos, unsigned ypos, unsigned endX);
	template<typename T> void cgb_draw(unsigned xpos, unsigned ypos, unsigned endX);

	void do_update(unsigned cycles);
	void update(unsigned cycleCounter);

public:
	LCD(const uint8_t *oamram, const uint8_t *vram_in);
	~LCD();
	void reset(bool cgb);
	void setVideoBlitter(VideoBlitter *vb);
	void videoBufferChange();
	void setVideoFilter(uint32_t n);
	std::vector<const FilterInfo*> filterInfo() const;
	unsigned int videoWidth() const;
	unsigned int videoHeight() const;
	
	void wdTileMapSelectChange(bool newValue, unsigned cycleCounter);
	void bgTileMapSelectChange(bool newValue, unsigned cycleCounter);
	void bgTileDataSelectChange(bool newValue, unsigned cycleCounter);
	void bgEnableChange(bool newValue, unsigned cycleCounter);
	void spriteEnableChange(bool newValue, unsigned cycleCounter);
	
	void dmgBgPaletteChange(const unsigned data, const unsigned cycleCounter) {
		update(cycleCounter);
		setDmgPalette(bgPalette, dmgColors, data);
	}
	
	void dmgSpPalette1Change(const unsigned data, const unsigned cycleCounter) {
		update(cycleCounter);
		setDmgPalette(spPalette, dmgColors, data);
	}
	
	void dmgSpPalette2Change(const unsigned data, const unsigned cycleCounter) {
		update(cycleCounter);
		setDmgPalette(spPalette + 4, dmgColors, data);
	}
	
	void cgbBgColorChange(const unsigned index, const unsigned bgr15, const unsigned cycleCounter) {
		update(cycleCounter);
		bgPalette[index] = (*gbcToFormat)(bgr15);
	}
	
	void cgbSpColorChange(const unsigned index, const unsigned bgr15, const unsigned cycleCounter) {
		update(cycleCounter);
		spPalette[index] = (*gbcToFormat)(bgr15);
	}
	
	void updateScreen(unsigned cc);
	void enableChange(unsigned statReg, unsigned cycleCounter);
	void preResetCounter(unsigned cycleCounter);
	void postResetCounter(unsigned oldCC, unsigned cycleCounter);
	void preSpeedChange(unsigned cycleCounter);
	void postSpeedChange(unsigned cycleCounter);
// 	unsigned get_mode(unsigned cycleCounter) /*const*/;
	bool vramAccessible(unsigned cycleCounter);
	bool cgbpAccessible(unsigned cycleCounter);
	bool oamAccessible(unsigned cycleCounter);
	void weChange(bool newValue, unsigned cycleCounter);
	void wxChange(unsigned newValue, unsigned cycleCounter);
	void wyChange(unsigned newValue, unsigned cycleCounter);
	void oamChange(unsigned cycleCounter);
	void scxChange(unsigned newScx, unsigned cycleCounter);
	void scyChange(unsigned newValue, unsigned cycleCounter);
	void spriteSizeChange(bool newLarge, unsigned cycleCounter);
	
	void vramChange(const unsigned cycleCounter) {
		update(cycleCounter);
	}
	
	unsigned get_stat(unsigned lycReg, unsigned cycleCounter);

	unsigned getLyReg(const unsigned cycleCounter) {
		if (cycleCounter >= lyCounter.time())
			update(cycleCounter);
			
		unsigned lyReg = lyCounter.ly();
		
		if ((lyCounter.time() - cycleCounter) <= 4) {
			if (lyReg == 153)
				lyReg = 0;
			else
				++lyReg;
		} else if (lyReg == 153)
			lyReg = 0;

		return lyReg;
	}
	
	unsigned nextMode1IrqTime() const {
		return mode1Irq.time();
	}
	
	void lcdstatChange(unsigned old, unsigned data, unsigned cycleCounter);
	void lycRegChange(unsigned data, unsigned statReg, unsigned cycleCounter);
	unsigned nextIrqEvent() const;
	unsigned getIfReg(unsigned cycleCounter);
	void setIfReg(unsigned ifReg_in, unsigned cycleCounter);

	unsigned nextHdmaTime(unsigned cycleCounter);
	bool isHdmaPeriod(unsigned cycleCounter);
	
	unsigned nextHdmaTimeInvalid() const {
		return mode3Event.time();
	}
};

#endif
