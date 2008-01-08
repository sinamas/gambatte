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

namespace Gambatte {
class VideoBlitter;
struct FilterInfo;
}

class Filter;

#include <vector>
#include "event_queue.h"
#include "videoblitter.h"

#include "video/video_event_comparer.h"
#include "video/ly_counter.h"
#include "video/window.h"
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
	unsigned long dmgColorsRgb32[3 * 4];
	unsigned long dmgColorsRgb16[3 * 4];
	unsigned long dmgColorsUyvy[3 * 4];

	unsigned long bgPalette[8 * 4];
	unsigned long spPalette[8 * 4];
	
	const unsigned char *const vram;
	const unsigned char *bgTileData;
	const unsigned char *bgTileMap;
	const unsigned char *wdTileMap;
	
	Gambatte::VideoBlitter *vBlitter;
	Filter *filter;
	
	void *dbuffer;
	void (LCD::*draw)(unsigned xpos, unsigned ypos, unsigned endX);
	unsigned long (*gbcToFormat)(unsigned bgr15);
	const unsigned long *dmgColors;
	
	unsigned long lastUpdate;
	unsigned long videoCycles;
	unsigned long enableDisplayM0Time;
	
	unsigned dpitch;
	unsigned winYPos;
	
	event_queue<VideoEvent*,VideoEventComparer> m3EventQueue;
	event_queue<VideoEvent*,VideoEventComparer> irqEventQueue;
	event_queue<VideoEvent*,VideoEventComparer> vEventQueue;
	
	LyCounter lyCounter;
	Window win;
	ScxReader scxReader;
	SpriteMapper spriteMapper;
	M3ExtraCycles m3ExtraCycles;
	ScReader scReader;
	BreakEvent breakEvent;
	Mode3Event mode3Event;
	
	LycIrq lycIrq;
	Mode0Irq mode0Irq;
	Mode1Irq mode1Irq;
	Mode2Irq mode2Irq;
	IrqEvent irqEvent;
	
	Gambatte::PixelBuffer pb;
	
	std::vector<Filter*> filters;
	
	unsigned char drawStartCycle;
	unsigned char scReadOffset;
	unsigned char ifReg;
	unsigned char tileIndexSign;
	
	bool doubleSpeed;
	bool enabled;
	bool cgb;
	bool bgEnable;
	bool spriteEnable;
	
	static void setDmgPalette(unsigned long *palette, const unsigned long *dmgColors, unsigned data);
	void setDmgPaletteColor(unsigned index, unsigned long rgb32);
	static unsigned long gbcToRgb32(unsigned bgr15);
	static unsigned long gbcToRgb16(unsigned bgr15);
	static unsigned long gbcToUyvy(unsigned bgr15);
	
	void setDBuffer();
	void resetVideoState(unsigned statReg, unsigned long cycleCounter);
	void rescheduleEvents(unsigned long cycleCounter);
	
	void setDoubleSpeed(bool enabled);

	void event();
	
	bool isMode0IrqPeriod(unsigned long cycleCounter);
	bool isMode2IrqPeriod(unsigned long cycleCounter);
	bool isLycIrqPeriod(unsigned lycReg, unsigned endCycles, unsigned long cycleCounter);
	bool isMode1IrqPeriod(unsigned long cycleCounter);

	template<typename T> void bg_drawPixels(T *buffer_line, unsigned xpos, unsigned end, unsigned scx, const unsigned char *tilemap, const unsigned char *tiledata);
	template<typename T> void drawSprites(T *buffer_line, unsigned ypos);

	template<typename T> void cgb_bg_drawPixels(T *buffer_line, unsigned xpos, unsigned end, unsigned scx, const unsigned char *tilemap, const unsigned char *tiledata, unsigned tileline);
	template<typename T> void cgb_drawSprites(T *buffer_line, unsigned ypos);
	
	void null_draw(unsigned xpos, unsigned ypos, unsigned endX);
	template<typename T> void dmg_draw(unsigned xpos, unsigned ypos, unsigned endX);
	template<typename T> void cgb_draw(unsigned xpos, unsigned ypos, unsigned endX);

	void do_update(unsigned cycles);
	void update(unsigned long cycleCounter);

public:
	LCD(const unsigned char *oamram, const unsigned char *vram_in);
	~LCD();
	void reset(bool cgb);
	void setVideoBlitter(Gambatte::VideoBlitter *vb);
	void videoBufferChange();
	void setVideoFilter(unsigned n);
	std::vector<const Gambatte::FilterInfo*> filterInfo() const;
	unsigned videoWidth() const;
	unsigned videoHeight() const;
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32);
	
	void wdTileMapSelectChange(bool newValue, unsigned long cycleCounter);
	void bgTileMapSelectChange(bool newValue, unsigned long cycleCounter);
	void bgTileDataSelectChange(bool newValue, unsigned long cycleCounter);
	void bgEnableChange(bool newValue, unsigned long cycleCounter);
	void spriteEnableChange(bool newValue, unsigned long cycleCounter);
	
	void dmgBgPaletteChange(const unsigned data, const unsigned long cycleCounter) {
		update(cycleCounter);
		setDmgPalette(bgPalette, dmgColors, data);
	}
	
	void dmgSpPalette1Change(const unsigned data, const unsigned long cycleCounter) {
		update(cycleCounter);
		setDmgPalette(spPalette, dmgColors + 4, data);
	}
	
	void dmgSpPalette2Change(const unsigned data, const unsigned long cycleCounter) {
		update(cycleCounter);
		setDmgPalette(spPalette + 4, dmgColors + 8, data);
	}
	
	void cgbBgColorChange(const unsigned index, const unsigned bgr15, const unsigned long cycleCounter) {
		update(cycleCounter);
		bgPalette[index] = (*gbcToFormat)(bgr15);
	}
	
	void cgbSpColorChange(const unsigned index, const unsigned bgr15, const unsigned long cycleCounter) {
		update(cycleCounter);
		spPalette[index] = (*gbcToFormat)(bgr15);
	}
	
	void updateScreen(unsigned long cc);
	void enableChange(unsigned statReg, unsigned long cycleCounter);
	void preResetCounter(unsigned long cycleCounter);
	void postResetCounter(unsigned long oldCC, unsigned long cycleCounter);
	void preSpeedChange(unsigned long cycleCounter);
	void postSpeedChange(unsigned long cycleCounter);
// 	unsigned get_mode(unsigned cycleCounter) /*const*/;
	bool vramAccessible(unsigned long cycleCounter);
	bool cgbpAccessible(unsigned long cycleCounter);
	bool oamAccessible(unsigned long cycleCounter);
	void weChange(bool newValue, unsigned long cycleCounter);
	void wxChange(unsigned newValue, unsigned long cycleCounter);
	void wyChange(unsigned newValue, unsigned long cycleCounter);
	void oamChange(unsigned long cycleCounter);
	void scxChange(unsigned newScx, unsigned long cycleCounter);
	void scyChange(unsigned newValue, unsigned long cycleCounter);
	void spriteSizeChange(bool newLarge, unsigned long cycleCounter);
	
	void vramChange(const unsigned long cycleCounter) {
		update(cycleCounter);
	}
	
	unsigned get_stat(unsigned lycReg, unsigned long cycleCounter);

	unsigned getLyReg(const unsigned long cycleCounter) {
		if (cycleCounter >= lyCounter.time())
			update(cycleCounter);
			
		unsigned lyReg = lyCounter.ly();
		
		if (lyCounter.time() - cycleCounter <= 4) {
			if (lyReg == 153)
				lyReg = 0;
			else
				++lyReg;
		} else if (lyReg == 153)
			lyReg = 0;

		return lyReg;
	}
	
	unsigned long nextMode1IrqTime() const {
		return mode1Irq.time();
	}
	
	void lcdstatChange(unsigned old, unsigned data, unsigned long cycleCounter);
	void lycRegChange(unsigned data, unsigned statReg, unsigned long cycleCounter);
	unsigned long nextIrqEvent() const;
	unsigned getIfReg(unsigned long cycleCounter);
	void setIfReg(unsigned ifReg_in, unsigned long cycleCounter);

	unsigned long nextHdmaTime(unsigned long cycleCounter);
	bool isHdmaPeriod(unsigned long cycleCounter);
	
	unsigned long nextHdmaTimeInvalid() const {
		return mode3Event.time();
	}
};

#endif
