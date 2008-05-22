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
#include "palettedialog.h"
#include "mainwindow.h"
#include "gambattesource.h"

static const QString strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
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
			const int argc, const char *const argv[]) : mw(mw), source(source), frameTime(4389, 262144) {
	mw->setWindowTitle("Gambatte");
	
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
		source->setSavedir(savepath.toAscii().data());
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
		
		romLoadedActions.append(fileMenu->addAction(tr("&Save State"), source, SLOT(saveState()), QString("F5")));
		romLoadedActions.append(fileMenu->addAction(tr("&Load State"), source, SLOT(loadState()), QString("F8")));
		
		{
			stateSlotMenu = fileMenu->addMenu(tr("S&elect State Slot"));
			stateSlotMenu->setEnabled(false);
			stateSlotMenu->addAction(tr("&Previous"), this, SLOT(prevStateSlot()), QString("F6"));
			stateSlotMenu->addAction(tr("&Next"), this, SLOT(nextStateSlot()), QString("F7"));
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
		
		romLoadedActions.append(pauseAction = playm->addAction(tr("&Pause"), this, SLOT(pauseChange()), QString("Pause")));
		pauseAction->setCheckable(true);
		romLoadedActions.append(playm->addAction(tr("Frame &Step"), this, SLOT(frameStep()), QString("F1")));
		playm->addSeparator();
		romLoadedActions.append(decFrameRateAction = playm->addAction(tr("&Decrease Frame Rate"), this, SLOT(decFrameRate()), QString("F2")));
		romLoadedActions.append(incFrameRateAction = playm->addAction(tr("&Increase Frame Rate"), this, SLOT(incFrameRate()), QString("F3")));
		romLoadedActions.append(playm->addAction(tr("&Reset Frame Rate"), this, SLOT(resetFrameRate()), QString("F4")));
	}
	
	QMenu *settingsm = mw->menuBar()->addMenu(tr("&Settings"));
	
	settingsm->addAction(tr("&Input..."), mw, SLOT(execInputDialog()));
	settingsm->addAction(tr("&Sound..."), mw, SLOT(execSoundDialog()));
	settingsm->addAction(tr("&Video..."), mw, SLOT(execVideoDialog()));
	
	settingsm->addSeparator();
	
	{
		QMenu *const palm = settingsm->addMenu(tr("DMG &Palette"));
		
		palm->addAction(tr("&Global..."), this, SLOT(execGlobalPaletteDialog()));
		
		romPaletteAct = palm->addAction(tr("Current &ROM..."), this, SLOT(execRomPaletteDialog()));
		romPaletteAct->setEnabled(false);
	}
	
	settingsm->addSeparator();
	
	{
		QAction *fsAct = settingsm->addAction(tr("&Full Screen"), mw, SLOT(toggleFullScreen()), tr("Ctrl+F"));
		fsAct->setCheckable(true);
	}
	
	foreach(QAction *a, romLoadedActions) {
		a->setEnabled(false);
	}
	
// 	settingsm->addAction(hideMenuAct);
	
	mw->menuBar()->addSeparator();
	
	QMenu *helpMenu = mw->menuBar()->addMenu(tr("&Help"));
	helpMenu->addAction(tr("&About"), this, SLOT(about()));
	
	mw->addActions(mw->menuBar()->actions());
	
	QAction *hideMenuAct = new QAction(mw);
	hideMenuAct->setShortcut(tr("Esc"));
	connect(hideMenuAct, SIGNAL(triggered()), mw, SLOT(toggleMenuHidden()));
	mw->addAction(hideMenuAct);
	
	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
	connect(source, SIGNAL(blit()), mw, SLOT(blit()));
	connect(source, SIGNAL(setTurbo(bool)), mw, SLOT(setTurbo(bool)));
	
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
	resetFrameRate();
	pauseAction->setChecked(false);
	pauseChange();
	
	if (source->load((fileName.toAscii()).data())) {
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
	
	foreach(QAction *a, romLoadedActions) {
		a->setEnabled(true);
	}
	
	stateSlotMenu->setEnabled(true);
	stateSlotGroup->actions().at(0)->setChecked(true);
	
	mw->run();
}

void GambatteMenuHandler::open() {
	mw->pause();
	
	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Open"), recentFileActs[0]->data().toString(),
			tr("Game Boy ROM Images (*.dmg *.gb *.gbc *.sgb *.zip);;All Files (*)"));
	
	if (!fileName.isEmpty())
		loadFile(fileName);
	
	mw->setFocus(); // giving back focus after getOpenFileName seems to fail at times, which can be problematic with current exclusive mode handling.
	pauseChange();
}

void GambatteMenuHandler::openRecentFile() {
	QAction *action = qobject_cast<QAction *>(sender());
	
	if (action)
		loadFile(action->data().toString());
}

void GambatteMenuHandler::about() {
	mw->pause();
	
	QMessageBox::about(
	                    mw,
	                    tr("About Gambatte"),
	                    tr("<h3>Gambatte Qt svn</h3>\
	                       <p><b>Author:</b> Sindre Aamås (<a href=\"mailto:aamas@stud.ntnu.no\">aamas@stud.ntnu.no</a>).<br>\
	                       <b>Homepage:</b> <a href=\"http://sourceforge.net/projects/gambatte\">http://sourceforge.net/projects/gambatte</a>.</p>\
	                       <p>Gambatte is an accuracy-focused, open-source, cross-platform Game Boy / Game Boy Color emulator written in C++. It is based on hundreds of corner case hardware tests, as well as previous documentation and reverse engineering efforts.</p>")
	                  );
	
	pauseChange();
}

void GambatteMenuHandler::globalPaletteChange() {
	romPaletteDialog->externalChange();
	setDmgPaletteColors();
}

void GambatteMenuHandler::romPaletteChange() {
	globalPaletteDialog->externalChange();
	setDmgPaletteColors();
}

void GambatteMenuHandler::setDmgPaletteColors() {
	for (unsigned palnum = 0; palnum < 3; ++palnum)
		for (unsigned colornum = 0; colornum < 4; ++colornum)
			source->setDmgPaletteColor(palnum, colornum, romPaletteDialog->getColor(palnum, colornum));
}

void GambatteMenuHandler::execGlobalPaletteDialog() {
	mw->execDialog(globalPaletteDialog);
}

void GambatteMenuHandler::execRomPaletteDialog() {
	mw->execDialog(romPaletteDialog);
}

void GambatteMenuHandler::prevStateSlot() {
	stateSlotGroup->actions().at(source->currentState() < 2 ? source->currentState() + 8 : source->currentState() - 2)->trigger();
}

void GambatteMenuHandler::nextStateSlot() {
	stateSlotGroup->actions().at(source->currentState())->trigger();
}

void GambatteMenuHandler::selectStateSlot() {
	if (QAction *action = stateSlotGroup->checkedAction())
		source->selectState(action->data().toInt());
}

void GambatteMenuHandler::saveStateAs() {
	mw->pause();
	
	const QString &fileName = QFileDialog::getSaveFileName(mw, tr("Save State"), QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));
	
	if (!fileName.isEmpty()) {
		source->saveState(fileName.toAscii().data());
	}
	
	pauseChange();
}

void GambatteMenuHandler::loadStateFrom() {
	mw->pause();
	
	const QString &fileName = QFileDialog::getOpenFileName(mw, tr("Load State"), QString(), tr("Gambatte Quick Save Files (*.gqs);;All Files (*)"));
	
	if (!fileName.isEmpty()) {
		source->loadState(fileName.toAscii().data());
	}
	
	pauseAction->setChecked(false);
	pauseChange();
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

void GambatteMenuHandler::decFrameRate() {
	if (!frameTime.inc())
		decFrameRateAction->setEnabled(false);
	
	incFrameRateAction->setEnabled(true);
		
	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
}

void GambatteMenuHandler::incFrameRate() {
	if (!frameTime.dec())
		incFrameRateAction->setEnabled(false);
	
	decFrameRateAction->setEnabled(true);
	
	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
}

void GambatteMenuHandler::resetFrameRate() {
	frameTime.reset();
	incFrameRateAction->setEnabled(true);
	decFrameRateAction->setEnabled(true);
	mw->setFrameTime(frameTime.get().num, frameTime.get().denom);
}
