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
#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <vector>
#include "SDL_Joystick/include/SDL_event.h"
#include "mediasource.h"

enum { AXIS_CENTERED = 0, AXIS_POSITIVE = 1, AXIS_NEGATIVE = 2 };

// wraps SDL_PollEvent, converting all values to hat-style bitset values (a single bit for buttons, two for axes, four for hats)
// only hats can have multiple bits set at once. In practice only axis values are converted (to AXIS_CENTERED, AXIS_POSITIVE or AXIS_NEGATIVE).
int pollJsEvent(SDL_Event *ev);

class InputBoxPair;

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

class InputBox : public QLineEdit {
	Q_OBJECT

	QWidget *nextFocus;
	SDL_Event data;
	int timerId;
	
private slots:
	void textEditedSlot(const QString &) {
		setData(data);
	}
	
protected:
	void contextMenuEvent(QContextMenuEvent *event);
	void focusInEvent(QFocusEvent *event);
	void focusOutEvent(QFocusEvent *event);
	void keyPressEvent(QKeyEvent *e);
	void timerEvent(QTimerEvent */*event*/);
	
public:
	InputBox(QWidget *nextFocus = 0);
	void setData(const SDL_Event &data) { setData(data.id, data.value); }
	void setData(unsigned id, int value = InputDialog::KBD_VALUE);
	void setNextFocus(QWidget *const nextFocus) { this->nextFocus = nextFocus; }
	const SDL_Event& getData() const { return data; }

public slots:
	void clearData() { setData(0, InputDialog::NULL_VALUE); }
};

class InputBoxPair : public QObject {
	Q_OBJECT

public:
	InputBox *const mainBox;
	InputBox *const altBox;
	
	InputBoxPair(InputBox *mainBox, InputBox *altBox) : mainBox(mainBox), altBox(altBox) {}
	
public slots:
	void clear();
};

#endif
