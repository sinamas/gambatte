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

#include "directdrawblitter.h"
#include "../dwmcontrol.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <cstring>
#include <iostream>

Q_DECLARE_METATYPE(GUID *)

BOOL WINAPI DirectDrawBlitter::enumCallback(GUID FAR *lpGUID,
                                            char *lpDriverDescription,
                                            char * ,
                                            LPVOID context,
                                            HMONITOR )
{
	DirectDrawBlitter *const thisptr = static_cast<DirectDrawBlitter *>(context);
	GUID *guidptr = 0;
	if (lpGUID) {
		thisptr->deviceList.append(*lpGUID);
		guidptr = &thisptr->deviceList.last();
	}

	thisptr->deviceSelector->addItem(lpDriverDescription, QVariant::fromValue(guidptr));

	return true;
}

DirectDrawBlitter::DirectDrawBlitter(VideoBufferLocker vbl, QWidget *parent)
: BlitterWidget(vbl, "DirectDraw", 2, parent)
, confWidget(new QWidget)
, deviceSelector(new QComboBox(confWidget.get()))
, vblank_(new QCheckBox(tr("Wait for vertical blank"), confWidget.get()),
          "directdrawblitter/vblank", false)
, flipping_(new QCheckBox(tr("Exclusive full screen"), confWidget.get()),
            "directdrawblitter/flipping", false)
, vblankflip_(new QCheckBox(tr("Flip during vertical blank"), confWidget.get()),
              "directdrawblitter/vblankflip", true)
, triplebuf_(new QCheckBox(tr("Triple buffering"), confWidget.get()),
             "directdrawblitter/triplebuf", false)
, videoSurface_(new QCheckBox(tr("Use video memory surface"), confWidget.get()),
                "directdrawblitter/videoSurface", true)
, lpDD()
, lpDDSPrimary()
, lpDDSBack()
, lpDDSSystem()
, lpDDSVideo()
, lpDDSClear()
, lpClipper()
, lastblank(0)
, clear(0)
, dhz(600)
, swapInterval(0)
, deviceIndex(0)
, exclusive(false)
, blitted(false)
{
	setAttribute(Qt::WA_PaintOnScreen, true);

	DirectDrawEnumerateExA(enumCallback, this, DDENUM_ATTACHEDSECONDARYDEVICES);
	if (deviceSelector->count() < 1)
		deviceSelector->addItem(QString(), QVariant::fromValue<GUID *>(0));

	QSettings settings;
	deviceIndex = settings.value("directdrawblitter/deviceIndex", deviceIndex).toUInt();
	if (deviceIndex >= static_cast<uint>(deviceSelector->count()))
		deviceIndex = 0;

	QVBoxLayout *const mainLayout = new QVBoxLayout(confWidget.get());
	mainLayout->setMargin(0);

	if (deviceSelector->count() > 2) {
		QHBoxLayout *hlayout = new QHBoxLayout;
		mainLayout->addLayout(hlayout);
		hlayout->addWidget(new QLabel(tr("DirectDraw device:")));
		hlayout->addWidget(deviceSelector);
	} else
		deviceSelector->hide();

	mainLayout->addWidget(vblank_.checkBox());
	vblank_.checkBox()->setToolTip(tr("Prevents tearing. Does not work well on all systems.\n"
	                                  "Ignored when exclusive full screen or DWM composition is active."));
	mainLayout->addWidget(flipping_.checkBox());
	flipping_.checkBox()->setToolTip(tr("Grabs device for better performance when full screen."));

	{
		QHBoxLayout *l = new QHBoxLayout;
		mainLayout->addLayout(l);
		l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
		l->addWidget(vblankflip_.checkBox());
		vblankflip_.checkBox()->setToolTip(tr("Prevents tearing. Recommended."));
		l = new QHBoxLayout;
		mainLayout->addLayout(l);
		l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
		l->addWidget(triplebuf_.checkBox());
	}

	mainLayout->addWidget(videoSurface_.checkBox());

	vblankflip_.checkBox()->setEnabled(flipping_.checkBox()->isChecked());
	triplebuf_.checkBox()->setEnabled(flipping_.checkBox()->isChecked());
	connect(flipping_.checkBox(), SIGNAL(toggled(bool)),
	        vblankflip_.checkBox(), SLOT(setEnabled(bool)));
	connect(flipping_.checkBox(), SIGNAL(toggled(bool)),
	        triplebuf_.checkBox(), SLOT(setEnabled(bool)));

	rejectSettings();
}

DirectDrawBlitter::~DirectDrawBlitter() {
	uninit();

	QSettings settings;
	settings.beginGroup("directdrawblitter");
	settings.setValue("deviceIndex", deviceIndex);
	settings.endGroup();
}

void DirectDrawBlitter::videoSurfaceBlit() {
	HRESULT ddrval = DD_OK;
	for (int n = 2; n-- && lpDDSSystem && lpDDSVideo;) {
		lpDDSSystem->PageLock(0);
		lpDDSVideo->PageLock(0);
		ddrval = lpDDSVideo->BltFast(0, 0, lpDDSSystem, 0, DDBLTFAST_WAIT);
		lpDDSVideo->PageUnlock(0);
		lpDDSSystem->PageUnlock(0);

		if (ddrval == DDERR_SURFACELOST) {
			restoreSurfaces();
		} else
			break;
	}

	if (ddrval != DD_OK)
		std::cerr << "lpDDSVideo->BltFast() failed" << std::endl;
}

void DirectDrawBlitter::systemSurfaceBlit() {
	LPDIRECTDRAWSURFACE7 lpDDSTmp = lpDDSSystem;
	lpDDSSystem = lpDDSVideo;
	lpDDSVideo = lpDDSTmp;
}

void DirectDrawBlitter::consumeBuffer(SetBuffer setInputBuffer) {
	if (!lpDDSSystem || !lpDDSVideo)
		return;

	lpDDSSystem->Unlock(0);
	if (videoSurface_.value())
		videoSurfaceBlit();
	else
		systemSurfaceBlit();

	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof ddsd;
	if (lpDDSSystem->Lock(0, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, 0) != DD_OK) {
		std::cerr << "lpDDSSystem->Lock() failed" << std::endl;
		ddsd.lpSurface = 0;
	}

	std::ptrdiff_t pitch = inBuffer().pixelFormat == PixelBuffer::RGB16
	                     ? ddsd.lPitch >> 1
	                     : ddsd.lPitch >> 2;
	setInputBuffer(ddsd.lpSurface, inBuffer().pixelFormat, pitch);
}

void DirectDrawBlitter::draw() {
	if (exclusive && flipping_.value())
		finalBlit(DDBLT_WAIT);
}

void DirectDrawBlitter::initPrimarySurface() {
	if (!lpClipper)
		return;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof ddsd);
	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (exclusive && flipping_.value()) {
		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = triplebuf_.value() ? 2 : 1;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
	}

	if (lpDD->CreateSurface(&ddsd, &lpDDSPrimary, 0) != DD_OK) {
		std::cerr << "lpDD->CreateSurface() failed" << std::endl;
		lpDDSBack = lpDDSPrimary = 0;
	} else {
		lpDDSBack = lpDDSPrimary;
		if (exclusive && flipping_.value()) {
			ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
			if (lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack) != DD_OK) {
				std::cerr << "lpDDSPrimary->GetAttachedSurface() failed" << std::endl;
				lpDDSBack = 0;
			}
		}

		if (lpDDSPrimary->SetClipper(lpClipper) != DD_OK)
			std::cerr << "SetClipper failed" << std::endl;
	}
}

static void initSubSurface(IDirectDraw7 *const lpDD,
                           IDirectDrawSurface7 *const lpDDSSystem,
                           IDirectDrawSurface7 *&lpDDSOut,
                           DWORD const w, DWORD const h,
                           DWORD const dwCaps)
{
	if (!lpDDSSystem)
		return;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof ddsd);
	ddsd.dwSize = sizeof ddsd;
	lpDDSSystem->GetSurfaceDesc(&ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | dwCaps;
	if (w)
		ddsd.dwWidth = w;
	if (h)
		ddsd.dwHeight = h;

	if (lpDD->CreateSurface(&ddsd, &lpDDSOut, 0) != DD_OK) {
		lpDDSOut = 0;
		std::cerr << "lpDD->CreateSurface() failed" << std::endl;
	}
}

void DirectDrawBlitter::initVideoSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSVideo, 0, 0,
	               videoSurface_.value() ? 0 : DDSCAPS_SYSTEMMEMORY);
}

void DirectDrawBlitter::initClearSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSClear, 1, 1, 0);
	if (lpDDSClear) {
		DDSURFACEDESC2 ddsd;
		ddsd.dwSize = sizeof ddsd;
		if (lpDDSClear->Lock(0, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, 0) != DD_OK) {
			std::cerr << "lpDDSClear->Lock() failed" << std::endl;
			lpDDSClear->Release();
			lpDDSClear = 0;
		} else {
			std::memset(ddsd.lpSurface, 0, ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
			lpDDSClear->Unlock(0);
		}
	}

	clear = 2 + triplebuf_.value();
}

void DirectDrawBlitter::init() {
	if (DirectDrawCreateEx(deviceSelector->itemData(deviceIndex).value<GUID *>(),
	                       reinterpret_cast<void **>(&lpDD), IID_IDirectDraw7, 0) != DD_OK) {
		std::cerr << "DirectDrawCreateEx failed" << std::endl;
		return uninit();
	}

	DWORD const sclFlags = exclusive && flipping_.value()
	                     ? DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN
	                     : DDSCL_NORMAL;
	if (lpDD->SetCooperativeLevel(parentWidget()->parentWidget()->winId(), sclFlags) != DD_OK) {
		std::cerr << "SetCooperativeLevel failed" << std::endl;
		return uninit();
	}
	if (lpDD->CreateClipper(0, &lpClipper, 0) != DD_OK) {
		std::cerr << "CreateClipper failed" << std::endl;
		return uninit();
	}
	if (lpClipper->SetHWnd(0, winId()) != DD_OK) {
		std::cerr << "SetHWnd failed" << std::endl;
		return uninit();
	}

	initPrimarySurface();
}

void DirectDrawBlitter::restoreSurfaces() {
	if (!lpDDSPrimary || !lpDDSVideo || !lpDDSClear)
		return;

	lpDDSPrimary->Restore();
	lpDDSVideo->Restore();
	lpDDSClear->Restore();
	if (lpDDSPrimary->IsLost() == DDERR_SURFACELOST
			|| lpDDSVideo->IsLost() == DDERR_SURFACELOST
			|| lpDDSClear->IsLost() == DDERR_SURFACELOST) {
		lpDDSPrimary->Release();
		initPrimarySurface();
		if (videoSurface_.value()) {
			lpDDSVideo->Release();
			initVideoSurface();
		}

		lpDDSClear->Release();
		initClearSurface();
	}
}

static void setDdPf(DDPIXELFORMAT *const ddpf,
                    PixelBuffer::PixelFormat *const pixelFormat,
                    LPDIRECTDRAWSURFACE7 lpDDSPrimary)
{
	bool alpha = false;

	ddpf->dwSize = sizeof *ddpf;
	if (lpDDSPrimary
			&& lpDDSPrimary->GetPixelFormat(ddpf) == DD_OK
			&& (ddpf->dwFlags & DDPF_RGB)
			&& ddpf->dwRGBBitCount == 16) {
		*pixelFormat = PixelBuffer::RGB16;
	} else {
		*pixelFormat = PixelBuffer::RGB32;
		alpha = ddpf->dwFlags & DDPF_ALPHAPIXELS;
	}

	std::memset(ddpf, 0, sizeof *ddpf);
	ddpf->dwFlags = DDPF_RGB;
	if (*pixelFormat == PixelBuffer::RGB16) {
		ddpf->dwRGBBitCount = 16;
		ddpf->dwRBitMask = 0xF800;
		ddpf->dwGBitMask = 0x07E0;
		ddpf->dwBBitMask = 0x001F;
	} else {
		ddpf->dwRGBBitCount = 32;
		ddpf->dwRBitMask = 0x00FF0000;
		ddpf->dwGBitMask = 0x0000FF00;
		ddpf->dwBBitMask = 0x000000FF;
		if (alpha) {
			ddpf->dwFlags |= DDPF_ALPHAPIXELS;
			ddpf->dwRGBAlphaBitMask = 0xFF000000;
		}
	}
}

void DirectDrawBlitter::setBufferDimensions(unsigned const w, unsigned const h,
                                            SetBuffer setInputBuffer)
{
	blitted = false;

	if (!lpDDSPrimary)
		return;

	if (lpDDSSystem) {
		lpDDSSystem->Release();
		lpDDSSystem = 0;
	}
	if (lpDDSVideo) {
		lpDDSVideo->Release();
		lpDDSVideo = 0;
	}
	if (lpDDSClear) {
		lpDDSClear->Release();
		lpDDSClear = 0;
	}

	DDPIXELFORMAT ddpf;
	PixelBuffer::PixelFormat pixelFormat;
	setDdPf(&ddpf, &pixelFormat, lpDDSPrimary);
	setInputBuffer(0, pixelFormat, 0);

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof ddsd);
	ddsd.dwSize = sizeof ddsd;
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddpfPixelFormat = ddpf;
	if (lpDD->CreateSurface(&ddsd, &lpDDSSystem, 0) != DD_OK) {
		std::cerr << "lpDD->CreateSurface() failed" << std::endl;
		return uninit();
	}

	initVideoSurface();
	initClearSurface();
	if (!lpDDSVideo || !lpDDSClear)
		return uninit();

	if (lpDDSSystem->Lock(0, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT, 0) != DD_OK) {
		std::cerr << "lpDDSSystem->Lock() failed" << std::endl;
		return uninit();
	}

	setInputBuffer(ddsd.lpSurface, pixelFormat,
	               pixelFormat == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2));
}

void DirectDrawBlitter::uninit() {
	lpDDSBack = 0;

	if (lpClipper) {
		lpClipper->Release();
		lpClipper = 0;
	}
	if (lpDDSPrimary) {
		lpDDSPrimary->Release();
		lpDDSPrimary = 0;
	}
	if (lpDDSSystem) {
		lpDDSSystem->Release();
		lpDDSSystem = 0;
	}
	if (lpDDSVideo) {
		lpDDSVideo->Release();
		lpDDSVideo = 0;
	}
	if (lpDDSClear) {
		lpDDSClear->Release();
		lpDDSClear = 0;
	}
	if (lpDD) {
		lpDD->Release();
		lpDD = 0;
	}
}

long DirectDrawBlitter::frameTimeEst() const {
	if (swapInterval)
		return ftEst.est();

	return BlitterWidget::frameTimeEst();
}

void DirectDrawBlitter::setSwapInterval(unsigned const newSwapInterval) {
	if (newSwapInterval != swapInterval) {
		swapInterval = newSwapInterval;
		ftEst = FtEst(swapInterval * 10000000 / dhz);
	}
}

HRESULT DirectDrawBlitter::backBlit(
		IDirectDrawSurface7 *const lpDDSSrc,
		RECT *const rcRectDest, DWORD const flags) {
	HRESULT ddrval = DD_OK;
	for (int n = 2; n-- && lpDDSSrc && lpDDSBack;) {
		lpDDSSrc->PageLock(0);
		ddrval = lpDDSBack->Blt(rcRectDest, lpDDSSrc, 0, flags, 0);
		lpDDSSrc->PageUnlock(0);

		if (ddrval == DDERR_SURFACELOST) {
			restoreSurfaces();
		} else
			break;
	}

	if (ddrval != DD_OK && ddrval != DDERR_WASSTILLDRAWING)
		std::cerr << "lpDDSBack->Blt() failed" << std::endl;

	return ddrval;
}

void DirectDrawBlitter::finalBlit(DWORD const waitflag) {
	if (clear && exclusive && flipping_.value() && !blitted) {
		lpClipper->SetHWnd(0, parentWidget()->parentWidget()->winId());

		RECT rcRectDest;
		GetWindowRect(parentWidget()->parentWidget()->winId(), &rcRectDest);

		bool const success = backBlit(lpDDSClear, &rcRectDest, waitflag) == DD_OK;
		lpClipper->SetHWnd(0, winId());
		clear -= success;
		if (!success)
			return;
	}

	RECT rcRectDest;
	GetWindowRect(winId(), &rcRectDest);
	blitted |= backBlit(lpDDSVideo, &rcRectDest, waitflag) == DD_OK;
}

int DirectDrawBlitter::present() {
	if (!lpDDSVideo || !lpDDSSystem || !lpDDSBack)
		return -1;

	HRESULT ddrval = DD_OK;
	if (exclusive && flipping_.value()) {
		if (swapInterval) {
			unsigned long const estft = ftEst.est();
			usec_t const swaplimit = getusecs() - lastblank > estft - estft / 32
			                       ? estft * 2 - 5000000 / dhz
			                       : 0;
			if (!blitted)
				finalBlit(DDBLT_WAIT);

			DWORD const flipflags = DDFLIP_WAIT
			                      | (swapInterval == 2 ? DDFLIP_INTERVAL2 : 0);
			if (lpDDSPrimary)
				ddrval = lpDDSPrimary->Flip(0, flipflags);

			usec_t now = getusecs();
			if (now - lastblank < swaplimit) {
				blitted = false;
				finalBlit(DDBLT_WAIT);
				if (lpDDSPrimary)
					ddrval = lpDDSPrimary->Flip(0, flipflags);

				now = getusecs();
			}

			lastblank = now;
		} else {
			if (!blitted)
				finalBlit(vblankflip_.value() ? DDBLT_DONOTWAIT : DDBLT_WAIT);

			if (lpDDSPrimary && blitted) {
				ddrval = lpDDSPrimary->Flip(
					0,
					  (vblankflip_.value() ? DDFLIP_DONOTWAIT : DDFLIP_WAIT)
					| (vblankflip_.value() ? 0 : DDFLIP_NOVSYNC));
			}
		}
	} else {
		if (unsigned const si = swapInterval
				? swapInterval
				: vblank_.value() && !DwmControl::isCompositingEnabled()) {
			usec_t const refreshPeriod = 10000000 / dhz;
			while (getusecs() - lastblank < (si - 1) * refreshPeriod + refreshPeriod / 2)
				Sleep(1);

			lpDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
			lastblank = getusecs();
		}

		finalBlit(DDBLT_WAIT);
	}

	if (swapInterval)
		ftEst.update(lastblank);

	blitted = false;

	if (ddrval != DD_OK && ddrval != DDERR_WASSTILLDRAWING)
		std::cerr << "lpDDSPrimary->Flip failed" << std::endl;

	return 0;
}

void DirectDrawBlitter::acceptSettings() {
	bool const oldFlipping = flipping_.value();
	bool const oldTriplebuf = triplebuf_.value();
	bool const oldVideoSurface = videoSurface_.value();
	bool needReinit = false;
	if (static_cast<int>(deviceIndex) != deviceSelector->currentIndex()) {
		deviceIndex = deviceSelector->currentIndex();
		needReinit = true;
	}

	vblank_.accept();
	flipping_.accept();
	vblankflip_.accept();
	triplebuf_.accept();
	videoSurface_.accept();

	if (flipping_.value() != oldFlipping)
		needReinit |= exclusive;
	if (triplebuf_.value() != oldTriplebuf)
		needReinit |= exclusive & flipping_.value();

	if (videoSurface_.value() != oldVideoSurface) {
		if (!needReinit && lpDDSVideo) {
			lpDDSVideo->Release();
			initVideoSurface();
		}
	}

	if (needReinit)
		reinit();
}

void DirectDrawBlitter::rejectSettings() const {
	vblank_.reject();
	flipping_.reject();
	vblankflip_.reject();
	triplebuf_.reject();
	videoSurface_.reject();
	deviceSelector->setCurrentIndex(deviceIndex);
}

void DirectDrawBlitter::rateChange(int const dhz) {
	this->dhz = dhz ? dhz : 600;
	ftEst = FtEst(swapInterval * 10000000 / this->dhz);
}

void DirectDrawBlitter::paintEvent(QPaintEvent *) {
	present();
}

void DirectDrawBlitter::setExclusive(bool const exclusive) {
	if (this->exclusive == exclusive)
		return;

	this->exclusive = exclusive;
	if (flipping_.value())
		reinit();
}

void DirectDrawBlitter::reinit() {
	if (isVisible()) {
		BufferLock lock(*this);
		SetBuffer setInputBuffer(lock);
		uninit();
		setInputBuffer(0, PixelBuffer::RGB32, 0);
		init();
		setBufferDimensions(inBuffer().width, inBuffer().height, setInputBuffer);
	}
}
