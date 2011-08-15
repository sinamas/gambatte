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
#ifndef AUDIOENGINECONF_H
#define AUDIOENGINECONF_H

class AudioEngine;
class QString;
class QWidget;

class ConstAudioEngineConf {
	const AudioEngine *const ae;
	
public:
	/*explicit */ConstAudioEngineConf(const AudioEngine *const ae) : ae(ae) {}
	const QString& nameString() const;
	QWidget* settingsWidget() const;
	void rejectSettings() const;
	
	bool operator==(ConstAudioEngineConf r) const { return ae == r.ae; }
	bool operator!=(ConstAudioEngineConf r) const { return ae != r.ae; }
};

class AudioEngineConf {
	AudioEngine *const ae;
	
public:
	/*explicit */AudioEngineConf(AudioEngine *const ae) : ae(ae) {}
	const QString& nameString() const;
	QWidget* settingsWidget() const;
	void acceptSettings() const;
	void rejectSettings() const;
	
	bool operator==(AudioEngineConf r) const { return ae == r.ae; }
	bool operator!=(AudioEngineConf r) const { return ae != r.ae; }
	operator const ConstAudioEngineConf() const { return ConstAudioEngineConf(ae); }
};

#endif
