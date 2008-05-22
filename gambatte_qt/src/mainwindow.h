/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include <vector>
#include <map>
#include <memory>
#include "samplescalculator.h"
#include "SDL_Joystick/include/SDL_joystick.h"
#include "SDL_Joystick/include/SDL_event.h"
#include "mediasource.h"

class QSize;
class AudioEngine;
class BlitterWidget;
class VideoDialog;
class InputDialog;
class SoundDialog;
class FullModeToggler;
class BlitterContainer;
class JoyObserver;
class QTimer;

class MainWindow : public QMainWindow {
	Q_OBJECT
	
public:
	struct InputObserver {
		virtual ~InputObserver() {}
		virtual void pressEvent() {}
		virtual void releaseEvent() {}
	};

private:
	class ButtonHandler : public InputObserver {
		MediaSource *source;
		unsigned buttonIndex;
		
	public:
		ButtonHandler(MediaSource *source, unsigned buttonIndex);
		void pressEvent();
		void releaseEvent();
	};
	
	class JoystickIniter {
		std::vector<SDL_Joystick*> joysticks;
	public:
		JoystickIniter();
		~JoystickIniter();
	};
		
	typedef std::multimap<unsigned,InputObserver*> keymap_t;
	typedef std::multimap<unsigned,JoyObserver*> joymap_t;
	
	MediaSource *const source;
	std::vector<AudioEngine*> audioEngines;
	std::vector<BlitterWidget*> blitters;
	std::vector<ButtonHandler> buttonHandlers;
	keymap_t keyInputs;
	joymap_t joyInputs;

	BlitterContainer *blitterContainer;
	InputDialog *inputDialog;
	SoundDialog *soundDialog;
	VideoDialog *videoDialog;
	BlitterWidget *blitter;
	const std::auto_ptr<FullModeToggler> fullModeToggler;
	qint16 *sndBuffer;
	AudioEngine *ae;
	QTimer *cursorTimer;
	
	unsigned samplesPrFrame;
	unsigned ftNum;
	unsigned ftDenom;
	unsigned paused;
	int sampleRate;
	int timerId;
	
	SamplesCalculator samplesCalc;
	JoystickIniter joyIniter;

	bool running;
	bool turbo;
	bool pauseOnDialogExec;
	bool cursorHidden;
	
	void initAudio();
	void setSamplesPrFrame();
	void soundEngineFailure();
	void clearInputVectors();
	void pushInputObserver(const SDL_Event &data, InputObserver *observer);
	void updateJoysticks();
	void resetWindowSize(const QSize &s);
	void uninitBlitter();
	void doSetFrameTime(unsigned num, unsigned denom);
	void doPause();
	void doUnpause();
	void showCursor();
	void correctFullScreenGeometry();
	
private slots:
	void hideCursor();
	void inputSettingsChange();
	void soundSettingsChange();
	void videoSettingsChange();
	
protected:
	void timerEvent(QTimerEvent *event);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void closeEvent(QCloseEvent *e);
	void showEvent(QShowEvent *e);
	void moveEvent(QMoveEvent *e);
	void resizeEvent(QResizeEvent *e);
	void focusOutEvent(QFocusEvent *event);
	void focusInEvent(QFocusEvent *event);

public:
	/**
	  * @param buttonLabels Labels used in input dialog for buttons of corresponding index used by
	  *                     source->button[Press|Release]Event.
	  *
	  * @param buttonDefaults Default Qt::keys for buttons of corresponding index. Every button
	  *                       requires a default key, but you can have more default keys than
	  *                       labels, in which case the button won't show up in the input dialog.
	  *
	  * @param videoSourceInfos Names and dimensions of available video sources of corresponding
	  *                         index in source->setVideoSource. At least one video source required.
	  *                         Can be replaced using the setVideoSources method.
	  *
	  * @param videoSourceLabel Label used for the combo box for selecting video source in the video
	  *                         dialog.
	  *
	  * @param aspectRatio Aspect ratio of video output. Ratio = aspectRatio.width() / aspectRatio.height().
	  *                    Is also used as the base size for choosing window sizes listed by the video
	  *                    dialog (except when 'scale by integer factors only' is selected). Window sizes
	  *                    listed are multiples of the base size. Use bigger width and height values
	  *                    to get fewer window sizes listed. Can be changed later with the setAspectRatio
	  *                    method.
	  *
	  * @param sampleRates Sample rates (samples pr second) selectable in the sound dialog. At least
	  *                    one is required. Can be changed later with the setSampleRates method.
	  */
	MainWindow(MediaSource *source,
	           const std::vector<std::string> &buttonLabels,
	           const std::vector<int> &buttonDefaults,
	           const std::vector<MediaSource::VideoSourceInfo> &videoSourceInfos,
	           const std::string &videoSourceLabel,
	           const QSize &aspectRatio,
	           const std::vector<int> &sampleRates);
	
	~MainWindow();
	const QSize& aspectRatio() const;
	
	/**
	  * Convenience method for launching dialogs. Pauses source updates if pauseOnDialogExec is set.
	  */
	void execDialog(QDialog *dialog);
	
	bool isPaused() const { return paused & 1; }
	bool isRunning() const { return running; }
	bool isTurbo() const { return turbo; }
	bool pausesOnDialogExec() const { return pauseOnDialogExec; }
	void setAspectRatio(const QSize &aspectRatio);
	
	/**
	  * Sets time period in seconds between calls to source->update(). Eg. for 60 Hz updates
	  * use setFrameTime(1, 60). Too high values of numerator or denominator _may_ lead to overflows.
	  */
	void setFrameTime(unsigned numerator, unsigned denominator);
	
	/**
	  * When pauseOnDialogExec is set, source->update()s will be paused when a dialog is launched
	  * from MainWindow, and unpaused when the dialog is closed. pauseOnDialogExec is on by default.
	  */
	void setPauseOnDialogExec(bool enable) { pauseOnDialogExec = enable; }
	void setSampleRates(const std::vector<int> &sampleRates);
	void setVideoSources(const std::vector<MediaSource::VideoSourceInfo> &sourceInfos);
	
	/**
	  * Does a single source->update() if isRunning() && isPaused() (unless it's also paused by a dialog or similar)
	  */
	void frameStep();

public slots:
	/**
	  * Should be called once each video frame when the pixel buffer given through source->setPixelBuffer
	  * is ready to be read from. Avoids keeping two sw buffers when a source can't guarantee a
	  * complete buffer at the end of updates.
	  */
	void blit();
	
	// for launching settings dialogs.
	void execVideoDialog();
	void execInputDialog();
	void execSoundDialog();
	
	/**
	  * Pauses calls to source->update() (and audio output), while keeping the last blitted image
	  * visible.
	  */
	void pause();
	
	/**
	  * Initializes audio and video and starts source updates. Needed to start when in stopped
	  * (!isRunning()) state. Must be called at least once initially to start source updates.
	  */
	void run();
	
	/**
	  * Turbo is a fast-forward that can be seen as temporarily setting frame time to zero while
	  * pausing audio output.
	  */
	void setTurbo(bool enable);
	
	/**
	  * Stops source updates, and uninitializes audio and video.
	  */
	void stop();
	
	/**
	  * The only method you should use to set/unset fullScreen. Will set MainWindow to fullScreen state,
	  * and switch to a user selected full screen mode. Use of QWidget methods like showFullScreen and
	  * methods changing window size/geometry is probably a bad idea.
	  */
	void toggleFullScreen();
	
	/**
	  * Using any other way of hiding/showing the menuBar is not recommended. Add QMenus and QActions
	  * to menuBar() to create a menu. You may also want to add QActions to MainWindow to have shortcuts
	  * work while the menuBar is hidden.
	  */
	void toggleMenuHidden();
	void toggleTurbo();
	
	/**
	  * Resume source updates and audio output after pause.
	  */
	void unpause();
};

#endif
