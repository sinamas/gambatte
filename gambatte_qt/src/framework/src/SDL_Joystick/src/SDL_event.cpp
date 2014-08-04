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

#include "SDL_event.h"

#include <deque>

static std::deque<SDL_Event> queue;
static unsigned int filter = 0;

void SDL_ResetEvents() {
	filter = 0;
	queue.clear();
}

void SDL_PushEvent(SDL_Event *const e) {
	if (filter & e->type) {
		e->zero = 0;
		queue.push_back(*e);
	}
}

void SDL_ClearEvents() {
	queue.clear();
}

void SDL_SetEventFilter(const unsigned int f) {
	filter = f;
}

int SDL_PollEvent(SDL_Event *const event) {
	if (!event || queue.empty())
		return 0;

	{
		const SDL_Event &front = queue.front();
		event->id = front.id;
		event->value = front.value;
	}

	queue.pop_front();

	return 1;
}
