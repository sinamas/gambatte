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

template<typename T>
static inline void do_scaleBuffer(const T *s, T *d, const unsigned srcW, const unsigned srcH, const unsigned dstPitch, const unsigned scale) {
	const unsigned dstW = srcW * scale;
	
	for (unsigned h = srcH; h--;) {
		for (unsigned w = srcW; w--;) {
			for (unsigned n = scale; n--;)
				*d++ = *s;
	
			++s;
		}
		
		s += dstPitch - dstW;
	
		for (unsigned n = scale; --n; d += dstPitch)
			std::memcpy(d, d - dstPitch, dstW * sizeof(T));
	}
}

template<typename T>
void scaleBuffer(const T *s, T *d, const unsigned srcW, const unsigned srcH, const unsigned dstPitch, const unsigned scale) {
	switch (scale) {
	case 2: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 2); break;
	case 3: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 3); break;
	case 4: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 4); break;
	case 5: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 5); break;
	case 6: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 6); break;
	case 7: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 7); break;
	case 8: do_scaleBuffer(s, d, srcW, srcH, dstPitch, 8); break;
	default: do_scaleBuffer(s, d, srcW, srcH, dstPitch, scale); break;
	}
}

#endif /*SCALEBUFFER_H_*/
