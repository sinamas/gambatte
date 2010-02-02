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
#include "samplebuffer.h"
#include "resample/resamplerinfo.h"
#include "resample/resampler.h"
#include "../mediasource.h"
#include <cstring>

void SampleBuffer::reset() {
	const long insrate = static_cast<long>(ft_.reciprocal().toFloat() * spf_.toFloat() + 0.5f);
	const long maxin = spf_.ceiled() + source_->overupdate;
	
	sndInBuffer.reset(0);
	resampler.reset();
	samplesBuffered_ = 0;
	
	if (insrate > 0 && outsrate > 0) {
		sndInBuffer.reset(maxin * 2);
		resampler.reset(ResamplerInfo::get(resamplerNo_).create(insrate, outsrate, maxin));
	}
}

long SampleBuffer::update(const PixelBuffer &pb) {
	long insamples = size() - samplesBuffered_;
	const long res = source_->update(pb, sndInBuffer + samplesBuffered_ * 2, insamples);
	samplesBuffered_ += insamples;
	return res < 0 ? res : samplesBuffered_ - insamples + res;
}

long SampleBuffer::read(const long insamples, qint16 *const out) {
	long outsamples;
	samplesBuffered_ -= insamples;
	
	if (out) {
		if (resampler->inRate() == resampler->outRate()) {
			std::memcpy(out, sndInBuffer, insamples * sizeof(qint16) * 2);
			outsamples = insamples;
		} else
			outsamples = resampler->resample(out, sndInBuffer, insamples);
	}
	
	std::memmove(sndInBuffer, sndInBuffer + insamples * 2, samplesBuffered_ * sizeof(qint16) * 2);
	return outsamples;
}
