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
#ifndef DIRECTSOUNDENGINE_H
#define DIRECTSOUNDENGINE_H

class QCheckBox;
class QComboBox;

#include "../audioengine.h"
#include <QList>
#include <memory>
#include <dsound.h>
#include "usec.h"
#include "rateest.h"

class DirectSoundEngine : public AudioEngine {
	RateEst est;
	const std::auto_ptr<QWidget> confWidget;
	QCheckBox *const primaryBufBox;
	QCheckBox *const globalBufBox;
	QComboBox *const deviceSelector;
	LPDIRECTSOUND lpDS;
	LPDIRECTSOUNDBUFFER lpDSB;
	QList<GUID> deviceList;
	unsigned bufSize;
	unsigned bufSzDiff; // Difference between real buffer and desired buffer size.
	unsigned deviceIndex;
	DWORD offset;
	DWORD lastpc;
	HWND hwnd;
	bool primaryBuf;
	bool useGlobalBuf;
	bool blankBuf;

	static BOOL CALLBACK enumCallback(LPGUID, const char*, const char*, LPVOID);

	int doInit(int rate, unsigned latency);
	void doAcceptSettings();
	int waitForSpace(DWORD &pc, DWORD &wc, unsigned space);
	int getPosAndStatusCheck(DWORD &status, DWORD &pc, DWORD &wc);
	int doWrite(void *buffer, unsigned frames, DWORD status, DWORD pc, DWORD wc);
	void fillBufferState(BufferState &s, DWORD pc, DWORD wc) const;

public:
	DirectSoundEngine(HWND hwnd);
	~DirectSoundEngine();
	void uninit();
	int write(void *buffer, unsigned frames);
	int write(void *buffer, unsigned samples, BufferState &preBufState_out, long &rate_out);
	long rateEstimate() const { return est.result(); }
	const BufferState bufferState() const;
	void pause();
	QWidget* settingsWidget() const { return confWidget.get(); }
	void rejectSettings() const;
};

#endif
