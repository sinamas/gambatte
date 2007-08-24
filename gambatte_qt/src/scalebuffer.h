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
#ifndef SCALEBUFFER_H_
#define SCALEBUFFER_H_

#include <cstring>

#define SCALE_BUFFER \
const unsigned dstW = srcW * scale; \
\
for (unsigned h = srcH; h--;) { \
	for (unsigned w = srcW; w--;) { \
		for (unsigned n = scale; n--;) \
			*d++ = *s; \
\
		++s; \
	} \
\
	for (unsigned n = scale; --n; d += dstW) \
		std::memcpy(d, d - dstW, dstW * sizeof(T)); \
}


template<typename T, const unsigned scale>
static void do_scaleBuffer(const T *s, T *d, const unsigned srcW, const unsigned srcH) {
	SCALE_BUFFER
}

template<typename T>
static void do_scaleBuffer(const T *s, T *d, const unsigned srcW, const unsigned srcH, const unsigned scale) {
	SCALE_BUFFER
}

#undef SCALE_BUFFER

template<typename T>
void scaleBuffer(const T *s, T *d, const unsigned srcW, const unsigned srcH, const unsigned scale) {
	switch (scale) {
	case 2: do_scaleBuffer<T, 2>(s, d, srcW, srcH); break;
	case 3: do_scaleBuffer<T, 3>(s, d, srcW, srcH); break;
	case 4: do_scaleBuffer<T, 4>(s, d, srcW, srcH); break;
	case 5: do_scaleBuffer<T, 5>(s, d, srcW, srcH); break;
	case 6: do_scaleBuffer<T, 6>(s, d, srcW, srcH); break;
	case 7: do_scaleBuffer<T, 7>(s, d, srcW, srcH); break;
	case 8: do_scaleBuffer<T, 8>(s, d, srcW, srcH); break;
	default: do_scaleBuffer(s, d, srcW, srcH, scale); break;
	}
}

#endif /*SCALEBUFFER_H_*/
