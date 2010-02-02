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
#include <QMutex>
#include <vector>
#include <map>
#include "SDL_event.h"

class InputBoxPair;

/** A dialog that lets the user map keyboard/joystick input
  * to actions. Pass descriptions for "Buttons" to be configured
  * to the constructor, and call input event methods to invoke Button::Actions
  * configured to activate on the respective input event.
  */
class InputDialog : public QDialog {
	Q_OBJECT
public:
	struct Button {
		// Label used in input settings dialog. If this is empty the button won't be configurable, but will use the defaultKey.
		QString label;
		
		// Tab label used in input settings dialog.
		QString category;
		
		// Default Qt::Key. Use Qt::Key_unknown for none.
		int defaultKey;
		
		// Default alternate Qt::Key. Use Qt::Key_unknown for none.
		int defaultAltKey;
		
		// called on button press / release
		struct Action {
			virtual void buttonPressed() {}
			virtual void buttonReleased() {}
			virtual ~Action() {}
		} *action;
	};
	
private:
	class JoyObserver {
		Button::Action *const action;
		const int mask;
	
		void notifyObserver(bool press) {
			if (press)
				action->buttonPressed();
			else
				action->buttonReleased();
		}
	
	public:
		JoyObserver(Button::Action *const action, const int mask) : action(action), mask(mask) {}
		void valueChanged(const int value) { notifyObserver((value & mask) == mask); }
	};
	
	typedef std::multimap<unsigned,Button::Action*> keymap_t;
	typedef std::multimap<unsigned,JoyObserver> joymap_t;
	
	const std::vector<Button> buttonInfos;
	std::vector<InputBoxPair*> inputBoxPairs;
	std::vector<SDL_Event> eventData;
	keymap_t keyInputs;
	joymap_t joyInputs;
	QMutex keymut;
	QMutex joymut;
	const bool deleteButtonActions;
	
	void resetMapping();
	void store();
	void restore();
	
public:
	InputDialog(const std::vector<Button> &buttonInfos,
	            bool deleteButtonActions = true,
	            QWidget *parent = 0);
	~InputDialog();
	
	// These invoke Button::Actions matching the key pressed/released
	void keyPressEvent(const QKeyEvent *);
	void keyReleaseEvent(const QKeyEvent *);
	void joystickEvent(const SDL_Event&);
	
public slots:
	void accept();
	void reject();
};

// used in implementation of InputDialog
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
	enum { NULL_VALUE = 0, KBD_VALUE = 0x7FFFFFFF };
	InputBox(QWidget *nextFocus = 0);
	void setData(const SDL_Event &data) { setData(data.id, data.value); }
	void setData(unsigned id, int value = KBD_VALUE);
	void setNextFocus(QWidget *const nextFocus) { this->nextFocus = nextFocus; }
	const SDL_Event& getData() const { return data; }

public slots:
	void clearData() { setData(0, NULL_VALUE); }
};

// used in implementation of InputDialog
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
