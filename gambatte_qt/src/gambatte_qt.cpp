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
#include "gambatte_qt.h"

#include <iostream>

#include <QtGui>
#include <QVBoxLayout>
#include <cstring>

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
#include "SDL_Joystick/include/SDL_joystick.h"

class GbKeyHandler {
	bool &gbButton;
	bool *const negGbButton;
public:
	GbKeyHandler(bool &gbButton, bool *negGbButton = NULL) : gbButton(gbButton), negGbButton(negGbButton) {}
	
	void handleValue(const bool keyPressed) {
		if ((gbButton = keyPressed) && negGbButton)
			*negGbButton = false;
	}
};

class GbJoyHandler {
	bool &gbButton;
	bool *const negGbButton;
	const int threshold;
public:
	GbJoyHandler(int threshold, bool &gbButton, bool *negGbButton = NULL) :
		gbButton(gbButton), negGbButton(negGbButton), threshold(threshold) {}
	
	void handleValue(const int value) {
		if ((gbButton = (value - threshold ^ threshold) >= 0) && negGbButton)
			*negGbButton = false;
	}
};

JoystickIniter::JoystickIniter() {
	SDL_JoystickInit();
	
	SDL_SetEventFilter(SDL_JOYAXISMOTION | SDL_JOYBUTTONCHANGE);
	
	const int numJs = SDL_NumJoysticks();
	
	for (int i = 0; i < numJs; ++i) {
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		
		if (joy)
			joysticks.push_back(joy);
	}
	
	SDL_JoystickUpdate();
	SDL_ClearEvents();
}

JoystickIniter::~JoystickIniter() {
	for (std::size_t i = 0; i < joysticks.size(); ++i)
		SDL_JoystickClose(joysticks[i]);
	
	SDL_JoystickQuit();
}

GambatteQt::GambatteQt(const int argc, const char *const argv[]) : resetVideoBuffer(gambatte) {
	blitter = NULL;
	ae = NULL;
	timerId = 0;
	running = false;
	turbo = false;
	//samplesPrFrame = ((sampleFormat.rate * 4389) / 262144) + 1;
	samplesPrFrame = 0;
	sampleRate = 0;
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
	connect(inputDialog, SIGNAL(accepted()), this, SLOT(inputSettingsChange()));

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
	
	blitterContainer = new BlitterContainer(*fullResToggler);
	blitterContainer->setMinimumSize(160, 144);
	setCentralWidget(blitterContainer);

	createActions();
	createMenus();

	setWindowTitle("Gambatte");

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
	inputSettingsChange();
	
	setFocus();
	
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			loadFile(QString(argv[i]));
			break;
		}
	}
}

GambatteQt::~GambatteQt() {
	clearInputVectors();

	delete fullResToggler;
	
	for (uint i = 0; i < blitters.size(); ++i)
		delete blitters[i];
	
	for (uint i = 0; i < audioEngines.size(); ++i)
		delete audioEngines[i];
	
	delete []sndBuffer;
}

void GambatteQt::open() {
	pause();
	
	QString fileName = QFileDialog::getOpenFileName(this, "Open", recentFileActs[0]->data().toString(), "Game Boy ROM images (*.dmg *.gb *.gbc *.sgb *.zip);;All files (*)");
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
	        tr("About Gambatte"),
	        tr("<h3>Gambatte Qt svn</h3>\
	            <p><b>Author:</b> Sindre Aamås (<a href=\"mailto:aamas@stud.ntnu.no\">aamas@stud.ntnu.no</a>).<br>\
	            <b>Homepage:</b> <a href=\"http://sourceforge.net/projects/gambatte\">http://sourceforge.net/projects/gambatte</a>.</p>\
	            <p>Gambatte is an accuracy-focused, open-source, cross-platform Game Boy / Game Boy Color emulator written in C++. It is based on hundreds of corner case hardware tests, as well as previous documentation and reverse engineering efforts.</p>")
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

void GambatteQt::clearInputVectors() {
	for (std::multimap<unsigned,GbKeyHandler*>::iterator it = keyInputs.begin(); it != keyInputs.end(); ++it)
		delete it->second;
		
	keyInputs.clear();
	
	for (std::multimap<unsigned,GbJoyHandler*>::iterator it = joyInputs.begin(); it != joyInputs.end(); ++it)
		delete it->second;
		
	joyInputs.clear();
}

void GambatteQt::pushGbInputHandler(const SDL_Event &data, bool &gbButton, bool *gbNegButton) {
	if (data.value)
		joyInputs.insert(std::pair<unsigned,GbJoyHandler*>(data.id, new GbJoyHandler(data.value, gbButton, gbNegButton)));
	else
		keyInputs.insert(std::pair<unsigned,GbKeyHandler*>(data.id, new GbKeyHandler(gbButton, gbNegButton)));
}

void GambatteQt::inputSettingsChange() {
	clearInputVectors();
	
	pushGbInputHandler(inputDialog->getUpData(), inputGetter.is.dpadUp, &inputGetter.is.dpadDown);
	pushGbInputHandler(inputDialog->getDownData(), inputGetter.is.dpadDown, &inputGetter.is.dpadUp);
	pushGbInputHandler(inputDialog->getLeftData(), inputGetter.is.dpadLeft, &inputGetter.is.dpadRight);
	pushGbInputHandler(inputDialog->getRightData(), inputGetter.is.dpadRight, &inputGetter.is.dpadLeft);
	pushGbInputHandler(inputDialog->getAData(), inputGetter.is.aButton);
	pushGbInputHandler(inputDialog->getBData(), inputGetter.is.bButton);
	pushGbInputHandler(inputDialog->getStartData(), inputGetter.is.startButton);
	pushGbInputHandler(inputDialog->getSelectData(), inputGetter.is.selectButton);
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
	updateJoysticks();
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
	
	std::memset(&inputGetter.is, 0, sizeof(inputGetter.is));
	
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
		setWindowTitle(tr("Gambatte"));
	else
		setWindowTitle(tr("%1 - %2").arg(strippedName(curFile)) .arg(tr("Gambatte")));

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

static bool unusedBool;

void GambatteQt::keyPressEvent(QKeyEvent *e) {
	e->accept();
	
	{
		using namespace std;
		
		pair<multimap<unsigned,GbKeyHandler*>::iterator,multimap<unsigned,GbKeyHandler*>::iterator> range = keyInputs.equal_range(e->key());
		
		for (multimap<unsigned,GbKeyHandler*>::iterator it = range.first; it != range.second; ++it)
			(it->second)->handleValue(true);
	}

	switch (e->key()) {
	case Qt::Key_Tab:
		turbo = true;
		
		if (ae)
			ae->pause();
			
		break;
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
	{
		using namespace std;
		
		pair<multimap<unsigned,GbKeyHandler*>::iterator,multimap<unsigned,GbKeyHandler*>::iterator> range = keyInputs.equal_range(e->key());
		
		for (multimap<unsigned,GbKeyHandler*>::iterator it = range.first; it != range.second; ++it)
			(it->second)->handleValue(false);
	}

	switch (e->key()) {
	case Qt::Key_Tab: turbo = false; break;
	default: e->ignore(); return;
	}
}

void GambatteQt::updateJoysticks() {
	using namespace std;
	
	SDL_ClearEvents();
	SDL_JoystickUpdate();
	
	SDL_Event ev;
	
	while (SDL_PollEvent(&ev)) {
		pair<multimap<unsigned,GbJoyHandler*>::iterator,multimap<unsigned,GbJoyHandler*>::iterator> range = joyInputs.equal_range(ev.id);
		
		for (multimap<unsigned,GbJoyHandler*>::iterator it = range.first; it != range.second; ++it)
			(it->second)->handleValue(ev.value);
	}
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
