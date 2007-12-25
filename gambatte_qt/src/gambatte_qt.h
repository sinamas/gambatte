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


#ifndef GAMBATTE_QT_H
#define GAMBATTE_QT_H

#include <QList>
#include <QMainWindow>
#include <QSize>
#include <vector>
#include <map>
#include <memory>
#include <gambatte.h>
#include "videobufferreseter.h"
#include "samplescalculator.h"
#include "SDL_Joystick/include/SDL_joystick.h"
#include "SDL_Joystick/include/SDL_event.h"

class QAction;
class QMenu;
// class QTextEdit;
class AudioEngine;
class BlitterWidget;
class VideoDialog;
class InputDialog;
class SoundDialog;
class PaletteDialog;
class FullResToggler;
class BlitterContainer;
class JoyObserver;

struct InputObserver {
	virtual ~InputObserver() {}
	virtual void valueChanged(bool value) = 0;
};

class GbDirHandler : public InputObserver {
	bool &gbButton;
	bool &negGbButton;
public:
	GbDirHandler(bool &gbButton, bool &negGbButton) : gbButton(gbButton), negGbButton(negGbButton) {}
	void valueChanged(const bool value) { if ((gbButton = value)) negGbButton = false; }
};

class GbButHandler : public InputObserver {
	bool &gbButton;
public:
	GbButHandler(bool &gbButton) : gbButton(gbButton) {}
	void valueChanged(const bool value) { gbButton = value; }
};

struct InputGetter : public Gambatte::InputStateGetter {
	Gambatte::InputState is;
	const Gambatte::InputState& operator()() { return is; }
};

class JoystickIniter {
	std::vector<SDL_Joystick*> joysticks;
public:
	JoystickIniter();
	~JoystickIniter();
};

class GambatteQt : public QMainWindow {
	Q_OBJECT
		
	typedef std::multimap<unsigned,InputObserver*> keymap_t;
	typedef std::multimap<unsigned,JoyObserver*> joymap_t;
	
	Gambatte::GB gambatte;

	std::vector<AudioEngine*> audioEngines;
	std::vector<BlitterWidget*> blitters;
	keymap_t keyInputs;
	joymap_t joyInputs;
	
	InputGetter inputGetter;
	GbDirHandler gbUpHandler;
	GbDirHandler gbDownHandler;
	GbDirHandler gbLeftHandler;
	GbDirHandler gbRightHandler;
	GbButHandler gbAHandler;
	GbButHandler gbBHandler;
	GbButHandler gbStartHandler;
	GbButHandler gbSelectHandler;
	VideoBufferReseter resetVideoBuffer;
	
	qint16 *sndBuffer;

	BlitterContainer *blitterContainer;
	QAction *exitAct;
	QAction *separatorAct;
	QAction *fsAct;
	QAction *hideMenuAct;
	QAction *resetAct;
	QAction *romPaletteAct;
	InputDialog *inputDialog;
	SoundDialog *soundDialog;
	VideoDialog *videoDialog;
	PaletteDialog *globalPaletteDialog;
	PaletteDialog *romPaletteDialog;
	BlitterWidget *blitter;
	const std::auto_ptr<FullResToggler> fullResToggler;

	enum { MaxRecentFiles = 8 };
	QAction *recentFileActs[MaxRecentFiles];
	
	AudioEngine *ae;
	
	int sampleRate;
	unsigned samplesPrFrame;
	
	int timerId;
	
	SamplesCalculator samplesCalc;
	JoystickIniter joyIniter;

	bool running;
	bool turbo;
	
	
	void createActions();
	void createMenus();
	void loadFile(const QString &fileName);
	void initAudio();
	void run();
	void stop();
	void pause();
	void unpause();
	void setSamplesPrFrame();
	void setCurrentFile(const QString &fileName);
	void setDmgPaletteColors();
	void soundEngineFailure();
	void updateRecentFileActions();
	QString strippedName(const QString &fullFileName);
	void execDialog(QDialog *dialog);
	void clearInputVectors();
	void pushInputObserver(const SDL_Event &data, InputObserver &observer);
	void updateJoysticks();
	void resetWindowSize(const QSize &s);
	
private slots:
	void open();
	void openRecentFile();
	void about();
	void toggleFullScreen();
	void toggleMenuHidden();
	void inputSettingsChange();
	void soundSettingsChange();
	void videoSettingsChange();
	void globalPaletteChange();
	void romPaletteChange();
	void execVideoDialog();
	void execInputDialog();
	void execSoundDialog();
	void execGlobalPaletteDialog();
	void execRomPaletteDialog();
	
protected:
	void timerEvent(QTimerEvent *event);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
	void closeEvent(QCloseEvent *e);
	void showEvent(QShowEvent *e);

public:
	GambatteQt(int argc, const char *const argv[]);
	~GambatteQt();
};

#endif
