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

class EnvelopeUnit : public SoundUnit {
public:
	struct VolOnOffEvent {
		virtual ~VolOnOffEvent() {}
		virtual void operator()(unsigned long /*cc*/) {}
	};

	explicit EnvelopeUnit(VolOnOffEvent &volOnOffEvent = nullEvent_);
	void event();
	bool dacIsOn() const { return nr2_ & 0xF8; }
	unsigned getVolume() const { return volume_; }
	bool nr2Change(unsigned newNr2);
	bool nr4Init(unsigned long cycleCounter);
	void reset();
	void saveState(SaveState::SPU::Env &estate) const;
	void loadState(SaveState::SPU::Env const &estate, unsigned nr2, unsigned long cc);

private:
	static VolOnOffEvent nullEvent_;
	VolOnOffEvent &volOnOffEvent_;
	unsigned char nr2_;
	unsigned char volume_;
};

}
