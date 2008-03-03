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
#ifndef SWSCALE_H
#define SWSCALE_H

#include <cstring>

template<typename T>
static void nearestNeighborScale(const T *src, T *dst, const unsigned inWidth, const unsigned inHeight, const unsigned outWidth, const unsigned outHeight, const unsigned dstPitch) {
	unsigned long vppos = 0;
	unsigned h = inHeight;
	
	do {
		unsigned long hppos = 0;
		unsigned w = inWidth;
		
		do {
			do {
				*dst++ = *src;
				hppos += inWidth;
			} while (hppos < outWidth);
			
			hppos -= outWidth;
			++src;
		} while (--w);
		
		dst += dstPitch - outWidth;
		vppos += inHeight;
		
		while (vppos < outHeight) {
			std::memcpy(dst, dst - dstPitch, outWidth * sizeof(T));
			dst += dstPitch;
			vppos += inHeight;
		}
		
		vppos -= outHeight;
	} while (--h);
}

template<typename T, const T c13mask, const T c2mask, const unsigned c13distance>
static void linearScale(const T *src, T *dst, const unsigned inWidth, const unsigned inHeight, const unsigned outWidth, const unsigned outHeight, const unsigned dstPitch) {
	struct Colorsum {
		unsigned long c13,c2;
	};
	
	Colorsum *const sums = new Colorsum[inWidth + 1];
	unsigned char *const hcoeffs = new unsigned char[outWidth];
	
	{
		unsigned long hppos = 0;
		unsigned w = inWidth;
		unsigned char *coeff = hcoeffs;
		
		do {
			do {
				*coeff++ = (hppos << c13distance) / outWidth;
				hppos += inWidth;
			} while (hppos < outWidth);
			
			hppos -= outWidth;
		} while (--w);
	}
	
	unsigned srcPitch = inWidth;
	unsigned long vppos = 0;
	unsigned h = inHeight;
	
	do {
		do {
			{
				const unsigned coeff = (vppos << c13distance) / outHeight;
				const T *s = src;
				Colorsum *sum = sums;
				unsigned n = inWidth;
				
				do {
					const T p1c13 = *s & c13mask;
					const T p1c2 = *s & c2mask;
					const T p2c13 = *(s+srcPitch) & c13mask;
					const T p2c2 = *(s+srcPitch) & c2mask;
					
					sum->c13 = p1c13 + ((p2c13 - p1c13) * coeff >> c13distance) & c13mask;
					sum->c2 = (p1c2 << c13distance) + (p2c2 - p1c2) * coeff;
					
					++sum;
					++s;
				} while (--n);
				
				*sum = *(sum-1);
			}
			
			{
				unsigned long hppos = 0;
				const Colorsum *sum = sums;
				const unsigned char *coeff = hcoeffs;
				unsigned w = inWidth;
				
				do {
					do {
						*dst++ = sum->c13 + (((sum+1)->c13 - sum->c13) * *coeff >> c13distance) & c13mask |
						         (sum->c2 << c13distance) + ((sum+1)->c2 - sum->c2) * *coeff >> c13distance * 2 & c2mask;
						hppos += inWidth;
						++coeff;
					} while (hppos < outWidth);
					
					hppos -= outWidth;
					++sum;
				} while (--w);
			}
			
			dst += dstPitch - outWidth;
			vppos += inHeight;
		} while (vppos < outHeight);
		
		vppos -= outHeight;
		src += srcPitch;
		
		if (h == 2)
			srcPitch = 0;
	} while (--h);
	
	delete []sums;
	delete []hcoeffs;
}

#endif
