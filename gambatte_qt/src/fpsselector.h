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

#ifndef FPSSELECTOR_H
#define FPSSELECTOR_H

#include <QObject>
#include <QSize>

class QComboBox;
class QWidget;

class FpsSelector : private QObject {
public:
	explicit FpsSelector(QWidget *widgetParent);
	~FpsSelector();
	void accept();
	void reject();
	QSize const value() const { return value_; }
	QWidget * widget() const;

private:
	Q_OBJECT

	QComboBox *const comboBox_;
	QSize value_;

private slots:
	void indexChanged(int index);
};

#endif
