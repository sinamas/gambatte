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

#include "blitterconf.h"
#include "blitterwidget.h"

QString const & ConstBlitterConf::nameString() const {
	return blitter_->nameString();
}

unsigned ConstBlitterConf::maxSwapInterval() const {
	return blitter_->maxSwapInterval();
}

QWidget * ConstBlitterConf::settingsWidget() const {
	return blitter_->settingsWidget();
}

void ConstBlitterConf::rejectSettings() const {
	blitter_->rejectSettings();
}

QString const & BlitterConf::nameString() const {
	return blitter_->nameString();
}

unsigned BlitterConf::maxSwapInterval() const {
	return blitter_->maxSwapInterval();
}

QWidget * BlitterConf::settingsWidget() const {
	return blitter_->settingsWidget();
}

void BlitterConf::acceptSettings() const {
	blitter_->acceptSettings();
}

void BlitterConf::rejectSettings() const {
	blitter_->rejectSettings();
}
