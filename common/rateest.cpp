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

void RateEst::init(const long srate) {
	this->srate.est = srate;
	this->srate.var = srate >> 13;
	last = 0;
	samples = 0;
	count = 16;
}

void RateEst::feed(const long samplesIn) {
	samples += samplesIn;
		
	if (--count == 0) {
		count = 16;
		
		const usec_t now = getusecs();
		
		if (last) {
			long est = samples * 1000000.0f / (now - last) + 0.5f;
			est = (srate.est * 15 + est + 8) >> 4;
			srate.var = (srate.var * 15 + std::abs(est - srate.est) + 8) >> 4;
			srate.est = est;
		}
		
		last = now;
		samples = 0;
	}
}
