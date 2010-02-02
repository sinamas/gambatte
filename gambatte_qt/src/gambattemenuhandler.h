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
#include <QSize>

class MainWindow;
class GambatteSource;
class QAction;
class PaletteDialog;
class QString;
class QActionGroup;
class QMenu;
class SoundDialog;
class VideoDialog;
class MiscDialog;

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
		
		bool incPossible() const { return index < STEPS * 2; }
		bool decPossible() const { return index; }
		bool resetPossible() const { return index != STEPS; }
		
		void inc() {
			if (index < STEPS * 2)
				++index;
		}
		
		void dec() {
			if (index)
				--index;
		}
		
		void reset() { index = STEPS; }
		
		const Rational& get() const {
			return frameTimes[index];
		}
	};
	
	MainWindow *const mw;
	GambatteSource *const source;
	SoundDialog *const soundDialog;
	VideoDialog *const videoDialog;
	MiscDialog *const miscDialog;
	QAction *recentFileActs[MaxRecentFiles];
	QAction *romPaletteAct;
	QAction *pauseAction;
	QAction *syncFrameRateAction;
	QAction *decFrameRateAction;
	QAction *incFrameRateAction;
	QAction *resetFrameRateAction;
	QAction *forceDmgAction;
#ifdef Q_WS_MAC
	QAction *fsAct;
#endif
	QMenu *recentMenu;
	QMenu *stateSlotMenu;
	PaletteDialog *globalPaletteDialog;
	PaletteDialog *romPaletteDialog;
	QActionGroup *stateSlotGroup;
	QList<QAction*> romLoadedActions;
	FrameTime frameTime;
	QSize wsz;
	unsigned pauseInc;
	
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void setDmgPaletteColors();
	void updateRecentFileActions();
	void frameRateChange();
	
private slots:
	void open();
	void openRecentFile();
	void about();
	void globalPaletteChange();
	void romPaletteChange();
	void videoDialogChange();
	void soundDialogChange();
	void miscDialogChange();
	void execGlobalPaletteDialog();
	void execRomPaletteDialog();
	void execInputDialog();
	void execSoundDialog();
	void execVideoDialog();
	void execMiscDialog();
	void prevStateSlot();
	void nextStateSlot();
	void selectStateSlot();
	void saveState();
	void saveStateAs();
	void loadState();
	void loadStateFrom();
	void pauseChange();
	void frameStep();
	void syncFrameRate();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	void escPressed();
	void videoBlitterFailure();
	void audioEngineFailure();
	void toggleFullScreen();
	void saveWindowSize();
	
public:
	GambatteMenuHandler(MainWindow *mw, GambatteSource *source, int argc, const char *const argv[]);
};

#endif
