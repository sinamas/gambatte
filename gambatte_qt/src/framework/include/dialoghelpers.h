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

#ifndef DIALOGHELPERS_H
#define DIALOGHELPERS_H

#include <QString>

class QCheckBox;
class QComboBox;

template<class Parent, class L>
inline L * addLayout(Parent *p, L *l) {
	p->addLayout(l);
	return l;
}

template<class Parent, class L>
L * addLayout(Parent *p, L *l, Qt::Alignment alignment) {
	p->addLayout(l);
	p->setAlignment(l, alignment);
	return l;
}

template<class Layout, class W>
inline W * addWidget(Layout *l, W *w) {
	l->addWidget(w);
	return w;
}

inline int filterValue(int value, int upper, int lower = 0, int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;

	return value;
}

class PersistCheckBox {
public:
	PersistCheckBox(QCheckBox *checkBox, QString const &key, bool defaultValue);
	~PersistCheckBox();
	void accept();
	void reject() const;
	bool value() const { return value_; }
	QCheckBox * checkBox() const { return checkBox_; }

private:
	QCheckBox *const checkBox_;
	QString const key_;
	bool value_;
};

class PersistComboBox {
public:
	PersistComboBox(QString const &key, QComboBox *box);
	~PersistComboBox();
	QComboBox * box() const { return box_; }
	int index() const { return index_; }
	void accept();
	void reject() const;

private:
	QString const key_;
	QComboBox *const box_;
	int index_;
};

#endif
