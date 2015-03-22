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

#ifndef CALLQUEUE_H
#define CALLQUEUE_H

#include "uncopyable.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>

union DefaultTypeAlignUnion {
	double d;
	void *p;
	long l;
};

template<std::size_t align_size = sizeof(DefaultTypeAlignUnion)>
class ParamQueue : Uncopyable {
public:
	explicit ParamQueue(std::size_t size = align_size * 2)
	: data_(new char[size])
	, dataend_(data_ + size)
	, start_(data_)
	, end_(data_)
	, avail_(size)
	{
	}

	~ParamQueue() {
		assert(empty());
		delete []data_;
	}

	template<class T>
	bool hasSpaceFor() const { return avail_ > PaddedSize<T>::r * 2 - 2; }

	template<class T>
	void push(T const &e);

	template<class T>
	T & front() {
		void *p = dataend_ - start_ < PaddedSize<T>::r ? data_ : start_;
		return *static_cast<T *>(p);
	}

	template<class T>
	T const & front() const {
		void const *p = dataend_ - start_ < PaddedSize<T>::r ? data_ : start_;
		return *static_cast<T const *>(p);
	}

	template<class T>
	void pop();

	std::size_t size() const { return dataend_ - data_; }
	static void swap(ParamQueue &a, ParamQueue &b);

private:
	char *data_;
	char *dataend_;
	char *start_;
	char *end_;
	std::size_t avail_;

	template<class T>
	struct PaddedSize {
		enum { r = (sizeof(T) + align_size - 1) / align_size * align_size };
	};

	bool empty() const { return avail_ == size(); }
};

template<std::size_t align_size>
template<class T>
void ParamQueue<align_size>::push(T const &e) {
	if (dataend_ - end_ < PaddedSize<T>::r) {
		avail_ -= dataend_ - end_;
		end_ = data_;
	}

	new (end_) T(e);
	end_ += PaddedSize<T>::r;
	avail_ -= PaddedSize<T>::r;
}

template<std::size_t align_size>
template<class T>
void ParamQueue<align_size>::pop() {
	if (dataend_ - start_ < PaddedSize<T>::r) {
		avail_ += dataend_ - start_;
		start_ = data_;
	}

	reinterpret_cast<T *>(start_)->~T();
	start_ += PaddedSize<T>::r;
	avail_ += PaddedSize<T>::r;
}

template<std::size_t align_size>
void ParamQueue<align_size>::swap(ParamQueue<align_size> &a, ParamQueue<align_size> &b) {
	std::swap(a.data_, b.data_);
	std::swap(a.dataend_, b.dataend_);
	std::swap(a.start_, b.start_);
	std::swap(a.end_, b.end_);
	std::swap(a.avail_, b.avail_);
}

template<class T, std::size_t align_size>
void popto(ParamQueue<align_size> &pq, ParamQueue<align_size> *pqout) {
	if (pqout)
		pqout->push(pq.template front<T>());

	pq.template pop<T>();
}

template<typename Callptr, std::size_t align_size>
struct CallQueueBase {
	struct Funptrs {
		Callptr call;
		void (*popto)(ParamQueue<align_size> &, ParamQueue<align_size> *);
	};

	ParamQueue<align_size> pq;
	std::deque<Funptrs const *> fq;

	~CallQueueBase() { popAllTo(0); }
	void popAllTo(ParamQueue<align_size> *pqout);
	void incpq();

	template<class T>
	void push(T const &e, Funptrs const &fptrs) {
		while (!pq.template hasSpaceFor<T>())
			incpq();

		pq.push(e);
		fq.push_back(&fptrs);
	}
};

template<typename Callptr, std::size_t align_size>
void CallQueueBase<Callptr, align_size>::popAllTo(ParamQueue<align_size> *pqout) {
	for (typename std::deque<Funptrs const *>::iterator it = fq.begin(); it != fq.end(); ++it)
		(*it)->popto(pq, pqout);
}

template<typename Callptr, std::size_t align_size>
void CallQueueBase<Callptr, align_size>::incpq() {
	ParamQueue<align_size> pq2(pq.size() * 2);
	popAllTo(&pq2);
	ParamQueue<align_size>::swap(pq, pq2);
}

class NoParam;

template<typename Param = NoParam, std::size_t align_size = sizeof(DefaultTypeAlignUnion)>
class CallQueue {
public:
	template<class T>
	void push(T const &e) {
		static typename Base::Funptrs const fptrs = { call<T>, popto<T, align_size> };
		base_.push(e, fptrs);
	}

	void pop(Param p) {
		base_.fq.front()->call(base_.pq, p);
		base_.fq.pop_front();
	}

	void pop_all(Param p) {
		while (!base_.fq.empty())
			pop(p);
	}

	std::size_t size() const { return base_.fq.size(); }
	bool empty() const { return base_.fq.empty(); }

private:
	typedef CallQueueBase<void (*)(ParamQueue<align_size> &, Param), align_size> Base;
	Base base_;

	template<class T>
	static void call(ParamQueue<align_size> &pq, Param p) {
		pq.template front<T>()(p);
		pq.template pop<T>();
	}
};

template<std::size_t align_size>
class CallQueue<NoParam, align_size> {
public:
	template<class T>
	void push(T const &e) {
		static typename Base::Funptrs const fptrs = { call<T>, popto<T, align_size> };
		base_.push(e, fptrs);
	}

	void pop() {
		base_.fq.front()->call(base_.pq);
		base_.fq.pop_front();
	}

	void pop_all() {
		while (!base_.fq.empty())
			pop();
	}

	std::size_t size() const { return base_.fq.size(); }
	bool empty() const { return base_.fq.empty(); }

private:
	typedef CallQueueBase<void (*)(ParamQueue<align_size> &), align_size> Base;
	Base base_;

	template<class T>
	static void call(ParamQueue<align_size> &pq) {
		pq.template front<T>()();
		pq.template pop<T>();
	}
};

#endif
