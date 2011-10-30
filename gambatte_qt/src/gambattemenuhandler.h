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

#include "scalingmethod.h"
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
class CheatDialog;

class FrameRateAdjuster : public QObject {
	Q_OBJECT
	
	class FrameTime {
	public:
		struct Rational {
			unsigned num;
			unsigned denom;
			Rational(unsigned num = 0, unsigned denom = 1) : num(num), denom(denom) {}
		};
		
	private:
		enum { STEPS = 16 };
		
		Rational frameTimes[STEPS * 2 + 1];
		unsigned index;
		
	public:
		FrameTime(unsigned baseNum, unsigned baseDenom);
		void setBaseFrameTime(unsigned baseNum, unsigned baseDenom);
		
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
		const Rational& get() const { return frameTimes[index]; }
		const Rational& base() const { return frameTimes[STEPS]; }
	} frameTime_;
	
	const MiscDialog &miscDialog_;
	MainWindow &mw_;
	QAction *const decFrameRateAction_;
	QAction *const incFrameRateAction_;
	QAction *const resetFrameRateAction_;
	bool enabled_;
	
	void changed();
	
private slots:
	void miscDialogChange();
	
public:
	FrameRateAdjuster(const MiscDialog &miscDialog, MainWindow &mw, QObject *parent = 0);
	const QList<QAction*> actions();
	
public slots:
	void setDisabled(bool disabled);
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
};

class WindowSizeMenu : public QObject {
	Q_OBJECT

	MainWindow *const mw_;
	QMenu *const menu_;
	QActionGroup *group_;
	const QSize maxSize_;

	void fillMenu(const QSize &sourceSize, ScalingMethod scalingMethod);
	void setCheckedSize(const QSize &size);
	const QSize checkedSize() const;

private slots:
	void triggered(QAction*);

public:
	WindowSizeMenu(MainWindow *mw, const VideoDialog *videoDialog);
	~WindowSizeMenu();
	QMenu* menu() const { return menu_; }
	void videoDialogChange(const VideoDialog *vd);
};

class GambatteMenuHandler : public QObject {
	Q_OBJECT
		
	enum { MaxRecentFiles = 9 };
	
	MainWindow *const mw;
	GambatteSource *const source;
	SoundDialog *const soundDialog;
	VideoDialog *const videoDialog;
	MiscDialog *const miscDialog;
	CheatDialog *const cheatDialog;
	QAction *recentFileActs[MaxRecentFiles];
	QAction *pauseAction;
	QAction *syncFrameRateAction;
	QAction *gbaCgbAction;
	QAction *forceDmgAction;
#ifdef Q_WS_MAC
	QAction *fsAct;
#endif
	QMenu *recentMenu;
	PaletteDialog *globalPaletteDialog;
	PaletteDialog *romPaletteDialog;
	QActionGroup *const stateSlotGroup;
	WindowSizeMenu windowSizeMenu;
	unsigned pauseInc;
	
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void setDmgPaletteColors();
	void updateRecentFileActions();
	
signals:
	void romLoaded(bool);
	void dmgRomLoaded(bool);
	
private slots:
	void open();
	void openRecentFile();
	void about();
	void globalPaletteChange();
	void romPaletteChange();
	void videoDialogChange();
	void soundDialogChange();
	void miscDialogChange();
	void cheatDialogChange();
	void reconsiderSyncFrameRateActionEnable();
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
	void reset();
	void pauseChange();
	void frameStep();
	void escPressed();
	void videoBlitterFailure();
	void audioEngineFailure();
	void toggleFullScreen();
	void saveWindowSizeIfNotFullScreen();
	
public:
	GambatteMenuHandler(MainWindow *mw, GambatteSource *source, int argc, const char *const argv[]);
	~GambatteMenuHandler();
};

#endif
