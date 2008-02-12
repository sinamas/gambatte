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
#include "ossengine.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <algorithm>
#include <cmath>

OssEngine::OssEngine() :
AudioEngine("OSS"),
conf("Custom DSP device:", "/dev/dsp", "ossengine"),
audio_fd(-1),
bufSize(0)
{}

OssEngine::~OssEngine() {
	uninit();
}

int OssEngine::init(int speed, const unsigned latency) {
	if ((audio_fd = open(conf.device(), O_WRONLY, 0)) == -1) {
		perror(conf.device());
		goto fail;
	}
	
	{
		int format = AFMT_S16_NE;
		
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1) {
			perror("SNDCTL_DSP_SETFMT");
			goto fail;
		}
		
		if (format != AFMT_S16_NE) {
			printf("oss: unsupported format\n");
			goto fail;
		}
	}
	
	{
		int channels = 2;
		
		if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
			perror("SNDCTL_DSP_CHANNELS");
			goto fail;
		}
		
		if (channels != 2) {
			printf("oss: unsupported number of channels\n");
			goto fail;
		}
	}
	
	if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed) == -1) {
		perror("SNDCTL_DSP_SPEED");
		goto fail;
	}
	
	{
		int arg = 0x00040000 | std::min(static_cast<int>(log2(static_cast<double>(speed * latency + 500) / 1000.0) + 0.5), 0xFFFF);
		
// 		const int bytes = (((speed * 4389) / 262144) + 1) * 4;
// 		int arg = 0x00040000 | std::min(static_cast<int>(log2(static_cast<float>(bytes))) + 1, 0xFFFF);
		
		if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &arg) == -1) {
			perror("SNDCTL_DSP_SETFRAGMENT");
// 			goto fail;
		}
	}
	
	{
		audio_buf_info info;
		
		if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info) == -1) {
			perror("SNDCTL_DSP_GETOSPACE");
			goto fail;
		}
		
		bufSize = info.fragstotal * info.fragsize >> 2;
	}
	
	return speed;
	
fail:
	uninit();
	return -1;
}

void OssEngine::uninit() {
	if (audio_fd != -1)
		close(audio_fd);
	
	audio_fd = -1;
}

int OssEngine::write(void *const buffer, const unsigned samples) {
	if (::write(audio_fd, buffer, samples * 4) != static_cast<int>(samples * 4))
		return -1;
	
	return 0;
}

const AudioEngine::BufferState OssEngine::bufferState() const {
	BufferState s;
	audio_buf_info info;
	
	if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info) == -1 || info.bytes < 0) {
		s.fromOverflow = s.fromUnderrun = BufferState::NOT_SUPPORTED;
	} else {
		s.fromUnderrun = bufSize - (info.bytes >> 2);
		s.fromOverflow = info.bytes >> 2;
	}
	
	return s;
}
