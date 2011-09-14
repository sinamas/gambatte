/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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
#include "openalengine.h"
#include <QtGlobal> // for Q_WS_WIN define
#include <cstdio>

#ifdef Q_WS_WIN
#include <windows.h>
#else
#include <time.h>
#endif

/*template<typename T>
class RingBuffer {
	T *const buf;
	const std::size_t sz;
	std::size_t rpos;
	std::size_t wpos;
	
public:
	RingBuffer(const std::size_t sz_in) : buf(new T[sz_in + 1]), sz(sz_in + 1), rpos(0), wpos(0) {}
	~RingBuffer() { delete []buf; }
	
	void clear() {
		wpos = rpos = 0;
	}
	
	void read(T *out, std::size_t num);
	
	std::size_t space() const {
		return (wpos < rpos ? 0 : sz) + rpos - wpos - 1;
	}
	
	void write(const T *in, std::size_t num);
};

template<typename T>
void RingBuffer<T>::read(T *out, std::size_t num) {
	if (rpos + num > sz) {
		const std::size_t n = sz - rpos;
		
		std::memcpy(out, buf + rpos, n * sizeof(T));
		
		rpos = 0;
		num -= n;
		out += n;
	}
	
	std::memcpy(out, buf + rpos, num * sizeof(T));
	
	if ((rpos += num) == sz)
		rpos = 0;
}

template<typename T>
void RingBuffer<T>::write(const T *in, std::size_t num) {
	if (wpos + num > sz) {
		const std::size_t n = sz - wpos;
		
		std::memcpy(buf + wpos, in, n * sizeof(T));
		
		wpos = 0;
		num -= n;
		in += n;
	}
	
	std::memcpy(buf + wpos, in, num * sizeof(T));
	
	if ((wpos += num) == sz)
		wpos = 0;
}*/

static const char* errorToString(ALenum error) {
	switch (error) {
	case AL_NO_ERROR: return "no error";
	case AL_INVALID_NAME: return "invalid name";
	case AL_INVALID_ENUM: return "invalid enum";
	case AL_INVALID_VALUE: return "invalid value";
	case AL_INVALID_OPERATION: return "invalid operation";
	case AL_OUT_OF_MEMORY: return "out of memory";
	}
	
	return "";
}

static ALint getSourcei(ALuint source, ALenum pname) {
	ALint value = 0;
	
	alGetSourcei(source, pname, &value);
	
	return value;
}

static const unsigned BUF_POW = 9;
static const unsigned BUF_SZ = 1 << BUF_POW;

OpenAlEngine::OpenAlEngine() :
AudioEngine("OpenAL"),
buf(NULL),
device(NULL),
context(NULL),
source(0),
buffers(0),
bufPos(0) {
}

OpenAlEngine::~OpenAlEngine() {
	uninit();
}

void OpenAlEngine::deleteProcessedBufs() const {
	ALint processed = getSourcei(source, AL_BUFFERS_PROCESSED);
	
	while (processed--) {
		ALuint bid = 0;
		
		alSourceUnqueueBuffers(source, 1, &bid);
		alDeleteBuffers(1, &bid);
	}
}

int OpenAlEngine::doInit(const int rate, unsigned latency) {
	class FailureCleaner {
		OpenAlEngine *const engine;
		
	public:
		bool failed;
		
		FailureCleaner(OpenAlEngine *engine) : engine(engine), failed(true) {}
		
		~FailureCleaner() {
			if (failed)
				engine->uninit();
		}
	} failureCleaner(this);
	
	ALenum error;
	
	if (!(device = alcOpenDevice(NULL))) {
		std::fprintf(stderr, "alcOpenDevice error\n");
		return -1;
	}
	
	if (!(context = alcCreateContext(device, NULL))) {
		std::fprintf(stderr, "alcCreateContext error\n");
		return -1;
	}
	
	alGetError();
	alcMakeContextCurrent(context);
	
	if ((error = alGetError()) != AL_NO_ERROR) {
		std::fprintf(stderr, "alcMakeContextCurrent error: %s\n", errorToString(error));
		return -1;
	}
	
	alGenSources (1, &source);
	
	if ((error = alGetError()) != AL_NO_ERROR) {
		std::fprintf(stderr, "alGenSources error: %s\n", errorToString(error));
		return -1;
	}
	
	failureCleaner.failed = false;
	buffers = rate * latency / (BUF_SZ * 1000);
	++buffers;
// 	std::printf("buffers: %u\n", buffers);
	buf = new qint16[BUF_SZ * 2];
	bufPos = 0;
	
	return rate;
}

void OpenAlEngine::uninit() {
	if (context) {
		if (alIsSource(source)) {
			alSourceStop(source);
			
			ALint queued = getSourcei(source, AL_BUFFERS_QUEUED);
			
			while (queued--) {
				ALuint bid = 0;
				
				alSourceUnqueueBuffers(source, 1, &bid);
				alDeleteBuffers(1, &bid);
			}
			
			alDeleteSources(1, &source);
		}
		
		alcMakeContextCurrent(NULL);
		alcDestroyContext(context);
	}
	
	if (device)
		alcCloseDevice(device);
	
	delete []buf;
	
	buf = NULL;
	context = NULL;
	device = NULL;
	source = 0;
}

int OpenAlEngine::write(void *const data, const unsigned samples) {
	{
		unsigned total = samples + bufPos;
		const qint16 *src = static_cast<const qint16*>(data);
		
		while (total >= BUF_SZ) {
			if (getSourcei(source, AL_BUFFERS_QUEUED) >= static_cast<int>(buffers)) {
				while (getSourcei(source, AL_BUFFERS_PROCESSED) < 1  && getSourcei(source, AL_SOURCE_STATE) == AL_PLAYING) {
#ifdef Q_WS_WIN
					Sleep(1);
#else
					timespec tspec = { 0, 500000 };
					nanosleep(&tspec, NULL);
#endif
				}
				
				ALuint bid = 0;
				alSourceUnqueueBuffers(source, 1, &bid);
				alDeleteBuffers(1, &bid);
			}
			
			std::memcpy(buf + bufPos * 2, src, (BUF_SZ - bufPos) * sizeof(qint16) * 2);
			src += (BUF_SZ - bufPos) * 2;
			total -= BUF_SZ;
			bufPos = 0;
			
			{
				ALuint bufid = 0;
				alGenBuffers(1, &bufid);
				alBufferData(bufid, AL_FORMAT_STEREO16, buf, BUF_SZ * sizeof(qint16) * 2, rate());
				alSourceQueueBuffers(source, 1, &bufid);
			}
		}
		
		std::memcpy(buf + bufPos * 2, src, (total - bufPos) * sizeof(qint16) * 2);
		
		bufPos = total;
	}
	
	if (getSourcei(source, AL_SOURCE_STATE) != AL_PLAYING) {
		//std::printf("UNDERRUN!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		alSourcePlay(source);
	}
	
	//if (alGetError() != AL_NO_ERROR)
	//	return -1;
	
	return 0;
}

const AudioEngine::BufferState OpenAlEngine::bufferState() const {
	deleteProcessedBufs();
	
	unsigned fromUnderrun = 0;
	
	{
		const ALint queued = getSourcei(source, AL_BUFFERS_QUEUED);
	
		if (queued > 0)
			fromUnderrun = queued * BUF_SZ - (BUF_SZ >> 1);
	}
	
	fromUnderrun += bufPos;
	
	const BufferState s = { fromUnderrun: fromUnderrun, fromOverflow: fromUnderrun > (buffers + 1) * BUF_SZ ? 0 : (buffers + 1) * BUF_SZ - fromUnderrun };
	
	//std::printf("fromUnderrun: %u\n", s.fromUnderrun);
	
	return s;
}
