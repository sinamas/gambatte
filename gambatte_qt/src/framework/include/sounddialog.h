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

class MainWindow;
class QVBoxLayout;
class QComboBox;
class QSpinBox;

#include <QDialog>

/** A utility class that can optionally be used to provide a GUI for
  * configuring sound/audio settings.
  */
class SoundDialog : public QDialog {
	Q_OBJECT
	
	const MainWindow *const mw;
	QVBoxLayout *const topLayout;
	QComboBox *const engineSelector;
	QComboBox *const resamplerSelector;
	QComboBox *const rateSelector;
	QSpinBox *const latencySelector;
	QWidget *engineWidget;
	int engineIndex_;
	int resamplerNum;
	int rate_;
	int latency_;
	
	void store();
	void restore();
	
private slots:
	void engineChange(int index);
	void rateIndexChange(int index);
	
public:
	explicit SoundDialog(const MainWindow *mw, QWidget *parent = 0);
	~SoundDialog();
	int engineIndex() const { return engineIndex_; }
	int resamplerNo() const { return resamplerNum; }
	int rate() const { return rate_; }
	int latency() const { return latency_; };
	
	static void applySettings(MainWindow *mw, const SoundDialog *sd);
	
public slots:
	void accept();
	void reject();
};

#endif
