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
#include "inputdialog.h"
#include "inputbox.h"
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <memory>

static std::auto_ptr<QSpinBox> makeFppBox(const int val) {
	std::auto_ptr<QSpinBox> box(new QSpinBox);
	box->setValue(val);
	box->setMinimum(1);
	box->setMaximum(9);
	box->setSuffix(" fpp");
	return box;
}

InputDialog::InputDialog(const std::vector<Button> &buttonInfos,
                         const bool deleteButtonActions,
                         QWidget *parent)
: QDialog(parent),
  buttonInfos(buttonInfos),
  inputBoxPairs(buttonInfos.size()),
  fppBoxes(buttonInfos.size() * 2),
  config(buttonInfos.size() * 2),
  deleteButtonActions(deleteButtonActions)
{
	setWindowTitle(tr("Input Settings"));
	
	QVBoxLayout *mainLayout = new QVBoxLayout;
	QPushButton *const okButton = new QPushButton(tr("OK"));
	QPushButton *const cancelButton = new QPushButton(tr("Cancel"));
	
	{
		QTabWidget *const tabw = new QTabWidget;
		
		for (std::size_t i = 0; i < buttonInfos.size(); ++i) {
			if (!buttonInfos[i].label.isEmpty()) {
				inputBoxPairs[i] = new InputBoxPair(new InputBox, new InputBox);
				
				int j = tabw->count() - 1;
				
				while (j >= 0 && tabw->tabText(j) != buttonInfos[i].category)
					--j;
				
				if (j < 0) {
					QWidget *const w = new QWidget;
					QBoxLayout *const boxl = new QVBoxLayout;
					boxl->addLayout(new QGridLayout);
					boxl->setAlignment(Qt::AlignTop);
					w->setLayout(boxl);
					j = tabw->addTab(w, buttonInfos[i].category);
				}
				
				QGridLayout *const gLayout = (QGridLayout*) tabw->widget(j)->layout()->itemAt(0);
				gLayout->addWidget(new QLabel(buttonInfos[i].label + ":"), i, 0);
				
				if (buttonInfos[i].defaultFpp) {
					QHBoxLayout *hLayout = new QHBoxLayout;
					hLayout->addWidget(inputBoxPairs[i]->mainBox);
					hLayout->addWidget(fppBoxes[i * 2] = makeFppBox(buttonInfos[i].defaultFpp).release());
					gLayout->addLayout(hLayout, i, 1);
					
					hLayout = new QHBoxLayout;
					hLayout->addWidget(inputBoxPairs[i]->altBox);
					hLayout->addWidget(fppBoxes[i * 2 + 1] = makeFppBox(buttonInfos[i].defaultFpp).release());
					gLayout->addLayout(hLayout, i, 2);
				} else {
					gLayout->addWidget(inputBoxPairs[i]->mainBox, i, 1);
					gLayout->addWidget(inputBoxPairs[i]->altBox, i, 2);
				}
				
				QPushButton *const clearButton = new QPushButton(tr("Clear"));
				gLayout->addWidget(clearButton, i, 3);
				connect(clearButton, SIGNAL(clicked()), inputBoxPairs[i], SLOT(clear()));
			}
		}
		
		for (int tabi = 0; tabi < tabw->count(); ++tabi) {
			QWidget *const w = tabw->widget(tabi);
			
			std::size_t i = 0;
			
			while (i < inputBoxPairs.size() && (!inputBoxPairs[i] || inputBoxPairs[i]->mainBox->parentWidget() != w))
				++i;
			
			while (i < inputBoxPairs.size()) {
				std::size_t j = i + 1;
				
				while (j < inputBoxPairs.size() && (!inputBoxPairs[j] || inputBoxPairs[j]->mainBox->parentWidget() != w))
					++j;
				
				if (j < inputBoxPairs.size()) {
					inputBoxPairs[i]->mainBox->setNextFocus(inputBoxPairs[j]->mainBox);
					inputBoxPairs[i]->altBox->setNextFocus(inputBoxPairs[j]->altBox);
				} else {
					inputBoxPairs[i]->mainBox->setNextFocus(okButton);
					inputBoxPairs[i]->altBox->setNextFocus(okButton);
				}
				
				i = j;
			}
		}
		
		mainLayout->addWidget(tabw);
	}
	
	QHBoxLayout *const hLayout = new QHBoxLayout;
	hLayout->addWidget(okButton);
	hLayout->addWidget(cancelButton);
	mainLayout->addLayout(hLayout);
	mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	okButton->setDefault(true);
	
	setLayout(mainLayout);
	
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	
	QSettings settings;
	settings.beginGroup("input");
	
	for (std::size_t i = 0; i < buttonInfos.size(); ++i) {
		if (!buttonInfos[i].label.isEmpty()) {
			config[i * 2    ].event.id = settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Key1", buttonInfos[i].defaultKey).toUInt();
			config[i * 2    ].event.value = settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Value1",
					buttonInfos[i].defaultKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE).toInt();
			config[i * 2    ].fpp = buttonInfos[i].defaultFpp ? settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Fpp1", buttonInfos[i].defaultFpp).toInt() : 0;
			
			config[i * 2 + 1].event.id = settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Key2", buttonInfos[i].defaultAltKey).toUInt();
			config[i * 2 + 1].event.value = settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Value2",
					buttonInfos[i].defaultAltKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE).toInt();
			config[i * 2 + 1].fpp = buttonInfos[i].defaultFpp ? settings.value(
					buttonInfos[i].category + buttonInfos[i].label + "Fpp2", buttonInfos[i].defaultFpp).toInt() : 0;
		} else {
			config[i * 2    ].event.id = buttonInfos[i].defaultKey;
			config[i * 2    ].event.value =
					buttonInfos[i].defaultKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE;
			config[i * 2    ].fpp = buttonInfos[i].defaultFpp;
			
			config[i * 2 + 1].event.id = buttonInfos[i].defaultAltKey;
			config[i * 2 + 1].event.value =
					buttonInfos[i].defaultAltKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE;
			config[i * 2 + 1].fpp = buttonInfos[i].defaultFpp;
		}
	}
	
	settings.endGroup();
	
	restore();
	resetMapping();
}

InputDialog::~InputDialog() {
	QSettings settings;
	settings.beginGroup("input");
	
	for (std::size_t i = 0; i < buttonInfos.size(); ++i) {
		if (!buttonInfos[i].label.isEmpty()) {
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Key1", config[i * 2].event.id);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Value1", config[i * 2].event.value);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Key2", config[i * 2 + 1].event.id);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Value2", config[i * 2 + 1].event.value);
			
			if (buttonInfos[i].defaultFpp) {
				settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Fpp1", config[i * 2    ].fpp);
				settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Fpp2", config[i * 2 + 1].fpp);
			}
		}
	}
	
	settings.endGroup();
	
	if (deleteButtonActions)
		for (std::size_t i = 0; i < buttonInfos.size(); ++i)
			delete buttonInfos[i].action;
}

void InputDialog::resetMapping() {
	{
		Mutual<KeyMapping>::Locked lkm(keymapping);
		lkm->map.clear();
		lkm->rapidvec.clear();
		
		for (std::size_t i = 0; i < config.size(); ++i) {
			if (config[i].event.value != InputBox::NULL_VALUE && config[i].event.value == InputBox::KBD_VALUE) {
				lkm->map.insert(std::make_pair(config[i].event.id,
						KeyMapped(buttonInfos[i >> 1].action, config[i].fpp)));
			}
		}
	}
	
	{
		Mutual<JoyMapping>::Locked ljm(joymapping);
		ljm->map.clear();
		ljm->rapidvec.clear();
		
		for (std::size_t i = 0; i < config.size(); ++i) {
			if (config[i].event.value != InputBox::NULL_VALUE && config[i].event.value != InputBox::KBD_VALUE) {
				ljm->map.insert(std::make_pair(config[i].event.id,
						JoyMapped(buttonInfos[i >> 1].action, config[i].event.value, config[i].fpp)));
			}
		}
	}
}

void InputDialog::store() {
	for (std::size_t i = 0; i < inputBoxPairs.size(); ++i) {
		if (inputBoxPairs[i]) {
			config[i * 2    ].event = inputBoxPairs[i]->mainBox->getData();
			config[i * 2 + 1].event = inputBoxPairs[i]->altBox->getData();
		}
	}
	
	for (std::size_t i = 0; i < fppBoxes.size(); ++i) {
		if (fppBoxes[i])
			config[i].fpp = fppBoxes[i]->value();
	}
	
	resetMapping();
}

void InputDialog::restore() {
	for (std::size_t i = 0; i < inputBoxPairs.size(); ++i) {
		if (inputBoxPairs[i]) {
			inputBoxPairs[i]->mainBox->setData(config[i * 2    ].event);
			inputBoxPairs[i]->altBox->setData( config[i * 2 + 1].event);
		}
	}
	
	for (std::size_t i = 0; i < fppBoxes.size(); ++i) {
		if (fppBoxes[i])
			fppBoxes[i]->setValue(config[i].fpp);
	}
}

template<class AutoPressVec>
static void setButtonPressed(AutoPressVec &v, InputDialog::Button::Action *const action, const int fpp) {
	if (fpp) {
		const typename AutoPressVec::iterator end = v.end();
		for (typename AutoPressVec::iterator it = v.begin(); it != end; ++it) {
			if (it->action == action && it->fpp == fpp)
				return;
		}
		
		v.push_back(typename AutoPressVec::value_type(action, fpp, 0));
	}
	
	action->buttonPressed();
}

template<class AutoPressVec>
static void unsetButtonPressed(AutoPressVec &v, InputDialog::Button::Action *const action, const int fpp) {
	if (fpp) {
		const typename AutoPressVec::iterator end = v.end();
		for (typename AutoPressVec::iterator it = v.begin(); it != end; ++it) {
			if (it->action == action && it->fpp == fpp) {
				v.erase(it);
				break;
			}
		}
	}
	
	action->buttonReleased();
}

void InputDialog::keyPressEvent(const QKeyEvent *const e) {
	if (const Mutual<KeyMapping>::TryLocked &lm = keymapping) {
		for (std::pair<keymap_t::iterator,keymap_t::iterator> range
				= lm->map.equal_range(e->key()); range.first != range.second; ++range.first) {
			  setButtonPressed(lm->rapidvec, range.first->second.action, range.first->second.fpp);
		}
	}
}

void InputDialog::keyReleaseEvent(const QKeyEvent *const e) {
	if (const Mutual<KeyMapping>::TryLocked &lm = keymapping) {
		for (std::pair<keymap_t::iterator,keymap_t::iterator> range
				= lm->map.equal_range(e->key()); range.first != range.second; ++range.first) {
			unsetButtonPressed(lm->rapidvec, range.first->second.action, range.first->second.fpp);
		}
	}
}

void InputDialog::joystickEvent(const SDL_Event &ev) {
	if (const Mutual<JoyMapping>::TryLocked &lm = joymapping) {
		for (std::pair<joymap_t::iterator,joymap_t::iterator> range
				= lm->map.equal_range(ev.id); range.first != range.second; ++range.first) {
			if ((ev.value & range.first->second.mask) == range.first->second.mask) {
				  setButtonPressed(lm->rapidvec, range.first->second.action, range.first->second.fpp);
			} else
				unsetButtonPressed(lm->rapidvec, range.first->second.action, range.first->second.fpp);
		}
	}
}

template<class AutoPressVec>
static void doConsumeAutoPress(AutoPressVec &v) {
	const typename AutoPressVec::iterator end = v.end();
	for (typename AutoPressVec::iterator it = v.begin(); it != end; ++it) {
		if (it->fcnt == 0) {
			it->fcnt = it->fpp;
			it->action->buttonReleased();
		} else if (--it->fcnt == 0)
			it->action->buttonPressed();
	}
}

void InputDialog::consumeAutoPress() {
	if (const Mutual<KeyMapping>::TryLocked &lm = keymapping)
		doConsumeAutoPress(lm->rapidvec);
	
	if (const Mutual<JoyMapping>::TryLocked &lm = joymapping)
		doConsumeAutoPress(lm->rapidvec);
}

void InputDialog::accept() {
	store();
	QDialog::accept();
}

void InputDialog::reject() {
	restore();
	QDialog::reject();
}
