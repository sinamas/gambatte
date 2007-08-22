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

class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QCheckBox;
class FilterInfo;
class BlitterWidget;
class FullResToggler;

#include "resinfo.h"

class VideoDialog : public QDialog {
	Q_OBJECT

	const std::vector<BlitterWidget*> &engines;
	const std::vector<ResInfo>& resVector;
	QVBoxLayout *topLayout;
	QWidget *engineWidget;
	QComboBox *engineSelector;
	QComboBox *winResSelector;
	QComboBox *winResSelectorBackup;
	QComboBox *fullResSelector;
// 	QComboBox *fullResSelectorBackup;
	QComboBox *hzSelector;
	QCheckBox *keepRatioBox;
	QCheckBox *integerScalingBox;
	QComboBox *filterSelector;
	int engineIndex;
	int winIndex;
	int fullIndex;
	int hzIndex;
	int filterIndexStore;
	bool keepRatio;
	bool integerScaling;
	
	void fillFullResSelector(const QSize &minimum);
	void store();
	void restore();

private slots:
	void engineChange(int index);
	void fullresChange(int index);
	void filterChange(int index);
	void keepRatioChange(bool checked);
	void integerScalingChange(bool checked);

public:
	VideoDialog(const std::vector<BlitterWidget*> &engines, std::vector<const FilterInfo*> filterInfo, const FullResToggler &resHandler, QWidget *parent = 0);
	~VideoDialog();
	const int engine() const;
	const QSize winRes() const;
	const unsigned fullMode() const;
	const unsigned fullRate() const;
	bool keepsRatio() const;
	bool scalesByInteger() const;
	const unsigned int filterIndex() const;
	
public slots:
	void accept();
	void reject();
};

#endif
