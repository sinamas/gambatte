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
#include "directsoundengine.h"

#include <cstring>
#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <iostream>

Q_DECLARE_METATYPE(GUID*)

BOOL CALLBACK DirectSoundEngine::enumCallback(LPGUID lpGuid, const char *lpcstrDescription, const char */*lpcstrModule*/, LPVOID lpContext) {
	if (lpGuid) {
		DirectSoundEngine *const thisptr = static_cast<DirectSoundEngine*>(lpContext);
		thisptr->deviceList.append(*lpGuid);
		thisptr->deviceSelector->addItem(lpcstrDescription, QVariant::fromValue(&thisptr->deviceList.last()));
	}

	return true;
}

DirectSoundEngine::DirectSoundEngine(HWND hwnd_in) :
	AudioEngine("DirectSound"),
	confWidget(new QWidget),
	primaryBufBox(new QCheckBox("Write to primary buffer")),
	globalBufBox(new QCheckBox("Global buffer")),
	deviceSelector(new QComboBox),
	lpDS(NULL),
	lpDSB(NULL),
	bufSize(0),
	bufSzDiff(0),
	deviceIndex(0),
	offset(0),
	lastpc(0),
	hwnd(hwnd_in),
	primaryBuf(false),
	useGlobalBuf(false),
	blankBuf(true)
{
	DirectSoundEnumerateA(enumCallback, this);

	if (deviceSelector->count() < 1)
		deviceSelector->addItem(QString(), QVariant::fromValue<GUID*>(NULL));

	{
		QVBoxLayout *const mainLayout = new QVBoxLayout;
		mainLayout->setMargin(0);

		if (deviceSelector->count() > 1) {
			QHBoxLayout *const hlayout = new QHBoxLayout;

			hlayout->addWidget(new QLabel(QString("DirectSound device:")));
			hlayout->addWidget(deviceSelector);

			mainLayout->addLayout(hlayout);
		}

		mainLayout->addWidget(primaryBufBox);
		mainLayout->addWidget(globalBufBox);
		confWidget->setLayout(mainLayout);
	}

	{
		QSettings settings;
		settings.beginGroup("directsoundengine");
		primaryBuf = settings.value("primaryBuf", primaryBuf).toBool();
		useGlobalBuf = settings.value("useGlobalBuf", useGlobalBuf).toBool();

		if ((deviceIndex = settings.value("deviceIndex", deviceIndex).toUInt()) >= static_cast<unsigned>(deviceSelector->count()))
			deviceIndex = 0;

		settings.endGroup();
	}

	rejectSettings();
}

DirectSoundEngine::~DirectSoundEngine() {
	uninit();

	QSettings settings;
	settings.beginGroup("directsoundengine");
	settings.setValue("primaryBuf", primaryBuf);
	settings.setValue("useGlobalBuf", useGlobalBuf);
	settings.setValue("deviceIndex", deviceIndex);
	settings.endGroup();
}

void DirectSoundEngine::doAcceptSettings() {
	primaryBuf = primaryBufBox->isChecked();
	useGlobalBuf = globalBufBox->isChecked();
	deviceIndex = deviceSelector->currentIndex();
}

void DirectSoundEngine::rejectSettings() const {
	primaryBufBox->setChecked(primaryBuf);
	globalBufBox->setChecked(useGlobalBuf);
	deviceSelector->setCurrentIndex(deviceIndex);
}

/*static unsigned nearestPowerOf2(const unsigned in) {
	unsigned out = in;

	out |= out >> 1;
	out |= out >> 2;
	out |= out >> 4;
	out |= out >> 8;
	out |= out >> 16;
	++out;

	if (!(out >> 2 & in))
		out >>= 1;

	return out;
}*/

int DirectSoundEngine::doInit(const int rate, const unsigned latency) {
	if (DirectSoundCreate(deviceSelector->itemData(deviceIndex).value<GUID*>(), &lpDS, NULL) != DS_OK) {
		lpDS = NULL;
		std::cerr << "DirectSoundCreate failed" << std::endl;
		goto fail;
	}

	if (lpDS->SetCooperativeLevel(hwnd, primaryBuf ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY) != DS_OK) {
		std::cerr << "SetCooperativeLevel failed" << std::endl;
		goto fail;
	}

	{
		DSBUFFERDESC dsbd;
		std::memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | (primaryBuf ? DSBCAPS_PRIMARYBUFFER : (useGlobalBuf ? DSBCAPS_GLOBALFOCUS : 0));

		WAVEFORMATEX wfe;
		std::memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.nChannels = 2;
		wfe.nSamplesPerSec = rate;
		wfe.wBitsPerSample = 16;
		wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample >> 3;
		wfe.nAvgBytesPerSec = rate * wfe.nBlockAlign;

		const unsigned desiredBufSz = /*nearestPowerOf2*/(((rate * latency + 500) / 1000) * 4);

		if (!primaryBuf) {
			dsbd.lpwfxFormat = &wfe;

			int bufferSize = desiredBufSz;

			if (bufferSize < DSBSIZE_MIN)
				bufferSize = DSBSIZE_MIN;

			if (bufferSize > DSBSIZE_MAX)
				bufferSize = DSBSIZE_MAX;

			dsbd.dwBufferBytes = bufferSize;
		}

		{
			GUID guidNULL;
			std::memset(&guidNULL,0,sizeof(GUID));
			dsbd.guid3DAlgorithm = guidNULL;
		}

		if (lpDS->CreateSoundBuffer(&dsbd, &lpDSB, NULL) != DS_OK) {
			lpDSB = NULL;
			std::cerr << "CreateSoundBuffer failed" << std::endl;
			goto fail;
		}

		if (primaryBuf && lpDSB->SetFormat(&wfe) != DS_OK) {
			std::cerr << "lpDSB->SetFormat failed" << std::endl;
			goto fail;
		}

		DSBCAPS dsbcaps;
		std::memset(&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		if (lpDSB->GetCaps(&dsbcaps) != DS_OK)
			goto fail;

		bufSize = dsbcaps.dwBufferBytes;

		// We use bufSzDiff to only use the lower desiredBufSz bytes of the buffer for effective latency closer to
		// the desired one, since we can't set the primary buffer size.
		bufSzDiff = primaryBuf && desiredBufSz < bufSize ? bufSize - desiredBufSz : 0;
	}

	// set offset for meaningful initial bufferState() results.
	if (lpDSB->GetCurrentPosition(&offset, NULL) != DS_OK)
		offset = 0;

	offset += 1;

	blankBuf = true;
	est.init(rate, rate, bufSize >> 2);

	return rate;

fail:
	uninit();
	return -1;
}

void DirectSoundEngine::uninit() {
	if (lpDSB) {
		lpDSB->Stop();
		lpDSB->Release();
	}

	lpDSB = NULL;

	if (lpDS)
		lpDS->Release();

	lpDS = NULL;
}

static unsigned limitCursor(unsigned cursor, const unsigned bufSz) {
	if (cursor >= bufSz)
		cursor -= bufSz;

	return cursor;
}

static int fromUnderrun(const int pc, const int wc, const int offset, const int bufSize) {
	return (offset > pc ? offset : offset + bufSize) - (wc < pc ? wc + bufSize : wc);
}

static int fromOverflow(int pc, const int offset, const int bufSize) {
	if ((pc -= offset) < 0)
		pc += bufSize;

	return pc;
}

static inline int decCursor(const int cursor, const int dec, const int bufSz) {
	return fromOverflow(cursor, dec, bufSz);
}

int DirectSoundEngine::waitForSpace(DWORD &pc, DWORD &wc, const unsigned space) {
	int fof = 0;
	unsigned n = 100;
	const int adjustedOffset = limitCursor(offset + bufSzDiff, bufSize);

	while (n-- && static_cast<unsigned>(fof = fromOverflow(pc, adjustedOffset, bufSize)) < space) {
		{
			DWORD status;
			lpDSB->GetStatus(&status);

			if (!(status & DSBSTATUS_PLAYING)) {
				const HRESULT res = lpDSB->Play(0, 0, DSBPLAY_LOOPING);

				if (res != DS_OK) {
					if (res != DSERR_BUFFERLOST)
						return -1;

					break;
				}
			}
		}

		Sleep(1);

		if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK)
			return -1;
	}

	{
		// Decrementing pc has the same effect as incrementing offset as far as fromOverflow is concerned,
		// but adjusting offset doesn't work well for fromUnderrun. We adjust offset rather than pc in the loop
		// because it changes less often.
		const int adjustedPc = decCursor(pc, bufSzDiff, bufSize);

		if (fromUnderrun(adjustedPc, wc, offset, bufSize) < 0) {
			//std::cout << "underrun" << std::endl;
			offset = wc;
			fof = fromOverflow(adjustedPc, offset, bufSize);
		}
	}

	return fof;
}

static int write(LPDIRECTSOUNDBUFFER lpDSB, const unsigned offset, void *const buffer, const unsigned bytes) {
	LPVOID ptr1;
	LPVOID ptr2;
	DWORD bytes1;
	DWORD bytes2;

	{
		const HRESULT res = lpDSB->Lock(offset, bytes, &ptr1, &bytes1, &ptr2, &bytes2, 0);

		if (res != DS_OK)
			return res == DSERR_BUFFERLOST ? 0 : -1;
	}

	std::memcpy(ptr1, buffer, bytes1);

	if (ptr2)
		std::memcpy(ptr2, static_cast<char*>(buffer) + bytes1, bytes2);

	lpDSB->Unlock(ptr1, bytes1, ptr2, bytes2);

	return 0;
}

int DirectSoundEngine::doWrite(void *buffer, const unsigned frames, const DWORD status, DWORD pc, DWORD wc) {
	if (!(status & DSBSTATUS_PLAYING)) {
		if (blankBuf) { // make sure we write from pc, so no uninitialized samples are played
			offset = wc = pc;
			pc = pc ? pc - 1 : bufSize - 1; // off by one to fool fromOverflow()
			blankBuf = !frames;
		}

		est.reset();
	} else {
		est.feed(((pc >= lastpc ? pc : bufSize + pc) - lastpc) >> 2);
	}

	lastpc = pc;

	unsigned bytes = frames * 4;
	const unsigned maxSpaceWait = (bufSize - bufSzDiff) / 8;

	while (bytes) {
		const int fof = waitForSpace(pc, wc, bytes < maxSpaceWait ? bytes : maxSpaceWait);

		if (fof <= 0)
			return fof;

		const unsigned n = static_cast<unsigned>(fof) < bytes ? static_cast<unsigned>(fof) : bytes;

		if (::write(lpDSB, offset, buffer, n) < 0) {
			std::cerr << "::write fail" << std::endl;
			return -1;
		}

		buffer = static_cast<char*>(buffer) + n;
		bytes -= n;

		offset = limitCursor(offset + n, bufSize);
	}

	if (wc != pc && static_cast<unsigned>(fromOverflow(pc, wc, bufSize)) <= bufSzDiff)
		offset = pc; // cause annoying disturbance to make user realize this is a bad configuration if it happens often.

	return 0;
}

int DirectSoundEngine::getPosAndStatusCheck(DWORD &status, DWORD &pc, DWORD &wc) {
	lpDSB->GetStatus(&status);

	if (status & DSBSTATUS_BUFFERLOST) {
		if (lpDSB->Restore() != DS_OK)
			return -1;

		blankBuf = true;
		status &= ~DSBSTATUS_PLAYING;
	}

	if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK)
		return -2;

	return 0;
}

void DirectSoundEngine::fillBufferState(BufferState &s, const DWORD pc, const DWORD wc) const {
	const int adjustedPc = decCursor(pc, bufSzDiff, bufSize);
	const int fur = fromUnderrun(adjustedPc, wc, offset, bufSize);

	if (fur < 0) {
		s.fromUnderrun = 0;
		s.fromOverflow = fromOverflow(adjustedPc, wc, bufSize) >> 2;
	} else {
		s.fromUnderrun = fur >> 2;
		s.fromOverflow = fromOverflow(adjustedPc, offset, bufSize) >> 2;
	}
}

int DirectSoundEngine::write(void *const buffer, const unsigned frames) {
	DWORD status, pc, wc;

	if (const int ret = getPosAndStatusCheck(status, pc, wc))
		return ret + 1;

	return doWrite(buffer, frames, status, pc, wc);
}

int DirectSoundEngine::write(void *const buffer, const unsigned frames, BufferState &preBufState_out, long &rate_out) {
	DWORD status, pc, wc;

	int ret = getPosAndStatusCheck(status, pc, wc);

	if (ret) {
		preBufState_out.fromOverflow = preBufState_out.fromUnderrun = BufferState::NOT_SUPPORTED;
		ret += 1;
	} else {
		fillBufferState(preBufState_out, pc, wc);
		ret = doWrite(buffer, frames, status, pc, wc);
	}

	rate_out = est.result();

	return ret;
}

const AudioEngine::BufferState DirectSoundEngine::bufferState() const {
	BufferState s;
	DWORD pc, wc;

	if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK) {
		s.fromOverflow = s.fromUnderrun = BufferState::NOT_SUPPORTED;
	} else {
		fillBufferState(s, pc, wc);
	}

	return s;
}

void DirectSoundEngine::pause() {
	if (lpDSB) {
		lpDSB->Stop();
	}
}
