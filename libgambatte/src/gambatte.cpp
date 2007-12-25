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
#include "gambatte.h"

#include "cpu.h"

namespace Gambatte {
GB::GB() : z80(new CPU) {}

GB::~GB() {
	delete z80;
}

void GB::runFor(const unsigned long cycles) {
	z80->runFor(cycles);
}

void GB::reset() {
	z80->reset();
}

void GB::setVideoBlitter(VideoBlitter *vb) {
	z80->setVideoBlitter(vb);
}

void GB::videoBufferChange() {
	z80->videoBufferChange();
}

unsigned GB::videoWidth() const {
	return z80->videoWidth();
}

unsigned GB::videoHeight() const {
	return z80->videoHeight();
}

void GB::setVideoFilter(const unsigned n) {
	z80->setVideoFilter(n);
}

std::vector<const FilterInfo*> GB::filterInfo() const {
	return z80->filterInfo();
}

void GB::setInputStateGetter(InputStateGetter *getInput) {
	z80->setInputStateGetter(getInput);
}

void GB::set_savedir(const char *sdir) {
	z80->set_savedir(sdir);
}

bool GB::load(const char* romfile) {
	return z80->load(romfile);
}

void GB::fill_buffer(uint_least16_t *const stream, const unsigned samples) {
	z80->sound_fill_buffer(stream, samples);
}

bool GB::isCgb() const {
	return z80->isCgb();
}

void GB::setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) {
	z80->setDmgPaletteColor(palNum, colorNum, rgb32);
}
}
