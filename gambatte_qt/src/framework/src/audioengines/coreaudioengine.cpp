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

#include "coreaudioengine.h"
#include <CoreServices/CoreServices.h>
#include <cmath>
#include <cstdio>

namespace {

static int createMutex(pthread_mutex_t* &mutptr) {
	mutptr = new pthread_mutex_t;
	int ret = pthread_mutex_init(mutptr, 0);
	if (ret) {
		delete mutptr;
		mutptr = 0;
	}

	return ret;
}

static void destroyMutex(pthread_mutex_t* &mutptr) {
	if (mutptr) {
		pthread_mutex_lock(mutptr);
		pthread_mutex_destroy(mutptr);
		delete mutptr;
		mutptr = 0;
	}
}

static int createCond(pthread_cond_t* &condptr) {
	condptr = new pthread_cond_t;
	int ret = pthread_cond_init(condptr, 0);
	if (ret) {
		delete condptr;
		condptr = 0;
	}

	return ret;
}

static void destroyCond(pthread_cond_t* &condptr) {
	if (condptr) {
		pthread_cond_destroy(condptr);
		delete condptr;
		condptr = 0;
	}
}

class MutexLocker : Uncopyable {
	pthread_mutex_t *const mut;

public:
	int const err;

	explicit MutexLocker(pthread_mutex_t *mut)
	: mut(mut)
	, err(pthread_mutex_lock(mut))
	{
	}

	~MutexLocker() {
		if (!err)
			pthread_mutex_unlock(mut);
	}
};

}

CoreAudioEngine::CoreAudioEngine()
: AudioEngine("CoreAudio"),
  outUnit(0),
  outUnitState(unit_closed),
  mutex(0),
  availCond(0),
  rateEst(0),
  running(false)
{
}

CoreAudioEngine::~CoreAudioEngine() {
	uninit();
}

std::size_t CoreAudioEngine::read(
		void *const stream, std::size_t frames,
		Float64 const rateScalar) {
	MutexLocker mutlock(mutex);
	if (!mutlock.err) {
		rateEst = rate() / rateScalar;

		if (frames > rbuf.used() / 2)
			frames = rbuf.used() / 2;

		rbuf.read(static_cast<SInt16 *>(stream), frames * 2);
		pthread_cond_signal(availCond);
	}

	return frames;
}

OSStatus CoreAudioEngine::renderProc(void *refCon,
                                     AudioUnitRenderActionFlags *,
                                     AudioTimeStamp const *timeStamp,
                                     UInt32 /*busNumber*/,
                                     UInt32 numFrames,
                                     AudioBufferList *ioData)
{
	ioData->mBuffers[0].mDataByteSize =
		static_cast<CoreAudioEngine *>(refCon)->read(ioData->mBuffers[0].mData,
		                                             numFrames,
		                                             timeStamp->mRateScalar) * 4;
 	return 0;
}

long CoreAudioEngine::doInit(long const rate, int const latency) {
	{
		ComponentDescription desc;
		desc.componentType = kAudioUnitType_Output;
		desc.componentSubType = kAudioUnitSubType_DefaultOutput;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;

		Component comp;
		if ((comp = FindNextComponent(0, &desc)) == 0) {
			std::fprintf(stderr, "Failed to find output unit component\n");
			return -1;
		}

		if (ComponentResult err = OpenAComponent(comp, &outUnit)) {
			std::fprintf(stderr, "Failed to open output unit component: %d\n",
			             static_cast<int>(err));
			return -1;
		}

		outUnitState = unit_opened;
	}

	if (ComponentResult err = AudioUnitInitialize(outUnit)) {
		std::fprintf(stderr, "Failed to initialize output unit component: %d\n",
		             static_cast<int>(err));
		return -1;
	}

	outUnitState = unit_inited;

	{
		AudioStreamBasicDescription desc;
		desc.mSampleRate = rate;
		desc.mFormatID = kAudioFormatLinearPCM;
		desc.mChannelsPerFrame = 2;
		desc.mBitsPerChannel = 16;
		desc.mFramesPerPacket = 1;
		desc.mBytesPerPacket = desc.mBytesPerFrame = 4;
		desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
#ifdef WORDS_BIGENDIAN
		desc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
#endif
		if (ComponentResult err =
				AudioUnitSetProperty(outUnit, kAudioUnitProperty_StreamFormat,
				                     kAudioUnitScope_Input, 0, &desc, sizeof desc)) {
			std::fprintf(stderr, "Failed to set the input format: %d\n",
			             static_cast<int>(err));
			return -1;
		}
	}

	{
		AURenderCallbackStruct renderCallback;
		renderCallback.inputProc = renderProc;
		renderCallback.inputProcRefCon = this;

		if (ComponentResult err =
				AudioUnitSetProperty(outUnit, kAudioUnitProperty_SetRenderCallback,
				                     kAudioUnitScope_Input, 0,
				                     &renderCallback, sizeof renderCallback)) {
			std::fprintf(stderr, "Failed to set render callback: %d\n",
			             static_cast<int>(err));
			return -1;
		}
	}

	if (int err = createMutex(mutex)) {
		std::fprintf(stderr, "Failed to create mutex: %d\n", err);
		return -1;
	}

	if (int err = createCond(availCond)) {
		std::fprintf(stderr, "Failed to create condition variable: %d\n", err);
		return -1;
	}

	rbuf.reset(((rate * latency + 500) / 1000) * 2);
	rbuf.fill(0);
	rateEst = rate;
	return rate;
}

void CoreAudioEngine::uninit() {
	if (outUnitState >= unit_inited)
		AudioUnitUninitialize(outUnit);
	if (outUnitState >= unit_opened)
		CloseComponent(outUnit);

	destroyMutex(mutex);
	destroyCond(availCond);
	outUnitState = unit_closed;
	running = false;
	rbuf.reset(0);
}

void CoreAudioEngine::pause() {
	if (running) {
		AudioOutputUnitStop(outUnit);
		running = false;
	}
}

int CoreAudioEngine::doWrite(void *const buffer, std::size_t samples) {
	if (!running) {
		if (ComponentResult err = AudioOutputUnitStart(outUnit)) {
			std::fprintf(stderr, "Failed to start output unit: %d\n",
			             static_cast<int>(err));
			return -1;
		}

		running = true;
	}

	SInt16 *inBuf = static_cast<SInt16 *>(buffer);
	std::size_t avail;
	while ((avail = rbuf.avail() / 2) < samples) {
		rbuf.write(inBuf, avail * 2);
		inBuf += avail * 2;
		samples -= avail;
		pthread_cond_wait(availCond, mutex);
	}

	rbuf.write(inBuf, samples * 2);
	return 0;
}

int CoreAudioEngine::write(
		void *buffer, std::size_t samples,
		BufferState &preBufState_out, long &rate_out) {
	MutexLocker mutlock(mutex);
	if (mutlock.err)
		return -1;

	preBufState_out.fromUnderrun = rbuf.used() / 2;
	preBufState_out.fromOverflow = rbuf.avail() / 2;
	rate_out = rateEst + 0.5;
	return doWrite(buffer, samples);
}

int CoreAudioEngine::write(void *buffer, std::size_t samples) {
	MutexLocker mutlock(mutex);
	if (mutlock.err)
		return -1;

	return doWrite(buffer, samples);
}

AudioEngine::BufferState CoreAudioEngine::bufferState() const {
	MutexLocker mutlock(mutex);
	BufferState bstate = { 0, 0 };
	bstate.fromUnderrun = rbuf.used() / 2;
	bstate.fromOverflow = rbuf.avail() / 2;
	return bstate;
}

long CoreAudioEngine::rateEstimate() const {
	MutexLocker mutlock(mutex);
	return rateEst + 0.5;
}
