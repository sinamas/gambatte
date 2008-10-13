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
#ifndef MAKE_SINC_KERNEL_H
#define MAKE_SINC_KERNEL_H

#include <cmath>
#include <cstdlib>

template<class Window>
void makeSincKernel(short *const kernel, const unsigned phases, const unsigned phaseLen, double fc, Window win) {
	static const double PI = 3.14159265358979323846;
	fc /= phases;
	
	/*{
		double *const dkernel = new double[phaseLen * phases];
		const long M = static_cast<long>(phaseLen) * phases - 1;
		
		for (long i = 0; i < M + 1; ++i) {
			const double sinc = i * 2 == M ?
					PI * fc :
					std::sin(PI * fc * (i * 2 - M)) / (i * 2 - M);
			
			dkernel[(i % phases) * phaseLen + i / phases] = win(i, M) * sinc;
		}
		
		{
			double gain = 0;
			
			for (long i = 0; i < M + 1; ++i)
				gain += std::abs(dkernel[i]);
			
			gain = phases * 0x10000 / gain;
			
			for (long i = 0; i < M + 1; ++i)
				kernel[i] = std::floor(dkernel[i] * gain + 0.5);
		}
		
		delete[] dkernel;
	}*/
	
	// The following is equivalent to the more readable version above
	
	const long M = static_cast<long>(phaseLen) * phases - 1;
	
	{
		double *const dkernel = new double[M / 2 + 1];
		
		for (long i = 0; i < M / 2 + 1; ++i) {
			const double sinc = i * 2 == M ?
					PI * fc :
					std::sin(PI * fc * (i * 2 - M)) / (i * 2 - M);
			
			dkernel[i] = win(i, M) * sinc;
		}
		
		double gain = 0;
		
		for (long i = 0; i < M / 2; ++i)
			gain += std::abs(dkernel[i]);
		
		gain *= 2;
		gain += std::abs((M & 1) ? dkernel[M / 2] * 2 : dkernel[M / 2]);
		gain = phases * 0x10000 / gain;
		
		{
			long kpos = 0;
			
			for (long i = 0; i < M / 2 + 1; ++i) {
				kernel[kpos] = std::floor(dkernel[i] * gain + 0.5);
				
				if ((kpos += phaseLen) > M)
					kpos -= M;
			}
		}
		
		delete[] dkernel;
	}
		
	for (unsigned phase = 0; phase < phases; ++phase) {
		short *k = kernel + phase * phaseLen;
		short *km = kernel + M - phase * phaseLen;
		
		for (long i = phase; i < M / 2 + 1; i += phases)
			*km-- = *k++;
	}
}

#endif
