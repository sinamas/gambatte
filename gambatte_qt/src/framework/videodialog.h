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
#ifndef VIDEODIALOG_H
#define VIDEODIALOG_H

#include <QDialog>
#include <QSize>
#include <vector>
#include <memory>

class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QRadioButton;
class QLabel;

// #include "resinfo.h"
#include "mainwindow.h"

/** A utility class that can optionally be used to provide a GUI for
  * configuring video settings.
  */
class VideoDialog : public QDialog {
	Q_OBJECT

public:
	struct VideoSourceInfo {
		// label used in the video dialog combobox.
		QString label;
		
		unsigned width;
		unsigned height;
	};
	
private:
	const MainWindow *const mw;
	QVBoxLayout *const topLayout;
	QWidget *engineWidget;
	QComboBox *const engineSelector;
	QComboBox *const winResSelector;
// 	const std::auto_ptr<QComboBox> winResSelectorBackup;
	std::vector<QComboBox*> fullResSelector;
	std::vector<QComboBox*> hzSelector;
	QRadioButton *const unrestrictedScalingButton;
	QRadioButton *const keepRatioButton;
	QRadioButton *const integerScalingButton;
	QComboBox *const sourceSelector;
	QLabel *const sourceSelectorLabel;
	ScalingMethod scaling;
	QSize aspRatio;
	const QSize defaultRes;
	std::vector<int> fullIndex;
	std::vector<int> hzIndex;
	int engineIndex;
	int winIndex;
	int sourceIndexStore;
	
	void fillWinResSelector();
	void fillFullResSelector();
	void fillSourceSelector(const std::vector<VideoSourceInfo> &sourceInfos);
	void store();
	void restore();

private slots:
	void engineChange(int index);
	void fullresChange(int index);
	void sourceChange(int index);
	void integerScalingChange(bool checked);

public:
	VideoDialog(const MainWindow *mw,
	            const std::vector<VideoSourceInfo> &sourceInfos,
	            const QString &sourcesLabel,
	            const QSize &aspectRatio,
	            QWidget *parent = 0);
	~VideoDialog();
	int blitterNo() const;
	const QSize windowSize() const;
	unsigned fullResIndex(unsigned screen) const;
	unsigned fullRateIndex(unsigned screen) const;
	unsigned sourceIndex() const { return sourceIndexStore; }
	const QSize sourceSize() const;
	void setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos);
	void setSourceSize(const QSize &sourceSize);
	ScalingMethod scalingMethod() const { return scaling; }
	const QSize& aspectRatio() const { return aspRatio; }
	void setAspectRatio(const QSize &aspectRatio);
	
public slots:
	void accept();
	void reject();
};

void applySettings(MainWindow *mw, const VideoDialog *vd);

#endif
