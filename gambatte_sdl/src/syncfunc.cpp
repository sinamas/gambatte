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
#include "syncfunc.h"
#include <adaptivesleep.h>
#include <SDL.h>

usec_t getusecs() {
	return SDL_GetTicks() * usec_t(1000);
}

void usecsleep(const usec_t usecs) {
	SDL_Delay((usecs + 999) / 1000);
}

void syncfunc(const long inc) {
	static AdaptiveSleep asleep;
	static usec_t last = getusecs();
	
	last += asleep.sleepUntil(last, inc);
	last += inc;
}
