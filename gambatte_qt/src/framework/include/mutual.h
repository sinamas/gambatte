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

#ifndef MUTUAL_H
#define MUTUAL_H

#include "uncopyable.h"
#include <QMutex>

template<class T>
class Mutual {
public:
	Mutual() {}
	explicit Mutual(T const &t) : t(t) {}

	class Locked : Uncopyable {
	public:
		Locked(Mutual &lc) : lc(lc) { lc.mut.lock(); }
		~Locked() { lc.mut.unlock(); }
		T * operator->() { return &lc.t; }
		T const * operator->() const { return &lc.t; }
		T & get() { return lc.t; }
		T const & get() const { return lc.t; }
	private:
		Mutual &lc;
	};

	class ConstLocked : Uncopyable {
	public:
		ConstLocked(Mutual const &lc) : lc(lc) { lc.mut.lock(); }
		~ConstLocked() { lc.mut.unlock(); }
		T const * operator->() const { return &lc.t; }
		T const & get() const { return lc.t; }
	private:
		Mutual const &lc;
	};

	class TryLocked : Uncopyable {
	public:
		TryLocked(Mutual &lc) : lc(lc.mut.tryLock() ? &lc : 0) {}
		~TryLocked() { if (lc) lc->mut.unlock(); }
		T * operator->() const { return &lc->t; }
		T * get() const { return lc ? &lc->t : 0; }
		operator bool() const { return lc; }
	private:
		Mutual *const lc;
	};

private:
	mutable QMutex mut;
	T t;
};

#endif
