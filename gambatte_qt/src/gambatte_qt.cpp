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
#include "sounddialog.h"
#include "palettedialog.h"
#include "blittercontainer.h"

#include "addaudioengines.h"
#include "addblitterwidgets.h"
#include "getfullrestoggler.h"
#include "audioengine.h"
#include "audioengines/nullaudioengine.h"
#include "fullrestoggler.h"

#ifdef PLATFORM_WIN32
#include <windows.h>
#endif

#include "resizesignalingmenubar.h"
#include "SDL_Joystick/include/SDL_joystick.h"

struct JoyObserver {
	virtual ~JoyObserver() {}
	virtual void valueChanged(int value) = 0;
};

class JoyAxisHandler : public JoyObserver {
	InputObserver &observer;
	const int threshold;
public:
	JoyAxisHandler(InputObserver &observer, const int threshold) : observer(observer), threshold(threshold) {}
	void valueChanged(const int value) { observer.valueChanged((value - threshold ^ threshold) >= 0); }
};

class JoyButHandler : public JoyObserver {
	InputObserver &observer;
public:
	JoyButHandler(InputObserver &observer) : observer(observer) {}
	void valueChanged(const int value) { observer.valueChanged(value); }
};

class JoyHatHandler : public JoyObserver {
	InputObserver &observer;
	const int mask;
public:
	JoyHatHandler(InputObserver &observer, const int mask) : observer(observer), mask(mask) {}
	void valueChanged(const int value) { observer.valueChanged((value & mask) == mask); }
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

GambatteQt::GambatteQt(const int argc, const char *const argv[]) :
	gbUpHandler(inputGetter.is.dpadUp, inputGetter.is.dpadDown),
	gbDownHandler(inputGetter.is.dpadDown, inputGetter.is.dpadUp),
	gbLeftHandler(inputGetter.is.dpadLeft, inputGetter.is.dpadRight),
	gbRightHandler(inputGetter.is.dpadRight, inputGetter.is.dpadLeft),
	gbAHandler(inputGetter.is.aButton),
	gbBHandler(inputGetter.is.bButton),
	gbStartHandler(inputGetter.is.startButton),
	gbSelectHandler(inputGetter.is.selectButton),
	resetVideoBuffer(gambatte),
	sndBuffer(NULL),
	blitter(NULL),
	fullResToggler(getFullResToggler()),
	ae(NULL),
	sampleRate(0),
	samplesPrFrame(0),
	timerId(0),
	running(false),
	turbo(false)
{
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
	audioEngines.push_back(new NullAudioEngine);
	soundDialog = new SoundDialog(audioEngines, this);
	connect(soundDialog, SIGNAL(accepted()), this, SLOT(soundSettingsChange()));
	
	inputDialog = new InputDialog(this);
	connect(inputDialog, SIGNAL(accepted()), this, SLOT(inputSettingsChange()));

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
		
		{
			QString palpath = savepath + "palettes";
			QDir::root().mkpath(palpath);
			globalPaletteDialog = new PaletteDialog(palpath, 0, this);
			romPaletteDialog = new PaletteDialog(palpath, globalPaletteDialog, this);
			connect(globalPaletteDialog, SIGNAL(accepted()), this, SLOT(globalPaletteChange()));
			connect(romPaletteDialog, SIGNAL(accepted()), this, SLOT(romPaletteChange()));
		}
		
		savepath += "saves";
		QDir::root().mkpath(savepath);
		gambatte.set_savedir(savepath.toAscii().data());
	}
	
	videoSettingsChange();
	inputSettingsChange();
	soundSettingsChange();
	
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
	keyInputs.clear();
	
	for (joymap_t::iterator it = joyInputs.begin(); it != joyInputs.end(); ++it)
		delete it->second;
		
	joyInputs.clear();
}

void GambatteQt::pushInputObserver(const SDL_Event &data, InputObserver &observer) {
	if (data.value == KBD_VALUE) {
		keyInputs.insert(std::pair<unsigned,InputObserver*>(data.id, &observer));
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

void GambatteQt::inputSettingsChange() {
	clearInputVectors();
	
	pushInputObserver(inputDialog->getUpData(), gbUpHandler);
	pushInputObserver(inputDialog->getDownData(), gbDownHandler);
	pushInputObserver(inputDialog->getLeftData(), gbLeftHandler);
	pushInputObserver(inputDialog->getRightData(), gbRightHandler);
	pushInputObserver(inputDialog->getAData(), gbAHandler);
	pushInputObserver(inputDialog->getBData(), gbBHandler);
	pushInputObserver(inputDialog->getStartData(), gbStartHandler);
	pushInputObserver(inputDialog->getSelectData(), gbSelectHandler);
}

void GambatteQt::soundSettingsChange() {
	if (running)
		initAudio();
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
				disconnect(fullResToggler.get(), SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
				
				if (running)
					blitter->uninit();
				
				blitter->setVisible(false);
			}
			
			blitter = blitters[engineIndex];
			blitter->setVisible(false);
			blitter->setUpdatesEnabled(updatesEnabled);
			//connect(fullResToggler, SIGNAL(modeChange()), blitter, SLOT(modeChange()));
			connect(fullResToggler.get(), SIGNAL(rateChange(int)), blitter, SLOT(rateChange(int)));
			fullResToggler->emitRate();
			blitterContainer->setBlitter(blitter);
			blitter->setVisible(visible);
			
			if (running) {
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
	resetVideoBuffer();
}

void GambatteQt::globalPaletteChange() {
	romPaletteDialog->externalChange();
	setDmgPaletteColors();
}

void GambatteQt::romPaletteChange() {
	globalPaletteDialog->externalChange();
	setDmgPaletteColors();
}

void GambatteQt::setDmgPaletteColors() {
	for (unsigned palnum = 0; palnum < 3; ++palnum)
		for (unsigned colornum = 0; colornum < 4; ++colornum)
			gambatte.setDmgPaletteColor(palnum, colornum, romPaletteDialog->getColor(palnum, colornum));
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
	
	fsAct = new QAction(tr("&Full Screen"), this);
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
	
	resetAct = new QAction(tr("&Reset"), this);
	resetAct->setShortcut(tr("Ctrl+R"));
	resetAct->setEnabled(false);
	connect(resetAct, SIGNAL(triggered()), recentFileActs[0], SLOT(trigger()));
	
	QAction *aboutAct = new QAction(tr("&About"), this);
	aboutAct->setStatusTip(tr("Show the application's About box"));
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	
	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAct);
	separatorAct = fileMenu->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
		fileMenu->addAction(recentFileActs[i]);
	fileMenu->addSeparator();
	fileMenu->addAction(resetAct);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAct);
	updateRecentFileActions();

	QMenu *settingsm = menuBar()->addMenu(tr("&Settings"));
	
	{
		QAction *act = new QAction(tr("&Input..."), this);
		connect(act, SIGNAL(triggered()), this, SLOT(execInputDialog()));
		settingsm->addAction(act);
	}
	
	{
		QAction *act = new QAction(tr("&Sound..."), this);
		connect(act, SIGNAL(triggered()), this, SLOT(execSoundDialog()));
		settingsm->addAction(act);
	}
	
	{
		QAction *act = new QAction(tr("&Video..."), this);
		connect(act, SIGNAL(triggered()), this, SLOT(execVideoDialog()));
		settingsm->addAction(act);
	}
	
	settingsm->addSeparator();
	
	{
		QMenu *const palm = settingsm->addMenu(tr("DMG &Palette"));
		
		{
			QAction *act = new QAction(tr("&Global..."), this);
			connect(act, SIGNAL(triggered()), this, SLOT(execGlobalPaletteDialog()));
			palm->addAction(act);
		}
		
		romPaletteAct = new QAction(tr("Current &ROM..."), this);
		romPaletteAct->setEnabled(false);
		connect(romPaletteAct, SIGNAL(triggered()), this, SLOT(execRomPaletteDialog()));
		palm->addAction(romPaletteAct);
	}
	
	settingsm->addSeparator();
	settingsm->addAction(fsAct);
	settingsm->addAction(hideMenuAct);

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
	
	if (!gambatte.isCgb()) {
		romPaletteDialog->setSettingsFile(QFileInfo(fileName).completeBaseName() + ".pal");
		setDmgPaletteColors();
	}
	
	romPaletteAct->setEnabled(!gambatte.isCgb());

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

void GambatteQt::execSoundDialog() {
	execDialog(soundDialog);
}

void GambatteQt::execGlobalPaletteDialog() {
	execDialog(globalPaletteDialog);
}

void GambatteQt::execRomPaletteDialog() {
	execDialog(romPaletteDialog);
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

void GambatteQt::initAudio() {
	if (ae)
		ae->uninit();
	
	ae = audioEngines[soundDialog->getEngineIndex()];
	
	if ((sampleRate = ae->init(soundDialog->getRate())) < 0) {
		ae = NULL;
		sampleRate = 0;
	}
	
	setSamplesPrFrame();
}

void GambatteQt::soundEngineFailure() {
	QMessageBox::critical(this, tr("Error"), tr("Sound engine failure."));
	soundDialog->exec();
}

void GambatteQt::timerEvent(QTimerEvent */*event*/) {
	if (!ae) { // avoid stupid recursive call detection by checking here rather than on init.
		soundEngineFailure();
		return;
	}
	
	updateJoysticks();
	gambatte.runFor(70224);
	
	if (!turbo)
		gambatte.fill_buffer(reinterpret_cast<uint16_t*>(sndBuffer), samplesCalc.getSamples());
	else
		gambatte.fill_buffer(0, 0);
	
	if (blitter->sync(turbo)) {
		QMessageBox::critical(this, tr("Error"), tr("Video engine failure."));
		blitter->uninit();
		blitter->init();
		gambatte.setVideoBlitter(blitter);
		
		ae->pause();
		
		videoDialog->exec();
		return;
	}
	
	if (!turbo) {
		unsigned lastSamples = samplesCalc.getSamples();
		
		const AudioEngine::BufferState bufState = ae->bufferState();
		
		if (bufState.fromUnderrun != 0xFFFFFFFF)
			samplesCalc.update(bufState.fromUnderrun, bufState.fromOverflow);
		
		if (ae->write(sndBuffer, lastSamples) < 0) {
			ae->pause();
			soundEngineFailure();
		}
	}
}

void GambatteQt::run() {
	if (running)
		return;
	
	running = true;
	
	resetAct->setEnabled(true);
	
	std::memset(&inputGetter.is, 0, sizeof(inputGetter.is));
	
#ifdef PLATFORM_WIN32
	timeBeginPeriod(1);
#endif
	
	initAudio();

	blitter->setVisible(true);
	blitter->init();
	gambatte.setVideoBlitter(blitter);
	
	timerId = startTimer(0);
}

void GambatteQt::stop() {
	if (!running)
		return;
	
	running = false;
	
	resetAct->setEnabled(false);
	
	killTimer(timerId);
	
	blitter->uninit();
	blitter->setVisible(false);

	if (ae)
		ae->uninit();
	
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
		setWindowTitle(tr("%1 - %2").arg(strippedName(curFile)).arg(tr("Gambatte")));

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
	e->accept();
	
	{
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
		
		for (keymap_t::iterator it = range.first; it != range.second; ++it)
			(it->second)->valueChanged(true);
	}

	switch (e->key()) {
	case Qt::Key_Tab:
		turbo = true;
		
		if (ae)
			ae->pause();
			
		break;
	case Qt::Key_Escape: hideMenuAct->trigger(); break;
	default: e->ignore(); break;
	}
	
	if (menuBar()->isHidden() && (e->modifiers()&Qt::ControlModifier)) {
		e->accept();
		switch (e->key()) {
		case Qt::Key_F: fsAct->trigger(); break;
		case Qt::Key_Q: exitAct->trigger(); break;
		case Qt::Key_R: if (resetAct->isEnabled()) { resetAct->trigger(); } break;
		default: break;
		}
	}
}

void GambatteQt::keyReleaseEvent(QKeyEvent *e) {
	{
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
		
		for (keymap_t::iterator it = range.first; it != range.second; ++it)
			(it->second)->valueChanged(false);
	}

	switch (e->key()) {
	case Qt::Key_Tab: turbo = false; break;
	default: e->ignore(); return;
	}
}

void GambatteQt::updateJoysticks() {
	SDL_ClearEvents();
	SDL_JoystickUpdate();
	
	SDL_Event ev;
	
	while (SDL_PollEvent(&ev)) {
		std::pair<joymap_t::iterator,joymap_t::iterator> range = joyInputs.equal_range(ev.id);
		
		for (joymap_t::iterator it = range.first; it != range.second; ++it)
			(it->second)->valueChanged(ev.value);
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
