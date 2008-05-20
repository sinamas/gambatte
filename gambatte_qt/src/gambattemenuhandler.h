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
#ifndef GAMBATTEMENUHANDLER_H
#define GAMBATTEMENUHANDLER_H

#include <QObject>
#include <QList>

class MainWindow;
class GambatteSource;
class QAction;
class PaletteDialog;
class QString;
class QActionGroup;
class QMenu;

class GambatteMenuHandler : public QObject {
	Q_OBJECT
		
	enum { MaxRecentFiles = 9 };
	
	MainWindow *const mw;
	GambatteSource *const source;
	QAction *recentFileActs[MaxRecentFiles];
	QMenu *recentMenu;
	QAction *romPaletteAct;
	PaletteDialog *globalPaletteDialog;
	PaletteDialog *romPaletteDialog;
	QActionGroup *stateSlotGroup;
	QMenu *stateSlotMenu;
	QList<QAction*> romLoadedActions;
	
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void setDmgPaletteColors();
	void updateRecentFileActions();
	
private slots:
	void open();
	void openRecentFile();
	void about();
	void globalPaletteChange();
	void romPaletteChange();
	void execGlobalPaletteDialog();
	void execRomPaletteDialog();
	void prevStateSlot();
	void nextStateSlot();
	void selectStateSlot();
	void saveStateAs();
	void loadStateFrom();
	
public:
	GambatteMenuHandler(MainWindow *mw, GambatteSource *source, int argc, const char *const argv[]);
};

#endif
