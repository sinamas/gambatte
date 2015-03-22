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

#include "directsoundengine.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cstring>
#include <iostream>

Q_DECLARE_METATYPE(GUID *)

BOOL CALLBACK DirectSoundEngine::enumCallback(LPGUID guid,
		char const *description, char const */*module*/, LPVOID context) {
	if (guid) {
		DirectSoundEngine *thisptr = static_cast<DirectSoundEngine *>(context);
		thisptr->deviceList.append(*guid);
		thisptr->deviceSelector->addItem(description,
		                                 QVariant::fromValue(&thisptr->deviceList.last()));
	}

	return true;
}

DirectSoundEngine::DirectSoundEngine(HWND hwnd_in)
: AudioEngine("DirectSound")
, confWidget(new QWidget)
, deviceSelector(new QComboBox(confWidget.get()))
, primaryBufBox(new QCheckBox(QObject::tr("Write to primary buffer"), confWidget.get()))
, globalBufBox(new QCheckBox(QObject::tr("Global buffer"), confWidget.get()))
, lpDS()
, lpDSB()
, bufSize(0)
, bufSzDiff(0)
, deviceIndex(0)
, offset(0)
, lastpc(0)
, hwnd(hwnd_in)
, primaryBuf(false)
, useGlobalBuf(false)
, blankBuf(true)
{
	DirectSoundEnumerateA(enumCallback, this);
	if (deviceSelector->count() < 1)
		deviceSelector->addItem(QString(), QVariant::fromValue<GUID *>(0));

	{
		QVBoxLayout *const mainLayout = new QVBoxLayout(confWidget.get());
		mainLayout->setMargin(0);

		if (deviceSelector->count() > 1) {
			QHBoxLayout *const hlayout = new QHBoxLayout;
			mainLayout->addLayout(hlayout);
			hlayout->addWidget(new QLabel(QObject::tr("DirectSound device:")));
			hlayout->addWidget(deviceSelector);
		} else
			deviceSelector->hide();

		mainLayout->addWidget(primaryBufBox);
		mainLayout->addWidget(globalBufBox);
	}

	{
		QSettings settings;
		settings.beginGroup("directsoundengine");
		primaryBuf = settings.value("primaryBuf", primaryBuf).toBool();
		useGlobalBuf = settings.value("useGlobalBuf", useGlobalBuf).toBool();
		deviceIndex = settings.value("deviceIndex", deviceIndex).toUInt();
		if (deviceIndex >= static_cast<uint>(deviceSelector->count()))
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

long DirectSoundEngine::doInit(long const rate, int const latency) {
	if (DirectSoundCreate(deviceSelector->itemData(deviceIndex).value<GUID *>(), &lpDS, 0) != DS_OK) {
		lpDS = 0;
		std::cerr << "DirectSoundCreate failed" << std::endl;
		return -1;
	}
	if (lpDS->SetCooperativeLevel(hwnd, primaryBuf ? DSSCL_WRITEPRIMARY : DSSCL_PRIORITY) != DS_OK) {
		std::cerr << "SetCooperativeLevel failed" << std::endl;
		return -1;
	}

	{
		DSBUFFERDESC dsbd;
		std::memset(&dsbd, 0, sizeof dsbd);
		dsbd.dwSize = sizeof dsbd;
		dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2
		             | (primaryBuf ? DSBCAPS_PRIMARYBUFFER : 0)
		             | (useGlobalBuf && !primaryBuf ? DSBCAPS_GLOBALFOCUS : 0);

		WAVEFORMATEX wfe;
		std::memset(&wfe, 0, sizeof wfe);
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.nChannels = 2;
		wfe.nSamplesPerSec = rate;
		wfe.wBitsPerSample = 16;
		wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample >> 3;
		wfe.nAvgBytesPerSec = rate * wfe.nBlockAlign;

		DWORD const desiredBufSz = ((rate * latency + 500) / 1000) * 4;

		if (!primaryBuf) {
			dsbd.lpwfxFormat = &wfe;
			dsbd.dwBufferBytes = desiredBufSz;
			dsbd.dwBufferBytes = std::max<DWORD>(dsbd.dwBufferBytes, DSBSIZE_MIN);
			dsbd.dwBufferBytes = std::min<DWORD>(dsbd.dwBufferBytes, DSBSIZE_MAX);
		}

		{
			GUID guidNULL;
			std::memset(&guidNULL, 0, sizeof guidNULL);
			dsbd.guid3DAlgorithm = guidNULL;
		}

		if (lpDS->CreateSoundBuffer(&dsbd, &lpDSB, 0) != DS_OK) {
			lpDSB = 0;
			std::cerr << "CreateSoundBuffer failed" << std::endl;
			return -1;
		}
		if (primaryBuf && lpDSB->SetFormat(&wfe) != DS_OK) {
			std::cerr << "lpDSB->SetFormat failed" << std::endl;
			return -1;
		}

		DSBCAPS dsbcaps;
		std::memset(&dsbcaps, 0, sizeof dsbcaps);
		dsbcaps.dwSize = sizeof dsbcaps;
		if (lpDSB->GetCaps(&dsbcaps) != DS_OK)
			return -1;

		bufSize = dsbcaps.dwBufferBytes;

		// We use bufSzDiff to only use the lower desiredBufSz bytes of the buffer for
		// effective latency closer to the desired one, since we cannot set the primary
		// buffer size.
		bufSzDiff = primaryBuf && desiredBufSz < bufSize
		          ? bufSize - desiredBufSz
		          : 0;
	}

	// set offset for meaningful initial bufferState() results.
	if (lpDSB->GetCurrentPosition(&offset, 0) != DS_OK)
		offset = 0;

	offset += 1;
	blankBuf = true;
	est = RateEst(rate, bufSize >> 2);
	return rate;
}

void DirectSoundEngine::uninit() {
	if (lpDSB) {
		lpDSB->Stop();
		lpDSB->Release();
		lpDSB = 0;
	}

	if (lpDS) {
		lpDS->Release();
		lpDS = 0;
	}
}

static DWORD limitCursor(DWORD cursor, DWORD bufSz) {
	if (cursor >= bufSz)
		cursor -= bufSz;

	return cursor;
}

static int fromUnderrun(int pc, int wc, int offset, int bufSize) {
	return (offset > pc ? offset : offset + bufSize) - (wc < pc ? wc + bufSize : wc);
}

static int fromOverflow(int pc, int offset, int bufSize) {
	if ((pc -= offset) < 0)
		pc += bufSize;

	return pc;
}

static inline int decCursor(int cursor, int dec, int bufSz) {
	return fromOverflow(cursor, dec, bufSz);
}

int DirectSoundEngine::waitForSpace(DWORD &pc, DWORD &wc, DWORD const space) {
	int fof = 0;
	int n = 100;
	int const adjustedOffset = limitCursor(offset + bufSzDiff, bufSize);

	while (n-- && static_cast<DWORD>(fof = fromOverflow(pc, adjustedOffset, bufSize)) < space) {
		{
			DWORD status;
			lpDSB->GetStatus(&status);

			if (!(status & DSBSTATUS_PLAYING)) {
				HRESULT res = lpDSB->Play(0, 0, DSBPLAY_LOOPING);
				if (res != DS_OK) {
					if (res != DSERR_BUFFERLOST)
						return -1;

					break;
				}
			}
		}

		Sleep(1); // blerk

		if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK)
			return -1;
	}

	{
		// Decrementing pc has the same effect as incrementing offset as far as fromOverflow is concerned,
		// but adjusting offset doesn't work well for fromUnderrun. We adjust offset rather than pc in the loop
		// because it changes less often.
		int const adjustedPc = decCursor(pc, bufSzDiff, bufSize);
		if (fromUnderrun(adjustedPc, wc, offset, bufSize) < 0) {
			//std::cout << "underrun" << std::endl;
			offset = wc;
			fof = fromOverflow(adjustedPc, offset, bufSize);
		}
	}

	return fof;
}

static int write(LPDIRECTSOUNDBUFFER lpDSB, DWORD const offset, void *const buffer, DWORD const bytes) {
	LPVOID ptr1;
	LPVOID ptr2;
	DWORD bytes1;
	DWORD bytes2;

	{
		HRESULT res = lpDSB->Lock(offset, bytes, &ptr1, &bytes1, &ptr2, &bytes2, 0);
		if (res != DS_OK)
			return res == DSERR_BUFFERLOST ? 0 : -1;
	}

	std::memcpy(ptr1, buffer, bytes1);
	if (ptr2)
		std::memcpy(ptr2, static_cast<char *>(buffer) + bytes1, bytes2);

	lpDSB->Unlock(ptr1, bytes1, ptr2, bytes2);

	return 0;
}

int DirectSoundEngine::doWrite(
		void *buffer, std::size_t const frames, DWORD const status, DWORD pc, DWORD wc) {
	if (!(status & DSBSTATUS_PLAYING)) {
		if (blankBuf) { // make sure we write from pc, so no uninitialized samples are played
			offset = wc = pc;
			pc = pc ? pc - 1 : bufSize - 1; // off by one to fool fromOverflow()
			blankBuf = !frames;
		}

		est.resetLastFeedTimeStamp();
	} else {
		est.feed(((pc >= lastpc ? pc : bufSize + pc) - lastpc) >> 2);
	}

	lastpc = pc;

	std::size_t bytes = frames * 4;
	std::size_t const maxSpaceWait = (bufSize - bufSzDiff) / 8;

	while (bytes) {
		int const fof = waitForSpace(pc, wc, std::min(bytes, maxSpaceWait));
		if (fof <= 0)
			return fof;

		DWORD const n = std::min(std::size_t(fof), bytes);
		if (::write(lpDSB, offset, buffer, n) < 0) {
			std::cerr << "::write fail" << std::endl;
			return -1;
		}

		buffer = static_cast<char *>(buffer) + n;
		bytes -= n;
		offset = limitCursor(offset + n, bufSize);
	}

	if (wc != pc && static_cast<DWORD>(fromOverflow(pc, wc, bufSize)) <= bufSzDiff) {
		// bad. cause some disturbance so it does not go unnoticed if it happens a lot.
		offset = pc;
	}

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

void DirectSoundEngine::fillBufferState(BufferState &s, DWORD const pc, DWORD const wc) const {
	int const adjustedPc = decCursor(pc, bufSzDiff, bufSize);
	int const fur = fromUnderrun(adjustedPc, wc, offset, bufSize);
	if (fur < 0) {
		s.fromUnderrun = 0;
		s.fromOverflow = fromOverflow(adjustedPc, wc, bufSize) >> 2;
	} else {
		s.fromUnderrun = fur >> 2;
		s.fromOverflow = fromOverflow(adjustedPc, offset, bufSize) >> 2;
	}
}

int DirectSoundEngine::write(void *buffer, std::size_t frames) {
	DWORD status, pc, wc;
	if (int ret = getPosAndStatusCheck(status, pc, wc))
		return ret + 1;

	return doWrite(buffer, frames, status, pc, wc);
}

int DirectSoundEngine::write(
		void *const buffer, std::size_t const frames,
		BufferState &preBufState_out, long &rate_out) {
	DWORD status, pc, wc;
	int ret = getPosAndStatusCheck(status, pc, wc);
	if (ret) {
		preBufState_out.fromOverflow = preBufState_out.fromUnderrun = BufferState::not_supported;
		ret += 1;
	} else {
		fillBufferState(preBufState_out, pc, wc);
		ret = doWrite(buffer, frames, status, pc, wc);
	}

	rate_out = est.result();
	return ret;
}

AudioEngine::BufferState DirectSoundEngine::bufferState() const {
	BufferState s;
	DWORD pc, wc;
	if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK) {
		s.fromOverflow = s.fromUnderrun = BufferState::not_supported;
	} else {
		fillBufferState(s, pc, wc);
	}

	return s;
}

void DirectSoundEngine::pause() {
	if (lpDSB)
		lpDSB->Stop();
}
