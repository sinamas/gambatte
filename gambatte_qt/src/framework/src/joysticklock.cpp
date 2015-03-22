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

#include "joysticklock.h"
#include <map>

static bool acceptEvent(SDL_Event &ev, int const insensitivity) {
	if (ev.type != SDL_JOYAXISMOTION)
		return true;

	enum { threshold = 8192 };
	typedef std::map<unsigned, int> Map;
	static Map axisState;
	Map::iterator const at =
		axisState.insert(Map::value_type(ev.id, SdlJoystick::axis_centered)).first;
	switch (at->second) {
	case SdlJoystick::axis_centered:
		if (ev.value >= threshold + insensitivity)
			ev.value = SdlJoystick::axis_positive;
		else if (ev.value <= -(threshold + insensitivity))
			ev.value = SdlJoystick::axis_negative;
		else
			return false;

		break;
	case SdlJoystick::axis_positive:
		if (ev.value >= threshold - insensitivity)
			return false;

		ev.value = ev.value <= -(threshold + insensitivity)
		         ? SdlJoystick::axis_negative
		         : SdlJoystick::axis_centered;
		break;
	case SdlJoystick::axis_negative:
		if (ev.value <= -(threshold - insensitivity))
			return false;

		ev.value = ev.value >= threshold + insensitivity
		         ? SdlJoystick::axis_positive
		         : SdlJoystick::axis_centered;
		break;
	}

	at->second = ev.value;
	return true;
}

int SdlJoystick::Locked::pollEvent(SDL_Event *ev, int insensitivity) {
	int evValid;

	do {
		evValid = SDL_PollEvent(ev);
	} while (evValid && !acceptEvent(*ev, insensitivity));

	return evValid;
}

QMutex SdlJoystick::mutex_;
