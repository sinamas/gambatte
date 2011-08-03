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
#include "fpsselector.h"
#include <QComboBox>
#include <QInputDialog>
#include <QSettings>

static int getCustomIndex(const QComboBox *const comboBox) {
	return comboBox->findText(QString("Other..."));
}

static void setFps(QComboBox *const comboBox, const QSize &value) {
	const int customIndex = getCustomIndex(comboBox);
	const int valueIndex = comboBox->findData(value);
	
	if (valueIndex < 0) {
		if (customIndex < comboBox->count()) {
			comboBox->addItem(QString::number(static_cast<double>(value.width()) / value.height()) + " fps", value);
			
			if (customIndex + 4 < comboBox->count())
				comboBox->removeItem(customIndex + 1);
			
			comboBox->setCurrentIndex(comboBox->count() - 1);
		}
	} else
		comboBox->setCurrentIndex(valueIndex);
}

FpsSelector::FpsSelector()
: comboBox_(new QComboBox),
  value_(262144, 4389)
{
	comboBox_->addItem("GB/GBC (" + QString::number(262144.0 / 4389.0) + " fps)", QSize(262144, 4389));
	comboBox_->addItem(QString("Other..."));
	
	const QSize &loadedValue = QSettings().value("misc/fps", value_).toSize();
	value_ = loadedValue.width() > 0 && loadedValue.height() > 0
			&& loadedValue.width() / loadedValue.height() > 0 ? loadedValue : value_;
	
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

void FpsSelector::indexChanged(const int index) {
	if (getCustomIndex(comboBox_) == index) {
		bool ok = false;
		
		const QSize v(QInputDialog::getDouble(comboBox_, tr("Set Frame Rate"), tr("Frame rate (fps):"),
				static_cast<double>(value_.width()) / value_.height(), 30.0, 120.0, 4, &ok) * 10000 + 0.5, 10000);
		
		setFps(comboBox_, ok ? v : value_);
	}
}
