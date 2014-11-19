//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef CORE_AUDIO_ENGINE_H
#define CORE_AUDIO_ENGINE_H

#include "../audioengine.h"
#include "ringbuffer.h"
#include <AudioUnit/AudioUnit.h>
#include <pthread.h>

class CoreAudioEngine : public AudioEngine {
public:
	CoreAudioEngine();
	virtual ~CoreAudioEngine();
	virtual void uninit();
	virtual int write(void *buffer, std::size_t frames);
	virtual int write(void *buffer, std::size_t samples,
	                  BufferState &preBufState_out, long &rate_out);
	virtual BufferState bufferState() const;
	virtual long rateEstimate() const;
	virtual void pause();

protected:
	virtual long doInit(long rate, int latency);

private:
	enum { unit_closed = 0, unit_opened, unit_inited };

	RingBuffer<SInt16> rbuf;
	AudioUnit outUnit;
	int outUnitState;
	pthread_mutex_t *mutex;
	pthread_cond_t *availCond;
	Float64 rateEst;
	bool running;

	static OSStatus renderProc(void *refCon, AudioUnitRenderActionFlags *inActionFlags,
	                           AudioTimeStamp const *timeStamp, UInt32 busNumber,
	                           UInt32 numFrames, AudioBufferList *ioData);
	std::size_t read(void *stream, std::size_t frames, Float64 rateScalar);
	int doWrite(void *buffer, std::size_t frames);
};

#endif
