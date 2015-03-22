//
//   Copyright (C) 2011 by sinamas <sinamas at users.sourceforge.net>
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

#include "dialoghelpers.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSettings>
#include <algorithm>

PersistCheckBox::PersistCheckBox(QCheckBox *const checkBox, QString const &key, bool defaultValue)
: checkBox_(checkBox), key_(key), value_(defaultValue)
{
	value_ = QSettings().value(key, defaultValue).toBool();
	reject();
}

PersistCheckBox::~PersistCheckBox() {
	QSettings settings;
	settings.setValue(key_, value_);
}

void PersistCheckBox::accept() {
	value_ = checkBox_->isChecked();
}

void PersistCheckBox::reject() const {
	checkBox_->setChecked(value_);
}

PersistComboBox::PersistComboBox(QString const &key, QComboBox *box)
: key_(key)
, box_(box)
, index_(std::max(0, box->currentIndex()))
{
	index_ = filterValue(box->findText(QSettings().value(key, box->itemText(index_)).toString()),
	                     box->count(), 0, index_);
	box->setCurrentIndex(index_);
}

PersistComboBox::~PersistComboBox() {
	QSettings settings;
	settings.setValue(key_, box_->itemText(index_));
}

void PersistComboBox::accept() {
	index_ = box_->currentIndex();
}

void PersistComboBox::reject() const {
	box_->setCurrentIndex(index_);
}
