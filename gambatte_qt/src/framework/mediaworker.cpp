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
#include "SDL_Joystick/include/SDL_joystick.h"
#include "mediasource.h"

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

// void MediaWorker::FrameWaiter::frameWait(const long syncft, SyncVar &waitingForSync) {
// 	const usec_t now = getusecs();
//
// 	if (now - base < syncft) {
// 		{
// 			SyncVar::Locked wfs(waitingForSync);
//
// 			/*while*/if (!wfs.get() && /*wfs.wait(now, syncft - (now - base)*/ wfs.wait((syncft - (now - base)) / 1000))
// 				;
//
// 			if (wfs.get())
// 				wfs.set(false);
// 		}
//
// 		asleep.sleepUntil(base, syncft);
// 		base += syncft;
// 	} else
// 		base = now;
// }
//
// void MediaWorker::FrameWaiter::syncedFrameWait(const long ft) {
// 	if (ft) {
// 		base += asleep.sleepUntil(base, ft);
// 		base += ft;
// 	}
// }

void MediaWorker::PauseVar::unpause(const unsigned bits) {
	mut.lock();

	if (var && !(var &= ~bits))
		cond.wakeAll();

	mut.unlock();
}

void MediaWorker::PauseVar::waitWhilePaused(MediaWorker::Callback *const cb, AudioEngine *const ae) {
	mut.lock();
	waiting = true;
	callq.pop_all();

	if (var) {
		if (ae && (var & 1))
			ae->pause();

		cb->paused();

		do {
			cond.wait(&mut);
			callq.pop_all();
		} while (var);
	}

	waiting = false;
	mut.unlock();
}

MediaWorker::MediaWorker(MediaSource *source, std::auto_ptr<Callback> callback, QObject *parent) :
QThread(parent),
callback(callback),
meanQueue(0, 0),
frameTimeEst(0),
doneVar(true),
sampleBuffer(source),
ae(NULL),
usecft(0),
estsrate(0),
base(0),
/*turbo(0), */
aelatency(0),
aerate(0),
audioBufLow(false) {
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
// 	SyncVar::Locked(syncVar_).set(PREPARE);
	pauseVar.unpause(~0U);
	wait();
// 	SyncVar::Locked(syncVar_).set(0);
	pauseVar.rewait();
	sampleBuffer.setOutSampleRate(0);
	sndOutBuffer.reset(0);
}

void MediaWorker::pause() {
	pauseVar.pause(1);

	if (ae && pauseVar.waitingForUnpause() && !AtomicVar<bool>::ConstLocked(doneVar).get())
		ae->pause();
}

void MediaWorker::initAudioEngine() {
	ae->init(aerate, aelatency);
	estsrate = ae->rate() > 0 ? ae->rate() : aerate;
	sampleBuffer.setOutSampleRate(estsrate);
	sndOutBuffer.reset(sampleBuffer.maxOut() * 2);
	meanQueue.reset(estsrate, estsrate >> 12);

	if (ae->rate() <= 0) {
		pauseVar.localPause(8);
		callback->audioEngineFailure();
	}
}

struct MediaWorker::SetAudioOut {
	MediaWorker &w; AudioEngine *const ae; const int rate; const int latency;

	void operator()() {
		const bool stopped = AtomicVar<bool>::ConstLocked(w.doneVar).get();

		if (!stopped && w.ae)
			w.ae->uninit();

		w.ae = ae;
		w.aerate = rate;
		w.aelatency = latency;

		if (!stopped)
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

void MediaWorker::setFrameTime(const Rational &ft) {
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

void MediaWorker::setSamplesPerFrame(const Rational &spf) {
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
	return static_cast<long>(static_cast<float>(nominalft - (nominalft >> 10)) * nominalSrate / (estSrate ? estSrate : 1));
}

// returns syncft
long MediaWorker::adaptToRateEstimation(const long estft) {
	if (estft) {
		meanQueue.push(static_cast<long>(static_cast<float>(estsrate) * estft / usecft + 0.5f));

		const long mean = meanQueue.mean();
		const long var = std::max(meanQueue.var(), 1L);

		if (sampleBuffer.resamplerOutRate() < mean - var || sampleBuffer.resamplerOutRate() > mean + var)
			adjustResamplerRate(mean);
	} else if (sampleBuffer.resamplerOutRate() != (ae->rate() > 0 ? ae->rate() : aerate))
		adjustResamplerRate((ae->rate() > 0 ? ae->rate() : aerate));

	return calculateSyncft(sampleBuffer.resamplerOutRate(), estsrate, usecft);
}

// we try to force the sync to begin in the GUI thread before we continue, which is why this code looks a bit funky.
// static void signalSync(SyncVar &syncVar, MediaWorker::Callback *cb) {
// // 	unsigned sync;
//
// 	{
// 		SyncVar::Locked sv(syncVar);
// 		sv.set(/*sync = */(sv.get() & PREPARE) ? 0 : SYNC);
// 	}
//
// // 	if (sync)
// // 		cb->sync();
//
// 	{
// 		SyncVar::Locked sv(syncVar);
//
// 		while (sv.get() & SYNC)
// 			sv.wait();
// 	}
// }

long MediaWorker::sourceUpdate() {
	class VidBuf {
		Callback *cb;
		PixelBuffer pb;
	public:
		VidBuf(Callback *cb) : cb(cb), pb(cb->videoBuffer()) {
			if (!cb->tryLockVideoBuffer()) {
				pb.data = 0;
				this->cb = 0;
			}
		}

		~VidBuf() { if (cb) cb->unlockVideoBuffer(); }
		const PixelBuffer& get() const { return pb; }
	} vidbuf(callback.get());

// 	turbo += turbo << 4;
	updateJoysticks();
	return sampleBuffer.update(vidbuf.get());
}

void MediaWorker::adjustResamplerRate(const long outRate) {
	sampleBuffer.adjustResamplerOutRate(outRate);

	const std::size_t sz = sampleBuffer.maxOut() * 2;

	if (sz > sndOutBuffer.size())
		sndOutBuffer.reset(sz);
}

static usec_t frameWait(const usec_t base, const usec_t syncft, SyncVar &waitingForSync) {
	const usec_t now = getusecs();

	if (now - base + syncft < syncft * 2) {
		SyncVar::Locked wfs(waitingForSync);

		/*while*/if (!wfs.get() && /*wfs.wait(now, syncft - (now - base)*/ wfs.wait((syncft * 2 - (now - base + syncft)) / 1000)) {
			;
		}

		return base + syncft;
	}

	return now;
}

static void blitWait(MediaWorker::Callback *const cb, SyncVar &waitingForSync) {
// 	unsigned notAcked;
//
// 	{
// 		SyncVar::Locked sv(syncVar);
//
// 		if ((notAcked = sv.get() & PREPARE))
// 			sv.set(0);
// 	}

	if (!cb->cancelBlit()) {
		SyncVar::Locked wfs(waitingForSync);

		if (!wfs.get())
			wfs.wait();

		wfs.set(false);
	}
}

static bool audioBufIsLow(const AudioEngine::BufferState &bstate, const int outsamples) {
	// This depends on static_cast<int>(AudioEngine::BufferState::NOT_SUPPORTED) == -1
	const int fur = bstate.fromUnderrun + outsamples;
	const int fof = static_cast<int>(bstate.fromOverflow) - outsamples;
	return fur < fof * 2;
}

void MediaWorker::run() {
#ifdef Q_WS_WIN
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif
	if (ae)
		initAudioEngine();

	for (;;) {
		pauseVar.waitWhilePaused(callback.get(), ae);

		if (AtomicVar<bool>::ConstLocked(doneVar).get())
			break;

		const long blitSamples = sourceUpdate();

		if (turboSkip.update()) {
			sampleBuffer.read(blitSamples >= 0 ? blitSamples : sampleBuffer.samplesBuffered(), 0);
		} else {
			const long syncft = blitSamples >= 0 ? adaptToRateEstimation(AtomicVar<long>::ConstLocked(frameTimeEst).get()) : 0;
			const bool blit = blitSamples >= 0 && !skipSched.skipNext(audioBufLow);

			if (blit)
				callback->blit(base, syncft);

			{
				const long outsamples = sampleBuffer.read(blit ? blitSamples : sampleBuffer.samplesBuffered(),
												static_cast<qint16*>(sndOutBuffer));
				AudioEngine::BufferState bstate = { AudioEngine::BufferState::NOT_SUPPORTED, AudioEngine::BufferState::NOT_SUPPORTED };

				if (ae->rate() > 0 && ae->write(sndOutBuffer, outsamples, bstate, estsrate) < 0) {
					ae->pause();
					pauseVar.pause(8);
					callback->audioEngineFailure();
				}

				audioBufLow = audioBufIsLow(bstate, outsamples);
			}

			if (blit) {
				base = frameWait(base, syncft, waitingForSync_);
				blitWait(callback.get(), waitingForSync_);
			}
		}
	}

	ae->uninit();

#ifdef Q_WS_WIN
	CoUninitialize();
#endif
}

bool MediaWorker::frameStep() {
	const long blitSamples = sourceUpdate();

	{
		const long outsamples = sampleBuffer.read(blitSamples >= 0 ? blitSamples : sampleBuffer.samplesBuffered(),
								static_cast<qint16*>(sndOutBuffer));

		if (ae->rate() > 0) {
			if (ae->write(sndOutBuffer, outsamples) < 0) {
				ae->pause();
				pauseVar.pause(8);
				callback->audioEngineFailure();
			} else
				ae->pause();
		}
	}

	return blitSamples >= 0;
}


#if 0

// returns syncft
long MediaWorker::adaptToRateEstimation(const long estft) {
	if (estft) {
		meanQueue.push(static_cast<long>(static_cast<float>(estsrate) * estft / (usecft - (usecft >> 11)) + 0.5f));

		const long mean = meanQueue.mean();
		const long var = std::max(meanQueue.var(), 1);

		if (sampleBuffer.resamplerOutRate() < mean || sampleBuffer.resamplerOutRate() > mean + var * 2)
			adjustResamplerRate(mean + var);

		return 0;
	}

	if (sampleBuffer.resamplerOutRate() != ae->rate())
		adjustResamplerRate(ae->rate());

	return calculateSyncft(ae->rate(), estsrate, usecft);
}

void MediaWorker::syncedUpdate(BlitterWidget *const blitter) {
	CallQ::Locked(callq)->pop_all();

	if (!ae || ae->rate() < 0) { // avoid stupid recursive call detection by checking here rather than on init.
		emit soundEngineFailure();
		return;
	}

	long outsamples = 0;
	pauseVar.unwait();
	const bool blit = sourceUpdate(&outsamples);
	pauseVar.rewait();
	const long syncft = blit ? adaptToRateEstimation(blitter->frameTimeEst()) : 0;

	if (blit)
		blitter->prepare();

	if (!(turbo & 0x30) && ae->write(sndOutBuffer, outsamples, bstate, estsrate) < 0) {
		ae->pause();
		emit soundEngineFailure();
		return;
	}

	if (blit) {
		frameWaiter.syncedFrameWait(syncft);

		if (blitter->sync() < 0) {
			emit videoEngineFailure();
			return;
		}
	}
}

void MainWindow::timerEvent(QTimerEvent*) {
	worker->syncedUpdate(blitter);
	workerPaused(this);
}

// NOTE: callInGuiThread
void MainWindow::setThreaded(const bool threaded) {
	if (threaded ^ this->threaded) {
		this->threaded = threaded;

		if (running) {
			if (threaded) {
				if (timerId)
					killTimer(timerId);

				timerId = 0;
				worker->reactivate();
			} else {
				struct StartTimer {
					void operator()(MainWindow *const mw) {
						if (mw->running && mw->threaded)
							mw->timerId = mw->startTimer(0);
					}
				};

				worker->deactivate();
				callWhenPaused(StartTimer());
			}
		}
	}
}

#endif
