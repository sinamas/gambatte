//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef RATIONAL_H
#define RATIONAL_H

struct Rational {
	long num;
	long denom;

	Rational(long num = 1, long denom = 1) : num(num), denom(denom) {}
	float toFloat() const { return static_cast<float>(num) / denom; }
	double toDouble() const { return static_cast<double>(num) / denom; }

	Rational reciprocal() const { return Rational(denom, num); }

	// assumes positive num and denom
	long ceiled() const { return (num - 1) / denom + 1; }
	long floored() const { return num / denom; }
	long rounded() const { return (num + (denom >> 1)) / denom; }
};

inline bool operator==(Rational const &lhs, Rational const &rhs) {
	return lhs.num == rhs.num && lhs.denom == rhs.denom;
}

inline bool operator!=(Rational const &lhs, Rational const &rhs) {
	return lhs.num != rhs.num || lhs.denom != rhs.denom;
}

#endif
