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

#include <iostream>

#include <QtGui>
#include <QVBoxLayout>

#include "gambatte_qt.h"
#include "blitterwidgets/qpainterblitter.h"
#include "blitterwidgets/qglblitter.h"
#include "videodialog.h"
#include "inputdialog.h"
#include "blittercontainer.h"

#include "addaudioengines.h"
#include "addblitterwidgets.h"
#include "getfullrestoggler.h"
#include "audioengine.h"
#include "fullrestoggler.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

#include "resizesignalingmenubar.h"

GambatteQt::GambatteQt() : resetVideoBuffer(gambatte) {
	blitter = NULL;
	ae = NULL;
	timerId = 0;
	running = false;
	turbo = false;
	//samplesPrFrame = ((sampleFormat.rate * 4389) / 262144) + 1;
	samplesPrFrame = 0;
	sndBuffer = NULL;
	
	setAttribute(Qt::WA_DeleteOnClose);

	setFocusPolicy(Qt::StrongFocus);
	
	{
		ResizeSignalingMenuBar *m = new ResizeSignalingMenuBar;
		connect(m, SIGNAL(resized()), this, SLOT(resetWindowSize()));
		setMenuBar(m);
	}
	
	{
		QSettings settings;
		settings.beginGroup("mainwindow");
		resize(settings.value("size", QSize(160, 144)).toSize());
		settings.endGroup();
	}
	
	addAudioEngines(audioEngines, winId());
	
	inputDialog = new InputDialog(this);

	fullResToggler = getFullResToggler();

	addBlitterWidgets(blitters, resetVideoBuffer);
	blitters.push_back(new QGLBlitter);
	blitters.push_back(new QPainterBlitter(resetVideoBuffer));
	
	for (std::vector<BlitterWidget*>::iterator it = blitters.begin(); it != blitters.end();) {
		if ((*it)->isUnusable()) {
			delete *it;
			it = blitters.erase(it);
		} else
			++it;
	}

	videoDialog = new VideoDialog(blitters, gambatte.filterInfo(), *fullResToggler, this);
	connect(videoDialog, SIGNAL(accepted()), this, SLOT(videoSettingsChange()));

	/*QPalette pal = palette();
	pal.setColor(QPalette::AlternateBase, QColor(0, 0, 0));
	setPalette(pal);
	setBackgroundRole(QPalette::AlternateBase);
	setAutoFillBackground(true);*/
	
	blitterContainer = new BlitterContainer(*fullResToggler);
	blitterContainer->setMinimumSize(160, 144);
	setCentralWidget(blitterContainer);

	createActions();
	createMenus();

	setWindowTitle("gambatte");

	gambatte.setInputStateGetter(&inputGetter);
	
	{
		QSettings iniSettings(QSettings::IniFormat, QSettings::UserScope, "gambatte", "gambatte_qt");
		QString savepath = iniSettings.fileName();
		savepath.truncate(iniSettings.fileName().lastIndexOf("/") + 1);
		savepath += "saves";
		QDir::root().mkpath(savepath);
		gambatte.set_savedir(savepath.toAscii().data());
	}
	
	videoSettingsChange();
	
	setFocus();
}

GambatteQt::~GambatteQt() {
	delete fullResToggler;
	
	for (uint i = 0; i < blitters.size(); ++i)
		delete blitters[i];
	
	for (uint i = 0; i < audioEngines.size(); ++i)
		delete audioEngines[i];
	
	delete []sndBuffer;
}

void GambatteQt::open() {
	pause();
	
	QString fileName = QFileDialog::getOpenFileName(this, "Open", recentFileActs[0]->data().toString(), "Game Boy ROM Images (*.gb *.gbc *.sgb)");
	if (!fileName.isEmpty())
		loadFile(fileName);
	
	unpause();
}

void GambatteQt::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		loadFile(action->data().toString());
}

void GambatteQt::about() {
	pause();
	
	QMessageBox::about(
	        this,
	        tr("About gambatte"),
	        tr("gambatte is a Game Boy / Game Boy Color emulator.")
	);
	
	unpause();
}

void GambatteQt::resetWindowSize() {
	if (isFullScreen())
		return;
	
	const QSize s(videoDialog->winRes());
	
	if (s == QSize(-1, -1)) {
		setMinimumSize(0, 0);
		setMaximumSize(0xFFFFFF, 0xFFFFFF);
	} else {
		setFixedSize(s.width(), s.height() + (menuBar()->isVisible() ? menuBar()->height() : 0));
	}
}

void GambatteQt::toggleFullScreen() {
	if (fullResToggler->isFullRes()) {
		fullResToggler->setFullRes(false);
		showNormal();
		resetWindowSize();
	} else {
		fullResToggler->setFullRes(true);
		showFullScreen();
	}
}

void GambatteQt::toggleMenuHidden() {
	menuBar()->setVisible(!menuBar()->isVisible());
	
	if (menuBar()->isVisible())
		centralWidget()->unsetCursor();
	else
		centralWidget()->setCursor(Qt::BlankCursor);
	
	resetWindowSize();
}

void GambatteQt::videoSettingsChange() {
	{
		const int engineIndex = videoDialog->engine();
		
		if (blitter != blitters[engineIndex]) {
			bool updatesEnabled = true;
			bool visible = false;
			
			if (blitter) {
				updatesEnabled = blitter->updatesEnabled();
				visible = blitter->isVisible();
				disconnect(fullResToggler, SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
				
				if (running)
					blitter->uninit();
				
				blitter->setVisible(false);
			}
			
			blitter = blitters[engineIndex];
			blitter->setVisible(false);
			blitter->setUpdatesEnabled(updatesEnabled);
			//connect(fullResToggler, SIGNAL(modeChange()), blitter, SLOT(modeChange()));
			connect(fullResToggler, SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
			fullResToggler->emitRate();
			blitterContainer->setBlitter(blitter);
			blitter->setVisible(visible);
			
			if (running) {
// 				XSync(QX11Info::display(), 0);
				blitter->init();
				gambatte.setVideoBlitter(blitter);
			}
		}
	}
	
	gambatte.setVideoFilter(videoDialog->filterIndex());
	centralWidget()->setMinimumSize(gambatte.videoWidth(), gambatte.videoHeight());
	
	resetWindowSize();
	
	if (fullResToggler->currentResIndex() != videoDialog->fullMode() ||
			fullResToggler->currentRateIndex() != videoDialog->fullRate()) {
		fullResToggler->setMode(videoDialog->fullMode(), videoDialog->fullRate());
		
#ifdef PLATFORM_WIN32
		if (fullResToggler->isFullRes()) {
			showNormal();
			showFullScreen();
		}
#endif
	}
	
	setSamplesPrFrame();
	
	blitter->keepAspectRatio(videoDialog->keepsRatio());
	blitter->scaleByInteger(videoDialog->scalesByInteger());
	
	blitterContainer->updateLayout();
}

void GambatteQt::createActions() {
	exitAct = new QAction(tr("E&xit"), this);
	exitAct->setShortcut(tr("Ctrl+Q"));
	exitAct->setStatusTip(tr("Exit the application"));
	connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
	
	hideMenuAct = new QAction(this);
	hideMenuAct->setVisible(false);
	hideMenuAct->setShortcut(tr("Esc"));
	connect(hideMenuAct, SIGNAL(triggered()), this, SLOT(toggleMenuHidden()));
	
	fsAct = new QAction(tr("&Full screen"), this);
	fsAct->setShortcut(tr("Ctrl+F"));
	fsAct->setCheckable(true);
	connect(fsAct, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
}

void GambatteQt::createMenus() {
	QAction *openAct = new QAction(tr("&Open..."), this);
	openAct->setShortcut(tr("Ctrl+O"));
	openAct->setStatusTip(tr("Open an existing file"));
	connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
	
	for (int i = 0; i < MaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}
	
	QAction *aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the application's About box"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAct);
	separatorAct = fileMenu->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		fileMenu->addAction(recentFileActs[i]);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);
	updateRecentFileActions();

	QMenu *videom = menuBar()->addMenu(tr("&Settings"));
	
	{
		QAction *inputDialogAct = new QAction(tr("&Input..."), this);
		connect(inputDialogAct, SIGNAL(triggered()), this, SLOT(execInputDialog()));
		videom->addAction(inputDialogAct);
	}
	
	{
		QAction *videoDialogAct = new QAction(tr("&Video..."), this);
		connect(videoDialogAct, SIGNAL(triggered()), this, SLOT(execVideoDialog()));
		videom->addAction(videoDialogAct);
	}
	
	videom->addAction(fsAct);
	videom->addAction(hideMenuAct);

	menuBar()->addSeparator();

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(aboutAct);
}

void GambatteQt::loadFile(const QString &fileName) {
	if (gambatte.load((fileName.toAscii()).data())) {
		stop();
		QMessageBox::critical(
		        this,
		        tr("Error"),
		        tr("Failed to load file ") + fileName + "."
		);
		return;
	}

	setCurrentFile(fileName);

	run();
}

void GambatteQt::execDialog(QDialog *const dialog) {
	pause();
	dialog->exec();
	unpause();
}

void GambatteQt::execVideoDialog() {
	execDialog(videoDialog);
}

void GambatteQt::execInputDialog() {
	execDialog(inputDialog);
}

void GambatteQt::setSamplesPrFrame() {
	const BlitterWidget::Rational r = blitter->frameTime();
	const unsigned old = samplesPrFrame;
	samplesPrFrame = ((sampleRate * r.numerator) / r.denominator) + 1;
	
	if (old != samplesPrFrame) {
		delete []sndBuffer;
		sndBuffer = new int16_t[(samplesPrFrame + 4) * 2];
		
		samplesCalc.setBaseSamples(samplesPrFrame);
	}
}

AudioEngine* GambatteQt::initAudio() {
	AudioEngine *ae;
	int rate = -1;
	
	while (!audioEngines.empty()) {
		ae = audioEngines.back();
		rate = ae->init();
		
		if (rate >= 0)
			break;
		
		delete ae;
		audioEngines.pop_back();
	}
	
	if (rate < 0) {
		ae = NULL;
		rate = 0;
	}
	
	sampleRate = rate;
	
	setSamplesPrFrame();
	
	return ae;
}

void GambatteQt::timerEvent(QTimerEvent */*event*/) {
	gambatte.runFor(70224);
	
	if (!turbo)
		gambatte.fill_buffer(reinterpret_cast<uint16_t*>(sndBuffer), samplesCalc.getSamples());
	
	if (blitter->sync(turbo)) {
		QMessageBox::critical(this, tr("Error"), tr("Video engine failure."));
		blitter->uninit();
		blitter->init();
		gambatte.setVideoBlitter(blitter);
		
		if (ae)
			ae->pause();
		
		videoDialog->exec();
		return;
	}
	
	if (!turbo) {
		if (ae) {
			unsigned lastSamples = samplesCalc.getSamples();
			
			const AudioEngine::BufferState bufState = ae->bufferState();
			
			if (bufState.fromUnderrun != 0xFFFFFFFF)
				samplesCalc.update(bufState.fromUnderrun, bufState.fromOverflow);
			
			if (ae->write(sndBuffer, lastSamples) < 0) {
				delete ae;
				audioEngines.pop_back();
				ae = initAudio();
			}
		}
	}
}

void GambatteQt::run() {
	if (running)
		return;
	
	running = true;
	
	memset(&inputGetter.is, 0, sizeof(inputGetter.is));
	
	ae = initAudio();
	
#ifdef PLATFORM_WIN32
	timeBeginPeriod(1);
#endif

	blitter->setVisible(true);
	blitter->init();
	gambatte.setVideoBlitter(blitter);
	
	timerId = startTimer(0);
}

void GambatteQt::stop() {
	if (!running)
		return;
	
	running = false;
	
	killTimer(timerId);
	
	blitter->uninit();
	blitter->setVisible(false);

	ae->uninit();
	ae = NULL;
	
#ifdef PLATFORM_WIN32
	timeEndPeriod(1);
#endif
}

void GambatteQt::pause() {
	if (!running || !timerId)
		return;
	
	if (ae)
		ae->pause();
	
	killTimer(timerId);
	timerId = 0;
}

void GambatteQt::unpause() {
	if (!running || timerId)
		return;
	
	timerId = startTimer(0);
}

void GambatteQt::setCurrentFile(const QString &fileName) {
	QString curFile = fileName;
	if (curFile.isEmpty())
		setWindowTitle(tr("gambatte"));
	else
		setWindowTitle(tr("%1 - %2").arg(strippedName(curFile)) .arg(tr("gambatte")));

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings.setValue("recentFileList", files);

	foreach(QWidget *widget, QApplication::topLevelWidgets()) {
		GambatteQt *mainWin = qobject_cast<GambatteQt *>(widget);
		if (mainWin)
			mainWin->updateRecentFileActions();
	}
}

void GambatteQt::updateRecentFileActions() {
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);

	separatorAct->setVisible(numRecentFiles > 0);
}

QString GambatteQt::strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

void GambatteQt::keyPressEvent(QKeyEvent *e) {
// 	memset(&is, 0, sizeof(is));
	
	e->accept();
	
	if (e->key() == inputDialog->getStartKey())
		inputGetter.is.startButton = 1;
	else if (e->key() == inputDialog->getSelectKey())
		inputGetter.is.selectButton = 1;
	else if (e->key() == inputDialog->getBKey())
		inputGetter.is.bButton = 1;
	else if (e->key() == inputDialog->getAKey())
		inputGetter.is.aButton = 1;
	else if (e->key() == inputDialog->getDownKey()) {
		inputGetter.is.dpadDown = 1;
		inputGetter.is.dpadUp = 0;
	} else if (e->key() == inputDialog->getUpKey()) {
		inputGetter.is.dpadUp = 1;
		inputGetter.is.dpadDown = 0;
	} else if (e->key() == inputDialog->getLeftKey()) {
		inputGetter.is.dpadLeft = 1;
		inputGetter.is.dpadRight = 0;
	} else if (e->key() == inputDialog->getRightKey()) {
		inputGetter.is.dpadRight = 1;
		inputGetter.is.dpadLeft = 0;
	}

	switch (e->key()) {
	case Qt::Key_Tab: turbo = true; break;
	case Qt::Key_Escape:
		hideMenuAct->trigger();
		break;
	case Qt::Key_F:
		if (e->modifiers()&Qt::ControlModifier) {
			fsAct->trigger();
		}
		break;
	case Qt::Key_Q:
		if (e->modifiers()&Qt::ControlModifier) {
			exitAct->trigger();
		}
		break;
	default: e->ignore(); break;
	}
}

void GambatteQt::keyReleaseEvent(QKeyEvent *e) {
// 	memset(&is, 0, sizeof(is));
	
	if (e->key() == inputDialog->getStartKey())
		inputGetter.is.startButton = 0;
	else if (e->key() == inputDialog->getSelectKey())
		inputGetter.is.selectButton = 0;
	else if (e->key() == inputDialog->getBKey())
		inputGetter.is.bButton = 0;
	else if (e->key() == inputDialog->getAKey())
		inputGetter.is.aButton = 0;
	else if (e->key() == inputDialog->getDownKey())
		inputGetter.is.dpadDown = 0;
	else if (e->key() == inputDialog->getUpKey())
		inputGetter.is.dpadUp = 0;
	else if (e->key() == inputDialog->getLeftKey())
		inputGetter.is.dpadLeft = 0;
	else if (e->key() == inputDialog->getRightKey())
		inputGetter.is.dpadRight = 0;

	switch (e->key()) {
	case Qt::Key_Tab: turbo = false; break;
	default: e->ignore(); return;
	}

// 	e->accept();
}

void GambatteQt::closeEvent(QCloseEvent *e) {
	stop();
	
	if (!isFullScreen()) {
		QSettings settings;
		settings.beginGroup("mainwindow");
		settings.setValue("size", size());
		settings.endGroup();
	}
	
	QMainWindow::closeEvent(e);
}

/*void GambatteQt::showEvent(QShowEvent *event) {
	QMainWindow::showEvent(event);
	resetWindowSize();
}*/
