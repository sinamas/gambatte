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

#ifndef SWSCALE_H
#define SWSCALE_H

#include "array.h"
#include <cstddef>
#include <cstring>

template<typename T>
void nearestNeighborScale(
		T *dst, std::ptrdiff_t const dstPitch, int const outWidth, int const outHeight,
		T const *src, std::ptrdiff_t const srcPitch, int const inWidth, int const inHeight) {
	int vppos = inHeight >> 1;
	int h = inHeight;

	while (h-- > 0) {
		{
			T const *const s = src + inWidth;
			int hppos = inWidth >> 1;
			int n = -inWidth;

			do {
				T const pxl = s[n];
				hppos -= outWidth;

				do {
					*dst++ = pxl;
				} while ((hppos += inWidth) < 0);
			} while (++n);
		}

		src += srcPitch;
		dst += dstPitch - outWidth;
		vppos -= outHeight;

		while ((vppos += inHeight) < 0) {
			std::memcpy(dst, dst - dstPitch, outWidth * sizeof *dst);
			dst += dstPitch;
		}
	}
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
void linearScale(T *dst, std::ptrdiff_t const dstPitch, int const outWidth, int const outHeight,
                 T const *src, std::ptrdiff_t const srcPitch, int const inWidth, int const inHeight) {
	if (inWidth <= 0 || inHeight <= 0)
		return;

	SimpleArray<T> const sums(inWidth + 1);
	SimpleArray<unsigned char> const hcoeffs(outWidth - 1);

	{
		int hppos = (outWidth + inWidth) >> 1;
		int w = inWidth;
		unsigned char *coeff = hcoeffs;

		do {
			while (hppos < outWidth) {
				*coeff++ = (hppos << c13distance) / outWidth;
				hppos += inWidth;
			}

			hppos -= outWidth;
		} while (--w);
	}

	std::ptrdiff_t sPitch = 0;
	int vppos = (outHeight + inHeight) >> 1;
	int h = inHeight;
	int hn = 2;

	do {
		do {
			while (vppos < outHeight) {
				{
					unsigned const coeff = (vppos << c13distance) / outHeight;
					T const *const s = src + inWidth;
					T *const sum = sums + inWidth + 1;
					int n = -inWidth;

					do {
						T const p1c13 = s[n]          & c13mask;
						T const p1c2  = s[n]          & c2mask ;
						T const p2c13 = s[n + sPitch] & c13mask;
						T const p2c2  = s[n + sPitch] & c2mask ;

						sum[n] = ((p1c13 + ((p2c13 - p1c13) * coeff >> c13distance)) & c13mask)
						       | ((p1c2  + ((p2c2  - p1c2)  * coeff >> c13distance)) & c2mask );
					} while (++n);

					sums[0] = sums[1];
				}

				{
					T const *const sum = sums + inWidth;
					int hppos = (outWidth + inWidth) >> 1;
					unsigned char const *coeff = hcoeffs;
					int n = -inWidth;

					do {
						T const p1c13 = sum[n]     & c13mask;
						T const p1c2  = sum[n]     & c2mask;
						T const p2c13 = sum[n + 1] & c13mask;
						T const p2c2  = sum[n + 1] & c2mask;

						hppos -= outWidth;

						while (hppos < 0) {
							*dst++ = ((p1c13 + ((p2c13 - p1c13) * *coeff >> c13distance))
							          & c13mask)
							       | ((p1c2  + ((p2c2  - p1c2)  * *coeff >> c13distance))
							          & c2mask);
							++coeff;
							hppos += inWidth;
						}
					} while (++n);

					do {
						*dst++ = *sum;
					} while ((hppos += inWidth) < (outWidth + inWidth) >> 1);
				}

				dst += dstPitch - outWidth;
				vppos += inHeight;
			}

			vppos -= outHeight;
			src += sPitch;
			sPitch = srcPitch;
		} while (--h);

		h = 1;
		sPitch = 0;
		vppos += outHeight - ((outHeight + inHeight) >> 1);
	} while (--hn);
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
static void semiLinearScale1d(T *out, int const outWidth,
		T const *in, int const inWidth, unsigned char const *coeff) {
	int hppos = inWidth;
	int n = -inWidth + 1;
	coeff -= n;
	in -= n;

	while (n < 0) {
		T const px1 = in[n];
		hppos -= outWidth;

		while (hppos < 0) {
			*out++ = px1;
			hppos += inWidth;
		}

		hppos += inWidth;

		T const p1c13 = px1       & c13mask;
		T const p1c2  = px1       & c2mask;
		T const p2c13 = in[n + 1] & c13mask;
		T const p2c2  = in[n + 1] & c2mask;

		*out++ = ((p1c13 + ((p2c13 - p1c13) * coeff[n] >> c13distance)) & c13mask)
		       | ((p1c2  + ((p2c2  - p1c2 ) * coeff[n] >> c13distance)) & c2mask );
		++n;
	}

	do {
		*out++ = *in;
	} while ((hppos += inWidth) <= outWidth);
}

template<typename T, T c13mask, T c2mask, unsigned c13distance>
void semiLinearScale(
		T *dst, std::ptrdiff_t const dstPitch, int const outWidth, int const outHeight,
		T const *src, std::ptrdiff_t const srcPitch, int const inWidth, int const inHeight) {
	if (inWidth <= 0 || inHeight <= 0)
		return;

	SimpleArray<T> const sums(inWidth);
	SimpleArray<unsigned char> const hcoeffs(inWidth);

	{
		int hppos = inWidth;
		int w = inWidth;
		unsigned char *coeff = hcoeffs;

		while (--w) {
			while (hppos < outWidth)
				hppos += inWidth;

			hppos -= outWidth;
			*coeff++ = (hppos << c13distance) / inWidth;
		}
	}

	int vppos = inHeight;
	int h = inHeight;

	do {
		if (vppos <= outHeight) {
			semiLinearScale1d<T,c13mask,c2mask,c13distance>(dst, outWidth, src, inWidth, hcoeffs);
			dst += dstPitch;

			while ((vppos += inHeight) <= outHeight) {
				std::memcpy(dst, dst - dstPitch, outWidth * sizeof *dst);
				dst += dstPitch;
			}
		}

		if ((vppos -= outHeight) < inHeight) {
			{
				unsigned const coeff = (vppos << c13distance) / inHeight;
				T *const sum = sums + inWidth;
				T const *const s = src + inWidth;
				int n = -inWidth;

				do {
					T const p1c13 = s[n]            & c13mask;
					T const p1c2  = s[n]            & c2mask;
					T const p2c13 = s[n + srcPitch] & c13mask;
					T const p2c2  = s[n + srcPitch] & c2mask;

					sum[n] = ((p1c13 + ((p2c13 - p1c13) * coeff >> c13distance)) & c13mask)
					       | ((p1c2  + ((p2c2  - p1c2 ) * coeff >> c13distance)) & c2mask );
				} while (++n);
			}

			semiLinearScale1d<T,c13mask,c2mask,c13distance>(dst, outWidth, sums, inWidth, hcoeffs);
			dst += dstPitch;
			vppos += inHeight;
		}

		src += srcPitch;
	} while (--h);
}

#endif
