/***************************************************************************
 *   Copyright (C) 2007-2009 by Sindre Aamås                               *
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
#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <memory>
#include <vector>
#include <QMutex>
#include <QSize>
#include <QTimer>
#include "uncopyable.h"
#include "SDL_joystick.h"
#include "resample/resamplerinfo.h"
#include "blittercontainer.h"
#include "blitterwidget.h"
#include "frameratecontrol.h"
#include "fullmodetoggler.h"
#include "mediaworker.h"
#include "audioengineconf.h"
#include "blitterconf.h"
#include "resinfo.h"
#include "scalingmethod.h"
#include "auto_vector.h"
#include "callqueue.h"
#include "pixelbuffer.h"
#include "dwmcontrol.h"

class QMainWindow;

class MediaWidget : public QObject {
	Q_OBJECT
	
	class Pauser {
		struct DoPause;
		struct DoUnpause;
		
		unsigned paused;
		
		void modifyPaused(unsigned newPaused, MediaWidget &mw);
	public:
		Pauser() : paused(0) {}
		bool isPaused() const { return paused; }
		void dec(unsigned dec, MediaWidget &mw) { modifyPaused(paused - dec, mw); }
		void inc(unsigned inc, MediaWidget &mw) { modifyPaused(paused + inc, mw); }
		void set(unsigned bm, MediaWidget &mw) { modifyPaused(paused | bm, mw); }
		void unset(unsigned bm, MediaWidget &mw) { modifyPaused(paused & ~bm, mw); }
	};
	
	const class JoystickIniter : Uncopyable {
		std::vector<SDL_Joystick*> joysticks;
	public:
		JoystickIniter();
		~JoystickIniter();
	} jsInit_;
	
	class WorkerCallback;
	struct FrameStepFun;
	
	QMutex vbmut;
	BlitterContainer *const blitterContainer;
	const auto_vector<AudioEngine> audioEngines;
	const auto_vector<BlitterWidget> blitters;
	const std::auto_ptr<FullModeToggler> fullModeToggler;
	WorkerCallback *const workerCallback_;
	MediaWorker *const worker;
	FrameRateControl frameRateControl;
	QTimer *const cursorTimer;
	QTimer *const jsTimer;
	CallQueue<> pausedq;
	Pauser pauser;
	DwmControl dwmControl_;
	unsigned focusPauseBit;
	bool running;
	
	friend class CallWhenMediaWorkerPaused;
	friend class PushMediaWorkerCall;
	virtual void customEvent(QEvent*);
	void execPausedQueue();
	void setBlitter(BlitterWidget *blitter);
	void setVideo(unsigned w, unsigned h, BlitterWidget *blitter);
	void updateSwapInterval();
	void emitVideoBlitterFailure() { emit videoBlitterFailure(); }
	void emitAudioEngineFailure();
	
private slots:
	void refreshRateChange(int refreshRate);
	void updateJoysticks();
	
public:
	MediaWidget(MediaSource *source, QWidget &parent);
	~MediaWidget();
	
	QWidget* widget() const { return blitterContainer; }
	
	/** @return compositionChange */
	bool winEvent(const void *message) { return dwmControl_.winEvent(message); }
	void hideEvent() { dwmControl_.hideEvent(); }
	void showEvent(const QWidget *parent) { dwmControl_.showEvent(); fullModeToggler->setScreen(parent); }
	void mouseMoveEvent() { blitterContainer->showCursor(); cursorTimer->start(); }
	void moveEvent(const QWidget *parent) { fullModeToggler->setScreen(parent); }
	void resizeEvent(const QWidget *parent);
	void focusOutEvent();
	void focusInEvent();
	void keyPressEvent(QKeyEvent*);
	void keyReleaseEvent(QKeyEvent*);
	
	bool tryLockFrameBuf(PixelBuffer &pb) {
		if (vbmut.tryLock()) {
			pb = running ? blitterContainer->blitter()->inBuffer() : PixelBuffer();
			return true;
		}
		
		return false;
	}
	
	void unlockFrameBuf() { vbmut.unlock(); }
	
	const QSize& videoSize() const { return blitterContainer->sourceSize(); }
	
	void setFrameTime(long num, long denom);
	void setSamplesPerFrame(long num, long denom = 1) { worker->setSamplesPerFrame(Rational(num, denom)); }
	
	void frameStep();
	void pause(unsigned bitmask = 1) { pauser.set(bitmask, *this); }
	void unpause(unsigned bitmask = 1) { pauser.unset(bitmask, *this); }
	void incPause(unsigned inc) { pauser.inc(inc, *this); }
	void decPause(unsigned dec) { pauser.dec(dec, *this); }
	void setPauseOnFocusOut(unsigned bitmask, bool hasFocus);
	
	void run();
	void stop();
	bool isRunning() const { return running; }
	
	void setAspectRatio(const QSize &ar) { blitterContainer->setAspectRatio(ar); }
	void setScalingMethod(ScalingMethod smet) { blitterContainer->setScalingMethod(smet); }
	
	const BlitterConf blitterConf(std::size_t blitterNo) { return BlitterConf(blitters[blitterNo]); }
	const ConstBlitterConf blitterConf(std::size_t blitterNo) const { return ConstBlitterConf(blitters[blitterNo]); }
	std::size_t numBlitters() const { return blitters.size(); }
	const BlitterConf currentBlitterConf() { return BlitterConf(blitterContainer->blitter()); }
	const ConstBlitterConf currentBlitterConf() const { return ConstBlitterConf(blitterContainer->blitter()); }
	
	void setVideoBlitter(std::size_t blitterNo) {
		setVideoFormatAndBlitter(blitterContainer->sourceSize().width(),
		                         blitterContainer->sourceSize().height(), blitterNo);
	}
	
	void setVideoFormat(unsigned w, unsigned h/*, PixelBuffer::PixelFormat pf*/) {
		setVideo(w, h, /*pf, */blitterContainer->blitter());
	}
	
	void setVideoFormatAndBlitter(unsigned w, unsigned h,
			/*PixelBuffer::PixelFormat pf, */std::size_t blitterNo) {
		setVideo(w, h,/* pf,*/ blitters[blitterNo]);
	}
	
	void setFastForwardSpeed(unsigned speed) { worker->setFastForwardSpeed(speed); }
	void setMode(std::size_t screenNo, std::size_t resIndex, std::size_t rateIndex) { fullModeToggler->setMode(screenNo, resIndex, rateIndex); }
	const std::vector<ResInfo>& modeVector(std::size_t screen) const { return fullModeToggler->modeVector(screen); }
	const QString screenName(std::size_t screen) const { return fullModeToggler->screenName(screen); }
	std::size_t screens() const { return fullModeToggler->screens(); }
	std::size_t currentResIndex(std::size_t screen) const { return fullModeToggler->currentResIndex(screen); }
	std::size_t currentRateIndex(std::size_t screen) const { return fullModeToggler->currentRateIndex(screen); }
	std::size_t currentScreen() const { return fullModeToggler->screen(); }
	const QRect fullScreenRect(const QWidget *w) const { return fullModeToggler->fullScreenRect(w); }
	void setFullMode(bool fullscreen) { fullModeToggler->setFullMode(fullscreen); }
	bool isFullMode() const { return fullModeToggler->isFullMode(); }
	void parentExclusiveEvent(bool exclusive) { blitterContainer->parentExclusiveEvent(exclusive); }
	
	const AudioEngineConf audioEngineConf(std::size_t aeNo) { return AudioEngineConf(audioEngines[aeNo]); }
	const ConstAudioEngineConf audioEngineConf(std::size_t aeNo) const { return ConstAudioEngineConf(audioEngines[aeNo]); }
	std::size_t numAudioEngines() const { return audioEngines.size(); }
	
	void setAudioOut(std::size_t engineNo, unsigned srateHz, unsigned msecLatency) {
		worker->setAudioOut(audioEngines[engineNo], srateHz, msecLatency);
	}
	
	std::size_t numResamplers() const { return ResamplerInfo::num(); }
	const char* resamplerDesc(std::size_t resamplerNo) const { return ResamplerInfo::get(resamplerNo).desc; }
	void setResampler(std::size_t resamplerNo) { worker->setResampler(resamplerNo); }
	
	void waitUntilPaused();
	
	template<class T>
	void callWhenPaused(const T &fun) {
		worker->qPause();
		pausedq.push(fun);
		execPausedQueue();
	}

	void resetAudio() { if (running) { worker->resetAudio(); } }
	void setDwmTripleBuffer(bool enable) { dwmControl_.setDwmTripleBuffer(enable); }
	void setFastForward(bool enable) { worker->setFastForward(enable); }
	void setSyncToRefreshRate(bool on) { frameRateControl.setRefreshRateSync(on); }
	
public slots:
	void hideCursor();
	
signals:
	void audioEngineFailure();
	void videoBlitterFailure();
};

#endif
