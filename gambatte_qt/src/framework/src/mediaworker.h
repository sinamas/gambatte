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
#ifndef MEDIAWORKER_H
#define MEDIAWORKER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <deque>
#include "callqueue.h"
#include "pixelbuffer.h"
#include "samplebuffer.h"
#include "syncvar.h"
#include "atomicvar.h"
#include "uncopyable.h"
#include "usec.h"

class MediaWorker : private QThread {
public:
	class Callback {
	public:
		virtual void paused() = 0;
		virtual void blit(usec_t synctimebase, usec_t synctimeinc) = 0;
		virtual bool cancelBlit() = 0;
// 		virtual void sync() = 0;
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
		void pause(unsigned bits) { mut.lock(); var |= bits; mut.unlock(); }
		void unpause(unsigned bits);
		void waitWhilePaused(Callback *cb, AudioOut &ao);
		bool waitingForUnpause() const { bool ret; mut.lock(); ret = waiting; mut.unlock(); return ret; }
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
		enum { sz = 16 };
		struct Elem { long sumpart, dsumpart; Elem() {} Elem(long sp, long dp) : sumpart(sp), dsumpart(dp) {} };
		typedef std::deque<Elem> q_type;
		q_type q;
		long sum;
		long dsum;

	public:
		MeanQueue(long mean, long var);
		void reset(long mean, long var);
		long mean() const { return sum / sz; }
		long var() const { return dsum / sz; }
		void push(long i);
	};
	
	struct ResetAudio;
	struct SetAudioOut;
	struct SetResampler;
	struct SetFrameTime;
	struct SetSamplesPerFrame;
	struct SetFastForward;

	const std::auto_ptr<Callback> callback;
	SyncVar waitingForSync_;
	MeanQueue meanQueue;
	PauseVar pauseVar;
	AtomicVar<long> frameTimeEst;
	AtomicVar<bool> doneVar;
	TurboSkip turboSkip;
	SampleBuffer sampleBuffer;
	Array<qint16> sndOutBuffer;
	std::auto_ptr<AudioOut> ao_;
	long usecft;

	friend class PushMediaWorkerCall;
	long adaptToRateEstimation(long estft);
	void adjustResamplerRate(long outRate);
	long sourceUpdate();
	void initAudioEngine();

protected:
	void run();

public:
	MediaWorker(MediaSource *source, class AudioEngine *ae, int aerate,
			int aelatency, std::auto_ptr<Callback> callback, QObject *parent = 0);
	MediaSource* source() /*const */{ return sampleBuffer.source(); }
	SyncVar& waitingForSync() /*const */{ return waitingForSync_; }
	void start();
	void stop();
	void pause();
	void unpause() { pauseVar.unpause(PauseVar::PAUSE_BIT); }
	void qPause() { pauseVar.pause(PauseVar::QPAUSE_BIT); }
	void qUnpause() { pauseVar.unpause(PauseVar::QPAUSE_BIT); }
	void recover() { pauseVar.unpause(PauseVar::FAIL_BIT); }
	bool paused() const { return pauseVar.waitingForUnpause(); }

	void resetAudio();
	void setAudioOut(class AudioEngine *newAe, int rate, int latency);
	void setResampler(unsigned resamplerNum);
	void setFrameTime(Rational ft);
	void setSamplesPerFrame(Rational spf);

	void setFrameTimeEstimate(const long ftest) { AtomicVar<long>::Locked(frameTimeEst).set(ftest); }
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
	mut.lock();
	callq.push(t);
	cond.wakeAll();

	if (stopped)
		callq.pop_all();

	mut.unlock();
}

#endif
