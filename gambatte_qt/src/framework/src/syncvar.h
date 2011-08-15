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
#ifndef SYNC_VAR_H
#define SYNC_VAR_H

#include <QMutex>
#include <QWaitCondition>

class SyncVar {
	QMutex mut;
	QWaitCondition cond;
	unsigned var;

public:
	class Locked : Uncopyable {
		SyncVar &sv;

	public:
		Locked(SyncVar &sv) : sv(sv) { sv.mut.lock(); }
		~Locked() { sv.mut.unlock(); }
		unsigned get() const { return sv.var; }
		void set(const unsigned var) { sv.var = var; sv.cond.wakeAll(); }
		bool wait(const unsigned long time = ULONG_MAX) { return sv.cond.wait(&sv.mut, time); }
// 		bool waitMaskedEqual(unsigned state, unsigned mask, unsigned long time = ULONG_MAX);
// 		bool waitMaskedNequal(unsigned state, unsigned mask, unsigned long time = ULONG_MAX);
// 		bool waitEqual(unsigned state, unsigned long time = ULONG_MAX) { return waitMaskedEqual(state, UINT_MAX, time); }
// 		bool waitNequal(unsigned state, unsigned long time = ULONG_MAX) { return waitMaskedNequal(state, UINT_MAX, time); }
// 		bool waitAnd(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedEqual(bits, bits, time); }
// 		bool waitNand(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedNequal(bits, bits, time); }
// 		bool waitNor(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedEqual(0, bits, time); }
// 		bool waitOr(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedNequal(0, bits, time); }
	};

	explicit SyncVar(const unsigned var = 0) : var(var) {}
};

/*bool SyncVar::Locked::waitMaskedEqual(const unsigned state, const unsigned mask, const unsigned long time) {
	while ((get() & mask) != state) {
		wait(time);

		if (time != ULONG_MAX)
			return (get() & mask) == state;
	}

	return true;
}

bool SyncVar::Locked::waitMaskedNequal(const unsigned state, const unsigned mask, const unsigned long time) {
	while ((get() & mask) == state) {
		wait(time);

		if (time != ULONG_MAX)
			return (get() & mask) != state;
	}

	return true;
}*/

#endif
