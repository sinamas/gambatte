//
//   Copyright (C) 2007-2009 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include "audioengineconf.h"
#include "auto_vector.h"
#include "blitterconf.h"
#include "blittercontainer.h"
#include "blitterwidget.h"
#include "dwmcontrol.h"
#include "frameratecontrol.h"
#include "fullmodetoggler.h"
#include "mediaworker.h"
#include "resample/resamplerinfo.h"
#include "scoped_ptr.h"
#include "SDL_joystick.h"
#include <QMutex>
#include <QSize>
#include <QTimer>

class MediaWidget : public QObject {
public:
	MediaWidget(MediaSource &source, QWidget &parent);
	virtual ~MediaWidget();
	QWidget * widget() const { return blitterContainer_; }

	/** @return compositionChange */
	bool winEvent(void const *message) { return dwmControl_.winEvent(message); }
	void hideEvent() { dwmControl_.hideEvent(); }
	void showEvent(QWidget const *parent) { dwmControl_.showEvent(); fullModeToggler_->setScreen(parent); }
	void mouseMoveEvent() { blitterContainer_->showCursor(); cursorTimer_->start(); }
	void moveEvent(QWidget const *parent) { fullModeToggler_->setScreen(parent); }
	void resizeEvent(QWidget const *parent);
	void focusOutEvent();
	void focusInEvent();
	void keyPressEvent(QKeyEvent *);
	void keyReleaseEvent(QKeyEvent *);

	bool tryLockFrameBuf(PixelBuffer &pb) {
		if (vbmut_.tryLock()) {
			pb = running_
			   ? blitterContainer_->blitter()->inBuffer()
			   : PixelBuffer();
			return true;
		}

		return false;
	}

	void unlockFrameBuf() { vbmut_.unlock(); }
	QSize const videoSize() const { return blitterContainer_->sourceSize(); }
	void setFrameTime(long num, long denom);
	void setSamplesPerFrame(long num, long denom = 1) { worker_->setSamplesPerFrame(Rational(num, denom)); }
	void frameStep();
	void pause(unsigned bitmask = 1) { pauser_.set(bitmask, *this); }
	void unpause(unsigned bitmask = 1) { pauser_.unset(bitmask, *this); }
	void incPause(int inc) { pauser_.inc(inc, *this); }
	void decPause(int dec) { pauser_.dec(dec, *this); }
	void setPauseOnFocusOut(unsigned bitmask, bool hasFocus);
	void run();
	void stop();
	bool isRunning() const { return running_; }
	void setAspectRatio(QSize const &ar) { blitterContainer_->setAspectRatio(ar); }
	void setScalingMethod(ScalingMethod smet) { blitterContainer_->setScalingMethod(smet); }

	BlitterConf blitterConf(std::size_t blitterNo) { return BlitterConf(blitters_[blitterNo]); }
	ConstBlitterConf blitterConf(std::size_t blitterNo) const { return ConstBlitterConf(blitters_[blitterNo]); }
	std::size_t numBlitters() const { return blitters_.size(); }
	BlitterConf currentBlitterConf() { return BlitterConf(blitterContainer_->blitter()); }
	ConstBlitterConf currentBlitterConf() const { return ConstBlitterConf(blitterContainer_->blitter()); }

	void setVideoBlitter(std::size_t blitterNo) {
		setVideoFormatAndBlitter(blitterContainer_->sourceSize(), blitterNo);
	}

	void setVideoFormat(QSize const &size) {
		setVideo(size, blitterContainer_->blitter());
	}

	void setVideoFormatAndBlitter(QSize const &size, std::size_t blitterNo) {
		setVideo(size, blitters_[blitterNo]);
	}

	void setFastForwardSpeed(int speed) { worker_->setFastForwardSpeed(speed); }

	void setMode(std::size_t screenNo, std::size_t resIndex, std::size_t rateIndex) {
		fullModeToggler_->setMode(screenNo, resIndex, rateIndex);
	}

	std::vector<ResInfo> const & modeVector(std::size_t screen) const { return fullModeToggler_->modeVector(screen); }
	QString const screenName(std::size_t screen) const { return fullModeToggler_->screenName(screen); }
	std::size_t screens() const { return fullModeToggler_->screens(); }
	std::size_t currentResIndex(std::size_t screen) const { return fullModeToggler_->currentResIndex(screen); }
	std::size_t currentRateIndex(std::size_t screen) const { return fullModeToggler_->currentRateIndex(screen); }
	std::size_t currentScreen() const { return fullModeToggler_->screen(); }
	QRect const fullScreenRect(QWidget const *w) const { return fullModeToggler_->fullScreenRect(w); }
	void setFullMode(bool fullscreen) { fullModeToggler_->setFullMode(fullscreen); }
	bool isFullMode() const { return fullModeToggler_->isFullMode(); }
	void parentExclusiveEvent(bool exclusive) { blitterContainer_->parentExclusiveEvent(exclusive); }

	AudioEngineConf audioEngineConf(std::size_t aeNo) { return AudioEngineConf(audioEngines_[aeNo]); }
	ConstAudioEngineConf audioEngineConf(std::size_t aeNo) const { return ConstAudioEngineConf(audioEngines_[aeNo]); }
	std::size_t numAudioEngines() const { return audioEngines_.size(); }

	void setAudioOut(std::size_t engineNo, long srateHz, int msecLatency, std::size_t resamplerNo) {
		worker_->setAudioOut(*audioEngines_[engineNo], srateHz, msecLatency, resamplerNo);
	}

	std::size_t numResamplers() const { return ResamplerInfo::num(); }
	char const * resamplerDesc(std::size_t resamplerNo) const { return ResamplerInfo::get(resamplerNo).desc; }

	void waitUntilPaused();

	template<class T>
	void callWhenPaused(T const &fun) {
		worker_->qPause();
		pausedq_.push(fun);
		execPausedQueue();
	}

	void resetAudio() { if (running_) { worker_->resetAudio(); } }
	void setDwmTripleBuffer(bool enable) { dwmControl_.setDwmTripleBuffer(enable); }
	void setFastForward(bool enable) { worker_->setFastForward(enable); }
	void setSyncToRefreshRate(bool on) { frameRateControl_.setRefreshRateSync(on); }

public slots:
	void hideCursor();

signals:
	void audioEngineFailure();
	void videoBlitterFailure();

protected:
	virtual void customEvent(QEvent *);

private:
	Q_OBJECT

	class Pauser {
	public:
		Pauser() : paused_(0) {}
		bool isPaused() const { return paused_; }
		void dec(int dec, MediaWidget &mw) { modifyPaused(paused_ - dec, mw); }
		void inc(int inc, MediaWidget &mw) { modifyPaused(paused_ + inc, mw); }
		void set(unsigned bm, MediaWidget &mw) { modifyPaused(paused_ | bm, mw); }
		void unset(unsigned bm, MediaWidget &mw) { modifyPaused(paused_ & ~bm, mw); }

	private:
		struct DoPause;
		struct DoUnpause;

		unsigned paused_;

		void modifyPaused(unsigned newPaused, MediaWidget &mw);
	};

	class JoystickIniter : Uncopyable {
	public:
		JoystickIniter();
		~JoystickIniter();

	private:
		std::vector<SDL_Joystick *> joysticks_;
	} const jsInit_;

	class WorkerCallback;
	struct FrameStepFun;

	QMutex vbmut_;
	BlitterContainer *const blitterContainer_;
	auto_vector<AudioEngine> const audioEngines_;
	auto_vector<BlitterWidget> const blitters_;
	scoped_ptr<FullModeToggler> const fullModeToggler_;
	scoped_ptr<WorkerCallback> const workerCallback_;
	MediaWorker *const worker_;
	FrameRateControl frameRateControl_;
	QTimer *const cursorTimer_;
	QTimer *const jsTimer_;
	CallQueue<> pausedq_;
	Pauser pauser_;
	DwmControl dwmControl_;
	unsigned focusPauseBit_;
	bool running_;

	friend class CallWhenMediaWorkerPaused;
	friend class PushMediaWorkerCall;
	void execPausedQueue();
	void setBlitter(BlitterWidget *blitter);
	void setVideo(QSize const &size, BlitterWidget *blitter);
	void updateSwapInterval();
	void emitVideoBlitterFailure() { emit videoBlitterFailure(); }
	void emitAudioEngineFailure();

private slots:
	void refreshRateChange(int refreshRate);
	void updateJoysticks();
};

#endif
