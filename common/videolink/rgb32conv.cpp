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
#include "rgb32conv.h"
#include "videolink.h"
#include <int.h>
#include <algorithm>

namespace {
class Rgb32ToUyvy {
	struct CacheUnit {
		Gambatte::uint_least32_t rgb32;
		Gambatte::uint_least32_t uyvy;
	};
	
	enum { cache_size = 0x100 };
	enum { cache_mask = cache_size - 1 };
	
	CacheUnit cache[cache_size];
	
public:
	Rgb32ToUyvy();
	void operator()(const Gambatte::uint_least32_t *s, Gambatte::uint_least32_t *d,
				unsigned w, unsigned h, unsigned srcPitch, unsigned dstPitch);
};

Rgb32ToUyvy::Rgb32ToUyvy() {
#ifdef WORDS_BIGENDIAN
	const CacheUnit c = { 0, 128ul << 24 | 16ul << 16 | 128 << 8 | 16 };
#else
	const CacheUnit c = { 0, 16ul << 24 | 128ul << 16 | 16 << 8 | 128 };
#endif
	std::fill(cache, cache + cache_size, c);
}

void Rgb32ToUyvy::operator()(const Gambatte::uint_least32_t *s, Gambatte::uint_least32_t *d, const unsigned w, unsigned h, const unsigned s_pitch, const unsigned d_pitch) {
	while (h--) {
		unsigned n = w >> 1;
		
		do {
			if ((cache[*s & cache_mask].rgb32 - *s) | (cache[*(s+1) & cache_mask].rgb32 - *(s+1))) {
				cache[*s     & cache_mask].rgb32 = *s;
				cache[*(s+1) & cache_mask].rgb32 = *(s+1);

				const unsigned long r = (*s >> 16 & 0x000000FF) | (*(s+1)       & 0x00FF0000);
				const unsigned long g = (*s >>  8 & 0x000000FF) | (*(s+1) <<  8 & 0x00FF0000);
				const unsigned long b = (*s       & 0x000000FF) | (*(s+1) << 16 & 0x00FF0000);

				const unsigned long y = r *  66 + g * 129 + b * 25 + ( 16 * 256 + 128) * 0x00010001ul;
				const unsigned long u = b * 112 - r *  38 - g * 74 + (128 * 256 + 128) * 0x00010001ul;
				const unsigned long v = r * 112 - g *  94 - b * 18 + (128 * 256 + 128) * 0x00010001ul;

#ifdef WORDS_BIGENDIAN
				cache[*s & cache_mask].uyvy = (u << 16 & 0xFF000000) | (y << 8 & 0x00FF0000) | (v & 0x0000FF00) | (y >> 8 & 0x000000FF);
				cache[*(s+1) & cache_mask].uyvy = (u & 0xFF000000) | (y >> 8 & 0x00FF0000) | (v >> 16 & 0x0000FF00) | y >> 24;
#else
				cache[*s & cache_mask].uyvy = (y << 16 & 0xFF000000) | (v << 8 & 0x00FF0000) | (y & 0x0000FF00) | (u >> 8 & 0x000000FF);
				cache[*(s+1) & cache_mask].uyvy = (y & 0xFF000000) | (v >> 8 & 0x00FF0000) | (y >> 16 & 0x0000FF00) | u >> 24;
#endif
			}
			
			*d     = cache[*s     & cache_mask].uyvy;
			*(d+1) = cache[*(s+1) & cache_mask].uyvy;
			s += 2;
			d += 2;
		} while (--n);

		s += s_pitch - w;
		d += d_pitch - w;
	}
}

static void rgb32ToRgb16(const Gambatte::uint_least32_t *s, Gambatte::uint_least16_t *d,
		const unsigned w, unsigned h, const unsigned srcPitch, const unsigned dstPitch)
{
	do {
		unsigned n = w;

		do {
			*d++ = (*s >> 8 & 0xF800) | (*s >> 5 & 0x07E0) | (*s >> 3 & 0x001F);
			++s;
		} while (--n);

		s += srcPitch - w;
		d += dstPitch - w;
	} while (--h);
}

class Rgb32ToUyvyLink : public VideoLink {
	Rgb32ToUyvy rgb32ToUyvy;
	const unsigned width_;
	const unsigned height_;

public:
	Rgb32ToUyvyLink(unsigned width, unsigned height) :
			VideoLink(new Gambatte::uint_least32_t[width * height], width),
			width_(width), height_(height) {}
	~Rgb32ToUyvyLink() { delete[] static_cast<Gambatte::uint_least32_t*>(inBuf()); }
	
	void draw(void *dst, unsigned dstpitch) {
		rgb32ToUyvy(static_cast<Gambatte::uint_least32_t*>(inBuf()),
				static_cast<Gambatte::uint_least32_t*>(dst), width_, height_, inPitch(), dstpitch);
	}
};

class Rgb32ToRgb16Link : public VideoLink {
	const unsigned width_;
	const unsigned height_;

public:
	Rgb32ToRgb16Link(unsigned width, unsigned height) :
			VideoLink(new Gambatte::uint_least32_t[width * height], width),
			width_(width), height_(height) {}
	~Rgb32ToRgb16Link() { delete[] static_cast<Gambatte::uint_least32_t*>(inBuf()); }
	
	void draw(void *dst, unsigned dstpitch) {
		rgb32ToRgb16(static_cast<Gambatte::uint_least32_t*>(inBuf()),
				static_cast<Gambatte::uint_least16_t*>(dst), width_, height_, inPitch(), dstpitch);
	}
};
}

VideoLink* Rgb32Conv::create(PixelFormat pf, unsigned width, unsigned height) {
	switch (pf) {
	case RGB16: return new Rgb32ToRgb16Link(width, height);
	case  UYVY: return new Rgb32ToUyvyLink(width, height);
	default: return 0;
	}
}
