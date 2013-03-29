/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#ifndef MEDIAWORKER_H
#define MEDIAWORKER_H

#include "atomicvar.h"
#include "callqueue.h"
#include "pixelbuffer.h"
#include "samplebuffer.h"
#include "syncvar.h"
#include "uncopyable.h"
#include "usec.h"
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>
#include <deque>

class AudioEngine;

class MediaWorker : private QThread {
public:
	class Callback {
	public:
		virtual void paused() = 0;
		virtual void blit(usec_t synctimebase, usec_t synctimeinc) = 0;
		virtual bool cancelBlit() = 0;
		virtual void audioEngineFailure() = 0;
		virtual bool tryLockVideoBuffer(PixelBuffer &pb) = 0;
		virtual void unlockVideoBuffer() = 0;
		virtual ~Callback() {}
	};

private:
	class AudioOut;

	class PauseVar {
		CallQueue<> callq;
		mutable QMutex mut;
		QWaitCondition cond;
		unsigned var;
		bool waiting;

		friend class PushMediaWorkerCall;
	public:
		enum { PAUSE_BIT = 1, QPAUSE_BIT = 2, FAIL_BIT = 4 };
		PauseVar() : var(0), waiting(true) {}
		void localPause(unsigned bits) { if (waiting) var |= bits; else pause(bits); }
		void pause(unsigned bits) { QMutexLocker l(&mut); var |= bits; }
		void unpause(unsigned bits);
		void waitWhilePaused(Callback &cb, AudioOut &ao);
		bool waitingForUnpause() const { QMutexLocker l(&mut); return waiting; }
		void unwait() { waiting = false; }
		void rewait() { waiting = true; }
		template<class T> void pushCall(const T &t, bool stopped);
	};

	class TurboSkip {
		unsigned cnt, inc, speed_;

	public:
		TurboSkip() : cnt(0), inc(0), speed_(4) {}

		void setEnabled(const bool enable) {
			if (enable)
				inc = 1;
			else
				cnt = inc = 0;
		}

		bool isEnabled() const { return inc; }
		void setSpeed(const unsigned speed) { speed_ = speed; }
		unsigned speed() const { return speed_; }

		bool update() {
			if ((cnt += inc) >= speed_)
				cnt = 0;

			return cnt;
		}
	};

	class MeanQueue {
		enum { size = 16 };
		struct Elem { long sumpart, dsumpart; Elem(long sp, long dp) : sumpart(sp), dsumpart(dp) {} };
		typedef std::deque<Elem> q_type;
		q_type q;
		long sum;
		long dsum;

	public:
		MeanQueue(long mean, long var);
		void reset(long mean, long var);
		long mean() const { return sum / size; }
		long var() const { return dsum / size; }
		void push(long i);
	};

	struct ResetAudio;
	struct SetAudioOut;
	struct SetFrameTime;
	struct SetSamplesPerFrame;
	struct SetFastForward;

	Callback &callback;
	SyncVar waitingForSync_;
	MeanQueue meanQueue;
	PauseVar pauseVar;
	AtomicVar<long> frameTimeEst;
	AtomicVar<bool> doneVar;
	TurboSkip turboSkip;
	SampleBuffer sampleBuffer;
	Array<qint16> sndOutBuffer;
	scoped_ptr<AudioOut> ao_;
	long usecft;

	friend class PushMediaWorkerCall;
	long adaptToRateEstimation(long estft);
	void adjustResamplerRate(long outRate);
	long sourceUpdate();
	void initAudioEngine();

protected:
	void run();

public:
	MediaWorker(MediaSource &source, AudioEngine &ae, int aerate, int aelatency,
	            std::size_t resamplerNo, Callback &callback, QObject *parent = 0);
	MediaSource & source() const { return sampleBuffer.source(); }
	SyncVar & waitingForSync() { return waitingForSync_; }
	void start();
	void stop();
	void pause();
	void unpause() { pauseVar.unpause(PauseVar::PAUSE_BIT); }
	void qPause() { pauseVar.pause(PauseVar::QPAUSE_BIT); }
	void qUnpause() { pauseVar.unpause(PauseVar::QPAUSE_BIT); }
	void recover() { pauseVar.unpause(PauseVar::FAIL_BIT); }
	bool paused() const { return pauseVar.waitingForUnpause(); }

	void resetAudio();
	void setAudioOut(AudioEngine &newAe, int rate, int latency, std::size_t resamplerNo);
	void setFrameTime(Rational ft);
	void setSamplesPerFrame(Rational spf);

	void setFrameTimeEstimate(long ftest) { AtomicVar<long>::Locked(frameTimeEst).set(ftest); }
	bool frameStep();

	void setFastForwardSpeed(unsigned speed) { turboSkip.setSpeed(speed); }
	unsigned fastForwardSpeed() const { return turboSkip.speed(); }
	void setFastForward(bool enable);
	bool fastForward() const { return turboSkip.isEnabled(); }

	void updateJoysticks();

	template<class T>
	void pushCall(const T &t) { pauseVar.pushCall(t, AtomicVar<bool>::ConstLocked(doneVar).get()); }
};

template<class T>
void MediaWorker::PauseVar::pushCall(const T &t, const bool stopped) {
	QMutexLocker l(&mut);
	callq.push(t);
	cond.wakeAll();
	if (stopped)
		callq.pop_all();
}

#endif
