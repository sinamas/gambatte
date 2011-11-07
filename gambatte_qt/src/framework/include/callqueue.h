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
#ifndef CALLQUEUE_H
#define CALLQUEUE_H

#include "uncopyable.h"
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <deque>

union DefaultTypeAlignUnion {
	double d;
	void *p;
	long l;
};

template<std::size_t ALIGN = sizeof(DefaultTypeAlignUnion)>
class ParamQueue : Uncopyable {
	char *data;
	char *dataend;
	char *start;
	char *end;
	std::size_t avail;
	
	template<class T> struct PaddedSize { enum { R = ((sizeof(T) + ALIGN - 1) / ALIGN) * ALIGN }; };
	
public:
	explicit ParamQueue(const std::size_t sz = ALIGN * 2) :
			data((char*) std::malloc(sz)), dataend(data + sz), start(data), end(data), avail(sz) {}
	~ParamQueue() { free(data); }
	
	template<class T> bool hasSpaceFor() {
		return avail > PaddedSize<T>::R * 2 - 2;
	}
	
	template<class T> void push(const T &t);
	
	template<class T> T& front() {
		return *reinterpret_cast<T*>(dataend - start < PaddedSize<T>::R ? data : start);
	}
	
	template<class T> const T& front() const {
		return *reinterpret_cast<const T*>(dataend - start < PaddedSize<T>::R ? data : start);
	}
	
	template<class T> void pop();
	
	std::size_t size() const { return dataend - data; }
	static void swap(ParamQueue &a, ParamQueue &b);
};

template<std::size_t ALIGN>
template<class T> void ParamQueue<ALIGN>::push(const T &t) {
	if (dataend - end < PaddedSize<T>::R) {
		avail -= dataend - end;
		end = data;
	}
	
	new (end) T(t);
	end += PaddedSize<T>::R;
	avail -= PaddedSize<T>::R;
}

template<std::size_t ALIGN>
template<class T> void ParamQueue<ALIGN>::pop() {
	if (dataend - start < PaddedSize<T>::R) {
		avail += dataend - start;
		start = data;
	}
	
	reinterpret_cast<T*>(start)->~T();
	start += PaddedSize<T>::R;
	avail += PaddedSize<T>::R;
}

template<std::size_t ALIGN>
void ParamQueue<ALIGN>::swap(ParamQueue<ALIGN> &a, ParamQueue<ALIGN> &b) {
	std::swap(a.data, b.data);
	std::swap(a.dataend, b.dataend);
	std::swap(a.start, b.start);
	std::swap(a.end, b.end);
	std::swap(a.avail, b.avail);
}

template<class T, std::size_t ALIGN> void popto(ParamQueue<ALIGN> &pq, ParamQueue<ALIGN> *const pqout) {
	if (pqout)
		pqout->push(pq.template front<T>());
	
	pq.template pop<T>();
}

template<typename Callptr, std::size_t ALIGN>
struct CallQueueBase {
	struct Funptrs {
		Callptr call;
		void (*popto)(ParamQueue<ALIGN>&, ParamQueue<ALIGN>*);
	};
	
	ParamQueue<ALIGN> pq;
	std::deque<const Funptrs*> fq;
	
	void popAllTo(ParamQueue<ALIGN> *const pqout);
	void incpq();
	
	template<class T> void push(const T &t, const Funptrs *const fptrs) {
		while (!pq.template hasSpaceFor<T>())
			incpq();
		
		pq.push(t);
		fq.push_back(fptrs);
	}
	
	~CallQueueBase() { popAllTo(0); }
};

template<typename Callptr, std::size_t ALIGN>
void CallQueueBase<Callptr, ALIGN>::popAllTo(ParamQueue<ALIGN> *const pqout) {
	for (typename std::deque<const Funptrs*>::iterator it = fq.begin(); it != fq.end(); ++it)
		(*it)->popto(pq, pqout);
}

template<typename Callptr, std::size_t ALIGN>
void CallQueueBase<Callptr, ALIGN>::incpq() {
	ParamQueue<ALIGN> pq2(pq.size() * 2);
	popAllTo(&pq2);
	ParamQueue<ALIGN>::swap(pq, pq2);
}

class NoParam;

template<typename Param = NoParam, std::size_t ALIGN = sizeof(DefaultTypeAlignUnion)>
class CallQueue {
	CallQueueBase<void (*)(ParamQueue<ALIGN>&, Param), ALIGN> base;
	
	template<class T> static void call(ParamQueue<ALIGN> &pq, Param p) {
		pq.template front<T>()(p);
		pq.template pop<T>();
	}
	
public:
	template<class T> void push(const T &t) {
		static const typename CallQueueBase<void (*)(ParamQueue<ALIGN>&, Param), ALIGN>::Funptrs fptrs = { call<T>, popto<T,ALIGN> };
		base.push(t, &fptrs);
	}
	
	void pop(Param p) {
		base.fq.front()->call(base.pq, p);
		base.fq.pop_front();
	}
	
	void pop_all(Param p) {
		while (!base.fq.empty())
			pop(p);
	}
	
	std::size_t size() const { return base.fq.size(); }
	bool empty() const { return base.fq.empty(); }
};

template<std::size_t ALIGN>
class CallQueue<NoParam, ALIGN> {
	CallQueueBase<void (*)(ParamQueue<ALIGN>&), ALIGN> base;

	template<class T> static void call(ParamQueue<ALIGN> &pq) {
		pq.template front<T>()();
		pq.template pop<T>();
	}
	
public:
	template<class T> void push(const T &t) {
		static const typename CallQueueBase<void (*)(ParamQueue<ALIGN>&), ALIGN>::Funptrs fptrs = { call<T>, popto<T,ALIGN> };
		base.push(t, &fptrs);
	}
	
	void pop() {
		base.fq.front()->call(base.pq);
		base.fq.pop_front();
	}
	
	void pop_all() {
		while (!base.fq.empty())
			pop();
	}
	
	std::size_t size() const { return base.fq.size(); }
	bool empty() const { return base.fq.empty(); }
};

#endif
