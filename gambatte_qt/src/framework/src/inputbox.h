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
#ifndef INPUTBOX_H
#define INPUTBOX_H

#include "SDL_event.h"
#include <QLineEdit>

class InputBox : public QLineEdit {
	Q_OBJECT

	QWidget *nextFocus;
	SDL_Event data;
	int timerId;
	int ignoreCnt;
	
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
	enum { NULL_VALUE = 0, KBD_VALUE = 0x7FFFFFFF };
	explicit InputBox(QWidget *nextFocus = 0);
	void setData(const SDL_Event &data) { setData(data.id, data.value); }
	void setData(unsigned id, int value = KBD_VALUE);
	void setNextFocus(QWidget *const nextFocus) { this->nextFocus = nextFocus; }
	const SDL_Event& getData() const { return data; }

public slots:
	void clearData() { setData(0, NULL_VALUE); }
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
