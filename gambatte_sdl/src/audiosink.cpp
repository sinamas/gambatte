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

#include "audiosink.h"
#include <SDL_thread.h>
#include <cstdio>

namespace {

static unsigned long nearestPowerOf2(unsigned long const in) {
	unsigned long out = in;
	out |= out >> 1;
	out |= out >> 2;
	out |= out >> 4;
	out |= out >> 8;
	out |= out >> 16;
	++out;

	if (!(out >> 2 & in))
		out >>= 1;

	return out;
}

static int openAudio(long srate, std::size_t samples,
                     void (*callback)(void *userdata, Uint8 *stream, int len),
                     void *userdata)
{
	SDL_AudioSpec spec;
	spec.freq = srate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = samples;
	spec.callback = callback;
	spec.userdata = userdata;
	if (SDL_OpenAudio(&spec, 0) < 0) {
		std::fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
		return -1;
	}

	return 0;
}

class LockGuard {
public:
	explicit LockGuard(SDL_mutex *m) : m_(m) { SDL_mutexP(m); }
private:
	struct LockDeleter { static void del(SDL_mutex *m) { SDL_mutexV(m); } };
	scoped_ptr<SDL_mutex, LockDeleter> const m_;
};

} // anon ns

struct AudioSink::SdlDeleter {
	static void del(SDL_mutex *m) { SDL_DestroyMutex(m); }
	static void del(SDL_cond *c) { SDL_DestroyCond(c); }
};

AudioSink::AudioSink(long const srate, int const latency, int const periods)
: rbuf_(nearestPowerOf2(srate * latency / ((periods + 1) * 1000)) * periods * 2)
, rateEst_(srate, rbuf_.size() / periods)
, mut_(SDL_CreateMutex())
, bufReadyCond_(SDL_CreateCond())
, failed_(openAudio(srate, rbuf_.size() / 2 / periods, fillBuffer, this) < 0)
{
	rbuf_.fill(0);
}

AudioSink::~AudioSink() {
	SDL_PauseAudio(1);
	SDL_CloseAudio();
}

AudioSink::Status AudioSink::write(Sint16 const *inBuf, std::size_t samples) {
	if (failed_)
		return Status(rbuf_.size() / 2, 0, rateEst_.result());

	LockGuard lock(mut_.get());
	Status const status(rbuf_.used() / 2, rbuf_.avail() / 2, rateEst_.result());

	for (std::size_t avail; (avail = rbuf_.avail() / 2) < samples;) {
		rbuf_.write(inBuf, avail * 2);
		inBuf += avail * 2;
		samples -= avail;
		SDL_CondWait(bufReadyCond_.get(), mut_.get());
	}

	rbuf_.write(inBuf, samples * 2);
	return status;
}

void AudioSink::read(Uint8 *const stream, std::size_t const len) {
	if (failed_)
		return;

	LockGuard lock(mut_.get());
	rbuf_.read(reinterpret_cast<Sint16 *>(stream), std::min(len / 2, rbuf_.used()));
	rateEst_.feed(len / 4);
	SDL_CondSignal(bufReadyCond_.get());
}
