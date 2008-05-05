/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
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
#include "int.h"

class OsdElement {
	unsigned x_;
	unsigned y_;
	unsigned w_;
	unsigned h_;
	
protected:
	OsdElement(unsigned x = 0, unsigned y = 0, unsigned w = 0, unsigned h = 0) {
		setPos(x, y);
		setSize(w, h);
	}
	
	void setPos(unsigned x, unsigned y) {
		x_ = x;
		y_ = y;
	}
	
	void setSize(unsigned w, unsigned h) {
		w_ = w;
		h_ = h;
	}
	
public:
	unsigned x() const { return x_; }
	unsigned y() const { return y_; }
	unsigned w() const { return w_; }
	unsigned h() const { return h_; }
	
	virtual const Gambatte::uint_least32_t* update() = 0;
};
