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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "callqueue.h"
#include "pixelbuffer.h"
#include "resinfo.h"
#include "scalingmethod.h"
#include "uncopyable.h"
#include <QMainWindow>
#include <vector>

class AudioEngineConf;
class BlitterConf;
class ConstAudioEngineConf;
class ConstBlitterConf;
class MediaWidget;
class MediaSource;
class MediaWorker;
class QSize;

/**
  * The MainWindow takes a MediaSource and presents the audio/video content
  * produced by it to the user.
  *
  * It calls MediaSource::update to request production of more audio/video content.
  * This happens in a worker thread that is separate from the main (GUI) thread.
  */
class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	/**
	  * Can be used to gain frame buffer access outside the MediaSource
	  * methods that take the frame buffer as argument.
	  */
	class FrameBuffer {
	public:
		explicit FrameBuffer(MainWindow &mw)
		: mw_(mw.w_)
		{
		}

		class Locked : Uncopyable {
		public:
			Locked(FrameBuffer fb);
			~Locked();
			PixelBuffer const & get() const { return pb_; }

		private:
			MediaWidget *mw_;
			PixelBuffer pb_;
		};

	private:
		MediaWidget *const mw_;
	};

	explicit MainWindow(MediaSource &source);

	/** Sets the duration in seconds of each video frame.
	  * Eg. use setFrameTime(1, 60) for 60 fps video.
	  * Can be used to speed things up or slow things down. Audio pitch will change accordingly.
	  */
	void setFrameTime(long num, long denom);

	/** Sets the mean number of audio samples produced by MediaSource::update for each video frame produced.
	  * So, for instance if you have 60 fps video, and 48 kHz audio, you would use setSamplesPerFrame(48000 / 60);
	  * or setSamplesPerFrame(48000, 60);
	  */
	void setSamplesPerFrame(long num, long denom = 1);

	/** Advance a single video frame forward. It only makes sense to use this when paused. */
	void frameStep();

	/** Pauses audio/video production. Will set the pause bits given by bitmask. Pauses when pause is not 0. */
	void pause(unsigned bitmask = 1);

	/** Resumes audio/video production. Will unset the pause bits given by bitmask. Unpauses when pause is 0. */
	void unpause(unsigned bitmask = 1);

	/** Pauses audio/video production. Will add inc to pause. Pauses when pause is not 0. */
	void incPause(int inc);

	/** Resumes audio/video production. Will subtract dec from pause. Unpauses when pause is 0. */
	void decPause(int dec);

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

	bool isRunning() const;

	/** The video format is the format of the video content produced by MediaSource.
	  * This will set the "frame buffer" to the requested format.
	  */
	void setVideoFormat(QSize const &size/*, PixelBuffer::PixelFormat pf*/);

	void setAspectRatio(QSize const &ar);
	void setScalingMethod(ScalingMethod smet);

	/**
	  * Sets the size of the video area of the MainWindow when not full screen.
	  * Should be used in place of methods like QMainWindow::resize, because this
	  * one does not screw up full screen.
	  *
	  * Pass an empty size for a variable size, such that the user may adjust
	  * the window size. Otherwise a fixed window size equal to size will be used.
	  * This window size does not include window borders or menus.
	  */
	void setWindowSize(QSize const &size);

	/** Each blitter has some properties that can be accessed through its BlitterConf. */
	BlitterConf blitterConf(std::size_t blitterNo);
	ConstBlitterConf blitterConf(std::size_t blitterNo) const;
	std::size_t numBlitters() const;
	BlitterConf currentBlitterConf();
	ConstBlitterConf currentBlitterConf() const;

	/** A video blitter is an engine (DirectDraw, Xv, etc.) responsible for putting video content on the screen. */
	void setVideoBlitter(std::size_t blitterNo);

	void setVideoFormatAndBlitter(QSize const &size,
			/*PixelBuffer::PixelFormat pf, */std::size_t blitterNo);

	/** speed = N, gives N times faster than normal when fastForward is enabled. */
	void setFastForwardSpeed(int speed);

	/** Sets the video mode that is used for full screen (see toggleFullScreen).
	  * A screen is basically a monitor. A different full screen mode can be selected for each screen.
	  * Which screen is used for full screen presentation depends on the location of the window.
	  * @param screenNo Screen number to select full screen mode for.
	  * @param resIndex Index of the selected resolution in the screens modeVector.
	  * @param rateIndex Index of the selected refresh rate for the selected resolution.
	  */
	void setFullScreenMode(std::size_t screenNo, std::size_t resIndex, std::size_t rateIndex);

	/** Returns the modes supported by each screen. */
	std::vector<ResInfo> const & modeVector(std::size_t screen) const;

	QString const screenName(std::size_t screen) const;

	/** Returns the number of screens. */
	std::size_t screens() const;

	/** Returns the current full screen resolution index selected for a screen. */
	std::size_t currentResIndex(std::size_t screen) const;

	/** Returns the current full screen rate index selected for a screen. */
	std::size_t currentRateIndex(std::size_t screen) const;

	/** Returns the current screen that will be used for full screen. */
	std::size_t currentScreen() const;

	/** Toggle full screen mode on/off. QMainWindow::isFullScreen() can be used to determine the current state.
	  * QMainWindow::setFullScreen should not be used.
	  */
	void toggleFullScreen();

	/** Each AudioEngine has some properties that can be accessed through its AudioEngineConf. */
	AudioEngineConf audioEngineConf(std::size_t aeNo);
	ConstAudioEngineConf audioEngineConf(std::size_t aeNo) const;
	std::size_t numAudioEngines() const;

	/** Audio resamplers of different performance can be selected. */
	std::size_t numResamplers() const;
	char const * resamplerDesc(std::size_t resamplerNo) const;

	/** Sets the AudioEngine (DirectSound, ALSA, etc.) to be used for audio output,
	  * as well as which output sampling rate, buffer size in milliseconds, and resampler to use.
	  * The sampling rate does not need to match the sampling rate of the audio content produced
	  * by the source, as the input will be converted to match the output rate.
	  */
	void setAudioOut(std::size_t engineNo, long srateHz, int msecLatency, std::size_t resamplerNo);

	/** Pause does not take effect immediately. Call this to wait until the worker thread is paused.
	  * Meant as a tool to simplify thread safety.
	  */
	void waitUntilPaused();

	/** Temporarily pauses the worker thread and calls fun once it has paused. Then unpauses.
	  * Returns before fun is actually called. Fun is called in an event at a later time.
	  * fun should implement operator() and have a copy-constructor.
	  * Meant as a tool to simplify thread safety.
	  */
	template<class T>
	void callWhenPaused(T const &fun);

	/** Puts fun into a queue of functors that are called in the worker thread at a later time.
	  * fun should implement operator() and have a copy-constructor.
	  * Meant as a tool to simplify thread safety.
	  * Generally you should prefer this to callWhenPaused, because callWhenPaused
	  * is more likely to cause audio underruns.
	  */
	template<class T>
	void callInWorkerThread(T const &fun);

	/** Discard buffered audio data */
	void resetAudio();

	void setDwmTripleBuffer(bool enable);
	static bool hasDwmCapability();
	static bool isDwmCompositionEnabled();

public slots:
	void hideCursor();
	void setFastForward(bool enable);

	/** Will synchronize frame rate to vertical retrace if the blitter supports a swapInterval of at least 1.
	  * Picks a swapInterval closest to the current frame rate.
	  * Literally speeds things up or slows things down to match the swapInterval. Audio pitch will change accordingly.
	  */
	void setSyncToRefreshRate(bool enable);

signals:
	void audioEngineFailure();
	void videoBlitterFailure();
	void closing();
	void dwmCompositionChange();

protected:
	virtual void mouseMoveEvent(QMouseEvent *);
	virtual void closeEvent(QCloseEvent *);
	virtual void moveEvent(QMoveEvent *);
	virtual void resizeEvent(QResizeEvent *);
	virtual void hideEvent(QHideEvent *);
	virtual void showEvent(QShowEvent *);
	virtual void focusOutEvent(QFocusEvent *);
	virtual void focusInEvent(QFocusEvent *);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
#ifdef Q_WS_WIN
	virtual bool winEvent(MSG *msg, long *result);
#endif

private:
	void correctFullScreenGeometry();
	void doSetWindowSize(QSize const &sz);
	void doShowFullScreen();

	MediaWidget *const w_;
	QSize winSize_;
	bool fullscreen_;
};

class CallWhenMediaWorkerPaused : Uncopyable {
	MediaWorker &worker_;
	CallQueue<> &callq_;
public:
	explicit CallWhenMediaWorkerPaused(MediaWidget &mw);
	~CallWhenMediaWorkerPaused();

	template<class T>
	void operator()(T const &function) const { callq_.push(function); }
};

template<class T>
void MainWindow::callWhenPaused(T const &fun) {
	CallWhenMediaWorkerPaused call(*w_);
	call(fun);
}

class PushMediaWorkerCall : Uncopyable {
	MediaWorker &worker_;
	CallQueue<> &callq_;
public:
	explicit PushMediaWorkerCall(MediaWidget &mw);
	~PushMediaWorkerCall();

	template<class T>
	void operator()(T const &function) const { callq_.push(function); }
};

template<class T>
void MainWindow::callInWorkerThread(T const &fun) {
	PushMediaWorkerCall pushMediaWorkerCall(*w_);
	pushMediaWorkerCall(fun);
}

#endif
