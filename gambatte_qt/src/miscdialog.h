/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#ifndef MISCDIALOG_H
#define MISCDIALOG_H

class QSpinBox;
class QCheckBox;

#include <QDialog>

class MiscDialog : public QDialog {
	QSpinBox *const turboSpeedBox;
	QCheckBox *const pauseOnDialogsBox;
	QCheckBox *const pauseOnFocusOutBox;
	int turboSpeed_;
	bool pauseOnDialogs_;
	bool pauseOnFocusOut_;
	
	void store();
	void restore();
public:
	MiscDialog(QWidget *parent = 0);
	~MiscDialog();
	int turboSpeed() const { return turboSpeed_; }
	bool pauseOnDialogs() const { return pauseOnDialogs_ | pauseOnFocusOut_; }
	bool pauseOnFocusOut() const { return pauseOnFocusOut_; }

public slots:
	void accept();
	void reject();
};

#endif
