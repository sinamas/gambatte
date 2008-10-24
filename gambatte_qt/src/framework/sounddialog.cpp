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
#include "sounddialog.h"
#include <resample/resamplerinfo.h>
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

struct SampleRateInfo {
	enum { NOT_SUPPORTED = -1 };
	
	// Distinct sample rate (stereo samples per second) alternatives selectable in the sound settings dialog.
	std::vector<int> rates;
	
	// The index of the rate in the rates list to be selected by default.
	std::size_t defaultRateIndex;
	
	// Minimum and maximum custom sample rates selectable in the sound settings dialog.
	// Set to NOT_SUPPORTED if you don't want to allow custom sample rates.
	int minCustomRate;
	int maxCustomRate;
};

static SampleRateInfo generateSampleRateInfo() {
	SampleRateInfo srinfo;
	
	srinfo.rates.push_back(48000);
	srinfo.rates.push_back(44100);
	srinfo.defaultRateIndex = 0;
	srinfo.minCustomRate = 8000;
	srinfo.maxCustomRate = 192000;
	
#ifdef Q_WS_MAC
	srinfo.defaultRateIndex = 1;
#endif
	
	return srinfo;
}

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

static void populateRateSelector(QComboBox *rateSelector, const SampleRateInfo &rateInfo) {
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

SoundDialog::SoundDialog(const std::vector<AudioEngine*> &engines, QWidget *parent) :
	QDialog(parent),
	engines(engines),
	topLayout(new QVBoxLayout),
	engineSelector(new QComboBox(this)),
	resamplerSelector(new QComboBox(this)),
	rateSelector(new QComboBox(this)),
	latencySelector(new QSpinBox(this)),
	engineWidget(NULL),
	engineIndex(0),
	resamplerNum(1),
	rate(0),
	latency(68)
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
		hLayout->addWidget(new QLabel(tr("Resampler:")));
		
		for (unsigned i = 0; i < ResamplerInfo::num(); ++i)
			resamplerSelector->addItem(ResamplerInfo::get(i).desc);
		
		hLayout->addWidget(resamplerSelector);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Sample rate:")));
		
		populateRateSelector(rateSelector, generateSampleRateInfo());
		
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
	engineIndex = filterValue(settings.value("engineIndex", engineIndex).toInt(), engineSelector->count());
	resamplerNum = filterValue(settings.value("resamplerNum", resamplerNum).toInt(), resamplerSelector->count(), 0, resamplerNum);
	setRate(rateSelector, settings.value("rate", rateSelector->itemData(rateSelector->currentIndex())).toInt());
	latency = filterValue(settings.value("latency", latency).toInt(), latencySelector->maximum(), latencySelector->minimum(), latency);
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
	settings.setValue("resamplerNum", resamplerNum);
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
	resamplerNum = resamplerSelector->currentIndex();
	rate = rateSelector->itemData(rateSelector->currentIndex()).toInt();
	latency = latencySelector->value();
}

void SoundDialog::restore() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	resamplerSelector->setCurrentIndex(resamplerNum);
	setRate(rateSelector, rate);
	latencySelector->setValue(latency);
}

void SoundDialog::accept() {
	store();
	QDialog::accept();
}

void SoundDialog::reject() {
	restore();
	QDialog::reject();
}
