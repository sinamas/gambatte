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

InputDialog::InputDialog(const std::vector<Button> &buttonInfos,
                         const bool deleteButtonActions,
                         QWidget *parent) :
QDialog(parent),
buttonInfos(buttonInfos),
inputBoxPairs(buttonInfos.size(), 0),
eventData(buttonInfos.size() * 2),
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
				gLayout->addWidget(inputBoxPairs[i]->mainBox, i, 1);
				gLayout->addWidget(inputBoxPairs[i]->altBox, i, 2);
				
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
			eventData[i * 2].id = settings.value(buttonInfos[i].category + buttonInfos[i].label + "Key1", buttonInfos[i].defaultKey).toUInt();
			eventData[i * 2].value = settings.value(buttonInfos[i].category + buttonInfos[i].label + "Value1",
			                                        buttonInfos[i].defaultKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE).toInt();
			
			eventData[i * 2 + 1].id = settings.value(buttonInfos[i].category + buttonInfos[i].label + "Key2", buttonInfos[i].defaultAltKey).toUInt();
			eventData[i * 2 + 1].value = settings.value(buttonInfos[i].category + buttonInfos[i].label + "Value2",
			                                            buttonInfos[i].defaultAltKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE).toInt();
		} else {
			eventData[i * 2].id = buttonInfos[i].defaultKey;
			eventData[i * 2].value = buttonInfos[i].defaultKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE;
			
			eventData[i * 2 + 1].id = buttonInfos[i].defaultAltKey;
			eventData[i * 2 + 1].value = buttonInfos[i].defaultAltKey == Qt::Key_unknown ? InputBox::NULL_VALUE : InputBox::KBD_VALUE;
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
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Key1", eventData[i * 2].id);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Value1", eventData[i * 2].value);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Key2", eventData[i * 2 + 1].id);
			settings.setValue(buttonInfos[i].category + buttonInfos[i].label + "Value2", eventData[i * 2 + 1].value);
		}
	}
	
	settings.endGroup();
	
	for (std::size_t i = 0; i < inputBoxPairs.size(); ++i)
		delete inputBoxPairs[i];
	
	if (deleteButtonActions)
		for (std::size_t i = 0; i < buttonInfos.size(); ++i)
			delete buttonInfos[i].action;
}

void InputDialog::resetMapping() {
	keymut.lock();
	joymut.lock();
	keyInputs.clear();
	joyInputs.clear();
	
	for (std::size_t i = 0; i < eventData.size(); ++i) {
		const SDL_Event &data = eventData[i];
		Button::Action *const action = buttonInfos[i >> 1].action;
	
		if (data.value != InputBox::NULL_VALUE) {
			if (data.value == InputBox::KBD_VALUE) {
				keyInputs.insert(std::pair<unsigned,Button::Action*>(data.id, action));
			} else {
				joyInputs.insert(std::pair<unsigned,JoyObserver>(data.id, JoyObserver(action, data.value)));
			}
		}
	}
	
	joymut.unlock();
	keymut.unlock();
}

void InputDialog::store() {
	for (std::size_t i = 0; i < inputBoxPairs.size(); ++i) {
		if (inputBoxPairs[i]) {
			eventData[i * 2] = inputBoxPairs[i]->mainBox->getData();
			eventData[i * 2 + 1] = inputBoxPairs[i]->altBox->getData();
		}
	}
	
	resetMapping();
}

void InputDialog::restore() {
	for (std::size_t i = 0; i < inputBoxPairs.size(); ++i) {
		if (inputBoxPairs[i]) {
			inputBoxPairs[i]->mainBox->setData(eventData[i * 2]);
			inputBoxPairs[i]->altBox->setData(eventData[i * 2 + 1]);
		}
	}
}

void InputDialog::keyPressEvent(const QKeyEvent *const e) {
	if (keymut.tryLock()) {
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
	
		while (range.first != range.second) {
			(range.first->second)->buttonPressed();
			++range.first;
		}
		
		keymut.unlock();
	}
}

void InputDialog::keyReleaseEvent(const QKeyEvent *const e) {
	if (keymut.tryLock()) {
		std::pair<keymap_t::iterator,keymap_t::iterator> range = keyInputs.equal_range(e->key());
	
		while (range.first != range.second) {
			(range.first->second)->buttonReleased();
			++range.first;
		}
		
		keymut.unlock();
	}
}

void InputDialog::joystickEvent(const SDL_Event &ev) {
	if (joymut.tryLock()) {
		std::pair<joymap_t::iterator,joymap_t::iterator> range = joyInputs.equal_range(ev.id);
	
		while (range.first != range.second) {
			(range.first->second).valueChanged(ev.value);
			++range.first;
		}
		
		joymut.unlock();
	}
}

void InputDialog::accept() {
	store();
	QDialog::accept();
}

void InputDialog::reject() {
	restore();
	QDialog::reject();
}
