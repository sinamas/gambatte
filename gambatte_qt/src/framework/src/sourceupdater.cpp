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

#include "sourceupdater.h"
#include "mediasource.h"
#include "resample/resamplerinfo.h"
#include <cstring>

template<class T>
static T * ptr_cast(void *p) { return static_cast<T *>(p); }

void SourceUpdater::reset() {
	long const insrate = static_cast<long>(ft_.reciprocal().toFloat() * spf_.toFloat() + 0.5f);
	std::size_t const maxin = spf_.ceiled() + source_.overUpdate;

	sndInBuffer_.reset(0);
	resampler_.reset();
	samplesBuffered_ = 0;

	if (insrate > 0 && outsrate_ > 0) {
		sndInBuffer_.reset(maxin);
		resampler_.reset(ResamplerInfo::get(resamplerNo_).create(insrate, outsrate_, maxin));
	}
}

std::ptrdiff_t SourceUpdater::update(PixelBuffer const &pb) {
	std::size_t updateSamples = sndInBuffer_.size() - samplesBuffered_;
	std::ptrdiff_t vidFrameDoneSampleNo =
		source_.update(pb, ptr_cast<qint16>(sndInBuffer_ + samplesBuffered_),
		               updateSamples);
	samplesBuffered_ += updateSamples;
	return vidFrameDoneSampleNo >= 0
	     ? std::ptrdiff_t(samplesBuffered_ - updateSamples + vidFrameDoneSampleNo)
	     : -1;
}

std::size_t SourceUpdater::readSamples(
		qint16 *const out, std::size_t const insamples, bool const alwaysResample) {
	std::size_t outsamples = 0;
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
