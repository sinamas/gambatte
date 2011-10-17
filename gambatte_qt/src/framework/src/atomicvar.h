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
#ifndef ATOMICVAR_H
#define ATOMICVAR_H

template<typename T>
class AtomicVar {
	mutable QMutex mut;
	T var;
public:
	AtomicVar() {}
	explicit AtomicVar(const T var) : var(var) {}

	class Locked : Uncopyable {
		AtomicVar &av;
	public:
		Locked(AtomicVar &av) : av(av) { av.mut.lock(); }
		~Locked() { av.mut.unlock(); }
		T get() const { return av.var; }
		void set(const T v) { av.var = v; }
	};

	class ConstLocked : Uncopyable {
		const AtomicVar &av;
	public:
		ConstLocked(const AtomicVar &av) : av(av) { av.mut.lock(); }
		~ConstLocked() { av.mut.unlock(); }
		T get() const { return av.var; }
	};
};

#endif
