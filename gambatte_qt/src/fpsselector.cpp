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

#include "fpsselector.h"
#include <QComboBox>
#include <QInputDialog>
#include <QSettings>

static int getCustomIndex(QComboBox const *comboBox) {
	return comboBox->findText(QObject::tr("Other..."));
}

static void setFps(QComboBox *const comboBox, QSize const &value) {
	int const valueIndex = comboBox->findData(value);
	if (valueIndex < 0) {
		comboBox->addItem(QString::number(double(value.width()) / value.height()) + " fps",
		                  value);

		int const customIndex = getCustomIndex(comboBox);
		if (customIndex + 4 < comboBox->count())
			comboBox->removeItem(customIndex + 1);

		comboBox->setCurrentIndex(comboBox->count() - 1);
	} else
		comboBox->setCurrentIndex(valueIndex);
}

FpsSelector::FpsSelector(QWidget *widgetParent)
: comboBox_(new QComboBox(widgetParent))
, value_(262144, 4389)
{
	comboBox_->addItem("GB/GBC (" + QString::number(262144.0 / 4389.0) + " fps)",
	                   QSize(262144, 4389));
	comboBox_->addItem(tr("Other..."));

	QSize const &loadedValue = QSettings().value("misc/fps", value_).toSize();
	value_ =    loadedValue.width() > 0
	         && loadedValue.height() > 0
	         && loadedValue.width() / loadedValue.height() > 0
	       ? loadedValue
	       : value_;
	reject();
	connect(comboBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(indexChanged(int)));
}

FpsSelector::~FpsSelector() {
	QSettings settings;
	settings.setValue("misc/fps", value_);
}

void FpsSelector::accept() {
	value_ = comboBox_->itemData(comboBox_->currentIndex()).toSize();
}

void FpsSelector::reject() {
	setFps(comboBox_, value_);
}

QWidget * FpsSelector::widget() const {
	return comboBox_;
}

void FpsSelector::indexChanged(int const index) {
	if (getCustomIndex(comboBox_) == index) {
		bool ok = false;
		double const fps =
			QInputDialog::getDouble(comboBox_, tr("Set Frame Rate"),
			                       tr("Frame rate (fps):"),
			                       double(value_.width()) / value_.height(),
			                       30.0, 120.0, 4, &ok);
		setFps(comboBox_,
		         ok
		       ? QSize(int(fps * 10000 + 0.5), 10000)
		       : value_);
	}
}
