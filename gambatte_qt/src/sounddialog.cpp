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
#include "sounddialog.h"

#include <QComboBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSize>
#include <QInputDialog>
#include <cassert>

#include "audioengine.h"

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

static void populateRateSelector(QComboBox *rateSelector, const MediaSource::SampleRateInfo &rateInfo) {
	assert(!rateInfo.rates.empty());
	assert(rateInfo.defaultRateIndex < rateInfo.rates.size());
	
	for (std::size_t i = 0; i < rateInfo.rates.size(); ++i)
		rateSelector->addItem(QString::number(rateInfo.rates[i]) + " Hz", rateInfo.rates[i]);
	
	if (rateInfo.minCustomRate > 0 && rateInfo.maxCustomRate > rateInfo.minCustomRate) {
		rateSelector->addItem(SoundDialog::tr("Other..."), QSize(rateInfo.minCustomRate, rateInfo.maxCustomRate));
	}
	
	rateSelector->setCurrentIndex(rateInfo.defaultRateIndex);
}

static int getCustomIndex(const QComboBox *rateSelector) {
	int i = rateSelector->count() - 2;
	
	while (i < rateSelector->count() && rateSelector->itemText(i).at(0).isNumber())
		++i;
	
	return i;
}

static void setRate(QComboBox *rateSelector, const int r) {
	const int customIndex = getCustomIndex(rateSelector);
	const int newIndex = rateSelector->findData(r);
	
	if (newIndex < 0) {
		if (customIndex < rateSelector->count()) {
			rateSelector->addItem(QString::number(r) + " Hz", r);
			
			if (customIndex + 2 != rateSelector->count())
				rateSelector->removeItem(customIndex + 1);
			
			rateSelector->setCurrentIndex(customIndex + 1);
		}
	} else
		rateSelector->setCurrentIndex(newIndex);
}

SoundDialog::SoundDialog(const std::vector<AudioEngine*> &engines, const MediaSource::SampleRateInfo &rateInfo, QWidget *parent) :
	QDialog(parent),
	engines(engines),
	topLayout(new QVBoxLayout),
	engineSelector(new QComboBox(this)),
	rateSelector(new QComboBox(this)),
	latencySelector(new QSpinBox(this)),
	engineWidget(NULL)
{
	setWindowTitle(tr("Sound Settings"));
	
	QVBoxLayout *const mainLayout = new QVBoxLayout;
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Sound engine:")));
	
		for (std::size_t i = 0; i < engines.size(); ++i)
			engineSelector->addItem(engines[i]->nameString);
		
		hLayout->addWidget(engineSelector);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Sample rate:")));
		
		populateRateSelector(rateSelector, rateInfo);
		
		hLayout->addWidget(rateSelector);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Buffer latency:")));
		latencySelector->setRange(4, 999);
		latencySelector->setSuffix(" ms");
		hLayout->addWidget(latencySelector);
		topLayout->addLayout(hLayout);
	}
	
	mainLayout->addLayout(topLayout);
	
	{
		QPushButton *const okButton = new QPushButton(tr("OK"), this);
		QPushButton *const cancelButton = new QPushButton(tr("Cancel"), this);
		QHBoxLayout *const hLayout = new QHBoxLayout;
		
		hLayout->addWidget(okButton);
		hLayout->addWidget(cancelButton);

		okButton->setDefault(true);
		
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
		
		mainLayout->addLayout(hLayout);
		mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	}
	
	setLayout(mainLayout);
	
	QSettings settings;
	settings.beginGroup("sound");
	engineIndex = filterValue(settings.value("engineIndex", 0).toInt(), engineSelector->count());
	setRate(rateSelector, settings.value("rate", rateSelector->itemData(rateSelector->currentIndex())).toInt());
	latency = filterValue(settings.value("latency", 67).toInt(), latencySelector->maximum(), latencySelector->minimum(), 67);
	settings.endGroup();
	
	rate = rateSelector->itemData(rateSelector->currentIndex()).toInt();
	engineChange(engineSelector->currentIndex());
	connect(engineSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChange(int)));
	connect(rateSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(rateIndexChange(int)));
	
	restore();
}

SoundDialog::~SoundDialog() {
	QSettings settings;
	settings.beginGroup("sound");
	settings.setValue("engineIndex", engineIndex);
	settings.setValue("rate", rate);
	settings.setValue("latency", latency);
	settings.endGroup();
}

void SoundDialog::engineChange(int index) {
	if (engineWidget) {
		topLayout->removeWidget(engineWidget);
		engineWidget->setParent(NULL);
	}
	
	if ((engineWidget = engines[index]->settingsWidget()))
		topLayout->insertWidget(1, engineWidget);
}

void SoundDialog::rateIndexChange(const int index) {
	if (getCustomIndex(rateSelector) == index) {
		const QSize &sz = rateSelector->itemData(index).toSize();
		const int currentRate = getRate();
		bool ok = false;
		
		int r = QInputDialog::getInteger(this, tr("Set Sample Rate"), tr("Sample rate (Hz):"), getRate(), sz.width(), sz.height(), 1, &ok);
		
		if (!ok)
			r = currentRate;
		
		setRate(rateSelector, r);
	}
}

void SoundDialog::store() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->acceptSettings();
	
	engineIndex = engineSelector->currentIndex();
	rate = rateSelector->itemData(rateSelector->currentIndex()).toInt();
	latency = latencySelector->value();
}

void SoundDialog::restore() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	setRate(rateSelector, rate);
	latencySelector->setValue(latency);
}

void SoundDialog::setRates(const MediaSource::SampleRateInfo &rateInfo) {
	restore();
	
	rateSelector->clear();
	populateRateSelector(rateSelector, rateInfo);
	setRate(rateSelector, rate);
	
	store();
	emit accepted();
}

void SoundDialog::accept() {
	store();
	QDialog::accept();
}

void SoundDialog::reject() {
	restore();
	QDialog::reject();
}
