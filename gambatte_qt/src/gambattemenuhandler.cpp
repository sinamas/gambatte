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
#include "gambattemenuhandler.h"

#include <QtGui>
#include <QActionGroup>
#include <QFileInfo>
#include <QSettings>
#include "palettedialog.h"
#include "blitterconf.h"
#include "mainwindow.h"
#include "sounddialog.h"
#include "videodialog.h"
#include "gambattesource.h"
#include "miscdialog.h"

static const QString strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

namespace {
struct TmpPauser {
	MainWindow *const mw;
	const unsigned inc;
	
	explicit TmpPauser(MainWindow *const mw, const unsigned inc = 4) : mw(mw), inc(inc) {
		mw->incPause(inc);
	}
	
	~TmpPauser() {
		mw->decPause(inc);
	}
};
}

FrameRateAdjuster::FrameTime::FrameTime(unsigned baseNum, unsigned baseDenom) : index(STEPS) {
	setBaseFrameTime(baseNum, baseDenom);
}

void FrameRateAdjuster::FrameTime::setBaseFrameTime(const unsigned baseNum, const unsigned baseDenom) {
	frameTimes[STEPS] = Rational(baseNum, baseDenom);
	
	const unsigned bnum = baseNum * 0x10000 / baseDenom;

	for (unsigned i = STEPS, num = bnum; i < STEPS * 2; ++i)
		frameTimes[i + 1] = Rational(num = num * 11 / 10, 0x10000);

	for (unsigned i = STEPS, num = bnum; i; --i)
		frameTimes[i - 1] = Rational(num = num * 10 / 11, 0x10000);
}

FrameRateAdjuster::FrameRateAdjuster(const MiscDialog &miscDialog, MainWindow &mw, QObject *const parent)
: QObject(parent),
  frameTime_(miscDialog.baseFps().height(), miscDialog.baseFps().width()),
  miscDialog_(miscDialog),
  mw_(mw),
  decFrameRateAction_(new QAction(tr("&Decrease Frame Rate"), &mw)),
  incFrameRateAction_(new QAction(tr("&Increase Frame Rate"), &mw)),
  resetFrameRateAction_(new QAction(tr("&Reset Frame Rate"), &mw)),
  enabled_(true)
{
	decFrameRateAction_->setShortcut(QString("Ctrl+D"));
	incFrameRateAction_->setShortcut(QString("Ctrl+I"));
	resetFrameRateAction_->setShortcut(QString("Ctrl+U"));
	
	connect(decFrameRateAction_,   SIGNAL(triggered()), this, SLOT(decFrameRate()));
	connect(incFrameRateAction_,   SIGNAL(triggered()), this, SLOT(incFrameRate()));
	connect(resetFrameRateAction_, SIGNAL(triggered()), this, SLOT(resetFrameRate()));
	connect(&miscDialog, SIGNAL(accepted()), this, SLOT(miscDialogChange()));
	
	changed();
}

const QList<QAction*> FrameRateAdjuster::actions() {
	QList<QAction*> l;
	l.append(decFrameRateAction_);
	l.append(incFrameRateAction_);
	l.append(resetFrameRateAction_);
	return l;
}

void FrameRateAdjuster::miscDialogChange() {
	const QSize &baseFps = miscDialog_.baseFps();
	frameTime_.setBaseFrameTime(baseFps.height(), baseFps.width());
	changed();
}

void FrameRateAdjuster::setDisabled(const bool disabled) {
	enabled_ = !disabled;
	changed();
}

void FrameRateAdjuster::decFrameRate() {
	if (enabled_) {
		frameTime_.inc();
		changed();
	}
}

void FrameRateAdjuster::incFrameRate() {
	if (enabled_) {
		frameTime_.dec();
		changed();
	}
}

void FrameRateAdjuster::resetFrameRate() {
	if (enabled_) {
		frameTime_.reset();
		changed();
	}
}

void FrameRateAdjuster::changed() {
	incFrameRateAction_->setEnabled(enabled_ && frameTime_.decPossible());
	decFrameRateAction_->setEnabled(enabled_ && frameTime_.incPossible());
	resetFrameRateAction_->setEnabled(enabled_ && frameTime_.resetPossible());
	
	const FrameTime::Rational &ft = enabled_ ? frameTime_.get() : frameTime_.base();
	mw_.setFrameTime(ft.num, ft.denom);
}

GambatteMenuHandler::GambatteMenuHandler(MainWindow *const mw,
		GambatteSource *const source, const int argc, const char *const argv[])
: mw(mw),
  source(source),
  soundDialog(new SoundDialog(mw, mw)),
  videoDialog(new VideoDialog(mw, source->generateVideoSourceInfos(), QString("Video filter:"), QSize(160, 144), mw)),
  miscDialog(new MiscDialog(mw)),
  stateSlotGroup(new QActionGroup(mw)),
  pauseInc(4)
{
	mw->setWindowTitle("Gambatte");
	source->inputDialog()->setParent(mw, source->inputDialog()->windowFlags());

	{
		QSettings iniSettings(QSettings::IniFormat, QSettings::UserScope, "gambatte", "gambatte_qt");
		QString savepath = iniSettings.fileName();
		savepath.truncate(iniSettings.fileName().lastIndexOf("/") + 1);

		{
			const QString &palpath = savepath + "palettes";
			QDir::root().mkpath(palpath);
			globalPaletteDialog = new PaletteDialog(palpath, 0, mw);
			romPaletteDialog = new PaletteDialog(palpath, globalPaletteDialog, mw);
			connect(globalPaletteDialog, SIGNAL(accepted()), this, SLOT(globalPaletteChange()));
			connect(romPaletteDialog, SIGNAL(accepted()), this, SLOT(romPaletteChange()));
		}

		savepath += "saves";
		QDir::root().mkpath(savepath);
		source->setSavedir(savepath.toLocal8Bit().constData());
	}

	QActionGroup *const romLoadedActions = new QActionGroup(mw);
	romLoadedActions->setExclusive(false);

	{
		for (int i = 0; i < MaxRecentFiles; ++i) {
			recentFileActs[i] = new QAction(mw);
			recentFileActs[i]->setVisible(false);
			connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		}

		QMenu *fileMenu = mw->menuBar()->addMenu(tr("&File"));
		fileMenu->addAction(tr("&Open..."), this, SLOT(open()), tr("Ctrl+O"));

		recentMenu = fileMenu->addMenu(tr("Open Re&cent"));

		for (int i = 0; i < MaxRecentFiles; ++i)
			recentMenu->addAction(recentFileActs[i]);

		fileMenu->addSeparator();
		romLoadedActions->addAction(fileMenu->addAction(tr("&Reset"), recentFileActs[0], SLOT(trigger()), tr("Ctrl+R")));
		fileMenu->addSeparator();

		romLoadedActions->addAction(fileMenu->addAction(tr("Save State &As..."), this, SLOT(saveStateAs())));
		romLoadedActions->addAction(fileMenu->addAction(tr("Load State &From..."), this, SLOT(loadStateFrom())));
		fileMenu->addSeparator();

		romLoadedActions->addAction(fileMenu->addAction(tr("&Save State"), this, SLOT(saveState()), QString("Ctrl+S")));
		romLoadedActions->addAction(fileMenu->addAction(tr("&Load State"), this, SLOT(loadState()), QString("Ctrl+L")));

		{
			QMenu *const stateSlotMenu = fileMenu->addMenu(tr("S&elect State Slot"));
			stateSlotMenu->setEnabled(false);
			stateSlotMenu->addAction(tr("&Previous"), this, SLOT(prevStateSlot()), QString("Ctrl+Z"));
			stateSlotMenu->addAction(tr("&Next"), this, SLOT(nextStateSlot()), QString("Ctrl+X"));
			stateSlotMenu->addSeparator();

			for (int i = 0; i < 10; ++i) {
				const int no = i == 9 ? 0 : i + 1;
				const QString &strno = QString::number(no);
				QAction *const action = stateSlotMenu->addAction("Slot &" + strno, this, SLOT(selectStateSlot()), strno);
				action->setCheckable(true);
				action->setData(no);
				stateSlotGroup->addAction(action);
			}
			
			connect(this, SIGNAL(romLoaded(bool)), stateSlotMenu, SLOT(setEnabled(bool)));
		}

		fileMenu->addSeparator();

		fileMenu->addAction(tr("E&xit"), qApp, SLOT(closeAllWindows()), tr("Ctrl+Q"));
		updateRecentFileActions();
	}
	
	FrameRateAdjuster *const frameRateAdjuster = new FrameRateAdjuster(*miscDialog, *mw, this);

	{
		QMenu *const playm = mw->menuBar()->addMenu(tr("&Play"));

		romLoadedActions->addAction(pauseAction = playm->addAction(tr("&Pause"), this, SLOT(pauseChange()), QString("Ctrl+P")));
		pauseAction->setCheckable(true);
		romLoadedActions->addAction(playm->addAction(tr("Frame &Step"), this, SLOT(frameStep()), QString("Ctrl+.")));
		playm->addSeparator();
		syncFrameRateAction = playm->addAction(tr("&Sync Frame Rate to Refresh Rate"));
		syncFrameRateAction->setCheckable(true);
		connect(syncFrameRateAction, SIGNAL(triggered(bool)), frameRateAdjuster, SLOT(setDisabled(bool)));
		connect(syncFrameRateAction, SIGNAL(triggered(bool)), mw               , SLOT(setSyncToRefreshRate(bool)));
		
		foreach (QAction *const action, frameRateAdjuster->actions())
			playm->addAction(romLoadedActions->addAction(action));
	}

	QMenu *const settingsm = mw->menuBar()->addMenu(tr("&Settings"));

	settingsm->addAction(tr("&Input..."), this, SLOT(execInputDialog()));
	settingsm->addAction(tr("&Miscellaneous..."), this, SLOT(execMiscDialog()));
	settingsm->addAction(tr("&Sound..."), this, SLOT(execSoundDialog()));
	settingsm->addAction(tr("&Video..."), this, SLOT(execVideoDialog()));

	settingsm->addSeparator();

	forceDmgAction = settingsm->addAction(tr("Force &DMG Mode"));
	forceDmgAction->setCheckable(true);

	{
		QMenu *const palm = settingsm->addMenu(tr("DMG &Palette"));

		palm->addAction(tr("&Global..."), this, SLOT(execGlobalPaletteDialog()));

		QAction *const romPaletteAct = palm->addAction(tr("Current &ROM..."), this, SLOT(execRomPaletteDialog()));
		romPaletteAct->setEnabled(false);
		connect(this, SIGNAL(dmgRomLoaded(bool)), romPaletteAct, SLOT(setEnabled(bool)));
	}

	settingsm->addSeparator();

	{
#ifndef Q_WS_MAC
		QAction *fsAct;
#endif
		fsAct = settingsm->addAction(tr("&Full Screen"), this, SLOT(toggleFullScreen()), tr("Ctrl+F"));
		fsAct->setCheckable(true);
	}

	romLoadedActions->setEnabled(false);

// 	settingsm->addAction(hideMenuAct);

	mw->menuBar()->addSeparator();

	QMenu *const helpMenu = mw->menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(tr("&About"), this, SLOT(about()));

	mw->addActions(mw->menuBar()->actions());

	{
		QAction *const escAct = new QAction(mw);
		escAct->setShortcut(tr("Esc"));
		connect(escAct, SIGNAL(triggered()), this, SLOT(escPressed()));
		mw->addAction(escAct);
	}

	mw->setSamplesPerFrame(35112);
	connect(source, SIGNAL(setTurbo(bool)), mw, SLOT(setFastForward(bool)));
	connect(source, SIGNAL(togglePause()), pauseAction, SLOT(trigger()));
	connect(source, SIGNAL(frameStep()), this, SLOT(frameStep()));
	connect(source, SIGNAL(decFrameRate()), frameRateAdjuster, SLOT(decFrameRate()));
	connect(source, SIGNAL(incFrameRate()), frameRateAdjuster, SLOT(incFrameRate()));
	connect(source, SIGNAL(resetFrameRate()), frameRateAdjuster, SLOT(resetFrameRate()));
	connect(source, SIGNAL(prevStateSlot()), this, SLOT(prevStateSlot()));
	connect(source, SIGNAL(nextStateSlot()), this, SLOT(nextStateSlot()));
	connect(source, SIGNAL(saveStateSignal()), this, SLOT(saveState()));
	connect(source, SIGNAL(loadStateSignal()), this, SLOT(loadState()));
	connect(videoDialog, SIGNAL(accepted()), this, SLOT(videoDialogChange()));
	connect(soundDialog, SIGNAL(accepted()), this, SLOT(soundDialogChange()));
	connect(miscDialog, SIGNAL(accepted()), this, SLOT(miscDialogChange()));
	connect(mw, SIGNAL(videoBlitterFailure()), this, SLOT(videoBlitterFailure()));
	connect(mw, SIGNAL(audioEngineFailure()), this, SLOT(audioEngineFailure()));
	connect(mw, SIGNAL(closing()), this, SLOT(saveWindowSizeIfNotFullScreen()));
	connect(this, SIGNAL(romLoaded(bool)), romLoadedActions, SLOT(setEnabled(bool)));
	connect(this, SIGNAL(romLoaded(bool)), stateSlotGroup->actions().at(0), SLOT(setChecked(bool)));
	
	videoDialogChange();
	soundDialogChange();
	miscDialogChange();
	
	mw->resize(QSettings().value("mainwindow/size", QSize(160, 144)).toSize());

	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			loadFile(QFileInfo(QString(argv[i])).absoluteFilePath());
			break;
		}
	}
}

void GambatteMenuHandler::updateRecentFileActions() {
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	const int numRecentFiles = qMin(files.size(), static_cast<int>(MaxRecentFiles));

	for (int i = 0; i < numRecentFiles; ++i) {
		const QString &text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}

	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);

	recentMenu->setEnabled(numRecentFiles > 0);
}

void GambatteMenuHandler::setCurrentFile(const QString &fileName) {
	mw->setWindowTitle(fileName.isEmpty() ? tr("Gambatte") : tr("%1 - %2").arg(strippedName(fileName)).arg(tr("Gambatte")));

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);

	while (files.size() > MaxRecentFiles)
		files.removeLast();

	settings.setValue("recentFileList", files);
	updateRecentFileActions();
}

void GambatteMenuHandler::loadFile(const QString &fileName) {
	TmpPauser tmpPauser(mw, 4);
	pauseAction->setChecked(false);
	pauseChange();
	mw->waitUntilPaused();

	if (source->load((fileName.toLocal8Bit()).constData(), forceDmgAction->isChecked())) {
		mw->stop();
		QMessageBox::critical(
		                       mw,
		                       tr("Error"),
		                       tr("Failed to load file ") + fileName + "."
		                     );

		return;
	}

	if (!source->isCgb()) {
		romPaletteDialog->setSettingsFile(QFileInfo(fileName).completeBaseName() + ".pal");
		setDmgPaletteColors();
	}

	setCurrentFile(fileName);
	
	emit romLoaded(true);
	emit dmgRomLoaded(!source->isCgb());

	mw->run();
}

void GambatteMenuHandler::open() {
	TmpPauser tmpPauser(mw, pauseInc);
	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Open"), recentFileActs[0]->data().toString(),
						tr("Game Boy ROM Images (*.dmg *.gb *.gbc *.sgb *.zip);;All Files (*)"));

	if (!fileName.isEmpty())
		loadFile(fileName);

	// giving back focus after getOpenFileName seems to fail at times, which can be problematic with current exclusive mode handling.
	mw->setFocus();
}

void GambatteMenuHandler::openRecentFile() {
	if (const QAction *const action = qobject_cast<QAction *>(sender()))
		loadFile(action->data().toString());
}

void GambatteMenuHandler::about() {
	TmpPauser tmpPauser(mw, pauseInc);

	QMessageBox::about(
	                    mw,
	                    "About Gambatte",
	                    QString::fromUtf8("<h3>Gambatte Qt SVN</h3>\
	                       <p><b>Author:</b> Sindre Aamås (<a href=\"mailto:sinamas@users.sourceforge.net\">sinamas@users.sourceforge.net</a>)<br>\
	                       <b>Homepage:</b> <a href=\"http://sourceforge.net/projects/gambatte\">http://sourceforge.net/projects/gambatte</a></p>\
	                       <p>Gambatte is an accuracy-focused, open-source, cross-platform Game Boy Color emulator written in C++. It is based on hundreds of corner case hardware tests, as well as previous documentation and reverse engineering efforts.</p>")
	                  );
}

void GambatteMenuHandler::globalPaletteChange() {
	romPaletteDialog->externalChange();
	setDmgPaletteColors();
}

void GambatteMenuHandler::romPaletteChange() {
	globalPaletteDialog->externalChange();
	setDmgPaletteColors();
}

namespace {
struct SetDmgPaletteColorFun {
	GambatteSource *source; unsigned palnum; unsigned colornum; unsigned rgb32;
	void operator()() const { source->setDmgPaletteColor(palnum, colornum, rgb32); }
};
}

void GambatteMenuHandler::setDmgPaletteColors() {
	for (unsigned palnum = 0; palnum < 3; ++palnum)
	for (unsigned colornum = 0; colornum < 4; ++colornum) {
		const SetDmgPaletteColorFun fun = { source, palnum, colornum, romPaletteDialog->getColor(palnum, colornum) };
		mw->callInWorkerThread(fun);
	}
}

namespace {
struct SetVideoSourceFun {
	GambatteSource *source; unsigned sourceIndex;
	void operator()() const { source->setVideoSource(sourceIndex); }
};
}

void GambatteMenuHandler::videoDialogChange() {
	{
		const SetVideoSourceFun fun = { source, videoDialog->sourceIndex() };
		mw->callInWorkerThread(fun);
	}

	applySettings(mw, videoDialog);

	if (mw->blitterConf(videoDialog->blitterNo()).maxSwapInterval()) {
		syncFrameRateAction->setEnabled(true);
	} else {
		if (syncFrameRateAction->isChecked())
			syncFrameRateAction->trigger();
		
		syncFrameRateAction->setEnabled(false);
	}
}

void GambatteMenuHandler::soundDialogChange() {
	SoundDialog::applySettings(mw, soundDialog);
}

void GambatteMenuHandler::miscDialogChange() {
	mw->setFastForwardSpeed(miscDialog->turboSpeed());
	mw->setPauseOnFocusOut(miscDialog->pauseOnFocusOut() ? 2 : 0);
	pauseInc = miscDialog->pauseOnDialogs() ? 4 : 0;
}

void GambatteMenuHandler::execGlobalPaletteDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	globalPaletteDialog->exec();
}

void GambatteMenuHandler::execRomPaletteDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	romPaletteDialog->exec();
}

void GambatteMenuHandler::execInputDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	source->inputDialog()->exec();
}

void GambatteMenuHandler::execSoundDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	soundDialog->exec();
}

void GambatteMenuHandler::execVideoDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	videoDialog->exec();
}

void GambatteMenuHandler::execMiscDialog() {
	TmpPauser tmpPauser(mw, pauseInc);
	miscDialog->exec();
}

void GambatteMenuHandler::prevStateSlot() {
	stateSlotGroup->actions().at(source->currentState() < 2 ? source->currentState() + 8 : source->currentState() - 2)->trigger();
}

void GambatteMenuHandler::nextStateSlot() {
	stateSlotGroup->actions().at(source->currentState())->trigger();
}

namespace {
struct SelectStateFun {
	GambatteSource *source; int i;
	void operator()() const { source->selectState(i); }
};
}

void GambatteMenuHandler::selectStateSlot() {
	if (QAction *action = stateSlotGroup->checkedAction()) {
		const SelectStateFun fun = { source, action->data().toInt() };
		mw->callInWorkerThread(fun);
	}
}

namespace {
struct SaveStateFun {
	GambatteSource *source;
	MainWindow::FrameBuffer fb;
	void operator()() const {
		source->saveState(MainWindow::FrameBuffer::Locked(fb).get());
	}
};
}

void GambatteMenuHandler::saveState() {
	const SaveStateFun fun = { source, MainWindow::FrameBuffer(mw) };
	mw->callInWorkerThread(fun);
}

namespace {
struct LoadStateFun {
	GambatteSource *source;
	void operator()() const { source->loadState(); }
};
}

void GambatteMenuHandler::loadState() {
	const LoadStateFun fun = { source };
	mw->callInWorkerThread(fun);
}

namespace {
struct SaveStateAsFun {
	GambatteSource *source;
	MainWindow::FrameBuffer fb;
	QString fileName;
	void operator()() const {
		source->saveState(MainWindow::FrameBuffer::Locked(fb).get(), fileName.toLocal8Bit().constData());
	}
};
}

void GambatteMenuHandler::saveStateAs() {
	TmpPauser tmpPauser(mw, pauseInc);
	const QString &fileName = QFileDialog::getSaveFileName(mw, tr("Save State"),
					QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));

	if (!fileName.isEmpty()) {
		const SaveStateAsFun fun = { source, MainWindow::FrameBuffer(mw), fileName };
		mw->callInWorkerThread(fun);
	}
}

namespace {
struct LoadStateFromFun {
	GambatteSource *source;
	QString fileName;
	void operator()() const {
		source->loadState(fileName.toLocal8Bit().constData());
	}
};
}

void GambatteMenuHandler::loadStateFrom() {
	TmpPauser tmpPauser(mw, pauseInc);
	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Load State"),
					QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));

	if (!fileName.isEmpty()) {
		const LoadStateFromFun fun = { source, fileName };
		mw->callInWorkerThread(fun);
	}
}

void GambatteMenuHandler::pauseChange() {
	if (pauseAction->isChecked())
		mw->pause();
	else
		mw->unpause();
}

void GambatteMenuHandler::frameStep() {
	if (pauseAction->isChecked())
		mw->frameStep();
	else
		pauseAction->trigger();
}

void GambatteMenuHandler::escPressed() {
#ifdef Q_WS_MAC
	if (fsAct->isChecked())
		fsAct->trigger();
#else
	mw->menuBar()->setVisible(!mw->menuBar()->isVisible());
	
	if (!mw->menuBar()->isVisible())
		mw->hideCursor();
#endif
}

void GambatteMenuHandler::videoBlitterFailure() {
	TmpPauser tmpPauser(mw, pauseInc);
	QMessageBox::critical(mw, tr("Video engine failure"),
			tr("Failed to update video output. This may be fixed by changing the video engine settings."));
	videoDialog->exec();
}

void GambatteMenuHandler::audioEngineFailure() {
	TmpPauser tmpPauser(mw, pauseInc);
	QMessageBox::critical(mw, tr("Sound engine failure"),
			tr("Failed to output audio. This may be fixed by changing the sound settings."));
	soundDialog->exec();
}

void GambatteMenuHandler::toggleFullScreen() {
	saveWindowSizeIfNotFullScreen();
	mw->toggleFullScreen();
}

void GambatteMenuHandler::saveWindowSizeIfNotFullScreen() {
	if (!mw->isFullScreen()) {
		QSettings settings;
		settings.setValue("mainwindow/size", mw->size());
	}
}
