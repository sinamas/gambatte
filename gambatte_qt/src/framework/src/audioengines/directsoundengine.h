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

#ifndef DIRECTSOUNDENGINE_H
#define DIRECTSOUNDENGINE_H

#include "../audioengine.h"
#include "rateest.h"
#include "scoped_ptr.h"
#include "usec.h"
#include <dsound.h>
#include <QList>

class QCheckBox;
class QComboBox;

class DirectSoundEngine : public AudioEngine {
public:
	explicit DirectSoundEngine(HWND hwnd);
	virtual ~DirectSoundEngine();
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
	RateEst est;
	scoped_ptr<QWidget> const confWidget;
	QComboBox *const deviceSelector;
	QCheckBox *const primaryBufBox;
	QCheckBox *const globalBufBox;
	LPDIRECTSOUND lpDS;
	LPDIRECTSOUNDBUFFER lpDSB;
	QList<GUID> deviceList;
	DWORD bufSize;
	DWORD bufSzDiff; // Difference between real buffer and desired buffer size.
	unsigned deviceIndex;
	DWORD offset;
	DWORD lastpc;
	HWND hwnd;
	bool primaryBuf;
	bool useGlobalBuf;
	bool blankBuf;

	static BOOL CALLBACK enumCallback(LPGUID, char const *, char const *, LPVOID);

	int waitForSpace(DWORD &pc, DWORD &wc, DWORD space);
	int getPosAndStatusCheck(DWORD &status, DWORD &pc, DWORD &wc);
	int doWrite(void *buffer, std::size_t frames, DWORD status, DWORD pc, DWORD wc);
	void fillBufferState(BufferState &s, DWORD pc, DWORD wc) const;
};

#endif
