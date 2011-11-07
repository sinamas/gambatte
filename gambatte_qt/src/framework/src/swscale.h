/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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

#include "array.h"
#include <cstring>

template<typename T>
void nearestNeighborScale(const T *src, T *dst, const unsigned inWidth,
		const unsigned inHeight, const unsigned outWidth, const unsigned outHeight, const unsigned dstPitch) {
	int vppos = inHeight >> 1;
	unsigned h = inHeight;
	
	do {
		{
			int hppos = inWidth >> 1;
			unsigned w = inWidth;
			
			do {
				const T pxl = *src++;
				hppos -= static_cast<int>(outWidth);
				
				do {
					*dst++ = pxl;
				} while ((hppos += static_cast<int>(inWidth)) < 0);
			} while (--w);
		}
		
		dst -= outWidth;
		dst += dstPitch;
		vppos -= static_cast<int>(outHeight);
		
		while ((vppos += static_cast<int>(inHeight)) < 0) {
			std::memcpy(dst, dst - dstPitch, outWidth * sizeof(T));
			dst += dstPitch;
		}
	} while (--h);
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
void linearScale(const T *src, T *dst, const unsigned inWidth, const unsigned inHeight,
		const unsigned outWidth, const unsigned outHeight, const unsigned dstPitch) {
	const ScopedArray<T> sums(new T[inWidth + 1]);
	const ScopedArray<unsigned char> hcoeffs(new unsigned char[outWidth - 1]);
	
	{
		unsigned hppos = (outWidth + inWidth) >> 1;
		unsigned w = inWidth;
		unsigned char *coeff = hcoeffs;
		
		do {
			while (hppos < outWidth) {
				*coeff++ = (hppos << c13distance) / outWidth;
				hppos += inWidth;
			}
			
			hppos -= outWidth;
		} while (--w);
	}
	
	unsigned vppos = (outHeight + inHeight) >> 1;
	unsigned srcPitch = 0;
	unsigned h = inHeight;
	unsigned hn = 2;
	
	do {
		do {
			while (vppos < outHeight) {
				{
					const unsigned coeff = (vppos << c13distance) / outHeight;
					const T *s = src;
					T *sum = sums + 1;
					unsigned n = inWidth;
					
					do {
						const T p1c13 = *s            & c13mask;
						const T p1c2  = *s            & c2mask ;
						const T p2c13 = *(s+srcPitch) & c13mask;
						const T p2c2  = *(s+srcPitch) & c2mask ;
						++s;
						
						*sum++ = ((p1c13 + ((p2c13 - p1c13) * coeff >> c13distance)) & c13mask) |
						         ((p1c2  + ((p2c2  - p1c2)  * coeff >> c13distance)) & c2mask );
					} while (--n);
					
					sums[0] = sums[1];
				}
				
				{
					const T *sum = sums;
					int hppos = (outWidth + inWidth) >> 1;
					const unsigned char *coeff = hcoeffs;
					unsigned w = inWidth;
					
					do {
						const T p1c13 = *sum     & c13mask;
						const T p1c2  = *sum     & c2mask;
						const T p2c13 = *(sum+1) & c13mask;
						const T p2c2  = *(sum+1) & c2mask;
						++sum;
						
						hppos -= static_cast<int>(outWidth);
						
						while (hppos < 0) {
							*dst++ = ((p1c13 + ((p2c13 - p1c13) * *coeff >> c13distance)) & c13mask) |
							         ((p1c2  + ((p2c2  - p1c2)  * *coeff >> c13distance)) & c2mask );
							++coeff;
							hppos += static_cast<int>(inWidth);
						}
					} while (--w);
					
					do {
						*dst++ = *sum;
					} while (static_cast<unsigned>(hppos += inWidth) < (outWidth + inWidth) >> 1);
				}
				
				dst += dstPitch - outWidth;
				vppos += inHeight;
			}
			
			vppos -= outHeight;
			src += srcPitch;
			srcPitch = inWidth;
		} while (--h);
		
		h = 1;
		srcPitch = 0;
		vppos += outHeight - ((outHeight + inHeight) >> 1);
	} while (--hn);
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
static void semiLinearScale1d(const T *in, T *out, const unsigned inWidth, const unsigned outWidth, const unsigned char *coeff) {
	int hppos = inWidth;
	unsigned w = inWidth;
	
	while (--w) {
		const T px1 = *in;
		hppos -= static_cast<int>(outWidth);
		
		while (hppos < 0) {
			*out++ = px1;
			hppos += static_cast<int>(inWidth);
		}
		
		hppos += static_cast<int>(inWidth);
		
		const T p1c13 = px1     & c13mask;
		const T p1c2  = px1     & c2mask;
		const T p2c13 = *(in+1) & c13mask;
		const T p2c2  = *(in+1) & c2mask;
		++in;
		
		*out++ = ((p1c13 + ((p2c13 - p1c13) * *coeff >> c13distance)) & c13mask) |
		         ((p1c2  + ((p2c2  - p1c2 ) * *coeff >> c13distance)) & c2mask ) ;
		++coeff;
	}
	
	do {
		*out++ = *in;
	} while (static_cast<unsigned>(hppos += inWidth) <= outWidth);
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
void semiLinearScale(const T *in, T *out, const unsigned inWidth, const unsigned inHeight,
		const unsigned outWidth, const unsigned outHeight, const unsigned outPitch) {
	const ScopedArray<T> sums(new T[inWidth]);
	const ScopedArray<unsigned char> hcoeffs(new unsigned char[inWidth]);
	
	{
		unsigned hppos = inWidth;
		unsigned w = inWidth;
		unsigned char *coeff = hcoeffs;
		
		while (--w) {
			while (hppos < outWidth)
				hppos += inWidth;
			
			hppos -= outWidth;
			*coeff++ = (hppos << c13distance) / inWidth;
		}
	}
	
	unsigned vppos = inHeight;
	unsigned h = inHeight;
	
	do {
		if (vppos <= outHeight) {
			semiLinearScale1d<T,c13mask,c2mask,c13distance>(in, out, inWidth, outWidth, hcoeffs);
			out += outPitch;
			
			while ((vppos += inHeight) <= outHeight) {
				std::memcpy(out, out - outPitch, outWidth * sizeof(T));
				out += outPitch;
			}
		}
		
		if ((vppos -= outHeight) < inHeight) {
			{
				const unsigned coeff = (vppos << c13distance) / inHeight;
				T *sum = sums;
				unsigned n = inWidth;
				
				do {
					const T p1c13 = *in           & c13mask;
					const T p1c2  = *in           & c2mask;
					const T p2c13 = *(in+inWidth) & c13mask;
					const T p2c2  = *(in+inWidth) & c2mask;
					++in;
					
					*sum++ = ((p1c13 + ((p2c13 - p1c13) * coeff >> c13distance)) & c13mask) |
					         ((p1c2  + ((p2c2  - p1c2 ) * coeff >> c13distance)) & c2mask ) ;
				} while (--n);
			}
			
			semiLinearScale1d<T,c13mask,c2mask,c13distance>(sums, out, inWidth, outWidth, hcoeffs);
			out += outPitch;
			vppos += inHeight;
		} else
			in += inWidth;
	} while (--h);
}

#endif
