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
#ifndef PERSIST_CHECKBOX_H
#define PERSIST_CHECKBOX_H

#include <QString>

class QCheckBox;
class QWidget;

class PersistCheckBox {
	QCheckBox *const checkBox_;
	const QString key_;
	bool value_;

public:
	PersistCheckBox(QCheckBox *checkBox, const QString &key, bool defaultValue);
	~PersistCheckBox();
	void accept();
	void reject() const;
	bool value() const { return value_; }
	QCheckBox * checkBox() const { return checkBox_; }
};

#endif
