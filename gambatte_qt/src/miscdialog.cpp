/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#include "miscdialog.h"
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>

MiscDialog::MiscDialog(QWidget *const parent) :
QDialog(parent),
turboSpeedBox(new QSpinBox(this)),
pauseOnDialogsBox(new QCheckBox(tr("Pause when displaying dialogs"), this)),
pauseOnFocusOutBox(new QCheckBox(tr("Pause on focus out"), this)),
turboSpeed_(4),
pauseOnDialogs_(true),
pauseOnFocusOut_(false) {
	setWindowTitle(tr("Miscellaneous Settings"));
	QVBoxLayout *const mainLayout = new QVBoxLayout;
	QVBoxLayout *const topLayout = new QVBoxLayout;
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(new QLabel(tr("Fast-forward speed:")));
		turboSpeedBox->setRange(2, 16);
		turboSpeedBox->setSuffix("x");
		hLayout->addWidget(turboSpeedBox);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(pauseOnDialogsBox);
		topLayout->addLayout(hLayout);
	}
	
	{
		QHBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(pauseOnFocusOutBox);
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
	settings.beginGroup("misc");
	turboSpeed_ = std::min(std::max(settings.value("turboSpeed", turboSpeed_).toInt(), 2), 16);
	pauseOnDialogs_ = settings.value("pauseOnDialogs", pauseOnDialogs_).toBool();
	pauseOnFocusOut_ = settings.value("pauseOnFocusOut", pauseOnFocusOut_).toBool();
	settings.endGroup();
	
	restore();
}

MiscDialog::~MiscDialog() {
	QSettings settings;
	settings.beginGroup("misc");
	settings.setValue("turboSpeed", turboSpeed_);
	settings.setValue("pauseOnDialogs", pauseOnDialogs_);
	settings.setValue("pauseOnFocusOut", pauseOnFocusOut_);
	settings.endGroup();
}

void MiscDialog::store() {
	turboSpeed_ = turboSpeedBox->value();
	pauseOnDialogs_ = pauseOnDialogsBox->isChecked();
	pauseOnFocusOut_ = pauseOnFocusOutBox->isChecked();
}

void MiscDialog::restore() {
	turboSpeedBox->setValue(turboSpeed_);
	pauseOnDialogsBox->setChecked(pauseOnDialogs_);
	pauseOnFocusOutBox->setChecked(pauseOnFocusOut_);
}

void MiscDialog::accept() {
	store();
	QDialog::accept();
}

void MiscDialog::reject() {
	restore();
	QDialog::reject();
}
