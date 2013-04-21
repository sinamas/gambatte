/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#ifndef CHAINRESAMPLER_H
#define CHAINRESAMPLER_H

#include "../resampler.h"
#include "../resamplerinfo.h"
#include "array.h"
#include "subresampler.h"
#include "upsampler.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <list>

class ChainResampler : public Resampler {
public:
	enum { channels = ResamplerInfo::channels };

	ChainResampler();
	virtual ~ChainResampler() { uninit(); }
	virtual void adjustRate(long inRate, long outRate);
	virtual void exactRatio(unsigned long &mul, unsigned long &div) const;
	virtual std::size_t maxOut(std::size_t /*inlen*/) const { return maxOut_; }
	virtual std::size_t resample(short *out, short const *in, std::size_t inlen);

	template<template<unsigned,unsigned> class Sinc>
	std::size_t init(long inRate, long outRate, std::size_t periodSize);
	void uninit();

private:
	typedef std::list<SubResampler*> List;
	typedef SubResampler * (*CreateSinc)(unsigned div, float rollOffStart,
	                                     float rollOffWidth, double gain);

	List list_;
	SubResampler *bigSinc_;
	Array<short> buffer_;
	short *buffer2_;
	std::size_t periodSize_;
	std::size_t maxOut_;

	static float get1ChainCost(float ratio, float finalRollOffLen) {
		return ratio / finalRollOffLen;
	}

	static float get2ChainMidRatio(float ratio, float finalRollOffLen,
	                               float midRollOffStartPlusEnd);
	static float get2ChainCost(float ratio, float finalRollOffLen, float midRatio,
	                           float midRollOffStartPlusEnd);

	static float get3ChainRatio2(float ratio1,
	                             float finalRollOffLen,
	                             float midRollOffStartPlusEnd) {
		return get2ChainMidRatio(ratio1, finalRollOffLen, midRollOffStartPlusEnd);
	}

	static float get3ChainRatio1(float ratio1, float finalRollOffLen, float ratio,
	                             float midRollOffStartPlusEnd);
	static float get3ChainCost(float ratio, float finalRollOffLen, float ratio1, float ratio2,
	                           float midRollOffStartPlusEnd);

	void downinitAddSincResamplers(double ratio, float outRate,
			CreateSinc createBigSinc, CreateSinc createSmallSinc,
			unsigned bigSincMul, unsigned smallSincMul, double gain);

	template<class Sinc>
	static SubResampler * createSinc(unsigned div,
			float rollOffStart, float rollOffWidth, double gain) {
		return new Sinc(div, typename Sinc::RollOff(rollOffStart, rollOffWidth), gain);
	}

	template<template<unsigned,unsigned> class Sinc>
	std::size_t downinit(long inRate, long outRate, std::size_t periodSize);

	template<template<unsigned,unsigned> class Sinc>
	std::size_t upinit(long inRate, long outRate, std::size_t periodSize);

	std::size_t reallocateBuffer();
};

template<template<unsigned,unsigned> class Sinc>
std::size_t ChainResampler::init(long inRate, long outRate, std::size_t periodSize) {
	setRate(inRate, outRate);

	if (outRate > inRate)
		return upinit<Sinc>(inRate, outRate, periodSize);
	else
		return downinit<Sinc>(inRate, outRate, periodSize);
}

template<template<unsigned,unsigned> class Sinc>
std::size_t ChainResampler::downinit(long const inRate,
                                     long const outRate,
                                     std::size_t const periodSize) {
	typedef Sinc<channels, 2048> BigSinc;
	typedef Sinc<channels,   32> SmallSinc;
	uninit();
	periodSize_ = periodSize;

	double ratio = static_cast<double>(inRate) / outRate;
	double gain = 1.0;
	while (ratio >= BigSinc::cicLimit() * 2) {
		int const div = std::min<int>(int(ratio / BigSinc::cicLimit()),
		                              BigSinc::Cic::MAX_DIV);
		list_.push_back(new typename BigSinc::Cic(div));
		ratio /= div;
		gain *= 1.0 / BigSinc::Cic::gain(div);
	}

	downinitAddSincResamplers(ratio, outRate, createSinc<BigSinc>,
			createSinc<SmallSinc>, BigSinc::MUL, SmallSinc::MUL, gain);
	return reallocateBuffer();
}

template<template<unsigned,unsigned> class Sinc>
std::size_t ChainResampler::upinit(long const inRate,
                                   long const outRate,
                                   std::size_t const periodSize) {
	typedef Sinc<channels, 2048> BigSinc;
	typedef Sinc<channels,   32> SmallSinc;
	uninit();
	periodSize_ = periodSize;

	double ratio = static_cast<double>(outRate) / inRate;
	// Spectral images above 20 kHz assumed inaudible
	// this post-polyphase zero stuffing causes some power loss though.
	{
		int const div = outRate / std::max(inRate, 40000l);
		if (div >= 2) {
			list_.push_front(new Upsampler<channels>(div));
			ratio /= div;
		}
	}

	float const rollOff = std::max((inRate - 36000.0f) / inRate, 0.2f);
	bigSinc_ = new BigSinc(static_cast<int>(BigSinc::MUL / ratio + 0.5),
	                       typename BigSinc::RollOff(0.5f * (1 - rollOff), 0.5f * rollOff),
	                       1.0);
	list_.push_front(bigSinc_); // note: inserted at the front
	return reallocateBuffer();
}

#endif
