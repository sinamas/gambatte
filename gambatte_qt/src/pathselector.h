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

#ifndef PATHSELECTOR_H
#define PATHSELECTOR_H

#include <QObject>
#include <QString>
#include <utility>

class QComboBox;
class QWidget;

class PathSelector : private QObject {
public:
	typedef std::pair<QString, QString> Mapping;

	PathSelector(QString const &caption,
	             QString const &settingskey,
	             Mapping const &default1,
	             Mapping const &default2,
	             QWidget *widgetParent);
	~PathSelector();
	void accept();
	void reject();
	QString const & value() const { return value_; }
	QWidget * widget() const;

private:
	Q_OBJECT

	QComboBox *const comboBox_;
	QString const caption_;
	QString const key_;
	QString value_;

private slots:
	void indexChanged(int index);
};

#endif
