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

template<class Parent, class ChildLayout>
static ChildLayout * addLayout(Parent *const parent, ChildLayout *const child) {
	parent->addLayout(child);
	return child;
}

template<class Parent, class ChildLayout>
static ChildLayout * addLayout(Parent *const parent, ChildLayout *const child, const Qt::Alignment alignment) {
	parent->addLayout(child);
	parent->setAlignment(child, alignment);
	return child;
}

MiscDialog::MiscDialog(QWidget *const parent) :
QDialog(parent),
turboSpeedBox(new QSpinBox(this)),
pauseOnDialogs_(new QCheckBox(tr("Pause when displaying dialogs"), this), "misc/pauseOnDialogs", true),
pauseOnFocusOut_(new QCheckBox(tr("Pause on focus out"), this), "misc/pauseOnFocusOut", false),
turboSpeed_(4) {
	setWindowTitle(tr("Miscellaneous Settings"));
	turboSpeedBox->setRange(2, 16);
	turboSpeedBox->setSuffix("x");
	
	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QVBoxLayout *const topLayout = addLayout(mainLayout, new QVBoxLayout);
	
	{
		QHBoxLayout *const hLayout = addLayout(topLayout, new QHBoxLayout);
		hLayout->addWidget(new QLabel(tr("Fast-forward speed:")));
		hLayout->addWidget(turboSpeedBox);
	}
	
	addLayout(topLayout, new QHBoxLayout)->addWidget(pauseOnDialogs_.checkBox());
	addLayout(topLayout, new QHBoxLayout)->addWidget(pauseOnFocusOut_.checkBox());
	
	{
		QPushButton *const okButton = new QPushButton(tr("OK"), this);
		QPushButton *const cancelButton = new QPushButton(tr("Cancel"), this);
		QHBoxLayout *const hLayout = addLayout(mainLayout, new QHBoxLayout, Qt::AlignBottom | Qt::AlignRight);
		
		hLayout->addWidget(okButton);
		hLayout->addWidget(cancelButton);

		okButton->setDefault(true);
		
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	}
	
	turboSpeed_ = std::min(std::max(QSettings().value("misc/turboSpeed", turboSpeed_).toInt(), 2), 16);
	restore();
}

MiscDialog::~MiscDialog() {
	QSettings settings;
	settings.setValue("misc/turboSpeed", turboSpeed_);
}

void MiscDialog::store() {
	turboSpeed_ = turboSpeedBox->value();
	pauseOnDialogs_.accept();
	pauseOnFocusOut_.accept();
}

void MiscDialog::restore() {
	turboSpeedBox->setValue(turboSpeed_);
	pauseOnDialogs_.reject();
	pauseOnFocusOut_.reject();
}

void MiscDialog::accept() {
	store();
	QDialog::accept();
}

void MiscDialog::reject() {
	restore();
	QDialog::reject();
}
