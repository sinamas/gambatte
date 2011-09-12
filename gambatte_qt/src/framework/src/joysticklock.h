/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#ifndef JOYSTICKLOCK_H
#define JOYSTICKLOCK_H

#include <QMutex>
#include "SDL_Joystick/include/SDL_event.h"

enum { AXIS_CENTERED = 0, AXIS_POSITIVE = 1, AXIS_NEGATIVE = 2 };

// Wraps SDL_PollEvent, converting all values to hat-style bitset values
// (a single bit for buttons, two for axes, four for hats).
// Only hats can have multiple bits set at once. In practice only axis values
// are converted (to AXIS_CENTERED, AXIS_POSITIVE or AXIS_NEGATIVE).
int pollJsEvent(SDL_Event *ev, int insensitivity = 0);

class JoystickLock {
	static QMutex mut;
public:
	static void lock() { mut.lock(); }
	static bool tryLock() { return mut.tryLock(); }
	static bool tryLock(int timeout) { return mut.tryLock(timeout); }
	static void unlock() { mut.unlock(); }
};

#endif
