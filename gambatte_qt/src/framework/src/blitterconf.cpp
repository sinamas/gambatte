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
#include "blitterconf.h"
#include "blitterwidget.h"

const QString& ConstBlitterConf::nameString() const {
	return blitter->nameString();
}

unsigned ConstBlitterConf::maxSwapInterval() const {
	return blitter->maxSwapInterval();
}

QWidget* ConstBlitterConf::settingsWidget() const {
	return blitter->settingsWidget();
}

void ConstBlitterConf::rejectSettings() const {
	blitter->rejectSettings();
}

const QString& BlitterConf::nameString() const {
	return blitter->nameString();
}

unsigned BlitterConf::maxSwapInterval() const {
	return blitter->maxSwapInterval();
}

QWidget* BlitterConf::settingsWidget() const {
	return blitter->settingsWidget();
}

void BlitterConf::acceptSettings() const {
	blitter->acceptSettings();
}

void BlitterConf::rejectSettings() const {
	blitter->rejectSettings();
}
