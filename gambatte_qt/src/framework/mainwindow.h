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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <vector>
#include <QMutex>
#include <QSize>
#include "audioengineconf.h"
#include "blitterconf.h"
#include "uncopyable.h"
#include "callqueue.h"
#include "mediaworker.h"
#include "blittercontainer.h"
#include "fullmodetoggler.h"
#include "SDL_Joystick/include/SDL_joystick.h"
#include "resample/resamplerinfo.h"
#include "auto_vector.h"

class MediaSource;

class MainWindow : public QMainWindow {
	Q_OBJECT
	
	class Pauser {
		struct DoPause;
		struct DoUnpause;
		
		unsigned paused;
		
		void modifyPaused(unsigned newPaused, MainWindow *mw);
	public:
		Pauser() : paused(0) {}
		bool isPaused() const { return paused; }
		void dec(unsigned dec, MainWindow *mw) { modifyPaused(paused - dec, mw); }
		void inc(unsigned inc, MainWindow *mw) { modifyPaused(paused + inc, mw); }
		void set(unsigned bm, MainWindow *mw) { modifyPaused(paused | bm, mw); }
		void unset(unsigned bm, MainWindow *mw) { modifyPaused(paused & ~bm, mw); }
	};
	
	struct ImageFormat {
		unsigned width, height;
// 		PixelBuffer::PixelFormat pixelFormat;
		ImageFormat(unsigned width = 0, unsigned height = 0/*,
		            PixelBuffer::PixelFormat pixelFormat = PixelBuffer::RGB32*/) : width(width), height(height)/*, pixelFormat(pixelFormat)*/ {}
	};
	
	class JoystickIniter {
		std::vector<SDL_Joystick*> joysticks;
	public:
		JoystickIniter();
		~JoystickIniter();
	};
	
	typedef CallQueue<MainWindow*> PausedQ;
	class WorkerCallback;
	struct FrameStepFun;
	
	MediaWorker *const worker;
	BlitterContainer *const blitterContainer;
	auto_vector<AudioEngine> audioEngines;
	auto_vector<BlitterWidget> blitters;
	const std::auto_ptr<FullModeToggler> fullModeToggler;
	QTimer *const cursorTimer;
	QTimer *const jsTimer;
	PausedQ pausedq;
	Pauser pauser;
	JoystickIniter joyIniter;
	ImageFormat imageFormat;
	QMutex vbmut;
	QSize windowSize;
	Rational frameTime_;
	unsigned focusPauseBit;
// 	int timerId;
	int hz;
	bool running;
// 	bool threaded;
	bool refreshRateSync;
	bool cursorHidden;
	
	void mouseMoveEvent(QMouseEvent *e);
	void customEvent(QEvent *ev);
	void closeEvent(QCloseEvent *e);
	void moveEvent(QMoveEvent */*event*/);
	void resizeEvent(QResizeEvent */*event*/);
	void showEvent(QShowEvent *e);
	void focusOutEvent(QFocusEvent */*event*/);
	void focusInEvent(QFocusEvent */*event*/);
	void keyPressEvent(QKeyEvent*);
	void keyReleaseEvent(QKeyEvent*);
	void rebuildVideoChain();
	void execPausedQueue();
	void updateMinimumSize();
	void setBlitter(BlitterWidget *blitter);
	void setVideo(unsigned w, unsigned h, /*PixelBuffer::PixelFormat pf,*/ /*const VideoFilter *vf, */BlitterWidget *blitter);
	void correctFullScreenGeometry();
	void doSetWindowSize(const QSize &sz);
	void updateSwapInterval();
	void showCursor();
	void emitVideoBlitterFailure() { emit videoBlitterFailure(); }
	void emitAudioEngineFailure() { emit audioEngineFailure(); worker->recover(); }
	
private slots:
	void hzChange(int hz);
	void updateJoysticks();
	
public:
	// Can be used to gain frame buffer access outside the MediaSource
	// methods that take the frame buffer as argument.
	class FrameBuffer {
		MainWindow *const mw;
	public:
		FrameBuffer(MainWindow *const mw) : mw(mw) {}
		
		class Locked : Uncopyable {
			MainWindow *mw;
			PixelBuffer pb;
		public:
			Locked(FrameBuffer fb);
			~Locked();
			const PixelBuffer& get() const { return pb; }
		};
	};

	MainWindow(MediaSource *source);
	
	bool isRunning() const { return running; }
	void setAspectRatio(const QSize &ar) { blitterContainer->setAspectRatio(ar); }
	void setScalingMethod(ScalingMethod smet) { blitterContainer->setScalingMethod(smet); }
	
	// sz = QSize(-1,-1) for variable width.
	void setWindowSize(const QSize &sz);
	
	// Each blitter has some properties that can be accessed through its BlitterConf.
	const BlitterConf blitterConf(unsigned blitterNo) { return BlitterConf(blitters[blitterNo]); }
	const ConstBlitterConf blitterConf(unsigned blitterNo) const { return ConstBlitterConf(blitters[blitterNo]); }
	unsigned numBlitters() const { return blitters.size(); }
	const BlitterConf currentBlitterConf();
	const ConstBlitterConf currentBlitterConf() const;
	
	void setVideoFormatAndBlitter(unsigned w, unsigned h,
			/*PixelBuffer::PixelFormat pf, */unsigned blitterNo)
	{
		setVideo(w, h,/* pf,*/ blitters[blitterNo]);
	}
	
	void setVideoFormat(unsigned w, unsigned h/*, PixelBuffer::PixelFormat pf*/) {
		setVideo(w, h, /*pf, */blitterContainer->blitter());
	}
	
	void setVideoBlitter(unsigned blitterNo) {
		setVideoFormatAndBlitter(imageFormat.width, imageFormat.height/*, imageFormat.pixelFormat*/, blitterNo);
	}
	
	// Will synchronize frame rate to vertical retrace if the blitter supports a swapInterval of at least 1.
	// Picks a swapInterval closest to the current frame rate.
	// Literally speeds things up or slows things down to match the swapInterval. Audio pitch will change accordingly.
	void setSyncToRefreshRate(bool on);
	
	// speed = N, gives N times faster than normal.
	void setFastForwardSpeed(unsigned speed) { worker->setFastForwardSpeed(speed); }
	
	// These can be used to configure the video mode that is used for full screen (see toggleFullScreen).
	void setFullScreenMode(unsigned screenNo, unsigned resIndex, unsigned rateIndex);
	const std::vector<ResInfo>& modeVector(unsigned screen) const { return fullModeToggler->modeVector(screen); }
	unsigned screens() const { return fullModeToggler->screens(); }
	unsigned currentResIndex(unsigned screen) const { return fullModeToggler->currentResIndex(screen); }
	unsigned currentRateIndex(unsigned screen) const { return fullModeToggler->currentRateIndex(screen); }
	unsigned currentScreen() const { return fullModeToggler->screen(); }
	void toggleFullScreen();
	
	// Each AudioEngine has some properties that can be accessed through its AudioEngineConf.
	const AudioEngineConf audioEngineConf(unsigned aeNo) { return AudioEngineConf(audioEngines[aeNo]); }
	const ConstAudioEngineConf audioEngineConf(unsigned aeNo) const { return ConstAudioEngineConf(audioEngines[aeNo]); }
	unsigned numAudioEngines() const { return audioEngines.size(); }
	void setAudioOut(unsigned engineNo, unsigned srateHz, unsigned msecLatency);
	
	unsigned numResamplers() const { return ResamplerInfo::num(); }
	const char* resamplerDesc(unsigned resamplerNo) const { return ResamplerInfo::get(resamplerNo).desc; }
	void setResampler(unsigned resamplerNo);
	
	void setFrameTime(unsigned num, unsigned denom);
	void setSamplesPerFrame(long num, long denom = 1);
	void frameStep();
	
	void pause(unsigned bitmask = 1) { pauser.set(bitmask, this); }
	void unpause(unsigned bitmask = 1) { pauser.unset(bitmask, this); }
	void incPause(unsigned inc) { pauser.inc(inc, this); }
	void decPause(unsigned dec) { pauser.dec(dec, this); }
	void setPauseOnFocusOut(unsigned bitmask);
	
	// Pause doesn't take effect immediately. Call this to wait until the worker thread is paused.
	void waitUntilPaused();
	
	void run();
	void stop();
	
	// Temporarily pauses the worker thread and calls fun once it has paused. Then unpauses.
	// Returns before fun is actually called. Fun is called in an event at a later time.
	// fun should implement operator() and have a copy-constructor.
	template<class T> void callWhenPaused(const T& fun);
	
	// Puts fun into a queue of functors that are called in the worker thread at a later time.
	// fun should implement operator() and have a copy-constructor.
	template<class T> void callInWorkerThread(const T& fun) { worker->pushCall(fun); }

public slots:
	void hideCursor();
	void setFastForward(bool enable) { worker->setFastForward(enable); }
	
signals:
	void audioEngineFailure();
	void videoBlitterFailure();
	void closing();
};

template<class T>
void MainWindow::callWhenPaused(const T& fun) {
	worker->qPause();
	pausedq.push(fun);
	execPausedQueue();
}

#endif
