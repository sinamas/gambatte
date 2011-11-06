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
#include "alsaengine.h"
#include <algorithm>
#include <cstdio>

AlsaEngine::AlsaEngine() :
	AudioEngine("ALSA"),
	conf("Custom PCM device:", "default", "alsaengine", "plughw"),
	pcm_handle(0),
	bufSize(0),
	prevfur(0)
{}

AlsaEngine::~AlsaEngine() {
	uninit();
}

int AlsaEngine::doInit(const int inrate, const unsigned latency) {
	unsigned rate = inrate;
	
	if (snd_pcm_open(&pcm_handle, conf.device(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		std::fprintf(stderr, "Error opening PCM device %s\n", conf.device());
		pcm_handle = 0;
		goto fail;
	}
	
	{
		snd_pcm_hw_params_t *hwparams;
		snd_pcm_hw_params_alloca(&hwparams);
		
		if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
			std::fprintf(stderr, "Can not configure this PCM device.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
			std::fprintf(stderr, "Error setting access.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16) < 0) {
			std::fprintf(stderr, "Error setting format.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, 0) < 0) {
			std::fprintf(stderr, "Error setting rate.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
			std::fprintf(stderr, "Error setting channels.\n");
			goto fail;
		}
		
		{
			unsigned ulatency = latency * 1000;
			
			if (snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &ulatency, 0) < 0) {
				std::fprintf(stderr, "Error setting buffer latency %u.\n", ulatency);
				goto fail;
			}
		}
		
		{
			unsigned val = 16;
			snd_pcm_hw_params_set_periods_max(pcm_handle, hwparams, &val, 0);
		}
		
		if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
			std::fprintf(stderr, "Error setting HW params.\n");
			goto fail;
		}
		
		{
			snd_pcm_uframes_t bSize = 0;
			
			if (snd_pcm_hw_params_get_buffer_size(hwparams, &bSize) < 0) {
				std::fprintf(stderr, "Error getting buffer size\n");
				goto fail;
			}
			
			bufSize = bSize;
		}
	}
	
	{
		snd_pcm_sw_params_t *swparams;
		snd_pcm_sw_params_alloca(&swparams);
		
		if (snd_pcm_sw_params_current(pcm_handle, swparams) < 0) {
			std::fprintf(stderr, "Error getting current swparams\n");
		} else if (snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, bufSize) < 0) {
			std::fprintf(stderr, "Error setting start threshold\n");
		} else if (snd_pcm_sw_params(pcm_handle, swparams) < 0) {
			std::fprintf(stderr, "Error setting swparams\n");
		}
	}
	
	prevfur = 0;
	est.init(rate, rate, bufSize);
	
	return rate;
	
fail:
	uninit();
	return -1;
}

void AlsaEngine::uninit() {
	if (pcm_handle)
		snd_pcm_close(pcm_handle);
	
	pcm_handle = 0;
}

int AlsaEngine::write(void *const buffer, const unsigned samples, const BufferState &bstate) {
	bool underrun = false;
	
	if (bstate.fromUnderrun == 0 || snd_pcm_state(pcm_handle) != SND_PCM_STATE_RUNNING) {
		underrun = true;
	} else if (prevfur > bstate.fromUnderrun && bstate.fromUnderrun != BufferState::NOT_SUPPORTED) {
		est.feed(prevfur - bstate.fromUnderrun);
	}
	
	prevfur = bstate.fromUnderrun + samples;
	
	for (int n = 4; n-- && snd_pcm_writei(pcm_handle, buffer, samples) < 0;) {
		snd_pcm_prepare(pcm_handle);
		underrun = true;
	}
	
	if (underrun)
		est.reset();
	
	return 0;
}

int AlsaEngine::write(void *const buffer, const unsigned samples) {
	return write(buffer, samples, bufferState());
}

int AlsaEngine::write(void *const buffer, const unsigned samples, BufferState &preBufState, long &rate) {
	const int ret = write(buffer, samples, preBufState = bufferState());
	rate = est.result();
	return ret;
}

const AudioEngine::BufferState AlsaEngine::bufferState() const {
	BufferState s;
	snd_pcm_sframes_t avail;
	
	snd_pcm_hwsync(pcm_handle);
	avail = snd_pcm_avail_update(pcm_handle);
	
	if (avail == -EPIPE)
		avail = bufSize;
	
	if (avail < 0) {
		s.fromOverflow = s.fromUnderrun = BufferState::NOT_SUPPORTED;
	} else {
		if (static_cast<unsigned>(avail) > bufSize)
			avail = bufSize;
		
		s.fromUnderrun = bufSize - avail;
		s.fromOverflow = avail;
	}
	
	return s;
}
