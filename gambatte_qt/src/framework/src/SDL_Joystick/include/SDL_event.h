/*
    Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef SDL_EVENT_H
#define SDL_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	union {
		struct {
			uint8_t type;
			uint8_t dev_num;
			uint8_t num;
			uint8_t zero;
		};

		uint32_t id;
	};

	union {
		struct {
			int16_t xrel;
			int16_t yrel;
		};

		int32_t value;
	};
} SDL_Event;

enum { SDL_JOYAXISMOTION = 0x01, SDL_JOYHATMOTION = 0x02, SDL_JOYBALLMOTION = 0x04, SDL_JOYBUTTONCHANGE = 0x08 };

extern void SDL_ResetEvents(void);

extern void SDL_PushEvent(SDL_Event *e);

extern void SDL_ClearEvents(void);

extern void SDL_SetEventFilter(unsigned int f);

extern int SDL_PollEvent(SDL_Event *event);

#ifdef __cplusplus
}
#endif

#endif
