/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

class MediaSource;
struct PixelBuffer;

#include <memory>
#include <QtGlobal>
#include <array.h>
#include <resample/resampler.h>
#include "rational.h"
#include "uncopyable.h"

class SampleBuffer : Uncopyable {
	MediaSource *const source_;
	std::auto_ptr<Resampler> resampler;
	Array<qint16> sndInBuffer;
	long samplesBuffered_;
	Rational spf_;
	Rational ft_;
	int outsrate;
	unsigned resamplerNo_;
	
	unsigned size() const { return sndInBuffer.size() >> 1; }
	void reset();
	
public:
	explicit SampleBuffer(MediaSource *source) : source_(source), spf_(0), ft_(1, 0), outsrate(0), resamplerNo_(1) { reset(); }
	long update(const PixelBuffer &pb);
	long read(long insamples, qint16 *out, bool alwaysResample);
	long samplesBuffered() const { return samplesBuffered_; }
	void setSpf(const Rational &spf) { spf_ = spf; reset(); }
	void setFt(const Rational &ft) { ft_ = ft; reset(); }
	void setResampler(const unsigned resamplerNo) { this->resamplerNo_ = resamplerNo; reset(); }
	void setOutSampleRate(const int outsrate) { this->outsrate = outsrate; reset(); }
	long maxOut() const { return resampler.get() ? resampler->maxOut(size()) : 0; }
	MediaSource* source() { return source_; }
	const MediaSource* source() const { return source_; }
	const Rational& spf() const { return spf_; }
	const Rational& ft() const { return ft_; }
	long resamplerOutRate() const { return resampler->outRate(); }
	void adjustResamplerOutRate(long outRate) { resampler->adjustRate(resampler->inRate(), outRate); }
	unsigned resamplerNo() const { return resamplerNo_; }
};

#endif
