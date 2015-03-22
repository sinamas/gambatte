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

#ifndef AUDIOENGINECONF_H
#define AUDIOENGINECONF_H

class AudioEngine;
class QString;
class QWidget;

class ConstAudioEngineConf {
public:
	/*explicit */ConstAudioEngineConf(AudioEngine const *ae) : ae_(ae) {}
	QString const & nameString() const;
	QWidget * settingsWidget() const;
	void rejectSettings() const;
	bool operator==(ConstAudioEngineConf r) const { return ae_ == r.ae_; }
	bool operator!=(ConstAudioEngineConf r) const { return ae_ != r.ae_; }

private:
	AudioEngine const *ae_;
};

class AudioEngineConf {
public:
	/*explicit */AudioEngineConf(AudioEngine *ae) : ae_(ae) {}
	QString const & nameString() const;
	QWidget * settingsWidget() const;
	void acceptSettings() const;
	void rejectSettings() const;
	bool operator==(AudioEngineConf r) const { return ae_ == r.ae_; }
	bool operator!=(AudioEngineConf r) const { return ae_ != r.ae_; }
	operator ConstAudioEngineConf() const { return ConstAudioEngineConf(ae_); }

private:
	AudioEngine *ae_;
};

#endif
