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
#ifndef WASAPIENGINE_H
#define WASAPIENGINE_H

#include "../audioengine.h"
#include "persistcheckbox.h"
#include <BaseTsd.h>
#include <memory>
#include "rateest.h"

class QComboBox;
class QCheckBox;
class IAudioClient;
class IAudioRenderClient;
class IAudioClock;

class WasapiEngine: public AudioEngine {
	const std::auto_ptr<QWidget> confWidget;
	QComboBox *const deviceSelector;
	PersistCheckBox exclusive_;
	IAudioClient *pAudioClient;
	IAudioRenderClient *pRenderClient;
	IAudioClock *pAudioClock;
	void *eventHandle_;
	unsigned pos_;
	unsigned posFrames;
	unsigned deviceIndex;
	unsigned nchannels_;
	UINT32 bufferFrameCount;
	RateEst est;
	bool started;

	int doInit(int rate, unsigned latency);
	void doAcceptSettings();
	int waitForSpace(UINT32 &numFramesPadding, unsigned space);
	int write(void *buffer, unsigned frames, UINT32 numFramesPadding);

public:
	static bool isUsable();
	WasapiEngine();
	~WasapiEngine();
	void uninit();
	int write(void *buffer, unsigned frames);
	int write(void *buffer, unsigned samples, BufferState &preBufState_out, long &rate_out);
	long rateEstimate() const { return est.result(); }
	const BufferState bufferState() const;
	void pause();
	QWidget* settingsWidget() const { return confWidget.get(); }
	void rejectSettings() const;
};

#endif /* WASAPIENGINE_H */
