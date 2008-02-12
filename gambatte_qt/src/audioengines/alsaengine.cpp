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

AlsaEngine::AlsaEngine() :
	AudioEngine("ALSA"),
	conf("Custom PCM device:", "hw:0,0", "alsaengine"),
	pcm_handle(NULL),
	bufSize(0)
{}

AlsaEngine::~AlsaEngine() {
	uninit();
}

int AlsaEngine::init(const int inrate, const unsigned latency) {
	unsigned rate = inrate;
	
	if (snd_pcm_open(&pcm_handle, conf.device(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		fprintf(stderr, "Error opening PCM device %s\n", conf.device());
		pcm_handle = NULL;
		goto fail;
	}
	
	{
		snd_pcm_hw_params_t *hwparams;
		snd_pcm_hw_params_alloca(&hwparams);
		
		if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
			fprintf(stderr, "Can not configure this PCM device.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
			fprintf(stderr, "Error setting access.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16) < 0) {
			fprintf(stderr, "Error setting format.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &rate, 0) < 0) {
			fprintf(stderr, "Error setting rate.\n");
			goto fail;
		}
		
		if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
			fprintf(stderr, "Error setting channels.\n");
			goto fail;
		}
		
		{
			unsigned periods = 2;
			
			if (snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, 0) < 0) {
				fprintf(stderr, "Error setting periods %u.\n", periods);
				goto fail;
			}
		}
		
		{
			snd_pcm_uframes_t bSize = (rate * latency + 500) / 1000;
			
			if (snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &bSize) < 0) {
				fprintf(stderr, "Error setting buffer size %u.\n", static_cast<unsigned>(bSize));
				goto fail;
			}
			
			bufSize = bSize;
		}
		
		if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
			fprintf(stderr, "Error setting HW params.\n");
			goto fail;
		}
	}
	
	return rate;
	
fail:
	uninit();
	return -1;
}

void AlsaEngine::uninit() {
	if (pcm_handle)
		snd_pcm_close(pcm_handle);
	
	pcm_handle = NULL;
}

int AlsaEngine::write(void *const buffer, const unsigned samples) {
	while (snd_pcm_writei(pcm_handle, buffer, samples) < 0) {
		snd_pcm_prepare(pcm_handle);
	}
	
	return 0;
}

const AudioEngine::BufferState AlsaEngine::bufferState() const {
	BufferState s;
	snd_pcm_sframes_t avail;
	
	if (snd_pcm_delay(pcm_handle, &avail)) {
		s.fromOverflow = s.fromUnderrun = BufferState::NOT_SUPPORTED;
	} else {
		if (avail < 0)
			avail = 0;
		
		s.fromUnderrun = avail;
		s.fromOverflow = bufSize - avail;
	}
	
	return s;
}
