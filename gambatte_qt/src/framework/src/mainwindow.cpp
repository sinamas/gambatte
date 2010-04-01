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
#include "../mainwindow.h"
#include <QEvent>
#include <QKeyEvent>
#include <QLayout>
#include <QCoreApplication>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include "adaptivesleep.h"
#include "mediaworker.h"
#include "blittercontainer.h"
#include "audioengines/nullaudioengine.h"
#include "blitterwidgets/qpainterblitter.h"
#include "blitterwidgets/qglblitter.h"
#include "getfullmodetoggler.h"
#include "SDL_event.h"
#include "addaudioengines.h"
#include "addblitterwidgets.h"

#ifdef Q_WS_WIN
#include <windows.h> // for timeBeginPeriod, timeEndPeriod
#endif

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

struct CustomEvent : QEvent {
	CustomEvent() : QEvent(QEvent::MaxUser) {}
	virtual ~CustomEvent() {}
	virtual void exec(MainWindow*) = 0;
};

struct MainWindow::Pauser::DoPause {
	void operator()(MainWindow *const mw) {
		if (mw->running) {
			mw->worker->pause();

// 			if (mw->timerId) {
// 				mw->killTimer(mw->timerId);
// 				mw->timerId = 0;
// 			}

			mw->jsTimer->start();
		}

		mw->blitterContainer->blitter()->setPaused(true);
	}
};

struct MainWindow::Pauser::DoUnpause {
	void operator()(MainWindow *const mw) {
		if (mw->running) {
			mw->jsTimer->stop();
			mw->worker->unpause();

// 			if (!mw->threaded && !mw->timerId)
// 				mw->timerId = mw->startTimer(0);
		}

		mw->blitterContainer->blitter()->setPaused(false);
	}
};

void MainWindow::Pauser::modifyPaused(const unsigned newPaused, MainWindow *const mw) {
	if (paused) {
		if (!newPaused)
			mw->callWhenPaused(DoUnpause());
	} else {
		if (newPaused)
			mw->callWhenPaused(DoPause());
	}

	paused = newPaused;
}

enum { BLIT_REQUESTED = 1, BLIT_POSTED = 2 };

static bool blitRequested(AtomicVar<unsigned> &blitState) {
	AtomicVar<unsigned>::Locked bs(blitState);

	if (bs.get() & BLIT_REQUESTED) {
		bs.set(BLIT_POSTED);
		return true;
	}

	return false;
}

class MainWindow::WorkerCallback : public MediaWorker::Callback {
	MainWindow *const mw;
	AdaptiveSleep asleep;
	usec_t synctimebase, synctimeinc;
	AtomicVar<unsigned> blitState;

public:
	WorkerCallback(MainWindow *const mw) : mw(mw), blitState(0) {}
	void blit(usec_t synctimebase, usec_t synctimeinc);
	bool cancelBlit();
	void paused();
	void audioEngineFailure();
	bool tryLockVideoBuffer() { return mw->vbmut.tryLock(); }
	void unlockVideoBuffer() { mw->vbmut.unlock(); }
	const PixelBuffer& videoBuffer() {
		return /*mw->videoChain.empty() ? */mw->blitterContainer->blitter()->inBuffer()/* : mw->videoChain.front()->inBuffer()*/;
	}
};

void MainWindow::WorkerCallback::blit(const usec_t synctimebase, const usec_t synctimeinc) {
	struct BlitEvent : CustomEvent {
		WorkerCallback &cb;
		BlitEvent(WorkerCallback &cb) : cb(cb) {}

		void exec(MainWindow *const mw) {
			if (blitRequested(cb.blitState) && mw->running) {
				usec_t base, inc;
				BlitterWidget *const blitter = mw->blitterContainer->blitter();

				mw->worker->source()->generateVideoFrame(blitter->inBuffer());
				blitter->blit();

				base = cb.synctimebase;
				inc  = cb.synctimeinc;

				SyncVar::Locked(mw->worker->waitingForSync()).set(true);

				blitter->draw();

				if (!blitter->frameTimeEst())
					cb.asleep.sleepUntil(base, inc);

				if (blitter->sync() < 0)
					mw->emitVideoBlitterFailure();

				mw->worker->setFrameTimeEstimate(blitter->frameTimeEst());
			}

			AtomicVar<unsigned>::Locked bs(cb.blitState);

			if (bs.get() & BLIT_REQUESTED)
				QCoreApplication::postEvent(mw, new BlitEvent(cb));
			else
				bs.set(0);
		}
	};

	this->synctimebase = synctimebase;
	this->synctimeinc  = synctimeinc;

	AtomicVar<unsigned>::Locked bs(blitState);

	if (!(bs.get() & BLIT_POSTED))
		QCoreApplication::postEvent(mw, new BlitEvent(*this));

	bs.set(BLIT_REQUESTED | BLIT_POSTED);
}

bool MainWindow::WorkerCallback::cancelBlit() {
	AtomicVar<unsigned>::Locked bs(blitState);

	const unsigned val = bs.get();
	bs.set(val & ~BLIT_REQUESTED);

	return val & BLIT_REQUESTED;
}

void MainWindow::WorkerCallback::paused() {
	struct PausedEvent : CustomEvent {
		void exec(MainWindow *const mw) { mw->execPausedQueue(); }
	};

	QCoreApplication::postEvent(mw, new PausedEvent);
}

void MainWindow::WorkerCallback::audioEngineFailure() {
	struct AudioFailureEvent : CustomEvent {
		void exec(MainWindow *const mw) { mw->emitAudioEngineFailure(); }
	};

	QCoreApplication::postEvent(mw, new AudioFailureEvent);
}

MainWindow::FrameBuffer::Locked::Locked(FrameBuffer fb) : mw(fb.mw), pb(mw->blitterContainer->blitter()->inBuffer()) {
	if (!mw->vbmut.tryLock()) {
		pb.data = 0;
		mw = 0;
	}
}

MainWindow::FrameBuffer::Locked::~Locked() {
	if (mw)
		mw->vbmut.unlock();
}

MainWindow::MainWindow(MediaSource *const source)
: worker(new MediaWorker(source, std::auto_ptr<MediaWorker::Callback>(new WorkerCallback(this)), this)),
  blitterContainer(new BlitterContainer(this)),
  fullModeToggler(getFullModeToggler(winId())),
  cursorTimer(new QTimer(this)),
  jsTimer(new QTimer(this)),
  imageFormat(320, 240),
  windowSize(-1, -1),
  frameTime_(1, 60),
  focusPauseBit(0),
//   timerId(0),
  hz(60),
  running(false),
//   threaded(true),
  refreshRateSync(false),
  cursorHidden(false)
{
// 	setAttribute(Qt::WA_DeleteOnClose);
	setFocusPolicy(Qt::StrongFocus);
	setCentralWidget(blitterContainer);
	addAudioEngines(audioEngines, winId());
	audioEngines.push_back(new NullAudioEngine);

	addBlitterWidgets(blitters, VideoBufferLocker(vbmut));
	blitters.push_back(new QGLBlitter(VideoBufferLocker(vbmut)));
	blitters.push_back(new QPainterBlitter(VideoBufferLocker(vbmut)));

	for (auto_vector<BlitterWidget>::iterator it = blitters.begin(); it != blitters.end();) {
		if ((*it)->isUnusable()) {
			delete *it;
			it = blitters.erase(it);
		} else
			++it;
	}

	for (auto_vector<BlitterWidget>::iterator it = blitters.begin(); it != blitters.end(); ++it) {
		(*it)->setVisible(false);
		(*it)->setParent(blitterContainer);
	}

	worker->setAudioOut(audioEngines.back(), 48000, 100);
	worker->setSamplesPerFrame(Rational(48000 / 60));
	blitterContainer->setBlitter(blitters.back());
	blitterContainer->blitter()->setPaused(false);
	blitterContainer->setMinimumSize(QSize(imageFormat.width, imageFormat.height));
	blitterContainer->setSourceSize(QSize(imageFormat.width, imageFormat.height));
	blitterContainer->setAspectRatio(QSize(imageFormat.width, imageFormat.height));
	connect(fullModeToggler.get(), SIGNAL(rateChange(int)), this, SLOT(hzChange(int)));
	fullModeToggler->emitRate();

	cursorTimer->setSingleShot(true);
	cursorTimer->setInterval(2000);
	connect(cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));

	jsTimer->setInterval(200);
	connect(jsTimer, SIGNAL(timeout()), this, SLOT(updateJoysticks()));

	setMouseTracking(true);
	setFocus();
}

void MainWindow::run() {
	if (running)
		return;

	running = true;

#ifdef Q_WS_WIN
	timeBeginPeriod(1);
#endif

	blitterContainer->blitter()->setVisible(true);
	blitterContainer->blitter()->init();
	rebuildVideoChain();

	if (pauser.isPaused()) {
		jsTimer->start();
		worker->pause();
	}/* else if (!threaded)
		timerId = startTimer(0);*/

// 	if (!threaded)
// 		worker->deactivate();

	SDL_JoystickUpdate();
	SDL_ClearEvents();
	worker->start();
}

void MainWindow::stop() {
	if (!running)
		return;

	worker->stop();
	jsTimer->stop();
// 	videoChain.clear();
	running = false;

// 	if (timerId) {
// 		killTimer(timerId);
// 		timerId = 0;
// 	}

	blitterContainer->blitter()->uninit();
	blitterContainer->blitter()->setVisible(false);

#ifdef Q_WS_WIN
	timeEndPeriod(1);
#endif
}

void MainWindow::rebuildVideoChain() {
	ImageFormat ifmt(imageFormat);
	/*videoChain.clear(); // and delete. can we do better than complete reinitialization?

	if (vfilter) {
		VideoLink *const filterLink = vfilter->create(ifmt.width, ifmt.height, ifmt.pixelFormat);

		if (filterLink->inBuffer().pixelFormat != ifmt.pixelFormat) {
			videoChain.push_back(VideoFormatConverter::create(ifmt.width,
					ifmt.height, ifmt.pixelFormat, vfilter->inBuffer().pixelFormat));
		}

		videoChain.push_back(filterLink);
		ifmt.width = filterLink->outWidth();
		ifmt.height = filterLink->outHeight();
		ifmt.pixelFormat = filterLink->outPixelFormat();
	}*/

	/*if (!*/blitterContainer->blitter()->setVideoFormat(ifmt.width, ifmt.height/*, ifmt.pixelFormat*/)/*) {*/;
// 		videoChain.push_back(VideoFormatConverter::create(ifmt.width, ifmt.height,
// 				ifmt.pixelFormat, blitterContainer->blitter()->inBuffer().pixelFormat));
// 	}

// 	worker->setVideoBuffer(/*videoChain.empty() ? */blitterContainer->blitter()->inBuffer()/* : videoChain.front()->inBuffer()*/);
}

static const QSize getFilteredSize(const QSize &sz/*, const VideoFiler *const filter*/) {
	return /*filter ? filter->outSize(sz) : */sz;
}

void MainWindow::updateMinimumSize() {
	if (layout()->sizeConstraint() != QLayout::SetFixedSize)
		centralWidget()->setMinimumSize(getFilteredSize(QSize(imageFormat.width, imageFormat.height)/*, vfilter*/));
}

void MainWindow::doSetWindowSize(const QSize &sz) {
	if (!isFullScreen() && isVisible()) {
		if (sz == QSize(-1, -1)) {
			centralWidget()->setMinimumSize(getFilteredSize(QSize(imageFormat.width, imageFormat.height)/*, vfilter*/));
			layout()->setSizeConstraint(QLayout::SetMinimumSize);
			setMinimumSize(1, 1); // needed on macx
			setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX); // needed on macx. needed for metacity full screen switching, layout()->setSizeConstraint(QLayout::SetMinAndMaxSize) won't do.
			resize(size()); // needed on macx
		} else {
			centralWidget()->setMinimumSize(sz);
			layout()->setSizeConstraint(QLayout::SetFixedSize);
		}
	}
}

void MainWindow::setWindowSize(const QSize &sz) {
	windowSize = sz;
	doSetWindowSize(sz);
}

void MainWindow::setBlitter(BlitterWidget *const blitter) {
	if (blitterContainer->blitter() != blitter) {
		bool visible = false;
		bool paused = false;

		if (blitterContainer->blitter()) {
			visible = blitterContainer->blitter()->isVisible();
			paused = blitterContainer->blitter()->isPaused();

			if (running)
				blitterContainer->blitter()->uninit();

			blitterContainer->blitter()->setVisible(false);
		}

		blitterContainer->setBlitter(blitter);
		blitterContainer->blitter()->setVisible(visible);
		blitterContainer->blitter()->setPaused(paused);
		blitterContainer->blitter()->rateChange(hz);
		updateSwapInterval();

		if (running)
			blitterContainer->blitter()->init();
	}
}

void MainWindow::setVideo(const unsigned w, const unsigned h,
		/*const PixelBuffer::PixelFormat pf,*/ /*const VideoFilter *const vf, */BlitterWidget *const blitter)
{
	if (imageFormat.width != w || imageFormat.height != h /*|| imageFormat.pixelFormat != pf*/ ||
					/*vfilter != vf || */blitter != blitterContainer->blitter())
	{
		vbmut.lock();
		imageFormat = ImageFormat(w, h/*, pf*/);
		setBlitter(blitter);
// 		vfilter = vf;

		if (running)
			rebuildVideoChain();

		vbmut.unlock();
		updateMinimumSize();
		blitterContainer->setSourceSize(QSize(w, h));
	}
}

void MainWindow::execPausedQueue() {
	if (worker->paused() && !pausedq.empty()) {
		pausedq.pop_all(this);
		worker->qUnpause();
	}
}

void MainWindow::customEvent(QEvent *const ev) {
	reinterpret_cast<CustomEvent*>(ev)->exec(this);
}

void MainWindow::closeEvent(QCloseEvent */*e*/) {
	stop();

// 	if (!isFullScreen())
// 		saveWindowSize(size());
	emit closing();

	fullModeToggler->setFullMode(false); // avoid misleading auto-minimize on close focusOut event.
}

void MainWindow::moveEvent(QMoveEvent */*event*/) {
	fullModeToggler->setScreen(this);
}

void MainWindow::resizeEvent(QResizeEvent */*event*/) {
	fullModeToggler->setScreen(this);
}

void MainWindow::showEvent(QShowEvent *) {
	doSetWindowSize(windowSize); // some window managers get pissed (xfwm4 breaks, metacity complains) if fixed window size is set too early.
}

void MainWindow::mouseMoveEvent(QMouseEvent */*e*/) {
	showCursor();
	cursorTimer->start();
}

void MainWindow::focusOutEvent(QFocusEvent */*event*/) {
	pauser.set(focusPauseBit, this);
	blitterContainer->parentExclusiveEvent(false);

// #ifndef Q_WS_MAC // Minimize is ugly on mac (especially full screen windows) and there doesn't seem to be a "qApp->hide()" which would be more appropriate.
// 	if (isFullScreen() && fullModeToggler->isFullMode() && !qApp->activeWindow()/* && QApplication::desktop()->numScreens() == 1*/) {
// 		fullModeToggler->setFullMode(false);
// 		showMinimized();
// 	}
// #endif

	showCursor();
	cursorTimer->stop();
}

void MainWindow::focusInEvent(QFocusEvent */*event*/) {
	if (isFullScreen() && !fullModeToggler->isFullMode()) {
		fullModeToggler->setFullMode(true);
		correctFullScreenGeometry();
	}

	blitterContainer->parentExclusiveEvent(isFullScreen());

// 	SDL_JoystickUpdate();
// 	SDL_ClearEvents();

	cursorTimer->start();
	pauser.unset(focusPauseBit, this);
}

void MainWindow::setPauseOnFocusOut(const unsigned bitmask) {
	if (bitmask != focusPauseBit) {
		if (hasFocus()) {
			pauser.unset(focusPauseBit, this);
		} else {
			pauser.set(bitmask, this);
			pauser.unset(focusPauseBit & ~bitmask, this);
		}

		focusPauseBit = bitmask;
	}
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
	e->ignore();

	if (running && !e->isAutoRepeat()) {
		worker->source()->keyPressEvent(e);
		hideCursor();
	}
}

void MainWindow::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();

	if (running && !e->isAutoRepeat())
		worker->source()->keyReleaseEvent(e);
}

void MainWindow::setAudioOut(unsigned engineNo, unsigned srateHz, unsigned msecLatency) {
	worker->setAudioOut(audioEngines[engineNo], srateHz, msecLatency);
}

void MainWindow::setResampler(unsigned resamplerNo) {
	worker->setResampler(resamplerNo);
}

void MainWindow::setFrameTime(unsigned num, unsigned denom) {
	if (!num) {
		num = 1;
		denom = 0xFFFF;
	} else if (!denom) {
		num = 0xFFFF;
		denom = 1;
	}

	frameTime_ = Rational(num, denom);
	updateSwapInterval();
}

void MainWindow::waitUntilPaused() {
	while (!pausedq.empty())
		QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
}

void MainWindow::setSamplesPerFrame(long num, long denom) {
	worker->setSamplesPerFrame(Rational(num, denom));
}

void MainWindow::setSyncToRefreshRate(bool on) {
	refreshRateSync = on;
	updateSwapInterval();
}

void MainWindow::correctFullScreenGeometry() {
	const QRect &screenRect = fullModeToggler->fullScreenRect(this);

	if (geometry() != screenRect) {
		setGeometry(screenRect);
	}
}

void MainWindow::setFullScreenMode(const unsigned screenNo, const unsigned resIndex, const unsigned rateIndex) {
	if (screenNo < fullModeToggler->screens() &&
			(fullModeToggler->currentResIndex(screenNo)  != resIndex ||
			 fullModeToggler->currentRateIndex(screenNo) != rateIndex)) {
		blitterContainer->parentExclusiveEvent(false);
		fullModeToggler->setMode(screenNo, resIndex, rateIndex);

		if (fullModeToggler->isFullMode() && screenNo == fullModeToggler->screen()) {
#ifdef Q_WS_WIN
			showNormal(); // is this really neccessary anymore?
			showFullScreen();
#endif
			correctFullScreenGeometry();
		}

		blitterContainer->parentExclusiveEvent(isFullScreen() & hasFocus());
	}
}

void MainWindow::toggleFullScreen() {
	if (isFullScreen()) {
		blitterContainer->parentExclusiveEvent(false);
		fullModeToggler->setFullMode(false);
		showNormal();
		doSetWindowSize(windowSize);
		activateWindow();
	} else {
		const int screen = QApplication::desktop()->screenNumber(this);

		fullModeToggler->setFullMode(true);
// 		saveWindowSize(size());
		doSetWindowSize(QSize(-1, -1));

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

void MainWindow::updateSwapInterval() {
	unsigned si = 0;

	if (refreshRateSync) {
		si = (frameTime_.num * hz + (frameTime_.denom >> 1)) / frameTime_.denom;

		if (si < 1)
			si = 1;

		if (si > blitterContainer->blitter()->maxSwapInterval())
			si = blitterContainer->blitter()->maxSwapInterval();
	}

	blitterContainer->blitter()->setSwapInterval(si);

	if (si)
		worker->setFrameTime(Rational(si, hz));
	else
		worker->setFrameTime(frameTime_);

	worker->setFrameTimeEstimate(blitterContainer->blitter()->frameTimeEst());
}

void MainWindow::hzChange(int hz) {
	if (hz < 1)
		hz = 60;

	this->hz = hz;
	blitterContainer->blitter()->rateChange(hz);
	updateSwapInterval();
}

void MainWindow::updateJoysticks() {
	worker->updateJoysticks();
}

const BlitterConf MainWindow::currentBlitterConf() {
	return BlitterConf(blitterContainer->blitter());
}

const ConstBlitterConf MainWindow::currentBlitterConf() const {
	return ConstBlitterConf(blitterContainer->blitter());
}

struct MainWindow::FrameStepFun {
	void operator()(MainWindow *const mw) {
		if (mw->running && mw->worker->frameStep()) {
			BlitterWidget *const blitter = mw->blitterContainer->blitter();

			mw->worker->source()->generateVideoFrame(blitter->inBuffer());
			blitter->blit();
			blitter->draw();

			if (blitter->sync() < 0)
				mw->emitVideoBlitterFailure();
		}
	}
};

void MainWindow::frameStep() {
	callWhenPaused(FrameStepFun());
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
