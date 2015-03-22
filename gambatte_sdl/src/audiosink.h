//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef AUDIOSINK_H
#define AUDIOSINK_H

#include "ringbuffer.h"
#include "rateest.h"
#include "scoped_ptr.h"
#include <SDL.h>
#include <cstddef>

class AudioSink {
public:
	struct Status {
		long fromUnderrun;
		long fromOverflow;
		long rate;

		Status(long fromUnderrun, long fromOverflow, long rate)
		: fromUnderrun(fromUnderrun), fromOverflow(fromOverflow), rate(rate)
		{
		}
	};

	AudioSink(long sampleRate, int latency, int periods);
	~AudioSink();
	Status write(Sint16 const *inBuf, std::size_t samples);

private:
	struct SdlDeleter;

	RingBuffer<Sint16> rbuf_;
	RateEst rateEst_;
	scoped_ptr<SDL_mutex, SdlDeleter> const mut_;
	scoped_ptr<SDL_cond, SdlDeleter> const bufReadyCond_;
	bool const failed_;

	static void fillBuffer(void *data, Uint8 *stream, int len) {
		static_cast<AudioSink *>(data)->read(stream, len);
	}

	void read(Uint8 *stream, std::size_t len);
};

#endif
