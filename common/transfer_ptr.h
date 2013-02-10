#ifndef TRANSFER_PTR_H
#define TRANSFER_PTR_H

#include "defined_ptr.h"

template<class T>
class transfer_ptr {
private:
	struct released { T *p; explicit released(T *p) : p(p) {} };
public:
	explicit transfer_ptr(T *p = 0) : p_(p) {}
	transfer_ptr(transfer_ptr &p) : p_(p.release()) {}
	transfer_ptr(released r) : p_(r.p) {}
	~transfer_ptr() { defined_delete(p_); }
	T * get() const { return p_; }
	T * release() { T *p = p_; p_ = 0; return p; }
	operator const released() { return released(release()); }
	T & operator*() const { return *p_; }
	T * operator->() const { return p_; }

private:
	T *p_;
	transfer_ptr & operator=(transfer_ptr const &);
};

#endif
