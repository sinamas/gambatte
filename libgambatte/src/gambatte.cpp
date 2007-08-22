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
#include "gambatte.h"

#include "cpu.h"

Gambatte::Gambatte() : z80(new CPU) {}

Gambatte::~Gambatte() {
	delete z80;
}

void Gambatte::runFor(const unsigned int cycles) {
	z80->runFor(cycles);
}

void Gambatte::reset() {
	z80->reset();
}

void Gambatte::setVideoBlitter(VideoBlitter *vb) {
	z80->setVideoBlitter(vb);
}

void Gambatte::videoBufferChange() {
	z80->videoBufferChange();
}

unsigned int Gambatte::videoWidth() const {
	return z80->videoWidth();
}

unsigned int Gambatte::videoHeight() const {
	return z80->videoHeight();
}

void Gambatte::setVideoFilter(const unsigned int n) {
	z80->setVideoFilter(n);
}

std::vector<const FilterInfo*> Gambatte::filterInfo() const {
	return z80->filterInfo();
}

void Gambatte::setInputStateGetter(InputStateGetter *getInput) {
	z80->setInputStateGetter(getInput);
}

void Gambatte::set_savedir(const char *sdir) {
	z80->set_savedir(sdir);
}

bool Gambatte::load(const char* romfile) {
	return z80->load(romfile);
}

void Gambatte::fill_buffer(uint16_t *const stream, const unsigned samples) {
	z80->sound_fill_buffer(stream, samples);
}
