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

#ifndef CUSTOMDEVCONF_H
#define CUSTOMDEVCONF_H

#include "scoped_ptr.h"
#include <QString>

class QCheckBox;
class QLineEdit;
class QWidget;

class CustomDevConf {
public:
	CustomDevConf(QString const &desc,
	              QString const &defaultDev,
	              QString const &confGroup,
	              QString const &customDev);
	~CustomDevConf();
	QWidget * settingsWidget() const { return confWidget_.get(); }
	void acceptSettings();
	void rejectSettings() const;
	QString const device() const;

private:
	QString const defaultDev_;
	QString const confGroup_;
	scoped_ptr<QWidget> const confWidget_;
	QCheckBox *const customDevBox_;
	QLineEdit *const customDevEdit_;
	QString customDev_;
	bool useCustomDev_;
};

#endif
