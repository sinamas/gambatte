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
#include "mainwindow.h"

// #include <iostream>

#include <QtGui>
#include <cstring>
#include <cassert>
#include <QApplication>
#include <QTimer>

#include "blitterwidgets/qpainterblitter.h"
#include "blitterwidgets/qglblitter.h"
#include "videodialog.h"
#include "inputdialog.h"
#include "sounddialog.h"
#include "blittercontainer.h"

#include "addaudioengines.h"
#include "addblitterwidgets.h"
#include "getfullmodetoggler.h"
#include "audioengine.h"
#include "audioengines/nullaudioengine.h"
#include "fullmodetoggler.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#endif

#include "SDL_Joystick/include/SDL_joystick.h"

MainWindow::ButtonHandler::ButtonHandler(MediaSource *source, unsigned buttonIndex) : source(source), buttonIndex(buttonIndex) {}
void MainWindow::ButtonHandler::pressEvent() { source->buttonPressEvent(buttonIndex); }
void MainWindow::ButtonHandler::releaseEvent() { source->buttonReleaseEvent(buttonIndex); }

class JoyObserver {
	MainWindow::InputObserver *const observer;
	
protected:
	void notifyObserver(bool press) {
		if (press)
			observer->pressEvent();
		else
			observer->releaseEvent();
	}
	
public:
	JoyObserver(MainWindow::InputObserver *observer) : observer(observer) {}
	virtual ~JoyObserver() {}
	virtual void valueChanged(int value) = 0;
};

class JoyAxisHandler : public JoyObserver {
	const int threshold;
public:
	JoyAxisHandler(MainWindow::InputObserver *observer, const int threshold) : JoyObserver(observer), threshold(threshold) {}
	void valueChanged(const int value) { notifyObserver((value - threshold ^ threshold) >= 0); }
};

class JoyButHandler : public JoyObserver {
public:
	JoyButHandler(MainWindow::InputObserver *observer) : JoyObserver(observer) {}
	void valueChanged(const int value) { notifyObserver(value); }
};

class JoyHatHandler : public JoyObserver {
	const int mask;
public:
	JoyHatHandler(MainWindow::InputObserver *observer, const int mask) : JoyObserver(observer), mask(mask) {}
	void valueChanged(const int value) { notifyObserver((value & mask) == mask); }
};

MainWindow::JoystickIniter::JoystickIniter() {
	SDL_JoystickInit();
	
	SDL_SetEventFilter(SDL_JOYAXISMOTION | SDL_JOYBUTTONCHANGE | SDL_JOYHATMOTION);
	
	const int numJs = SDL_NumJoysticks();
	
	for (int i = 0; i < numJs; ++i) {
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		
		if (joy)
			joysticks.push_back(joy);
	}
	
	SDL_JoystickUpdate();
	SDL_ClearEvents();
}

MainWindow::JoystickIniter::~JoystickIniter() {
	for (std::size_t i = 0; i < joysticks.size(); ++i)
		SDL_JoystickClose(joysticks[i]);
	
	SDL_JoystickQuit();
}

MainWindow::MainWindow(MediaSource *source,
                       const std::vector<std::string> &buttonLabels,
                       const std::vector<int> &buttonDefaults,
                       const std::vector<MediaSource::VideoSourceInfo> &videoSourceInfos,
                       const std::string &videoSourceLabel,
                       const QSize &aspectRatio,
                       const std::vector<int> &sampleRates) :
	source(source),
	buttonHandlers(buttonDefaults.size(), ButtonHandler(0, 0)),
	blitter(NULL),
	fullModeToggler(getFullModeToggler(winId())),
	sndBuffer(NULL),
	ae(NULL),
	cursorTimer(NULL),
	samplesPrFrame(0),
	ftNum(1),
	ftDenom(60),
	paused(0),
	sampleRate(0),
	timerId(0),
	running(false),
	turbo(false),
	pauseOnDialogExec(true),
	cursorHidden(false)
{
	assert(!videoSourceInfos.empty());
	
	for (unsigned i = 0; i < buttonHandlers.size(); ++i)
		buttonHandlers[i] = ButtonHandler(source, i);
	
	setAttribute(Qt::WA_DeleteOnClose);

	setFocusPolicy(Qt::StrongFocus);
	
	{
		QSettings settings;
		settings.beginGroup("mainwindow");
		resize(settings.value("size", QSize(160, 144)).toSize());
		settings.endGroup();
	}
	
	addAudioEngines(audioEngines, winId());
	audioEngines.push_back(new NullAudioEngine);
	soundDialog = new SoundDialog(audioEngines, sampleRates, this);
	connect(soundDialog, SIGNAL(accepted()), this, SLOT(soundSettingsChange()));
	
	inputDialog = new InputDialog(buttonLabels, buttonDefaults, this);
	connect(inputDialog, SIGNAL(accepted()), this, SLOT(inputSettingsChange()));

	addBlitterWidgets(blitters, PixelBufferSetter(source));
	blitters.push_back(new QGLBlitter(PixelBufferSetter(source)));
	blitters.push_back(new QPainterBlitter(PixelBufferSetter(source)));
	
	for (std::vector<BlitterWidget*>::iterator it = blitters.begin(); it != blitters.end();) {
		if ((*it)->isUnusable()) {
			delete *it;
			it = blitters.erase(it);
		} else
			++it;
	}

	videoDialog = new VideoDialog(blitters, videoSourceInfos, videoSourceLabel, fullModeToggler.get(), aspectRatio, this);
	connect(videoDialog, SIGNAL(accepted()), this, SLOT(videoSettingsChange()));
	
	blitterContainer = new BlitterContainer(videoDialog, this);
	blitterContainer->setMinimumSize(160, 144);
	setCentralWidget(blitterContainer);
	
	source->setPixelBuffer(NULL, MediaSource::RGB32, 0);
	
	videoSettingsChange();
	inputSettingsChange();
	soundSettingsChange();
	
	setFrameTime(ftNum, ftDenom);
	
	cursorTimer = new QTimer(this);
	cursorTimer->setSingleShot(true);
	cursorTimer->setInterval(2500);
	connect(cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));
	setMouseTracking(true);
	setFocus();
}

MainWindow::~MainWindow() {
	clearInputVectors();
	
	for (uint i = 0; i < blitters.size(); ++i)
		delete blitters[i];
	
	for (uint i = 0; i < audioEngines.size(); ++i)
		delete audioEngines[i];
	
	delete []sndBuffer;
}

void MainWindow::resetWindowSize(const QSize &s) {
	if (isFullScreen() || !isVisible())
		return;
	
	if (s == QSize(-1, -1)) {
		centralWidget()->setMinimumSize(videoDialog->sourceSize());
		layout()->setSizeConstraint(QLayout::SetMinimumSize);
		setMinimumSize(1, 1); // needed on macx
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX); // needed on macx. needed for metacity full screen switching, layout()->setSizeConstraint(QLayout::SetMinAndMaxSize) won't do.
		resize(size()); // needed on macx
	} else {
		centralWidget()->setMinimumSize(s.width(), s.height());
		layout()->setSizeConstraint(QLayout::SetFixedSize);
// 		setFixedSize(s.width(), s.height() + (menuBar()->isVisible() ? menuBar()->height() : 0));
	}
}

static void saveWindowSize(const QSize &s) {
	QSettings settings;
	settings.beginGroup("mainwindow");
	settings.setValue("size", s);
	settings.endGroup();
}

void MainWindow::correctFullScreenGeometry() {
	const QRect &screenRect = fullModeToggler->fullScreenRect(this);
	
	if (geometry() != screenRect) {
		setGeometry(screenRect);
	}
}

void MainWindow::toggleFullScreen() {
	if (isFullScreen()) {
		blitterContainer->parentExclusiveEvent(false);
		fullModeToggler->setFullMode(false);
		showNormal();
		resetWindowSize(videoDialog->winRes());
		activateWindow();
	} else {
		const int screen = QApplication::desktop()->screenNumber(this);
		
		fullModeToggler->setFullMode(true);
		saveWindowSize(size());
		resetWindowSize(QSize(-1, -1));
		
		// If the window is outside the screen it will be moved to the primary screen by Qt.
		{
			const QRect &rect = QApplication::desktop()->screenGeometry(screen);
			QPoint p(pos());
			
			if (p.x() > rect.right())
				p.setX(rect.right());
			
			if (p.y() > rect.bottom())
				p.setY(rect.bottom());
			
			if (p != pos())
				move(p);
		}
		
		showFullScreen();
		correctFullScreenGeometry();
		blitterContainer->parentExclusiveEvent(hasFocus());
	}
}

void MainWindow::toggleMenuHidden() {
#ifdef Q_WS_MAC
//	if (isFullScreen())
//		toggleFullScreen();
#else
	menuBar()->setVisible(!menuBar()->isVisible());
	
	if (!menuBar()->isVisible()) {
		hideCursor();
	}
#endif
}

void MainWindow::clearInputVectors() {
	keyInputs.clear();
	
	for (joymap_t::iterator it = joyInputs.begin(); it != joyInputs.end(); ++it)
		delete it->second;
		
	joyInputs.clear();
}

void MainWindow::pushInputObserver(const SDL_Event &data, InputObserver *observer) {
	if (data.value == KBD_VALUE) {
		keyInputs.insert(std::pair<unsigned,InputObserver*>(data.id, observer));
	} else {
		JoyObserver *jhandler = NULL;
		
		switch (data.type) {
		case SDL_JOYAXISMOTION: jhandler = new JoyAxisHandler(observer, data.value); break;
		case SDL_JOYHATMOTION: jhandler = new JoyHatHandler(observer, data.value); break;
		case SDL_JOYBUTTONCHANGE: jhandler = new JoyButHandler(observer); break;
		default: return;
		}
		
		joyInputs.insert(std::pair<unsigned,JoyObserver*>(data.id, jhandler));
	}
}

void MainWindow::inputSettingsChange() {
	clearInputVectors();
	
	for (unsigned i = 0; i < inputDialog->getData().size(); ++i)
		pushInputObserver(inputDialog->getData()[i], &buttonHandlers[i]);
}

void MainWindow::soundSettingsChange() {
	if (running)
		initAudio();
}

void MainWindow::uninitBlitter() {
	blitter->uninit();
	source->setPixelBuffer(NULL, MediaSource::RGB32, 0);
}

void MainWindow::videoSettingsChange() {
	{
		const int engineIndex = videoDialog->engine();
		
		if (blitter != blitters[engineIndex]) {
			bool updatesEnabled = true;
			bool visible = false;
			
			if (blitter) {
				updatesEnabled = blitter->updatesEnabled();
				visible = blitter->isVisible();
				disconnect(fullModeToggler.get(), SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
				
				if (running) {
					uninitBlitter();
				}
				
				blitter->setVisible(false);
			}
			
			blitter = blitters[engineIndex];
			blitter->setVisible(false);
			blitter->setUpdatesEnabled(updatesEnabled);
			//connect(fullResToggler, SIGNAL(modeChange()), blitter, SLOT(modeChange()));
			connect(fullModeToggler.get(), SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
			fullModeToggler->emitRate();
			blitterContainer->setBlitter(blitter);
			blitter->setVisible(visible);
			
			if (running) {
				blitter->init();
			}
		}
	}
	
	source->setVideoSource(videoDialog->sourceIndex());
	
	if (running)
		blitter->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());
	
	resetWindowSize(videoDialog->winRes());
	
	const unsigned screens = fullModeToggler->screens();
	
	for (unsigned i = 0; i < screens; ++i) {
		if (fullModeToggler->currentResIndex(i) != videoDialog->fullMode(i) ||
				fullModeToggler->currentRateIndex(i) != videoDialog->fullRate(i)) {
			blitterContainer->parentExclusiveEvent(false);
			fullModeToggler->setMode(i, videoDialog->fullMode(i), videoDialog->fullRate(i));
			
			if (fullModeToggler->isFullMode() && i == fullModeToggler->screen()) {
#ifdef Q_WS_WIN
				showNormal();
				showFullScreen();
#endif
				correctFullScreenGeometry();
			}
			
			blitterContainer->parentExclusiveEvent(isFullScreen() & hasFocus());
		}
	}
	
	setSamplesPrFrame();
	
	blitterContainer->updateLayout();
}

void MainWindow::execDialog(QDialog *const dialog) {
	const bool pausing = pauseOnDialogExec;
	
	paused += pausing << 1;
	
	if (paused)
		doPause();
	
	dialog->exec();
	
	paused -= pausing << 1;
	
	if (!paused)
		doUnpause();
}

void MainWindow::execVideoDialog() {
	execDialog(videoDialog);
}

void MainWindow::execInputDialog() {
	execDialog(inputDialog);
}

void MainWindow::execSoundDialog() {
	execDialog(soundDialog);
}

const QSize& MainWindow::aspectRatio() const {
	return videoDialog->aspectRatio();
}

void MainWindow::setAspectRatio(const QSize &aspectRatio) {
	videoDialog->setAspectRatio(aspectRatio);
}

void MainWindow::setSampleRates(const std::vector<int> &sampleRates) {
	soundDialog->setRates(sampleRates);
}

void MainWindow::setVideoSources(const std::vector<MediaSource::VideoSourceInfo> &sourceInfos) {
	videoDialog->setVideoSources(sourceInfos);
}

void MainWindow::doSetFrameTime(unsigned num, unsigned denom) {
	for (unsigned i = 0; i < blitters.size(); ++i)
		blitters[i]->setFrameTime(BlitterWidget::Rational(num, denom));
	
	setSamplesPrFrame();
}

void MainWindow::setFrameTime(unsigned num, unsigned denom) {
	if (!num) {
		num = 1;
		denom = 0xFFFF;
	} else if (!denom) {
		num = 0xFFFF;
		denom = 1;
	}

	ftNum = num;
	ftDenom = denom;
	
	if (!turbo)
		doSetFrameTime(num, denom);
}

void MainWindow::setSamplesPrFrame() {
	const BlitterWidget::Rational r = blitter->frameTime();
	const unsigned old = samplesPrFrame;
	samplesPrFrame = (sampleRate * r.numerator) / r.denominator + 1;
	
	if (old != samplesPrFrame) {
		delete []sndBuffer;
		sndBuffer = new qint16[(samplesPrFrame + 4) * 2];
		source->setSampleBuffer(sndBuffer, sampleRate);
		samplesCalc.setBaseSamples(samplesPrFrame);
	}
}

void MainWindow::initAudio() {
	if (ae)
		ae->uninit();
	
	ae = audioEngines[soundDialog->getEngineIndex()];
	
	if ((sampleRate = ae->init(soundDialog->getRate(), soundDialog->getLatency())) < 0) {
		ae = NULL;
		sampleRate = 0;
	}
	
	setSamplesPrFrame();
}

void MainWindow::soundEngineFailure() {
	QMessageBox::critical(this, tr("Error"), tr("Sound engine failure."));
	soundDialog->exec();
}

void MainWindow::timerEvent(QTimerEvent */*event*/) {
	if (!ae) { // avoid stupid recursive call detection by checking here rather than on init.
		soundEngineFailure();
		return;
	}
	
	updateJoysticks();
	
	source->update(samplesCalc.getSamples());
	
	if (blitter->sync(turbo)) {
		QMessageBox::critical(this, tr("Error"), tr("Video engine failure."));
		uninitBlitter();
		blitter->init();
		blitter->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());
		
		ae->pause();
		
		videoDialog->exec();
		return;
	}
	
	if (!turbo) {
		const unsigned lastSamples = samplesCalc.getSamples();
		const AudioEngine::BufferState bufState = ae->bufferState();
		
		if (bufState.fromUnderrun != AudioEngine::BufferState::NOT_SUPPORTED)
			samplesCalc.update(bufState.fromUnderrun, bufState.fromOverflow);
		
		if (ae->write(sndBuffer, lastSamples) < 0) {
			ae->pause();
			soundEngineFailure();
		}
	}
}

void MainWindow::run() {
	if (running)
		return;
	
	running = true;
	
#ifdef PLATFORM_WIN32
	timeBeginPeriod(1);
#endif
	
	initAudio();

	blitter->setVisible(true);
	blitter->init();
	blitter->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());
	
	if (!paused)
		timerId = startTimer(0);
}

void MainWindow::stop() {
	if (!running)
		return;
	
	running = false;
	
	if (timerId) {
		killTimer(timerId);
		timerId = 0;
	}
	
	uninitBlitter();
	blitter->setVisible(false);

	if (ae)
		ae->uninit();
	
#ifdef PLATFORM_WIN32
	timeEndPeriod(1);
#endif
}

void MainWindow::doPause() {
	if (!running || !timerId)
		return;
	
	if (ae)
		ae->pause();
	
	killTimer(timerId);
	timerId = 0;
}

void MainWindow::doUnpause() {
	if (!running || timerId)
		return;
	
	timerId = startTimer(0);
}

void MainWindow::pause() {
	paused |= 1;
	doPause();
}

void MainWindow::unpause() {
	paused &= ~1;
	
	if (!paused)
		doUnpause();
}

void MainWindow::frameStep() {
	if (isRunning() && paused == 1) {
		timerEvent(NULL);
		
		if (ae)
			ae->pause();
	}
}

void MainWindow::setTurbo(bool enable) {
	if (enable != turbo) {
		turbo = enable;
		
		if (enable) {
			if (ae)
				ae->pause();
			
			doSetFrameTime(1, 0xFFFF);
		} else
			doSetFrameTime(ftNum, ftDenom);
	}
}

void MainWindow::toggleTurbo() {
	setTurbo(!turbo);
}

void MainWindow::hideCursor() {
	if (!cursorHidden)
		centralWidget()->setCursor(Qt::BlankCursor);
	
	cursorHidden = true;
}

void MainWindow::showCursor() {
	if (cursorHidden)
		centralWidget()->unsetCursor();
	
	cursorHidden = false;
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
	e->ignore();
	
	{
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
		
		while (range.first != range.second) {
			(range.first->second)->pressEvent();
			++range.first;
		}
	}
	
	hideCursor();
}

void MainWindow::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();
	
	{
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
		
		while (range.first != range.second) {
			(range.first->second)->releaseEvent();
			++range.first;
		}
	}
}

void MainWindow::updateJoysticks() {
	if (hasFocus()/* || QApplication::desktop()->numScreens() != 1*/) {
		bool hit = false;
		
		SDL_ClearEvents();
		SDL_JoystickUpdate();
		
		SDL_Event ev;
		
		while (SDL_PollEvent(&ev)) {
			std::pair<joymap_t::iterator,joymap_t::iterator> range = joyInputs.equal_range(ev.id);
			
			while (range.first != range.second) {
				(range.first->second)->valueChanged(ev.value);
				++range.first;
			}
			
			hit = true;
		}
		
		if (hit) {
#ifdef Q_WS_X11
			XResetScreenSaver(QX11Info::display());
#endif
			hideCursor();
		}
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent */*e*/) {
	showCursor();
	cursorTimer->start();
}

void MainWindow::closeEvent(QCloseEvent */*e*/) {
	stop();
	
	if (!isFullScreen()) {
		saveWindowSize(size());
	}
	
	fullModeToggler->setFullMode(false); // avoid misleading auto-minimize on close focusOut event.
}

void MainWindow::showEvent(QShowEvent */*event*/) {
	resetWindowSize(videoDialog->winRes()); // some window managers get pissed (xfwm4 breaks, metacity complains) if fixed window size is set too early.
}

void MainWindow::moveEvent(QMoveEvent */*event*/) {
	fullModeToggler->setScreen(this);
}

void MainWindow::resizeEvent(QResizeEvent */*event*/) {
	fullModeToggler->setScreen(this);
}

void MainWindow::focusOutEvent(QFocusEvent */*event*/) {
	blitterContainer->parentExclusiveEvent(false);
	
#ifndef Q_WS_MAC // Minimize is ugly on mac (especially full screen windows) and there doesn't seem to be a "qApp->hide()" which would be more appropriate.
	if (isFullScreen() && fullModeToggler->isFullMode() && !qApp->activeWindow()/* && QApplication::desktop()->numScreens() == 1*/) {
		fullModeToggler->setFullMode(false);
		showMinimized();
	}
#endif
	
	showCursor();
	cursorTimer->stop();
}

void MainWindow::focusInEvent(QFocusEvent */*event*/) {
	if (isFullScreen() && !fullModeToggler->isFullMode()) {
		fullModeToggler->setFullMode(true);
		correctFullScreenGeometry();
	}
	
	blitterContainer->parentExclusiveEvent(isFullScreen());
	
	cursorTimer->start();
}

void MainWindow::blit() {
	blitter->blit();
}
