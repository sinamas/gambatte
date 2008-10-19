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
#include "rateest.h"
#include <cstdlib>

static long limit(long est, const long reference) {
	if (est > reference + (reference >> 7))
		est = reference + (reference >> 7);
	else if (est < reference - (reference >> 7))
		est = reference - (reference >> 7);

	return est;
}

void RateEst::init(long srate, long reference) {
	srate <<= UPSHIFT;
	reference <<= UPSHIFT;

	this->srate.est = limit(srate, reference);
	this->srate.var = srate >> 12;
	last = 0;
	this->reference = reference;
	samples = 0;
	count = 1;
}

void RateEst::feed(const long samplesIn) {
	samples += samplesIn;

	if (--count == 0) {
		count = 32;

		const usec_t now = getusecs();

		if (last) {
			long est = samples * (1000000.0f * UP) / (now - last) + 0.5f;
			est = limit((srate.est * 31 + est + 16) >> 5, reference);
			srate.var = (srate.var * 15 + std::abs(est - srate.est) + 8) >> 4;
			srate.est = est;
		}

		last = now;
		samples = 0;
	}
}
