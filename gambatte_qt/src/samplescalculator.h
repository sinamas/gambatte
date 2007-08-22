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
#ifndef SAMPLESCALCULATOR_H
#define SAMPLESCALCULATOR_H

class SamplesCalculator {
	unsigned samples;
	unsigned baseSamples;
	unsigned updates;
	unsigned lastFromUnderrun;
	unsigned lastUnderrunTime;
	unsigned lastOverflowTime;
	unsigned samplesOverflowed;
	
	const unsigned maxDiff;
	
public:
	SamplesCalculator(unsigned baseSamples = 804, unsigned maxDiff = 4);
	
	void setBaseSamples(unsigned samples);
	void update(unsigned fromUnderrun, unsigned fromOverflow);
	
	unsigned getSamples() const {
		return samples;
	}
};

#endif
