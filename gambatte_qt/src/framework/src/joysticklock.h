//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef JOYSTICKLOCK_H
#define JOYSTICKLOCK_H

#include "SDL_Joystick/include/SDL_event.h"
#include "SDL_Joystick/include/SDL_joystick.h"
#include "uncopyable.h"
#include <QMutex>

// SDL Joystick uses global state. This is a helper to make it easier
// to verify thread-safe operation.
class SdlJoystick {
public:
	enum { axis_centered = 0, axis_positive = 1, axis_negative = 2 };
	class TryLocked;

	class Locked : Uncopyable {
	public:
		Locked() { mutex_.lock(); }

		explicit Locked(TryLocked &tl) {
			if (!tl)
				mutex_.lock();

			tl.releaseTo(*this);
		}

		~Locked() { mutex_.unlock(); }

		// Wraps SDL_PollEvent, converting axis event values to
		// axis_centered, axis_positive, or axis_negative.
		int pollEvent(SDL_Event * , int insensitivity = 0);

		void update() { SDL_JoystickUpdate(); }
		void clearEvents() { SDL_ClearEvents(); }
	};

	class TryLocked : Uncopyable {
	public:
		TryLocked() : m_(mutex_.tryLock() ? &mutex_ : 0) {}
		~TryLocked() { if (m_) m_->unlock(); }
		operator bool() const { return m_; }
		void releaseTo(Locked &) { m_ = 0; }

	private:
		QMutex *m_;
	};

private:
	SdlJoystick();
	static QMutex mutex_;
};

#endif
