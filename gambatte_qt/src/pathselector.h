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
#ifndef PATHSELECTOR_H
#define PATHSELECTOR_H

#include <QObject>
#include <QString>
#include <utility>

class QComboBox;
class QWidget;

class PathSelector : public QObject {
	Q_OBJECT
	
	QComboBox *const comboBox_;
	QString value_;
	const QString caption_;
	const QString key_;
	
private slots:
	void indexChanged(int index);
	
public:
	typedef std::pair<QString, QString> Mapping;
	
	PathSelector(const QString &caption, const QString &settingskey,
	             const Mapping &default1, const Mapping &default2 = Mapping());
	~PathSelector();
	void accept();
	void reject();
	const QString & value() const { return value_; }
	QWidget * widget() const;
};

#endif
