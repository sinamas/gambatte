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
#include "samplescalculator.h"

SamplesCalculator::SamplesCalculator(const unsigned baseSamples, const unsigned maxDiff)
: maxDiff(maxDiff)
{
	setBaseSamples(baseSamples);
}

void SamplesCalculator::setBaseSamples(const unsigned baseSamples) {
	this->baseSamples = baseSamples;
	samples = baseSamples;
	lastFromUnderrun = baseSamples >> 2;
	lastOverflowTime = lastUnderrunTime = updates = samplesOverflowed = 0;
}

// #include <cstdio>

void SamplesCalculator::update(const unsigned fromUnderrun, const unsigned fromOverflow) {
	++updates;
	
	if (fromUnderrun < samples * 2) {
		if (fromUnderrun <= lastFromUnderrun && samples < baseSamples + maxDiff && updates - lastUnderrunTime >= 60) {
			++samples;
			lastFromUnderrun = fromUnderrun;
			lastUnderrunTime = updates;
// 			printf("samples: %u\n", samples);
			lastOverflowTime = samplesOverflowed = 0;
		}
	} else {
		const unsigned of = samples - fromOverflow;
		
		if (!(of & 0x80000000)) {
			if (!lastOverflowTime)
				lastOverflowTime = updates;
			else {
				samplesOverflowed += of;
				
				if (samples > baseSamples - maxDiff && samplesOverflowed >= (updates - lastOverflowTime) * 2 + samples && updates - lastOverflowTime >= 300) {
					--samples;
					lastFromUnderrun = 0xFFFFFFFF;
// 					printf("samples: %u\n", samples);
					samplesOverflowed = 0;
					lastOverflowTime = updates;
				}
			}
		}
	}
}
