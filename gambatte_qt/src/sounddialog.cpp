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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>

#include "audioengine.h"

static int filterValue(const int value, const int upper, const int lower = 0, const int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;
	
	return value;
}

SoundDialog::SoundDialog(const std::vector<AudioEngine*> &engines, QWidget *parent) :
	QDialog(parent),
	engines(engines),
	topLayout(new QVBoxLayout),
	engineSelector(new QComboBox(this)),
	rateSelector(new QComboBox(this)),
	engineWidget(NULL)
{
	setWindowTitle(tr("Sound settings"));
	
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
		rateSelector->addItem("48000 Hz", 48000);
		rateSelector->addItem("44100 Hz", 44100);
		hLayout->addWidget(rateSelector);
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
}

void SoundDialog::restore() {
	for (std::size_t i = 0; i < engines.size(); ++i)
		engines[i]->rejectSettings();
	
	engineSelector->setCurrentIndex(engineIndex);
	rateSelector->setCurrentIndex(rateIndex);
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
