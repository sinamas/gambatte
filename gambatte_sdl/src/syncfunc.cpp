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
#include "syncfunc.h"

#include <SDL.h>

namespace SyncFunc {
struct timeval {
	long tv_sec;
	long tv_usec;
};

void gettimeofday(timeval *const t, void *) {
	const unsigned long tv_msec = SDL_GetTicks();
	
	t->tv_sec = tv_msec / 1000;
	t->tv_usec = (tv_msec % 1000) * 1000;
}
}

void syncFunc() {
	using SyncFunc::timeval;
	using SyncFunc::gettimeofday;

	timeval t;
	gettimeofday(&t, NULL);
	static timeval time = t;
	static long late = 0;
	static unsigned noSleep = 60;
	
	if (time.tv_sec > t.tv_sec || (time.tv_sec == t.tv_sec && time.tv_usec > t.tv_usec)) {
		timeval tmp;
		tmp.tv_sec = 0;
		tmp.tv_usec = time.tv_usec - t.tv_usec;
		if (time.tv_sec != t.tv_sec)
			tmp.tv_usec += 1000000;
		
		if (tmp.tv_usec > late) {
			tmp.tv_usec -= late;
			
			if (tmp.tv_usec >= 1000000) {
				tmp.tv_usec -= 1000000;
				++tmp.tv_sec;
			}
			
			SDL_Delay(tmp.tv_sec * 1000 + (tmp.tv_usec + 500) / 1000);
			
			gettimeofday(&t, NULL);
			late -= (time.tv_sec - t.tv_sec) * 1000000 + time.tv_usec - t.tv_usec >> 1;
// 			printf("late: %d\n", late);
			
			if (late < 0)
				late = 0;
			
			noSleep = 60;
		} else if (noSleep-- == 0) {
			noSleep = 60;
			late = 0;
		}
		
		while (time.tv_sec > t.tv_sec || (time.tv_sec == t.tv_sec && time.tv_usec > t.tv_usec)) {
			gettimeofday(&t, NULL);
		}
		
	} else
		time = t;
	
	time.tv_usec += 16743;
	
	if (time.tv_usec >= 1000000) {
		time.tv_usec -= 1000000;
		++time.tv_sec;
	}
}
