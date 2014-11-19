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

#ifndef WASAPIENGINE_H
#define WASAPIENGINE_H

#include "../audioengine.h"
#include "dialoghelpers.h"
#include "rateest.h"
#include "scoped_ptr.h"
#include <BaseTsd.h>

class QComboBox;
class QCheckBox;
class IAudioClient;
class IAudioRenderClient;
class IAudioClock;

class WasapiEngine: public AudioEngine {
public:
	static bool isUsable();
	WasapiEngine();
	virtual ~WasapiEngine();
	virtual void uninit();
	virtual int write(void *buffer, std::size_t frames);
	virtual int write(void *buffer, std::size_t samples,
	                  BufferState &preBufState_out, long &rate_out);
	virtual long rateEstimate() const { return est.result(); }
	virtual BufferState bufferState() const;
	virtual void pause();
	virtual QWidget * settingsWidget() const { return confWidget.get(); }
	virtual void rejectSettings() const;

protected:
	virtual long doInit(long rate, int latency);
	virtual void doAcceptSettings();

private:
	scoped_ptr<QWidget> const confWidget;
	QComboBox *const deviceSelector;
	PersistCheckBox exclusive_;
	IAudioClient *pAudioClient;
	IAudioRenderClient *pRenderClient;
	IAudioClock *pAudioClock;
	void *eventHandle_;
	UINT32 pos_;
	UINT32 posFrames;
	unsigned deviceIndex;
	int nchannels_;
	UINT32 bufferFrameCount;
	RateEst est;
	bool started;

	int waitForSpace(UINT32 &numFramesPadding, UINT32 space);
	int write(void *buffer, std::size_t frames, UINT32 numFramesPadding);
};

#endif /* WASAPIENGINE_H */
