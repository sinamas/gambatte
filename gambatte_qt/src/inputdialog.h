/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>

class InputBox;

class InputDialog : public QDialog {
	Q_OBJECT
	
	InputBox *upBox;
	InputBox *downBox;
	InputBox *leftBox;
	InputBox *rightBox;
	InputBox *aBox;
	InputBox *bBox;
	InputBox *startBox;
	InputBox *selectBox;
	
	int upKey;
	int downKey;
	int leftKey;
	int rightKey;
	int aKey;
	int bKey;
	int startKey;
	int selectKey;
	
	void store();
	void restore();
	
public:
	InputDialog(QWidget *parent = 0);
	~InputDialog();
	
	int getUpKey() const { return upKey; }
	int getDownKey() const { return downKey; }
	int getLeftKey() const { return leftKey; }
	int getRightKey() const { return rightKey; }
	int getAKey() const { return aKey; }
	int getBKey() const { return bKey; }
	int getStartKey() const { return startKey; }
	int getSelectKey() const { return selectKey; }
	
public slots:
	void accept();
	void reject();
};

#endif
