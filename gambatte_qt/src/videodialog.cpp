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
#include "videodialog.h"

#include "blitterwidget.h"

#include <QPushButton>
#include <QComboBox>
#include <QRadioButton>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QGroupBox>

// #include <iostream>

#include "fullrestoggler.h"

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

VideoDialog::VideoDialog(const std::vector<BlitterWidget*> &blitters,
                         const std::vector<MediaSource::VideoSourceInfo> &sourceInfos,
                         const std::string &sourcesLabel,
                         const FullResToggler& resHandler,
                         const QSize &aspectRatio,
                         QWidget *parent) :
QDialog(parent),
engines(blitters),
resVector(resHandler.resVector()),
topLayout(new QVBoxLayout),
engineWidget(NULL),
engineSelector(new QComboBox),
winResSelector(new QComboBox),
fullResSelector(new QComboBox),
hzSelector(new QComboBox),
unrestrictedScalingButton(new QRadioButton(QString("None"))),
keepRatioButton(new QRadioButton(QString("Keep aspect ratio"))),
integerScalingButton(new QRadioButton(QString("Only scale by integer factors"))),
sourceSelector(new QComboBox),
scaling(KEEP_RATIO),
aspRatio(aspectRatio),
defaultRes(0),
engineIndex(0),
winIndex(0),
fullIndex(0),
hzIndex(0),
sourceIndexStore(0)
{
	defaultRes = resHandler.currentResIndex();

	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(tr("Video Engine:")));
	hLayout->addWidget(engineSelector);
	topLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(QString(tr("Windowed resolution:"))));
	hLayout->addWidget(winResSelector);
	topLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(QString(tr("Full Screen mode:"))));
	QHBoxLayout *hhLayout = new QHBoxLayout;
	hhLayout->addWidget(fullResSelector);
	hhLayout->addWidget(hzSelector);
	hLayout->addLayout(hhLayout);
	topLayout->addLayout(hLayout);

	{
		QGroupBox *f = new QGroupBox("Scaling restrictions");
		f->setLayout(new QVBoxLayout);
		f->layout()->addWidget(unrestrictedScalingButton);
		f->layout()->addWidget(keepRatioButton);
		f->layout()->addWidget(integerScalingButton);
		topLayout->addWidget(f);
	}

	hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(sourcesLabel.c_str()));
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
	connect(fullResSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(fullresChange(int)));
	connect(integerScalingButton, SIGNAL(toggled(bool)), this, SLOT(integerScalingChange(bool)));
	connect(sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	
	for (std::size_t i = 0; i < sourceInfos.size(); ++i) {
		sourceSelector->addItem(sourceInfos[i].handle.c_str(), QSize(sourceInfos[i].width, sourceInfos[i].height));
	}
	
	for (std::size_t i = 0; i < engines.size(); ++i) {
		engineSelector->addItem(engines[i]->nameString);
	}
	
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
	
	fullIndex = filterValue(fullResSelector->findText(settings.value("fullRes").toString()), fullResSelector->count(), 0, resHandler.currentResIndex());
	winIndex = filterValue(settings.value("winIndex", winResSelector->count() - 1).toInt(), winResSelector->count(), 0, winResSelector->count() - 1);
	
	restore();
	
	hzIndex = filterValue(hzSelector->findData(settings.value("hz").toUInt()), hzSelector->count(), 0, resHandler.currentRateIndex());
	
	restore();
	store();
	
	settings.endGroup();

	setWindowTitle(tr("Video Settings"));
}

VideoDialog::~VideoDialog() {
// 	delete fullResSelectorBackup;
	
	QSettings settings;
	settings.beginGroup("video");
	settings.setValue("fullRes", fullResSelector->itemText(fullIndex));
	settings.setValue("winIndex", winIndex);
	settings.setValue("hz", fullRate());
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
	
	const int maxW = resVector.empty() ? 1600 : resVector[defaultRes].w;
	const int maxH = resVector.empty() ? 1200 : resVector[defaultRes].h;
	
	while (sz.width() <= maxW && sz.height() <= maxH) {
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
	const QString oldtext(fullResSelector->itemText(fullResSelector->currentIndex()));
	const int oldHzIndex = hzSelector->currentIndex();
	
	fullResSelector->clear();
	
	const QSize &sourceSize = sourceSelector->itemData(sourceSelector->currentIndex()).toSize();
	
	unsigned maxArea = 0;
	unsigned maxAreaI = 0;
	
	for (unsigned i = 0; i < resVector.size(); ++i) {
		const int hres = resVector[i].w;
		const int vres = resVector[i].h;
		
		if (hres >= sourceSize.width() && vres >= sourceSize.height()) {
			fullResSelector->addItem(QString::number(hres) + QString("x") + QString::number(vres), i);
		} else {
			const unsigned area = std::min(hres, sourceSize.width()) * std::min(vres, sourceSize.height());
			
			if (area > maxArea) {
				maxArea = area;
				maxAreaI = i;
			}
		}
	}
	
	//add resolution giving maximal area if all resolutions are too small.
	if (fullResSelector->count() < 1 && maxArea)
		fullResSelector->addItem(QString::number(resVector[maxAreaI].w) + "x" + QString::number(resVector[maxAreaI].h), maxAreaI);
	
	const int newIndex = fullResSelector->findText(oldtext);
	
	if (newIndex >= 0) {
		fullResSelector->setCurrentIndex(newIndex);
		hzSelector->setCurrentIndex(oldHzIndex);
	}
}

void VideoDialog::store() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->acceptSettings();
	
	engineIndex = engineSelector->currentIndex();
	
	if (unrestrictedScalingButton->isChecked())
		scaling = UNRESTRICTED;
	else if (keepRatioButton->isChecked())
		scaling = KEEP_RATIO;
	else
		scaling = INTEGER;
	
	sourceIndexStore = sourceSelector->currentIndex();
	winIndex = winResSelector->currentIndex();
	fullIndex = fullResSelector->currentIndex();
	hzIndex = hzSelector->currentIndex();
}

void VideoDialog::restore() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	
	switch (scaling) {
	case UNRESTRICTED: unrestrictedScalingButton->click(); break;
	case KEEP_RATIO: keepRatioButton->click(); break;
	case INTEGER: integerScalingButton->click(); break;
	}
	
	sourceSelector->setCurrentIndex(sourceIndexStore);
	winResSelector->setCurrentIndex(winIndex);
	fullResSelector->setCurrentIndex(fullIndex);
	hzSelector->setCurrentIndex(hzIndex);
}

void VideoDialog::engineChange(int index) {
	if (engineWidget) {
		topLayout->removeWidget(engineWidget);
		engineWidget->setParent(NULL);
	}
	
	engineWidget = engines[index]->settingsWidget();
	
	if (engineWidget)
		topLayout->insertWidget(1, engineWidget);
	
	if (engines[index]->integerOnlyScaler) {
		integerScalingButton->click();
		keepRatioButton->setEnabled(false);
		integerScalingButton->setEnabled(false);
		unrestrictedScalingButton->setEnabled(false);
	} else {
		keepRatioButton->setEnabled(true);
		integerScalingButton->setEnabled(true);
		unrestrictedScalingButton->setEnabled(true);
	}
}

void VideoDialog::fullresChange(int index) {
	hzSelector->clear();

	if (index >= 0) {
		const std::vector<short> &v = resVector[index].rates;
		
		for (unsigned int i = 0; i < v.size(); ++i)
			hzSelector->addItem(QString::number(v[i]) + QString(" Hz"), i);
	}
}

void VideoDialog::sourceChange(int /*index*/) {
	fillWinResSelector();
	fillFullResSelector();
}

void VideoDialog::integerScalingChange(bool /*checked*/) {
	sourceChange(sourceSelector->currentIndex());
}

int VideoDialog::engine() const {
	return engineIndex;
}

const QSize VideoDialog::winRes() const {
	return winResSelector->itemData(winIndex).toSize();
}

unsigned VideoDialog::fullMode() const {
	//return fullResSelector->currentIndex();
// 	return fullResSelectorBackup->findText(fullResSelector->itemText(fullResSelector->currentIndex()));
	return fullResSelector->itemData(fullIndex).toUInt();
}

unsigned VideoDialog::fullRate() const {
	return hzSelector->itemData(hzIndex).toUInt();
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

void VideoDialog::setVideoSources(const std::vector<MediaSource::VideoSourceInfo> &sourceInfos) {
	restore();
	disconnect(sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	sourceSelector->clear();
	
	for (std::size_t i = 0; i < sourceInfos.size(); ++i)
		sourceSelector->addItem(sourceInfos[i].handle.c_str(), QSize(sourceInfos[i].width, sourceInfos[i].height));
	
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
