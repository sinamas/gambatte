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

#include "alsaengine.h"
#include "../audioengine.h"
#include "customdevconf.h"
#include "rateest.h"
#include "scoped_ptr.h"
#include <QObject>
#include <alsa/asoundlib.h>
#include <cstdio>

namespace {

struct PcmDeleter {
	static void del(snd_pcm_t *pcm) {
		if (pcm)
			snd_pcm_close(pcm);
	}
};

class AlsaEngine : public AudioEngine {
public:
	AlsaEngine()
	: AudioEngine("ALSA")
	, conf_(QObject::tr("Custom PCM device:"), "default", "alsaengine", "plughw")
	, bufSize_(0)
	, prevfur_(0)
	{
	}

	virtual void uninit() { pcm_.reset(); }

	virtual int write(void *buffer, std::size_t samples) {
		return write(buffer, samples, bufferState());
	}

	virtual int write(void *buffer, std::size_t samples, BufferState &preBufState, long &rate) {
		int ret = write(buffer, samples, preBufState = bufferState());
		rate = est_.result();
		return ret;
	}

	virtual long rateEstimate() const { return est_.result(); }
	virtual BufferState bufferState() const;
	virtual void pause() { prevfur_ = 0; est_.resetLastFeedTimeStamp(); }
	virtual bool flushPausedBuffers() const { return true; }
	virtual QWidget * settingsWidget() const { return conf_.settingsWidget(); }
	virtual void rejectSettings() const { conf_.rejectSettings(); }

protected:
	virtual long doInit(long rate, int latency);
	virtual void doAcceptSettings() { conf_.acceptSettings(); }

private:
	CustomDevConf conf_;
	RateEst est_;
	scoped_ptr<snd_pcm_t, PcmDeleter> pcm_;
	snd_pcm_uframes_t bufSize_;
	snd_pcm_uframes_t prevfur_;

	int write(void *buffer, snd_pcm_uframes_t samples, BufferState const &bstate);
};

static transfer_ptr<snd_pcm_t, PcmDeleter> openPcm(CustomDevConf const &conf) {
	snd_pcm_t *p = 0;
	if (snd_pcm_open(&p, conf.device().toLocal8Bit().data(), SND_PCM_STREAM_PLAYBACK, 0) < 0) {
		std::fprintf(stderr, "Error opening PCM device %s\n",
		             conf.device().toLocal8Bit().data());
		return transfer_ptr<snd_pcm_t, PcmDeleter>();
	}

	return transfer_ptr<snd_pcm_t, PcmDeleter>(p);
}

long AlsaEngine::doInit(long const inrate, int const latency) {
	unsigned rate = inrate;
	transfer_ptr<snd_pcm_t, PcmDeleter> pcm = openPcm(conf_);
	if (!pcm)
		return -1;

	{
		snd_pcm_hw_params_t *hwparams;
		snd_pcm_hw_params_alloca(&hwparams);
		if (snd_pcm_hw_params_any(pcm.get(), hwparams) < 0) {
			std::fprintf(stderr, "Cannot configure this PCM device.\n");
			return -1;
		}

		if (snd_pcm_hw_params_set_access(pcm.get(), hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
			std::fprintf(stderr, "Error setting access.\n");
			return -1;
		}
		if (snd_pcm_hw_params_set_format(pcm.get(), hwparams, SND_PCM_FORMAT_S16) < 0) {
			std::fprintf(stderr, "Error setting format.\n");
			return -1;
		}
		if (snd_pcm_hw_params_set_rate_near(pcm.get(), hwparams, &rate, 0) < 0) {
			std::fprintf(stderr, "Error setting rate.\n");
			return -1;
		}
		if (snd_pcm_hw_params_set_channels(pcm.get(), hwparams, 2) < 0) {
			std::fprintf(stderr, "Error setting channels.\n");
			return -1;
		}

		{
			unsigned ulatency = latency * 1000;
			if (snd_pcm_hw_params_set_buffer_time_near(pcm.get(), hwparams, &ulatency, 0) < 0) {
				std::fprintf(stderr, "Error setting buffer latency %u.\n", ulatency);
				return -1;
			}
		}

		{
			unsigned val = 16;
			snd_pcm_hw_params_set_periods_max(pcm.get(), hwparams, &val, 0);
		}

		if (snd_pcm_hw_params(pcm.get(), hwparams) < 0) {
			std::fprintf(stderr, "Error setting HW params.\n");
			return -1;
		}

		{
			snd_pcm_uframes_t bSize = 0;
			if (snd_pcm_hw_params_get_buffer_size(hwparams, &bSize) < 0) {
				std::fprintf(stderr, "Error getting buffer size\n");
				return -1;
			}

			bufSize_ = bSize;
		}
	}

	{
		snd_pcm_sw_params_t *swparams;
		snd_pcm_sw_params_alloca(&swparams);
		if (snd_pcm_sw_params_current(pcm.get(), swparams) < 0) {
			std::fprintf(stderr, "Error getting current swparams\n");
		} else if (snd_pcm_sw_params_set_start_threshold(pcm.get(), swparams, bufSize_) < 0) {
			std::fprintf(stderr, "Error setting start threshold\n");
		} else if (snd_pcm_sw_params(pcm.get(), swparams) < 0) {
			std::fprintf(stderr, "Error setting swparams\n");
		}
	}

	prevfur_ = 0;
	est_ = RateEst(rate, bufSize_);
	pcm_ = pcm;
	return rate;
}

int AlsaEngine::write(void *const buffer, snd_pcm_uframes_t const samples, BufferState const &bstate) {
	bool underrun = false;
	if (bstate.fromUnderrun == 0
			|| snd_pcm_state(pcm_.get()) != SND_PCM_STATE_RUNNING) {
		underrun = true;
	} else if (prevfur_ > bstate.fromUnderrun
			&& bstate.fromUnderrun != BufferState::not_supported) {
		est_.feed(prevfur_ - bstate.fromUnderrun);
	}

	prevfur_ = bstate.fromUnderrun + samples;

	for (int n = 4; n-- && snd_pcm_writei(pcm_.get(), buffer, samples) < 0;) {
		snd_pcm_prepare(pcm_.get());
		underrun = true;
	}

	if (underrun)
		est_.resetLastFeedTimeStamp();

	return 0;
}

AudioEngine::BufferState AlsaEngine::bufferState() const {
	snd_pcm_hwsync(pcm_.get());

	snd_pcm_sframes_t avail = snd_pcm_avail_update(pcm_.get());
	if (avail == -EPIPE)
		avail = bufSize_;

	BufferState s;
	if (avail < 0) {
		s.fromOverflow = s.fromUnderrun = BufferState::not_supported;
	} else {
		if (static_cast<snd_pcm_uframes_t>(avail) > bufSize_)
			avail = bufSize_;

		s.fromUnderrun = bufSize_ - avail;
		s.fromOverflow = avail;
	}

	return s;
}

} // anon ns

transfer_ptr<AudioEngine> createAlsaEngine() {
	return transfer_ptr<AudioEngine>(new AlsaEngine);
}
