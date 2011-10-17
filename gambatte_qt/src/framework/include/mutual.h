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
#ifndef MUTUAL_H
#define MUTUAL_H

#include "uncopyable.h"
#include <QMutex>

template<class T>
class Mutual {
	mutable QMutex mut;
	T t;

public:
	Mutual() {}
	explicit Mutual(const T &t) : t(t) {}

	class Locked : Uncopyable {
		Mutual &lc;
	public:
		Locked(Mutual &lc) : lc(lc) { lc.mut.lock(); }
		~Locked() { lc.mut.unlock(); }
		T* operator->() { return &lc.t; }
		const T* operator->() const { return &lc.t; }
		T& get() { return lc.t; }
		const T& get() const { return lc.t; }
	};

	class ConstLocked : Uncopyable {
		const Mutual &lc;
	public:
		ConstLocked(const Mutual &lc) : lc(lc) { lc.mut.lock(); }
		~ConstLocked() { lc.mut.unlock(); }
		const T* operator->() const { return &lc.t; }
		const T& get() const { return lc.t; }
	};
	
	class TryLocked : Uncopyable {
		Mutual *const lc;
	public:
		TryLocked(Mutual &lc) : lc(lc.mut.tryLock() ? &lc : 0) {}
		~TryLocked() { if (lc) lc->mut.unlock(); }
		T* operator->() const { return &lc->t; }
		T* get() const { return lc ? &lc->t : 0; }
		operator bool() const { return lc; }
	};
};

#endif
