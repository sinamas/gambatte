//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#pragma once

#include "sound_unit.h"
#include "../savestate.h"

namespace gambatte {

class MasterDisabler;

class LengthCounter : public SoundUnit {
public:
	LengthCounter(MasterDisabler &disabler, unsigned lengthMask);
	virtual void event();
	void nr1Change(unsigned newNr1, unsigned nr4, unsigned long cc);
	void nr4Change(unsigned oldNr4, unsigned newNr4, unsigned long cc);
	void saveState(SaveState::SPU::LCounter &lstate) const;
	void loadState(SaveState::SPU::LCounter const &lstate, unsigned long cc);

private:
	MasterDisabler &disableMaster_;
	unsigned short lengthCounter_;
	unsigned char const lengthMask_;
};

}
