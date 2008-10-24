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
	
	class FrameTime {
	public:
		struct Rational {
			unsigned num;
			unsigned denom;
			Rational(unsigned num = 0, unsigned denom = 0) : num(num), denom(denom) {}
		};
		
	private:
		enum { STEPS = 16 };
		
		Rational frameTimes[STEPS * 2 + 1];
		unsigned index;
		
	public:
		FrameTime(unsigned baseNum, unsigned baseDenom);
		
		bool inc() {
			if (index < STEPS * 2)
				++index;
			
			return index < STEPS * 2;
		}
		
		bool dec() {
			if (index)
				--index;
			
			return index;
		}
		
		void reset() { index = STEPS; }
		
		const Rational& get() const {
			return frameTimes[index];
		}
	};
	
	MainWindow *const mw;
	GambatteSource *const source;
	QAction *recentFileActs[MaxRecentFiles];
	QAction *romPaletteAct;
	QAction *pauseAction;
	QAction *decFrameRateAction;
	QAction *incFrameRateAction;
	QAction *forceDmgAction;
	QMenu *recentMenu;
	QMenu *stateSlotMenu;
	PaletteDialog *globalPaletteDialog;
	PaletteDialog *romPaletteDialog;
	QActionGroup *stateSlotGroup;
	QList<QAction*> romLoadedActions;
	FrameTime frameTime;
	
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
	void pauseChange();
	void frameStep();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	
public:
	GambatteMenuHandler(MainWindow *mw, GambatteSource *source, int argc, const char *const argv[]);
};

#endif
