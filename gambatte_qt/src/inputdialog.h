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
#include <string>
#include <vector>
#include "SDL_Joystick/include/SDL_event.h"

static const int KBD_VALUE = 0;
static const int JSBUTTON_VALUE = 1;
static const int JSPAXIS_VALUE = 8192;
static const int JSNAXIS_VALUE = -JSPAXIS_VALUE;

class InputBox;

class InputDialog : public QDialog {
	Q_OBJECT
	
	const std::vector<std::string> buttonLabels;
	std::vector<InputBox*> inputBoxes;
	std::vector<SDL_Event> eventData;
	
	void store();
	void restore();
	
public:
	InputDialog(const std::vector<std::string> &buttonLabels,
	            const std::vector<int> &buttonDefaults,
	            QWidget *parent = 0);
	~InputDialog();
	
	const std::vector<SDL_Event>& getData() const { return eventData; }
	
public slots:
	void accept();
	void reject();
};

#endif
