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
#include "joysticklock.h"
#include "SDL_Joystick/include/SDL_joystick.h"
#include <map>

QMutex JoystickLock::mut;

int pollJsEvent(SDL_Event *const ev, const int insensitivity) {
	typedef std::map<unsigned, int> map_t;
	static map_t axisState;

	int evValid;
	
	do {
		evValid = SDL_PollEvent(ev);
		
		if (evValid && ev->type == SDL_JOYAXISMOTION) {
			enum { THRESHOLD = 8192 };
			const map_t::iterator it = axisState.insert(map_t::value_type(ev->id, 0)).first;
			
			switch (it->second) {
			case 0:
				if (ev->value >= THRESHOLD + insensitivity)
					ev->value = AXIS_POSITIVE;
				else if (ev->value <= -(THRESHOLD + insensitivity))
					ev->value = AXIS_NEGATIVE;
				else
					continue;

				break;
			case AXIS_POSITIVE:
				if (ev->value >= THRESHOLD - insensitivity)
					continue;

				ev->value = ev->value <= -(THRESHOLD + insensitivity) ? AXIS_NEGATIVE : 0;
				break;
			case AXIS_NEGATIVE:
				if (ev->value <= -(THRESHOLD - insensitivity))
					continue;

				ev->value = ev->value >= THRESHOLD + insensitivity ? AXIS_POSITIVE : 0;
				break;
			}

			it->second = ev->value;
		}
	} while (false);
	
	return evValid;
}
