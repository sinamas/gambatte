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
	if (est > reference + (reference >> 6))
		est = reference + (reference >> 6);
	else if (est < reference - (reference >> 6))
		est = reference - (reference >> 6);

	return est;
}

void RateEst::init(long srate, long reference) {
	srate <<= UPSHIFT;
	reference <<= UPSHIFT;

	this->srate.est = limit(srate, reference);
	this->srate.var = srate >> 12;
	last = 0;
	this->reference = reference;
	samples = ((this->srate.est >> UPSHIFT) * 12) << 5;
	usecs = 12000000 << 5;
}

void RateEst::feed(const long samplesIn) {
	const usec_t now = getusecs();
	
	if (last) {
		samples += samplesIn << 5;
		usecs += (now - last) << 5;
		
		long est = samples * (1000000.0f * UP) / usecs + 0.5f;
		est = limit((srate.est * 31 + est + 16) >> 5, reference);
		srate.var = (srate.var * 15 + std::abs(est - srate.est) + 8) >> 4;
		srate.est = est;
		
		if (usecs > 16000000 << 5) {
			samples = (samples * 3 + 2) >> 2;
			usecs = (usecs * 3 + 2) >> 2;
		}
	}
	
	last = now;
}
