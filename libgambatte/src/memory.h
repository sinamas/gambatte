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
#ifndef MEMORY_H
#define MEMORY_H

#include "int.h"
#include "video.h"
#include "sound.h"

#include "interrupter.h"
#include "rtc.h"

namespace Gambatte {
class InputStateGetter;
class FilterInfo;
}

class Memory {
	enum cartridgetype { plain, mbc1, mbc2, mbc3, mbc5 };
	enum events { HDMA_RESCHEDULE, DMA, INTERRUPTS, BLIT, UNHALT, END };
	enum irqEvents { /*MODE0, MODE1, MODE2, LYC,*/ TIMA, /*M0RESC,*/ SERIAL };
	
	static const unsigned long COUNTER_DISABLED = 0xFFFFFFFF;
	
	unsigned char ioamhram[0x200];
	unsigned char vram[0x2000 * 2];
	unsigned char *rmem[0x10];
	unsigned char *wmem[0x10];
	unsigned char cgb_bgp_data[8 * 8];
	unsigned char cgb_objp_data[8 * 8];
	
	unsigned char *memchunk;
	unsigned char *romdata[2];
	unsigned char *wramdata[2];
	unsigned char *rambankdata;
	unsigned char *rdisabled_ram;
	unsigned char *wdisabled_ram;
	unsigned char *oamDmaSrc;
	unsigned char *vrambank;
	unsigned char *rsrambankptr;
	unsigned char *wsrambankptr;

	char* romfile;
	char *savedir;
	char *savename;
	
	Gambatte::InputStateGetter *getInput;

	unsigned long div_lastUpdate;
	unsigned long tima_lastUpdate;
	unsigned long next_timatime;
	unsigned long next_blittime;
	unsigned long next_irqtime;
	unsigned long next_dmatime;
	unsigned long next_hdmaReschedule;
	unsigned long next_unhalttime;
	unsigned long next_endtime;
	unsigned long next_irqEventTime;
	unsigned long tmatime;
	unsigned long next_serialtime;
	unsigned long next_eventtime;
	unsigned long lastOamDmaUpdate;
	
	LCD display;
	PSG sound;
	Interrupter interrupter;
	Rtc rtc;

	events next_event;
	irqEvents next_irqEvent;
	cartridgetype romtype;
	
	unsigned short rombanks;
	unsigned short rombank;
	unsigned short dmaSource;
	unsigned short dmaDestination;
	
	unsigned char rambank;
	unsigned char rambanks;
	unsigned char oamDmaArea1Lower;
	unsigned char oamDmaArea1Width;
	unsigned char oamDmaArea2Upper;
	unsigned char oamDmaPos;

	bool cgb;
	bool doubleSpeed;
	bool IME;
	bool enable_ram;
	bool rambank_mode;
	bool battery, rtcRom;
	bool hdma_transfer;
	bool active;

	void init();
	void saveram();
	void save_rtc();
	void updateInput();

	void setRombank();
	void setRambank();
	void setBanks();
	void updateOamDma(unsigned long cycleCounter);
	void startOamDma(unsigned long cycleCounter);
	void endOamDma(unsigned long cycleCounter);
	void setOamDmaSrc();
	
	unsigned char nontrivial_ff_read(unsigned P, unsigned long cycleCounter);
	unsigned char nontrivial_read(unsigned P, unsigned long cycleCounter);
	void nontrivial_ff_write(unsigned P, unsigned data, unsigned long cycleCounter);
	void mbc_write(unsigned P, unsigned data);
	void nontrivial_write(unsigned P, unsigned data, unsigned long cycleCounter);

	void set_event();
	void set_irqEvent();
	void update_irqEvents(unsigned long cc);
	void update_tima(unsigned long cycleCounter);
	
	void rescheduleIrq(unsigned long cycleCounter);
	void rescheduleHdmaReschedule();
	
	void refreshPalettes(unsigned long cycleCounter);
	
	bool isDoubleSpeed() const { return doubleSpeed; }

public:
	Memory(const Interrupter &interrupter);
	~Memory();

	void reset();
	void reload();

	void speedChange(unsigned long cycleCounter);
	bool isCgb() const { return cgb; }
	bool getIME() const { return IME; }
	unsigned long getNextEventTime() const { return next_eventtime; }
	
	bool isActive() const { return active; }

	void ei(unsigned long cycleCounter);

	void di() {
		IME = 0;
		next_irqtime = COUNTER_DISABLED;
		
		if (next_event == INTERRUPTS)
			set_event();
		
// 		next_eitime=0;
// 		if(next_event==EI) set_event();
	}

	unsigned char ff_read(const unsigned P, const unsigned long cycleCounter) {
		return P < 0xFF80 ? nontrivial_ff_read(P, cycleCounter) : ioamhram[P - 0xFE00];
	}

	unsigned char read(const unsigned P, const unsigned long cycleCounter) {
		return rmem[P >> 12] ? rmem[P >> 12][P] : nontrivial_read(P, cycleCounter);
	}
	
	unsigned char pc_read(const unsigned P, const unsigned long cycleCounter) {
		return read(P, cycleCounter);
	}

	void write(const unsigned P, const unsigned data, const unsigned long cycleCounter) {
		if (wmem[P >> 12])
			wmem[P >> 12][P] = data;
		else
			nontrivial_write(P, data, cycleCounter);
	}
	
	void ff_write(const unsigned P, const unsigned data, const unsigned long cycleCounter) {
		if ((P + 1 & 0xFF) < 0x81)
			nontrivial_ff_write(P, data, cycleCounter);
		else
			ioamhram[P - 0xFE00] = data;
	}

	unsigned long event(unsigned long cycleCounter);
	unsigned long resetCounters(unsigned long cycleCounter);

	bool loadROM();
	bool loadROM(const char* romfile);
	void set_savedir(const char *dir);

	void setInputStateGetter(Gambatte::InputStateGetter *getInput) {
		this->getInput = getInput;
	}

	void schedule_unhalt();
	void inc_endtime(unsigned long inc);
	
	void sound_fill_buffer(Gambatte::uint_least16_t *stream, unsigned samples, unsigned long cycleCounter);
	void setVideoBlitter(Gambatte::VideoBlitter * vb, unsigned long cycleCounter);
	void setVideoFilter(unsigned int n, unsigned long cycleCounter);
	
	void videoBufferChange(unsigned long cycleCounter);
	
	unsigned videoWidth() const {
		return display.videoWidth();
	}
	
	unsigned videoHeight() const {
		return display.videoHeight();
	}
	
	std::vector<const Gambatte::FilterInfo*> filterInfo() const {
		return display.filterInfo();
	}
	
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32, unsigned long cycleCounter);
};

#endif
