/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

class MediaSource;
struct PixelBuffer;

#include "array.h"
#include "rational.h"
#include "resample/resampler.h"
#include "scoped_ptr.h"
#include <QtGlobal>

class SampleBuffer : Uncopyable {
	MediaSource *const source_;
	scoped_ptr<Resampler> resampler_;
	Array<quint32> sndInBuffer_;
	long samplesBuffered_;
	Rational spf_;
	Rational ft_;
	long outsrate_;
	std::size_t resamplerNo_;

	void reset();

public:
	explicit SampleBuffer(MediaSource *source)
	: source_(source)
	, spf_(0)
	, ft_(1, 0)
	, outsrate_(0)
	, resamplerNo_(1)
	{
		reset();
	}

	long update(const PixelBuffer &pb);
	long read(long insamples, qint16 *out, bool alwaysResample);
	long samplesBuffered() const { return samplesBuffered_; }
	void setSpf(const Rational &spf) { spf_ = spf; reset(); }
	void setFt(const Rational &ft) { ft_ = ft; reset(); }

	void setOutSampleRate(long outsrate, std::size_t resamplerNo) {
		outsrate_ = outsrate;
		resamplerNo_ = resamplerNo;
		reset();
	}

	void setOutSampleRate(long outsrate) { setOutSampleRate(outsrate, resamplerNo_); }
	long maxOut() const { return resampler_ ? resampler_->maxOut(sndInBuffer_.size()) : 0; }
	MediaSource* source() { return source_; }
	const MediaSource* source() const { return source_; }
	const Rational& spf() const { return spf_; }
	const Rational& ft() const { return ft_; }
	long resamplerOutRate() const { return resampler_->outRate(); }
	void adjustResamplerOutRate(long outRate) { resampler_->adjustRate(resampler_->inRate(), outRate); }
};

#endif
