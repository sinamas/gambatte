/***************************************************************************
 *   Copyright (C) 2008-2009 by Sindre Aamås                               *
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
#ifndef KAISER50SINC_H
#define KAISER50SINC_H

#include "convoluter.h"
#include "subresampler.h"
#include "makesinckernel.h"
#include "i0.h"
#include "cic3.h"
#include "array.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

template<unsigned channels, unsigned phases>
class Kaiser50Sinc : public SubResampler {
	PolyPhaseConvoluter<channels, phases> convoluter_;
	Array<short> kernel;
	
	static double kaiserWin(const long n, const long M) {
		static const double beta = 4.62;
		static const double i0beta_rec = 1.0 / i0(beta);
		
		double x = static_cast<double>(n * 2) / M - 1.0;
		x = x * x;
		x = beta * std::sqrt(1.0 - x);
		
		return i0(x) * i0beta_rec;
	}
	
	void init(unsigned div, unsigned phaseLen, double fc, double gain);
	
public:
	enum { MUL = phases };
	
	typedef Cic3<channels> Cic;
	static float cicLimit() { return 4.2f; }

	class RollOff {
		static unsigned toTaps(const float rollOffWidth) {
			static const float widthTimesTaps = 2.715f;
			return std::max(static_cast<unsigned>(std::ceil(widthTimesTaps / rollOffWidth)), 4u);
		}
		
		static float toFc(const float rollOffStart, const int taps) {
			static const float startToFcDeltaTimesTaps = 1.2f;
			return startToFcDeltaTimesTaps / taps + rollOffStart;
		}
		
	public:
		const unsigned taps;
		const float fc;
		
		RollOff(float rollOffStart, float rollOffWidth) : taps(toTaps(rollOffWidth)), fc(toFc(rollOffStart, taps)) {}
	};

	Kaiser50Sinc(unsigned div, unsigned phaseLen, double fc) { init(div, phaseLen, fc, 1.0); }
	Kaiser50Sinc(unsigned div, RollOff ro, double gain) { init(div, ro.taps, ro.fc, gain); }
	std::size_t resample(short *out, const short *in, std::size_t inlen) { return convoluter_.filter(out, in, inlen); }
	void adjustDiv(unsigned div) { convoluter_.adjustDiv(div); }
	unsigned mul() const { return MUL; }
	unsigned div() const { return convoluter_.div(); }
};

template<unsigned channels, unsigned phases>
void Kaiser50Sinc<channels, phases>::init(unsigned div, unsigned phaseLen, double fc, double gain) {
	kernel.reset(phaseLen * phases);
	makeSincKernel(kernel, phases, phaseLen, fc, kaiserWin, gain);
	convoluter_.reset(kernel, phaseLen, div);
}

#endif
