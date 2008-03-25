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
#include "palettedialog.h"
#include "mainwindow.h"
#include "gambattesource.h"

static const QString strippedName(const QString &fullFileName) {
	return QFileInfo(fullFileName).fileName();
}

GambatteMenuHandler::GambatteMenuHandler(MainWindow *const mw, GambatteSource *const source, const int argc, const char *const argv[]) : mw(mw), source(source) {
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
		QAction *openAct = new QAction(tr("&Open..."), mw);
		openAct->setShortcut(tr("Ctrl+O"));
		openAct->setStatusTip(tr("Open an existing file"));
		connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
		
		for (int i = 0; i < MaxRecentFiles; ++i) {
			recentFileActs[i] = new QAction(mw);
			recentFileActs[i]->setVisible(false);
			connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
		}
		
		resetAct = new QAction(tr("&Reset"), mw);
		resetAct->setShortcut(tr("Ctrl+R"));
		resetAct->setEnabled(false);
		connect(resetAct, SIGNAL(triggered()), recentFileActs[0], SLOT(trigger()));
		
		QMenu *fileMenu = mw->menuBar()->addMenu(tr("&File"));
		fileMenu->addAction(openAct);
		separatorAct = fileMenu->addSeparator();
		
		for (int i = 0; i < MaxRecentFiles; ++i)
			fileMenu->addAction(recentFileActs[i]);
		
		fileMenu->addSeparator();
		fileMenu->addAction(resetAct);
		fileMenu->addSeparator();
		
		QAction *exitAct = new QAction(tr("E&xit"), mw);
		exitAct->setShortcut(tr("Ctrl+Q"));
		exitAct->setStatusTip(tr("Exit the application"));
		connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
		
		fileMenu->addAction(exitAct);
		updateRecentFileActions();
	}
	
	QMenu *settingsm = mw->menuBar()->addMenu(tr("&Settings"));
	
	{
		QAction *const act = new QAction(tr("&Input..."), mw);
		connect(act, SIGNAL(triggered()), mw, SLOT(execInputDialog()));
		settingsm->addAction(act);
	}
	
	{
		QAction *const act = new QAction(tr("&Sound..."), mw);
		connect(act, SIGNAL(triggered()), mw, SLOT(execSoundDialog()));
		settingsm->addAction(act);
	}
	
	{
		QAction *const act = new QAction(tr("&Video..."), mw);
		connect(act, SIGNAL(triggered()), mw, SLOT(execVideoDialog()));
		settingsm->addAction(act);
	}
	
	settingsm->addSeparator();
	
	{
		QMenu *const palm = settingsm->addMenu(tr("DMG &Palette"));
		
		{
			QAction *act = new QAction(tr("&Global..."), mw);
			connect(act, SIGNAL(triggered()), this, SLOT(execGlobalPaletteDialog()));
			palm->addAction(act);
		}
		
		romPaletteAct = new QAction(tr("Current &ROM..."), mw);
		romPaletteAct->setEnabled(false);
		connect(romPaletteAct, SIGNAL(triggered()), this, SLOT(execRomPaletteDialog()));
		palm->addAction(romPaletteAct);
	}
	
	settingsm->addSeparator();
	
	{
		QAction *fsAct = new QAction(tr("&Full Screen"), mw);
		fsAct->setShortcut(tr("Ctrl+F"));
		fsAct->setCheckable(true);
		connect(fsAct, SIGNAL(triggered()), mw, SLOT(toggleFullScreen()));
		settingsm->addAction(fsAct);
	}
	
// 	settingsm->addAction(hideMenuAct);
	
	mw->menuBar()->addSeparator();
	
	QMenu *helpMenu = mw->menuBar()->addMenu(tr("&Help"));
	QAction *aboutAct = new QAction(tr("&About"), mw);
	connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
	helpMenu->addAction(aboutAct);
	
	mw->addActions(mw->menuBar()->actions());
	
	QAction *hideMenuAct = new QAction(mw);
	hideMenuAct->setShortcut(tr("Esc"));
	connect(hideMenuAct, SIGNAL(triggered()), mw, SLOT(toggleMenuHidden()));
	mw->addAction(hideMenuAct);
	
	mw->setFrameTime(4389, 262144);
	connect(source, SIGNAL(blit()), mw, SLOT(blit()));
	connect(source, SIGNAL(setTurbo(bool)), mw, SLOT(setTurbo(bool)));
	
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			loadFile(QString(argv[i]));
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
	
	separatorAct->setVisible(numRecentFiles > 0);
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
	resetAct->setEnabled(true);
	
	mw->run();
}

void GambatteMenuHandler::open() {
	mw->pause();
	
	QString fileName = QFileDialog::getOpenFileName(mw, "Open", recentFileActs[0]->data().toString(), "Game Boy ROM images (*.dmg *.gb *.gbc *.sgb *.zip);;All files (*)");
	
	if (!fileName.isEmpty())
		loadFile(fileName);
	
	mw->unpause();
	mw->setFocus();
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
	
	mw->unpause();
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
