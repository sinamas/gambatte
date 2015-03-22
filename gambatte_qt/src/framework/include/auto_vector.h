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

#ifndef AUTO_VECTOR_H
#define AUTO_VECTOR_H

#include "defined_ptr.h"
#include <algorithm>
#include <vector>

template<class T, class Allocator = std::allocator<T*> >
class auto_vector : private std::vector<T*, Allocator> {
private:
	struct released {
		std::vector<T*, Allocator> v;
		explicit released(std::vector<T*, Allocator> const &v): v(v) {}
	};

public:
	typedef std::vector<T*, Allocator> base;
	typedef typename base::const_iterator const_iterator;
	typedef typename base::const_iterator iterator;
	typedef typename base::size_type size_type;

	explicit auto_vector(Allocator const &a = Allocator()) : base(a) {}
	explicit auto_vector(size_type n, Allocator const &a = Allocator()) : base(n, 0, a) {}
	auto_vector(auto_vector &v) : base() { swap(v); }
	auto_vector(released const &v) : base(v.v) {}

	template<class InputIterator>
	auto_vector(InputIterator first, InputIterator last, Allocator const &a = Allocator())
	: base(first, last, a)
	{
	}

	~auto_vector() { clear(); }

	using base::size;
	using base::max_size;
	using base::capacity;
	using base::empty;
	using base::reserve;
	using base::push_back;

	iterator begin() const { return base::begin(); }
	iterator end() const { return base::end(); }
	iterator rbegin() const { return base::rbegin(); }
	iterator rend() const { return base::rend(); }
	T * operator[](size_type i) const { return base::operator[](i); }
	T * at(size_type i) const { return base::at(i); }
	T * front() const { return base::front(); }
	T * back() const { return base::back(); }

	template<class InputIterator>
	void assign(InputIterator first, InputIterator last) {
		clear();
		base::assign(first, last);
	}

	void assign(size_type n) {
		clear();
		base::assign(n);
	}

	void pop_back() {
		if (!empty())
			defined_delete(back());

		base::pop_back();
	}

	iterator insert(iterator position, T *x) { return base::insert(baseit(position), x); }

	template<class InputIterator>
	void insert(iterator position, InputIterator first, InputIterator last) {
		base::insert(baseit(position), first, last);
	}

	iterator erase(iterator position) {
		if (position != end())
			defined_delete(*position);

		return base::erase(baseit(position));
	}

	iterator erase(iterator first, iterator last) {
		std::for_each(first, last, defined_delete<T>);
		return base::erase(baseit(first), baseit(last));
	}

	void swap(auto_vector &vec) { base::swap(vec); }
	void clear() { erase(begin(), end()); }
	T * reset(size_type i, T *x) { defined_delete(base::operator[](i)); return base::operator[](i) = x; }
	base const & get() const { return *this; }
	operator released const() { base v; v.swap(*this); return released(v); }

private:
	auto_vector& operator=(auto_vector const &v);
	typename base::iterator baseit(iterator it) { return base::begin() + (it - begin()); }
};

#endif
