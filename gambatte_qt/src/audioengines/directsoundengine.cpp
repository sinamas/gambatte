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
#include "directsoundengine.h"

#include <iostream>
#include <cstring>

DirectSoundEngine::DirectSoundEngine(HWND hwnd_in) : lpDS(NULL), lpDSB(NULL), hwnd(hwnd_in) {}

DirectSoundEngine::~DirectSoundEngine() {
	uninit();
}

int DirectSoundEngine::init() {
	unsigned rate = 48000;
	
	if (DirectSoundCreate(NULL, &lpDS, NULL) != DS_OK) {
		lpDS = NULL;
		goto fail;
	}
	
	if (lpDS->SetCooperativeLevel(hwnd, DSSCL_PRIORITY) != DS_OK)
		goto fail;
	
	{
		DSBUFFERDESC dsbd;
		std::memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		
		{
			int bufferSize = (((rate * 4389) / 262144) + 1) * 8 * 4;
			
			bufferSize |= bufferSize >> 1;
			bufferSize |= bufferSize >> 2;
			bufferSize |= bufferSize >> 4;
			bufferSize |= bufferSize >> 8;
			bufferSize |= bufferSize >> 16;
			++bufferSize;
			
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
			rate = 44100;
			wfe.nSamplesPerSec = rate;
			wfe.nAvgBytesPerSec = rate * wfe.nBlockAlign;
			
			if (lpDS->CreateSoundBuffer(&dsbd, &lpDSB, NULL) != DS_OK) {
				lpDSB = NULL;
				goto fail;
			}
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
		s.fromOverflow = s.fromUnderrun = bufSize >> 2;
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
