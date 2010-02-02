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
#include "videodialog.h"

#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QGroupBox>
#include <QApplication>
#include <QDesktopWidget>

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

VideoDialog::VideoDialog(const MainWindow *const mw,
                         const std::vector<VideoSourceInfo> &sourceInfos,
                         const QString &sourcesLabel,
                         const QSize &aspectRatio,
                         QWidget *parent) :
QDialog(parent),
mw(mw),
topLayout(new QVBoxLayout),
engineWidget(NULL),
engineSelector(new QComboBox),
winResSelector(new QComboBox),
unrestrictedScalingButton(new QRadioButton(QString("None"))),
keepRatioButton(new QRadioButton(QString("Keep aspect ratio"))),
integerScalingButton(new QRadioButton(QString("Only scale by integer factors"))),
sourceSelector(new QComboBox),
sourceSelectorLabel(new QLabel(sourcesLabel)),
scaling(KEEP_RATIO),
aspRatio(aspectRatio),
defaultRes(QApplication::desktop()->screen()->size()),
engineIndex(0),
winIndex(0),
sourceIndexStore(0)
{
	fullIndex.resize(mw->screens());
	hzIndex.resize(fullIndex.size());
	fullResSelector.resize(fullIndex.size());
	hzSelector.resize(fullIndex.size());

	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(tr("Video engine:")));
	hLayout->addWidget(engineSelector);
	topLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(QString(tr("Windowed resolution:"))));
	hLayout->addWidget(winResSelector);
	topLayout->addLayout(hLayout);

	for (unsigned i = 0; i < hzSelector.size(); ++i) {
		hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel("Full screen mode (screen " + QString::number(i) + "):"));
		QHBoxLayout *hhLayout = new QHBoxLayout;
		hhLayout->addWidget((fullResSelector[i] = new QComboBox));
		hhLayout->addWidget((hzSelector[i] = new QComboBox));
		hLayout->addLayout(hhLayout);
		topLayout->addLayout(hLayout);
	}

	{
		QGroupBox *f = new QGroupBox("Scaling restrictions");
		f->setLayout(new QVBoxLayout);
		f->layout()->addWidget(unrestrictedScalingButton);
		f->layout()->addWidget(keepRatioButton);
		f->layout()->addWidget(integerScalingButton);
		topLayout->addWidget(f);
	}

	hLayout = new QHBoxLayout;
	hLayout->addWidget(sourceSelectorLabel);
	hLayout->addWidget(sourceSelector);
	topLayout->addLayout(hLayout);

	mainLayout->addLayout(topLayout);
	mainLayout->setAlignment(topLayout, Qt::AlignTop);

	hLayout = new QHBoxLayout;
	QPushButton *okButton = new QPushButton(tr("OK"));
	hLayout->addWidget(okButton);
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	hLayout->addWidget(cancelButton);
	mainLayout->addLayout(hLayout);
	mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	okButton->setDefault(true);

	connect(engineSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChange(int)));
	
	for (unsigned i = 0; i < fullResSelector.size(); ++i) {
		connect(fullResSelector[i], SIGNAL(currentIndexChanged(int)), this, SLOT(fullresChange(int)));
	}
	
	connect(integerScalingButton, SIGNAL(toggled(bool)), this, SLOT(integerScalingChange(bool)));
	connect(sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	
	fillSourceSelector(sourceInfos);
	
	for (std::size_t i = 0; i < mw->numBlitters(); ++i) {
		engineSelector->addItem(mw->blitterConf(i).nameString());
	}
	
	fillWinResSelector();
	fillFullResSelector();
	keepRatioButton->click();
	
	QSettings settings;
	settings.beginGroup("video");
	
	engineIndex = filterValue(settings.value("engineIndex", 0).toInt(), engineSelector->count());
	sourceIndexStore = filterValue(settings.value("sourceIndexStore", 0).toInt(), sourceSelector->count());
	
	switch (settings.value("scalingType", KEEP_RATIO).toInt()) {
	case UNRESTRICTED: scaling = UNRESTRICTED; break;
	case INTEGER: scaling = INTEGER; break;
	default: scaling = KEEP_RATIO; break;
	}
	
	restore();
	
	for (unsigned i = 0; i < fullIndex.size(); ++i) {
		fullIndex[i] = filterValue(fullResSelector[i]->findText(settings.value("fullRes" + QString::number(i)).toString()),
		                           fullResSelector[i]->count(),
		                           0,
		                           mw->currentResIndex(i));
	}
		
	winIndex = filterValue(settings.value("winIndex", winResSelector->count() - 1).toInt(), winResSelector->count(), 0, winResSelector->count() - 1);
	
	restore();
	
	for (unsigned i = 0; i < hzIndex.size(); ++i) {
		hzIndex[i] = filterValue(hzSelector[i]->findText(settings.value("hz" + QString::number(i)).toString()),
		                         hzSelector[i]->count(),
		                         0,
		                         mw->currentRateIndex(i));
	}
	
	restore();
	store();
	
	settings.endGroup();

	setWindowTitle(tr("Video Settings"));
}

VideoDialog::~VideoDialog() {
// 	delete fullResSelectorBackup;
	
	QSettings settings;
	settings.beginGroup("video");
	
	for (unsigned i = 0; i < fullIndex.size(); ++i) {
		settings.setValue("fullRes" + QString::number(i), fullResSelector[i]->itemText(fullIndex[i]));
	}
	
	settings.setValue("winIndex", winIndex);
	
	for (unsigned i = 0; i < hzIndex.size(); ++i) {
		settings.setValue("hz" + QString::number(i), hzSelector[i]->itemText(hzIndex[i]));
	}
	
	settings.setValue("scalingType", (int)scaling);
	settings.setValue("sourceIndexStore", sourceIndexStore);
	settings.setValue("engineIndex", engineIndex);
	settings.endGroup();
}

void VideoDialog::fillWinResSelector() {
	const QString oldtext(winResSelector->itemText(winResSelector->currentIndex()));
	
	winResSelector->clear();
	
	const QSize &sourceSize = sourceSelector->itemData(sourceSelector->currentIndex()).toSize();
	QSize basesz(integerScalingButton->isChecked() ? sourceSize : aspRatio);
	
	/*if (!integerScalingButton->isChecked()) {
		const unsigned scale = std::max(sourceSize.width() / aspRatio.width(), sourceSize.height() / aspRatio.height());
		
		basesz = QSize(aspRatio.width() * scale, aspRatio.height() * scale);
		
		if (basesz.width() < sourceSize.width() || basesz.height() < sourceSize.height())
			basesz += aspRatio;
	}*/
	
	QSize sz(basesz);
	
	while (sz.width() <= defaultRes.width() && sz.height() <= defaultRes.height()) {
		if (sz.width() >= sourceSize.width() && sz.height() >= sourceSize.height())
			winResSelector->addItem(QString::number(sz.width()) + "x" + QString::number(sz.height()), sz);
		
		sz += basesz;
	}
	
	winResSelector->addItem(QString(tr("Variable")), QVariant(QSize(-1, -1)));
	
	const int newIndex = winResSelector->findText(oldtext);
	
	if (newIndex >= 0)
		winResSelector->setCurrentIndex(newIndex);
}

void VideoDialog::fillFullResSelector() {
	for (unsigned j = 0; j < fullResSelector.size(); ++j) {
		const QString oldtext(fullResSelector[j]->itemText(fullResSelector[j]->currentIndex()));
		const int oldHzIndex = hzSelector[j]->currentIndex();
		
		fullResSelector[j]->clear();
		
		const QSize &sourceSize = sourceSelector->itemData(sourceSelector->currentIndex()).toSize();
		
		unsigned maxArea = 0;
		unsigned maxAreaI = 0;
		
		const std::vector<ResInfo> &resVector = mw->modeVector(j);
		
		for (unsigned i = 0; i < resVector.size(); ++i) {
			const int hres = resVector[i].w;
			const int vres = resVector[i].h;
			
			if (hres >= sourceSize.width() && vres >= sourceSize.height()) {
				fullResSelector[j]->addItem(QString::number(hres) + QString("x") + QString::number(vres), i);
			} else {
				const unsigned area = std::min(hres, sourceSize.width()) * std::min(vres, sourceSize.height());
				
				if (area > maxArea) {
					maxArea = area;
					maxAreaI = i;
				}
			}
		}
		
		//add resolution giving maximal area if all resolutions are too small.
		if (fullResSelector[j]->count() < 1 && maxArea)
			fullResSelector[j]->addItem(QString::number(resVector[maxAreaI].w) + "x" + QString::number(resVector[maxAreaI].h), maxAreaI);
		
		const int newIndex = fullResSelector[j]->findText(oldtext);
		
		if (newIndex >= 0) {
			fullResSelector[j]->setCurrentIndex(newIndex);
			hzSelector[j]->setCurrentIndex(oldHzIndex);
		}
	}
}

void VideoDialog::store() {
// 	for (std::size_t i = 0; i < mw->numBlitters(); ++i)
// 		mw->blitterConf(i).acceptSettings();
	
	engineIndex = engineSelector->currentIndex();
	
	if (unrestrictedScalingButton->isChecked())
		scaling = UNRESTRICTED;
	else if (keepRatioButton->isChecked())
		scaling = KEEP_RATIO;
	else
		scaling = INTEGER;
	
	sourceIndexStore = sourceSelector->currentIndex();
	winIndex = winResSelector->currentIndex();
	
	for (unsigned i = 0; i < fullResSelector.size(); ++i) {
		fullIndex[i] = fullResSelector[i]->currentIndex();
		hzIndex[i] = hzSelector[i]->currentIndex();
	}
}

void VideoDialog::restore() {
	for (std::size_t i = 0; i < mw->numBlitters(); ++i)
		mw->blitterConf(i).rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	
	switch (scaling) {
	case UNRESTRICTED: unrestrictedScalingButton->click(); break;
	case KEEP_RATIO: keepRatioButton->click(); break;
	case INTEGER: integerScalingButton->click(); break;
	}
	
	sourceSelector->setCurrentIndex(sourceIndexStore);
	winResSelector->setCurrentIndex(winIndex);
	
	for (unsigned i = 0; i < fullResSelector.size(); ++i) {
		fullResSelector[i]->setCurrentIndex(fullIndex[i]);
		hzSelector[i]->setCurrentIndex(hzIndex[i]);
	}
}

void VideoDialog::engineChange(int index) {
	if (engineWidget) {
		topLayout->removeWidget(engineWidget);
		engineWidget->setParent(NULL);
	}
	
	engineWidget = mw->blitterConf(index).settingsWidget();
	
	if (engineWidget)
		topLayout->insertWidget(1, engineWidget);
	
	/*if (mw->blitterConf(index)->integerOnlyScaler) {
		integerScalingButton->click();
		keepRatioButton->setEnabled(false);
		integerScalingButton->setEnabled(false);
		unrestrictedScalingButton->setEnabled(false);
	} else */{
		keepRatioButton->setEnabled(true);
		integerScalingButton->setEnabled(true);
		unrestrictedScalingButton->setEnabled(true);
	}
}

void VideoDialog::fullresChange(int index) {
	for (unsigned i = 0; i < fullResSelector.size(); ++i) {
		if (sender() == fullResSelector[i]) {
			hzSelector[i]->clear();
		
			if (index >= 0) {
				const std::vector<short> &v = mw->modeVector(i)[index].rates;
				
				for (unsigned int j = 0; j < v.size(); ++j)
					hzSelector[i]->addItem(QString::number(v[j]) + QString(" Hz"), j);
			}
		}
	}
}

void VideoDialog::sourceChange(int /*index*/) {
	fillWinResSelector();
	fillFullResSelector();
}

void VideoDialog::integerScalingChange(bool /*checked*/) {
	sourceChange(sourceSelector->currentIndex());
}

int VideoDialog::blitterNo() const {
	return engineIndex;
}

const QSize VideoDialog::windowSize() const {
	return winResSelector->itemData(winIndex).toSize();
}

unsigned VideoDialog::fullResIndex(unsigned screen) const {
	//return fullResSelector->currentIndex();
// 	return fullResSelectorBackup->findText(fullResSelector->itemText(fullResSelector->currentIndex()));
	return fullResSelector[screen]->itemData(fullIndex[screen]).toUInt();
}

unsigned VideoDialog::fullRateIndex(unsigned screen) const {
	return hzSelector[screen]->itemData(hzIndex[screen]).toUInt();
}

const QSize VideoDialog::sourceSize() const {
	return sourceSelector->itemData(sourceIndex()).toSize();
}

void VideoDialog::setAspectRatio(const QSize &aspectRatio) {
	restore();
	aspRatio = aspectRatio;
	fillWinResSelector();
	store();
	emit accepted();
}

void VideoDialog::fillSourceSelector(const std::vector<VideoSourceInfo> &sourceInfos) {
	for (std::size_t i = 0; i < sourceInfos.size(); ++i)
		sourceSelector->addItem(sourceInfos[i].label, QSize(sourceInfos[i].width, sourceInfos[i].height));
	
	if (sourceSelector->count() < 2) {
		sourceSelector->hide();
		sourceSelectorLabel->hide();
	} else {
		sourceSelector->show();
		sourceSelectorLabel->show();
	}
}

void VideoDialog::setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos) {
	restore();
	disconnect(sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	sourceSelector->clear();
	
	fillSourceSelector(sourceInfos);
	
	connect(sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	sourceChange(sourceSelector->currentIndex());
	store();
	emit accepted();
}

void VideoDialog::accept() {
	store();
	QDialog::accept();
}

void VideoDialog::reject() {
	restore();
	QDialog::reject();
}

void applySettings(MainWindow *const mw, const VideoDialog *const vd) {
	{
		const BlitterConf curBlitter = mw->currentBlitterConf();
		
		for (std::size_t i = 0; i < mw->numBlitters(); ++i)
			if (mw->blitterConf(i) != curBlitter)
				mw->blitterConf(i).acceptSettings();
		
		const QSize &srcSz = vd->sourceSize();
		mw->setVideoFormatAndBlitter(srcSz.width(), srcSz.height(), vd->blitterNo());
		curBlitter.acceptSettings();
	}
	
	mw->setAspectRatio(vd->aspectRatio());
	mw->setScalingMethod(vd->scalingMethod());
	mw->setWindowSize(vd->windowSize());
	
	for (unsigned i = 0; i < mw->screens(); ++i)
		mw->setFullScreenMode(i, vd->fullResIndex(i), vd->fullRateIndex(i));
}
