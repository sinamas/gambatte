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

#include "auto_vector.h"
#include "resinfo.h"
#include "scalingmethod.h"
#include <QDialog>
#include <QSize>
#include <vector>
#include <memory>

class MainWindow;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QAbstractButton;
class QRadioButton;
class QLabel;
class QBoxLayout;

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
	class PersistInt {
		const QString key_;
		int i_;
	public:
		PersistInt(const QString &key, int upper, int lower = 0, int defaultVal = 0);
		~PersistInt();
		PersistInt & operator=(int i) { i_ = i; return *this; }
		operator int() const { return i_; }
	};
	
	class EngineSelector {
		QComboBox *const comboBox_;
		PersistInt index_;
		
	public:
		explicit EngineSelector(const MainWindow *mw);
		void addToLayout(QBoxLayout *topLayout);
		const QComboBox * comboBox() const { return comboBox_; }
		void store();
		void restore();
		int index() const { return index_; }
	};
	
	class ScalingMethodSelector {
		QRadioButton *const unrestrictedScalingButton_;
		QRadioButton *const keepRatioButton_;
		QRadioButton *const integerScalingButton_;
		PersistInt scaling_;
		
	public:
		ScalingMethodSelector();
		void addToLayout(QLayout *layout);
		const QAbstractButton * integerScalingButton() const;
		void store();
		void restore();
		ScalingMethod scalingMethod() const { return static_cast<ScalingMethod>(static_cast<int>(scaling_)); }
	};
	
	class SourceSelector {
		QLabel *const label_;
		QComboBox *const comboBox_;
		PersistInt index_;
		
	public:
		SourceSelector(const QString &sourcesLabel, const std::vector<VideoSourceInfo> &sourceInfos);
		void addToLayout(QBoxLayout *layout);
		const QComboBox * comboBox() const { return comboBox_; }
		void setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos);
		void store();
		void restore();
		int index() const { return index_; }
	};
	
	class FullResSelector {
		QComboBox *const comboBox_;
		const QString key_;
		int index_;
		
		void fillComboBox(const QSize &sourceSize, const std::vector<ResInfo> &resVector);
	public:
		FullResSelector(const QString &key, int defaultIndex,
				const QSize &sourceSize, const std::vector<ResInfo> &resVector);
		~FullResSelector();
		QWidget * widget();
		const QComboBox * comboBox() const { return comboBox_; }
		void store();
		void restore();
		void setSourceSize(const QSize &sourceSize, const std::vector<ResInfo> &resVector);
		int index() const { return index_; }
	};
	
	class FullHzSelector {
		QComboBox *const comboBox_;
		const QString key_;
		int index_;
		
	public:
		FullHzSelector(const QString &key, const std::vector<short> &rates, int defaultIndex);
		~FullHzSelector();
		QWidget * widget();
		const QComboBox * comboBox() const { return comboBox_; }
		void store();
		void restore();
		void setRates(const std::vector<short> &rates);
		int index() const { return index_; }
	};
	
	const MainWindow *const mw;
	QVBoxLayout *const topLayout;
	QWidget *engineWidget;
	EngineSelector engineSelector;
	ScalingMethodSelector scalingMethodSelector;
	SourceSelector sourceSelector;
	const auto_vector<FullResSelector> fullResSelectors;
	const auto_vector<FullHzSelector> fullHzSelectors;
	
	static auto_vector<FullResSelector> makeFullResSelectors(const QSize &sourceSize, const MainWindow *mw);
	static auto_vector<FullHzSelector> makeFullHzSelectors(
			const auto_vector<FullResSelector> &fullResSelectors, const MainWindow *mw);
	void fillFullResSelectors();
	void store();
	void restore();

private slots:
	void engineChange(int index);
	void fullresChange(int index);
	void sourceChange(int index);

public:
	VideoDialog(const MainWindow *mw,
	            const std::vector<VideoSourceInfo> &sourceInfos,
	            const QString &sourcesLabel,
	            QWidget *parent = 0);
	int blitterNo() const;
	unsigned fullResIndex(unsigned screen) const;
	unsigned fullRateIndex(unsigned screen) const;
	unsigned sourceIndex() const { return sourceSelector.index(); }
	const QSize sourceSize() const;
	void setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos);
	void setSourceSize(const QSize &sourceSize);
	ScalingMethod scalingMethod() const { return scalingMethodSelector.scalingMethod(); }
	
public slots:
	void accept();
	void reject();
};

void applySettings(MainWindow *mw, const VideoDialog *vd);

#endif
