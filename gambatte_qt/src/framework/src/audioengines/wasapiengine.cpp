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

#include "wasapiengine.h"
#include "wasapiinc.h"
#include <MMReg.h>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <cstring>
#include <string>
#include <iostream>

static CLSID const CLSID_MMDeviceEnumerator = {
	0xBCDE0395, 0xE52F, 0x467C,
	{ 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E }
};

static IID const IID_IMMDeviceEnumerator = {
	0xA95664D2, 0x9614, 0x4F35,
	{ 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 }
};

static IID const IID_IAudioClient = {
	0x1CB9AD4C, 0xDBFA, 0x4c32,
	{ 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 }
};

static IID const IID_IAudioRenderClient = {
	0xF294ACFC, 0x3146, 0x4483,
	{ 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2 }
};

static IID const IID_IAudioClock = {
	0xCD63314F, 0x3FBA, 0x4a1b,
	{ 0x81, 0x2C, 0xEF, 0x96, 0x35, 0x87, 0x28, 0xE7 }
};

static PROPERTYKEY const PKEY_Device_FriendlyName = {
	{ 
		0xa45c254e, 0xdf1c, 0x4efd,
		{ 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 }
	},
	14
};

template<typename T>
static void safeRelease(T *&t) {
	if (t) {
		t->Release();
		t = 0;
	}
}

Q_DECLARE_METATYPE(std::wstring);

static void addDeviceSelectorItem(QComboBox *const deviceSelector, IMMDevice *const pEndpoint) {
	LPWSTR pwszID = 0;
	IPropertyStore *pProps = 0;
	pEndpoint->GetId(&pwszID);
	pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
	if (pwszID && pProps) {
		PROPVARIANT varName;
		std::memset(&varName, 0, sizeof varName);
		if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
			deviceSelector->addItem(QString::fromWCharArray(varName.pwszVal),
			                        QVariant::fromValue(std::wstring(pwszID)));
			CoTaskMemFree(varName.pwszVal);
			//PropVariantClear(&varName);
		}

		CoTaskMemFree(pwszID);
		pProps->Release();
	}
}

static void fillDeviceSelector(QComboBox *const deviceSelector) {
	IMMDeviceEnumerator *pEnumerator = 0;
	IMMDeviceCollection *pCollection = 0;
	UINT count = 0;

	CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator,
	                 reinterpret_cast<void **>(&pEnumerator));
	if (pEnumerator) {
		pEnumerator->EnumAudioEndpoints(eRender,
		                                DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED,
		                                &pCollection);
	}

	if (pCollection && FAILED(pCollection->GetCount(&count)))
		count = 0;

	for (ULONG i = 0; i < count; ++i) {
		IMMDevice *pEndpoint = 0;
		pCollection->Item(i, &pEndpoint);
		if (pEndpoint) {
			addDeviceSelectorItem(deviceSelector, pEndpoint);
			pEndpoint->Release();
		}
	}

	safeRelease(pCollection);
	safeRelease(pEnumerator);
}

bool WasapiEngine::isUsable() {
	IMMDeviceEnumerator *pEnumerator = 0;
	HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL,
	                              IID_IMMDeviceEnumerator,
	                              reinterpret_cast<void **>(&pEnumerator));
	if (pEnumerator)
		pEnumerator->Release();

	return SUCCEEDED(hr);
}

WasapiEngine::WasapiEngine()
: AudioEngine("WASAPI")
, confWidget(new QWidget)
, deviceSelector(new QComboBox(confWidget.get()))
, exclusive_(new QCheckBox(QObject::tr("Exclusive mode"), confWidget.get()),
             "wasapiengine/exclusive", false)
, pAudioClient()
, pRenderClient()
, pAudioClock()
, eventHandle_(0)
, pos_(0)
, posFrames(0)
, deviceIndex(0)
, nchannels_(2)
, bufferFrameCount(0)
, started(false)
{
	fillDeviceSelector(deviceSelector);

	{
		QVBoxLayout *const mainLayout = new QVBoxLayout(confWidget.get());
		mainLayout->setMargin(0);

		if (deviceSelector->count() > 1) {
			QHBoxLayout *hlayout = new QHBoxLayout;
			mainLayout->addLayout(hlayout);
			hlayout->addWidget(new QLabel(QObject::tr("WASAPI device:")));
			hlayout->addWidget(deviceSelector);
		} else
			deviceSelector->hide();

		mainLayout->addWidget(exclusive_.checkBox());
	}

	{
		QSettings settings;
		settings.beginGroup("wasapiengine");
		deviceIndex = settings.value("deviceIndex", deviceIndex).toUInt();
		if (deviceIndex >= static_cast<uint>(deviceSelector->count()))
			deviceIndex = 0;

		settings.endGroup();
	}

	rejectSettings();
}

WasapiEngine::~WasapiEngine() {
	uninit();

	QSettings settings;
	settings.beginGroup("wasapiengine");
	settings.setValue("deviceIndex", deviceIndex);
	settings.endGroup();
}

void WasapiEngine::doAcceptSettings() {
	exclusive_.accept();
	deviceIndex = deviceSelector->currentIndex();
}

void WasapiEngine::rejectSettings() const {
	exclusive_.reject();
	deviceSelector->setCurrentIndex(deviceIndex);
}

long WasapiEngine::doInit(long rate, int const latency) {
	{
		IMMDevice *pDevice = 0;
		IMMDeviceEnumerator *pEnumerator = 0;
		HRESULT hr;

		if (FAILED(CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL,
		                            IID_IMMDeviceEnumerator,
		                            reinterpret_cast<void **>(&pEnumerator)))) {
			std::cerr << "CoCreateInstance failed" << std::endl;
			return -1;
		}

		if (deviceSelector->count() > 1) {
			hr = pEnumerator->GetDevice(
				deviceSelector->itemData(deviceIndex).value<std::wstring>().c_str(),
				&pDevice);
		} else
			hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

		pEnumerator->Release();
		if (FAILED(hr)) {
			std::cerr << "pEnumerator->GetDefaultAudioEndpoint failed" << std::endl;
			return -1;
		}

		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, 0,
		                       reinterpret_cast<void **>(&pAudioClient));
		pDevice->Release();
		if (FAILED(hr)) {
			std::cerr << "pDevice->Activate failed" << std::endl;
			return -1;
		}
	}

	{
		static GUID const ksdataformat_subtype_pcm = {
			WAVE_FORMAT_PCM, 0x0000, 0x0010,
			{ 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
		};

		WAVEFORMATEXTENSIBLE wfext;
		std::memset(&wfext, 0, sizeof wfext);
		wfext.Format.cbSize = sizeof wfext - sizeof wfext.Format;
		wfext.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wfext.Format.nChannels = 2;
		wfext.Format.nSamplesPerSec = rate;
		wfext.Format.wBitsPerSample = 16;
		wfext.Samples.wValidBitsPerSample = 16;
		wfext.SubFormat = ksdataformat_subtype_pcm;
		wfext.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

		if (!exclusive_.value()) {
			WAVEFORMATEX *mwfe = 0;
			if (SUCCEEDED(pAudioClient->GetMixFormat(&mwfe)) && mwfe) {
				wfext.Format.nChannels = std::max<int>(mwfe->nChannels, 2);
				wfext.Format.nSamplesPerSec = mwfe->nSamplesPerSec;
				if (mwfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
					wfext.dwChannelMask =
						reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mwfe)->dwChannelMask;
				}

				CoTaskMemFree(mwfe);
			} else
				std::cerr << "pAudioClient->GetMixFormat failed\r\n";

			if (!(eventHandle_ = CreateEventA(0, false, false, 0)))
				std::cerr << "CreateEvent failed\r\n";
		}

		wfext.Format.nBlockAlign = wfext.Format.nChannels * wfext.Format.wBitsPerSample >> 3;
		wfext.Format.nAvgBytesPerSec = wfext.Format.nSamplesPerSec * wfext.Format.nBlockAlign;
		nchannels_ = wfext.Format.nChannels;
		rate = wfext.Format.nSamplesPerSec;

		HRESULT hr = pAudioClient->Initialize(
			exclusive_.value() ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
			eventHandle_ ? AUDCLNT_STREAMFLAGS_EVENTCALLBACK : 0,
			latency * 10000,
			0,
			&wfext.Format,
			0);
		if (FAILED(hr)) {
			std::cerr << "pAudioClient->Initialize failed: " << hr << "\r\n";
			return -1;
		}
	}

	if (eventHandle_ && FAILED(pAudioClient->SetEventHandle(eventHandle_))) {
		std::cerr << "pAudioClient->SetEventHandle failed\r\n";
		return -1;
	}
	if (FAILED(pAudioClient->GetBufferSize(&bufferFrameCount))) {
		std::cerr << "pAudioClient->GetBufferSize failed" << std::endl;
		return -1;
	}
	if (FAILED(pAudioClient->GetService(IID_IAudioRenderClient,
	                                    reinterpret_cast<void **>(&pRenderClient)))) {
		std::cerr << "pAudioClient->GetService failed" << std::endl;
		return -1;
	}
	if (FAILED(pAudioClient->GetService(IID_IAudioClock,
	                                    reinterpret_cast<void **>(&pAudioClock)))) {
		std::cerr << "pAudioClient->GetService failed" << std::endl;
		return -1;
	}

	{
		UINT64 freq = 0;
		pAudioClock->GetFrequency(&freq);
		posFrames = freq / (rate ? rate : 1);
	}

	pos_ = 0;
	est = RateEst(rate, bufferFrameCount);

	return rate;
}

void WasapiEngine::uninit() {
	safeRelease(pAudioClock);
	safeRelease(pRenderClient);
	safeRelease(pAudioClient);

	if (eventHandle_ && !CloseHandle(eventHandle_))
		std::cerr << "CloseHandle failed\r\n";

	eventHandle_ = 0;
	started = false;
}

int WasapiEngine::waitForSpace(UINT32 &numFramesPadding, UINT32 const space) {
	for (int n = eventHandle_ ? 10 : 100; n-- && bufferFrameCount - numFramesPadding < space;) {
		if (!started) {
			if (FAILED(pAudioClient->Start()))
				return -1;

			started = true;
		}

		if (eventHandle_) {
			WaitForSingleObject(eventHandle_, 11);
		} else
			Sleep(1);

		if (FAILED(pAudioClient->GetCurrentPadding(&numFramesPadding)))
			return -1;
	}

	return bufferFrameCount - numFramesPadding;
}

static int write(IAudioRenderClient *const pRenderClient,
		void *const buffer, std::size_t const frames, int const nchannels) {
	BYTE *pData = 0;
	if (FAILED(pRenderClient->GetBuffer(frames, &pData)))
		return -1;

	if (nchannels > 2) {
		std::memset(pData, 0, frames * nchannels * 2);
		for (std::size_t i = 0; i < frames; ++i) {
			*reinterpret_cast<UINT32 *>(pData + i * nchannels * 2) =
				static_cast<UINT32 const *>(buffer)[i];
		}
	} else
		std::memcpy(pData, buffer, frames * 4);

	pRenderClient->ReleaseBuffer(frames, 0);

	return 0;
}

int WasapiEngine::write(void *buffer, std::size_t frames, UINT32 numFramesPadding) {
	if (posFrames) {
		UINT64 pos = 0;
		UINT64 qpcpos = 0;
		if (started && numFramesPadding) {
			if (SUCCEEDED(pAudioClock->GetPosition(&pos, &qpcpos)) && pos_) {
				est.feed((static_cast<UINT32>(pos) - pos_) / posFrames,
				         qpcpos / 10);
			} else
				est.resetLastFeedTimeStamp();
		}

		pos_ = pos;
	}

	std::size_t const maxSpaceWait = bufferFrameCount / 8;

	while (frames) {
		int const fof = waitForSpace(numFramesPadding,
		                             std::min(frames, maxSpaceWait));
		if (fof <= 0)
			return fof;

		std::size_t const n = std::min(std::size_t(fof), frames);
		if (::write(pRenderClient, buffer, n, nchannels_) < 0) {
			std::cerr << "::write fail" << std::endl;
			return -1;
		}

		buffer = static_cast<char *>(buffer) + n * 4;
		frames -= n;
		numFramesPadding += n;
	}

	return 0;
}

int WasapiEngine::write(void *buffer, std::size_t samples) {
	UINT32 numFramesPadding = 0;
	if (FAILED(pAudioClient->GetCurrentPadding(&numFramesPadding)))
		return -1;

	return write(buffer, samples, numFramesPadding);
}

int WasapiEngine::write(
		void *buffer, std::size_t samples,
		BufferState &preBufState_out, long &rate_out) {
	UINT32 numFramesPadding = 0;
	if (FAILED(pAudioClient->GetCurrentPadding(&numFramesPadding)))
		return -1;

	preBufState_out.fromUnderrun = numFramesPadding;
	preBufState_out.fromOverflow = bufferFrameCount - numFramesPadding;

	int ret = write(buffer, samples, numFramesPadding);
	rate_out = est.result();
	return ret;
}

AudioEngine::BufferState WasapiEngine::bufferState() const {
	BufferState s;
	UINT32 numFramesPadding = 0;
	if (FAILED(pAudioClient->GetCurrentPadding(&numFramesPadding))) {
		s.fromOverflow = s.fromUnderrun = BufferState::not_supported;
	} else {
		s.fromUnderrun = numFramesPadding;
		s.fromOverflow = bufferFrameCount - numFramesPadding;
	}

	return s;
}

void WasapiEngine::pause() {
	if (pAudioClient && started) {
		pAudioClient->Stop();
		started = false;
	}
}
