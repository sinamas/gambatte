/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef OSSENGINE_H
#define OSSENGINE_H

#include "../audioengine.h"
#include "customdevconf.h"
#include "rateest.h"

class OssEngine : public AudioEngine {
	CustomDevConf conf;
	RateEst est;
	int audio_fd;
	unsigned bufSize;
	unsigned fragSize;
	unsigned prevbytes;
	
	int doInit(int rate, unsigned latency);
	void doAcceptSettings() { conf.acceptSettings(); }
	int write(void *buffer, unsigned samples, const BufferState &bstate);
	
public:
	OssEngine();
	~OssEngine();
	void uninit();
	int write(void *buffer, unsigned samples);
	int write(void *buffer, unsigned samples, BufferState &preBufState_out, long &rate_out);
	long rateEstimate() const { return est.result(); }
	const BufferState bufferState() const;
	void pause() { prevbytes = 0; est.reset(); }
	bool flushPausedBuffers() const { return true; }
	QWidget* settingsWidget() const { return conf.settingsWidget(); }
	void rejectSettings() const { conf.rejectSettings(); }
};

#endif
