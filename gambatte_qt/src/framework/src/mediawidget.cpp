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
#include "mediawidget.h"
#include <QEvent>
#include <QKeyEvent>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QtGlobal> // for Q_WS_WIN define
#include "adaptivesleep.h"
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

MediaWidget::JoystickIniter::JoystickIniter() {
	SDL_JoystickInit();

	SDL_SetEventFilter(SDL_JOYAXISMOTION | SDL_JOYBUTTONCHANGE | SDL_JOYHATMOTION);

	const int numJs = SDL_NumJoysticks();

	for (int i = 0; i < numJs; ++i) {
		if (SDL_Joystick *joy = SDL_JoystickOpen(i))
			joysticks.push_back(joy);
	}

	SDL_JoystickUpdate();
	SDL_ClearEvents();
}

MediaWidget::JoystickIniter::~JoystickIniter() {
	for (std::size_t i = 0; i < joysticks.size(); ++i)
		SDL_JoystickClose(joysticks[i]);

	SDL_JoystickQuit();
}

namespace {
struct CustomEvent : QEvent {
	CustomEvent() : QEvent(QEvent::MaxUser) {}
	virtual ~CustomEvent() {}
	virtual void exec() = 0;
};
}

struct MediaWidget::Pauser::DoPause {
	MediaWidget &mw;
	explicit DoPause(MediaWidget &mw) : mw(mw) {}
	
	void operator()() const {
		if (mw.running) {
			mw.worker->pause();
			mw.jsTimer->start();
		}

		mw.blitterContainer->blitter()->setPaused(true);
	}
};

struct MediaWidget::Pauser::DoUnpause {
	MediaWidget &mw;
	explicit DoUnpause(MediaWidget &mw) : mw(mw) {}
	
	void operator()() const {
		if (mw.running) {
			mw.jsTimer->stop();
			mw.worker->unpause();
		}

		mw.blitterContainer->blitter()->setPaused(false);
	}
};

void MediaWidget::Pauser::modifyPaused(const unsigned newPaused, MediaWidget &mw) {
	if (paused) {
		if (!newPaused)
			mw.callWhenPaused(DoUnpause(mw));
	} else {
		if (newPaused)
			mw.callWhenPaused(DoPause(mw));
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

class MediaWidget::WorkerCallback : public MediaWorker::Callback {
	MediaWidget &mw;
	AdaptiveSleep asleep;
	usec_t synctimebase, synctimeinc;
	AtomicVar<unsigned> blitState;

public:
	explicit WorkerCallback(MediaWidget &mw) : mw(mw), blitState(0) {}
	void blit(usec_t synctimebase, usec_t synctimeinc);
	bool cancelBlit();
	void paused();
	void audioEngineFailure();
	
	bool tryLockVideoBuffer(PixelBuffer &pb) {
		if (mw.vbmut.tryLock()) {
			pb = mw.blitterContainer->blitter()->inBuffer();
			return true;
		}
		
		return false;
	}
	
	void unlockVideoBuffer() { mw.vbmut.unlock(); }

	void consumeBlitRequest();
};

void MediaWidget::WorkerCallback::blit(const usec_t synctimebase, const usec_t synctimeinc) {
	struct BlitEvent : CustomEvent {
		WorkerCallback &cb;
		explicit BlitEvent(WorkerCallback &cb) : cb(cb) {}

		void exec() {
			cb.consumeBlitRequest();

			AtomicVar<unsigned>::Locked bs(cb.blitState);

			if (bs.get() & BLIT_REQUESTED) {
				QCoreApplication::postEvent(&cb.mw, new BlitEvent(cb));
			} else
				bs.set(0);
		}
	};

	this->synctimebase = synctimebase;
	this->synctimeinc  = synctimeinc;

	AtomicVar<unsigned>::Locked bs(blitState);

	if (!(bs.get() & BLIT_POSTED))
		QCoreApplication::postEvent(&mw, new BlitEvent(*this));

	bs.set(BLIT_REQUESTED | BLIT_POSTED);
}

bool MediaWidget::WorkerCallback::cancelBlit() {
	AtomicVar<unsigned>::Locked bs(blitState);

	const unsigned val = bs.get();
	bs.set(val & ~BLIT_REQUESTED);

	return val & BLIT_REQUESTED;
}

void MediaWidget::WorkerCallback::paused() {
	struct PausedEvent : CustomEvent {
		MediaWidget &mw;
		explicit PausedEvent(MediaWidget &mw) : mw(mw) {}
		void exec() { mw.execPausedQueue(); }
	};

	QCoreApplication::postEvent(&mw, new PausedEvent(mw));
}

void MediaWidget::WorkerCallback::audioEngineFailure() {
	struct AudioFailureEvent : CustomEvent {
		MediaWidget &mw;
		explicit AudioFailureEvent(MediaWidget &mw) : mw(mw) {}
		void exec() { mw.emitAudioEngineFailure(); }
	};

	QCoreApplication::postEvent(&mw, new AudioFailureEvent(mw));
}

void MediaWidget::WorkerCallback::consumeBlitRequest() {
	if (blitRequested(blitState) && mw.running) {
		usec_t base, inc;
		BlitterWidget *const blitter = mw.blitterContainer->blitter();
		MediaWorker *const worker = mw.worker;

		worker->source()->generateVideoFrame(blitter->inBuffer());
		blitter->blit();

		base = synctimebase;
		inc  = synctimeinc;

		SyncVar::Locked(worker->waitingForSync()).set(true);

		blitter->draw();

		if (!blitter->frameTimeEst())
			asleep.sleepUntil(base, inc);

		if (blitter->sync() < 0)
			mw.emitVideoBlitterFailure();

		worker->setFrameTimeEstimate(blitter->frameTimeEst());
		mw.dwmControl_.tick();
	}
}

static auto_vector<BlitterWidget> makeBlitterWidgets(const VideoBufferLocker vbl, const DwmControlHwndChange hwndc) {
	auto_vector<BlitterWidget> blitters;
	
	addBlitterWidgets(blitters, vbl);
	blitters.push_back(new QGLBlitter(vbl, hwndc));
	blitters.push_back(new QPainterBlitter(vbl));
	
	for (auto_vector<BlitterWidget>::iterator it = blitters.begin(); it != blitters.end();) {
		if ((*it)->isUnusable()) {
			it = blitters.erase(it);
		} else
			++it;
	}

	return blitters;
}

static auto_vector<AudioEngine> makeAudioEngines(const WId winId) {
	auto_vector<AudioEngine> audioEngines;
	
	addAudioEngines(audioEngines, winId);
	audioEngines.push_back(new NullAudioEngine);
	
	return audioEngines;
}

MediaWidget::MediaWidget(MediaSource *const source, QWidget &parent)
: QObject(&parent),
  vbmut(),
  blitterContainer(new BlitterContainer(&parent)),
  audioEngines(makeAudioEngines(parent.winId())),
  blitters(makeBlitterWidgets(VideoBufferLocker(vbmut), DwmControlHwndChange(dwmControl_))),
  fullModeToggler(getFullModeToggler(parent.winId())),
  workerCallback_(new WorkerCallback(*this)),
  worker(new MediaWorker(source, audioEngines.back(), 48000, 100,
		std::auto_ptr<MediaWorker::Callback>(workerCallback_), this)),
  frameRateControl(*worker, blitters.back()),
  cursorTimer(new QTimer(this)),
  jsTimer(new QTimer(this)),
  dwmControl_(blitters.get()),
  focusPauseBit(0),
  running(false)
{
	for (auto_vector<BlitterWidget>::const_iterator it = blitters.begin(); it != blitters.end(); ++it) {
		(*it)->setVisible(false);
		(*it)->setParent(blitterContainer);
	}

	worker->setSamplesPerFrame(Rational(48000 / 60));
	blitterContainer->setBlitter(blitters.back());
	blitterContainer->blitter()->setPaused(false);
	blitterContainer->setMinimumSize(QSize(320, 240));
	blitterContainer->setSourceSize(QSize(320, 240));
	blitterContainer->setAspectRatio(QSize(320, 240));
	connect(fullModeToggler.get(), SIGNAL(rateChange(int)), this, SLOT(refreshRateChange(int)));
	fullModeToggler->emitRate();

	cursorTimer->setSingleShot(true);
	cursorTimer->setInterval(2000);
	connect(cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));

	jsTimer->setInterval(200);
	connect(jsTimer, SIGNAL(timeout()), this, SLOT(updateJoysticks()));

	dwmControl_.setDwmTripleBuffer(true);
}

MediaWidget::~MediaWidget() {
	if (fullModeToggler->isFullMode()) {
		fullModeToggler->setFullMode(false);
	}
}

void MediaWidget::run() {
	if (running)
		return;

	running = true;

#ifdef Q_WS_WIN
	timeBeginPeriod(1);
#endif

	blitterContainer->blitter()->setVisible(true);
	blitterContainer->blitter()->init();
	blitterContainer->blitter()->setVideoFormat(blitterContainer->sourceSize().width(),
	                                            blitterContainer->sourceSize().height());

	if (pauser.isPaused()) {
		jsTimer->start();
		worker->pause();
	}

	SDL_JoystickUpdate();
	SDL_ClearEvents();
	worker->start();
}

void MediaWidget::stop() {
	if (!running)
		return;

	worker->stop();
	jsTimer->stop();
	running = false;

	blitterContainer->blitter()->uninit();
	blitterContainer->blitter()->setVisible(false);

#ifdef Q_WS_WIN
	timeEndPeriod(1);
#endif
}

void MediaWidget::setBlitter(BlitterWidget *const blitter) {
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
		frameRateControl.setBlitter(blitterContainer->blitter());

		if (running)
			blitterContainer->blitter()->init();
	}
}

void MediaWidget::setVideo(const unsigned w, const unsigned h, BlitterWidget *const blitter) {
	if (QSize(w, h) != blitterContainer->sourceSize() || blitter != blitterContainer->blitter()) {
		{
			const QMutexLocker vblock(&vbmut);
			setBlitter(blitter);

			if (running)
				blitterContainer->blitter()->setVideoFormat(w, h);
		}
		
		blitterContainer->setSourceSize(QSize(w, h));
	}
}

void MediaWidget::execPausedQueue() {
	if (worker->paused() && !pausedq.empty()) {
		pausedq.pop_all();
		worker->qUnpause();
	}
}

void MediaWidget::customEvent(QEvent *const ev) {
	reinterpret_cast<CustomEvent*>(ev)->exec();
}

void MediaWidget::resizeEvent(const QWidget *parent) {
	fullModeToggler->setScreen(parent);
#ifdef Q_WS_WIN
	// Events are blocked while resizing on windows. Workaround.
	workerCallback_->consumeBlitRequest();
#endif
}

void MediaWidget::focusOutEvent() {
	pauser.set(focusPauseBit, *this);

// #ifndef Q_WS_MAC // Minimize is ugly on mac (especially full screen windows) and there doesn't seem to be a "qApp->hide()" which would be more appropriate.
// 	if (isFullScreen() && fullModeToggler->isFullMode() && !qApp->activeWindow()/* && QApplication::desktop()->numScreens() == 1*/) {
// 		fullModeToggler->setFullMode(false);
// 		showMinimized();
// 	}
// #endif

	blitterContainer->showCursor();
	cursorTimer->stop();
}

void MediaWidget::focusInEvent() {
// 	if (isFullScreen() && !fullModeToggler->isFullMode()) {
// 		fullModeToggler->setFullMode(true);
// 		correctFullScreenGeometry();
// 	}

// 	SDL_JoystickUpdate();
// 	SDL_ClearEvents();

	cursorTimer->start();
	pauser.unset(focusPauseBit, *this);
}

void MediaWidget::setPauseOnFocusOut(const unsigned bitmask, const bool hasFocus) {
	if (bitmask != focusPauseBit) {
		if (hasFocus) {
			pauser.unset(focusPauseBit, *this);
		} else {
			pauser.set(bitmask, *this);
			pauser.unset(focusPauseBit & ~bitmask, *this);
		}

		focusPauseBit = bitmask;
	}
}

void MediaWidget::keyPressEvent(QKeyEvent *e) {
	e->ignore();

	if (running && !e->isAutoRepeat()) {
		worker->source()->keyPressEvent(e);
		blitterContainer->hideCursor();
	}
}

void MediaWidget::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();

	if (running && !e->isAutoRepeat())
		worker->source()->keyReleaseEvent(e);
}

void MediaWidget::setFrameTime(long num, long denom) {
	if (!num) {
		num = 1;
		denom = 0xFFFF;
	} else if (!denom) {
		num = 0xFFFF;
		denom = 1;
	}
	
	frameRateControl.setFrameTime(Rational(num, denom));
}

void MediaWidget::waitUntilPaused() {
	while (!pausedq.empty())
		QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
}

void MediaWidget::refreshRateChange(int refreshRate) {
	frameRateControl.setRefreshRate(refreshRate);
}

void MediaWidget::updateJoysticks() {
	worker->updateJoysticks();
}

struct MediaWidget::FrameStepFun {
	MediaWidget &mw;
	explicit FrameStepFun(MediaWidget &mw) : mw(mw) {}
	
	void operator()() const {
		if (mw.running && mw.worker->frameStep()) {
			BlitterWidget *const blitter = mw.blitterContainer->blitter();

			mw.worker->source()->generateVideoFrame(blitter->inBuffer());
			blitter->blit();
			blitter->draw();

			if (blitter->sync() < 0)
				mw.emitVideoBlitterFailure();
		}
	}
};

void MediaWidget::frameStep() {
	callWhenPaused(FrameStepFun(*this));
}

void MediaWidget::hideCursor() {
	blitterContainer->hideCursor();
}

void MediaWidget::emitAudioEngineFailure() {
	emit audioEngineFailure();
	worker->recover();
}
