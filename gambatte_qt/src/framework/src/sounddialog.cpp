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

#include "sounddialog.h"
#include "audioengineconf.h"
#include "mainwindow.h"
#include "resample/resamplerinfo.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSize>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

static QComboBox * createRateBox(QWidget *parent) {
	enum { custom_rate_min =   8000,
	       custom_rate_max = 192000 };
	static int const rates[] = {
#ifdef Q_WS_MAC
		44100, 48000, 96000,
#else
		48000, 44100, 96000,
#endif
	};

	QComboBox *const box = new QComboBox(parent);
	for (std::size_t i = 0; i < sizeof rates / sizeof *rates; ++i)
		box->addItem(QString::number(rates[i]) + " Hz", rates[i]);

	box->addItem(QObject::tr("Other..."),
	             QSize(custom_rate_min, custom_rate_max));
	box->setCurrentIndex(0);

	return box;
}

static int getCustomIndex(QComboBox const *rateBox) {
	int i = rateBox->count() - 2;
	while (i < rateBox->count() && rateBox->itemText(i).at(0).isNumber())
		++i;

	return i;
}

static void setRate(QComboBox *const rateBox, int const r) {
	int const customIndex = getCustomIndex(rateBox);
	int const newIndex = rateBox->findData(r);
	if (newIndex < 0) {
		if (customIndex < rateBox->count()) {
			rateBox->addItem(QString::number(r) + " Hz", r);
			if (customIndex + 2 != rateBox->count())
				rateBox->removeItem(customIndex + 1);

			rateBox->setCurrentIndex(customIndex + 1);
		}
	} else
		rateBox->setCurrentIndex(newIndex);
}

static QComboBox * createEngineBox(MainWindow const &mw, QWidget *parent) {
	QComboBox *const box = new QComboBox(parent);
	for (std::size_t i = 0, n = mw.numAudioEngines(); i < n; ++i) {
		ConstAudioEngineConf conf = mw.audioEngineConf(i);
		box->addItem(conf.nameString()
		           + (i == 0 && n > 2
		              ? " [" + QObject::tr("recommended") + ']'
		              : QString()));
		if (QWidget *w = conf.settingsWidget()) {
			w->hide();
			w->setParent(parent);
		}
	}

	return box;
}

static QComboBox * createResamplerBox(MainWindow const &mw, QWidget *parent) {
	QComboBox *const box = new QComboBox(parent);
	for (std::size_t i = 0, n = mw.numResamplers(); i < n; ++i)
		box->addItem(mw.resamplerDesc(i));

	box->setCurrentIndex(1);
	return box;
}

} // anon ns

SoundDialog::SoundDialog(MainWindow const &mw, QWidget *const parent)
: QDialog(parent)
, mw_(mw)
, engineSelector_("sound/engineIndex", createEngineBox(mw, this))
, resamplerSelector_("sound/resamplerNum", createResamplerBox(mw, this))
, rateBox_(createRateBox(this))
, latencyBox_(new QSpinBox(this))
, engineWidget_()
, rate_(0)
, latency_(68)
{
	setWindowTitle(tr("Sound Settings"));

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QVBoxLayout *const topLayout = addLayout(mainLayout, new QVBoxLayout);

	{
		QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Sound engine:")));
		hLayout->addWidget(engineSelector_.box());
	}

	{
		QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Resampler:")));
		hLayout->addWidget(resamplerSelector_.box());
	}

	{
		QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Sample rate:")));
		hLayout->addWidget(rateBox_);
	}

	{
		QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Buffer latency:")));
		latencyBox_->setRange(8, 999);
		latencyBox_->setSuffix(" ms");
		hLayout->addWidget(latencyBox_);
	}

	{
		QHBoxLayout *const hLayout = addLayout(mainLayout, new QHBoxLayout,
		                                       Qt::AlignBottom | Qt::AlignRight);
		QPushButton *const okButton = addWidget(hLayout, new QPushButton(tr("OK")));
		QPushButton *const cancelButton = addWidget(hLayout, new QPushButton(tr("Cancel")));
		okButton->setDefault(true);
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	}

	QSettings settings;
	setRate(rateBox_,
	        settings.value("sound/rate",
		               rateBox_->itemData(rateBox_->currentIndex())).toInt());
	rate_ = rateBox_->itemData(rateBox_->currentIndex()).toInt();
	latency_ = filterValue(settings.value("sound/latency", latency_).toInt(),
	                       latencyBox_->maximum() + 1, latencyBox_->minimum(), latency_);
	latencyBox_->setValue(latency_);

	engineChange(engineSelector_.index());
	connect(engineSelector_.box(), SIGNAL(currentIndexChanged(int)),
	        this, SLOT(engineChange(int)));
	connect(rateBox_, SIGNAL(currentIndexChanged(int)), this, SLOT(rateIndexChange(int)));
}

SoundDialog::~SoundDialog() {
	QSettings settings;
	settings.setValue("sound/rate", rate_);
	settings.setValue("sound/latency", latency_);
}

void SoundDialog::engineChange(int const index) {
	QBoxLayout *const topLayout = static_cast<QBoxLayout *>(layout()->itemAt(0)->layout());
	if (engineWidget_) {
		engineWidget_->hide();
		topLayout->removeWidget(engineWidget_);
	}
	if ((engineWidget_ = mw_.audioEngineConf(index).settingsWidget())) {
		topLayout->insertWidget(1, engineWidget_);
		engineWidget_->show();
	}
}

void SoundDialog::rateIndexChange(int const index) {
	if (getCustomIndex(rateBox_) == index) {
		QSize const size = rateBox_->itemData(index).toSize();
		int const currentRate = rate_;
		bool ok = false;
		int r = QInputDialog::getInteger(this, tr("Set Sample Rate"),
		                                 tr("Sample rate (Hz):"), currentRate,
		                                 size.width(), size.height(), 1, &ok);
		if (!ok)
			r = currentRate;

		setRate(rateBox_, r);
	}
}

void SoundDialog::store() {
	engineSelector_.accept();
	resamplerSelector_.accept();
	rate_ = rateBox_->itemData(rateBox_->currentIndex()).toInt();
	latency_ = latencyBox_->value();
}

void SoundDialog::restore() {
	for (std::size_t i = 0, n = mw_.numAudioEngines(); i < n; ++i)
		mw_.audioEngineConf(i).rejectSettings();

	engineSelector_.reject();
	resamplerSelector_.reject();
	setRate(rateBox_, rate_);
	latencyBox_->setValue(latency_);
}

void SoundDialog::accept() {
	store();
	QDialog::accept();
}

void SoundDialog::reject() {
	restore();
	QDialog::reject();
}

void applySettings(MainWindow &mw, SoundDialog const &sd) {
	for (std::size_t i = 0, n = mw.numAudioEngines(); i < n; ++i)
		mw.audioEngineConf(i).acceptSettings();

	mw.setAudioOut(sd.engineIndex(), sd.rate(), sd.latency(), sd.resamplerNo());
}
