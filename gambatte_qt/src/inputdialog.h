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
#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include "SDL_Joystick/include/SDL_event.h"

static const int KBD_VALUE = 0;
static const int JSBUTTON_VALUE = 1;
static const int JSPAXIS_VALUE = 8192;
static const int JSNAXIS_VALUE = -JSPAXIS_VALUE;

class InputBox;

class InputDialog : public QDialog {
	Q_OBJECT
	
	InputBox *upBox;
	InputBox *downBox;
	InputBox *leftBox;
	InputBox *rightBox;
	InputBox *aBox;
	InputBox *bBox;
	InputBox *startBox;
	InputBox *selectBox;
	
	SDL_Event upData;
	SDL_Event downData;
	SDL_Event leftData;
	SDL_Event rightData;
	SDL_Event aData;
	SDL_Event bData;
	SDL_Event startData;
	SDL_Event selectData;
	
	void store();
	void restore();
	
public:
	InputDialog(QWidget *parent = 0);
	~InputDialog();
	
	const SDL_Event& getUpData() const { return upData; }
	const SDL_Event& getDownData() const { return downData; }
	const SDL_Event& getLeftData() const { return leftData; }
	const SDL_Event& getRightData() const { return rightData; }
	const SDL_Event& getAData() const { return aData; }
	const SDL_Event& getBData() const { return bData; }
	const SDL_Event& getStartData() const { return startData; }
	const SDL_Event& getSelectData() const { return selectData; }
	
public slots:
	void accept();
	void reject();
};

#endif
