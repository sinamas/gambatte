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

#include "openalengine.h"
#include "../audioengine.h"
#include "array.h"
#include "scoped_ptr.h"
#include <QtGlobal> // for Q_WS_ define

#ifdef Q_WS_MAC
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/alc.h>
#include <AL/al.h>
#endif

#ifdef Q_WS_WIN
#include <windows.h>
#else
#include <time.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {

static char const * errorToString(ALenum error) {
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

static std::size_t const buf_size = 1 << 9;

struct AlDeleter {
	static void del(ALCdevice *d) { if (d) { alcCloseDevice(d); } }

	static void del(ALCcontext *c) {
		if (c) {
			alcMakeContextCurrent(0);
			alcDestroyContext(c);
		}
	}
};

class OpenAlEngine : public AudioEngine {
public:
	OpenAlEngine()
	: AudioEngine("OpenAL")
	, source_(0)
	, buffers_(0)
	, bufPos_(0)
	{
	}

	virtual ~OpenAlEngine() { uninit(); }
	virtual void uninit();
	virtual int write(void *buffer, std::size_t samples);
	virtual BufferState bufferState() const;

protected:
	virtual long doInit(long rate, int latency);

private:
	Array<qint16> buf_;
	scoped_ptr<ALCdevice,  AlDeleter> device_;
	scoped_ptr<ALCcontext, AlDeleter> context_;
	ALuint source_;
	int buffers_;
	std::size_t bufPos_;

	void deleteProcessedBufs() const;
};

void OpenAlEngine::deleteProcessedBufs() const {
	ALint processed = getSourcei(source_, AL_BUFFERS_PROCESSED);
	while (processed--) {
		ALuint bid = 0;
		alSourceUnqueueBuffers(source_, 1, &bid);
		alDeleteBuffers(1, &bid);
	}
}

long OpenAlEngine::doInit(long const rate, int latency) {
	device_.reset(alcOpenDevice(0));
	if (!device_) {
		std::fprintf(stderr, "alcOpenDevice error\n");
		return -1;
	}

	context_.reset(alcCreateContext(device_.get(), 0));
	if (!context_) {
		std::fprintf(stderr, "alcCreateContext error\n");
		return -1;
	}

	alGetError();
	alcMakeContextCurrent(context_.get());

	ALenum error;
	if ((error = alGetError()) != AL_NO_ERROR) {
		std::fprintf(stderr, "alcMakeContextCurrent error: %s\n", errorToString(error));
		return -1;
	}

	alGenSources(1, &source_);
	if ((error = alGetError()) != AL_NO_ERROR) {
		std::fprintf(stderr, "alGenSources error: %s\n", errorToString(error));
		return -1;
	}

	buffers_ = rate * latency / (buf_size * 1000) + 1;
	buf_.reset(buf_size * 2);
	bufPos_ = 0;

	return rate;
}

void OpenAlEngine::uninit() {
	if (context_ && alIsSource(source_)) {
		alSourceStop(source_);

		ALint queued = getSourcei(source_, AL_BUFFERS_QUEUED);
		while (queued--) {
			ALuint bid = 0;
			alSourceUnqueueBuffers(source_, 1, &bid);
			alDeleteBuffers(1, &bid);
		}

		alDeleteSources(1, &source_);
	}

	context_.reset();
	device_.reset();
	buf_.reset();
	source_ = 0;
}

int OpenAlEngine::write(void *const data, std::size_t const samples) {
	qint16 const *src = static_cast<qint16 const *>(data);
	std::size_t total = samples + bufPos_;
	while (total >= buf_size) {
		if (getSourcei(source_, AL_BUFFERS_QUEUED) >= buffers_) {
			while (getSourcei(source_, AL_BUFFERS_PROCESSED) < 1
					&& getSourcei(source_, AL_SOURCE_STATE) == AL_PLAYING) {
#ifdef Q_WS_WIN
				Sleep(1);
#else
				timespec tspec = { 0, 500000 };
				nanosleep(&tspec, 0);
#endif
			}

			ALuint bid = 0;
			alSourceUnqueueBuffers(source_, 1, &bid);
			alDeleteBuffers(1, &bid);
		}

		std::memcpy(buf_ + bufPos_ * 2, src, (buf_size - bufPos_) * 2 * sizeof *buf_);
		src += (buf_size - bufPos_) * 2;
		total -= buf_size;
		bufPos_ = 0;

		ALuint bufid = 0;
		alGenBuffers(1, &bufid);
		alBufferData(bufid, AL_FORMAT_STEREO16, buf_, buf_size * 2 * sizeof *buf_, rate());
		alSourceQueueBuffers(source_, 1, &bufid);
	}

	std::memcpy(buf_ + bufPos_ * 2, src, (total - bufPos_) * 2 * sizeof *buf_);
	bufPos_ = total;

	if (getSourcei(source_, AL_SOURCE_STATE) != AL_PLAYING)
		alSourcePlay(source_);

	return 0;
}

AudioEngine::BufferState OpenAlEngine::bufferState() const {
	deleteProcessedBufs();

	ALint const queued = getSourcei(source_, AL_BUFFERS_QUEUED);
	BufferState s = { 0, 0 };
	s.fromUnderrun = queued > 0 ? queued * buf_size - buf_size / 2 : 0;
	s.fromUnderrun += bufPos_;
	s.fromUnderrun = std::min<std::size_t>(s.fromUnderrun, (buffers_ + 1) * buf_size);
	s.fromOverflow = (buffers_ + 1) * buf_size - s.fromUnderrun;

	return s;
}

} // anon ns

transfer_ptr<AudioEngine> createOpenAlEngine() {
	return transfer_ptr<AudioEngine>(new OpenAlEngine);
}
