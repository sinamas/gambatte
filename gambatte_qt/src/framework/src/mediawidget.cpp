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

#include "mediawidget.h"
#include "adaptivesleep.h"
#include "addaudioengines.h"
#include "addblitterwidgets.h"
#include "audioengines/nullaudioengine.h"
#include "blitterwidgets/qpainterblitter.h"
#include "blitterwidgets/qglblitter.h"
#include "getfullmodetoggler.h"
#include "mediasource.h"
#include "SDL_event.h"
#include <QEvent>
#include <QKeyEvent>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QtGlobal> // for Q_WS_WIN define
#include <algorithm>

#ifdef Q_WS_WIN
#include <windows.h> // for timeBeginPeriod, timeEndPeriod
#endif

namespace {

struct CustomEvent : QEvent {
	CustomEvent() : QEvent(QEvent::MaxUser) {}
	virtual ~CustomEvent() {}
	virtual void exec() = 0;
};

}

MediaWidget::JoystickIniter::JoystickIniter() {
	SDL_JoystickInit();
	SDL_SetEventFilter(SDL_JOYAXISMOTION | SDL_JOYBUTTONCHANGE | SDL_JOYHATMOTION);

	for (int i = 0, n = SDL_NumJoysticks(); i < n; ++i) {
		if (SDL_Joystick *joy = SDL_JoystickOpen(i))
			joysticks_.push_back(joy);
	}

	SDL_JoystickUpdate();
	SDL_ClearEvents();
}

MediaWidget::JoystickIniter::~JoystickIniter() {
	std::for_each(joysticks_.begin(), joysticks_.end(), SDL_JoystickClose);
	SDL_JoystickQuit();
}

struct MediaWidget::Pauser::DoPause {
	MediaWidget &mw;
	explicit DoPause(MediaWidget &mw) : mw(mw) {}

	void operator()() const {
		if (mw.running_) {
			mw.worker_->pause();
			mw.jsTimer_->start();
		}

		mw.blitterContainer_->blitter()->setPaused(true);
	}
};

struct MediaWidget::Pauser::DoUnpause {
	MediaWidget &mw;
	explicit DoUnpause(MediaWidget &mw) : mw(mw) {}

	void operator()() const {
		if (mw.running_) {
			mw.jsTimer_->stop();
			mw.worker_->unpause();
		}

		mw.blitterContainer_->blitter()->setPaused(false);
	}
};

void MediaWidget::Pauser::modifyPaused(unsigned const newPaused, MediaWidget &mw) {
	if (paused_) {
		if (!newPaused)
			mw.callWhenPaused(DoUnpause(mw));
	} else {
		if (newPaused)
			mw.callWhenPaused(DoPause(mw));
	}

	paused_ = newPaused;
}

enum { blit_requested = 1, blit_posted = 2 };

static bool blitRequested(AtomicVar<unsigned> &blitState) {
	AtomicVar<unsigned>::Locked bs(blitState);
	if (bs.get() & blit_requested) {
		bs.set(blit_posted);
		return true;
	}

	return false;
}

class MediaWidget::WorkerCallback : public MediaWorker::Callback {
public:
	explicit WorkerCallback(MediaWidget &mw)
	: mw_(mw)
	, synctimebase_(0)
	, synctimeinc_(0)
	, blitState_(0)
	{
	}

	virtual void blit(usec_t synctimebase, usec_t synctimeinc);
	virtual bool cancelBlit();
	virtual void paused();
	virtual void audioEngineFailure();

	virtual bool tryLockVideoBuffer(PixelBuffer &pb) {
		if (mw_.vbmut_.tryLock()) {
			pb = mw_.blitterContainer_->blitter()->inBuffer();
			return true;
		}

		return false;
	}

	virtual void unlockVideoBuffer() { mw_.vbmut_.unlock(); }
	void consumeBlitRequest();

private:
	MediaWidget &mw_;
	AdaptiveSleep asleep_;
	usec_t synctimebase_, synctimeinc_;
	AtomicVar<unsigned> blitState_;
};

void MediaWidget::WorkerCallback::blit(usec_t const synctimebase, usec_t const synctimeinc) {
	struct BlitEvent : CustomEvent {
		WorkerCallback &cb;
		explicit BlitEvent(WorkerCallback &cb) : cb(cb) {}

		virtual void exec() {
			cb.consumeBlitRequest();

			AtomicVar<unsigned>::Locked bs(cb.blitState_);
			if (bs.get() & blit_requested) {
				QCoreApplication::postEvent(&cb.mw_, new BlitEvent(cb));
			} else
				bs.set(0);
		}
	};

	synctimebase_ = synctimebase;
	synctimeinc_  = synctimeinc;

	AtomicVar<unsigned>::Locked bs(blitState_);
	if (!(bs.get() & blit_posted))
		QCoreApplication::postEvent(&mw_, new BlitEvent(*this));

	bs.set(blit_requested | blit_posted);
}

bool MediaWidget::WorkerCallback::cancelBlit() {
	AtomicVar<unsigned>::Locked bs(blitState_);
	unsigned val = bs.get();
	bs.set(val & ~blit_requested);
	return val & blit_requested;
}

void MediaWidget::WorkerCallback::paused() {
	struct PausedEvent : CustomEvent {
		MediaWidget &mw;
		explicit PausedEvent(MediaWidget &mw) : mw(mw) {}
		virtual void exec() { mw.execPausedQueue(); }
	};

	QCoreApplication::postEvent(&mw_, new PausedEvent(mw_));
}

void MediaWidget::WorkerCallback::audioEngineFailure() {
	struct AudioFailureEvent : CustomEvent {
		MediaWidget &mw;
		explicit AudioFailureEvent(MediaWidget &mw) : mw(mw) {}
		virtual void exec() { mw.emitAudioEngineFailure(); }
	};

	QCoreApplication::postEvent(&mw_, new AudioFailureEvent(mw_));
}

void MediaWidget::WorkerCallback::consumeBlitRequest() {
	if (blitRequested(blitState_) && mw_.running_) {
		BlitterWidget *const blitter = mw_.blitterContainer_->blitter();
		MediaWorker *const worker = mw_.worker_;
		worker->source().generateVideoFrame(blitter->inBuffer());
		blitter->consumeInputBuffer();

		usec_t const base = synctimebase_;
		usec_t const inc  = synctimeinc_;
		SyncVar::Locked(worker->waitingForSync()).set(true);

		blitter->draw();
		if (!blitter->frameTimeEst())
			asleep_.sleepUntil(base, inc);

		if (blitter->present() < 0)
			mw_.emitVideoBlitterFailure();

		worker->setFrameTimeEstimate(blitter->frameTimeEst());
		mw_.dwmControl_.tick();
	}
}

static auto_vector<BlitterWidget> makeBlitterWidgets(VideoBufferLocker vbl, DwmControlHwndChange hwndc) {
	auto_vector<BlitterWidget> blitters;
	addBlitterWidgets(blitters, vbl);
	blitters.push_back(createQGLBlitter(vbl, hwndc).release());
	blitters.push_back(createQPainterBlitter(vbl).release());

	for (auto_vector<BlitterWidget>::iterator it = blitters.begin(); it != blitters.end();) {
		if ((*it)->isUnusable()) {
			it = blitters.erase(it);
		} else
			++it;
	}

	return blitters;
}

static auto_vector<AudioEngine> makeAudioEngines(WId winId) {
	auto_vector<AudioEngine> audioEngines;
	addAudioEngines(audioEngines, winId);
	audioEngines.push_back(new NullAudioEngine);

	return audioEngines;
}

MediaWidget::MediaWidget(MediaSource &source, QWidget &parent)
: QObject(&parent)
, blitterContainer_(new BlitterContainer(&parent))
, audioEngines_(makeAudioEngines(parent.winId()))
, blitters_(makeBlitterWidgets(VideoBufferLocker(vbmut_), DwmControlHwndChange(dwmControl_)))
, fullModeToggler_(getFullModeToggler(parent.winId()))
, workerCallback_(new WorkerCallback(*this))
, worker_(new MediaWorker(source, *audioEngines_.back(), 48000, 100, 1,
                          *workerCallback_, this))
, frameRateControl_(*worker_, blitters_.back())
, cursorTimer_(new QTimer(this))
, jsTimer_(new QTimer(this))
, dwmControl_(blitters_.get())
, focusPauseBit_(0)
, running_(false)
{
	for (auto_vector<BlitterWidget>::const_iterator it =
			blitters_.begin(); it != blitters_.end(); ++it) {
		(*it)->setVisible(false);
		(*it)->setParent(blitterContainer_);
	}

	worker_->setSamplesPerFrame(Rational(48000 / 60));
	blitterContainer_->setBlitter(blitters_.back());
	blitterContainer_->blitter()->setPaused(false);
	blitterContainer_->setMinimumSize(QSize(320, 240));
	blitterContainer_->setSourceSize(QSize(320, 240));
	blitterContainer_->setAspectRatio(QSize(320, 240));
	connect(fullModeToggler_.get(), SIGNAL(rateChange(int)), this, SLOT(refreshRateChange(int)));
	fullModeToggler_->emitRate();

	cursorTimer_->setSingleShot(true);
	cursorTimer_->setInterval(2000);
	connect(cursorTimer_, SIGNAL(timeout()), this, SLOT(hideCursor()));

	jsTimer_->setInterval(200);
	connect(jsTimer_, SIGNAL(timeout()), this, SLOT(updateJoysticks()));

	dwmControl_.setDwmTripleBuffer(true);
}

MediaWidget::~MediaWidget() {
	if (fullModeToggler_->isFullMode())
		fullModeToggler_->setFullMode(false);
}

void MediaWidget::run() {
	if (running_)
		return;

	running_ = true;
#ifdef Q_WS_WIN
	timeBeginPeriod(1);
#endif

	blitterContainer_->blitter()->setVisible(true);
	blitterContainer_->blitter()->init();
	blitterContainer_->blitter()->setVideoFormat(blitterContainer_->sourceSize());

	if (pauser_.isPaused()) {
		jsTimer_->start();
		worker_->pause();
	}

	SDL_JoystickUpdate();
	SDL_ClearEvents();
	worker_->start();
}

void MediaWidget::stop() {
	if (!running_)
		return;

	worker_->stop();
	jsTimer_->stop();
	running_ = false;

	blitterContainer_->blitter()->uninit();
	blitterContainer_->blitter()->setVisible(false);
#ifdef Q_WS_WIN
	timeEndPeriod(1);
#endif
}

void MediaWidget::setBlitter(BlitterWidget *const blitter) {
	if (blitterContainer_->blitter() != blitter) {
		bool visible = false;
		bool paused = false;
		if (blitterContainer_->blitter()) {
			visible = blitterContainer_->blitter()->isVisible();
			paused = blitterContainer_->blitter()->isPaused();

			if (running_)
				blitterContainer_->blitter()->uninit();

			blitterContainer_->blitter()->setVisible(false);
		}

		blitterContainer_->setBlitter(blitter);
		blitterContainer_->blitter()->setVisible(visible);
		blitterContainer_->blitter()->setPaused(paused);
		frameRateControl_.setBlitter(blitterContainer_->blitter());
		if (running_)
			blitterContainer_->blitter()->init();
	}
}

void MediaWidget::setVideo(QSize const &size, BlitterWidget *blitter) {
	if (size == blitterContainer_->sourceSize() && blitter == blitterContainer_->blitter())
		return;

	{
		QMutexLocker vblock(&vbmut_);
		setBlitter(blitter);
		if (running_)
			blitterContainer_->blitter()->setVideoFormat(size);
	}

	blitterContainer_->setSourceSize(size);
}

void MediaWidget::execPausedQueue() {
	if (worker_->paused() && !pausedq_.empty()) {
		pausedq_.pop_all();
		worker_->qUnpause();
	}
}

void MediaWidget::customEvent(QEvent *ev) {
	static_cast<CustomEvent *>(ev)->exec();
}

void MediaWidget::resizeEvent(QWidget const *parent) {
	fullModeToggler_->setScreen(parent);
#ifdef Q_WS_WIN
	// Events are blocked while resizing on windows. Workaround.
	workerCallback_->consumeBlitRequest();
#endif
}

void MediaWidget::focusOutEvent() {
	pauser_.set(focusPauseBit_, *this);

// Minimize is ugly on mac (especially full screen windows) and there does not seem to
// be a "qApp->hide()" which would be more appropriate.
// #ifndef Q_WS_MAC
// 	if (isFullScreen() && fullModeToggler_->isFullMode()
// 			&& !qApp->activeWindow()/* && QApplication::desktop()->numScreens() == 1*/) {
// 		fullModeToggler_->setFullMode(false);
// 		showMinimized();
// 	}
// #endif

	blitterContainer_->showCursor();
	cursorTimer_->stop();
}

void MediaWidget::focusInEvent() {
// 	if (isFullScreen() && !fullModeToggler_->isFullMode()) {
// 		fullModeToggler_->setFullMode(true);
// 		correctFullScreenGeometry();
// 	}

// 	SDL_JoystickUpdate();
// 	SDL_ClearEvents();

	cursorTimer_->start();
	pauser_.unset(focusPauseBit_, *this);
}

void MediaWidget::setPauseOnFocusOut(unsigned const bitmask, bool const hasFocus) {
	if (bitmask != focusPauseBit_) {
		if (hasFocus) {
			pauser_.unset(focusPauseBit_, *this);
		} else {
			pauser_.set(bitmask, *this);
			pauser_.unset(focusPauseBit_ & ~bitmask, *this);
		}

		focusPauseBit_ = bitmask;
	}
}

void MediaWidget::keyPressEvent(QKeyEvent *e) {
	e->ignore();

	if (running_ && !e->isAutoRepeat()) {
		worker_->source().keyPressEvent(e);
		blitterContainer_->hideCursor();
	}
}

void MediaWidget::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();

	if (running_ && !e->isAutoRepeat())
		worker_->source().keyReleaseEvent(e);
}

void MediaWidget::setFrameTime(long num, long denom) {
	if (num == 0) {
		num = 1;
		denom = 0xFFFF;
	} else if (denom == 0) {
		num = 0xFFFF;
		denom = 1;
	}

	frameRateControl_.setFrameTime(Rational(num, denom));
}

void MediaWidget::waitUntilPaused() {
	while (!pausedq_.empty())
		QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
}

void MediaWidget::refreshRateChange(int refreshRate) {
	frameRateControl_.setRefreshRate(refreshRate);
}

void MediaWidget::updateJoysticks() {
	worker_->updateJoysticks();
}

struct MediaWidget::FrameStepFun {
	MediaWidget &mw;
	explicit FrameStepFun(MediaWidget &mw) : mw(mw) {}

	void operator()() const {
		if (mw.running_ && mw.worker_->frameStep()) {
			BlitterWidget *const blitter = mw.blitterContainer_->blitter();
			mw.worker_->source().generateVideoFrame(blitter->inBuffer());
			blitter->consumeInputBuffer();
			blitter->draw();
			if (blitter->present() < 0)
				mw.emitVideoBlitterFailure();
		}
	}
};

void MediaWidget::frameStep() {
	callWhenPaused(FrameStepFun(*this));
}

void MediaWidget::hideCursor() {
	blitterContainer_->hideCursor();
}

void MediaWidget::emitAudioEngineFailure() {
	emit audioEngineFailure();
	worker_->recover();
}
