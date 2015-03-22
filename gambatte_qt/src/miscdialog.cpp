//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#include "miscdialog.h"
#include "mainwindow.h"
#include <QCheckBox>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

MiscDialog::MiscDialog(QString const &savepath, QWidget *parent)
: QDialog(parent)
, turboSpeedBox(new QSpinBox(this))
, pauseOnDialogs_(new QCheckBox(tr("Pause when displaying dialogs"), this), "misc/pauseOnDialogs", true)
, pauseOnFocusOut_(new QCheckBox(tr("Pause on focus out"), this), "misc/pauseOnFocusOut", false)
, fpsSelector_(this)
, dwmTripleBuf_(new QCheckBox(tr("DWM triple buffering"), this), "misc/dwmTripleBuf", true)
, multicartCompat_(new QCheckBox(tr("Multicart compatibility"), this), "misc/multicartCompat", false)
, savepathSelector_("Choose Save Path:",
                    "misc/savepath",
                    std::make_pair(QDir::toNativeSeparators(savepath), savepath),
                    std::make_pair(tr("Same folder as ROM image"), QString()),
                    this)
, turboSpeed_(8)
{
	setWindowTitle(tr("Miscellaneous Settings"));
	turboSpeedBox->setRange(2, 16);
	turboSpeedBox->setSuffix("x");

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QVBoxLayout *const topLayout = addLayout(mainLayout, new QVBoxLayout);

	{
		QHBoxLayout *hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Fast-forward speed:")));
		hLayout->addWidget(turboSpeedBox);
	}

	addLayout(topLayout, new QHBoxLayout)->addWidget(pauseOnDialogs_.checkBox());
	addLayout(topLayout, new QHBoxLayout)->addWidget(pauseOnFocusOut_.checkBox());

	{
		QHBoxLayout *hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Base frame rate:")));
		hLayout->addWidget(fpsSelector_.widget());
	}

	if (MainWindow::hasDwmCapability()) {
		dwmTripleBuf_.checkBox()->setToolTip(tr(
			"Avoids excessive frame duplication when DWM composition is active. Recommended."));
		addLayout(topLayout, new QHBoxLayout)->addWidget(dwmTripleBuf_.checkBox());
	} else
		dwmTripleBuf_.checkBox()->hide();

	multicartCompat_.checkBox()->setToolTip(tr(
		"Support certain multicart ROM images by not strictly respecting ROM header MBC type."));
	addLayout(topLayout, new QHBoxLayout)->addWidget(multicartCompat_.checkBox());

	{
		QHBoxLayout *hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Save path:")));
		hLayout->addWidget(savepathSelector_.widget());
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

	turboSpeed_ = std::min(std::max(QSettings().value("misc/turboSpeed", turboSpeed_).toInt(), 2),
	                       16);
	restore();
}

MiscDialog::~MiscDialog() {
	QSettings settings;
	settings.setValue("misc/turboSpeed", turboSpeed_);
}

void MiscDialog::restore() {
	fpsSelector_.reject();
	turboSpeedBox->setValue(turboSpeed_);
	pauseOnDialogs_.reject();
	pauseOnFocusOut_.reject();
	dwmTripleBuf_.reject();
	multicartCompat_.reject();
	savepathSelector_.reject();
}

void MiscDialog::accept() {
	fpsSelector_.accept();
	turboSpeed_ = turboSpeedBox->value();
	pauseOnDialogs_.accept();
	pauseOnFocusOut_.accept();
	dwmTripleBuf_.accept();
	multicartCompat_.accept();
	savepathSelector_.accept();
	QDialog::accept();
}

void MiscDialog::reject() {
	restore();
	QDialog::reject();
}
