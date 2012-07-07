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
#ifndef HAMMINGSINC_H
#define HAMMINGSINC_H

#include "convoluter.h"
#include "subresampler.h"
#include "makesinckernel.h"
#include "cic3.h"
#include "array.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

template<unsigned channels, unsigned phases>
class HammingSinc : public SubResampler {
	PolyPhaseConvoluter<channels, phases> convoluter_;
	Array<short> kernel;
	
	static double hammingWin(const long i, const long M) {
		static const double PI = 3.14159265358979323846;
		return 0.53836 - 0.46164 * std::cos(2 * PI * i / M);
	}
	
	void init(unsigned div, unsigned phaseLen, double fc, double gain);
	
public:
	enum { MUL = phases };
	
	typedef Cic3<channels> Cic;
	static float cicLimit() { return 4.2f; }

	class RollOff {
		static unsigned toTaps(const float rollOffWidth) {
			static const float widthTimesTaps = 3.0f;
			return std::max(static_cast<unsigned>(std::ceil(widthTimesTaps / rollOffWidth)), 4u);
		}
		
		static float toFc(const float rollOffStart, const int taps) {
			static const float startToFcDeltaTimesTaps = 1.27f;
			return startToFcDeltaTimesTaps / taps + rollOffStart;
		}
		
	public:
		const unsigned taps;
		const float fc;
		
		RollOff(float rollOffStart, float rollOffWidth) : taps(toTaps(rollOffWidth)), fc(toFc(rollOffStart, taps)) {}
	};

	HammingSinc(unsigned div, unsigned phaseLen, double fc) { init(div, phaseLen, fc, 1.0); }
	HammingSinc(unsigned div, RollOff ro, double gain) { init(div, ro.taps, ro.fc, gain); }
	std::size_t resample(short *out, const short *in, std::size_t inlen) { return convoluter_.filter(out, in, inlen); }
	void adjustDiv(unsigned div) { convoluter_.adjustDiv(div); }
	unsigned mul() const { return MUL; }
	unsigned div() const { return convoluter_.div(); }
};

template<unsigned channels, unsigned phases>
void HammingSinc<channels, phases>::init(unsigned div, unsigned phaseLen, double fc, double gain) {
	kernel.reset(phaseLen * phases);
	makeSincKernel(kernel, phases, phaseLen, fc, hammingWin, gain);
	convoluter_.reset(kernel, phaseLen, div);
}

#endif
