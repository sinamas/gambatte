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
#include "customdevconf.h"

#include <QWidget>
#include <QCheckBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QSettings>

CustomDevConf::CustomDevConf(const char *desc, const char *defaultstr, const char *confgroup, const char *customdevstr) :
defaultstr(defaultstr),
confgroup(confgroup),
confWidget(new QWidget),
customDevBox(new QCheckBox(QString(desc))),
customDevEdit(new QLineEdit),
customDevStr(customdevstr ? customdevstr : defaultstr),
useCustomDev(false)
{
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(customDevBox);
	confWidget->layout()->addWidget(customDevEdit);
	
	{
		QSettings settings;
		settings.beginGroup(confgroup);
		useCustomDev = settings.value("useCustomDev", useCustomDev).toBool();
		customDevStr = settings.value("customDevStr", customDevStr).toByteArray();
		settings.endGroup();
	}
	
	customDevBoxChange(customDevBox->isChecked());
	connect(customDevBox, SIGNAL(toggled(bool)), this, SLOT(customDevBoxChange(bool)));
	rejectSettings();
}

CustomDevConf::~CustomDevConf() {
	QSettings settings;
	settings.beginGroup(confgroup);
	settings.setValue("useCustomDev", useCustomDev);
	settings.setValue("customDevStr", customDevStr);
	settings.endGroup();
}

void CustomDevConf::customDevBoxChange(const bool checked) {
	customDevEdit->setEnabled(checked);
}

void CustomDevConf::acceptSettings() {
	useCustomDev = customDevBox->isChecked();
	customDevStr = customDevEdit->text().toAscii();
}

void CustomDevConf::rejectSettings() const {
	customDevBox->setChecked(useCustomDev);
	customDevEdit->setText(customDevStr);
}

const char* CustomDevConf::device() const {
	return useCustomDev ? customDevStr.data() : defaultstr;
}
