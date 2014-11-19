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

#ifndef INPUTBOX_H
#define INPUTBOX_H

#include "joysticklock.h"
#include "scoped_ptr.h"
#include <QLineEdit>

class InputBox : public QLineEdit {
public:
	enum { value_null = 0, value_kbd = 0x7fffffff };
	explicit InputBox(QWidget *nextFocus = 0);
	void setData(SDL_Event const &data) { setData(data.id, data.value); }
	void setData(unsigned id, int value = value_kbd);
	void setNextFocus(QWidget *nextFocus) { nextFocus_ = nextFocus; }
	SDL_Event const & data() const { return data_; }

public slots:
	void clear();

signals:
	void redundantClear();

protected:
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void focusInEvent(QFocusEvent *event);
	virtual void focusOutEvent(QFocusEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void timerEvent(QTimerEvent *event);

private:
	Q_OBJECT

	QWidget *nextFocus_;
	SDL_Event data_;
	scoped_ptr<SdlJoystick::Locked> js_;
	int timerId_;
	int ignoreCnt_;

private slots:
	void textEditedSlot() { setData(data_); }
};

#endif
