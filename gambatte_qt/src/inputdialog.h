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
#include <vector>
#include "SDL_Joystick/include/SDL_event.h"
#include "mediasource.h"

enum { AXIS_CENTERED = 0, AXIS_POSITIVE = 1, AXIS_NEGATIVE = 2 };

// wraps SDL_PollEvent, converting all values to hat-style bitset values (a single bit for buttons, two for axes, four for hats)
// only hats can have multiple bits set at once. In practice only axis values are converted (to AXIS_CENTERED, AXIS_POSITIVE or AXIS_NEGATIVE).
int pollJsEvent(SDL_Event *ev);

class InputBox;

class InputBoxPair : public QObject {
	Q_OBJECT

public:
	InputBox *const mainBox;
	InputBox *const altBox;
	
	InputBoxPair(InputBox *mainBox, InputBox *altBox) : mainBox(mainBox), altBox(altBox) {}
	
public slots:
	void clear();
};

class InputDialog : public QDialog {
	Q_OBJECT
	
	const std::vector<MediaSource::ButtonInfo> buttonInfos;
	std::vector<InputBoxPair*> inputBoxPairs;
	std::vector<SDL_Event> eventData;
	
	void store();
	void restore();
	
public:
	enum { NULL_VALUE = 0, KBD_VALUE = 0x7FFFFFFF };
	
	InputDialog(const std::vector<MediaSource::ButtonInfo> &buttonInfos,
	            QWidget *parent = 0);
	~InputDialog();
	
	const std::vector<SDL_Event>& getData() const { return eventData; }
	
public slots:
	void accept();
	void reject();
};

#endif
