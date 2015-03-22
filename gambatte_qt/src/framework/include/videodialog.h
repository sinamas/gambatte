//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include "auto_vector.h"
#include "dialoghelpers.h"
#include "resinfo.h"
#include "scalingmethod.h"
#include <QDialog>
#include <QSize>
#include <vector>

class MainWindow;
class QRadioButton;

/**
  * A utility class that can optionally be used to provide a GUI for
  * configuring video settings.
  */
class VideoDialog : public QDialog {
public:
	struct VideoSourceInfo {
		QString label;
		QSize size;
	};

	VideoDialog(MainWindow const &mw,
	            std::vector<VideoSourceInfo> const &sourceInfos,
	            QString const &sourcesLabel,
	            QWidget *parent = 0);
	std::size_t blitterNo() const { return engineSelector_.index(); }
	std::size_t fullResIndex(std::size_t screen) const;
	std::size_t fullRateIndex(std::size_t screen) const;
	std::size_t sourceIndex() const { return sourceSelector_.index(); }
	QSize const sourceSize() const;
	ScalingMethod scalingMethod() const { return scalingMethodSelector_.scalingMethod(); }

public slots:
	virtual void accept();
	virtual void reject();

private:
	Q_OBJECT

	class ScalingMethodSelector {
	public:
		explicit ScalingMethodSelector(QWidget *parent);
		~ScalingMethodSelector();
		void addToLayout(QLayout *layout);
		void accept();
		void reject();
		ScalingMethod scalingMethod() const { return scaling_; }

	private:
		QRadioButton *const unrestrictedScalingButton_;
		QRadioButton *const keepRatioButton_;
		QRadioButton *const integerScalingButton_;
		ScalingMethod scaling_;
	};

	MainWindow const &mw_;
	PersistComboBox engineSelector_;
	PersistComboBox sourceSelector_;
	auto_vector<PersistComboBox> const fullResSelectors_;
	auto_vector<PersistComboBox> const fullHzSelectors_;
	ScalingMethodSelector scalingMethodSelector_;
	QWidget *engineWidget_;

	void resIndexChange(std::size_t screen, int index);

private slots:
	void engineChange(int index);
	void resIndexChange(int index);
	void sourceChange(int index);
};

void applySettings(MainWindow &mw, VideoDialog const &vd);

#endif
