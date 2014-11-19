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

#include "pathselector.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QSettings>

static int getCustomIndex(QComboBox const *comboBox) {
	return comboBox->findText(QObject::tr("Other..."));
}

static void setPath(QComboBox *const comboBox, QString const &value) {
	int const valueIndex = comboBox->findData(value);
	if (valueIndex < 0) {
		comboBox->addItem(QDir::toNativeSeparators(value), value);

		int const customIndex = getCustomIndex(comboBox);
		if (comboBox->count() > customIndex + 2)
			comboBox->removeItem(customIndex + 1);

		comboBox->setCurrentIndex(comboBox->count() - 1);
	} else
		comboBox->setCurrentIndex(valueIndex);
}

PathSelector::PathSelector(QString const &caption,
                           QString const &settingskey,
                           Mapping const &default1,
                           Mapping const &default2,
                           QWidget *widgetParent)
: comboBox_(new QComboBox(widgetParent))
, caption_(caption)
, key_(settingskey)
, value_(QSettings().value(key_, default1.second).toString())
{
	comboBox_->addItem(default1.first, default1.second);
	if (default2 != Mapping())
		comboBox_->addItem(default2.first, default2.second);

	comboBox_->addItem(tr("Other..."));

	reject();
	connect(comboBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(indexChanged(int)));
}

PathSelector::~PathSelector() {
	QSettings settings;
	settings.setValue(key_, value_);
}

void PathSelector::accept() {
	value_ = comboBox_->itemData(comboBox_->currentIndex()).toString();
}

void PathSelector::reject() {
	setPath(comboBox_, value_);
}

QWidget * PathSelector::widget() const {
	return comboBox_;
}

void PathSelector::indexChanged(int const index) {
	if (getCustomIndex(comboBox_) == index) {
		QString const &dir = QFileDialog::getExistingDirectory(comboBox_, caption_, value_);
		setPath(comboBox_, dir.isEmpty() ? value_ : dir);
	}
}
