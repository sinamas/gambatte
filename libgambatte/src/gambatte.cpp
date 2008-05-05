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
#include "savestate.h"
#include "statesaver.h"
#include "initstate.h"
#include <string>
#include <sstream>
#include <fstream>

class SaveStateOsdElement : public OsdElement {
	Gambatte::uint_least32_t pixels[StateSaver::SS_WIDTH * StateSaver::SS_HEIGHT];
	unsigned life;
	
public:
	SaveStateOsdElement(const char *fileName, unsigned stateNo);
	const Gambatte::uint_least32_t* update();
};

SaveStateOsdElement::SaveStateOsdElement(const char *fileName, unsigned stateNo) : OsdElement(stateNo * ((160 - StateSaver::SS_WIDTH) / 10) + ((160 - StateSaver::SS_WIDTH) / 10) / 2, 4, StateSaver::SS_WIDTH, StateSaver::SS_HEIGHT), life(4 * 60) {
	std::memset(pixels, 0, sizeof(pixels));
	
	std::ifstream file(fileName);
	
	if (file.is_open() && file.get() == 0) {
		file.ignore(4);
		file.read(reinterpret_cast<char*>(pixels), sizeof(pixels));
	}
}

const Gambatte::uint_least32_t* SaveStateOsdElement::update() {
	if (life--)
		return pixels;
	
	return 0;
}

static const std::string itos(int i) {
	std::stringstream ss;
	
	ss << i;
	
	std::string out;
	
	ss >> out;
	
	return out;
}

static const std::string statePath(const std::string &basePath, int stateNo) {
	return basePath + "_" + itos(stateNo) + ".gqs";
}

namespace Gambatte {
GB::GB() : z80(new CPU), stateNo(0) {}

GB::~GB() {
	delete z80;
}

void GB::runFor(const unsigned long cycles) {
	z80->runFor(cycles);
}

void GB::reset() {
	z80->saveSavedata();
	
	SaveState state;
	z80->setStatePtrs(state);
	setInitState(state, z80->isCgb());
	z80->loadState(state);
	z80->loadSavedata();
	
// 	z80->reset();
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
	z80->saveSavedata();
	
	const bool failed = z80->load(romfile);
	
	if (!failed) {
		SaveState state;
		z80->setStatePtrs(state);
		setInitState(state, z80->isCgb());
		z80->loadState(state);
		z80->loadSavedata();
	}
	
	return failed;
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

void GB::saveState() {
	SaveState state;
	z80->setStatePtrs(state);
	z80->saveState(state);
	StateSaver::saveState(state, statePath(z80->saveBasePath(), stateNo).c_str());
}

void GB::loadState() {
	z80->saveSavedata();
	
	SaveState state;
	z80->setStatePtrs(state);
	
	if (StateSaver::loadState(state, statePath(z80->saveBasePath(), stateNo).c_str()))
		z80->loadState(state);
}

void GB::selectState(int n) {
	n -= (n / 10) * 10;
	stateNo = n < 0 ? n + 10 : n;
	z80->setOsdElement(std::auto_ptr<OsdElement>(new SaveStateOsdElement(statePath(z80->saveBasePath(), stateNo).c_str(), stateNo)));
}
}
