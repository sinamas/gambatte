/***************************************************************************
 *   Copyright (C) 2011 by Sindre Aamås                                    *
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
#include "../persistcheckbox.h"
#include <QCheckBox>
#include <QSettings>

PersistCheckBox::PersistCheckBox(QCheckBox *const checkBox, const QString &key, const bool defaultValue)
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
