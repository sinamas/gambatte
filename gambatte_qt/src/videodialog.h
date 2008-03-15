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
#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include <vector>
#include <memory>

class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QRadioButton;
class BlitterWidget;
class FullModeToggler;

#include "resinfo.h"
#include "mediasource.h"

enum ScalingType { UNRESTRICTED, KEEP_RATIO, INTEGER };

class VideoDialog : public QDialog {
	Q_OBJECT

	const std::vector<BlitterWidget*> &engines;
	const FullModeToggler *const resHandler;
	QVBoxLayout *const topLayout;
	QWidget *engineWidget;
	QComboBox *const engineSelector;
	QComboBox *const winResSelector;
	const std::auto_ptr<QComboBox> winResSelectorBackup;
	std::vector<QComboBox*> fullResSelector;
	std::vector<QComboBox*> hzSelector;
	QRadioButton *const unrestrictedScalingButton;
	QRadioButton *const keepRatioButton;
	QRadioButton *const integerScalingButton;
	QComboBox *const sourceSelector;
	ScalingType scaling;
	QSize aspRatio;
	const QSize defaultRes;
	std::vector<int> fullIndex;
	std::vector<int> hzIndex;
	int engineIndex;
	int winIndex;
	int sourceIndexStore;
	
	void fillWinResSelector();
	void fillFullResSelector();
	void store();
	void restore();

private slots:
	void engineChange(int index);
	void fullresChange(int index);
	void sourceChange(int index);
	void integerScalingChange(bool checked);

public:
	VideoDialog(const std::vector<BlitterWidget*> &engines,
	            const std::vector<MediaSource::VideoSourceInfo> &sourceInfos,
	            const std::string &sourcesLabel,
	            const FullModeToggler *resHandler,
	            const QSize &aspectRatio,
	            QWidget *parent = 0);
	~VideoDialog();
	int engine() const;
	const QSize winRes() const;
	unsigned fullMode(unsigned screen) const;
	unsigned fullRate(unsigned screen) const;
	unsigned sourceIndex() const { return sourceIndexStore; }
	const QSize sourceSize() const;
	ScalingType scalingType() const { return scaling; }
	const QSize& aspectRatio() const { return aspRatio; }
	void setAspectRatio(const QSize &aspectRatio);
	void setVideoSources(const std::vector<MediaSource::VideoSourceInfo> &sourceInfos);
	
	
public slots:
	void accept();
	void reject();
};

#endif
