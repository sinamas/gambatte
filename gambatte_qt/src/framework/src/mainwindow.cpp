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
#include "mainwindow.h"
#include "mediawidget.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QtGlobal> // for Q_WS_WIN define

MainWindow::FrameBuffer::Locked::Locked(FrameBuffer fb) : mw(0), pb() {
	if (fb.mw->tryLockFrameBuf(pb))
		mw = fb.mw;
}

MainWindow::FrameBuffer::Locked::~Locked() {
	if (mw)
		mw->unlockFrameBuf();
}

MainWindow::MainWindow(MediaSource *const source)
: w_(new MediaWidget(source, *this)), winSize_(-1, -1), fullscreen_(false)
{
	setFocusPolicy(Qt::StrongFocus);
	setCentralWidget(w_->widget());
	setMouseTracking(true);
	setFocus();
	
	connect(w_, SIGNAL( audioEngineFailure()), this, SIGNAL( audioEngineFailure()));
	connect(w_, SIGNAL(videoBlitterFailure()), this, SIGNAL(videoBlitterFailure()));
}

void MainWindow::run() { w_->run(); }
void MainWindow::stop() { w_->stop(); }

void MainWindow::setWindowSize(const QSize &sz) {
	winSize_ = sz;
	
	if (!fullscreen_ && isVisible())
		doSetWindowSize(sz);
}

void MainWindow::toggleFullScreen() {
	if (fullscreen_) {
		fullscreen_ = false;
		w_->parentExclusiveEvent(false);
		w_->setFullMode(false);
		showNormal();
		
		if (isVisible())
			doSetWindowSize(winSize_);
		
		activateWindow();
	} else {
		fullscreen_ = true;
		
		if (isVisible())
			doShowFullScreen();
	}
}

void MainWindow::setVideoFormat(unsigned w, unsigned h) {
	w_->setVideoFormat(w, h);
	
	if (winSize_ == QSize(-1, -1))
		centralWidget()->setMinimumSize(w_->videoSize());
}

void MainWindow::setVideoFormatAndBlitter(unsigned w, unsigned h, std::size_t blitterNo) {
	w_->setVideoFormatAndBlitter(w, h, blitterNo);
	
	if (winSize_ == QSize(-1, -1))
		centralWidget()->setMinimumSize(w_->videoSize());
}

void MainWindow::setVideoBlitter(std::size_t blitterNo) { w_->setVideoBlitter(blitterNo); }
void MainWindow::setAspectRatio(const QSize &ar) { w_->setAspectRatio(ar); }
void MainWindow::setScalingMethod(ScalingMethod smet) { w_->setScalingMethod(smet); }

void MainWindow::setAudioOut(std::size_t engineNo, unsigned srateHz, unsigned msecLatency) { w_->setAudioOut(engineNo, srateHz, msecLatency); }
void MainWindow::setFrameTime(long num, long denom) { w_->setFrameTime(num, denom); }
void MainWindow::waitUntilPaused() { w_->waitUntilPaused(); }
void MainWindow::setSamplesPerFrame(long num, long denom) { w_->setSamplesPerFrame(num, denom); }
void MainWindow::setSyncToRefreshRate(bool on) { w_->setSyncToRefreshRate(on); }

void MainWindow::setFullScreenMode(std::size_t screenNo, std::size_t resIndex, std::size_t rateIndex) {
	if (screenNo < w_->screens()
			&& (w_->currentResIndex(screenNo) != resIndex || w_->currentRateIndex(screenNo) != rateIndex)) {
		if (w_->isFullMode() && screenNo == w_->currentScreen()) {
			w_->parentExclusiveEvent(false);
			w_->setMode(screenNo, resIndex, rateIndex);
#ifdef Q_WS_WIN
			showNormal();
			showFullScreen();
#endif
			correctFullScreenGeometry();
			w_->parentExclusiveEvent(isFullScreen() & hasFocus());
		} else
			w_->setMode(screenNo, resIndex, rateIndex);
	}
}

const std::vector<ResInfo>& MainWindow::modeVector(std::size_t screen) const { return w_->modeVector(screen); }
const QString MainWindow::screenName(std::size_t screen) const { return w_->screenName(screen); }
std::size_t MainWindow::screens() const { return w_->screens(); }
std::size_t MainWindow::currentResIndex(std::size_t screen) const { return w_->currentResIndex(screen); }
std::size_t MainWindow::currentRateIndex(std::size_t screen) const { return w_->currentRateIndex(screen); }
std::size_t MainWindow::currentScreen() const { return w_->currentScreen(); }
const BlitterConf MainWindow::currentBlitterConf() { return w_->currentBlitterConf(); }
const ConstBlitterConf MainWindow::currentBlitterConf() const { return w_->currentBlitterConf(); }
void MainWindow::frameStep() { w_->frameStep(); }
void MainWindow::hideCursor() { w_->hideCursor(); }
void MainWindow::setFastForwardSpeed(unsigned speed) { w_->setFastForwardSpeed(speed); }
void MainWindow::setFastForward(bool enable) { w_->setFastForward(enable); }
void MainWindow::pause(unsigned bitmask) { w_->pause(bitmask); }
void MainWindow::unpause(unsigned bitmask) { w_->unpause(bitmask); }
void MainWindow::incPause(unsigned inc) { w_->incPause(inc); }
void MainWindow::decPause(unsigned dec) { w_->decPause(dec); }
bool MainWindow::isRunning() const { return w_->isRunning(); }
const BlitterConf MainWindow::blitterConf(std::size_t blitterNo) { return w_->blitterConf(blitterNo); }
const ConstBlitterConf MainWindow::blitterConf(std::size_t blitterNo) const { return w_->blitterConf(blitterNo); }
std::size_t MainWindow::numBlitters() const { return w_->numBlitters(); }
const AudioEngineConf MainWindow::audioEngineConf(std::size_t aeNo) { return w_->audioEngineConf(aeNo); }
const ConstAudioEngineConf MainWindow::audioEngineConf(std::size_t aeNo) const { return w_->audioEngineConf(aeNo); }
std::size_t MainWindow::numAudioEngines() const { return w_->numAudioEngines(); }
std::size_t MainWindow::numResamplers() const { return w_->numResamplers(); }
const char* MainWindow::resamplerDesc(std::size_t resamplerNo) const { return w_->resamplerDesc(resamplerNo); }
void MainWindow::setResampler(std::size_t resamplerNo) { w_->setResampler(resamplerNo); }
void MainWindow::resetAudio() { w_->resetAudio(); }
void MainWindow::setDwmTripleBuffer(bool enable) { w_->setDwmTripleBuffer(enable); }
bool MainWindow::hasDwmCapability() { return DwmControl::hasDwmCapability(); }
bool MainWindow::isDwmCompositionEnabled() { return DwmControl::isCompositingEnabled(); }

void MainWindow::closeEvent(QCloseEvent */*e*/) {
	w_->stop();
	emit closing();
// 	w_->setFullMode(false); // avoid misleading auto-minimize on close focusOut event.
}

void MainWindow::hideEvent(QHideEvent *) {
	w_->hideEvent();
}

void MainWindow::showEvent(QShowEvent *) {
	// some window managers get pissed (xfwm4 breaks, metacity complains) if fixed window size is set too early.
	if (!fullscreen_)
		doSetWindowSize(winSize_);
	
	w_->showEvent(this);
}

void MainWindow::focusOutEvent(QFocusEvent *) {
	w_->parentExclusiveEvent(false);
	w_->focusOutEvent();
}

void MainWindow::focusInEvent(QFocusEvent *) {
	w_->focusInEvent();
	w_->parentExclusiveEvent(isFullScreen());
	
	// urk, delay full screen until getting focus if not visible to avoid WMs screwing up.
	if (fullscreen_ && !w_->isFullMode())
		doShowFullScreen();
}

void MainWindow::moveEvent(QMoveEvent *) { w_->moveEvent(this); }
void MainWindow::resizeEvent(QResizeEvent *) { w_->resizeEvent(this); }
void MainWindow::mouseMoveEvent(QMouseEvent *) { w_->mouseMoveEvent(); }
void MainWindow::setPauseOnFocusOut(unsigned bitmask) { w_->setPauseOnFocusOut(bitmask, hasFocus()); }
void MainWindow::keyPressEvent(QKeyEvent *e) { w_->keyPressEvent(e); }
void MainWindow::keyReleaseEvent(QKeyEvent *e) { w_->keyReleaseEvent(e); }

#ifdef Q_WS_WIN
bool MainWindow::winEvent(MSG *msg, long *) {
	if (w_->winEvent(msg))
		emit dwmCompositionChange();

	return false;
}
#endif

void MainWindow::doShowFullScreen() {
	const int screen = QApplication::desktop()->screenNumber(this);

	w_->setFullMode(true);
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
	centralWidget()->hide();
	centralWidget()->show();
#endif
	w_->parentExclusiveEvent(hasFocus());
}

void MainWindow::doSetWindowSize(const QSize &sz) {
	if (sz == QSize(-1, -1)) {
		centralWidget()->setMinimumSize(w_->videoSize());
		layout()->setSizeConstraint(QLayout::SetMinimumSize);
		setMinimumSize(1, 1); // needed on macx
		setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX); // needed on macx. needed for metacity full screen switching, layout()->setSizeConstraint(QLayout::SetMinAndMaxSize) won't do.
		resize(size()); // needed on macx
	} else {
		centralWidget()->setMinimumSize(sz);
		layout()->setSizeConstraint(QLayout::SetFixedSize);
	}
}

void MainWindow::correctFullScreenGeometry() {
	const QRect &screenRect = w_->fullScreenRect(this);

	if (geometry() != screenRect)
		setGeometry(screenRect);

}

CallWhenMediaWorkerPaused::CallWhenMediaWorkerPaused(MediaWidget &mw)
: worker_(*mw.worker), callq_(mw.pausedq)
{
	worker_.qPause();
}

CallWhenMediaWorkerPaused::~CallWhenMediaWorkerPaused() {
	if (worker_.paused()) {
		callq_.pop_all();
		worker_.qUnpause();
	}
}

PushMediaWorkerCall::PushMediaWorkerCall(MediaWidget &mw)
: worker_(*mw.worker), callq_(worker_.pauseVar.callq)
{
	worker_.pauseVar.mut.lock();
}

PushMediaWorkerCall::~PushMediaWorkerCall() {
	worker_.pauseVar.cond.wakeAll();

	if (AtomicVar<bool>::ConstLocked(worker_.doneVar).get())
		callq_.pop_all();

	worker_.pauseVar.mut.unlock();
}
