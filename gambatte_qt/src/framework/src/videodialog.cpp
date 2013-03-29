/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#include "blitterconf.h"
#include "mainwindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDesktopWidget>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QString>
#include <QVBoxLayout>
#include <functional>

static int filterValue(int value, int upper, int lower = 0, int fallback = 0) {
	if (value >= upper || value < lower)
		return fallback;

	return value;
}

VideoDialog::PersistInt::PersistInt(const QString &key, const int upper, const int lower, const int defaultVal)
: key_(key), i_(filterValue(QSettings().value(key, defaultVal).toInt(), upper, lower, defaultVal))
{
}

VideoDialog::PersistInt::~PersistInt() {
	QSettings settings;
	settings.setValue(key_, i_);
}

VideoDialog::EngineSelector::EngineSelector(MainWindow const &mw)
: comboBox_(new QComboBox)
, index_("video/engineIndex", mw.numBlitters())
{
	for (std::size_t i = 0; i < mw.numBlitters(); ++i)
		comboBox_->addItem(mw.blitterConf(i).nameString());

	restore();
}

void VideoDialog::EngineSelector::addToLayout(QBoxLayout *const topLayout) {
	QHBoxLayout *const hLayout = new QHBoxLayout;
	hLayout->addWidget(new QLabel(tr("Video engine:")));
	hLayout->addWidget(comboBox_);
	topLayout->addLayout(hLayout);
}

void VideoDialog::EngineSelector::store() {
	index_ = comboBox_->currentIndex();
}

void VideoDialog::EngineSelector::restore() {
	comboBox_->setCurrentIndex(index_);
}

VideoDialog::ScalingMethodSelector::ScalingMethodSelector()
: unrestrictedScalingButton_(new QRadioButton(QString("None"))),
  keepRatioButton_(new QRadioButton(QString("Keep aspect ratio"))),
  integerScalingButton_(new QRadioButton(QString("Only scale by integer factors"))),
  scaling_("video/scalingType", NUM_SCALING_METHODS, 0, KEEP_RATIO)
{
	restore();
}

void VideoDialog::ScalingMethodSelector::addToLayout(QLayout *const containingLayout) {
	QGroupBox *const groupBox = new QGroupBox("Scaling restrictions");
	groupBox->setLayout(new QVBoxLayout);
	groupBox->layout()->addWidget(unrestrictedScalingButton_);
	groupBox->layout()->addWidget(keepRatioButton_);
	groupBox->layout()->addWidget(integerScalingButton_);
	containingLayout->addWidget(groupBox);
}

void VideoDialog::ScalingMethodSelector::store() {
	if (integerScalingButton_->isChecked()) {
		scaling_ = INTEGER;
	} else if (keepRatioButton_->isChecked()) {
		scaling_ = KEEP_RATIO;
	} else
		scaling_ = UNRESTRICTED;
}

void VideoDialog::ScalingMethodSelector::restore() {
	switch (scaling_) {
	case UNRESTRICTED: unrestrictedScalingButton_->click(); break;
	case KEEP_RATIO: keepRatioButton_->click(); break;
	case INTEGER: integerScalingButton_->click(); break;
	}
}

const QAbstractButton * VideoDialog::ScalingMethodSelector::integerScalingButton() const {
	return integerScalingButton_;
}

VideoDialog::SourceSelector::SourceSelector(const QString &sourcesLabel, const std::vector<VideoSourceInfo> &sourceInfos)
: label_(new QLabel(sourcesLabel)), comboBox_(new QComboBox), index_("video/sourceIndexStore", sourceInfos.size())
{
	setVideoSources(sourceInfos);
	restore();
}

void VideoDialog::SourceSelector::addToLayout(QBoxLayout *const containingLayout) {
	QHBoxLayout *const hLayout = new QHBoxLayout;
	hLayout->addWidget(label_);
	hLayout->addWidget(comboBox_);
	containingLayout->addLayout(hLayout);
}

void VideoDialog::SourceSelector::store() {
	index_ = comboBox_->currentIndex();
}

void VideoDialog::SourceSelector::restore() {
	comboBox_->setCurrentIndex(index_);
}

void VideoDialog::SourceSelector::setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos) {
	comboBox_->clear();

	for (std::size_t i = 0; i < sourceInfos.size(); ++i)
		comboBox_->addItem(sourceInfos[i].label, QSize(sourceInfos[i].width, sourceInfos[i].height));

	if (comboBox_->count() < 2) {
		comboBox_->hide();
		label_->hide();
	} else if (comboBox_->parentWidget()) {
		comboBox_->show();
		label_->show();
	}
}

class VideoDialog::FullResSelector {
public:
	~FullResSelector();
	QWidget * widget() const { return comboBox_; }
	QComboBox const * comboBox() const { return comboBox_; }
	void store() { index_ = comboBox_->currentIndex(); }
	void restore() { comboBox_->setCurrentIndex(index_); }
	void setSourceSize(QSize const &sourceSize, std::vector<ResInfo> const &resVector);
	int index() const { return index_; }

	static auto_vector<FullResSelector> createSelectors(QSize const &sourceSize,
	                                                    MainWindow const &mw)
	{
		auto_vector<FullResSelector> v(mw.screens());
		for (std::size_t i = 0; i < v.size(); ++i) {
			v.reset(i, new FullResSelector("video/fullRes" + QString::number(i),
			                               mw.currentResIndex(i), sourceSize,
			                               mw.modeVector(i)));
		}

		return v;
	}

private:
	QComboBox *const comboBox_;
	QString const key_;
	int index_;

	FullResSelector(QString const &key,
	                int const defaultIndex,
	                QSize const &sourceSize,
	                std::vector<ResInfo> const &resVector)
	: comboBox_(new QComboBox)
	, key_(key)
	, index_(defaultIndex)
	{
		fillComboBox(sourceSize, resVector);
		index_ = filterValue(comboBox_->findText(QSettings().value(key).toString()),
		                     comboBox_->count(), 0, index_);
		restore();
	}

	void fillComboBox(QSize const &sourceSize, std::vector<ResInfo> const &resVector);
};

VideoDialog::FullResSelector::~FullResSelector() {
	QSettings settings;
	settings.setValue(key_, comboBox_->itemText(index_));
}

void VideoDialog::FullResSelector::setSourceSize(
		QSize const &sourceSize, std::vector<ResInfo> const &resVector) {
	QString const oldtext(comboBox_->itemText(comboBox_->currentIndex()));
	comboBox_->clear();
	fillComboBox(sourceSize, resVector);

	int newIndex = comboBox_->findText(oldtext);
	if (newIndex >= 0)
		comboBox_->setCurrentIndex(newIndex);
}

void VideoDialog::FullResSelector::fillComboBox(
		QSize const &sourceSize, std::vector<ResInfo> const &resVector) {
	long maxArea = 0;
	std::size_t maxAreaI = 0;

	for (std::size_t i = 0; i < resVector.size(); ++i) {
		int const hres = resVector[i].w;
		int const vres = resVector[i].h;

		if (hres >= sourceSize.width() && vres >= sourceSize.height()) {
			comboBox_->addItem(QString::number(hres) + QString("x") + QString::number(vres),
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
	if (comboBox_->count() < 1 && maxArea) {
		comboBox_->addItem(QString::number(resVector[maxAreaI].w)
		                     + "x" + QString::number(resVector[maxAreaI].h),
		                   static_cast<uint>(maxAreaI));
	}
}

class VideoDialog::FullHzSelector {
public:
	~FullHzSelector();
	QWidget * widget() const { return comboBox_; }
	QComboBox const * comboBox() const { return comboBox_; }
	void store() { index_ = comboBox_->currentIndex(); }
	void restore() { comboBox_->setCurrentIndex(index_); }
	void setRates(std::vector<short> const &rates);
	int index() const { return index_; }

	static auto_vector<FullHzSelector> createSelectors(
			auto_vector<FullResSelector> const &fullResSelectors,
			MainWindow const &mw)
	{
		auto_vector<FullHzSelector> v(fullResSelectors.size());
		for (std::size_t i = 0; i < v.size(); ++i) {
			int const resindex = fullResSelectors[i]->comboBox()->currentIndex();
			std::vector<short> const &rates = resindex >= 0
			                                ? mw.modeVector(i)[resindex].rates
			                                : std::vector<short>();
			int defaultRateIndex = resindex == static_cast<int>(mw.currentResIndex(i))
			                     ? mw.currentRateIndex(i)
			                     : 0;
			v.reset(i, new FullHzSelector("video/hz" + QString::number(i),
			                              rates, defaultRateIndex));
		}

		return v;
	}

private:
	QComboBox *const comboBox_;
	QString const key_;
	int index_;

	FullHzSelector(QString const &key,
	               std::vector<short> const &rates,
	               int const defaultIndex)
	: comboBox_(new QComboBox)
	, key_(key)
	, index_(0)
	{
		setRates(rates);
		index_ = filterValue(comboBox_->findText(QSettings().value(key).toString()),
		                     comboBox_->count(),
		                     0,
		                     defaultIndex);
		restore();
	}
};

VideoDialog::FullHzSelector::~FullHzSelector() {
	QSettings settings;
	settings.setValue(key_, comboBox_->itemText(index_));
}

void VideoDialog::FullHzSelector::setRates(std::vector<short> const &rates) {
	comboBox_->clear();
	for (std::size_t i = 0; i < rates.size(); ++i) {
		comboBox_->addItem(QString::number(rates[i] / 10.0) + QString(" Hz"),
		                   static_cast<uint>(i));
	}
}

VideoDialog::VideoDialog(MainWindow const &mw,
                         std::vector<VideoSourceInfo> const &sourceInfos,
                         QString const &sourcesLabel,
                         QWidget *parent)
: QDialog(parent)
, mw_(mw)
, topLayout(new QVBoxLayout)
, engineWidget(0)
, engineSelector(mw)
, sourceSelector(sourcesLabel, sourceInfos)
, fullResSelectors(FullResSelector::createSelectors(
	sourceSelector.comboBox()->itemData(sourceSelector.comboBox()->currentIndex()).toSize(),
	mw))
, fullHzSelectors(FullHzSelector::createSelectors(fullResSelectors, mw))
{
	QVBoxLayout *mainLayout = new QVBoxLayout;
	setLayout(mainLayout);

	engineSelector.addToLayout(topLayout);

	if ((engineWidget = mw.blitterConf(engineSelector.comboBox()->currentIndex()).settingsWidget()))
		topLayout->addWidget(engineWidget);

	for (std::size_t i = 0; i < fullResSelectors.size(); ++i) {
		QHBoxLayout *const hLayout = new QHBoxLayout;
		QLabel *const label = new QLabel("Full screen mode (" + mw.screenName(i) + "):");
		hLayout->addWidget(label);

		QHBoxLayout *const hhLayout = new QHBoxLayout;
		hhLayout->addWidget(fullResSelectors[i]->widget());
		hhLayout->addWidget(fullHzSelectors[i]->widget());
		hLayout->addLayout(hhLayout);
		topLayout->addLayout(hLayout);

		if (mw.modeVector(i).empty()) {
			label->hide();
			fullResSelectors[i]->widget()->hide();
			fullHzSelectors[i]->widget()->hide();
		}
	}

	scalingMethodSelector.addToLayout(topLayout);
	sourceSelector.addToLayout(topLayout);

	mainLayout->addLayout(topLayout);
	mainLayout->setAlignment(topLayout, Qt::AlignTop);

	QHBoxLayout *const hLayout = new QHBoxLayout;
	QPushButton *const okButton = new QPushButton(tr("OK"));
	hLayout->addWidget(okButton);
	QPushButton *const cancelButton = new QPushButton(tr("Cancel"));
	hLayout->addWidget(cancelButton);
	mainLayout->addLayout(hLayout);
	mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	okButton->setDefault(true);

	connect(engineSelector.comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(engineChange(int)));

	for (std::size_t i = 0; i < fullResSelectors.size(); ++i) {
		connect(fullResSelectors[i]->comboBox(), SIGNAL(currentIndexChanged(int)),
		        this, SLOT(fullresChange(int)));
	}

	connect(sourceSelector.comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	setWindowTitle(tr("Video Settings"));
}

VideoDialog::~VideoDialog() {
}

void VideoDialog::fillFullResSelectors() {
	QSize const &sourceSize =
		sourceSelector.comboBox()->itemData(sourceSelector.comboBox()->currentIndex()).toSize();
	for (std::size_t i = 0; i < fullResSelectors.size(); ++i) {
		int const oldResIndex = fullResSelectors[i]->comboBox()->currentIndex();

		disconnect(fullResSelectors[i]->comboBox(), SIGNAL(currentIndexChanged(int)),
		           this, SLOT(fullresChange(int)));
		fullResSelectors[i]->setSourceSize(sourceSize, mw_.modeVector(i));
		connect(fullResSelectors[i]->comboBox(), SIGNAL(currentIndexChanged(int)),
		        this, SLOT(fullresChange(int)));

		int const newResIndex = fullResSelectors[i]->comboBox()->currentIndex();
		if (newResIndex != oldResIndex) {
			fullHzSelectors[i]->setRates(newResIndex >= 0
				? mw_.modeVector(i)[newResIndex].rates
				: std::vector<short>());
		}
	}
}

void VideoDialog::store() {
	engineSelector.store();
	scalingMethodSelector.store();
	sourceSelector.store();
	std::for_each(fullResSelectors.begin(), fullResSelectors.end(), std::mem_fun(&FullResSelector::store));
	std::for_each(fullHzSelectors.begin(), fullHzSelectors.end(), std::mem_fun(&FullHzSelector::store));
}

void VideoDialog::restore() {
	for (std::size_t i = 0; i < mw_.numBlitters(); ++i)
		mw_.blitterConf(i).rejectSettings();

	engineSelector.restore();
	scalingMethodSelector.restore();
	sourceSelector.restore();
	std::for_each(fullResSelectors.begin(), fullResSelectors.end(), std::mem_fun(&FullResSelector::restore));
	std::for_each(fullHzSelectors.begin(), fullHzSelectors.end(), std::mem_fun(&FullHzSelector::restore));
}

void VideoDialog::engineChange(int index) {
	if (engineWidget) {
		topLayout->removeWidget(engineWidget);
		engineWidget->setParent(0);
	}

	if ((engineWidget = mw_.blitterConf(index).settingsWidget()))
		topLayout->insertWidget(1, engineWidget);
}

void VideoDialog::fullresChange(int const index) {
	for (std::size_t i = 0; i < fullResSelectors.size(); ++i) {
		if (sender() == fullResSelectors[i]->comboBox()) {
			fullHzSelectors[i]->setRates(index >= 0
				? mw_.modeVector(i)[index].rates
				: std::vector<short>());
			break;
		}
	}
}

void VideoDialog::sourceChange(int /*index*/) {
	fillFullResSelectors();
}

int VideoDialog::blitterNo() const {
	return engineSelector.index();
}

unsigned VideoDialog::fullResIndex(unsigned screen) const {
	return fullResSelectors[screen]->comboBox()->itemData(fullResSelectors[screen]->index()).toUInt();
}

unsigned VideoDialog::fullRateIndex(unsigned screen) const {
	return fullHzSelectors[screen]->comboBox()->itemData(fullHzSelectors[screen]->index()).toUInt();
}

const QSize VideoDialog::sourceSize() const {
	return sourceSelector.comboBox()->itemData(sourceIndex()).toSize();
}

void VideoDialog::setVideoSources(const std::vector<VideoSourceInfo> &sourceInfos) {
	restore();
	disconnect(sourceSelector.comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	sourceSelector.setVideoSources(sourceInfos);
	connect(sourceSelector.comboBox(), SIGNAL(currentIndexChanged(int)), this, SLOT(sourceChange(int)));
	sourceChange(sourceSelector.comboBox()->currentIndex());
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

void applySettings(MainWindow &mw, VideoDialog const &vd) {
	{
		BlitterConf const curBlitter = mw.currentBlitterConf();
		for (std::size_t i = 0; i < mw.numBlitters(); ++i) {
			if (mw.blitterConf(i) != curBlitter)
				mw.blitterConf(i).acceptSettings();
		}

		QSize const srcSz = vd.sourceSize();
		mw.setVideoFormatAndBlitter(srcSz.width(), srcSz.height(), vd.blitterNo());
		curBlitter.acceptSettings();
	}

	mw.setScalingMethod(vd.scalingMethod());

	for (std::size_t i = 0; i < mw.screens(); ++i)
		mw.setFullScreenMode(i, vd.fullResIndex(i), vd.fullRateIndex(i));
}
