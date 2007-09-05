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

static const unsigned bufferSize = 35112 + 16 + 2048; //FIXME: DMA can prevent process from returning for up to 4096 cycles.

void PSG::init(const uint8_t *const ioram) {
	std::memset(buffer, 0, bufferSize*sizeof(uint32_t));
	bufferPos = 0;
	snd_lastUpdate = 0x102A0;
	enabled = true;
	reset(ioram);
}

void PSG::reset(const uint8_t *const ioram) {
	map_so(ioram[0x25]);
	set_so_volume(ioram[0x24]);
	ch1.reset(ioram[0x10], ioram[0x11], ioram[0x12], ioram[0x13], ioram[0x14]);
	ch2.reset(ioram[0x16], ioram[0x17], ioram[0x18], ioram[0x19]);
	ch3.reset(ioram[0x1A], ioram[0x1B], ioram[0x1C], ioram[0x1D], ioram[0x1E]);
	for (unsigned i = 0; i < 0x10; ++i)
		ch3.waveRamWrite(i, ioram[0x30 | i]);
	ch4.reset(ioram[0x20], ioram[0x21], ioram[0x22], ioram[0x23]);
// 	sound_master = (ioram[0x26] & 0x80) != 0;
}

PSG::PSG() {
	buffer = new uint32_t[bufferSize];
// 	snd_lastUpdate = 0;
// 	init();
}

PSG::~PSG() {
	delete[] buffer;
}

/*void PSG::all_off() {
	ch1.deactivate();
	ch2.deactivate();
	ch3.deactivate();
	ch4.deactivate();
}*/

void PSG::accumulate_channels(const uint32_t next_update) {
	uint32_t *const buf = buffer + bufferPos;

	const unsigned soVol = (so1_volume << 16) | so2_volume;
	
	ch1.update(buf, soVol, next_update);
	ch2.update(buf, soVol, next_update);
	ch3.update(buf, soVol, next_update);
	ch4.update(buf, soVol, next_update);
}

void PSG::generate_samples(const unsigned cycleCounter, const unsigned doubleSpeed) {
	const uint32_t cycles = (cycleCounter - snd_lastUpdate) >> (1 + doubleSpeed);
	snd_lastUpdate += cycles << (1 + doubleSpeed);

	const uint_fast32_t c = std::min(cycles, bufferSize - bufferPos);
// 	if (c < cycles)
// 		printf("oh fuck! %u\n", cycles - c);

	if (enabled && c)
		accumulate_channels(c);
	bufferPos += c;
}

void PSG::resetCounter(const unsigned newCc, const unsigned oldCc, const unsigned doubleSpeed) {
	generate_samples(oldCc, doubleSpeed);
	snd_lastUpdate = newCc - (oldCc - snd_lastUpdate);
}

void PSG::fill_buffer(uint16_t *stream, const unsigned samples) {
// 	printf("bufPos: %u\n", bufferPos);
	const unsigned endPos = std::min(bufferPos, 35112U);
	
	if (stream && samples) {
// 	generate_samples();

	uint16_t *const streamEnd = stream + samples * 2;
	const uint32_t *buf = buffer;

	const unsigned ratio = (endPos << 16) / samples;
	
	if (ratio) {
		unsigned whole = ratio >> 16;
		unsigned frac = ratio & 0xFFFF;
		unsigned so1 = 0;
		unsigned so2 = 0;
		
		while (stream < streamEnd) {
			{
				unsigned soTmp = 0;
				
				for (unsigned n = whole; n--;)
					soTmp += *buf++;
					
				so1 += soTmp & 0xFFFF0000;
				so2 += (soTmp << 16) & 0xFFFFFFFF;
			}

			{
				const unsigned borderSample = *buf++;
				const unsigned so1Tmp = (borderSample >> 16) * frac;
				const unsigned so2Tmp = (borderSample & 0xFFFF) * frac;
				
				so1 += so1Tmp;
				so2 += so2Tmp;
	
				*stream++ = ((so2 + ratio / 2) / ratio) << 5;
				*stream++ = ((so1 + ratio / 2) / ratio) << 5;
				
				so1 = (borderSample & 0xFFFF0000) - so1Tmp;
				so2 = ((borderSample << 16) & 0xFFFFFFFF) - so2Tmp;
			}

			const unsigned nextTotal = ratio - ((1 << 16) - frac);
			whole = nextTotal >> 16;
			frac = nextTotal & 0xFFFF;
		}
	}
	}

	bufferPos -= endPos;
	std::memmove(buffer, buffer + endPos, bufferPos * sizeof(uint32_t));
	std::memset(buffer + bufferPos, 0, (bufferSize - bufferPos) * sizeof(uint32_t));
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

void PSG::clear_buffer() {
	std::memset(buffer, 0, bufferSize*sizeof(uint32_t));
}
