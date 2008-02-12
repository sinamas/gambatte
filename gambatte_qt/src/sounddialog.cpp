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
#include <cassert>

#include "audioengine.h"

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

SoundDialog::SoundDialog(const std::vector<AudioEngine*> &engines, const std::vector<int> &rates, QWidget *parent) :
	QDialog(parent),
	engines(engines),
	topLayout(new QVBoxLayout),
	engineSelector(new QComboBox(this)),
	rateSelector(new QComboBox(this)),
	latencySelector(new QSpinBox(this)),
	engineWidget(NULL)
{
	assert(!rates.empty());
	
	setWindowTitle(tr("Sound Settings"));
	
	QVBoxLayout *const mainLayout = new QVBoxLayout;
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Sound Engine:")));
	
		for (std::size_t i = 0; i < engines.size(); ++i)
			engineSelector->addItem(engines[i]->nameString);
		
		hLayout->addWidget(engineSelector);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Sample Rate:")));
		
		for (unsigned i = 0; i < rates.size(); ++i)
			rateSelector->addItem(QString::number(rates[i]) + " Hz", rates[i]);
		
		hLayout->addWidget(rateSelector);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Buffer Latency:")));
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
	rateIndex = filterValue(settings.value("rateIndex", 0).toInt(), rateSelector->count());
	latency = filterValue(settings.value("latency", 133).toInt(), latencySelector->maximum(), latencySelector->minimum());
	settings.endGroup();
	
	engineChange(engineSelector->currentIndex());
	connect(engineSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(engineChange(int)));
	
	restore();
}

SoundDialog::~SoundDialog() {
	QSettings settings;
	settings.beginGroup("sound");
	settings.setValue("engineIndex", engineIndex);
	settings.setValue("rateIndex", rateIndex);
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

void SoundDialog::store() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->acceptSettings();
	
	engineIndex = engineSelector->currentIndex();
	rateIndex = rateSelector->currentIndex();
	latency = latencySelector->value();
}

void SoundDialog::restore() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	rateSelector->setCurrentIndex(rateIndex);
	latencySelector->setValue(latency);
}

void SoundDialog::setRates(const std::vector<int> &rates) {
	restore();
	
	const int oldVal = rateSelector->itemData(rateIndex).toInt();
	rateSelector->clear();
	
	for (unsigned i = 0; i < rates.size(); ++i)
		rateSelector->addItem(QString::number(rates[i]) + " Hz", rates[i]);
	
	const int newIndex = rateSelector->findData(oldVal);
	
	if (newIndex >= 0)
		rateSelector->setCurrentIndex(newIndex);
	
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

int SoundDialog::getRate() const {
	return rateSelector->itemData(rateIndex).toInt();
}
