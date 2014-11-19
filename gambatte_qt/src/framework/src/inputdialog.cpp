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

#include "inputdialog.h"
#include "dialoghelpers.h"
#include "inputbox.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

template<class W>
static W * addWidget(QGridLayout *l, W *w, int row, int col) {
	l->addWidget(w, row, col);
	return w;
}

template<class L>
static L * addLayout(QGridLayout *parent, L *l, int row, int col) {
	parent->addLayout(l, row, col);
	return l;
}

static QSpinBox * addFppBox(QLayout *parent, int fpp) {
	QSpinBox *const box = addWidget(parent, new QSpinBox);
	box->setValue(fpp);
	box->setMinimum(1);
	box->setMaximum(9);
	box->setSuffix(" fpp");
	return box;
}

InputDialog::InputDialog(auto_vector<Button> &buttons, QWidget *parent)
: QDialog(parent)
, buttons_(buttons)
, inputBoxes_(buttons_.size() * 2)
, fppBoxes_(  buttons_.size() * 2)
, config_(    buttons_.size() * 2)
{
	setWindowTitle(tr("Input Settings"));

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QTabWidget *const tabw = addWidget(mainLayout, new QTabWidget);
	QHBoxLayout *const okCancelLayout = addLayout(mainLayout, new QHBoxLayout,
	                                              Qt::AlignBottom | Qt::AlignRight);
	QPushButton *const okButton     = addWidget(okCancelLayout, new QPushButton(tr("OK")));
	QPushButton *const cancelButton = addWidget(okCancelLayout, new QPushButton(tr("Cancel")));
	okButton->setDefault(true);
	connect(okButton,     SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	for (std::size_t i = 0; i < buttons_.size(); ++i) {
		QString const label = buttons_[i]->label();
		if (!label.isEmpty()) {
			QString const category = buttons_[i]->category();
			int j = tabw->count() - 1;
			while (j >= 0 && tabw->tabText(j) != category)
				--j;

			if (j < 0) {
				QWidget *const w = new QWidget;
				j = tabw->addTab(w, category);

				QBoxLayout *boxl = new QVBoxLayout(w);
				boxl->addLayout(new QGridLayout);
				boxl->setAlignment(Qt::AlignTop);
			}

			int const defaultFpp = buttons_[i]->defaultFpp();
			QGridLayout *const gLayout =
				static_cast<QGridLayout *>(tabw->widget(j)->layout()->itemAt(0));
			gLayout->addWidget(new QLabel(label + ':'), i, 0);

			if (defaultFpp) {
				QHBoxLayout *hLayout = addLayout(gLayout, new QHBoxLayout, i, 1);
				inputBoxes_[i * 2    ] = addWidget(hLayout, new InputBox);
				  fppBoxes_[i * 2    ] = addFppBox(hLayout, defaultFpp);

				hLayout = addLayout(gLayout, new QHBoxLayout, i, 2);
				inputBoxes_[i * 2 + 1] = addWidget(hLayout, new InputBox);
				  fppBoxes_[i * 2 + 1] = addFppBox(hLayout, defaultFpp);
			} else {
				inputBoxes_[i * 2    ] = addWidget(gLayout, new InputBox, i, 1);
				inputBoxes_[i * 2 + 1] = addWidget(gLayout, new InputBox, i, 2);
			}

			QPushButton *const clearButton =
				addWidget(gLayout, new QPushButton(tr("Clear")), i, 3);
			connect(clearButton, SIGNAL(clicked()), inputBoxes_[i * 2 + 1], SLOT(clear()));
			connect(inputBoxes_[i * 2 + 1], SIGNAL(redundantClear()),
			        inputBoxes_[i * 2    ], SLOT(clear()));
		}
	}

	for (int tabi = 0; tabi < tabw->count(); ++tabi) {
		QWidget *const w = tabw->widget(tabi);
		std::size_t i = 0;
		while (i < inputBoxes_.size()
				&& (!inputBoxes_[i]
				    || inputBoxes_[i]->parentWidget() != w)) {
			i += 2;
		}

		while (i < inputBoxes_.size()) {
			std::size_t j = i + 2;
			while (j < inputBoxes_.size()
					&& (!inputBoxes_[j]
					    || inputBoxes_[j]->parentWidget() != w)) {
				j += 2;
			}

			if (j < inputBoxes_.size()) {
				inputBoxes_[i    ]->setNextFocus(inputBoxes_[j    ]);
				inputBoxes_[i + 1]->setNextFocus(inputBoxes_[j + 1]);
			} else {
				inputBoxes_[i    ]->setNextFocus(okButton);
				inputBoxes_[i + 1]->setNextFocus(okButton);
			}

			i = j;
		}
	}

	QSettings settings;
	settings.beginGroup("input");

	for (std::size_t i = 0; i < buttons_.size(); ++i) {
		int const defaultKey = buttons_[i]->defaultKey();
		int const defaultAltKey = buttons_[i]->defaultAltKey();
		int const defaultFpp = buttons_[i]->defaultFpp();
		QString const label = buttons_[i]->label();
		if (!label.isEmpty()) {
			QString const category = buttons_[i]->category();
			config_[i * 2    ].event.id =
				settings.value(category + label + "Key1", defaultKey).toUInt();
			config_[i * 2    ].event.value =
				settings.value(category + label + "Value1",
				                 defaultKey
				               ? InputBox::value_null
				               : InputBox::value_kbd).toInt();
			config_[i * 2    ].fpp = defaultFpp
				? settings.value(category + label + "Fpp1", defaultFpp).toInt()
				: 0;

			config_[i * 2 + 1].event.id =
				settings.value(category + label + "Key2", defaultAltKey).toUInt();
			config_[i * 2 + 1].event.value =
				settings.value(category + label + "Value2",
				                 defaultAltKey
				               ? InputBox::value_null
				               : InputBox::value_kbd).toInt();
			config_[i * 2 + 1].fpp = defaultFpp
				? settings.value(category + label + "Fpp2", defaultFpp).toInt()
				: 0;
		} else {
			config_[i * 2    ].event.id = defaultKey;
			config_[i * 2    ].event.value = defaultKey
			                               ? InputBox::value_null
			                               : InputBox::value_kbd;
			config_[i * 2    ].fpp = defaultFpp;

			config_[i * 2 + 1].event.id = defaultAltKey;
			config_[i * 2 + 1].event.value = defaultAltKey
			                               ? InputBox::value_null
			                               : InputBox::value_kbd;
			config_[i * 2 + 1].fpp = defaultFpp;
		}
	}

	settings.endGroup();
	restore();
	resetMapping();
}

InputDialog::~InputDialog() {
	QSettings settings;
	settings.beginGroup("input");

	for (std::size_t i = 0; i < buttons_.size(); ++i) {
		QString const label = buttons_[i]->label();
		if (!label.isEmpty()) {
			QString const category = buttons_[i]->category();
			settings.setValue(category + label + "Key1",   config_[i * 2    ].event.id);
			settings.setValue(category + label + "Value1", config_[i * 2    ].event.value);
			settings.setValue(category + label + "Key2",   config_[i * 2 + 1].event.id);
			settings.setValue(category + label + "Value2", config_[i * 2 + 1].event.value);
			if (buttons_[i]->defaultFpp()) {
				settings.setValue(category + label + "Fpp1", config_[i * 2    ].fpp);
				settings.setValue(category + label + "Fpp2", config_[i * 2 + 1].fpp);
			}
		}
	}

	settings.endGroup();
}

void InputDialog::resetMapping() {
	{
		Mutual<KeyMapping>::Locked lkm(keyMapping_);
		lkm->map.clear();
		lkm->rapidvec.clear();

		for (std::size_t i = 0; i < config_.size(); ++i) {
			if (config_[i].event.value == InputBox::value_kbd) {
				lkm->map.insert(std::make_pair(
					config_[i].event.id,
					MappedKey(buttons_[i >> 1], config_[i].fpp)));
			}
		}
	}

	{
		Mutual<JoyMapping>::Locked ljm(joyMapping_);
		ljm->map.clear();
		ljm->rapidvec.clear();

		for (std::size_t i = 0; i < config_.size(); ++i) {
			if (config_[i].event.value != InputBox::value_null
					&& config_[i].event.value != InputBox::value_kbd) {
				ljm->map.insert(std::make_pair(
					config_[i].event.id,
					MappedJoy(buttons_[i >> 1], config_[i].event.value,
					          config_[i].fpp)));
			}
		}
	}
}

void InputDialog::store() {
	for (std::size_t i = 0; i < inputBoxes_.size(); ++i) {
		if (inputBoxes_[i])
			config_[i].event = inputBoxes_[i]->data();
	}

	for (std::size_t i = 0; i < fppBoxes_.size(); ++i) {
		if (fppBoxes_[i])
			config_[i].fpp = fppBoxes_[i]->value();
	}

	resetMapping();
}

void InputDialog::restore() {
	for (std::size_t i = 0; i < inputBoxes_.size(); ++i) {
		if (inputBoxes_[i])
			inputBoxes_[i]->setData(config_[i].event);
	}

	for (std::size_t i = 0; i < fppBoxes_.size(); ++i) {
		if (fppBoxes_[i])
			fppBoxes_[i]->setValue(config_[i].fpp);
	}
}

template<class AutoPressVec>
static void setButtonPressed(AutoPressVec &v, InputDialog::Button *const button, int const fpp) {
	if (fpp) {
		for (typename AutoPressVec::iterator it = v.begin(); it != v.end(); ++it) {
			if (it->button == button && it->fpp == fpp)
				return;
		}

		v.push_back(typename AutoPressVec::value_type(button, fpp, 0));
	}

	button->pressed();
}

template<class AutoPressVec>
static void unsetButtonPressed(AutoPressVec &v, InputDialog::Button *const button, int const fpp) {
	if (fpp) {
		for (typename AutoPressVec::iterator it = v.begin(); it != v.end(); ++it) {
			if (it->button == button && it->fpp == fpp) {
				v.erase(it);
				break;
			}
		}
	}

	button->released();
}

void InputDialog::keyPress(QKeyEvent const *const e) {
	if (Mutual<KeyMapping>::TryLocked const &lm = keyMapping_) {
		for (std::pair<Kmap::iterator, Kmap::iterator> r
				= lm->map.equal_range(e->key()); r.first != r.second; ++r.first) {
			setButtonPressed(lm->rapidvec, r.first->second.button,
			                 r.first->second.fpp);
		}
	}
}

void InputDialog::keyRelease(QKeyEvent const *const e) {
	if (Mutual<KeyMapping>::TryLocked const &lm = keyMapping_) {
		for (std::pair<Kmap::iterator, Kmap::iterator> r
				= lm->map.equal_range(e->key()); r.first != r.second; ++r.first) {
			unsetButtonPressed(lm->rapidvec, r.first->second.button,
			                   r.first->second.fpp);
		}
	}
}

void InputDialog::joystickEvent(SDL_Event const &ev) {
	if (Mutual<JoyMapping>::TryLocked const &lm = joyMapping_) {
		for (std::pair<Jmap::iterator, Jmap::iterator> r
				= lm->map.equal_range(ev.id); r.first != r.second; ++r.first) {
			if ((ev.value & r.first->second.mask) == r.first->second.mask) {
				  setButtonPressed(lm->rapidvec, r.first->second.button,
				                   r.first->second.fpp);
			} else {
				unsetButtonPressed(lm->rapidvec, r.first->second.button,
				                   r.first->second.fpp);
			}
		}
	}
}

template<class AutoPressVec>
static void doConsumeAutoPress(AutoPressVec &v) {
	for (typename AutoPressVec::iterator it = v.begin(); it != v.end(); ++it) {
		if (it->fcnt == 0) {
			it->fcnt = it->fpp;
			it->button->released();
		} else if (--it->fcnt == 0)
			it->button->pressed();
	}
}

void InputDialog::consumeAutoPress() {
	if (Mutual<KeyMapping>::TryLocked const &lm = keyMapping_)
		doConsumeAutoPress(lm->rapidvec);
	if (Mutual<JoyMapping>::TryLocked const &lm = joyMapping_)
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
