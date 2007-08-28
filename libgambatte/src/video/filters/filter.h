/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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

#include <stdint.h>

class Rgb32Putter {
public:
	typedef uint32_t pixel_t;
	
	void operator()(pixel_t *const dest, const unsigned rgb32) {
		*dest = rgb32;
	}
};

class Rgb16Putter {
public:
	typedef uint16_t pixel_t;
	
	void operator()(pixel_t *const dest, const unsigned rgb32) {
// 		const unsigned tmp = (rgb32 & 0xFCFEFC) + 0x040204;
		const unsigned tmp = rgb32;
		
		*dest = tmp >> 8 & 0xF800 | tmp >> 5 & 0x07E0 | tmp >> 3 & 0x001F;
	}
};

class UyvyPutter {
public:
	typedef uint32_t pixel_t;
	
	void operator()(pixel_t *const dest, const unsigned rgb32) {
		const unsigned r = rgb32 >> 16;
		const unsigned g = rgb32 >> 8 & 0xFF;
		const unsigned b = rgb32 & 0xFF;
		
		const unsigned y = r * 66 + g * 129 + b * 25 + 16 * 256 + 128 >> 8;
		const unsigned u = b * 112 - r * 38 - g * 74 + 128 * 256 + 128 >> 8;
		const unsigned v = r * 112 - g * 94 - b * 18 + 128 * 256 + 128 >> 8;
		
		uint8_t * d = reinterpret_cast<uint8_t*>(dest);
		*d++ = u;
		*d++ = y;
		*d++ = v;
		*d++ = y;
	}
};

struct FilterInfo;

class Filter {
public:
	virtual ~Filter() {}
	virtual void init() {};
	virtual void outit() {};
	virtual const FilterInfo& info() = 0;
	virtual void filter(Rgb32Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb32Putter putPixel) = 0;
	virtual void filter(Rgb16Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb16Putter putPixel) = 0;
	virtual void filter(UyvyPutter::pixel_t *const dbuffer, const unsigned pitch, UyvyPutter putPixel) = 0;
	virtual uint32_t* inBuffer() = 0;
	virtual unsigned inPitch() = 0;
};

#endif
