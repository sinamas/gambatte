/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef FILTER_H
#define FILTER_H

#include "int.h"

class Rgb32Putter {
public:
	typedef Gambatte::uint_least32_t pixel_t;
	
	void operator()(pixel_t *const dest, const unsigned long rgb32) {
		*dest = rgb32;
	}
};

class Rgb16Putter {
public:
	typedef Gambatte::uint_least16_t pixel_t;
	
	static unsigned toRgb16(const unsigned long rgb32) {
		return rgb32 >> 8 & 0xF800 | rgb32 >> 5 & 0x07E0 | rgb32 >> 3 & 0x001F;
	}
	
	void operator()(pixel_t *const dest, const unsigned long rgb32) {
		*dest = toRgb16(rgb32);
	}
};

class UyvyPutter {
	static void convert(unsigned &y, unsigned &u, unsigned &v, const unsigned long rgb32) {
		const unsigned r = rgb32 >> 16;
		const unsigned g = rgb32 >> 8 & 0xFF;
		const unsigned b = rgb32 & 0xFF;
		
		y = r * 66 + g * 129 + b * 25 + 16 * 256 + 128 >> 8;
		u = b * 112 - r * 38 - g * 74 + 128 * 256 + 128 >> 8;
		v = r * 112 - g * 94 - b * 18 + 128 * 256 + 128 >> 8;
	}
	
public:
	typedef Gambatte::uint_least32_t pixel_t;
	
	static unsigned long toUyvy(const unsigned long rgb32) {
		unsigned y, u, v;
		convert(y, u, v, rgb32);
		
#ifdef WORDS_BIGENDIAN
		return static_cast<unsigned long>(u) << 24 | static_cast<unsigned long>(y) << 16 | v << 8 | y;
#else
		return static_cast<unsigned long>(y) << 24 | static_cast<unsigned long>(v) << 16 | y << 8 | u;
#endif
	}
	
	void operator()(pixel_t *const dest, const unsigned long rgb32) {
#ifdef CHAR_WIDTH_8
		unsigned y, u, v;
		convert(y, u, v, rgb32);
		
		unsigned char *d = reinterpret_cast<unsigned char*>(dest);
		*d++ = u;
		*d++ = y;
		*d++ = v;
		*d++ = y;
#else
		*dest = toUyvy(rgb32);
#endif
	}
};

namespace Gambatte {
struct FilterInfo;
}

class Filter {
public:
	virtual ~Filter() {}
	virtual void init() {};
	virtual void outit() {};
	virtual const Gambatte::FilterInfo& info() = 0;
	virtual void filter(Rgb32Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb32Putter putPixel) = 0;
	virtual void filter(Rgb16Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb16Putter putPixel) = 0;
	virtual void filter(UyvyPutter::pixel_t *const dbuffer, const unsigned pitch, UyvyPutter putPixel) = 0;
	virtual Gambatte::uint_least32_t* inBuffer() = 0;
	virtual unsigned inPitch() = 0;
};

#endif
