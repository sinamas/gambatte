//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef GAMBATTEMENUHANDLER_H
#define GAMBATTEMENUHANDLER_H

#include "scalingmethod.h"
#include <QList>
#include <QObject>
#include <QSize>

class CheatDialog;
class GambatteSource;
class MainWindow;
class MiscDialog;
class PaletteDialog;
class QAction;
class QActionGroup;
class QMenu;
class QString;
class SoundDialog;
class VideoDialog;

class FrameRateAdjuster : public QObject {
public:
	FrameRateAdjuster(MiscDialog const &miscDialog, MainWindow &mw, QObject *parent = 0);
	QList<QAction *> const actions();

public slots:
	void setDisabled(bool disabled);
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();

private:
	Q_OBJECT

	class FrameTime {
	public:
		struct Rational {
			unsigned num;
			unsigned denom;
			Rational(unsigned num = 0, unsigned denom = 1) : num(num), denom(denom) {}
		};

		FrameTime(unsigned baseNum, unsigned baseDenom);
		void setBaseFrameTime(unsigned baseNum, unsigned baseDenom);
		bool incPossible() const { return index_ < num_steps * 2; }
		bool decPossible() const { return index_; }
		bool resetPossible() const { return index_ != num_steps; }

		void inc() {
			if (index_ < num_steps * 2)
				++index_;
		}

		void dec() {
			if (index_)
				--index_;
		}

		void reset() { index_ = num_steps; }
		Rational const & get() const { return frameTimes_[index_]; }
		Rational const & base() const { return frameTimes_[num_steps]; }

	private:
		enum { num_steps = 16 };
		Rational frameTimes_[num_steps * 2 + 1];
		std::size_t index_;
	} frameTime_;
	MiscDialog const &miscDialog_;
	MainWindow &mw_;
	QAction *const decFrameRateAction_;
	QAction *const incFrameRateAction_;
	QAction *const resetFrameRateAction_;
	bool enabled_;

	void changed();

private slots:
	void miscDialogChange();
};

class WindowSizeMenu : private QObject {
public:
	WindowSizeMenu(MainWindow &mw, VideoDialog const &videoDialog);
	~WindowSizeMenu();
	QMenu * menu() const { return menu_; }
	void videoDialogChange(VideoDialog const &vd);

private:
	Q_OBJECT

	MainWindow &mw_;
	QMenu *const menu_;
	QActionGroup *group_;
	QSize const maxSize_;

	void fillMenu(QSize const &sourceSize, ScalingMethod scalingMethod);
	void setCheckedSize(QSize const &size);
	QSize const checkedSize() const;

private slots:
	void triggered();
};

class GambatteMenuHandler : private QObject {
public:
	GambatteMenuHandler(MainWindow &mw, GambatteSource &source,
	                    int argc, char const *const argv[]);
	~GambatteMenuHandler();

private:
	Q_OBJECT

	enum { max_recent_files = 9 };

	MainWindow &mw_;
	GambatteSource &source_;
	SoundDialog *const soundDialog_;
	VideoDialog *const videoDialog_;
	MiscDialog *const miscDialog_;
	CheatDialog *const cheatDialog_;
	QAction *recentFileActs_[max_recent_files];
	QAction *pauseAction_;
	QAction *syncFrameRateAction_;
	QAction *gbaCgbAction_;
	QAction *forceDmgAction_;
	QAction *fsAct_;
	QMenu *recentMenu_;
	PaletteDialog *globalPaletteDialog_;
	PaletteDialog *romPaletteDialog_;
	QActionGroup *const stateSlotGroup_;
	WindowSizeMenu windowSizeMenu_;
	int pauseInc_;

	void loadFile(QString const &fileName);
	void setCurrentFile(QString const &fileName);
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
};

#endif
