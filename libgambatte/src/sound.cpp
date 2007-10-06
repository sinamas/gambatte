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
#include "sound.h"

#include <cstring>
#include <algorithm>

/*
	Frame Sequencer

	Step    Length Ctr  Vol Env     Sweep
	- - - - - - - - - - - - - - - - - - - -
	0       Clock       -           Clock
S	1       -           Clock       -
	2       Clock       -           -
	3       -           -           -
	4       Clock       -           Clock
	5       -           -           -
	6       Clock       -           -
	7       -           -           -
	- - - - - - - - - - - - - - - - - - - -
	Rate    256 Hz      64 Hz       128 Hz

S) start step on sound power on.
*/

static const unsigned bufferSize = 35112 + 16 + 2048; //FIXME: DMA can prevent process from returning for up to 4096 cycles.

void PSG::init(const uint8_t *const ioram, const bool cgb) {
	bufferPos = 0;
	lastUpdate = 0x102A0;
	enabled = true;
	map_so(ioram[0x25]);
	set_so_volume(ioram[0x24]);
	ch1.init(lastUpdate >> 1, cgb);
	ch2.init(lastUpdate >> 1, cgb);
	ch3.init(lastUpdate >> 1, cgb);
	for (unsigned i = 0; i < 0x10; ++i)
		ch3.waveRamWrite(i, ioram[0x30 | i]);
	ch4.init(lastUpdate >> 1, cgb);
}

void PSG::reset() {
	ch1.reset();
	ch2.reset();
	ch3.reset();
	ch4.reset();
}

PSG::PSG() {
	buffer = new uint32_t[bufferSize];
}

PSG::~PSG() {
	delete[] buffer;
}

void PSG::accumulate_channels(const unsigned next_update) {
	uint32_t *const buf = buffer + bufferPos;

	const unsigned soVol = (so1_volume << 16) | so2_volume;
	
	ch1.update(buf, soVol, next_update);
	ch2.update(buf, soVol, next_update);
	ch3.update(buf, soVol, next_update);
	ch4.update(buf, soVol, next_update);
}

void PSG::generate_samples(const unsigned cycleCounter, const unsigned doubleSpeed) {
	const unsigned cycles = cycleCounter - lastUpdate >> (1 + doubleSpeed);
	lastUpdate += cycles << (1 + doubleSpeed);

	if (bufferSize - bufferPos < cycles)
		bufferPos = bufferSize - cycles;

	if (cycles)
		accumulate_channels(cycles);
	
	bufferPos += cycles;
}

void PSG::resetCounter(const unsigned newCc, const unsigned oldCc, const unsigned doubleSpeed) {
	generate_samples(oldCc, doubleSpeed);
	lastUpdate = newCc - (oldCc - lastUpdate);
}

void PSG::fill_buffer(uint16_t *stream, const unsigned samples) {
	const unsigned endPos = std::min(bufferPos, 35112U);
	
	if (stream && samples && endPos >= samples) {
		uint16_t *const streamEnd = stream + samples * 2;
		const uint32_t *buf = buffer;
	
		const unsigned ratio = (endPos << 16) / samples;
		
		unsigned whole = ratio >> 16;
		unsigned frac = ratio & 0xFFFF;
		unsigned so1 = 0;
		unsigned so2 = 0;
		
		while (stream < streamEnd) {
			{
				unsigned soTmp = 0;
				
				for (const uint32_t *const end = buf + whole; buf != end;)
					soTmp += *buf++;
				
				so1 += soTmp & 0xFFFF0000;
				so2 += soTmp << 16 & 0xFFFFFFFF;
			}
			
			{
				const unsigned borderSample = *buf++;
				const unsigned so1Tmp = (borderSample >> 16) * frac;
				const unsigned so2Tmp = (borderSample & 0xFFFF) * frac;
				
				so1 += so1Tmp;
				so2 += so2Tmp;
				
				*stream++ = (so2 / 4389) * samples + 8192 >> 14;
				*stream++ = (so1 / 4389) * samples + 8192 >> 14;
				
				so1 = (borderSample & 0xFFFF0000) - so1Tmp;
				so2 = (borderSample << 16 & 0xFFFFFFFF) - so2Tmp;
			}
			
			const unsigned nextTotal = ratio - ((1 << 16) - frac);
			whole = nextTotal >> 16;
			frac = nextTotal & 0xFFFF;
		}
	}

	bufferPos -= endPos;
	std::memmove(buffer, buffer + endPos, bufferPos * sizeof(uint32_t));
}

void PSG::set_so_volume(const unsigned nr50) {
// 	printf("nr50 = 0x%X\n", nr50);
	so1_volume = (nr50 & 0x7) + 1;
	so2_volume = (nr50 >> 4 & 0x7) + 1;
}

void PSG::map_so(const unsigned nr51) {
	ch1.setSo(nr51 & 0x01, (nr51 & 0x10) != 0);
	ch2.setSo((nr51 & 0x02) != 0, (nr51 & 0x20) != 0);
	ch3.setSo((nr51 & 0x04) != 0, (nr51 & 0x40) != 0);
	ch4.setSo((nr51 & 0x08) != 0, (nr51 & 0x80) != 0);
}

unsigned PSG::getStatus() const {
	return ch1.isActive() | (ch2.isActive() << 1) | (ch3.isActive() << 2) | (ch4.isActive() << 3);
}
