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

QMutex JoystickLock::mut(QMutex::Recursive);

int pollJsEvent(SDL_Event *ev) {
	typedef std::map<unsigned, int> map_t;
	
	static map_t axisState;
	
	int returnVal;
	
	do {
		returnVal = SDL_PollEvent(ev);
		
		if (returnVal && ev->type == SDL_JOYAXISMOTION) {
			if (ev->value > 8192)
				ev->value = AXIS_POSITIVE;
			else if (ev->value < -8192)
				ev->value = AXIS_NEGATIVE;
			else
				ev->value = 0;
			
			const std::pair<map_t::iterator, bool> &axisInsert = axisState.insert(map_t::value_type(ev->id, ev->value));
			
			if (!axisInsert.second && axisInsert.first->second == ev->value)
				continue;
			
			axisInsert.first->second = ev->value;
		}
	} while (false);
	
	return returnVal;
}
