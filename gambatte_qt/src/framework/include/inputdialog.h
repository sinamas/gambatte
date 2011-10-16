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

#include "auto_vector.h"
#include "mutual.h"
#include "SDL_event.h"
#include <QDialog>
#include <QSpinBox>
#include <vector>
#include <map>

class InputBoxPair;

/** A utility class that can optionally be used to provide a GUI for
  * mapping keyboard/joystick input to actions.
  *
  * Pass descriptions for "Buttons" to be configured
  * to the constructor, and call input event methods to invoke Button::Actions
  * configured to activate on the respective input event.
  */
class InputDialog : public QDialog, private Uncopyable {
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
		
		// Default number of frames per auto-repeat press. 0 for auto-repeat disabled.
		unsigned char defaultFpp;
	};
	
private:
	struct KeyMapped {
		Button::Action *const action;
		const unsigned char fpp;
		KeyMapped(Button::Action *action, int fpp) : action(action), fpp(fpp) {}
	};
	
	struct JoyMapped {
		Button::Action *const action;
		const int mask;
		const unsigned char fpp;
		JoyMapped(Button::Action *action, int mask, int fpp) : action(action), mask(mask), fpp(fpp) {}
	};
	
	struct AutoPress {
		Button::Action *action;
		unsigned char fpp;
		unsigned char fcnt;
		AutoPress(Button::Action *action, unsigned char fpp, unsigned char fcnt) : action(action), fpp(fpp), fcnt(fcnt) {}
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
	
	typedef std::multimap<unsigned,KeyMapped> keymap_t;
	typedef std::multimap<unsigned,JoyMapped> joymap_t;
	typedef Mapping<keymap_t> KeyMapping;
	typedef Mapping<joymap_t> JoyMapping;
	
	const std::vector<Button> buttonInfos;
	auto_vector<InputBoxPair> inputBoxPairs;
	auto_vector<QSpinBox> fppBoxes;
	std::vector<Config> config;
	Mutual<KeyMapping> keymapping;
	Mutual<JoyMapping> joymapping;
	const bool deleteButtonActions;
	
	void resetMapping();
	void store();
	void restore();
	
public:
	explicit InputDialog(const std::vector<Button> &buttonInfos,
	            bool deleteButtonActions = true,
	            QWidget *parent = 0);
	~InputDialog();
	
	// These invoke Button::Actions matching the key pressed/released
	void keyPressEvent(const QKeyEvent *);
	void keyReleaseEvent(const QKeyEvent *);
	void joystickEvent(const SDL_Event&);
	
	// Call once every frame to tick auto-repeat pressing.
	void consumeAutoPress();
	
public slots:
	void accept();
	void reject();
};

#endif
