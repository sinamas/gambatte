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

#ifndef MEDIAWORKER_H
#define MEDIAWORKER_H

#include "atomicvar.h"
#include "callqueue.h"
#include "sourceupdater.h"
#include "syncvar.h"
#include "usec.h"
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>
#include <deque>

class AudioEngine;
struct PixelBuffer;

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

	MediaWorker(MediaSource &source, AudioEngine &ae, long aerate, int aelatency,
	            std::size_t resamplerNo, Callback &callback, QObject *parent = 0);
	MediaSource & source() const { return sourceUpdater_.source(); }
	SyncVar & waitingForSync() { return waitingForSync_; }
	void start();
	void stop();
	void pause();
	void unpause() { pauseVar_.unpause(PauseVar::pause_bit); }
	void qPause() { pauseVar_.pause(PauseVar::qpause_bit); }
	void qUnpause() { pauseVar_.unpause(PauseVar::qpause_bit); }
	void recover() { pauseVar_.unpause(PauseVar::fail_bit); }
	bool paused() const { return pauseVar_.waitingForUnpause(); }

	void resetAudio();
	void setAudioOut(AudioEngine &newAe, long rate, int latency, std::size_t resamplerNo);
	void setFrameTime(Rational ft);
	void setSamplesPerFrame(Rational spf);
	void setFrameTimeEstimate(long ftest) { AtomicVar<long>::Locked(frameTimeEst_).set(ftest); }
	bool frameStep();

	void setFastForwardSpeed(int speed) { turboSkip_.setSpeed(speed); }
	int fastForwardSpeed() const { return turboSkip_.speed(); }
	void setFastForward(bool enable);
	bool fastForward() const { return turboSkip_.isEnabled(); }

	void updateJoysticks();

	template<class T>
	void pushCall(T const &t) { pauseVar_.pushCall(t, AtomicVar<bool>::ConstLocked(doneVar_).get()); }

protected:
	virtual void run();

private:
	class AudioOut;

	class PauseVar {
	public:
		enum { pause_bit = 1, qpause_bit = 2, fail_bit = 4 };

		PauseVar()
		: var_(0)
		, waiting_(true)
		{
		}

		void localPause(unsigned bits) {
			if (waiting_) {
				var_ |= bits;
			} else
				pause(bits);
		}

		void pause(unsigned bits) { QMutexLocker l(&mut_); var_ |= bits; }
		void unpause(unsigned bits);
		void waitWhilePaused(Callback &cb, AudioOut &ao);
		bool waitingForUnpause() const { QMutexLocker l(&mut_); return waiting_; }
		void unwait() { waiting_ = false; }
		void rewait() { waiting_ = true; }
		template<class T> void pushCall(T const &t, bool stopped);

	private:
		CallQueue<> callq_;
		mutable QMutex mut_;
		QWaitCondition cond_;
		unsigned var_;
		bool waiting_;

		friend class PushMediaWorkerCall;
	};

	class TurboSkip {
	public:
		TurboSkip()
		: cnt_(0)
		, inc_(0)
		, speed_(4)
		{
		}

		void setEnabled(bool enable) {
			if (enable)
				inc_ = 1;
			else
				cnt_ = inc_ = 0;
		}

		bool isEnabled() const { return inc_; }
		void setSpeed(int speed) { speed_ = speed; }
		int speed() const { return speed_; }

		bool update() {
			if ((cnt_ += inc_) >= speed_)
				cnt_ = 0;

			return cnt_;
		}

	private:
		int cnt_, inc_, speed_;
	};

	class MeanQueue {
	public:
		MeanQueue(long mean, long var);
		void reset(long mean, long var);
		long mean() const { return sum_ / size; }
		long var() const { return dsum_ / size; }
		void push(long i);

	private:
		enum { size = 16 };
		struct Elem {
			long sumpart, dsumpart;
			Elem(long sp, long dp) : sumpart(sp), dsumpart(dp) {}
		};

		std::deque<Elem> q_;
		long sum_;
		long dsum_;
	};

	struct ResetAudio;
	struct SetAudioOut;
	struct SetFrameTime;
	struct SetSamplesPerFrame;
	struct SetFastForward;

	Callback &callback_;
	SyncVar waitingForSync_;
	MeanQueue meanQueue_;
	PauseVar pauseVar_;
	AtomicVar<long> frameTimeEst_;
	AtomicVar<bool> doneVar_;
	TurboSkip turboSkip_;
	SourceUpdater sourceUpdater_;
	Array<qint16> sndOutBuffer_;
	scoped_ptr<AudioOut> ao_;
	long usecft_;

	friend class PushMediaWorkerCall;
	long adaptToRateEstimation(long estft);
	void adjustResamplerRate(long outRate);
	std::ptrdiff_t sourceUpdate();
	void initAudioEngine();
};

template<class T>
void MediaWorker::PauseVar::pushCall(T const &t, bool const stopped) {
	QMutexLocker l(&mut_);
	callq_.push(t);
	cond_.wakeAll();
	if (stopped)
		callq_.pop_all();
}

#endif
