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

#include "audioengineconf.h"
#include "audioengine.h"

QString const & ConstAudioEngineConf::nameString() const {
	return ae_->nameString();
}

QWidget * ConstAudioEngineConf::settingsWidget() const {
	return ae_->settingsWidget();
}

void ConstAudioEngineConf::rejectSettings() const {
	ae_->rejectSettings();
}

QString const & AudioEngineConf::nameString() const {
	return ae_->nameString();
}

QWidget * AudioEngineConf::settingsWidget() const {
	return ae_->settingsWidget();
}

void AudioEngineConf::acceptSettings() const {
	ae_->acceptSettings();
}

void AudioEngineConf::rejectSettings() const {
	ae_->rejectSettings();
}
