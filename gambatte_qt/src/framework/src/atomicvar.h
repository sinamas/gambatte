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

#ifndef ATOMICVAR_H
#define ATOMICVAR_H

#include "uncopyable.h"
#include <QMutex>

template<typename T>
class AtomicVar {
public:
	AtomicVar() : var_() {}
	explicit AtomicVar(T var) : var_(var) {}

	class Locked : Uncopyable {
	public:
		Locked(AtomicVar &av) : av(av) { av.mut_.lock(); }
		~Locked() { av.mut_.unlock(); }
		T get() const { return av.var_; }
		void set(T v) { av.var_ = v; }

	private:
		AtomicVar &av;
	};

	class ConstLocked : Uncopyable {
	public:
		ConstLocked(AtomicVar const &av) : av(av) { av.mut_.lock(); }
		~ConstLocked() { av.mut_.unlock(); }
		T get() const { return av.var_; }

	private:
		AtomicVar const &av;
	};

private:
	mutable QMutex mut_;
	T var_;
};

#endif
