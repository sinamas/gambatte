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
#include "transfer_ptr.h"
#include "upsampler.h"
#include <algorithm>
#include <list>

class SubResampler;

class ChainResampler : public Resampler {
public:
	enum { channels = ResamplerInfo::channels };

	template<template<unsigned, unsigned> class Sinc>
	static Resampler * create(long inRate, long outRate, std::size_t periodSize);

	virtual ~ChainResampler();
	virtual void adjustRate(long inRate, long outRate);
	virtual void exactRatio(unsigned long &mul, unsigned long &div) const;
	virtual std::size_t maxOut(std::size_t /*inlen*/) const { return maxOut_; }
	virtual std::size_t resample(short *out, short const *in, std::size_t inlen);

private:
	typedef std::list<SubResampler *> List;
	typedef SubResampler * (*CreateSinc)(unsigned div, float rollOffStart,
	                                     float rollOffWidth, double gain);

	List list_;
	SubResampler *bigSinc_;
	Array<short> buffer_;
	short *buffer2_;
	std::size_t const periodSize_;
	std::size_t maxOut_;

	ChainResampler(long inRate, long outRate, std::size_t periodSize);
	void downinitAddSincResamplers(double ratio, float outRate,
			CreateSinc createBigSinc, CreateSinc createSmallSinc,
			unsigned bigSincMul, unsigned smallSincMul, double gain);
	void reallocateBuffer();

	template<class Sinc>
	static SubResampler * createSinc(unsigned div,
			float rollOffStart, float rollOffWidth, double gain) {
		return new Sinc(div, typename Sinc::RollOff(rollOffStart, rollOffWidth), gain);
	}

	template<template<unsigned, unsigned> class Sinc>
	void downinit(long inRate, long outRate);

	template<template<unsigned, unsigned> class Sinc>
	void upinit(long inRate, long outRate);
};

template<template<unsigned, unsigned> class Sinc>
Resampler * ChainResampler::create(long inRate, long outRate, std::size_t periodSize) {
	transfer_ptr<ChainResampler> r(new ChainResampler(inRate, outRate, periodSize));
	if (outRate > inRate)
		r->upinit<Sinc>(inRate, outRate);
	else
		r->downinit<Sinc>(inRate, outRate);

	return r.release();
}

template<template<unsigned, unsigned> class Sinc>
void ChainResampler::downinit(long const inRate,
                              long const outRate) {
	typedef Sinc<channels, 2048> BigSinc;
	typedef Sinc<channels,   32> SmallSinc;

	double ratio = static_cast<double>(inRate) / outRate;
	double gain = 1.0;
	while (ratio >= BigSinc::cicLimit() * 2) {
		int const div = std::min<int>(int(ratio / BigSinc::cicLimit()),
		                              BigSinc::Cic::MAX_DIV);
		list_.push_back(new typename BigSinc::Cic(div));
		ratio /= div;
		gain *= 1.0 / BigSinc::Cic::gain(div);
	}

	downinitAddSincResamplers(ratio, outRate,
	                          createSinc<BigSinc>, createSinc<SmallSinc>,
	                          BigSinc::MUL, SmallSinc::MUL, gain);
	reallocateBuffer();
}

template<template<unsigned, unsigned> class Sinc>
void ChainResampler::upinit(long const inRate,
                            long const outRate) {
	typedef Sinc<channels, 2048> BigSinc;
	typedef Sinc<channels,   32> SmallSinc;

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
	reallocateBuffer();
}

#endif
