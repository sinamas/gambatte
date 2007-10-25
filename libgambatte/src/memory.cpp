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
#include "memory.h"
#include "video.h"
#include "sound.h"
#include "inputstate.h"
#include "inputstategetter.h"
#include "file/file.h"
#include <cstring>

// static const uint32_t timaClock[4]={ 1024, 16, 64, 256 };
static const uint8_t timaClock[4] = { 10, 4, 6, 8 };

/*
static const uint8_t soundRegInitValues[0x17] = { 0x80, 0x3F, 0x00, 0xFF, 0xBF,
						  0xFF, 0x3F, 0x00, 0xFF, 0xBF,
						  0x7F, 0xFF, 0x9F, 0xFF, 0xBF,
						  0xFF, 0xFF, 0x00, 0x00, 0xBF,
						  0x00, 0x00, 0x70 };
*/

static const uint8_t feaxDump[0x60] = {
	0x18, 0x01, 0xEF, 0xDE, 0x06, 0x4A, 0xCD, 0xBD, 
	0x18, 0x01, 0xEF, 0xDE, 0x06, 0x4A, 0xCD, 0xBD, 
	0x18, 0x01, 0xEF, 0xDE, 0x06, 0x4A, 0xCD, 0xBD, 
	0x18, 0x01, 0xEF, 0xDE, 0x06, 0x4A, 0xCD, 0xBD, 
	0x00, 0x90, 0xF7, 0x7F, 0xC0, 0xB1, 0xB4, 0xFB, 
	0x00, 0x90, 0xF7, 0x7F, 0xC0, 0xB1, 0xB4, 0xFB, 
	0x00, 0x90, 0xF7, 0x7F, 0xC0, 0xB1, 0xB4, 0xFB, 
	0x00, 0x90, 0xF7, 0x7F, 0xC0, 0xB1, 0xB4, 0xFB, 
	0x24, 0x1B, 0xFD, 0x3A, 0x10, 0x12, 0xAD, 0x45, 
	0x24, 0x1B, 0xFD, 0x3A, 0x10, 0x12, 0xAD, 0x45, 
	0x24, 0x1B, 0xFD, 0x3A, 0x10, 0x12, 0xAD, 0x45, 
	0x24, 0x1B, 0xFD, 0x3A, 0x10, 0x12, 0xAD, 0x45
};

static const uint8_t ffxxDump[0x100] = {
	0xCF, 0x00, 0x7C, 0xFF, 0x43, 0x00, 0x00, 0xF8, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 
	0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 
	0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF, 
	0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 
	0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xC0, 0xFF, 0xC1, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 
	0xF8, 0xFF, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 
	0x45, 0xEC, 0x52, 0xFA, 0x08, 0xB7, 0x07, 0x5D, 
	0x01, 0xFD, 0xC0, 0xFF, 0x08, 0xFC, 0x00, 0xE5, 
	0x0B, 0xF8, 0xC2, 0xCE, 0xF4, 0xF9, 0x0F, 0x7F, 
	0x45, 0x6D, 0x3D, 0xFE, 0x46, 0x97, 0x33, 0x5E, 
	0x08, 0xEF, 0xF1, 0xFF, 0x86, 0x83, 0x24, 0x74, 
	0x12, 0xFC, 0x00, 0x9F, 0xB4, 0xB7, 0x06, 0xD5, 
	0xD0, 0x7A, 0x00, 0x9E, 0x04, 0x5F, 0x41, 0x2F, 
	0x1D, 0x77, 0x36, 0x75, 0x81, 0xAA, 0x70, 0x3A, 
	0x98, 0xD1, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 
	0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0x0B, 0x00
};

static const uint8_t cgbObjpDump[0x40] = {
	0x00, 0x00, 0xF2, 0xAB, 
	0x61, 0xC2, 0xD9, 0xBA, 
	0x88, 0x6E, 0xDD, 0x63, 
	0x28, 0x27, 0xFB, 0x9F, 
	0x35, 0x42, 0xD6, 0xD4, 
	0x50, 0x48, 0x57, 0x5E, 
	0x23, 0x3E, 0x3D, 0xCA, 
	0x71, 0x21, 0x37, 0xC0, 
	0xC6, 0xB3, 0xFB, 0xF9, 
	0x08, 0x00, 0x8D, 0x29, 
	0xA3, 0x20, 0xDB, 0x87, 
	0x62, 0x05, 0x5D, 0xD4, 
	0x0E, 0x08, 0xFE, 0xAF, 
	0x20, 0x02, 0xD7, 0xFF, 
	0x07, 0x6A, 0x55, 0xEC, 
	0x83, 0x40, 0x0B, 0x77
};

Memory::Memory(const Interrupter &interrupter_in): display(memory + 0xFE00, vram), interrupter(interrupter_in) {
	romdata = NULL;
	rambankdata = NULL;
	cgb_wramdata = NULL;
	romfile = NULL;
	savedir = NULL;
	savename = NULL;
	getInput = NULL;
	init();
}

void Memory::init() {
	next_timatime = uint32_t(-1);
	tmatime = uint32_t(-1);
	next_unhalttime = uint32_t(-1);
	next_endtime = 0x102A0;
	next_dmatime = uint32_t(-1);
	next_hdmaReschedule = uint32_t(-1);
	next_blittime = 0x102A0 + 144 * 456 - 1;
	next_irqtime = uint32_t(-1);
	div_lastUpdate = 0;
	tima_lastUpdate = 0;
	hdma_transfer = 0;
	dmaSource = 0;
	dmaDestination = 0;
	std::memset(cgb_bgp_data, 0xFF, sizeof(cgb_bgp_data));
	battery = false;
	rtcRom = false;
	enable_ram = false;
	rambank_mode = false;
	rambank = 0;
	rombank = 1;
	next_serialtime = uint32_t(-1);
	rombanks = 1;
	IME = false;
	set_irqEvent();
	set_event();
	rtc.reset();

	std::memset(memory, 0x00, sizeof(memory));
	std::memset(disabled_ram, 0xFF, 0x1000);
	std::memset(vram, 0, 0x4000);
	
	for (unsigned addr = 0xC000; addr < 0xC800; addr += 0x10) {
		std::memset(memory + addr + 0x00, 0xFF, 0x08);
		std::memset(memory + addr + 0x08, 0x00, 0x08);
	}
	
	for (unsigned addr = 0xC800; addr < 0xD000; addr += 0x10) {
		std::memset(memory + addr + 0x00, 0x00, 0x08);
		std::memset(memory + addr + 0x08, 0xFF, 0x08);
	}
	
	for (unsigned addr = 0xCE00; addr < 0xD000; addr += 0x10) {
		memory[addr + 0x02] = 0xFF;
		memory[addr + 0x0A] = 0x00;
	}
	
	std::memcpy(memory + 0xD000, memory + 0xC000, 0x1000);
	
	std::memcpy(memory + 0xFEA0, feaxDump, sizeof(feaxDump));
	std::memcpy(memory + 0xFF00, ffxxDump, sizeof(ffxxDump));
	
	memory[0xFF04] = 0x1C;
	memory[0xFF0F] = 0xE0;
	memory[0xFF40] = 0x91;
	memory[0xFF41] = 0x80;
	memory[0xFF44] = 0x00;

	mem[0x0] = memory;
	mem[0x1] = &memory[0x1000];
	mem[0x2] = &memory[0x2000];
	mem[0x3] = &memory[0x3000];
	mem[0x4] = &memory[0x4000];
	mem[0x5] = &memory[0x5000];
	mem[0x6] = &memory[0x6000];
	mem[0x7] = &memory[0x7000];
	mem[0x8] = vram;
	mem[0x9] = mem[0x8] + 0x1000;
	mem[0xA] = disabled_ram;
	mem[0xB] = disabled_ram;
	mem[0xC] = &memory[0xC000];
	mem[0xD] = &memory[0xD000];
	mem[0xE] = mem[0xC];
	mem[0xF] = &memory[0xF000];
	
	for (unsigned i = 0x00; i < 0x40; i += 0x02) {
		cgb_bgp_data[i] = 0xFF;
		cgb_bgp_data[i + 1] = 0x7F;
	}
	
	std::memcpy(cgb_objp_data, cgbObjpDump, sizeof(cgbObjpDump));
}

void Memory::reset() {
	if (battery) saveram();
	init();
}

void Memory::reload() {
	reset();
	loadROM();
}

void Memory::refreshPalettes(const unsigned cycleCounter) {
	if (isCgb()) {
		for (unsigned i = 0; i < 8 * 8; i += 2) {
			display.cgbBgColorChange(i >> 1, cgb_bgp_data[i] | (cgb_bgp_data[i + 1] << 8), cycleCounter);
			display.cgbSpColorChange(i >> 1, cgb_objp_data[i] | (cgb_objp_data[i + 1] << 8), cycleCounter);
		}
	} else {
		display.dmgBgPaletteChange(memory[0xFF47], cycleCounter);
		display.dmgSpPalette1Change(memory[0xFF48], cycleCounter);
		display.dmgSpPalette2Change(memory[0xFF49], cycleCounter);
	}
}

void Memory::schedule_unhalt() {
	next_unhalttime = std::min(next_irqEventTime, display.nextIrqEvent()) + isCgb() * 4;
	set_event();
}

void Memory::rescheduleIrq(const unsigned cycleCounter) {
	if (IME) {
		memory[0xFF0F] |= display.getIfReg(cycleCounter) & 3;
		
		next_irqtime = (memory[0xFF0F] & memory[0xFFFF] & 0x1F) ? cycleCounter : std::min(next_irqEventTime, display.nextIrqEvent());
		
		set_event();
	}
}

void Memory::rescheduleHdmaReschedule() {
	if (hdma_transfer) {
		const unsigned newTime = display.nextHdmaTimeInvalid();
		
		if (newTime < next_hdmaReschedule) {
			next_hdmaReschedule = newTime;
			
			if (newTime < next_eventtime) {
				next_eventtime = newTime;
				next_event = HDMA_RESCHEDULE;
			}
		}
	}
}

void Memory::ei(const unsigned cycleCounter) {
	IME = 1;
	memory[0xFF0F] |= display.getIfReg(cycleCounter);
	
	const unsigned nextScheduled = std::min(next_irqEventTime, display.nextIrqEvent());
	next_irqtime = (((memory[0xFF0F] & memory[0xFFFF] & 0x1F) || nextScheduled < cycleCounter) ? cycleCounter : nextScheduled) + 1;
	
	if (next_irqtime < next_eventtime) {
		next_eventtime = next_irqtime;
		next_event = INTERRUPTS;
	}
}

void Memory::inc_endtime(const unsigned inc) {
	active = true;
	next_endtime += inc << isDoubleSpeed();
	set_event();
}

void Memory::set_irqEvent() {
	/*next_irqEvent = next_mode;
	next_irqEventTime = next_modetime;
	if (next_lyctime < next_irqEventTime) {
		next_irqEventTime = next_lyctime;
		next_irqEvent = LYC;
	}*/
	//next_irqEventTime = uint32_t(-1);
	//if (next_timatime < next_irqEventTime) {
		next_irqEventTime = next_timatime;
		next_irqEvent = TIMA;
	//}
	/*if (next_m0RescTime < next_irqEventTime) {
		next_irqEventTime = next_m0RescTime;
		next_irqEvent = M0RESC;
	}*/
	if (next_serialtime < next_irqEventTime) {
		next_irqEvent = SERIAL;
		next_irqEventTime = next_serialtime;
	}
}

void Memory::update_irqEvents(const unsigned cc) {
	while (next_irqEventTime <= cc) {
		switch (next_irqEvent) {
// 		case MODE0:
// 			printf("mode0 irq: ly: %u, m3exCycles: %u\n", display.get_ly(next_modetime + 1), lastM3ExtraCycles);
// 			{
// 				const uint_fast32_t ly = display.get_ly(next_modetime + 1);
// // 				printf("mode0 ly: %u, next_mode0: %u\n", ly, display.next_mode0());
// // 				if(/*lastModeIRQ!=ly && */(next_lyctime>next_modetime+linetime*154 || next_lyctime<next_modetime+linetime*(memory[0xFF45]?153:152))) {
// 				if (next_lyctime == uint32_t(-1) || (ly != memory[0xFF45])) {
// 					memory[0xFF0F] |= 2;
// 				}
// 				if (ly == 143) {
// 					next_mode = MODE1;
// 					next_modetime += mode0time - lastM3ExtraCycles;
// 				} /*else if(memory[0xFF41]&0x20) {
// 					next_mode=MODE2;
// 					next_modetime+=mode0time;
// 				} */else {
// 					const unsigned m3ec = display.m3ExtraCycles(ly + 1, CycleCounter);
// 				        next_modetime += linetime - lastM3ExtraCycles + m3ec;
// 				        lastM3ExtraCycles = m3ec;
// 				        /* if(reschedule_mode) {
// 				        	reschedule_mode=false;
// 				        	schedule_mode();
// 				        } */
// 				}
// 			}
// 			break;
// 		case MODE1:
// 			printf("mode1 irq\n");
// // 			printf("mode1 ly: %u, next_mode2:%u\n", display.get_ly(next_modetime), display.next_mode2());
// 			if (memory[0xFF41]&0x20) {
// 				next_modetime += linetime * 10/*-4*/;
// 				next_mode = MODE2;
// 			} else if (memory[0xFF41]&0x8) {
// 				const unsigned m3ec = display.m3ExtraCycles(0, CycleCounter);
// 				next_modetime += linetime * 10 + mode2time + mode3time + m3ec;
// 				lastM3ExtraCycles = m3ec;
// 				next_mode = MODE0;
// 			} else
// 				next_modetime += linetime * 154;
// 			memory[0xFF0F] |= 1;
// 			if (memory[0xFF41]&0x10) {
// 				memory[0xFF0F] |= 2;
// 			}
// 			break;
		/*case MODE2:
			{
			printf("mode2 irq\n");
			const uint_fast32_t ly = display.get_ly(next_modetime + 5);
			// printf("mode2 intended ly: %u, actual ly: %u, next_mode2:%u\n", ly, display.get_ly(CycleCounter), display.next_mode2());
			if (next_lyctime == uint32_t(-1) || (memory[0xFF45] && ly != (memory[0xFF45] + 1U)) || (memory[0xFF45] == 0 && ly > 1)) {
// 			if(next_lyctime==uint32_t(-1) || (next_lyctime<next_modetime+linetime*(memory[0xFF45]?153:152))) {
				memory[0xFF0F] |= 2;
			}
			next_modetime += linetime;
			if (ly == 0)
				next_modetime -= 4;
			else if (ly == 143) {
				next_modetime += 4;
				next_mode = MODE1;
			}
			}
			break;*/
// 		case LYC:
// 			{
// 			printf("lyc irq\n");
// // 			const uint_fast32_t ly=display.get_ly(next_lyctime);
// // 			printf("lyc intended ly: %u, actual ly: %u, lyc: %u, next_mode2:%u\n", ly, display.get_ly(CycleCounter), memory[0xFF45], display.next_mode2());
// 			if (!(memory[0xFF41]&0x20) || memory[0xFF45] > 143 || memory[0xFF45] == 0) {
// 				memory[0xFF0F] |= 0x2;
// 			}
// 			next_lyctime += linetime * 154;
// 			/* if(reschedule_lyc) {
// 				reschedule_lyc=false;
// 				if(enable_display && (memory[0xFF41]&0x40) && memory[0xFF45]<154) {
// 					next_lyctime=CycleCounter+display.next_lycIrq();
// 				} else
// 					next_lyctime=uint32_t(-1);
// 			} */
// 			}
// 			break;
		case TIMA:
// 			printf("tima irq\n");
// 			printf("TIMA intended ly: %u, actual ly: %u, next_mode2:%u\n", display.get_ly(next_timatime), display.get_ly(CycleCounter), display.next_mode2());
			// memory[0xFF05]=memory[0xFF06];
			// tima_lastUpdate=next_timatime;
			memory[0xFF0F] |= 4;
			next_timatime += (256 - memory[0xFF06]) << timaClock[memory[0xFF07] & 3];
			break;
		/*case M0RESC:
			if (next_mode == MODE0) {
				const unsigned m3ec = display.m3ExtraCycles(display.get_ly(next_modetime), CycleCounter);
				next_modetime -= lastM3ExtraCycles;
				next_modetime += m3ec;
				lastM3ExtraCycles = m3ec;
			}
			next_m0RescTime = display.nextMode3Update(CycleCounter);
			break;*/
		case SERIAL:
			next_serialtime = uint32_t(-1);
			memory[0xFF01] = 0xFF;
			memory[0xFF02] &= 0x7F;
			memory[0xFF0F] |= 8;
			break;
		}
		set_irqEvent();
	}
}

void Memory::set_event() {
	next_event = INTERRUPTS;
	next_eventtime = next_irqtime;
	if (next_hdmaReschedule < next_eventtime) {
		next_eventtime = next_hdmaReschedule;
		next_event = HDMA_RESCHEDULE;
	}
	if (next_dmatime < next_eventtime) {
		next_eventtime = next_dmatime;
		next_event = DMA;
	}
	if (next_unhalttime < next_eventtime) {
		next_eventtime = next_unhalttime;
		next_event = UNHALT;
	}
	if (next_blittime < next_eventtime) {
		next_event = BLIT;
		next_eventtime = next_blittime;
	}
	if (next_endtime < next_eventtime) {
		next_eventtime = next_endtime;
		next_event = END;
	}
}

unsigned Memory::event(unsigned cycleCounter) {
	switch (next_event) {
	case HDMA_RESCHEDULE:
// 		printf("hdma_reschedule\n");
		next_dmatime = display.nextHdmaTime(cycleCounter);
		next_hdmaReschedule = display.nextHdmaTimeInvalid();
		break;
	case DMA:
// 		printf("dma\n");
		{
			const bool doubleSpeed = isDoubleSpeed();
			uint16_t dmaSrc = dmaSource;
			unsigned dmaDest = dmaDestination;
			unsigned dmaLength = ((memory[0xFF55] & 0x7F) + 0x1) * 0x10;
			
			cycleCounter += (6 >> doubleSpeed) + doubleSpeed;
			
			unsigned length = hdma_transfer ? 0x10 : dmaLength;
			
			if ((dmaDest + length) & 0x10000) {
				length = 0x10000 - dmaDest;
				memory[0xFF55] |= 0x80;
			}
			
			dmaLength -= length;
			if (!(memory[0xFF40] & 0x80))
				dmaLength = 0;
			
			while (length--) {
				write(0x8000 | (dmaDest++ & 0x1FFF), read(dmaSrc++, cycleCounter), cycleCounter);
				cycleCounter += 2 << doubleSpeed;
			}
			
			dmaSource = dmaSrc;
			dmaDestination = dmaDest;
			memory[0xFF55] = (dmaLength / 0x10 - 0x1) | (memory[0xFF55] & 0x80);
			
			if (memory[0xFF55] & 0x80) {
				next_hdmaReschedule = next_dmatime = uint32_t(-1);
				hdma_transfer = 0;
			}
			
			if (hdma_transfer)
				next_dmatime = display.nextHdmaTime(cycleCounter);
		}
		break;
	case INTERRUPTS:
// 		printf("interrupts\n");
		update_irqEvents(cycleCounter);
		memory[0xFF0F] |= display.getIfReg(cycleCounter) & 3;
		{
			/*unsigned interrupt = memory[0xFF0F] & memory[0xFFFF];
			interrupt |= interrupt << 1;
			interrupt |= interrupt << 2;
			interrupt |= interrupt << 1;
			interrupt = ~interrupt;
			++interrupt;
			interrupt &= 0x1F;
			
			if (interrupt) {
				memory[0xFF0F] &= ~interrupt;
				display.setIfReg(memory[0xFF0F], CycleCounter);
				IME = false;
				
				unsigned address = interrupt;
				interrupt >>= 1;
				address -= interrupt & 0x0C;
				interrupt >>= 1;
				address -= interrupt & 5;
				address += interrupt >> 2;
				
				address <<= 3;
				address += 0x38;
				
				z80.interrupt(address);
			}*/
			
			const unsigned interrupt = memory[0xFF0F] & memory[0xFFFF] & 0x1F;
			
			if (interrupt) {
				unsigned n;
				unsigned address;
				
				if ((n = interrupt & 0x01))
					address = 0x40;
				else if ((n = interrupt & 0x02))
					address = 0x48;
				else if ((n = interrupt & 0x04))
					address = 0x50;
				else if ((n = interrupt & 0x08))
					address = 0x58;
				else {
					n = 0x10;
					address = 0x60;
				}
				
				memory[0xFF0F] &= ~n;
				display.setIfReg(memory[0xFF0F], cycleCounter);
				IME = false;
				cycleCounter = interrupter.interrupt(address, cycleCounter, *this);
			}
		}
		next_irqtime = IME ? std::min(next_irqEventTime, display.nextIrqEvent()) : uint32_t(-1);
		break;
	case BLIT:
// 		printf("blit\n");
		display.updateScreen(next_blittime);
		
		if (memory[0xFF40] & 0x80)
			next_blittime += 70224 << isDoubleSpeed();
		else
			next_blittime = uint32_t(-1);
		break;
	case UNHALT:
// 		printf("unhalt\n");
		update_irqEvents(cycleCounter);
		memory[0xFF0F] |= display.getIfReg(cycleCounter) & 3;
		// if(IME) {
		// 	next_irqtime=(memory[0xFF0F]&memory[0xFFFF]&0xF)?CycleCounter:((next_irqEventTime+1)?(next_irqEventTime):next_irqEventTime);
		// 	set_event();
		// }
		if (memory[0xFF0F] & memory[0xFFFF] & 0x1F) {
			next_unhalttime = uint32_t(-1);
			interrupter.unhalt();
		} else
			next_unhalttime = std::min(next_irqEventTime, display.nextIrqEvent()) + isCgb() * 4;
		break;
	case END:
// 		printf("end\n");
		active = false;
		return cycleCounter;
	}
	
	set_event();
	
	return cycleCounter;
}

void Memory::speedChange(const unsigned cycleCounter) {
	if (isCgb() && (memory[0xFF4D] & 0x1)) {
		printf("speedChange\n");
		
		update_irqEvents(cycleCounter);
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		display.preSpeedChange(cycleCounter);
		
		memory[0xFF4D] = ~memory[0xFF4D] & 0x80;
		
		display.postSpeedChange(cycleCounter);
		
		if (hdma_transfer) {
			next_dmatime = display.nextHdmaTime(cycleCounter);
			next_hdmaReschedule = display.nextHdmaTimeInvalid();
		}
		
		next_blittime = (memory[0xFF40] & 0x80) ? display.nextMode1IrqTime() : uint32_t(-1);
		next_endtime = cycleCounter + (isDoubleSpeed() ? next_endtime - cycleCounter << 1 : (next_endtime - cycleCounter >> 1));
		set_irqEvent();
		rescheduleIrq(cycleCounter);
		set_event();
	}
}

unsigned Memory::resetCounters(unsigned cycleCounter) {
	printf("RESETTING COUNTERS!!!!!!!!!!!!!!!!111111111\n");
	update_irqEvents(cycleCounter);
	rescheduleIrq(cycleCounter);
	display.preResetCounter(cycleCounter);

	const unsigned oldCC = cycleCounter;
	
	{
		const unsigned divinc = (cycleCounter - div_lastUpdate) >> 8;
		memory[0xFF04] += divinc;
		div_lastUpdate += divinc << 8;
	}

	if (memory[0xFF07] & 0x04) {
		update_tima(cycleCounter);
	}
	
	const unsigned dec = 0x80000000 - 1024;

	div_lastUpdate -= dec;
	
	if (memory[0xFF07] & 0x04)
		tima_lastUpdate -= dec;
	
	next_eventtime -= dec;
	if (next_irqEventTime != uint32_t(-1))
		next_irqEventTime -= dec;
	if (next_timatime != uint32_t(-1))
		next_timatime -= dec;
	if (next_blittime != uint32_t(-1))
		next_blittime -= dec;
	next_endtime -= dec;
	if (next_dmatime != uint32_t(-1)) next_dmatime -= dec;
	if (next_hdmaReschedule != uint32_t(-1)) next_hdmaReschedule -= dec;
	if (next_irqtime != uint32_t(-1)) next_irqtime -= dec;
	if (next_serialtime != uint32_t(-1)) next_serialtime -= dec;
	if (tmatime != uint32_t(-1))
		tmatime -= dec;
	
	cycleCounter -= dec;
	
	display.postResetCounter(oldCC, cycleCounter);
	sound.resetCounter(cycleCounter, oldCC, isDoubleSpeed());
	
	return cycleCounter;
}

void Memory::updateInput() {
	unsigned button = 0xFF;
	unsigned dpad = 0xFF;

	if (getInput) {
		const InputState &is = (*getInput)();

		button ^= is.startButton << 3;
		button ^= is.selectButton << 2;
		button ^= is.bButton << 1;
		button ^= is.aButton;

		dpad ^= is.dpadDown << 3;
		dpad ^= is.dpadUp << 2;
		dpad ^= is.dpadLeft << 1;
		dpad ^= is.dpadRight;
	}

	memory[0xFF00] |= 0xF;
	
	if (!(memory[0xFF00] & 0x10))
		memory[0xFF00] &= dpad;
	
	if (!(memory[0xFF00] & 0x20))
		memory[0xFF00] &= button;
}

void Memory::swap_rombank() {
	unsigned bank = rombank;
	
	if (romtype == mbc1 && !(bank & 0x1F) || romtype == mbc5 && !bank)
		++bank;
	
	mem[0x4] = &romdata[bank*0x4000];
	mem[0x5] = mem[0x4] + 0x1000;
	mem[0x6] = mem[0x4] + 0x2000;
	mem[0x7] = mem[0x4] + 0x3000;
	rombankptr = mem[0x4] - 0x4000;
}

//TODO: Accurate implementation.
void Memory::oamDma(const unsigned cycleCounter) {
	const unsigned address = memory[0xFF46] << 8;

	unsigned i = 0;
	
	while (memory[0xFE00 | i] == read(address | i, cycleCounter) && i < 0xA0)
		++i;

	if (i < 0xA0) {
		display.oamChange(cycleCounter);
		rescheduleIrq(cycleCounter);
		rescheduleHdmaReschedule();
		
		do {
			memory[0xFE00 | i] = read(address | i, cycleCounter);
			++i;
		} while (i < 0xA0);
	}
}

void Memory::update_tima(const unsigned cycleCounter) {
	const unsigned ticks = cycleCounter - tima_lastUpdate >> timaClock[memory[0xFF07] & 3];
	
	tima_lastUpdate += ticks << timaClock[memory[0xFF07] & 3];
	
	if (cycleCounter >= tmatime) {
		if (cycleCounter >= tmatime + 4)
			tmatime = uint32_t(-1);
		
		memory[0xFF05] = memory[0xFF06];
	}
	
	unsigned tmp = memory[0xFF05] + ticks;
	
	while (tmp > 0x100)
		tmp -= 0x100 - memory[0xFF06];
	
	if (tmp == 0x100) {
		tmatime = tima_lastUpdate + 3;
		
		if (cycleCounter >= tmatime) {
			if (cycleCounter >= tmatime + 4)
				tmatime = uint32_t(-1);
			
			tmp = memory[0xFF06];
		}
	}
	
	memory[0xFF05] = tmp;
}

uint8_t Memory::ff_read(const unsigned P, const unsigned cycleCounter) {
	switch (P & 0xFF) {
	case 0x00:
		updateInput();
		break;
	case 0x04:
// 		printf("div read\n");
		{
			const unsigned divcycles = (cycleCounter - div_lastUpdate) >> 8;
			memory[P] += divcycles;
			div_lastUpdate += divcycles << 8;
		}
		break;
	case 0x05:
// 		printf("tima read\n");
		if (memory[0xFF07] & 0x04)
			update_tima(cycleCounter);
		break;
	case 0x0F:
		update_irqEvents(cycleCounter);
		memory[0xFF0F] |= display.getIfReg(cycleCounter) & 3;
		rescheduleIrq(cycleCounter);
		break;
	case 0x26:
// 		printf("sound status read\n");
		if (memory[P] & 0x80) {
			sound.generate_samples(cycleCounter, isDoubleSpeed());
			memory[P] = 0xF0 | sound.getStatus();
		} else
			memory[P] = 0x70;
		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		return sound.waveRamRead(P & 0xF);
	case 0x41:
		return memory[P] | display.get_stat(memory[0xFF45], cycleCounter);
	case 0x44:
		return display.getLyReg(cycleCounter/*+4*/);
	case 0x69:
		if(!isCgb())
			break;
		
		if (display.cgbpAccessible(cycleCounter))
			return cgb_bgp_data[memory[0xFF68] & 0x3F];
		
		return 0xFF;
	case 0x6B:
		if(!isCgb())
			break;
		
		if (display.cgbpAccessible(cycleCounter))
			return cgb_objp_data[memory[0xFF6A] & 0x3F];
		
		return 0xFF;
	default: break;
	}
	
	return memory[P];
}

uint8_t Memory::read(const unsigned P, const unsigned cycleCounter) {
	if ((P & 0xC000) == 0x8000) {
		if (P & 0x2000) {
			if (rtc.getActive())
				return *rtc.getActive();
		} else if (!display.vramAccessible(cycleCounter))
			return 0xFF;
	} else if (((P + 0x80) & 0xFFFF) >= 0xFE80) {
		if (P & 0x100)
			return ff_read(P, cycleCounter);
		
		if (!display.oamAccessible(cycleCounter)) {
			return 0xFF;
		}
	}

	return mem[P >> 12][P & 0xFFF];
}

void Memory::ff_write(const uint16_t P, uint8_t data, const unsigned cycleCounter) {
// 	printf("mem[0x%X] = 0x%X\n", P, data);
	switch (P & 0xFF) {
	case 0x00:
		memory[P] &= 0xCF;
		memory[P] |= data & 0xF0;
		return;
	case 0x01:
		update_irqEvents(cycleCounter);
// 		if (IME) {
// 			next_irqtime = (memory[0xFF0F] & memory[0xFFFF] & 0x1F) ? CycleCounter : std::min(next_irqEventTime, display.nextIrqEvent());
// 			set_event();
// 		}
		break;
	case 0x02:
		update_irqEvents(cycleCounter);
		
		if ((data & 0x81) == 0x81) {
			next_serialtime = cycleCounter;
			next_serialtime += (isCgb() && (data & 0x2)) ? 128 : 4096;
			set_irqEvent();
		}
		
		rescheduleIrq(cycleCounter);
		break;
		//If rom is trying to write to DIV register, reset it to 0.
	case 0x04:
// 		printf("DIV write\n");
		memory[P] = 0;
		div_lastUpdate = cycleCounter;
		return;
	case 0x05:
		// printf("tima write\n");
		if (memory[0xFF07] & 0x04) {
			update_irqEvents(cycleCounter);
			update_tima(cycleCounter);
			
			if (tmatime - cycleCounter < 4)
				tmatime = uint32_t(-1);
			
			next_timatime = tima_lastUpdate + ((256 - data) << timaClock[memory[0xFF07] & 3]) + 1;
			set_irqEvent();
			rescheduleIrq(cycleCounter);
		}
		break;
	case 0x06:
		if (memory[0xFF07] & 0x04) {
			update_irqEvents(cycleCounter);
			update_tima(cycleCounter);
			// set_irqEvent();
// 			if (IME) {
// 				next_irqtime = (memory[0xFF0F] & memory[0xFFFF] & 0x1F) ? CycleCounter : std::min(next_irqEventTime, display.nextIrqEvent());
// 				set_event();
// 			}
		}
		break;
	case 0x07:
		// printf("tac write: %i\n", data);
		if ((memory[P] ^ data) & 7) {
			if (memory[P] & 0x04) {
				update_irqEvents(cycleCounter);
				update_tima(cycleCounter);
				
				tima_lastUpdate -= (1 << (timaClock[memory[P] & 3] - 1)) + 3;
				tmatime -= (1 << (timaClock[memory[P] & 3] - 1)) + 3;
				next_timatime -= (1 << (timaClock[memory[P] & 3] - 1)) + 3;
				set_irqEvent();
				update_tima(cycleCounter);
				update_irqEvents(cycleCounter);
				
				tmatime = uint32_t(-1);
				next_timatime = uint32_t(-1);
			}
			
			if (data & 4) {
				tima_lastUpdate = (cycleCounter >> timaClock[data & 3]) << timaClock[data & 3];
				next_timatime = tima_lastUpdate + (256 - memory[0xFF05] << timaClock[data & 3]) + 1;
			}
			
			memory[P] = 0xF8 | (data & 0x7);
			set_irqEvent();
			rescheduleIrq(cycleCounter);
		}
		return;
	case 0x0F:
		update_irqEvents(cycleCounter);
		display.setIfReg(data, cycleCounter);
		memory[P] = 0xE0 | data;
		rescheduleIrq(cycleCounter);
		return;
	case 0x10:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr10(data);
		data |= 0x80;
		break;
	case 0x11:
		if(!sound.isEnabled()) {
			if (isCgb())
				return;
			
			data &= 0x3F;
		}
		
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr11(data);
		data |= 0x3F;
		break;
	case 0x12:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr12(data);
		break;
	case 0x13:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr13(data);
		return;
	case 0x14:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr14(data);
		data |= 0xBF;
		break;
	case 0x15: return;
	case 0x16:
		if(!sound.isEnabled()) {
			if (isCgb())
				return;
			
			data &= 0x3F;
		}
		
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr21(data);
		data |= 0x3F;
		break;
	case 0x17:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr22(data);
		break;
	case 0x18:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr23(data);
		return;
	case 0x19:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr24(data);
		data |= 0xBF;
		break;
	case 0x1A:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr30(data);
		data |= 0x7F;
		break;
	case 0x1B:
		if(!sound.isEnabled() && isCgb()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr31(data);
		return;
	case 0x1C:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr32(data);
		data |= 0x9F;
		break;
	case 0x1D:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr33(data);
		return;
	case 0x1E:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr34(data);
		
// 		if (data & 0x80) {
// 			for (unsigned i = 0; i < 16; ++i)
// 				memory[0xFF30 | i] ^= 0x80 | 0x20 | 0x8 | 0x2;
// 		}
		
		data |= 0xBF;
		break;
	case 0x1F: return;
	case 0x20:
		if(!sound.isEnabled() && isCgb()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr41(data);
		return;
	case 0x21:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr42(data);
		break;
	case 0x22:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr43(data);
		break;
	case 0x23:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_nr44(data);
		data |= 0xBF;
		break;
	case 0x24:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.set_so_volume(data);
		break;
	case 0x25:
		if(!sound.isEnabled()) return;
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.map_so(data);
		break;
	case 0x26:
		if ((memory[P] ^ data) & 0x80) {
			sound.generate_samples(cycleCounter, isDoubleSpeed());
			if (!(data & 0x80)) {
				for (unsigned i = 0xFF10; i < 0xFF26; ++i)
					ff_write(i, 0, cycleCounter);
				
// 				std::memcpy(memory + 0xFF10, soundRegInitValues, sizeof(soundRegInitValues));
				sound.setEnabled(false);
			} else {
				sound.reset(/*memory + 0xFF00, isDoubleSpeed()*/);
				sound.setEnabled(true);
			}
		}
		data = data & 0x80 | memory[P] & 0x7F;
		break;
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F: return;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		sound.generate_samples(cycleCounter, isDoubleSpeed());
		sound.waveRamWrite(P & 0xF, data);
		break;
	case 0x40:
		if (memory[P] != data) {
			if ((memory[P] ^ data) & 0x80) {
				update_irqEvents(cycleCounter);
				const unsigned lyc = display.get_stat(memory[0xFF45], cycleCounter) & 4;
				display.enableChange(memory[0xFF41], cycleCounter);
				memory[0xFF44] = 0;
// 				enable_display = bool(data & 0x80);
				memory[0xFF41] &= 0xF8;
				
				if (data & 0x80) {
					next_blittime = display.nextMode1IrqTime() + (70224 << isDoubleSpeed());
				} else {
					memory[0xFF41] |= lyc; //Mr. Do! needs conicidence flag preserved.
					next_blittime = cycleCounter + (456 * 4 << isDoubleSpeed());
					
					if (hdma_transfer)
						next_dmatime = cycleCounter;
					
					next_hdmaReschedule = uint32_t(-1);
				}

				set_event();
			}
			
			if ((memory[P] ^ data) & 0x4) {
				display.spriteSizeChange(data & 0x4, cycleCounter);
			}
			
			if ((memory[P] ^ data) & 0x20) {
// 				printf("%u: weChange to %u\n", CycleCounter, (data & 0x20) != 0);
				display.weChange(data & 0x20, cycleCounter);
			}
			
			if ((memory[P] ^ data) & 0x40)
				display.wdTileMapSelectChange(data & 0x40, cycleCounter);
			
			if ((memory[P] ^ data) & 0x08)
				display.bgTileMapSelectChange(data & 0x08, cycleCounter);
			
			if ((memory[P] ^ data) & 0x10)
				display.bgTileDataSelectChange(data & 0x10, cycleCounter);
			
			if ((memory[P] ^ data) & 0x02)
				display.spriteEnableChange(data & 0x02, cycleCounter);
			
			if ((memory[P] ^ data) & 0x01)
				display.bgEnableChange(data & 0x01, cycleCounter);
			
			//scheduleM0Resc();
			rescheduleIrq(cycleCounter);
			rescheduleHdmaReschedule();
			
			memory[P] = data;
		}
		return;
	case 0x41:
		display.lcdstatChange(memory[0xFF41], data, cycleCounter);
		memory[P] = (memory[P] & 0x87) | (data & 0x78);
		rescheduleIrq(cycleCounter);
		return;
	case 0x42:
		if (memory[P] != data) {
			display.scyChange(data, cycleCounter);
		}
		break;
	case 0x43:
		if (memory[P] != data) {
			display.scxChange(data, cycleCounter);
			//scheduleM0Resc();
			rescheduleIrq(cycleCounter);
			rescheduleHdmaReschedule();
		}
		break;
		//If rom is trying to write to LY register, reset it to 0.
	case 0x44:
		printf("OMFG!!!! LY WRITE!!\n");
// 		update_irqEvents(CycleCounter);
		memory[P] = 0;
		display.enableChange(memory[0xFF41], cycleCounter);
		display.enableChange(memory[0xFF41], cycleCounter);
		// lastModeIRQ=0xFF;
		next_blittime = (memory[0xFF40] & 0x80) ? display.nextMode1IrqTime() : uint32_t(-1);
		/*if (enable_display && (memory[0xFF41]&0x40) && memory[0xFF45] < 154) {
			// lastLycIRQ=uint32_t(-1);
			if (memory[0xFF45])
				next_lyctime = CycleCounter + memory[0xFF45] * linetime - 1;
			else
				next_lyctime = CycleCounter + linetime * 153 + (8 << doubleSpeed) - 1;
			// set_event();
		}
		set_irqEvent();*/
		if (hdma_transfer) {
			next_dmatime = display.nextHdmaTime(cycleCounter);
			next_hdmaReschedule = display.nextHdmaTimeInvalid();
		}
		set_event();
		rescheduleIrq(cycleCounter);
		/* LYC(); resetCycles();*/
		return;
	case 0x45:
		display.lycRegChange(data, memory[0xFF41], cycleCounter);
		rescheduleIrq(cycleCounter);
		break;
	case 0x46:
		memory[P] = data;
		oamDma(cycleCounter);
		return;
	case 0x47:
		if (!isCgb() && memory[P] != data) {
			display.dmgBgPaletteChange(data, cycleCounter);
		}
		break;
	case 0x48:
		if (!isCgb() && memory[P] != data) {
			display.dmgSpPalette1Change(data, cycleCounter);
		}
		break;
	case 0x49:
		if (!isCgb() && memory[P] != data) {
			display.dmgSpPalette2Change(data, cycleCounter);
		}
		break;
	case 0x4A:
		if (memory[P] != data) {
// 			printf("%u: wyChange to %u\n", CycleCounter, data);
			display.wyChange(data, cycleCounter);
// 			scheduleM0Resc();
			rescheduleIrq(cycleCounter);
			rescheduleHdmaReschedule();
		}
		break;
	case 0x4B:
		if (memory[P] != data) {
// 			printf("%u: wxChange to %u\n", CycleCounter, data);
			display.wxChange(data, cycleCounter);
// 			scheduleM0Resc();
			rescheduleIrq(cycleCounter);
			rescheduleHdmaReschedule();
		}
		break;

		//cgb stuff:
	case 0x4D:
		memory[P] |= data & 0x01;
		return;
		//Select vram bank
	case 0x4F:
		if (isCgb()) {
			//cgb_vrambank = (data & 0x01);
			mem[0x8] = vram + (data & 0x01) * 0x2000;
			mem[0x9] = mem[0x8] + 0x1000;
		}
		memory[P] = 0xFE | (data/*&0x01*/);
		return;
	case 0x51:
		if (!isCgb())
			break;
		
		dmaSource = (data << 8) | (dmaSource & 0xFF);
		return;
	case 0x52:
		if (!isCgb())
			break;
		
		dmaSource = (dmaSource & 0xFF00) | (data & 0xF0);
		return;
	case 0x53:
		if (!isCgb())
			break;
		
		dmaDestination = (data << 8) | (dmaDestination & 0xFF);
		return;
	case 0x54:
		if (!isCgb())
			break;
		
		dmaDestination = (dmaDestination & 0xFF00) | (data & 0xF0);
		return;
	case 0x55:
		if (!isCgb())
			break;
		
		memory[P] = data & 0x7F;
		
		if (hdma_transfer) {
			if (!(data&0x80)) {
				memory[P] |= 0x80;
				
				if (next_dmatime > cycleCounter) {
					hdma_transfer = 0;
					next_hdmaReschedule = next_dmatime = uint32_t(-1);
					set_event();
				}
			}
			return;
		}
		
		if (data & 0x80) {
			hdma_transfer = 1;
			
			if (!(memory[0xFF40] & 0x80) || display.isHdmaPeriod(cycleCounter)) {
				next_dmatime = cycleCounter;
				next_hdmaReschedule = uint32_t(-1);
			} else {
				next_dmatime = display.nextHdmaTime(cycleCounter);
				next_hdmaReschedule = display.nextHdmaTimeInvalid();
			}
		} else
			next_dmatime = cycleCounter;
		
		set_event();
		return;
		//Set bg palette index
// 	case 0x68:
// 		cgb_bgp_index = (data & 0x3F);
// 		cgb_bgp_autoInc = (data & 0x80);
// 		break;
		//Write to bg palette data
	case 0x69:
		{
			const unsigned cgb_bgp_index = memory[0xFF68] & 0x3F;
			
			if (cgb_bgp_data[cgb_bgp_index] != data) {
				if (display.cgbpAccessible(cycleCounter)) {
					cgb_bgp_data[cgb_bgp_index] = data;
					display.cgbBgColorChange(cgb_bgp_index >> 1, cgb_bgp_data[cgb_bgp_index & ~1] | (cgb_bgp_data[cgb_bgp_index | 1] << 8), cycleCounter);
				} /*else
					printf("palette write during mode3 at line %i. Next mode0: %u\n", display.get_ly(CycleCounter), display.next_mode0(CycleCounter));*/
			}
			
			memory[0xFF68] = memory[0xFF68] & ~0x3F | (cgb_bgp_index + (memory[0xFF68] >> 7)) & 0x3F;
		}
		
		return;
// 	case 0x6A:
// 		cgb_objp_index = (data & 0x3F);
// 		cgb_objp_autoInc = (data & 0x80);
// 		break;
		//Write to obj palette data.
	case 0x6B:
		{
			const unsigned cgb_objp_index = memory[0xFF6A] & 0x3F;
			
			if (cgb_objp_data[cgb_objp_index] != data) {
				if (display.cgbpAccessible(cycleCounter)) { //fixes color panel demo/lego racers
					cgb_objp_data[cgb_objp_index] = data;
					display.cgbSpColorChange(cgb_objp_index >> 1, cgb_objp_data[cgb_objp_index & ~1] | (cgb_objp_data[cgb_objp_index | 1] << 8), cycleCounter);
				} /*else
					printf("palette write during mode3 at line %i. Next mode0: %u\n", display.get_ly(CycleCounter), display.next_mode0(CycleCounter));*/
			}
			
			memory[0xFF6A] = memory[0xFF6A] & ~0x3F | (cgb_objp_index + (memory[0xFF6A] >> 7)) & 0x3F;
		}
		
		return;
	case 0x70:
		if (isCgb()) {
			unsigned bank = data & 0x07;
			
			if (!bank)
				bank = 1;
			
			mem[0xD] = cgb_wramdata + bank * 0x1000;
			std::memcpy(mem[0xF], mem[0xD], 0xE00); //sync wram-mirror.
		}
		break;

	case 0xFF:
		memory[P] = data;
		rescheduleIrq(cycleCounter);
		break;
	default:
		break;
	}
	
	memory[P] = data;
}

void Memory::mbc_write(const uint16_t P, const uint8_t data) {
// 	printf("mem[0x%X] = 0x%X\n", P, data);
	
	switch ((P >> 12) & 0x7) {
	case 0x0:
	case 0x1: //Most MBCs write 0x?A to addresses lower than 0x2000 to enable ram.
		if (romtype == mbc2 && (P&0x0100)) break;
		
		enable_ram = ((data & 0x0F) == 0xA);
		
		if (rtcRom)
			rtc.setEnabled(enable_ram);
		
		if (enable_ram) {
			mem[0xA] = &rambankdata[rambank*0x2000];
			mem[0xB] = mem[0xA] + 0x1000;
		} else {
			mem[0xA] = disabled_ram;
			mem[0xB] = disabled_ram;
		}
		
		break;
		//MBC1 writes ???n nnnn to address area 0x2000-0x3FFF, ???n nnnn makes up the lower digits to determine which rombank to load.
		//MBC3 writes ?nnn nnnn to address area 0x2000-0x3FFF, ?nnn nnnn makes up the lower digits to determine which rombank to load.
		//MBC5 writes nnnn nnnn to address area 0x2000-0x2FFF, nnnn nnnn makes up the lower digits to determine which rombank to load.
		//MBC5 writes bit8 of the number that determines which rombank to load to address 0x3000-0x3FFF.
	case 0x2:
		switch (romtype) {
		case plain:
			return;
		case mbc5:
			rombank = (rombank & 0x100) | data;
			rombank = rombank % rombanks;
			swap_rombank();
			return;
		default:
			break; //Only supposed to break one level.
		}
	case 0x3:
		switch (romtype) {
		case mbc1:
			rombank = rambank_mode ? (data & 0x1F) : ((rombank & 0x60) | (data & 0x1F));
			break;
		case mbc2:
			if (P & 0x0100) {
				rombank = data & 0x0F;
				break;
			}
			
			return;
		case mbc3:
			rombank = data & 0x7F;
			break;
		case mbc5:
			rombank = ((data & 0x1) << 8) | (rombank & 0xFF);
			break;
		default:
			return;
		}
		
		rombank = rombank % rombanks;
		swap_rombank();
		break;
		//MBC1 writes ???? ??nn to area 0x4000-0x5FFF either to determine rambank to load, or upper 2 bits of the rombank number to load, depending on rom-mode.
		//MBC3 writes ???? ??nn to area 0x4000-0x5FFF to determine rambank to load
		//MBC5 writes ???? nnnn to area 0x4000-0x5FFF to determine rambank to load
	case 0x4:
	case 0x5:
		switch (romtype) {
		case mbc1:
			if (rambank_mode) {
				rambank = (data & 0x03);
				break;
			}
			
			rombank = (((data & 0x03) << 5) | (rombank & 0x1F));
			rombank = rombank % rombanks;
			swap_rombank();
			return;
		case mbc3:
			if (rtcRom)
				rtc.swapActive(data);
			
			rambank = data & 0x03;
			break;
		case mbc5:
			rambank = (data & 0x0F);
			break;
		default:
			return;
		}
		
		rambank &= rambanks - 1;
		
		if (enable_ram) {
			mem[0xA] = &rambankdata[rambank*0x2000];
			mem[0xB] = mem[0xA] + 0x1000;
		}
		
		break;
		//MBC1: If ???? ???1 is written to area 0x6000-0x7FFFF rom will be set to rambank mode.
	case 0x6:
	case 0x7:
		switch (romtype) {
		case mbc1:
			rambank_mode = (data & 0x01);
			break;
		case mbc3:
			rtc.latch(data);
			break;
		default:
			break;
		}
		
		break;
//     default: break;
	}
}

void Memory::write(const uint16_t P, const uint8_t data, const unsigned cycleCounter) {
	if (P < 0x8000) {
		mbc_write(P, data);
		return;
	}

	if (P < 0xA000) {
		if (mem[P>>12][P&0xFFF] != data) {
			if (memory[0xFF40] & 0x80) {
				if (!display.vramAccessible(cycleCounter)) {
// 					printf("vram write during mode3 at line %i. Next mode0: %u\n", display.get_ly(CycleCounter), display.next_mode0(CycleCounter));
					return;
				}
				display.vramChange(cycleCounter);
			}
		}
	} else if (P < 0xC000) {
		if (!enable_ram)
			return;
		
		if (rtc.getActive()) {
			rtc.write(data);
			return;
		}
	} else if (P >= 0xD000 && P < 0xDE00) {
		memory[P+0x2000] = data;
	} else if (P >= 0xF000) {
		if (P < 0xFE00)
			mem[0xD][P&0xFFF] = data;
		else if (P < 0xFEA0) {
			if (memory[P] != data) {
				if (memory[0xFF40] & 0x80) {
					if (!display.oamAccessible(cycleCounter)) { //mode2 or mode3.
// 						printf("oam write during mode2 at line %i\n", display.get_ly(CycleCounter));
						return;
					}
				}
				
				display.oamChange(cycleCounter);
				//scheduleM0Resc();
				rescheduleIrq(cycleCounter);
				rescheduleHdmaReschedule();
			}
		} else if ((P < 0xFF80 && P >= 0xFF00) || P == 0xFFFF) {
			ff_write(P, data, cycleCounter);
			return;
		}
		memory[P] = data;
		return;
	}

	mem[P>>12][P&0xFFF] = data;
}

bool Memory::loadROM(const char* file) {
	if (romfile != NULL) {
		delete []romfile;
		romfile = NULL;
	}
	romfile = new char[std::strlen(file)+1];
	std::strcpy(romfile, file);
	return loadROM();
}

bool Memory::loadROM() {
	delete[]cgb_wramdata;
	cgb_wramdata = NULL;

	File rom(romfile);
	
	if (!rom.is_open()) {
		return 1;
	}
	
	unsigned size = 32768;
	rom.read(reinterpret_cast<char*>(&memory[0x0]), size);

	if (isCgb()) {
// 		cgb = 1;
		cgb_wramdata = new uint8_t[0x8000];
		
		std::memcpy(cgb_wramdata, memory + 0xC000, 0x2000);
		std::memcpy(cgb_wramdata + 0x2000, cgb_wramdata, 0x2000);
		std::memcpy(cgb_wramdata + 0x4000, cgb_wramdata, 0x4000);
		
		mem[0xC] = &cgb_wramdata[0];
		mem[0xD] = &cgb_wramdata[0x1000];
		mem[0xE] = mem[0xC];
		
		memory[0xFF30] = 0x00;
		memory[0xFF31] = 0xFF;
		memory[0xFF32] = 0x00;
		memory[0xFF33] = 0xFF;
		memory[0xFF34] = 0x00;
		memory[0xFF35] = 0xFF;
		memory[0xFF36] = 0x00;
		memory[0xFF37] = 0xFF;
		memory[0xFF38] = 0x00;
		memory[0xFF39] = 0xFF;
		memory[0xFF3A] = 0x00;
		memory[0xFF3B] = 0xFF;
		memory[0xFF3C] = 0x00;
		memory[0xFF3D] = 0xFF;
		memory[0xFF3E] = 0x00;
		memory[0xFF3F] = 0xFF;
	} else {
		memory[0xFF30] = 0xAC;
		memory[0xFF31] = 0xDD;
		memory[0xFF32] = 0xDA;
		memory[0xFF33] = 0x48;
		memory[0xFF34] = 0x36;
		memory[0xFF35] = 0x02;
		memory[0xFF36] = 0xCF;
		memory[0xFF37] = 0x16;
		memory[0xFF38] = 0x2C;
		memory[0xFF39] = 0x04;
		memory[0xFF3A] = 0xE5;
		memory[0xFF3B] = 0x2C;
		memory[0xFF3C] = 0xAC;
		memory[0xFF3D] = 0xDD;
		memory[0xFF3E] = 0xDA;
		memory[0xFF3F] = 0x48;
	}

	std::memcpy(mem[0xF], mem[0xD], 0xE00); //Make sure the wram-mirror is synced.

	switch (memory[0x0147]) {
	case 0x00: printf("Plain ROM loaded.\n");
		romtype = plain;
		break;
	case 0x01: printf("MBC1 ROM loaded.\n");
		romtype = mbc1;
		break;
	case 0x02: printf("MBC1 ROM+RAM loaded.\n");
		romtype = mbc1;
		break;
	case 0x03: printf("MBC1 ROM+RAM+BATTERY loaded.\n");
		romtype = mbc1;
		battery = 1;
		break;
	case 0x05: printf("MBC2 ROM loaded.\n");
		romtype = mbc2;
		break;
	case 0x06: printf("MBC2 ROM+BATTERY loaded.\n");
		romtype = mbc2;
		battery = 1;
		break;
	case 0x08: printf("Plain ROM with additional RAM loaded.\n");
		break;
	case 0x09: printf("Plain ROM with additional RAM and Battery loaded.\n");
		battery = 1;
		break;
	case 0x0B: /*cout << "MM01 ROM not supported.\n";*/
		return 1;
		break;
	case 0x0C: /*cout << "MM01 ROM not supported.\n";*/
		return 1;
		break;
	case 0x0D: /*cout << "MM01 ROM not supported.\n";*/
		return 1;
		break;
	case 0x0F: printf("MBC3 ROM+TIMER+BATTERY loaded.\n");
		romtype = mbc3;
		battery = true;
		rtcRom = true;
		break;
	case 0x10: printf("MBC3 ROM+TIMER+RAM+BATTERY loaded.\n");
		romtype = mbc3;
		battery = true;
		rtcRom = true;
		break;
	case 0x11: printf("MBC3 ROM loaded.\n");
		romtype = mbc3;
		break;
	case 0x12: printf("MBC3 ROM+RAM loaded.\n");
		romtype = mbc3;
		break;
	case 0x13: printf("MBC3 ROM+RAM+BATTERY loaded.\n");
		romtype = mbc3;
		battery = 1;
		break;
	case 0x15: /*cout << "MBC4 ROM not supported.\n";*/
		return 1;
		break;
	case 0x16: /*cout << "MBC4 ROM not supported.\n";*/
		return 1;
		break;
	case 0x17: /*cout << "MBC4 ROM not supported.\n";*/
		return 1;
		break;
	case 0x19: printf("MBC5 ROM loaded.\n");
		romtype = mbc5;
		break;
	case 0x1A: printf("MBC5 ROM+RAM loaded.\n");
		romtype = mbc5;
		break;
	case 0x1B: printf("MBC5 ROM+RAM+BATTERY loaded.\n");
		romtype = mbc5;
		battery = 1;
		break;
	case 0x1C: printf("MBC5+RUMLE ROM not supported.\n");
		romtype = mbc5;
		break;
	case 0x1D: printf("MBC5+RUMLE+RAM ROM not suported.\n");
		romtype = mbc5;
		break;
	case 0x1E: printf("MBC5+RUMLE+RAM+BATTERY ROM not supported.\n");
		romtype = mbc5;
		battery = 1;
		break;
	case 0xFC: /*cout << "Pocket Camera ROM not supported.\n";*/
		return 1;
		break;
	case 0xFD: /*cout << "Bandai TAMA5 ROM not supported.\n";*/
		return 1;
		break;
	case 0xFE: /*cout << "HuC3 ROM not supported.\n";*/
		return 1;
		break;
	case 0xFF: /*cout << "HuC1 ROM not supported.\n";*/
		return 1;
		break;
	default: /*cout << "Wrong data-format, corrupt or unsupported ROM loaded.\n";*/
		return 1;
	}
	
	/*switch (memory[0x0148]) {
	case 0x00:
		rombanks = 2;
		break;
	case 0x01:
		rombanks = 4;
		break;
	case 0x02:
		rombanks = 8;
		break;
	case 0x03:
		rombanks = 16;
		break;
	case 0x04:
		rombanks = 32;
		break;
	case 0x05:
		rombanks = 64;
		break;
	case 0x06:
		rombanks = 128;
		break;
	case 0x07:
		rombanks = 256;
		break;
	case 0x08:
		rombanks = 512;
		break;
	case 0x52:
		rombanks = 72;
		break;
	case 0x53:
		rombanks = 80;
		break;
	case 0x54:
		rombanks = 96;
		break;
	default:
		return 1;
	}
	
	printf("rombanks: %u\n", rombanks);*/
	
	switch (memory[0x0149]) {
//     case 0x00: /*cout << "No RAM\n";*/ rambankrom=0; break;
	case 0x01: /*cout << "2kB RAM\n";*/ /*rambankrom=1; break;*/
	case 0x02: /*cout << "8kB RAM\n";*/
		rambanks = 1;
		break;
	case 0x03: /*cout << "32kB RAM\n";*/
		rambanks = 4;
		break;
	case 0x04: /*cout << "128kB RAM\n";*/
		rambanks = 16;
		break;
	case 0x05: /*cout << "undocumented kB RAM\n";*/
		rambanks = 16;
		break;
	default: /*cout << "Wrong data-format, corrupt or unsupported ROM loaded.\n";*/
		rambanks = 16;
		break;
	}
	
	printf("rambanks: %u\n", rambanks);
	
	rombanks = rom.size() / 0x4000;
	printf("rombanks: %u\n", rombanks);
	rom.rewind();
	
	delete []romdata;
	romdata = new uint8_t[rombanks * 0x4000];
	rom.read(reinterpret_cast<char*>(romdata), rombanks * 0x4000);
	rom.close();

	for (unsigned i = 0;i < 0x8;++i)
		mem[i] = &romdata[i*0x1000];
	
	rombankptr = mem[0x4] - 0x4000;

	delete []rambankdata;
	rambankdata = new uint8_t[rambanks*0x2000];
	std::memset(rambankdata, 0xFF, rambanks*0x2000);

	{
		char *tmp = strrchr(romfile, '/');
		
		if (tmp == NULL || savedir == NULL)
			tmp = romfile;
		else
			++tmp;
		
		const unsigned int namelen = strrchr(tmp, '.') == NULL ? std::strlen(tmp) : strrchr(tmp, '.') - tmp;
		delete []savename;
		savename = new char[namelen+1];
		strncpy(savename, tmp, namelen);
		savename[namelen] = '\0';
	}

	if (battery) {
		char *savefile;
		
		if (savedir != NULL) {
			savefile = new char[5 + std::strlen(savedir) + std::strlen(savename)];
			sprintf(savefile, "%s%s.sav", savedir, savename);
		} else {
			savefile = new char[5 + std::strlen(savename)];
			sprintf(savefile, "%s.sav", savename);
		}

		std::ifstream load(savefile, std::ios::binary | std::ios::in);
		
		if (load.is_open()) {
			switch (memory[0x0149]) {
			case 0x00:
				size = 0;
				break;
			case 0x01:
				size = 0x800;
				break;
			case 0x02:
				size = 0x2000;
				break;
			case 0x03:
				size = 0x8000;
				break;
			default:
				size = 0x20000;
				break;
			}
			load.read(reinterpret_cast<char*>(rambankdata), size);
			load.close();
			//memcpy(&memory[0xA000], rambankdata[0], 0x2000);
		}
		//else cout << "No savefile available\n";
		delete []savefile;
	}

	if (rtcRom) {
		char *savefile;
		
		if (savedir != NULL) {
			savefile = new char[5 + std::strlen(savedir) + std::strlen(savename)];
			sprintf(savefile, "%s%s.rtc", savedir, savename);
		} else {
			savefile = new char[5 + std::strlen(savename)];
			sprintf(savefile, "%s.rtc", savename);
		}

		std::ifstream load(savefile, std::ios::binary | std::ios::in);
		
		std::time_t basetime;
		
		if (load.is_open()) {
			load.read(reinterpret_cast<char*>(&basetime), sizeof(basetime));
			load.close();
		} else
			basetime = time(NULL);
		
		delete []savefile;
		
		rtc.setBaseTime(basetime);
	}

	sound.init(memory + 0xFF00, isCgb());
	display.reset(isCgb());
	refreshPalettes(0x102A0);
	next_blittime = (memory[0xFF40] & 0x80) ? display.nextMode1IrqTime() : uint32_t(-1);

	return 0;
}

void Memory::set_savedir(const char *dir) {
	delete []savedir;
	savedir = NULL;

	if (dir != NULL) {
		savedir = new char[std::strlen(dir) + 2];
		std::strcpy(savedir, dir);
		
		if (savedir[std::strlen(dir) - 1] != '/') {
			savedir[std::strlen(dir)] = '/';
			savedir[std::strlen(dir) + 1] = '\0';
		}
	}
}

void Memory::saveram() {
	char *savefile;
	
	if (savedir != NULL) {
		savefile = new char[5 + std::strlen(savedir) + std::strlen(savename)];
		sprintf(savefile, "%s%s.sav", savedir, savename);
	} else {
		savefile = new char[5 + std::strlen(savename)];
		sprintf(savefile, "%s.sav", savename);
	}

	std::ofstream save(savefile, std::ios::binary | std::ios::out);
	
	if (save.is_open()) {
		unsigned size/*=0x2000*/;
		
		switch (memory[0x0149]) {
		case 0x00:
			size = 0;
			break;
		case 0x01:
			size = 0x800;
			break;
		case 0x02:
			size = 0x2000;
			break;
		case 0x03:
			size = 0x8000;
			break;
		default:
			size = 0x20000;
			break;
		}
		
		save.write(reinterpret_cast<char*>(rambankdata), size);
		save.close();
	}  /*else cout << "Saving ramdata failed\n";*/
	
	delete []savefile;
}

void Memory::save_rtc() {
	char *savefile;
	
	if (savedir != NULL) {
		savefile = new char[5 + std::strlen(savedir) + std::strlen(savename)];
		sprintf(savefile, "%s%s.rtc", savedir, savename);
	} else {
		savefile = new char[5 + std::strlen(savename)];
		sprintf(savefile, "%s.rtc", savename);
	}

	std::ofstream save(savefile, std::ios::binary | std::ios::out);
	
	if (save.is_open()) {
		std::time_t basetime = rtc.getBaseTime();
		save.write(reinterpret_cast<char*>(&basetime), sizeof(basetime));
		save.close();
	}  /*else cout << "Saving rtcdata failed\n";*/
	
	delete []savefile;
}

void Memory::sound_fill_buffer(uint16_t *const stream, const unsigned samples, const unsigned cycleCounter) {
	sound.generate_samples(cycleCounter, isDoubleSpeed());
	sound.fill_buffer(stream, samples);
}

void Memory::setVideoBlitter(VideoBlitter *const vb, const unsigned cycleCounter) {
	display.setVideoBlitter(vb);
	refreshPalettes(cycleCounter);
}

void Memory::videoBufferChange(const unsigned cycleCounter) {
	display.videoBufferChange();
	refreshPalettes(cycleCounter);
}

void Memory::setVideoFilter(const unsigned int n, const unsigned cycleCounter) {
	display.setVideoFilter(n);
	refreshPalettes(cycleCounter);
}

void Memory::setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32, unsigned cycleCounter) {
	display.setDmgPaletteColor(palNum, colorNum, rgb32);
	refreshPalettes(cycleCounter);
}

Memory::~Memory() {
	if (battery)
		saveram();
	
	if (rtcRom)
		save_rtc();
	
	if (romfile != NULL) {
		delete []romfile;
		romfile = NULL;
	}
	
	delete []savedir;
	delete []savename;
	delete []romdata;
	delete []rambankdata;
	delete[]cgb_wramdata;
}
