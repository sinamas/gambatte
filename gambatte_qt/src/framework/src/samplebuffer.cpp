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
#include "samplebuffer.h"
#include "mediasource.h"
#include "resample/resamplerinfo.h"
#include <cstring>

template<class T>
static T * ptr_cast(void *p) { return static_cast<T *>(p); }

void SampleBuffer::reset() {
	const long insrate = static_cast<long>(ft_.reciprocal().toFloat() * spf_.toFloat() + 0.5f);
	const long maxin = spf_.ceiled() + source_->overupdate;

	sndInBuffer_.reset(0);
	resampler_.reset();
	samplesBuffered_ = 0;

	if (insrate > 0 && outsrate_ > 0) {
		sndInBuffer_.reset(maxin);
		resampler_.reset(ResamplerInfo::get(resamplerNo_).create(insrate, outsrate_, maxin));
	}
}

long SampleBuffer::update(const PixelBuffer &pb) {
	long insamples = sndInBuffer_.size() - samplesBuffered_;
	const long res = source_->update(pb, ptr_cast<qint16>(sndInBuffer_ + samplesBuffered_),
	                                 insamples);
	samplesBuffered_ += insamples;
	return res < 0 ? res : samplesBuffered_ - insamples + res;
}

long SampleBuffer::read(const long insamples, qint16 *const out, const bool alwaysResample) {
	long outsamples = 0;
	samplesBuffered_ -= insamples;

	if (out) {
		if (resampler_->inRate() == resampler_->outRate() && !alwaysResample) {
			std::memcpy(out, sndInBuffer_, insamples * sizeof *sndInBuffer_);
			outsamples = insamples;
		} else {
			outsamples = resampler_->resample(out, ptr_cast<qint16>(sndInBuffer_),
			                                  insamples);
		}
	}

	std::memmove(sndInBuffer_, sndInBuffer_ + insamples,
	             samplesBuffered_ * sizeof *sndInBuffer_);
	return outsamples;
}
