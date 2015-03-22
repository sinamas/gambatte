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

#include "customdevconf.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

CustomDevConf::CustomDevConf(QString const &desc,
                             QString const &defaultDev,
                             QString const &confGroup,
                             QString const &customDev)
: defaultDev_(defaultDev)
, confGroup_(confGroup)
, confWidget_(new QWidget)
, customDevBox_(new QCheckBox(desc, confWidget_.get()))
, customDevEdit_(new QLineEdit(confWidget_.get()))
, customDev_(customDev)
, useCustomDev_(false)
{
	confWidget_->setLayout(new QVBoxLayout);
	confWidget_->layout()->setMargin(0);
	confWidget_->layout()->addWidget(customDevBox_);
	confWidget_->layout()->addWidget(customDevEdit_);

	{
		QSettings settings;
		settings.beginGroup(confGroup);
		useCustomDev_ = settings.value("useCustomDev", useCustomDev_).toBool();
		customDev_ = settings.value("customDevStr", customDev_).toString();
		settings.endGroup();
	}

	customDevEdit_->setEnabled(customDevBox_->isChecked());
	QObject::connect(customDevBox_, SIGNAL(toggled(bool)),
	                 customDevEdit_, SLOT(setEnabled(bool)));
	rejectSettings();
}

CustomDevConf::~CustomDevConf() {
	QSettings settings;
	settings.beginGroup(confGroup_);
	settings.setValue("useCustomDev", useCustomDev_);
	settings.setValue("customDevStr", customDev_);
	settings.endGroup();
}

void CustomDevConf::acceptSettings() {
	useCustomDev_ = customDevBox_->isChecked();
	customDev_ = customDevEdit_->text();
}

void CustomDevConf::rejectSettings() const {
	customDevBox_->setChecked(useCustomDev_);
	customDevEdit_->setText(customDev_);
}

QString const CustomDevConf::device() const {
	return useCustomDev_ ? customDev_ : defaultDev_;
}
