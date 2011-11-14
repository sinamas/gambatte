/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#include "mediaworker.h"
#include "joysticklock.h"
#include "SDL_joystick.h"
#include "mediasource.h"
#include "audioengine.h"
#include "skipsched.h"
#include "mmpriority.h"
#include <QtGlobal> // for Q_WS_WIN define

#ifdef Q_WS_WIN
#include <Objbase.h> // For CoInitialize
#endif

MediaWorker::MeanQueue::MeanQueue(const long mean, const long var) : q(sz, Elem(mean, var)) { sum = mean * sz; dsum = var * sz; }

void MediaWorker::MeanQueue::reset(const long mean, const long var) {
	for (q_type::iterator it = q.begin(); it != q.end(); ++it)
		*it = Elem(mean, var);

	sum = mean * sz;
	dsum = var * sz;
}

void MediaWorker::MeanQueue::push(const long i) {
	const Elem newelem(i, std::abs(i - mean()));
	const Elem &oldelem = q.front();
	sum += newelem.sumpart - oldelem.sumpart;
	dsum += newelem.dsumpart - oldelem.dsumpart;
	q.pop_front();
	q.push_back(newelem);
}

class MediaWorker::AudioOut : Uncopyable {
	AudioEngine &ae_;
	const long rate_;
	const unsigned latency_;
	
	long estrate_;
	bool inited_;
	
public:
	AudioOut(AudioEngine &ae, long rate, unsigned latency)
	: ae_(ae), rate_(rate), latency_(latency), estrate_(rate), inited_(false)
	{
	}
	
	~AudioOut() {
		uninit();
	}
	
	void init() {
		inited_ = true;
		ae_.init(rate_, latency_);
		estrate_ = rate();
	}
	
	void uninit() {
		if (inited_) {
			ae_.uninit();
			inited_ = false;
		}
	}
	
	void pause() {
		if (successfullyInitialized())
			ae_.pause();
	}

	bool flushPausedBuffers() const { return ae_.flushPausedBuffers(); }
	long rate() const { return ae_.rate() > 0 ? ae_.rate() : rate_; }
	long estimatedRate() const { return estrate_; }
	bool initialized() const { return inited_; }
	bool successfullyInitialized() const { return inited_ && ae_.rate() > 0; }
	
	int write(qint16 *buf, std::size_t samples, AudioEngine::BufferState &preBstateOut) {
		return ae_.write(buf, samples, preBstateOut, estrate_);
	}
	
	int write(qint16 *buf, std::size_t samples) {
		return ae_.write(buf, samples);
	}
};

void MediaWorker::PauseVar::unpause(const unsigned bits) {
	mut.lock();

	if (var && !(var &= ~bits))
		cond.wakeAll();

	mut.unlock();
}

void MediaWorker::PauseVar::waitWhilePaused(MediaWorker::Callback *const cb, AudioOut &ao) {
	mut.lock();
	waiting = true;
	callq.pop_all();

	if (var) {
		if (var & 1)
			ao.pause();

		cb->paused();

		do {
			cond.wait(&mut);
			callq.pop_all();
		} while (var);
	}

	waiting = false;
	mut.unlock();
}

MediaWorker::MediaWorker(MediaSource *source, AudioEngine *ae, int aerate,
		int aelatency, std::auto_ptr<Callback> callback, QObject *parent)
: QThread(parent),
  callback(callback),
  meanQueue(0, 0),
  frameTimeEst(0),
  doneVar(true),
  sampleBuffer(source),
  ao_(new AudioOut(*ae, aerate, aelatency)),
  usecft(0)
{
}

void MediaWorker::start() {
	if (AtomicVar<bool>::ConstLocked(doneVar).get()) {
		wait();

		AtomicVar<bool>::Locked(doneVar).set(false);
		pauseVar.unwait();

		QThread::start();
	}
}

void MediaWorker::stop() {
	AtomicVar<bool>::Locked(doneVar).set(true);
	pauseVar.unpause(~0U);
	wait();
	pauseVar.rewait();
}

void MediaWorker::pause() {
	pauseVar.pause(PauseVar::PAUSE_BIT);

	if (pauseVar.waitingForUnpause())
		ao_->pause();
}

void MediaWorker::initAudioEngine() {
	ao_->init();
	sampleBuffer.setOutSampleRate(ao_->rate());
	sndOutBuffer.reset(sampleBuffer.maxOut() * 2);
	meanQueue.reset(ao_->rate(), ao_->rate() >> 12);

	if (!ao_->successfullyInitialized()) {
		pauseVar.localPause(PauseVar::FAIL_BIT);
		callback->audioEngineFailure();
	}
}

struct MediaWorker::ResetAudio {
	MediaWorker &w;

	void operator()() const {
		if (w.ao_->initialized() && !w.ao_->flushPausedBuffers()) {
			w.ao_->uninit();
			w.initAudioEngine();
		}
	}
};

void MediaWorker::resetAudio() {
	const ResetAudio resetAudioStruct = { *this };
	pushCall(resetAudioStruct);
}

struct MediaWorker::SetAudioOut {
	MediaWorker &w; AudioEngine *const ae; const int rate; const int latency;

	void operator()() {
		const bool inited = w.ao_->initialized();
		w.ao_.reset();
		w.ao_.reset(new AudioOut(*ae, rate, latency));

		if (inited)
			w.initAudioEngine();
	}
};

void MediaWorker::setAudioOut(AudioEngine *const newAe, const int rate, const int latency) {
	const SetAudioOut setAudioOutStruct = { *this, newAe, rate, latency };
	pushCall(setAudioOutStruct);
}

struct MediaWorker::SetResampler {
	MediaWorker &w; const unsigned resamplerNum;

	void operator()() {
		if (w.sampleBuffer.resamplerNo() != resamplerNum) {
			w.sampleBuffer.setResampler(resamplerNum);
			w.sndOutBuffer.reset(w.sampleBuffer.maxOut() * 2);
		}
	}
};

void MediaWorker::setResampler(const unsigned resamplerNum) {
	const SetResampler setResamplerStruct = { *this, resamplerNum };
	pushCall(setResamplerStruct);
}

struct MediaWorker::SetFrameTime {
	MediaWorker &w; const Rational ft;

	void operator()() {
		if (ft.num != w.sampleBuffer.ft().num || ft.denom != w.sampleBuffer.ft().denom) {
			w.usecft = static_cast<long>(ft.toFloat() * 1000000.0f + 0.5f);
			w.sampleBuffer.setFt(ft);
			w.sndOutBuffer.reset(w.sampleBuffer.maxOut() * 2);
		}
	}
};

void MediaWorker::setFrameTime(const Rational ft) {
	const SetFrameTime setFrameTimeStruct = { *this, ft };
	pushCall(setFrameTimeStruct);
}

struct MediaWorker::SetSamplesPerFrame {
	MediaWorker &w; const Rational spf;

	void operator()() {
		if (w.sampleBuffer.spf().num != spf.num || w.sampleBuffer.spf().denom != spf.denom) {
			w.sampleBuffer.setSpf(spf);
			w.sndOutBuffer.reset(w.sampleBuffer.maxOut() * 2);
		}
	}
};

void MediaWorker::setSamplesPerFrame(const Rational spf) {
	const SetSamplesPerFrame setSamplesPerFrameStruct = { *this, spf };
	pushCall(setSamplesPerFrameStruct);
}

struct MediaWorker::SetFastForward {
	MediaWorker::TurboSkip &ts; const bool enable;
	void operator()() { ts.setEnabled(enable); }
};

void MediaWorker::setFastForward(const bool enable) {
	const SetFastForward setFastForwardStruct = { turboSkip, enable };
	pushCall(setFastForwardStruct);
}

void MediaWorker::updateJoysticks() {
	if (JoystickLock::tryLock()) {
		SDL_JoystickUpdate();

		SDL_Event ev;

		while (pollJsEvent(&ev))
			source()->joystickEvent(ev);

		JoystickLock::unlock();
	}
}

static long calculateSyncft(const long nominalSrate, const long estSrate, const long nominalft) {
	return estSrate ? static_cast<long>(static_cast<qint64>(nominalft - (nominalft >> 10)) * nominalSrate / estSrate)
	                : nominalft - (nominalft >> 10);
}

// returns syncft
long MediaWorker::adaptToRateEstimation(const long estft) {
	if (estft) {
		meanQueue.push(static_cast<long>((static_cast<qint64>(ao_->estimatedRate()) * estft + (usecft >> 1)) / usecft));

		const long mean = meanQueue.mean();
		const long var = std::max(meanQueue.var(), 1L);

		if (sampleBuffer.resamplerOutRate() < mean - var || sampleBuffer.resamplerOutRate() > mean + var)
			adjustResamplerRate(mean);
	} else if (sampleBuffer.resamplerOutRate() != ao_->rate())
		adjustResamplerRate(ao_->rate());

	return calculateSyncft(sampleBuffer.resamplerOutRate(), ao_->estimatedRate(), usecft);
}

long MediaWorker::sourceUpdate() {
	class VidBuf {
		Callback *cb;
		PixelBuffer pb;
	public:
		explicit VidBuf(Callback *cb) : cb(cb), pb() {
			if (!cb->tryLockVideoBuffer(pb))
				this->cb = 0;
		}

		~VidBuf() { if (cb) cb->unlockVideoBuffer(); }
		const PixelBuffer& get() const { return pb; }
	} vidbuf(callback.get());

	updateJoysticks();
	return sampleBuffer.update(vidbuf.get());
}

void MediaWorker::adjustResamplerRate(const long outRate) {
	sampleBuffer.adjustResamplerOutRate(outRate);

	const std::size_t sz = sampleBuffer.maxOut() * 2;

	if (sz > sndOutBuffer.size())
		sndOutBuffer.reset(sz);
}

namespace {
struct NowDelta { usec_t now, inc;
                  NowDelta(usec_t now, usec_t inc) : now(now), inc(inc) {} };
}

static const NowDelta frameWait(const NowDelta basetime,
		const usec_t syncft, const usec_t usecsFromUnderrun, SyncVar &waitingForSync) {
	const usec_t now = getusecs();
	const usec_t target = basetime.now + basetime.inc + syncft;
	
	if (target - now < basetime.inc + syncft) {
		if (target - now >= usecsFromUnderrun - (usecsFromUnderrun >> 2))
			return basetime;
		
		SyncVar::Locked wfs(waitingForSync);
		
		if (!wfs.get())
			wfs.wait((target - now) / 1000);
		
		return NowDelta(now, target - now);
	}
	
	return NowDelta(now, 0);
}

static void blitWait(MediaWorker::Callback *const cb, SyncVar &waitingForSync) {
	if (!cb->cancelBlit()) {
		SyncVar::Locked wfs(waitingForSync);

		if (!wfs.get())
			wfs.wait();

		wfs.set(false);
	}
}

static bool audioBufIsLow(const AudioEngine::BufferState &bstate, const int outsamples) {
	if (bstate.fromUnderrun == AudioEngine::BufferState::NOT_SUPPORTED)
		return false;
	
	const int fur = bstate.fromUnderrun + outsamples;
	const int fof = static_cast<int>(bstate.fromOverflow) - outsamples;
	return fur < fof * 2;
}

static usec_t usecsFromUnderrun(const AudioEngine::BufferState &bstate, const int outsamples, const long estsrate) {
	if (bstate.fromUnderrun == AudioEngine::BufferState::NOT_SUPPORTED || estsrate == 0)
		return 0x10000000;
	
	const int fur = bstate.fromUnderrun + outsamples;
	const int fof = static_cast<int>(bstate.fromOverflow) - outsamples;
	return (fof < 0 ? fur + fof : fur) * 1000000LL / estsrate;
}

void MediaWorker::run() {
#ifdef Q_WS_WIN
	const class CoInit : Uncopyable {
	public:
		CoInit() { CoInitializeEx(NULL, COINIT_MULTITHREADED); }
		~CoInit() { CoUninitialize(); }
	} coinit;
#endif

	const class AoInit : Uncopyable {
		MediaWorker &w_;
	public:
		explicit AoInit(MediaWorker &w) : w_(w) {
			w.initAudioEngine();
		}
		
		~AoInit() {
			w_.ao_->uninit();
			w_.sampleBuffer.setOutSampleRate(0);
			w_.sndOutBuffer.reset(0);
		}
	} aoinit(*this);
	
	const SetThreadPriorityAudio setmmprio;

	SkipSched skipSched;
	bool audioBufLow = false;
	NowDelta basetime(0, 0);

	for (;;) {
		pauseVar.waitWhilePaused(callback.get(), *ao_);

		if (AtomicVar<bool>::ConstLocked(doneVar).get())
			break;

		const long blitSamples = sourceUpdate();
		const long ftEst = AtomicVar<long>::ConstLocked(frameTimeEst).get();

		if (turboSkip.update()) {
			sampleBuffer.read(blitSamples >= 0 ? blitSamples : sampleBuffer.samplesBuffered(), 0, ftEst != 0);
		} else {
			const long syncft = blitSamples >= 0 ? adaptToRateEstimation(ftEst) : 0;
			const bool blit   = blitSamples >= 0 && !skipSched.skipNext(audioBufLow);

			if (blit)
				callback->blit(basetime.now, basetime.inc + syncft);

			const long outsamples = sampleBuffer.read(
					blit ? blitSamples : sampleBuffer.samplesBuffered(),
					static_cast<qint16*>(sndOutBuffer), ftEst != 0);

			AudioEngine::BufferState bstate = { AudioEngine::BufferState::NOT_SUPPORTED,
			                                    AudioEngine::BufferState::NOT_SUPPORTED };

			if (ao_->successfullyInitialized() && ao_->write(sndOutBuffer, outsamples, bstate) < 0) {
				ao_->pause();
				pauseVar.pause(PauseVar::FAIL_BIT);
				callback->audioEngineFailure();
			}

			audioBufLow = audioBufIsLow(bstate, outsamples);

			if (blit) {
				basetime = frameWait(basetime, syncft, usecsFromUnderrun(
						bstate, outsamples, ao_->estimatedRate()), waitingForSync_);
				blitWait(callback.get(), waitingForSync_);
			}
		}
	}
}

bool MediaWorker::frameStep() {
	const long blitSamples = sourceUpdate();
	const long outsamples = sampleBuffer.read(
			blitSamples >= 0 ? blitSamples : sampleBuffer.samplesBuffered(),
			static_cast<qint16*>(sndOutBuffer), AtomicVar<long>::ConstLocked(frameTimeEst).get() != 0);

	if (ao_->successfullyInitialized()) {
		if (ao_->write(sndOutBuffer, outsamples) < 0) {
			ao_->pause();
			pauseVar.pause(PauseVar::FAIL_BIT);
			callback->audioEngineFailure();
		} else
			ao_->pause();
	}

	return blitSamples >= 0;
}
