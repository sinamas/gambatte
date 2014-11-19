//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SOURCEUPDATER_H
#define SOURCEUPDATER_H

#include "array.h"
#include "rational.h"
#include "resample/resampler.h"
#include "scoped_ptr.h"
#include <QtGlobal>

class MediaSource;
struct PixelBuffer;

class SourceUpdater {
public:
	explicit SourceUpdater(MediaSource &source)
	: source_(source)
	, samplesBuffered_(0)
	, spf_(0)
	, ft_(1, 0)
	, outsrate_(0)
	, resamplerNo_(1)
	{
		reset();
	}

	std::ptrdiff_t update(PixelBuffer const &pb);
	std::size_t readSamples(qint16 *out, std::size_t insamples, bool alwaysResample);
	std::size_t samplesBuffered() const { return samplesBuffered_; }
	void setSpf(Rational const &spf) { spf_ = spf; reset(); }
	void setFt(Rational const &ft) { ft_ = ft; reset(); }

	void setOutSampleRate(long outsrate, std::size_t resamplerNo) {
		outsrate_ = outsrate;
		resamplerNo_ = resamplerNo;
		reset();
	}

	void setOutSampleRate(long outsrate) { setOutSampleRate(outsrate, resamplerNo_); }
	std::size_t maxOut() const { return resampler_ ? resampler_->maxOut(sndInBuffer_.size()) : 0; }
	MediaSource & source() const { return source_; }
	Rational spf() const { return spf_; }
	Rational ft() const { return ft_; }
	long resamplerOutRate() const { return resampler_->outRate(); }
	void adjustResamplerOutRate(long outRate) { resampler_->adjustRate(resampler_->inRate(), outRate); }

private:
	MediaSource &source_;
	scoped_ptr<Resampler> resampler_;
	Array<quint32> sndInBuffer_;
	std::size_t samplesBuffered_;
	Rational spf_;
	Rational ft_;
	long outsrate_;
	std::size_t resamplerNo_;

	void reset();
};

#endif
