//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#include "mediaworker.h"
#include "audioengine.h"
#include "joysticklock.h"
#include "mediasource.h"
#include "mmpriority.h"
#include "pixelbuffer.h"
#include "skipsched.h"
#include <QtGlobal> // for Q_WS_WIN define
#ifdef Q_WS_WIN
#include <Objbase.h> // For CoInitialize
#endif
#include <cstdlib>

MediaWorker::MeanQueue::MeanQueue(long mean, long var)
: q_(size, Elem(mean, var))
, sum_(mean * size)
, dsum_(var * size)
{
}

void MediaWorker::MeanQueue::reset(long mean, long var) {
	q_.assign(size, Elem(mean, var));
	sum_ = mean * size;
	dsum_ = var * size;
}

void MediaWorker::MeanQueue::push(long const i) {
	Elem const newelem(i, std::abs(i - mean()));
	Elem const &oldelem = q_.front();
	sum_ += newelem.sumpart - oldelem.sumpart;
	dsum_ += newelem.dsumpart - oldelem.dsumpart;
	q_.pop_front();
	q_.push_back(newelem);
}

class MediaWorker::AudioOut : Uncopyable {
public:
	AudioOut(AudioEngine &ae, long rate, int latency, std::size_t resamplerNo)
	: ae_(ae)
	, resamplerNo_(resamplerNo)
	, rate_(rate)
	, latency_(latency)
	, estrate_(rate)
	, inited_(false)
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
	std::size_t resamplerNo() const { return resamplerNo_; }
	bool initialized() const { return inited_; }
	bool successfullyInitialized() const { return inited_ && ae_.rate() > 0; }

	int write(qint16 *buf, std::size_t samples, AudioEngine::BufferState &preBstateOut) {
		return ae_.write(buf, samples, preBstateOut, estrate_);
	}

	int write(qint16 *buf, std::size_t samples) {
		return ae_.write(buf, samples);
	}

private:
	AudioEngine &ae_;
	std::size_t const resamplerNo_;
	long const rate_;
	int const latency_;
	long estrate_;
	bool inited_;
};

void MediaWorker::PauseVar::unpause(unsigned bits) {
	QMutexLocker l(&mut_);
	if (var_ && !(var_ &= ~bits))
		cond_.wakeAll();
}

void MediaWorker::PauseVar::waitWhilePaused(MediaWorker::Callback &cb, AudioOut &ao) {
	QMutexLocker locker(&mut_);
	waiting_ = true;
	callq_.pop_all();
	if (var_) {
		if (var_ & 1)
			ao.pause();

		cb.paused();

		do {
			cond_.wait(locker.mutex());
			callq_.pop_all();
		} while (var_);
	}

	waiting_ = false;
}

MediaWorker::MediaWorker(MediaSource &source,
                         AudioEngine &ae, long aerate, int aelatency,
                         std::size_t resamplerNo,
                         Callback &callback,
                         QObject *parent)
: QThread(parent)
, callback_(callback)
, meanQueue_(0, 0)
, frameTimeEst_(0)
, doneVar_(true)
, sourceUpdater_(source)
, ao_(new AudioOut(ae, aerate, aelatency, resamplerNo))
, usecft_(0)
{
}

void MediaWorker::start() {
	if (AtomicVar<bool>::ConstLocked(doneVar_).get()) {
		wait();
		AtomicVar<bool>::Locked(doneVar_).set(false);
		pauseVar_.unwait();
		QThread::start();
	}
}

void MediaWorker::stop() {
	AtomicVar<bool>::Locked(doneVar_).set(true);
	pauseVar_.unpause(~0U);
	wait();
	pauseVar_.rewait();
}

void MediaWorker::pause() {
	pauseVar_.pause(PauseVar::pause_bit);
	if (pauseVar_.waitingForUnpause())
		ao_->pause();
}

void MediaWorker::initAudioEngine() {
	ao_->init();
	sourceUpdater_.setOutSampleRate(ao_->rate(), ao_->resamplerNo());
	sndOutBuffer_.reset(sourceUpdater_.maxOut() * 2);
	meanQueue_.reset(ao_->rate(), ao_->rate() >> 12);

	if (!ao_->successfullyInitialized()) {
		pauseVar_.localPause(PauseVar::fail_bit);
		callback_.audioEngineFailure();
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
	ResetAudio resetAudioStruct = { *this };
	pushCall(resetAudioStruct);
}

struct MediaWorker::SetAudioOut {
	MediaWorker &w; AudioEngine &ae; long const rate; int const latency;
	std::size_t const resamplerNo;
	void operator()() const {
		bool const inited = w.ao_->initialized();
		w.ao_.reset();
		w.ao_.reset(new AudioOut(ae, rate, latency, resamplerNo));

		if (inited)
			w.initAudioEngine();
	}
};

void MediaWorker::setAudioOut(AudioEngine &newAe, long rate, int latency, std::size_t resamplerNo) {
	SetAudioOut setAudioOutStruct = { *this, newAe, rate, latency, resamplerNo };
	pushCall(setAudioOutStruct);
}

struct MediaWorker::SetFrameTime {
	MediaWorker &w; Rational const ft;
	void operator()() {
		if (ft != w.sourceUpdater_.ft()) {
			w.usecft_ = static_cast<long>(ft.toFloat() * 1000000.0f + 0.5f);
			w.sourceUpdater_.setFt(ft);
			w.sndOutBuffer_.reset(w.sourceUpdater_.maxOut() * 2);
		}
	}
};

void MediaWorker::setFrameTime(Rational ft) {
	SetFrameTime setFrameTimeStruct = { *this, ft };
	pushCall(setFrameTimeStruct);
}

struct MediaWorker::SetSamplesPerFrame {
	MediaWorker &w; Rational const spf;
	void operator()() {
		if (w.sourceUpdater_.spf() != spf) {
			w.sourceUpdater_.setSpf(spf);
			w.sndOutBuffer_.reset(w.sourceUpdater_.maxOut() * 2);
		}
	}
};

void MediaWorker::setSamplesPerFrame(Rational spf) {
	SetSamplesPerFrame setSamplesPerFrameStruct = { *this, spf };
	pushCall(setSamplesPerFrameStruct);
}

struct MediaWorker::SetFastForward {
	MediaWorker::TurboSkip &ts; bool const enable;
	void operator()() { ts.setEnabled(enable); }
};

void MediaWorker::setFastForward(bool enable) {
	SetFastForward setFastForwardStruct = { turboSkip_, enable };
	pushCall(setFastForwardStruct);
}

void MediaWorker::updateJoysticks() {
	SdlJoystick::TryLocked tl;
	if (tl) {
		SdlJoystick::Locked js(tl);
		js.update();

		SDL_Event ev;
		while (js.pollEvent(&ev))
			source().joystickEvent(ev);
	}
}

static long calculateSyncft(long nominalSrate, long estSrate, long nominalft) {
	if (estSrate)
		return qint64(nominalft - (nominalft >> 10)) * nominalSrate / estSrate;

	return nominalft - (nominalft >> 10);
}

// returns syncft
long MediaWorker::adaptToRateEstimation(long const estft) {
	if (estft) {
		meanQueue_.push((qint64(ao_->estimatedRate()) * estft + (usecft_ >> 1)) / usecft_);

		long mean = meanQueue_.mean();
		long var = std::max(meanQueue_.var(), 1L);

		if (sourceUpdater_.resamplerOutRate() < mean - var
				|| sourceUpdater_.resamplerOutRate() > mean + var) {
			adjustResamplerRate(mean);
		}
	} else if (sourceUpdater_.resamplerOutRate() != ao_->rate())
		adjustResamplerRate(ao_->rate());

	return calculateSyncft(sourceUpdater_.resamplerOutRate(), ao_->estimatedRate(), usecft_);
}

std::ptrdiff_t MediaWorker::sourceUpdate() {
	class VidBuf {
		Callback *cb;
		PixelBuffer pb;
	public:
		explicit VidBuf(Callback *cb) : cb(cb), pb() {
			if (!cb->tryLockVideoBuffer(pb))
				this->cb = 0;
		}

		~VidBuf() { if (cb) cb->unlockVideoBuffer(); }
		PixelBuffer const & get() const { return pb; }
	} vidbuf(&callback_);

	updateJoysticks();
	return sourceUpdater_.update(vidbuf.get());
}

void MediaWorker::adjustResamplerRate(long const outRate) {
	sourceUpdater_.adjustResamplerOutRate(outRate);

	std::size_t size = sourceUpdater_.maxOut() * 2;
	if (size > sndOutBuffer_.size())
		sndOutBuffer_.reset(size);
}

namespace {

struct NowDelta {
	usec_t now, inc;
	NowDelta(usec_t now, usec_t inc) : now(now), inc(inc) {}
};

static NowDelta frameWait(NowDelta const basetime,
                          usec_t const syncft,
                          usec_t const usecsFromUnderrun,
                          SyncVar &waitingForSync)
{
	usec_t const now = getusecs();
	usec_t const target = basetime.now + basetime.inc + syncft;

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

static void blitWait(MediaWorker::Callback &cb, SyncVar &waitingForSync) {
	if (!cb.cancelBlit()) {
		SyncVar::Locked wfs(waitingForSync);
		if (!wfs.get())
			wfs.wait();

		wfs.set(false);
	}
}

static bool audioBufIsLow(AudioEngine::BufferState const &bstate, std::ptrdiff_t const outsamples) {
	if (bstate.fromUnderrun == AudioEngine::BufferState::not_supported)
		return false;

	std::ptrdiff_t fur = bstate.fromUnderrun + outsamples;
	std::ptrdiff_t fof = static_cast<std::ptrdiff_t>(bstate.fromOverflow) - outsamples;
	return fur < fof * 2;
}

static usec_t usecsFromUnderrun(AudioEngine::BufferState const &bstate,
                                std::ptrdiff_t const outsamples,
                                long const estsrate)
{
	if (bstate.fromUnderrun == AudioEngine::BufferState::not_supported || estsrate == 0)
		return 0x10000000;

	std::ptrdiff_t fur = bstate.fromUnderrun + outsamples;
	std::ptrdiff_t fof = static_cast<std::ptrdiff_t>(bstate.fromOverflow) - outsamples;
	return (fof < 0 ? fur + fof : fur) * 1000000LL / estsrate;
}

} // anon ns

void MediaWorker::run() {
#ifdef Q_WS_WIN
	class CoInit : Uncopyable {
	public:
		CoInit() { CoInitializeEx(0, COINIT_MULTITHREADED); }
		~CoInit() { CoUninitialize(); }
	} coinit;
#endif
	class AoInit : Uncopyable {
		MediaWorker &w_;
	public:
		explicit AoInit(MediaWorker &w) : w_(w) {
			w.initAudioEngine();
		}

		~AoInit() {
			w_.ao_->uninit();
			w_.sourceUpdater_.setOutSampleRate(0);
			w_.sndOutBuffer_.reset(0);
		}
	} aoinit(*this);
	SetThreadPriorityAudio const setmmprio;
	SkipSched skipSched;
	bool audioBufLow = false;
	NowDelta basetime(0, 0);

	for (;;) {
		pauseVar_.waitWhilePaused(callback_, *ao_);
		if (AtomicVar<bool>::ConstLocked(doneVar_).get())
			break;

		std::ptrdiff_t const blitSamples = sourceUpdate();
		long const ftEst = AtomicVar<long>::ConstLocked(frameTimeEst_).get();

		if (turboSkip_.update()) {
			std::size_t sourceSamplesToRead = blitSamples >= 0
			                                ? blitSamples
			                                : sourceUpdater_.samplesBuffered();
			sourceUpdater_.readSamples(0, sourceSamplesToRead, ftEst != 0);
		} else {
			long const syncft = blitSamples >= 0 ? adaptToRateEstimation(ftEst) : 0;
			bool const blit   = blitSamples >= 0 && !skipSched.skipNext(audioBufLow);

			if (blit)
				callback_.blit(basetime.now, basetime.inc + syncft);

			std::size_t const outsamples =
				sourceUpdater_.readSamples(sndOutBuffer_, 
				                   blit ? blitSamples : sourceUpdater_.samplesBuffered(),
				                   ftEst != 0);
			AudioEngine::BufferState bstate = { AudioEngine::BufferState::not_supported,
			                                    AudioEngine::BufferState::not_supported };
			if (ao_->successfullyInitialized()
					&& ao_->write(sndOutBuffer_, outsamples, bstate) < 0) {
				ao_->pause();
				pauseVar_.pause(PauseVar::fail_bit);
				callback_.audioEngineFailure();
			}

			audioBufLow = audioBufIsLow(bstate, outsamples);

			if (blit) {
				basetime = frameWait(basetime, syncft, usecsFromUnderrun(
					bstate, outsamples, ao_->estimatedRate()), waitingForSync_);
				blitWait(callback_, waitingForSync_);
			}
		}
	}
}

bool MediaWorker::frameStep() {
	std::ptrdiff_t const blitSamples = sourceUpdate();
	std::size_t const outsamples =
		sourceUpdater_.readSamples(sndOutBuffer_,
			blitSamples >= 0 ? blitSamples : sourceUpdater_.samplesBuffered(),
			AtomicVar<long>::ConstLocked(frameTimeEst_).get() != 0);
	if (ao_->successfullyInitialized()) {
		if (ao_->write(sndOutBuffer_, outsamples) < 0) {
			ao_->pause();
			pauseVar_.pause(PauseVar::fail_bit);
			callback_.audioEngineFailure();
		} else
			ao_->pause();
	}

	return blitSamples >= 0;
}
