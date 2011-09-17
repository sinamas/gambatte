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
#include "pathselector.h"
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QSettings>

static int getCustomIndex(const QComboBox *const comboBox) {
	return comboBox->findText(QObject::tr("Other..."));
}

static void setPath(QComboBox *const comboBox, const QString &value) {
	const int valueIndex = comboBox->findData(value);
	
	if (valueIndex < 0) {
		comboBox->addItem(QDir::toNativeSeparators(value), value);
		
		const int customIndex = getCustomIndex(comboBox);
		
		if (comboBox->count() > customIndex + 2)
			comboBox->removeItem(customIndex + 1);
		
		comboBox->setCurrentIndex(comboBox->count() - 1);
	} else
		comboBox->setCurrentIndex(valueIndex);
}

PathSelector::PathSelector(const QString &caption, const QString &settingskey,
                           const Mapping &default1, const Mapping &default2)
: comboBox_(new QComboBox),
  value_(default1.second),
  caption_(caption),
  key_(settingskey)
{
	comboBox_->addItem(default1.first, default1.second);
	
	if (default2 != Mapping())
		comboBox_->addItem(default2.first, default2.second);
	
	comboBox_->addItem(tr("Other..."));
	
	value_ = QSettings().value(key_, value_).toString();
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

void PathSelector::indexChanged(const int index) {
	if (getCustomIndex(comboBox_) == index) {
		const QString &dir = QFileDialog::getExistingDirectory(comboBox_, caption_, value_);
		setPath(comboBox_, dir.isEmpty() ? value_ : dir);
	}
}
