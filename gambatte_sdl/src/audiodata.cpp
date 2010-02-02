/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include "audiodata.h"
#include <SDL_thread.h>
#include <cstdio>

/*static unsigned ceiledPowerOf2(unsigned t) {
	--t;
	t |= t >> 1;
	t |= t >> 2;
	t |= t >> 4;
	t |= t >> 8;
	t |= t >> 16;
	++t;
	
	return t;
}*/

static unsigned nearestPowerOf2(const unsigned in) {
	unsigned out = in;
	
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

AudioData::AudioData(const unsigned srate, const unsigned latency, const unsigned periods) :
rbuf(nearestPowerOf2(srate * latency / ((periods + 1) * 1000)) * periods * 2),
rateEst(srate),
mut(SDL_CreateMutex()),
bufReadyCond(SDL_CreateCond()),
failed(false) {
	rbuf.fill(0);
	
	SDL_AudioSpec spec;
	spec.freq = srate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = (rbuf.size() / 2) / periods;
	spec.callback = fill_buffer;
	spec.userdata = this;
	
	if (SDL_OpenAudio(&spec, NULL) < 0) {
		std::fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		failed = true;
	}
}

AudioData::~AudioData() {
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	SDL_DestroyCond(bufReadyCond);
	SDL_DestroyMutex(mut);
}

const AudioData::Status AudioData::write(const Sint16 *inBuf, unsigned samples) {
	if (failed) {
		const Status status = { rbuf.size() >> 1, 0, rateEst.result() };
		return status;
	}
	
	SDL_mutexP(mut);
	const Status status = { rbuf.used() / 2, rbuf.avail() / 2, rateEst.result() };
	
	{
		std::size_t avail;
		
		while ((avail = rbuf.avail() / 2) < samples) {
			rbuf.write(inBuf, avail * 2);
			inBuf += avail * 2;
			samples -= avail;
			SDL_CondWait(bufReadyCond, mut);
		}
	}
	
	rbuf.write(inBuf, samples * 2);
	SDL_mutexV(mut);
	
	return status;
}

void AudioData::read(Uint8 *const stream, const int len) {
	if (failed)
		return;
	
	SDL_mutexP(mut);
	
	rbuf.read(reinterpret_cast<Sint16*>(stream), std::min(static_cast<std::size_t>(len) / 2, rbuf.used()));
	rateEst.feed(len / 4);
	
	SDL_CondSignal(bufReadyCond);
	
	SDL_mutexV(mut);
}
