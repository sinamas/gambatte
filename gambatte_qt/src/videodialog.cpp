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

#include <filterinfo.h>
#include "blitterwidget.h"

#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSettings>

#include <iostream>

#include "fullrestoggler.h"

VideoDialog::VideoDialog(const std::vector<BlitterWidget*> &blitters, std::vector<const FilterInfo*> filterInfo, const FullResToggler& resHandler, QWidget *parent) :
QDialog(parent),
engines(blitters),
resVector(resHandler.resVector()),
engineIndex(0),
winIndex(0),
fullIndex(0),
hzIndex(0),
filterIndexStore(0),
keepRatio(true),
integerScaling(false)
{
	ResInfo currentRes;
	if (resVector.empty()) {
		currentRes.w = 1600;
		currentRes.h = 1200;
	} else
		currentRes = resVector.at(resHandler.currentResIndex());
	

	std::cout << currentRes.w << "x" << currentRes.h << std::endl;

	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	QLabel *winResLabel = new QLabel(QString(tr("Windowed resolution:")));

	winResSelector = new QComboBox;
	winResSelectorBackup = new QComboBox;
	
	{
		unsigned hres = 160;
		unsigned vres = 144;
		
		while (hres <= currentRes.w && vres <= currentRes.h) {
			winResSelector->addItem(QString::number(hres) + QString("x") + QString::number(vres), QVariant(QSize(hres, vres)));
			winResSelectorBackup->addItem(QString::number(hres) + QString("x") + QString::number(vres), QVariant(QSize(hres, vres)));
			hres += 160;
			vres += 144;
		}
	}
	
	winResSelector->addItem(QString(tr("Variable")), QVariant(QSize(-1, -1)));
// 	winResSelectorBackup->addItem(QString(tr("Variable")), QVariant(QSize(-1, -1)));

	QLabel *fullResLabel = new QLabel(QString(tr("Full screen resolution:")));
	fullResSelector = new QComboBox;
// 	fullResSelectorBackup = new QComboBox;
	hzSelector = new QComboBox;
	
	fillFullResSelector(QSize(160, 144));
	
// 	fullResSelector->setCurrentIndex(fullIndex);
	
	for (unsigned int i = 0; i < currentRes.rates.size(); ++i)
		hzSelector->addItem(QString::number(currentRes.rates[i]) + QString(" Hz"), i);

	QPushButton *okButton = new QPushButton(tr("OK"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));

	topLayout = new QVBoxLayout;

	QHBoxLayout *hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(tr("Video engine:")));
	engineSelector = new QComboBox;
	
	for (unsigned int i = 0; i < engines.size(); ++i) {
		engineSelector->addItem(engines[i]->nameString);
	}
	
	hLayout->addWidget(engineSelector);
	topLayout->addLayout(hLayout);
	engineWidget = engines[0]->settingsWidget();
	
	if (engineWidget)
		topLayout->addWidget(engineWidget);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(winResLabel);
	hLayout->addWidget(winResSelector);
	topLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(fullResLabel);
	QHBoxLayout *hhLayout = new QHBoxLayout;
	hhLayout->addWidget(fullResSelector);
	hhLayout->addWidget(hzSelector);
	hLayout->addLayout(hhLayout);
	topLayout->addLayout(hLayout);

	keepRatioBox = new QCheckBox(QString("Keep aspect ratio"));
	topLayout->addWidget(keepRatioBox);

	integerScalingBox = new QCheckBox(QString("Only scale by integer factors"));
	topLayout->addWidget(integerScalingBox);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(tr("Video filter:")));
	filterSelector = new QComboBox;
	
	{
		unsigned maxW = 0;
		unsigned maxH = 0;
		
		for (unsigned i = 0, max = 0; i < resVector.size(); ++i) {
			const unsigned cur = std::min(resVector[i].w * 9, resVector[i].h * 10);
			
			if (cur > max) {
				max = cur;
				maxW = resVector[i].w;
				maxH = resVector[i].h;
			}
		}
		
		if (!maxW) {
			maxW = 1600;
			maxH = 1200;
		}
		
		for (unsigned int i = 0; i < filterInfo.size(); ++i) {
			if (filterInfo[i]->outWidth <= maxW && filterInfo[i]->outHeight <= maxH)
				filterSelector->addItem(filterInfo[i]->handle.c_str(), QSize(filterInfo[i]->outWidth, filterInfo[i]->outHeight));
		}
	}
	
	hLayout->addWidget(filterSelector);
	topLayout->addLayout(hLayout);

	mainLayout->addLayout(topLayout);
	mainLayout->setAlignment(topLayout, Qt::AlignTop);

	hLayout = new QHBoxLayout;
	hLayout->addWidget(okButton);
	hLayout->addWidget(cancelButton);
	mainLayout->addLayout(hLayout);
	mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	okButton->setDefault(true);

	connect(engineSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChange(int)));
	connect(fullResSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(fullresChange(int)));
	connect(keepRatioBox, SIGNAL(toggled(bool)), this, SLOT(keepRatioChange(bool)));
	connect(integerScalingBox, SIGNAL(toggled(bool)), this, SLOT(integerScalingChange(bool)));
	connect(filterSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(filterChange(int)));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
// 	connect(this, SIGNAL(accepted()), this, SLOT(store()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
// 	connect(this, SIGNAL(rejected()), this, SLOT(restore()));
	
	QSettings settings;
	settings.beginGroup("video");
	
	if (static_cast<unsigned>(fullIndex = settings.value("fullIndex", resHandler.currentResIndex()).toInt()) >= static_cast<unsigned>(fullResSelector->count()))
		fullIndex = resHandler.currentResIndex();
	
	if (static_cast<unsigned>(filterIndexStore = settings.value("filterIndexStore", 0).toInt()) >= static_cast<unsigned>(filterSelector->count()))
		filterIndexStore = 0;
	
	if (static_cast<unsigned>(engineIndex = settings.value("engineIndex", 0).toInt()) >= static_cast<unsigned>(engineSelector->count()))
		engineIndex = 0;
	
	restore();
	store();
	
	if (static_cast<unsigned>(winIndex = settings.value("winIndex", winResSelector->count() - 1).toInt()) >= static_cast<unsigned>(winResSelector->count()))
		winIndex = winResSelector->count() - 1;
	
	if (static_cast<unsigned>(hzIndex = settings.value("hzIndex", resHandler.currentRateIndex()).toInt()) >= static_cast<unsigned>(hzSelector->count()))
		hzIndex = resHandler.currentRateIndex();
	
	keepRatio = settings.value("keepRatio", true).toBool();
	integerScaling = settings.value("integerScaling", false).toBool();
	
	restore();
	engineChange(engineIndex);
	store();
	
	settings.endGroup();

	setWindowTitle(tr("Video settings"));
}

VideoDialog::~VideoDialog() {
	delete winResSelectorBackup;
// 	delete fullResSelectorBackup;
	
	QSettings settings;
	settings.beginGroup("video");
	settings.setValue("fullIndex", fullIndex);
	settings.setValue("winIndex", winIndex);
	settings.setValue("hzIndex", hzIndex);
	settings.setValue("keepRatio", keepRatio);
	settings.setValue("integerScaling", integerScaling);
	settings.setValue("filterIndexStore", filterIndexStore);
	settings.setValue("engineIndex", engineIndex);
	settings.endGroup();
}

void VideoDialog::fillFullResSelector(const QSize &minimumRes) {
	while (fullResSelector->count())
		fullResSelector->removeItem(0);
	
	for (std::size_t i = 0; i < resVector.size(); ++i) {
		const int hres = resVector[i].w;
		const int vres = resVector[i].h;
		
		if (hres >= minimumRes.width() && vres >= minimumRes.height())
			fullResSelector->addItem(QString::number(hres) + QString("x") + QString::number(vres), (uint)i);
	}
}

void VideoDialog::store() {
	engineIndex = engineSelector->currentIndex();
	
	for (unsigned int i = 0; i < engines.size(); ++i)
		engines[i]->acceptSettings();
	
	hzIndex = hzSelector->currentIndex();
	winIndex = winResSelector->currentIndex();
	fullIndex = fullResSelector->currentIndex();
	keepRatio = keepRatioBox->checkState() != Qt::Unchecked;
	integerScaling = integerScalingBox->checkState() != Qt::Unchecked;
	filterIndexStore = filterSelector->currentIndex();
}

void VideoDialog::restore() {
	keepRatioBox->setChecked(keepRatio);
	integerScalingBox->setChecked(integerScaling);
	filterSelector->setCurrentIndex(filterIndexStore);
	winResSelector->setCurrentIndex(winIndex);
	fullResSelector->setCurrentIndex(fullIndex);
	hzSelector->setCurrentIndex(hzIndex);
	
	for (unsigned int i = 0;i < engines.size();++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
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
		keepRatioBox->setChecked(true);
		integerScalingBox->setChecked(true);
		keepRatioBox->setEnabled(false);
		integerScalingBox->setEnabled(false);
	} else {
		keepRatioBox->setEnabled(true);
		
		if (keepRatioBox->checkState() != Qt::Unchecked)
			integerScalingBox->setEnabled(true);
	}
}

void VideoDialog::fullresChange(int index) {
	while (hzSelector->count())
		hzSelector->removeItem(0);

	if (index >= 0) {
		const std::vector<short> &v = resVector[index].rates;
		
		for (unsigned int i = 0; i < v.size(); ++i)
			hzSelector->addItem(QString::number(v[i]) + QString(" Hz"), i);
	}
}

void VideoDialog::filterChange(int index) {
	const QSize filterSize = filterSelector->itemData(index).toSize();

	{
		QString oldtext = winResSelector->itemText(winResSelector->currentIndex());
		
		while (winResSelector->count())
			winResSelector->removeItem(0);

		if (integerScalingBox->checkState() != Qt::Unchecked) {
			QSize fSize = filterSize;
			
			for (int i = 0;i < winResSelectorBackup->count();++i) {
				QSize s = winResSelectorBackup->itemData(i).toSize();
				
				if (s == fSize) {
					winResSelector->addItem(winResSelectorBackup->itemText(i), s);
					fSize += filterSize;
				}
			}
		} else {
			for (int i = 0;i < winResSelectorBackup->count();++i) {
				QSize s = winResSelectorBackup->itemData(i).toSize();
				
				if (s.width() >= filterSize.width() && s.height() >= filterSize.height())
					winResSelector->addItem(winResSelectorBackup->itemText(i), s);
			}
		}
		winResSelector->addItem(QString(tr("Variable")), QVariant(QSize(-1, -1)));
		int newIndex = winResSelector->findText(oldtext);
		
		if (newIndex >= 0)
			winResSelector->setCurrentIndex(newIndex);
	}

	{
		QString oldtext = fullResSelector->itemText(fullResSelector->currentIndex());
		int oldHzIndex = hzSelector->currentIndex();
		
		fillFullResSelector(filterSize);
		
		int newIndex = fullResSelector->findText(oldtext);
		
		if (newIndex >= 0) {
			fullResSelector->setCurrentIndex(newIndex);
			hzSelector->setCurrentIndex(oldHzIndex);
		}
	}
}

void VideoDialog::keepRatioChange(bool checked) {
	if (integerScalingBox->checkState() != Qt::Unchecked)
		integerScalingBox->setChecked(checked);
	
	integerScalingBox->setEnabled(checked);
}

void VideoDialog::integerScalingChange(bool /*checked*/) {
	filterChange(filterSelector->currentIndex());
}

const int VideoDialog::engine() const {
	return engineSelector->currentIndex();
}

const QSize VideoDialog::winRes() const {
	return winResSelector->itemData(winResSelector->currentIndex()).toSize();
}

const unsigned VideoDialog::fullMode() const {
	//return fullResSelector->currentIndex();
// 	return fullResSelectorBackup->findText(fullResSelector->itemText(fullResSelector->currentIndex()));
	return fullResSelector->itemData(fullResSelector->currentIndex()).toUInt();
}

const unsigned VideoDialog::fullRate() const {
	return hzSelector->itemData(hzSelector->currentIndex()).toUInt();
}

bool VideoDialog::keepsRatio() const {
	return keepRatio;
}

bool VideoDialog::scalesByInteger() const {
	return integerScaling;
}

const unsigned int VideoDialog::filterIndex() const {
	return filterSelector->currentIndex();
}

void VideoDialog::accept() {
	store();
	QDialog::accept();
}

void VideoDialog::reject() {
	restore();
	QDialog::reject();
}
