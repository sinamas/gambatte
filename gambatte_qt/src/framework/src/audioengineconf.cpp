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
#include "audioengineconf.h"
#include "audioengine.h"

const QString& ConstAudioEngineConf::nameString() const {
	return ae->nameString();
}

QWidget* ConstAudioEngineConf::settingsWidget() const {
	return ae->settingsWidget();
}

void ConstAudioEngineConf::rejectSettings() const {
	ae->rejectSettings();
}

const QString& AudioEngineConf::nameString() const {
	return ae->nameString();
}

QWidget* AudioEngineConf::settingsWidget() const {
	return ae->settingsWidget();
}

void AudioEngineConf::acceptSettings() const {
	ae->acceptSettings();
}

void AudioEngineConf::rejectSettings() const {
	ae->rejectSettings();
}
