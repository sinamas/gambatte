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
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#include "video.h"
#include "sound.h"

#include "interrupter.h"
#include "rtc.h"

struct InputState;
class InputStateGetter;

class Memory {
	enum cartridgetype { plain, mbc1, mbc2, mbc3, mbc5 };
	enum events { HDMA_RESCHEDULE, DMA, INTERRUPTS, BLIT, UNHALT, END };
	enum irqEvents { /*MODE0, MODE1, MODE2, LYC,*/ TIMA, /*M0RESC,*/ SERIAL };
	
	uint8_t memory[0x10000];
	uint8_t vram[0x2000 * 2];
	uint8_t disabled_ram[0x1000];
	uint8_t *mem[0x10];
	uint8_t cgb_bgp_data[8 * 8];
	uint8_t cgb_objp_data[8 * 8];
	
	uint8_t *rombankptr;
	uint8_t *rambankdata;
	uint8_t *cgb_wramdata;

	char* romfile;
	uint8_t *romdata;
	char *savedir;
	char *savename;
	
	InputStateGetter *getInput;

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

	void swap_rombank();
	void oamDma(unsigned long cycleCounter);
	void mbc_write(uint16_t P, uint8_t data);

	void set_event();
	void set_irqEvent();
	void update_irqEvents(unsigned long cc);
	void update_tima(unsigned long cycleCounter);
	
	void rescheduleIrq(unsigned long cycleCounter);
	void rescheduleHdmaReschedule();
	
	void refreshPalettes(unsigned long cycleCounter);

public:
	Memory(const Interrupter &interrupter);
	~Memory();

	void reset();
	void reload();

	void speedChange(unsigned long cycleCounter);
	bool isDoubleSpeed() const { return memory[0xFF4D] >> 7; }
	bool isCgb() const { return memory[0x0143] >> 7; }
	bool getIME() const { return IME; }
	unsigned long getNextEventTime() const { return next_eventtime; }
	
	bool isActive() const { return active; }

	void ei(unsigned long cycleCounter);

	void di() {
		IME = 0;
		next_irqtime = uint32_t(-1);
		if (next_event == INTERRUPTS)
			set_event();
//     next_eitime=0;
//     if(next_event==EI) set_event();
	}


	uint8_t ff_read(unsigned P, unsigned long cycleCounter);

	uint8_t read(unsigned P, unsigned long cycleCounter);
	
	uint8_t pc_read(const unsigned P, const unsigned long cycleCounter) {
		if (P < 0x4000)
			return mem[0][P];
		
		if (P < 0x8000)
			return rombankptr[P];
		
		return read(P, cycleCounter);
	}

	void write(uint16_t P, uint8_t data, unsigned long cycleCounter);
	void ff_write(uint16_t P, uint8_t data, unsigned long cycleCounter);

	unsigned long event(unsigned long cycleCounter);
	unsigned long resetCounters(unsigned long cycleCounter);

	bool loadROM();
	bool loadROM(const char* romfile);
	void set_savedir(const char *dir);

	void setInputStateGetter(InputStateGetter *getInput) {
		this->getInput = getInput;
	}

	void schedule_unhalt();
	void inc_endtime(unsigned long inc);
	
	void sound_fill_buffer(uint16_t *stream, unsigned samples, unsigned long cycleCounter);
	void setVideoBlitter(VideoBlitter * vb, unsigned long cycleCounter);
	void setVideoFilter(unsigned int n, unsigned long cycleCounter);
	
	void videoBufferChange(unsigned long cycleCounter);
	
	unsigned videoWidth() const {
		return display.videoWidth();
	}
	
	unsigned videoHeight() const {
		return display.videoHeight();
	}
	
	std::vector<const FilterInfo*> filterInfo() const {
		return display.filterInfo();
	}
	
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned long rgb32, unsigned long cycleCounter);
};

#endif
