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
#include "samplebuffer.h"
#include "callqueue.h"
#include "../pixelbuffer.h"
#include "audioengine.h"
#include "uncopyable.h"
#include "usec.h"

// enum { PREPARE = 1, SYNC = 2 };

class SyncVar {
	QMutex mut;
	QWaitCondition cond;
	unsigned var;

public:
	class Locked : Uncopyable {
		SyncVar &sv;

	public:
		Locked(SyncVar &sv) : sv(sv) { sv.mut.lock(); }
		~Locked() { sv.mut.unlock(); }
		unsigned get() const { return sv.var; }
		void set(const unsigned var) { sv.var = var; sv.cond.wakeAll(); }
		bool wait(const unsigned long time = ULONG_MAX) { return sv.cond.wait(&sv.mut, time); }
// 		bool waitMaskedEqual(unsigned state, unsigned mask, unsigned long time = ULONG_MAX);
// 		bool waitMaskedNequal(unsigned state, unsigned mask, unsigned long time = ULONG_MAX);
// 		bool waitEqual(unsigned state, unsigned long time = ULONG_MAX) { return waitMaskedEqual(state, UINT_MAX, time); }
// 		bool waitNequal(unsigned state, unsigned long time = ULONG_MAX) { return waitMaskedNequal(state, UINT_MAX, time); }
// 		bool waitAnd(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedEqual(bits, bits, time); }
// 		bool waitNand(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedNequal(bits, bits, time); }
// 		bool waitNor(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedEqual(0, bits, time); }
// 		bool waitOr(unsigned bits, unsigned long time = ULONG_MAX) { return waitMaskedNequal(0, bits, time); }
	};

	explicit SyncVar(const unsigned var = 0) : var(var) {}
};

/*bool SyncVar::Locked::waitMaskedEqual(const unsigned state, const unsigned mask, const unsigned long time) {
	while ((get() & mask) != state) {
		wait(time);

		if (time != ULONG_MAX)
			return (get() & mask) == state;
	}

	return true;
}

bool SyncVar::Locked::waitMaskedNequal(const unsigned state, const unsigned mask, const unsigned long time) {
	while ((get() & mask) == state) {
		wait(time);

		if (time != ULONG_MAX)
			return (get() & mask) != state;
	}

	return true;
}*/

template<class T>
class Mutual {
	mutable QMutex mut;
	T t;

public:
	Mutual() : mut(QMutex::Recursive) {}
	explicit Mutual(const T &t) : mut(QMutex::Recursive), t(t) {}

	class Locked : Uncopyable {
		Mutual &lc;
	public:
		Locked(Mutual &lc) : lc(lc) { lc.mut.lock(); }
		~Locked() { lc.mut.unlock(); }
		T* operator->() { return &lc.t; }
		const T* operator->() const { return &lc.t; }
		T& get() { return lc.t; }
		const T& get() const { return lc.t; }
	};

	class ConstLocked : Uncopyable {
		const Mutual &lc;
	public:
		ConstLocked(const Mutual &lc) : lc(lc) { lc.mut.lock(); }
		~ConstLocked() { lc.mut.unlock(); }
		const T* operator->() const { return &lc.t; }
		const T& get() const { return lc.t; }
	};
};

template<typename T>
class AtomicVar {
	mutable QMutex mut;
	T var;
public:
	AtomicVar() : mut(QMutex::Recursive) {}
	explicit AtomicVar(const T var) : mut(QMutex::Recursive), var(var) {}

	class Locked : Uncopyable {
		AtomicVar &av;
	public:
		Locked(AtomicVar &av) : av(av) { av.mut.lock(); }
		~Locked() { av.mut.unlock(); }
		T get() const { return av.var; }
		void set(const T v) { av.var = v; }
	};

	class ConstLocked : Uncopyable {
		const AtomicVar &av;
	public:
		ConstLocked(const AtomicVar &av) : av(av) { av.mut.lock(); }
		~ConstLocked() { av.mut.unlock(); }
		T get() const { return av.var; }
	};
};

class MediaWorker : private QThread {
public:
	class Callback {
	public:
		virtual void paused() = 0;
		virtual void blit(usec_t synctimebase, usec_t synctimeinc) = 0;
		virtual bool cancelBlit() = 0;
// 		virtual void sync() = 0;
		virtual void audioEngineFailure() = 0;
		virtual bool tryLockVideoBuffer() = 0;
		virtual void unlockVideoBuffer() = 0;
		virtual const PixelBuffer& videoBuffer() = 0;
		virtual ~Callback() {}
	};

private:
	class PauseVar {
		CallQueue<> callq;
		mutable QMutex mut;
		QWaitCondition cond;
		unsigned var;
		bool waiting;

	public:
		PauseVar() : var(0), waiting(true) {}
		void localPause(unsigned bits) { if (waiting) var |= bits; else pause(bits); }
		void pause(unsigned bits) { mut.lock(); var |= bits; mut.unlock(); }
		void unpause(unsigned bits);
		void waitWhilePaused(Callback *cb, AudioEngine *ae);
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
	AudioEngine *ae;
	long usecft;
	long estsrate;
	unsigned aelatency;
	int aerate;

	long adaptToRateEstimation(long estft);
	void adjustResamplerRate(long outRate);
	long sourceUpdate();
	void initAudioEngine();

protected:
	void run();

public:
	MediaWorker(MediaSource *source, std::auto_ptr<Callback> callback, QObject *parent = 0);
	MediaSource* source() /*const */{ return sampleBuffer.source(); }
	SyncVar& waitingForSync() /*const */{ return waitingForSync_; }
	void start();
	void stop();
	void pause();
	void unpause() { pauseVar.unpause(1); }
	void qPause() { pauseVar.pause(2); }
	void qUnpause() { pauseVar.unpause(2); }
	void deactivate() { pauseVar.pause(4); }
	void reactivate() { pauseVar.unpause(4); }
	void recover() { pauseVar.unpause(8); }
	bool paused() const { return pauseVar.waitingForUnpause(); }

	void setAudioOut(AudioEngine *newAe, int rate, int latency);
	void setResampler(unsigned resamplerNum);
	void setFrameTime(const Rational &ft);
	void setSamplesPerFrame(const Rational &spf);

	void setFrameTimeEstimate(const long ftest) { AtomicVar<long>::Locked(frameTimeEst).set(ftest); }
	bool frameStep();

	void setFastForwardSpeed(unsigned speed) { turboSkip.setSpeed(speed); }
	unsigned fastForwardSpeed() const { return turboSkip.speed(); }
	void setFastForward(bool enable);
	bool fastForward() const { return turboSkip.isEnabled(); }

	void updateJoysticks();

	template<class T> void pushCall(const T &t) { pauseVar.pushCall(t, AtomicVar<bool>::ConstLocked(doneVar).get()); }
};

template<class T> void MediaWorker::PauseVar::pushCall(const T &t, const bool stopped) {
	mut.lock();
	callq.push(t);
	cond.wakeAll();

	if (stopped)
		callq.pop_all();

	mut.unlock();
}

#endif
