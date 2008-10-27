/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <QtGui>
#include <QApplication>
#include <QTimer>

#include <resample/resamplerinfo.h>

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
	const int mask;

	void notifyObserver(bool press) {
		if (press)
			observer->pressEvent();
		else
			observer->releaseEvent();
	}

public:
	JoyObserver(MainWindow::InputObserver *observer, const int mask) : observer(observer), mask(mask) {}
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

void MainWindow::SampleBuffer::reset(const Rational &spf, const unsigned overupdate) {
	sndInBuffer.reset((spf.ceil() + overupdate) * 2);
	this->spf = spf;
	num = 0;
	samplesBuffered = 0;
}

std::size_t MainWindow::SampleBuffer::update(qint16 *const out, MediaSource *const source, Resampler *const resampler) {
	num += spf.num;
	const long insamples = num / spf.denom;
	num -= insamples * spf.denom;

	samplesBuffered += source->update(sndInBuffer + samplesBuffered * 2, insamples - samplesBuffered);
	samplesBuffered -= insamples;

	std::size_t outsamples = 0;

	if (out) {
		if (resampler->inRate() == resampler->outRate()) {
			std::memcpy(out, sndInBuffer, insamples * sizeof(qint16) * 2);
			outsamples = insamples;
		} else
			outsamples = resampler->resample(out, sndInBuffer, insamples);
	}

	std::memmove(sndInBuffer, sndInBuffer + insamples * 2, samplesBuffered * sizeof(qint16) * 2);

	return outsamples;
}

MainWindow::MainWindow(MediaSource *source,
                       const std::vector<MediaSource::ButtonInfo> &buttonInfos,
                       const std::vector<MediaSource::VideoSourceInfo> &videoSourceInfos,
                       const QString &videoSourceLabel,
                       const QSize &aspectRatio) :
	source(source),
	buttonHandlers(buttonInfos.size(), ButtonHandler(0, 0)),
	fullModeToggler(getFullModeToggler(winId())),
	sampleBuffer(Rational(735, 1), source->overupdate),
	sndOutBuffer(0),
	ae(NULL),
	cursorTimer(NULL),
	jsTimer(NULL),
	ftNum(1),
	ftDenom(60),
	paused(0),
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
	soundDialog = new SoundDialog(audioEngines, this);
	connect(soundDialog, SIGNAL(accepted()), this, SLOT(soundSettingsChange()));

	inputDialog = new InputDialog(buttonInfos, this);
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

	for (std::vector<BlitterWidget*>::iterator it = blitters.begin(); it != blitters.end(); ++it) {
		(*it)->setVisible(false);
		(*it)->setParent(blitterContainer);
	}

	source->setPixelBuffer(NULL, MediaSource::RGB32, 0);

	videoSettingsChange();
	inputSettingsChange();
	soundSettingsChange();

	setFrameTime(ftNum, ftDenom);

	cursorTimer = new QTimer(this);
	cursorTimer->setSingleShot(true);
	cursorTimer->setInterval(2500);
	connect(cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));

	jsTimer = new QTimer(this);
	jsTimer->setInterval(133);
	connect(jsTimer, SIGNAL(timeout()), this, SLOT(updateJoysticks()));

	setMouseTracking(true);
	setFocus();
}

MainWindow::~MainWindow() {
	clearInputVectors();

	for (uint i = 0; i < blitters.size(); ++i)
		delete blitters[i];

	for (uint i = 0; i < audioEngines.size(); ++i)
		delete audioEngines[i];
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
			else if (p.x() < rect.left())
				p.setX(rect.left());

			if (p.y() > rect.bottom())
				p.setY(rect.bottom());
			else if (p.y() < rect.top())
				p.setY(rect.top());

			if (p != pos())
				move(p);
		}

		showFullScreen();
		correctFullScreenGeometry();
#ifdef Q_WS_MAC // work around annoying random non-updating OpenGL on Mac OS X after full screen.
		blitterContainer->hide();
		blitterContainer->show();
#endif
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
	if (data.value != InputDialog::NULL_VALUE) {
		if (data.value == InputDialog::KBD_VALUE) {
			keyInputs.insert(std::pair<unsigned,InputObserver*>(data.id, observer));
		} else {
			joyInputs.insert(std::pair<unsigned,JoyObserver*>(data.id, new JoyObserver(observer, data.value)));
		}
	}
}

void MainWindow::inputSettingsChange() {
	clearInputVectors();

	for (std::size_t i = 0; i < inputDialog->getData().size(); ++i)
		pushInputObserver(inputDialog->getData()[i], &buttonHandlers[i >> 1]);
}

void MainWindow::soundSettingsChange() {
	if (running)
		initAudio();
}

void MainWindow::uninitBlitter() {
	blitterContainer->blitter()->uninit();
	source->setPixelBuffer(NULL, MediaSource::RGB32, 0);
}

void MainWindow::videoSettingsChange() {
	{
		const int engineIndex = videoDialog->engine();

		if (blitterContainer->blitter() != blitters[engineIndex]) {
			bool visible = false;

			if (blitterContainer->blitter()) {
				visible = blitterContainer->blitter()->isVisible();
				disconnect(fullModeToggler.get(), SIGNAL(rateChange(int)), blitterContainer->blitter(), SLOT(rateChange(int)));

				if (running)
					uninitBlitter();

				blitterContainer->blitter()->setVisible(false);
			}

			blitterContainer->setBlitter(blitters[engineIndex]);
			//connect(fullResToggler, SIGNAL(modeChange()), blitterContainer->blitter(), SLOT(modeChange()));
			connect(fullModeToggler.get(), SIGNAL(rateChange(int)), blitterContainer->blitter(), SLOT(rateChange(int)));
			fullModeToggler->emitRate();
			blitterContainer->blitter()->setVisible(visible);

			if (running)
				blitterContainer->blitter()->init();
		}
	}

	source->setVideoSource(videoDialog->sourceIndex());

	if (running)
		blitterContainer->blitter()->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());

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

// 	setSampleRate();

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

void MainWindow::setVideoSources(const std::vector<MediaSource::VideoSourceInfo> &sourceInfos) {
	videoDialog->setVideoSources(sourceInfos);
}

void MainWindow::doSetFrameTime(unsigned num, unsigned denom) {
	for (unsigned i = 0; i < blitters.size(); ++i)
		blitters[i]->setFrameTime(num * 1000000.0f / denom + 0.5f);
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

	setSampleRate();
}

static void adjustResamplerRate(Array<qint16> &sndOutBuf, Resampler *const resampler, const long maxspf, const long outRate) {
	resampler->adjustRate(resampler->inRate(), outRate);

	const std::size_t sz = resampler->maxOut(maxspf) * 2;

	if (sz > sndOutBuf.size())
		sndOutBuf.reset(sz);
}

void MainWindow::setSampleRate() {
	if (ae) {
		const Rational fr(ftDenom, ftNum);
		const long insrate = fr.toDouble() * sampleBuffer.samplesPerFrame().toDouble() + 0.5;
		const long maxspf = sampleBuffer.samplesPerFrame().ceil();

		resampler.reset();
		resampler.reset(ResamplerInfo::get(soundDialog->getResamplerNum()).create(insrate, ae->rate(), maxspf));
		sndOutBuffer.reset(resampler->maxOut(maxspf) * 2);
	}
}

void MainWindow::setSamplesPerFrame(const long num, const long denom) {
	sampleBuffer.reset(Rational(num, denom), source->overupdate);
	setSampleRate();
}

void MainWindow::initAudio() {
	if (ae)
		ae->uninit();

	ae = audioEngines[soundDialog->getEngineIndex()];

	if (ae->init(soundDialog->getRate(), soundDialog->getLatency()) < 0)
		ae = NULL;

	setSampleRate();
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

	const std::size_t outsamples = sampleBuffer.update(turbo ? NULL : static_cast<qint16*>(sndOutBuffer), source, resampler.get());

	long syncft = 0;

	if (!turbo) {
		RateEst::Result rsrate;
		AudioEngine::BufferState bstate;

		if (ae->write(sndOutBuffer, outsamples, bstate, rsrate) < 0) {
			ae->pause();
			soundEngineFailure();
			return;
		}

		const long usecft = blitterContainer->blitter()->frameTime();
		syncft = static_cast<float>(usecft - (usecft >> 10)) * ae->rate() / rsrate.est;

		if (bstate.fromUnderrun != AudioEngine::BufferState::NOT_SUPPORTED &&
				  bstate.fromUnderrun + outsamples * 2 < bstate.fromOverflow)
			syncft >>= 1;

		const BlitterWidget::Estimate &estft = blitterContainer->blitter()->frameTimeEst();

		if (estft.est) {
			float est = static_cast<float>(rsrate.est) * estft.est;
			const float var = static_cast<float>(rsrate.est + rsrate.var) * (estft.est + estft.var) - est;
			est += var;

			if (std::fabs(est - resampler->outRate() * static_cast<float>(usecft - (usecft >> 11))) > var * 2)
				adjustResamplerRate(sndOutBuffer, resampler.get(), sampleBuffer.samplesPerFrame().ceil(), est / (usecft - (usecft >> 11)));
		} else if (resampler->outRate() != ae->rate())
			adjustResamplerRate(sndOutBuffer, resampler.get(), sampleBuffer.samplesPerFrame().ceil(), ae->rate());
	}

	if (blitterContainer->blitter()->sync(syncft) < 0) {
		QMessageBox::critical(this, tr("Error"), tr("Video engine failure."));
		uninitBlitter();
		blitterContainer->blitter()->init();
		blitterContainer->blitter()->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());

		ae->pause();

		videoDialog->exec();
		return;
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

	blitterContainer->blitter()->setVisible(true);
	blitterContainer->blitter()->init();
	blitterContainer->blitter()->setBufferDimensions(videoDialog->sourceSize().width(), videoDialog->sourceSize().height());

	if (!paused)
		timerId = startTimer(0);
	else
		jsTimer->start();
}

void MainWindow::stop() {
	if (!running)
		return;

	running = false;
	jsTimer->stop();

	if (timerId) {
		killTimer(timerId);
		timerId = 0;
	}

	uninitBlitter();
	blitterContainer->blitter()->setVisible(false);

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
	jsTimer->start();
}

void MainWindow::doUnpause() {
	if (!running || timerId)
		return;

	jsTimer->stop();
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

	if (isRunning() && !e->isAutoRepeat()) {
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());

		while (range.first != range.second) {
			(range.first->second)->pressEvent();
			++range.first;
		}

		hideCursor();
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();

	if (isRunning() && !e->isAutoRepeat()) {
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

		SDL_JoystickUpdate();

		SDL_Event ev;

		while (pollJsEvent(&ev)) {
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

	SDL_JoystickUpdate();
	SDL_ClearEvents();

	cursorTimer->start();
}

void MainWindow::blit() {
	blitterContainer->blitter()->blit();
}
