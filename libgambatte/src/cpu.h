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
#ifndef CPU_H
#define CPU_H

#include <stdint.h>

#include "memory.h"

class CPU {
	Memory memory;
	
	uint32_t cycleCounter_;

	uint16_t PC_;
	uint16_t SP;

	uint8_t A_, B, C, D, E, F, H, L;

	uint32_t HF1, HF2, ZF, CF;

	bool skip;
	bool halted;
	
	void process(unsigned cycles);
	
public:
	
	CPU();
	void init();
// 	void halt();

// 	unsigned interrupt(unsigned address, unsigned cycleCounter);
	
	void runFor(unsigned int cycles);
	void reset();
	
	void setVideoBlitter(VideoBlitter *vb) {
		memory.setVideoBlitter(vb, cycleCounter_);
	}
	
	void videoBufferChange() {
		memory.videoBufferChange(cycleCounter_);
	}
	
	unsigned int videoWidth() const {
		return memory.videoWidth();
	}
	
	unsigned int videoHeight() const {
		return memory.videoHeight();
	}
	
	void setVideoFilter(const unsigned int n) {
		memory.setVideoFilter(n, cycleCounter_);
	}
	
	std::vector<const FilterInfo*> filterInfo() const {
		return memory.filterInfo();
	}
	
	void setInputStateGetter(InputStateGetter *getInput) {
		memory.setInputStateGetter(getInput);
	}
	
	void set_savedir(const char *sdir) {
		memory.set_savedir(sdir);
	}
	
	bool load(const char* romfile);
	
	void sound_fill_buffer(uint16_t *const stream, const unsigned samples) {
		memory.sound_fill_buffer(stream, samples, cycleCounter_);
	}
};

#endif
