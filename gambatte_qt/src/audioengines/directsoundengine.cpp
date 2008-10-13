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
	globalBufBox(new QCheckBox("Global buffer")),
	deviceSelector(new QComboBox),
	lpDS(NULL),
	lpDSB(NULL),
	bufSize(0),
	deviceIndex(0),
	offset(0),
	hwnd(hwnd_in),
	useGlobalBuf(false)
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
		
		mainLayout->addWidget(globalBufBox);
		confWidget->setLayout(mainLayout);
	}
	
	{
		QSettings settings;
		settings.beginGroup("directsoundengine");
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
	settings.setValue("useGlobalBuf", useGlobalBuf);
	settings.setValue("deviceIndex", deviceIndex);
	settings.endGroup();
}

void DirectSoundEngine::acceptSettings() {
	useGlobalBuf = globalBufBox->isChecked();
	deviceIndex = deviceSelector->currentIndex();
}

void DirectSoundEngine::rejectSettings() {
	globalBufBox->setChecked(useGlobalBuf);
	deviceSelector->setCurrentIndex(deviceIndex);
}

static unsigned nearestPowerOf2(const unsigned in) {
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
}

int DirectSoundEngine::doInit(const int rate, const unsigned latency) {
	if (DirectSoundCreate(deviceSelector->itemData(deviceIndex).value<GUID*>(), &lpDS, NULL) != DS_OK) {
		lpDS = NULL;
		goto fail;
	}
	
	if (lpDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY) != DS_OK)
		goto fail;
	
	{
		DSBUFFERDESC dsbd;
		std::memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | (useGlobalBuf ? DSBCAPS_GLOBALFOCUS : 0);
		
		{
			/*int bufferSize = (((rate * 4389) / 262144) + 1) * 8 * 4;
			
			--bufferSize;
			bufferSize |= bufferSize >> 1;
			bufferSize |= bufferSize >> 2;
			bufferSize |= bufferSize >> 4;
			bufferSize |= bufferSize >> 8;
			bufferSize |= bufferSize >> 16;
			++bufferSize;*/
			
			int bufferSize = nearestPowerOf2(((rate * latency + 500) / 1000) * 4);
			
			if (bufferSize < DSBSIZE_MIN)
				bufferSize = DSBSIZE_MIN;
			
			if (bufferSize > DSBSIZE_MAX)
				bufferSize = DSBSIZE_MAX;
			
			dsbd.dwBufferBytes = bufSize = bufferSize;
		}
		
		WAVEFORMATEX wfe;
		std::memset(&wfe, 0, sizeof(wfe));
		wfe.wFormatTag = WAVE_FORMAT_PCM;
		wfe.nChannels = 2;
		wfe.nSamplesPerSec = rate;
		wfe.wBitsPerSample = 16;
		wfe.nBlockAlign = wfe.nChannels * wfe.wBitsPerSample >> 3;
		wfe.nAvgBytesPerSec = rate * wfe.nBlockAlign;
		
		dsbd.lpwfxFormat = &wfe;
		
		{
			GUID guidNULL;
			std::memset(&guidNULL,0,sizeof(GUID));
			dsbd.guid3DAlgorithm = guidNULL;
		}
		
		if (lpDS->CreateSoundBuffer(&dsbd, &lpDSB, NULL) != DS_OK) {
			lpDSB = NULL;
			goto fail;
		}
	}
	
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

int DirectSoundEngine::write(void *const buffer, const unsigned frames) {
	DWORD status;
	lpDSB->GetStatus(&status);
	
	if (status & DSBSTATUS_BUFFERLOST) {
		lpDSB->Restore();
		status &= ~DSBSTATUS_PLAYING;
	}
	
	if (!(status & DSBSTATUS_PLAYING)) {
// 		std:: cout << "notplaying" << std::endl;
		offset = bufSize >> 1;
		lpDSB->SetCurrentPosition(0);
	} else for (DWORD pc, wc;;) {
		if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK)
			return -1;
		
		//std::cout << "wc-pc: " << ((wc < pc ? bufSize : 0) + wc - pc) << std::endl;
		if (offset > pc && offset < wc) {
// 			std::cout << "underrun" << std::endl;
			offset = wc;
			break;
		}
		
		if ((pc < offset ? bufSize : 0) + pc - offset >= frames * 4)
			break;
		
		Sleep(1);
	}
	
	{
		LPVOID ptr1;
		LPVOID ptr2;
		DWORD bytes1;
		DWORD bytes2;
		
		if (lpDSB->Lock(offset, frames * 4, &ptr1, &bytes1, &ptr2, &bytes2, 0) != DS_OK)
			return 0;
		
		std::memcpy(ptr1, buffer, bytes1);
		
		if (ptr2) {
			std::memcpy(ptr2, static_cast<char*>(buffer) + bytes1, bytes2);
		}
		
		lpDSB->Unlock(ptr1, bytes1, ptr2, bytes2);
	}
	
	if ((offset += frames * 4) >= bufSize)
		offset -= bufSize;
	
	if (!(status & DSBSTATUS_PLAYING)) {
		lpDSB->Play(0, 0, DSBPLAY_LOOPING);
	}
	
	return 0;
}

const AudioEngine::BufferState DirectSoundEngine::bufferState() const {
	BufferState s;
	DWORD pc, wc;
	
	if (lpDSB->GetCurrentPosition(&pc, &wc) != DS_OK) {
		s.fromOverflow = s.fromUnderrun = BufferState::NOT_SUPPORTED;
	} else if (offset > pc && offset < wc) {
		s.fromUnderrun = 0;
		s.fromOverflow = bufSize >> 2;
	} else {
		s.fromUnderrun = (offset < wc ? bufSize : 0) + offset - wc >> 2;
		s.fromOverflow = (pc < offset ? bufSize : 0) + pc - offset >> 2;
	}
	
	return s;
}

void DirectSoundEngine::pause() {
	if (lpDSB) {
		lpDSB->Stop();
	}
}
