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

#ifndef SOUNDDIALOG_H
#define SOUNDDIALOG_H

class AudioEngine;
class QVBoxLayout;
class QComboBox;

#include <QDialog>
#include <vector>

class SoundDialog : public QDialog {
	Q_OBJECT
	
	const std::vector<AudioEngine*> &engines;
	QVBoxLayout *const topLayout;
	QComboBox *const engineSelector;
	QComboBox *const rateSelector;
	QWidget *engineWidget;
	int engineIndex;
	int rateIndex;
	
	void store();
	void restore();
	
private slots:
	void engineChange(int index);
	
public:
	SoundDialog(const std::vector<AudioEngine*> &engines, QWidget *parent = 0);
	~SoundDialog();
	int getEngineIndex() const { return engineIndex; }
	int getRate() const;
	
public slots:
	void accept();
	void reject();
};

#endif
