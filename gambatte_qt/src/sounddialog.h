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

#ifndef SOUNDDIALOG_H
#define SOUNDDIALOG_H

class AudioEngine;
class QVBoxLayout;
class QComboBox;
class QSpinBox;

#include <QDialog>
#include "mediasource.h"

class SoundDialog : public QDialog {
	Q_OBJECT
	
	const std::vector<AudioEngine*> &engines;
	QVBoxLayout *const topLayout;
	QComboBox *const engineSelector;
	QComboBox *const resamplerSelector;
	QComboBox *const rateSelector;
	QSpinBox *const latencySelector;
	QWidget *engineWidget;
	int engineIndex;
	int resamplerNum;	
	int rate;
	int latency;
	
	void store();
	void restore();
	
private slots:
	void engineChange(int index);
	void rateIndexChange(int index);
	
public:
	SoundDialog(const std::vector<AudioEngine*> &engines, const MediaSource::SampleRateInfo &rateInfo, QWidget *parent = 0);
	~SoundDialog();
	int getEngineIndex() const { return engineIndex; }
	int getResamplerNum() const { return resamplerNum; }
	int getRate() const { return rate; }
	int getLatency() const { return latency; };
	void setRates(const MediaSource::SampleRateInfo &rateInfo);
	
public slots:
	void accept();
	void reject();
};

#endif
