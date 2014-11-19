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

#include "ossengine.h"
#include "../audioengine.h"
#include "customdevconf.h"
#include "rateest.h"
#include <QObject>
#include <QtGlobal>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef Q_OS_OPENBSD
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <cmath>
#include <cstdio>

namespace {

static char const * defaultDspDevPath() {
#ifdef Q_OS_OPENBSD
	return "/dev/audio";
#else
	return "/dev/dsp";
#endif
}

class OssEngine : public AudioEngine {
public:
	OssEngine()
	: AudioEngine("OSS")
	, conf_(QObject::tr("Custom DSP device:"), defaultDspDevPath(),
	        "ossengine", defaultDspDevPath())
	, fd_(-1)
	, bufSize_(0)
	, fragSize_(0)
	, prevbytes_(0)
	{
	}

	virtual ~OssEngine() { uninit(); }

	virtual void uninit() {
		if (fd_ != -1) {
			close(fd_);
			fd_ = -1;
		}
	}

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
	virtual void pause() { prevbytes_ = 0; est_.resetLastFeedTimeStamp(); }
	virtual bool flushPausedBuffers() const { return true; }
	virtual QWidget * settingsWidget() const { return conf_.settingsWidget(); }
	virtual void rejectSettings() const { conf_.rejectSettings(); }

protected:
	virtual long doInit(long rate, int latency);
	virtual void doAcceptSettings() { conf_.acceptSettings(); }

private:
	CustomDevConf conf_;
	RateEst est_;
	int fd_;
	std::size_t bufSize_;
	std::size_t fragSize_;
	std::size_t prevbytes_;

	int write(void *buffer, std::size_t samples, BufferState const &bstate);
};

long OssEngine::doInit(long rate, int const latency) {
	if ((fd_ = open(conf_.device().toLocal8Bit().data(), O_WRONLY, 0)) == -1) {
		std::perror(conf_.device().toLocal8Bit().data());
		return -1;
	}

	{
		int channels = 2;
		if (ioctl(fd_, SNDCTL_DSP_CHANNELS, &channels) == -1) {
			std::perror("SNDCTL_DSP_CHANNELS");
			return -1;
		}
		if (channels != 2) {
			std::fputs("OssEngine: unsupported number of channels\n", stderr);
			return -1;
		}
	}

	{
		int format = AFMT_S16_NE;
		if (ioctl(fd_, SNDCTL_DSP_SETFMT, &format) == -1) {
			std::perror("SNDCTL_DSP_SETFMT");
			return -1;
		}
		if (format != AFMT_S16_NE) {
			std::fputs("OssEngine: unsupported format\n", stderr);
			return -1;
		}
	}

	{
		int speed = rate;
		if (ioctl(fd_, SNDCTL_DSP_SPEED, &speed) == -1) {
			std::perror("SNDCTL_DSP_SPEED");
			return -1;
		}

		rate = speed;
	}

	{
		int arg = 0x60000 | int(std::log(rate * latency * 4 / 6000.0) / std::log(2.0) + 0.5);
		if (ioctl(fd_, SNDCTL_DSP_SETFRAGMENT, &arg) == -1)
			std::perror("SNDCTL_DSP_SETFRAGMENT");
	}

	{
		audio_buf_info info;
		if (ioctl(fd_, SNDCTL_DSP_GETOSPACE, &info) == -1) {
			std::perror("SNDCTL_DSP_GETOSPACE");
			return -1;
		}

		bufSize_ = info.bytes >> 2;
		fragSize_ = info.fragsize >> 2;
	}

	prevbytes_ = 0;
	est_ = RateEst(rate, bufSize_);
	return rate;
}

int OssEngine::write(void *const buffer, std::size_t const samples, BufferState const &bstate) {
	if (bstate.fromUnderrun != BufferState::not_supported) {
		count_info ci;
		if (ioctl(fd_, SNDCTL_DSP_GETOPTR, &ci) != -1) {
			if (static_cast<std::size_t>(ci.bytes) > prevbytes_) {
				if (bstate.fromUnderrun > fragSize_)
					est_.feed((ci.bytes - prevbytes_) >> 2);
				else
					est_.resetLastFeedTimeStamp();
			}

			prevbytes_ = ci.bytes;
		}
	}

	if (::write(fd_, buffer, samples * 4) != static_cast<int>(samples * 4))
		return -1;

	return 0;
}

AudioEngine::BufferState OssEngine::bufferState() const {
	BufferState s;
	audio_buf_info info;
	if (ioctl(fd_, SNDCTL_DSP_GETOSPACE, &info) == -1) {
		s.fromOverflow = s.fromUnderrun = BufferState::not_supported;
	} else {
		if (info.bytes < 0)
			info.bytes = 0;
		else if (static_cast<std::size_t>(info.bytes >> 2) > bufSize_)
			info.bytes = bufSize_ << 2;

		s.fromUnderrun = bufSize_ - (info.bytes >> 2);
		s.fromOverflow = info.bytes >> 2;
	}

	return s;
}

} // anon ns

transfer_ptr<AudioEngine> createOssEngine() {
	return transfer_ptr<AudioEngine>(new OssEngine);
}
