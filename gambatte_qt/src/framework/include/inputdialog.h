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

#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include "auto_vector.h"
#include "mutual.h"
#include "SDL_event.h"
#include <QDialog>
#include <map>
#include <vector>

class InputBox;
class QSpinBox;

/**
  * A utility class that can optionally be used to provide a GUI for
  * mapping keyboard/joystick input to actions.
  *
  * Pass descriptions for "Buttons" to be configured to the ctor, and call
  * input event methods to invoke Button::Actions configured to activate on
  * the respective input event.
  */
class InputDialog : public QDialog, private Uncopyable {
public:
	class Button {
	public:
		virtual ~Button() {}

		// empty QString hides the button
		virtual QString const label() const = 0;
		virtual QString const category() const = 0;

		// Qt::Key. 0 for none
		virtual int defaultKey() const = 0;
		virtual int defaultAltKey() const = 0;

		// rapid fire default frames per press. 0 disables rapid fire.
		virtual unsigned char defaultFpp() const = 0;

		virtual void pressed() = 0;
		virtual void released() = 0;
	};

	explicit InputDialog(auto_vector<Button> &buttons, QWidget *parent = 0);
	virtual ~InputDialog();

	// These invoke Button::pressed/released for matching buttons
	void keyPress(QKeyEvent const *);
	void keyRelease(QKeyEvent const *);
	void joystickEvent(SDL_Event const &);

	// Call once every frame to tick rapid-fire pressing.
	void consumeAutoPress();

public slots:
	virtual void accept();
	virtual void reject();

private:
	struct MappedKey {
		Button *const button;
		unsigned char const fpp;

		MappedKey(Button *button, unsigned char fpp) 
		: button(button), fpp(fpp)
		{
		}
	};

	struct MappedJoy {
		Button *const button;
		int const mask;
		unsigned char const fpp;

		MappedJoy(Button *button, int mask, unsigned char fpp)
		: button(button), mask(mask), fpp(fpp)
		{
		}
	};

	struct AutoPress {
		Button *button;
		unsigned char fpp;
		unsigned char fcnt;

		AutoPress(Button *button, unsigned char fpp, unsigned char fcnt)
		: button(button), fpp(fpp), fcnt(fcnt)
		{
		}
	};

	struct Config {
		SDL_Event event;
		unsigned char fpp;
	};

	template<class Map>
	struct Mapping {
		Map map;
		std::vector<AutoPress> rapidvec;
	};

	typedef std::multimap<unsigned, MappedKey> Kmap;
	typedef std::multimap<unsigned, MappedJoy> Jmap;
	typedef Mapping<Kmap> KeyMapping;
	typedef Mapping<Jmap> JoyMapping;

	auto_vector<Button> const buttons_;
	std::vector<InputBox *> inputBoxes_;
	std::vector<QSpinBox *> fppBoxes_;
	std::vector<Config> config_;
	Mutual<KeyMapping> keyMapping_;
	Mutual<JoyMapping> joyMapping_;

	void resetMapping();
	void store();
	void restore();
};

#endif
