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
#ifndef CUSTOMDEVCONF_H
#define CUSTOMDEVCONF_H

class QCheckBox;
class QLineEdit;
class QWidget;

#include <QObject>
#include <QByteArray>
#include <memory>

class CustomDevConf : public QObject {
	Q_OBJECT
	
	const char *const defaultstr;
	const char *const confgroup;
	const std::auto_ptr<QWidget> confWidget;
	QCheckBox *const customDevBox;
	QLineEdit *const customDevEdit;
	QByteArray customDevStr;
	bool useCustomDev;
	
private slots:
	void customDevBoxChange(bool checked);
	
public:
	CustomDevConf(const char *desc, const char *defaultstr, const char *confgroup, const char *customdevstr = 0);
	~CustomDevConf();
	QWidget* settingsWidget() const { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings() const;
	const char* device() const;
};

#endif
