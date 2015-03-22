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

#include "videodialog.h"
#include "blitterconf.h"
#include "mainwindow.h"
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>
#include <functional>

VideoDialog::ScalingMethodSelector::ScalingMethodSelector(QWidget *parent)
: unrestrictedScalingButton_(new QRadioButton(QObject::tr("None"), parent))
, keepRatioButton_(new QRadioButton(QObject::tr("Keep aspect ratio"), parent))
, integerScalingButton_(new QRadioButton(QObject::tr("Only scale by integer factors"), parent))
, scaling_(ScalingMethod(filterValue(QSettings().value("video/scalingType", scaling_keep_ratio).toInt(),
                                     num_scaling_methods, 0, scaling_keep_ratio)))
{
	reject();
}

VideoDialog::ScalingMethodSelector::~ScalingMethodSelector() {
	QSettings settings;
	settings.setValue("video/scalingType", scaling_);
}

void VideoDialog::ScalingMethodSelector::addToLayout(QLayout *const containingLayout) {
	QGroupBox *const groupBox =
		addWidget(containingLayout, new QGroupBox(QObject::tr("Scaling restrictions")));
	groupBox->setLayout(new QVBoxLayout);
	groupBox->layout()->addWidget(unrestrictedScalingButton_);
	groupBox->layout()->addWidget(keepRatioButton_);
	groupBox->layout()->addWidget(integerScalingButton_);
}

void VideoDialog::ScalingMethodSelector::accept() {
	if (integerScalingButton_->isChecked()) {
		scaling_ = scaling_integer;
	} else if (keepRatioButton_->isChecked()) {
		scaling_ = scaling_keep_ratio;
	} else
		scaling_ = scaling_unrestricted;
}

void VideoDialog::ScalingMethodSelector::reject() {
	switch (scaling_) {
	case scaling_unrestricted: unrestrictedScalingButton_->click(); break;
	case scaling_keep_ratio: keepRatioButton_->click(); break;
	case scaling_integer: integerScalingButton_->click(); break;
	}
}

static void fillResBox(QComboBox *const box,
                       QSize const &sourceSize,
                       std::vector<ResInfo> const &resVector)
{
	long maxArea = 0;
	std::size_t maxAreaI = 0;
	for (std::size_t i = 0; i < resVector.size(); ++i) {
		int const hres = resVector[i].w;
		int const vres = resVector[i].h;

		if (hres >= sourceSize.width() && vres >= sourceSize.height()) {
			box->addItem(QString::number(hres) + 'x' + QString::number(vres),
			             static_cast<uint>(i));
		} else {
			long area = long(std::min(hres, sourceSize.width()))
			            * std::min(vres, sourceSize.height());
			if (area > maxArea) {
				maxArea = area;
				maxAreaI = i;
			}
		}
	}

	// add resolution giving maximal area if all resolutions are too small.
	if (box->count() < 1 && maxArea) {
		box->addItem(QString::number(resVector[maxAreaI].w)
		               + 'x' + QString::number(resVector[maxAreaI].h),
		             static_cast<uint>(maxAreaI));
	}
}

static QComboBox * createResBox(QSize const &sourceSize,
                                std::vector<ResInfo> const &resVector,
                                int defaultIndex,
                                QWidget *parent)
{
	QComboBox *box = new QComboBox(parent);
	fillResBox(box, sourceSize, resVector);
	box->setCurrentIndex(box->findData(defaultIndex));
	return box;
}

static auto_vector<PersistComboBox> createFullResSelectors(QSize const &sourceSize,
                                                           MainWindow const &mw,
                                                           QWidget *parent)
{
	auto_vector<PersistComboBox> v(mw.screens());
	for (std::size_t i = 0; i < v.size(); ++i) {
		v.reset(i, new PersistComboBox("video/fullRes" + QString::number(i),
		                               createResBox(sourceSize, mw.modeVector(i),
		                                            mw.currentResIndex(i), parent)));
	}

	return v;
}

static void addRates(QComboBox *box, std::vector<short> const &rates) {
	for (std::size_t i = 0; i < rates.size(); ++i)
		box->addItem(QString::number(rates[i] / 10.0) + " Hz");
}

static QComboBox * createHzBox(std::vector<short> const &rates,
                               std::size_t defaultIndex, QWidget *parent) {
	QComboBox *box = new QComboBox(parent);
	addRates(box, rates);
	box->setCurrentIndex(defaultIndex);
	return box;
}

static auto_vector<PersistComboBox> createFullHzSelectors(
		auto_vector<PersistComboBox> const &fullResSelectors,
		MainWindow const &mw,
		QWidget *parent)
{
	auto_vector<PersistComboBox> v(fullResSelectors.size());
	for (std::size_t i = 0; i < v.size(); ++i) {
		int resBoxIndex = fullResSelectors[i]->box()->currentIndex();
		std::size_t resIndex = fullResSelectors[i]->box()->itemData(resBoxIndex).toUInt();
		std::vector<short> const &rates =
			resBoxIndex >= 0 ? mw.modeVector(i)[resIndex].rates : std::vector<short>();
		std::size_t defaultRateIndex =
			resIndex == mw.currentResIndex(i) ? mw.currentRateIndex(i) : 0;
		v.reset(i, new PersistComboBox("video/hz" + QString::number(i),
		                               createHzBox(rates, defaultRateIndex, parent)));
	}

	return v;
}

static QComboBox * createEngineBox(MainWindow const &mw, QWidget *parent) {
	QComboBox *box = new QComboBox(parent);
	for (std::size_t i = 0, n = mw.numBlitters(); i < n; ++i) {
		ConstBlitterConf bconf = mw.blitterConf(i);
		box->addItem(bconf.nameString());
		if (QWidget *w = bconf.settingsWidget()) {
			w->hide();
			w->setParent(parent);
		}
	}

	return box;
}

static QComboBox * createSourceBox(std::vector<VideoDialog::VideoSourceInfo> const &sourceInfos,
                                   QWidget *parent)
{
	QComboBox *const box = new QComboBox(parent);
	for (std::size_t i = 0; i < sourceInfos.size(); ++i)
		box->addItem(sourceInfos[i].label, sourceInfos[i].size);

	return box;
}

VideoDialog::VideoDialog(MainWindow const &mw,
                         std::vector<VideoSourceInfo> const &sourceInfos,
                         QString const &sourcesLabel,
                         QWidget *parent)
: QDialog(parent)
, mw_(mw)
, engineSelector_("video/engine", createEngineBox(mw, this))
, sourceSelector_("video/source", createSourceBox(sourceInfos, this))
, fullResSelectors_(createFullResSelectors(
	sourceSelector_.box()->itemData(sourceSelector_.box()->currentIndex()).toSize(),
	mw,
	this))
, fullHzSelectors_(createFullHzSelectors(fullResSelectors_, mw, this))
, scalingMethodSelector_(this)
, engineWidget_()
{
	setWindowTitle(tr("Video Settings"));

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QVBoxLayout *const topLayout = addLayout(mainLayout, new QVBoxLayout, Qt::AlignTop);

	{
		QHBoxLayout *hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Video engine:")));
		hLayout->addWidget(engineSelector_.box());
	}

	if ((engineWidget_ = mw.blitterConf(engineSelector_.box()->currentIndex()).settingsWidget())) {
		topLayout->addWidget(engineWidget_);
		engineWidget_->show();
	}

	for (std::size_t i = 0; i < fullResSelectors_.size(); ++i) {
		if (mw.modeVector(i).empty()) {
			fullResSelectors_[i]->box()->hide();
			fullHzSelectors_[i]->box()->hide();
		} else {
			QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
			hLayout->addWidget(new QLabel(tr("Full screen mode (")
			                              + mw.screenName(i) + "):"));
			QHBoxLayout *const hhLayout = addLayout(hLayout, new QHBoxLayout);
			hhLayout->addWidget(fullResSelectors_[i]->box());
			hhLayout->addWidget(fullHzSelectors_[i]->box());
		}
	}

	scalingMethodSelector_.addToLayout(topLayout);
	if (sourceSelector_.box()->count() > 1) {
		QHBoxLayout *hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(sourcesLabel));
		sourceSelector_.box()->setParent(0); // reparent to fix tab order
		hLayout->addWidget(sourceSelector_.box());
	} else {
		sourceSelector_.box()->hide();
	}

	QHBoxLayout *const hLayout = addLayout(mainLayout, new QHBoxLayout,
	                                       Qt::AlignBottom | Qt::AlignRight);
	QPushButton *const okButton = addWidget(hLayout, new QPushButton(tr("OK")));
	QPushButton *const cancelButton = addWidget(hLayout, new QPushButton(tr("Cancel")));
	okButton->setDefault(true);

	connect(engineSelector_.box(), SIGNAL(currentIndexChanged(int)),
	        this, SLOT(engineChange(int)));
	for (std::size_t i = 0; i < fullResSelectors_.size(); ++i) {
		connect(fullResSelectors_[i]->box(), SIGNAL(currentIndexChanged(int)),
		        this, SLOT(resIndexChange(int)));
	}

	connect(sourceSelector_.box(), SIGNAL(currentIndexChanged(int)),
	        this, SLOT(sourceChange(int)));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void VideoDialog::engineChange(int const index) {
	QBoxLayout *const topLayout = static_cast<QBoxLayout *>(layout()->itemAt(0)->layout());
	if (engineWidget_) {
		engineWidget_->hide();
		topLayout->removeWidget(engineWidget_);
	}
	if ((engineWidget_ = mw_.blitterConf(index).settingsWidget())) {
		topLayout->insertWidget(1, engineWidget_);
		engineWidget_->show();
	}
}

void VideoDialog::resIndexChange(std::size_t const screen, int const index) {
	QComboBox const *const resBox = fullResSelectors_[screen]->box();
	std::vector<short> const &rates = index >= 0
		? mw_.modeVector(screen)[resBox->itemData(index).toUInt()].rates
		: std::vector<short>();
	fullHzSelectors_[screen]->box()->clear();
	addRates(fullHzSelectors_[screen]->box(), rates);
}

void VideoDialog::resIndexChange(int const index) {
	for (std::size_t i = 0; i < fullResSelectors_.size(); ++i) {
		if (sender() == fullResSelectors_[i]->box())
			return resIndexChange(i, index);
	}
}

void VideoDialog::sourceChange(int const sourceIndex) {
	QSize const sourceSize = sourceSelector_.box()->itemData(sourceIndex).toSize();
	for (std::size_t i = 0; i < fullResSelectors_.size(); ++i) {
		QComboBox *const resBox = fullResSelectors_[i]->box();
		QString const oldText = resBox->currentText();
		disconnect(resBox, SIGNAL(currentIndexChanged(int)), this, SLOT(resIndexChange(int)));
		resBox->clear();
		fillResBox(resBox, sourceSize, mw_.modeVector(i));

		int const foundOldTextIndex = resBox->findText(oldText);
		if (foundOldTextIndex >= 0) {
			resBox->setCurrentIndex(foundOldTextIndex);
		} else {
			resIndexChange(i, resBox->currentIndex());
		}

		connect(resBox, SIGNAL(currentIndexChanged(int)), this, SLOT(resIndexChange(int)));
	}
}

std::size_t VideoDialog::fullResIndex(std::size_t screen) const {
	PersistComboBox const *sel = fullResSelectors_[screen];
	return sel->box()->itemData(sel->index()).toUInt();
}

std::size_t VideoDialog::fullRateIndex(std::size_t screen) const {
	return fullHzSelectors_[screen]->index();
}

QSize const VideoDialog::sourceSize() const {
	return sourceSelector_.box()->itemData(sourceIndex()).toSize();
}

void VideoDialog::accept() {
	engineSelector_.accept();
	scalingMethodSelector_.accept();
	sourceSelector_.accept();
	std::for_each(fullResSelectors_.begin(), fullResSelectors_.end(),
	              std::mem_fun(&PersistComboBox::accept));
	std::for_each(fullHzSelectors_.begin(), fullHzSelectors_.end(),
	              std::mem_fun(&PersistComboBox::accept));
	QDialog::accept();
}

void VideoDialog::reject() {
	for (std::size_t i = 0, n = mw_.numBlitters(); i < n; ++i)
		mw_.blitterConf(i).rejectSettings();

	engineSelector_.reject();
	scalingMethodSelector_.reject();
	sourceSelector_.reject();
	std::for_each(fullResSelectors_.begin(), fullResSelectors_.end(),
	              std::mem_fun(&PersistComboBox::reject));
	std::for_each(fullHzSelectors_.begin(), fullHzSelectors_.end(),
	              std::mem_fun(&PersistComboBox::reject));
	QDialog::reject();
}

void applySettings(MainWindow &mw, VideoDialog const &vd) {
	{
		BlitterConf const curBlitter = mw.currentBlitterConf();
		for (std::size_t i = 0, n = mw.numBlitters(); i < n; ++i) {
			if (mw.blitterConf(i) != curBlitter)
				mw.blitterConf(i).acceptSettings();
		}

		QSize const srcSize = vd.sourceSize();
		mw.setVideoFormatAndBlitter(srcSize, vd.blitterNo());
		curBlitter.acceptSettings();
	}

	mw.setScalingMethod(vd.scalingMethod());

	for (std::size_t i = 0, n = mw.screens(); i < n; ++i)
		mw.setFullScreenMode(i, vd.fullResIndex(i), vd.fullRateIndex(i));
}
