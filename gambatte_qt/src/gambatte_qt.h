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


#ifndef GAMBATTE_QT_H
#define GAMBATTE_QT_H

#include <QList>
#include <QMainWindow>
#include <QSize>
#include <vector>
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
class FullResToggler;
class BlitterContainer;
class GbKeyHandler;
class GbJoyHandler;

class InputGetter : public InputStateGetter {
public:
	InputState is;
	const InputState& operator()() { return is; }
};

class JoystickIniter {
	std::vector<SDL_Joystick*> joysticks;
public:
	JoystickIniter();
	~JoystickIniter();
};

class GambatteQt : public QMainWindow {
	Q_OBJECT
	
	Gambatte gambatte;

	std::vector<AudioEngine*> audioEngines;
	std::vector<BlitterWidget*> blitters;
	std::vector<GbKeyHandler*> keyInputs;
	std::vector<GbJoyHandler*> joyInputs;
	
	InputGetter inputGetter;
	VideoBufferReseter resetVideoBuffer;
	
	int16_t *sndBuffer;

	BlitterContainer *blitterContainer;
	QAction *exitAct;
	QAction *separatorAct;
	QAction *fsAct;
	QAction *hideMenuAct;
	InputDialog *inputDialog;
	VideoDialog* videoDialog;
	BlitterWidget *blitter;
	FullResToggler *fullResToggler;

	enum { MaxRecentFiles = 8 };
	QAction *recentFileActs[MaxRecentFiles];
	
	AudioEngine *ae;
	
	unsigned sampleRate;
	unsigned samplesPrFrame;
	
	int timerId;
	
	SamplesCalculator samplesCalc;
	JoystickIniter joyIniter;

	bool running;
	bool turbo;
	
	
	void createActions();
	void createMenus();
	void loadFile(const QString &fileName);
	AudioEngine* initAudio();
	void run();
	void stop();
	void pause();
	void unpause();
	void setSamplesPrFrame();
	void setCurrentFile(const QString &fileName);
	void updateRecentFileActions();
	QString strippedName(const QString &fullFileName);
	void execDialog(QDialog *dialog);
	void clearInputVectors();
	void pushGbInputHandler(const SDL_Event &data, bool &gbButton, bool *gbNegButton = NULL);
	void updateJoysticks();
	
private slots:
	void open();
	void openRecentFile();
	void about();
	void resetWindowSize();
	void toggleFullScreen();
	void toggleMenuHidden();
	void inputSettingsChange();
	void videoSettingsChange();
	void execVideoDialog();
	void execInputDialog();
	
protected:
	void timerEvent(QTimerEvent *event);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
	void closeEvent(QCloseEvent *e);

public:
	GambatteQt(int argc, const char *const argv[]);
	~GambatteQt();
};

#endif
