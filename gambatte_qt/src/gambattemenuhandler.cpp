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
#include "gambattemenuhandler.h"

#include <QtGui>
#include <QActionGroup>
#include <QFileInfo>
#include <QSettings>
#include "palettedialog.h"
#include "framework/mainwindow.h"
#include "framework/sounddialog.h"
#include "framework/videodialog.h"
#include "gambattesource.h"
#include "miscdialog.h"

static const QString strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

namespace {
struct TmpPauser {
	MainWindow *const mw;
	const unsigned inc;
	
	TmpPauser(MainWindow *const mw, const unsigned inc = 4) : mw(mw), inc(inc) {
		mw->incPause(inc);
	}
	
	~TmpPauser() {
		mw->decPause(inc);
	}
};
}

GambatteMenuHandler::FrameTime::FrameTime(unsigned baseNum, unsigned baseDenom) : index(STEPS) {
	frameTimes[index] = Rational(baseNum, baseDenom);

	for (unsigned i = index; i < STEPS * 2; ++i) {
		frameTimes[i + 1] = Rational(frameTimes[i].num * 11 * 0x10000 / (frameTimes[i].denom * 10), 0x10000);
	}

	for (unsigned i = index; i; --i) {
		frameTimes[i - 1] = Rational(frameTimes[i].num * 10 * 0x10000 / (frameTimes[i].denom * 11), 0x10000);
	}
}

GambatteMenuHandler::GambatteMenuHandler(MainWindow *const mw, GambatteSource *const source,
			const int argc, const char *const argv[]) :
mw(mw),
source(source),
soundDialog(new SoundDialog(mw, mw)),
videoDialog(new VideoDialog(mw, source->generateVideoSourceInfos(), QString("Video filter:"), QSize(160, 144), mw)),
miscDialog(new MiscDialog(mw)),
frameTime(4389, 262144),
pauseInc(4)
{
	mw->setWindowTitle("Gambatte");
	source->inputDialog()->setParent(mw, source->inputDialog()->windowFlags());

	{
		QSettings iniSettings(QSettings::IniFormat, QSettings::UserScope, "gambatte", "gambatte_qt");
		QString savepath = iniSettings.fileName();
		savepath.truncate(iniSettings.fileName().lastIndexOf("/") + 1);

		{
			QString palpath = savepath + "palettes";
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

	{
		for (int i = 0; i < MaxRecentFiles; ++i) {
			recentFileActs[i] = new QAction(mw);
			recentFileActs[i]->setVisible(false);
			connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		}

		QMenu *fileMenu = mw->menuBar()->addMenu(tr("&File"));
		fileMenu->addAction(tr("&Open..."), this, SLOT(open()), tr("Ctrl+O"));

		{
			recentMenu = fileMenu->addMenu(tr("Open Re&cent"));

			for (int i = 0; i < MaxRecentFiles; ++i)
				recentMenu->addAction(recentFileActs[i]);
		}

		fileMenu->addSeparator();
		romLoadedActions.append(fileMenu->addAction(tr("&Reset"), recentFileActs[0], SLOT(trigger()), tr("Ctrl+R")));
		fileMenu->addSeparator();

		romLoadedActions.append(fileMenu->addAction(tr("Save State &As..."), this, SLOT(saveStateAs())));
		romLoadedActions.append(fileMenu->addAction(tr("Load State &From..."), this, SLOT(loadStateFrom())));
		fileMenu->addSeparator();

		romLoadedActions.append(fileMenu->addAction(tr("&Save State"), this, SLOT(saveState()), QString("Ctrl+S")));
		romLoadedActions.append(fileMenu->addAction(tr("&Load State"), this, SLOT(loadState()), QString("Ctrl+L")));

		{
			stateSlotMenu = fileMenu->addMenu(tr("S&elect State Slot"));
			stateSlotMenu->setEnabled(false);
			stateSlotMenu->addAction(tr("&Previous"), this, SLOT(prevStateSlot()), QString("Ctrl+Z"));
			stateSlotMenu->addAction(tr("&Next"), this, SLOT(nextStateSlot()), QString("Ctrl+X"));
			stateSlotMenu->addSeparator();

			stateSlotGroup = new QActionGroup(mw);

			for (int i = 0; i < 10; ++i) {
				const int no = i == 9 ? 0 : i + 1;
				const QString &strno = QString::number(no);
				QAction *action = stateSlotMenu->addAction("Slot &" + strno, this, SLOT(selectStateSlot()), strno);

				action->setCheckable(true);
				action->setData(no);
				stateSlotGroup->addAction(action);
			}
		}

		fileMenu->addSeparator();

		fileMenu->addAction(tr("E&xit"), qApp, SLOT(closeAllWindows()), tr("Ctrl+Q"));
		updateRecentFileActions();
	}

	{
		QMenu *const playm = mw->menuBar()->addMenu(tr("&Play"));

		romLoadedActions.append(pauseAction = playm->addAction(tr("&Pause"), this, SLOT(pauseChange()), QString("Ctrl+P")));
		pauseAction->setCheckable(true);
		romLoadedActions.append(playm->addAction(tr("Frame &Step"), this, SLOT(frameStep()), QString("Ctrl+.")));
		playm->addSeparator();
		/*romLoadedActions.append(*/syncFrameRateAction = playm->addAction(tr("&Sync Frame Rate to Refresh Rate"), this, SLOT(syncFrameRate()))/*)*/;
		syncFrameRateAction->setCheckable(true);
		romLoadedActions.append(decFrameRateAction = playm->addAction(tr("&Decrease Frame Rate"), this, SLOT(decFrameRate()), QString("Ctrl+D")));
		romLoadedActions.append(incFrameRateAction = playm->addAction(tr("&Increase Frame Rate"), this, SLOT(incFrameRate()), QString("Ctrl+I")));
		romLoadedActions.append(resetFrameRateAction = playm->addAction(tr("&Reset Frame Rate"), this, SLOT(resetFrameRate()), QString("Ctrl+U")));
	}

	QMenu *settingsm = mw->menuBar()->addMenu(tr("&Settings"));

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

		romPaletteAct = palm->addAction(tr("Current &ROM..."), this, SLOT(execRomPaletteDialog()));
		romPaletteAct->setEnabled(false);
	}

	settingsm->addSeparator();

	{
#ifndef Q_WS_MAC
		QAction *fsAct;
#endif
		fsAct = settingsm->addAction(tr("&Full Screen"), this, SLOT(toggleFullScreen()), tr("Ctrl+F"));
		fsAct->setCheckable(true);
	}

	foreach (QAction *a, romLoadedActions) {
		a->setEnabled(false);
	}

// 	settingsm->addAction(hideMenuAct);

	mw->menuBar()->addSeparator();

	QMenu *helpMenu = mw->menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(tr("&About"), this, SLOT(about()));

	mw->addActions(mw->menuBar()->actions());

	{
		QAction *const escAct = new QAction(mw);
		escAct->setShortcut(tr("Esc"));
		connect(escAct, SIGNAL(triggered()), this, SLOT(escPressed()));
		mw->addAction(escAct);
	}

	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
	mw->setSamplesPerFrame(35112);
	connect(source, SIGNAL(setTurbo(bool)), mw, SLOT(setFastForward(bool)));
	connect(source, SIGNAL(togglePause()), pauseAction, SLOT(trigger()));
	connect(source, SIGNAL(frameStep()), this, SLOT(frameStep()));
	connect(source, SIGNAL(decFrameRate()), this, SLOT(decFrameRate()));
	connect(source, SIGNAL(incFrameRate()), this, SLOT(incFrameRate()));
	connect(source, SIGNAL(resetFrameRate()), this, SLOT(resetFrameRate()));
	connect(source, SIGNAL(prevStateSlot()), this, SLOT(prevStateSlot()));
	connect(source, SIGNAL(nextStateSlot()), this, SLOT(nextStateSlot()));
	connect(source, SIGNAL(saveStateSignal()), this, SLOT(saveState()));
	connect(source, SIGNAL(loadStateSignal()), this, SLOT(loadState()));
	connect(videoDialog, SIGNAL(accepted()), this, SLOT(videoDialogChange()));
	connect(soundDialog, SIGNAL(accepted()), this, SLOT(soundDialogChange()));
	connect(miscDialog, SIGNAL(accepted()), this, SLOT(miscDialogChange()));
	connect(mw, SIGNAL(videoBlitterFailure()), this, SLOT(videoBlitterFailure()));
	connect(mw, SIGNAL(audioEngineFailure()), this, SLOT(audioEngineFailure()));
	connect(mw, SIGNAL(closing()), this, SLOT(saveWindowSize()));
	
	videoDialogChange();
	soundDialogChange();
	miscDialogChange();
	
	{
		QSettings settings;
		settings.beginGroup("mainwindow");
		mw->resize(settings.value("size", QSize(160, 144)).toSize());
		settings.endGroup();
	}

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

	int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}

	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);

	recentMenu->setEnabled(numRecentFiles > 0);
}

void GambatteMenuHandler::setCurrentFile(const QString &fileName) {
	QString curFile = fileName;

	if (curFile.isEmpty())
		mw->setWindowTitle(tr("Gambatte"));
	else
		mw->setWindowTitle(tr("%1 - %2").arg(strippedName(curFile)).arg(tr("Gambatte")));

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

	romPaletteAct->setEnabled(!source->isCgb());

	setCurrentFile(fileName);

	foreach (QAction *a, romLoadedActions) {
		a->setEnabled(true);
	}

	stateSlotMenu->setEnabled(true);
	stateSlotGroup->actions().at(0)->setChecked(true);
	resetFrameRate();

	mw->run();
}

void GambatteMenuHandler::open() {
	TmpPauser tmpPauser(mw, pauseInc);

	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Open"), recentFileActs[0]->data().toString(),
			tr("Game Boy ROM Images (*.dmg *.gb *.gbc *.sgb *.zip);;All Files (*)"));

	if (!fileName.isEmpty())
		loadFile(fileName);

	mw->setFocus(); // giving back focus after getOpenFileName seems to fail at times, which can be problematic with current exclusive mode handling.
}

void GambatteMenuHandler::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());

	if (action)
		loadFile(action->data().toString());
}

void GambatteMenuHandler::about() {
	TmpPauser tmpPauser(mw, pauseInc);

	QMessageBox::about(
	                    mw,
	                    tr("About Gambatte"),
	                    tr("<h3>Gambatte Qt SVN</h3>\
	                       <p><b>Author:</b> Sindre Aam�s (<a href=\"mailto:aamas@stud.ntnu.no\">aamas@stud.ntnu.no</a>).<br>\
	                       <b>Homepage:</b> <a href=\"http://sourceforge.net/projects/gambatte\">http://sourceforge.net/projects/gambatte</a>.</p>\
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
	void operator()() { source->setDmgPaletteColor(palnum, colornum, rgb32); }
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
	void operator()() { source->setVideoSource(sourceIndex); }
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
	void operator()() { source->selectState(i); }
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
	void operator()() {
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
	void operator()() { source->loadState(); }
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
	void operator()() {
		source->saveState(MainWindow::FrameBuffer::Locked(fb).get(), fileName.toLocal8Bit().constData());
	}
};
}

void GambatteMenuHandler::saveStateAs() {
	TmpPauser tmpPauser(mw, pauseInc);
	
	const QString &fileName = QFileDialog::getSaveFileName(mw, tr("Save State"), QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));

	if (!fileName.isEmpty()) {
		const SaveStateAsFun fun = { source, MainWindow::FrameBuffer(mw), fileName };
		mw->callInWorkerThread(fun);
	}
}

namespace {
struct LoadStateFromFun {
	GambatteSource *source;
	QString fileName;
	void operator()() {
		source->loadState(fileName.toLocal8Bit().constData());
	}
};
}

void GambatteMenuHandler::loadStateFrom() {
	TmpPauser tmpPauser(mw, pauseInc);
	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Load State"), QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));

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

void GambatteMenuHandler::frameRateChange() {
	incFrameRateAction->setEnabled(frameTime.decPossible());
	decFrameRateAction->setEnabled(frameTime.incPossible());
	resetFrameRateAction->setEnabled(frameTime.resetPossible());
	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
}

void GambatteMenuHandler::syncFrameRate() {
	const bool checked = syncFrameRateAction->isChecked();
	mw->setSyncToRefreshRate(checked);
	
	if (mw->isRunning()) {
		incFrameRateAction->setEnabled(!checked & frameTime.decPossible());
		decFrameRateAction->setEnabled(!checked & frameTime.incPossible());
		resetFrameRateAction->setEnabled(!checked & frameTime.resetPossible());
	}
}

void GambatteMenuHandler::decFrameRate() {
	frameTime.inc();
	frameRateChange();
}

void GambatteMenuHandler::incFrameRate() {
	frameTime.dec();
	frameRateChange();
}

void GambatteMenuHandler::resetFrameRate() {
	frameTime.reset();
	frameRateChange();
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
	QMessageBox::critical(mw, tr("Video engine failure"), tr("Failed to update video output. This may be fixed by changing the video engine settings."));
	videoDialog->exec();
}

void GambatteMenuHandler::audioEngineFailure() {
	TmpPauser tmpPauser(mw, pauseInc);
	QMessageBox::critical(mw, tr("Sound engine failure"), tr("Failed to output audio. This may be fixed by changing the sound settings."));
	soundDialog->exec();
}

void GambatteMenuHandler::toggleFullScreen() {
	if (!mw->isFullScreen())
		wsz = mw->size();
	
	mw->toggleFullScreen();
}

void GambatteMenuHandler::saveWindowSize() {
	QSettings settings;
	settings.beginGroup("mainwindow");
	settings.setValue("size", mw->isFullScreen() ? wsz : mw->size());
	settings.endGroup();
}
