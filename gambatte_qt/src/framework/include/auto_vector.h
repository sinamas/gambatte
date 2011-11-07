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
#ifndef AUTO_VECTOR
#define AUTO_VECTOR

#include <vector>

template<class T, class Allocator = std::allocator<T*> >
class auto_vector : private std::vector<T*, Allocator> {
	struct auto_vector_temporary {
		std::vector<T*, Allocator> v;
		explicit auto_vector_temporary(const std::vector<T*, Allocator> &v): v(v) {}
	};
	
public:
	typedef typename std::vector<T*, Allocator>::const_iterator const_iterator;
	typedef typename std::vector<T*, Allocator>::iterator iterator;
	typedef typename std::vector<T*, Allocator>::size_type size_type;
	
	explicit auto_vector(const Allocator &a = Allocator()) : std::vector<T*, Allocator>(a) {}
	explicit auto_vector(size_type n, const Allocator &a = Allocator()) : std::vector<T*, Allocator>(n, 0, a) {}
	auto_vector(auto_vector &v) : std::vector<T*, Allocator>() { swap(v); }
	auto_vector(const auto_vector_temporary &v) : std::vector<T*, Allocator>(v.v) {}
	
	template<class InputIterator>
	auto_vector(InputIterator first, InputIterator last, const Allocator& a = Allocator()) : std::vector<T*, Allocator>(first, last, a) {}
	~auto_vector() { clear(); }

	auto_vector& operator=(auto_vector &v) { clear(); swap(v); }
	
	using std::vector<T*, Allocator>::begin;
	using std::vector<T*, Allocator>::end;
	using std::vector<T*, Allocator>::rbegin;
	using std::vector<T*, Allocator>::rend;
	using std::vector<T*, Allocator>::size;
	using std::vector<T*, Allocator>::max_size;
	using std::vector<T*, Allocator>::capacity;
	using std::vector<T*, Allocator>::empty;
	using std::vector<T*, Allocator>::reserve;
	using std::vector<T*, Allocator>::operator[];
	using std::vector<T*, Allocator>::at;
	using std::vector<T*, Allocator>::front;
	using std::vector<T*, Allocator>::back;
	using std::vector<T*, Allocator>::push_back;
	
	void resize(size_type sz) { std::vector<T*, Allocator>::resize(sz, 0); }
	
	template<class InputIterator>
	void assign(InputIterator first, InputIterator last) {
		clear();
		std::vector<T*, Allocator>::assign(first, last);
	}
	
	void assign(size_type n) {
		clear();
		std::vector<T*, Allocator>::assign(n);
	}
	
	void pop_back() {
		if (!empty())
			delete back();
		
		std::vector<T*, Allocator>::pop_back();
	}
	
	iterator insert(iterator position, T *x) { std::vector<T*, Allocator>::insert(position, x); }
	void insert(iterator position, size_type n) { std::vector<T*, Allocator>::insert(position, n, 0); }
	
	template<class InputIterator>
	void insert(iterator position, InputIterator first, InputIterator last) { std::vector<T*, Allocator>::insert(position, first, last); }
	
	iterator erase(iterator position) {
		if (position != end())
			delete *position;
		
		return std::vector<T*, Allocator>::erase(position);
	}
	
	iterator erase(iterator first, iterator last) {
		for (iterator it = first; it != last; ++it)
			delete *it;
		
		return std::vector<T*, Allocator>::erase(first, last);
	}
	
	void swap(auto_vector &vec) { std::vector<T*, Allocator>::swap(vec); }
	
	void clear() {
		for (iterator it = begin(); it != end(); ++it)
			delete *it;
		
		std::vector<T*, Allocator>::clear();
	}
	
	const std::vector<T*,Allocator> get() const { return *this; }

	operator const auto_vector_temporary() { std::vector<T*, Allocator> v; v.swap(*this); return auto_vector_temporary(v); }
};

#endif
