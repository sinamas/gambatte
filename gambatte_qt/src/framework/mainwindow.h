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
#include "src/callqueue.h"
#include "src/mediaworker.h"
#include "src/blittercontainer.h"
#include "src/fullmodetoggler.h"
#include "SDL_joystick.h"
#include "resample/resamplerinfo.h"
#include "src/auto_vector.h"

class MediaSource;

/**
  * The MainWindow is one of the two main classes in this framework.
  * It takes a MediaSource and presents the audio/video content produced
  * by it to the user.
  *
  * It calls MediaSource::update to request production of more audio/video content.
  * This happens in a worker thread that is separate from the main (GUI) thread.
  */
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
	/** Can be used to gain frame buffer access outside the MediaSource
	  * methods that take the frame buffer as argument.
	  * The frame buffer is the buffer that a MediaSource should write
	  * its video content to. It's generally not an actual video memory frame buffer.
	  */
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
	
	/** Sets the duration in seconds of each video frame.
	  * Eg. use setFrameTime(1, 60) for 60 fps video.
	  * Can be used to speed things up or slow things down. Audio pitch will change accordingly.
	  */
	void setFrameTime(unsigned num, unsigned denom);
	
	/** Sets the mean number of audio samples produced by MediaSource::update for each video frame produced.
	  * So, for instance if you have 60 fps video, and 48 kHz audio, you would use setSamplesPerFrame(48000 / 60);
	  * or setSamplesPerFrame(48000, 60);
	  */
	void setSamplesPerFrame(long num, long denom = 1);
	
	/** Advance a single video frame forward. It only makes sense to use this when paused. */
	void frameStep();
	
	/** Pauses audio/video production. Will set the pauseVar bits given by bitmask. Pauses when pauseVar is not 0. */
	void pause(unsigned bitmask = 1) { pauser.set(bitmask, this); }
	
	/** Resumes audio/video production. Will unset the pauseVar bits given by bitmask. Unpauses when pauseVar is 0. */
	void unpause(unsigned bitmask = 1) { pauser.unset(bitmask, this); }
	
	/** Pauses audio/video production. Will add inc to pauseVar. Pauses when pauseVar is not 0. */
	void incPause(unsigned inc) { pauser.inc(inc, this); }
	
	/** Resumes audio/video production. Will subtract dec from pauseVar. Unpauses when pauseVar is 0. */
	void decPause(unsigned dec) { pauser.dec(dec, this); }
	
	/** Pauses audio/video production automatically when the central widget of the MainWindow loses focus.
	  * Calls pause(bitmask) on focusOut, and unpause(bitmask) on focusIn.
	  */
	void setPauseOnFocusOut(unsigned bitmask);
	
	/** Run must be called once to initialize resources and start audio/video production. */
	void run();
	
	/** Stop can be called to release resources and stop audio/video production.
	  * Run will have to be called again to resume. Blocks until stopped.
	  */
	void stop();
	
	bool isRunning() const { return running; }
	
	/** The video format is the format of the video content produced by MediaSource.
	  * This will set the "frame buffer" to the requested format.
	  */
	void setVideoFormat(unsigned w, unsigned h/*, PixelBuffer::PixelFormat pf*/) {
		setVideo(w, h, /*pf, */blitterContainer->blitter());
	}
	
	void setAspectRatio(const QSize &ar) { blitterContainer->setAspectRatio(ar); }
	void setScalingMethod(ScalingMethod smet) { blitterContainer->setScalingMethod(smet); }
	
	/** Sets the size of the presentation area of the MainWindow when not full screen.
	  * Should be used in place of methods like QMainWindow::resize, because this one doesn't screw up full screen.
	  * Pass sz = QSize(-1,-1) for a variable size, such that the user may adjust the window size.
	  * Otherwise a fixed window size equal to sz will be used. This window size does not include window borders or menus.
	  */
	void setWindowSize(const QSize &sz);
	
	/** Each blitter has some properties that can be accessed through its BlitterConf. */
	const BlitterConf blitterConf(unsigned blitterNo) { return BlitterConf(blitters[blitterNo]); }
	const ConstBlitterConf blitterConf(unsigned blitterNo) const { return ConstBlitterConf(blitters[blitterNo]); }
	unsigned numBlitters() const { return blitters.size(); }
	const BlitterConf currentBlitterConf();
	const ConstBlitterConf currentBlitterConf() const;
	
	/** A video blitter is an engine (DirectDraw, Xv, etc.) responsible for putting video content on the screen. */
	void setVideoBlitter(unsigned blitterNo) {
		setVideoFormatAndBlitter(imageFormat.width, imageFormat.height/*, imageFormat.pixelFormat*/, blitterNo);
	}
	
	void setVideoFormatAndBlitter(unsigned w, unsigned h,
			/*PixelBuffer::PixelFormat pf, */unsigned blitterNo)
	{
		setVideo(w, h,/* pf,*/ blitters[blitterNo]);
	}
	
	/** Will synchronize frame rate to vertical retrace if the blitter supports a swapInterval of at least 1.
	  * Picks a swapInterval closest to the current frame rate.
	  * Literally speeds things up or slows things down to match the swapInterval. Audio pitch will change accordingly.
	  */
	void setSyncToRefreshRate(bool on);
	
	/** speed = N, gives N times faster than normal when fastForward is enabled. */
	void setFastForwardSpeed(unsigned speed) { worker->setFastForwardSpeed(speed); }
	
	/** Sets the video mode that is used for full screen (see toggleFullScreen).
	  * A screen is basically a monitor. A different full screen mode can be selected for each screen.
	  * Which screen is used for full screen presentation depends on the location of the window.
	  * @param screenNo Screen number to select full screen mode for.
	  * @param resIndex Index of the selected resolution in the screens modeVector.
	  * @param rateIndex Index of the selected refresh rate for the selected resolution.
	  */
	void setFullScreenMode(unsigned screenNo, unsigned resIndex, unsigned rateIndex);
	
	/** Returns the modes supported by each screen. */
	const std::vector<ResInfo>& modeVector(unsigned screen) const { return fullModeToggler->modeVector(screen); }
	
	/** Returns the number of screens. */
	unsigned screens() const { return fullModeToggler->screens(); }
	
	/** Returns the current full screen resolution index selected for a screen. */
	unsigned currentResIndex(unsigned screen) const { return fullModeToggler->currentResIndex(screen); }
	
	/** Returns the current full screen rate index selected for a screen. */
	unsigned currentRateIndex(unsigned screen) const { return fullModeToggler->currentRateIndex(screen); }
	
	/** Returns the current screen that will be used for full screen. */
	unsigned currentScreen() const { return fullModeToggler->screen(); }
	
	/** Toggle full screen mode on/off. QMainWindow::isFullScreen() can be used to determine the current state.
	  * QMainWindow::setFullScreen should not be used.
	  */
	void toggleFullScreen();
	
	/** Each AudioEngine has some properties that can be accessed through its AudioEngineConf. */
	const AudioEngineConf audioEngineConf(unsigned aeNo) { return AudioEngineConf(audioEngines[aeNo]); }
	const ConstAudioEngineConf audioEngineConf(unsigned aeNo) const { return ConstAudioEngineConf(audioEngines[aeNo]); }
	unsigned numAudioEngines() const { return audioEngines.size(); }
	
	/** Sets the AudioEngine (DirectSound, ALSA, etc.) to be used for audio output,
	  * as well as which output sampling rate and buffer size in milliseconds to use.
	  * This does not need to match the sampling rate of the audio content produced
	  * by the source, as it will be converted to match the output rate.
	  */
	void setAudioOut(unsigned engineNo, unsigned srateHz, unsigned msecLatency);
	
	/** A few different audio resamplers of different performance can be selected between. */
	unsigned numResamplers() const { return ResamplerInfo::num(); }
	const char* resamplerDesc(unsigned resamplerNo) const { return ResamplerInfo::get(resamplerNo).desc; }
	void setResampler(unsigned resamplerNo);
	
	/** Pause doesn't take effect immediately. Call this to wait until the worker thread is paused.
	  * Meant as a tool to simplify thread safety.
	  */
	void waitUntilPaused();
	
	/** Temporarily pauses the worker thread and calls fun once it has paused. Then unpauses.
	  * Returns before fun is actually called. Fun is called in an event at a later time.
	  * fun should implement operator() and have a copy-constructor.
	  * Meant as a tool to simplify thread safety.
	  */
	template<class T> void callWhenPaused(const T& fun);
	
	/** Puts fun into a queue of functors that are called in the worker thread at a later time.
	  * fun should implement operator() and have a copy-constructor.
	  * Meant as a tool to simplify thread safety.
	  * Generally you should prefer this to callWhenPaused, because callWhenPaused
	  * is more likely to cause audio underruns.
	  */
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
