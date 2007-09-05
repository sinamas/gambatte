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
#include "directdrawblitter.h"

#include "../videobufferreseter.h"

#include <QCheckBox>
#include <QSettings>
#include <QVBoxLayout>
#include <iostream>
#include <cstring>

DirectDrawBlitter::DirectDrawBlitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent) :
	BlitterWidget("DirectDraw", parent),
	resetVideoBuffer(resetVideoBuffer_in),
	confWidget(new QWidget),
	vblankBox(new QCheckBox(QString("Sync to vertical blank in 60 Hz modes"))),
	flippingBox(new QCheckBox(QString("Page flipping"))),
	videoSurfaceBox(new QCheckBox(QString("Use video memory surface"))),
	lpDD(NULL),
	lpDDSPrimary(NULL),
	lpDDSSystem(NULL),
	lpDDSVideo(NULL),
	lpClipper(NULL),
	keepRatio(true),
	integerScaling(false),
	exclusive(false)
{
	pixb.pixels = NULL;
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	
	QSettings settings;
	settings.beginGroup("directdrawblitter");
	vblank = settings.value("vblank", false).toBool();
	flipping = settings.value("flipping", false).toBool();
	videoSurface = settings.value("videoSurface", true).toBool();
	settings.endGroup();
	
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(vblankBox);
	confWidget->layout()->addWidget(flippingBox);
	confWidget->layout()->addWidget(videoSurfaceBox);
	vblankBox->setChecked(vblank);
	flippingBox->setChecked(flipping);
	videoSurfaceBox->setChecked(videoSurface);
}

DirectDrawBlitter::~DirectDrawBlitter() {
	uninit();
	
	QSettings settings;
	settings.beginGroup("directdrawblitter");
	settings.setValue("vblank", vblank);
	settings.setValue("flipping", flipping);
	settings.setValue("videoSurface", videoSurface);
	settings.endGroup();
	
	delete confWidget;
}

void DirectDrawBlitter::videoSurfaceBlit() {
	lpDDSSystem->PageLock(0);
	lpDDSVideo->PageLock(0);
	
	HRESULT ddrval;
	ddrval = lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT);
	
	if (ddrval == DDERR_SURFACELOST) {
		if (!restoreSurfaces())
			ddrval = lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT);
	}
	
	if (ddrval != DD_OK) {
		std::cout << "lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT) failed" << std::endl;
		
		if (lpDDSPrimary) {
			lpDDSPrimary->Release();
			lpDDSPrimary = NULL;
		}
	}
	
	lpDDSVideo->PageUnlock(0);
	lpDDSSystem->PageUnlock(0);
}

void DirectDrawBlitter::systemSurfaceBlit() {
	LPDIRECTDRAWSURFACE7 lpDDSTmp = lpDDSSystem;
	lpDDSSystem = lpDDSVideo;
	lpDDSVideo = lpDDSTmp;
}

void DirectDrawBlitter::blit() {
	if (!lpDDSPrimary)
		return;
	
	lpDDSSystem->Unlock(NULL);
	
	if (videoSurface)
		videoSurfaceBlit();
	else
		systemSurfaceBlit();
	
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(ddsd); 
	
	if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL) != DD_OK)
		std::cout << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL) failed" << std::endl;
	
	pixb.pixels = ddsd.lpSurface;
	pixb.pitch = pixb.format == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2);
	resetVideoBuffer();
}

const PixelBuffer DirectDrawBlitter::inBuffer() {
	return pixb;
}

bool DirectDrawBlitter::initPrimarySurface() {
	{
		DDSURFACEDESC2 ddsd;
		std::memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd); 
		ddsd.dwFlags = DDSD_CAPS; 
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		
		if (exclusive && flipping) {
			ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
			ddsd.dwBackBufferCount = 1;
			ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		}
		
		HRESULT ddrval = lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL);
		lpDDSBack = lpDDSPrimary;
		
		if (exclusive && flipping && ddrval == DD_OK) {
			ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
			ddrval = lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack); 
		}
		
		if (ddrval != DD_OK) {
			std::cout << "CreateSurface failed" << std::endl;
			lpDDSPrimary = NULL;
			return true;
		}
	}
	
	if (lpDDSPrimary->SetClipper(lpClipper) != DD_OK) {
		std::cout << "SetClipper failed" << std::endl;
		return true;
	}
	
	return false;
}

bool DirectDrawBlitter::initVideoSurface() {
	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd); 
	lpDDSSystem->GetSurfaceDesc(&ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT; 
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	
	if (!videoSurface)
		ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	
	if (lpDD->CreateSurface(&ddsd, &lpDDSVideo, NULL) != DD_OK) {
		lpDDSVideo = NULL;
		return true;
	}
	
	return false;
}

void DirectDrawBlitter::init() {
	if (DirectDrawCreateEx(NULL, reinterpret_cast<void**>(&lpDD), IID_IDirectDraw7, NULL) != DD_OK) {
		std::cout << "DirectDrawCreateEx failed" << std::endl;
		goto fail;
	}
	
	if (lpDD->SetCooperativeLevel(GetParent(GetParent(winId())), (exclusive && flipping) ? DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN : DDSCL_NORMAL) != DD_OK) {
		std::cout << "SetCooperativeLevel failed" << std::endl;
		goto fail;
	}
	
	if (lpDD->CreateClipper(0, &lpClipper, NULL) != DD_OK) {
		std::cout << "CreateClipper failed" << std::endl;
		goto fail;
	}
	
	if (lpClipper->SetHWnd(0, winId()) != DD_OK) {
		std::cout << "SetHWnd failed" << std::endl;
		goto fail;
	}
	
	if (initPrimarySurface())
		goto fail;
	
	return;
	
fail:
	uninit();
}

bool DirectDrawBlitter::restoreSurfaces() {
	lpDDSPrimary->Restore();
	lpDDSVideo->Restore();
	
	if (lpDDSPrimary->IsLost() != DDERR_SURFACELOST && lpDDSVideo->IsLost() != DDERR_SURFACELOST)
		return false;
	
	lpDDSPrimary->Release();
	
	if (initPrimarySurface()) {
		return true;
	}
	
	if (videoSurface) {
		lpDDSVideo->Release();
		
		if (initVideoSurface())
			return true;
	}
	
	return false;
}

static void setDdPf(DDPIXELFORMAT *const ddpf, PixelBuffer *const pixb, LPDIRECTDRAWSURFACE7 lpDDSPrimary) {
	bool alpha = false;

	ddpf->dwSize = sizeof(DDPIXELFORMAT);
	
	if (lpDDSPrimary && lpDDSPrimary->GetPixelFormat(ddpf) == DD_OK && (ddpf->dwFlags & DDPF_RGB) && ddpf->dwRGBBitCount == 16) {
		pixb->format = PixelBuffer::RGB16;
	} else {
		pixb->format = PixelBuffer::RGB32;
		alpha = ddpf->dwFlags & DDPF_ALPHAPIXELS;
	}
	
	std::memset(ddpf, 0, sizeof(DDPIXELFORMAT));
	ddpf->dwFlags = DDPF_RGB;
	
	if (pixb->format == PixelBuffer::RGB16) {
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

void DirectDrawBlitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	if (lpDDSPrimary == NULL)
		return;
	
	if (lpDDSSystem) {
		lpDDSSystem->Release();
		lpDDSSystem = NULL;
	}
	
	if (lpDDSVideo) {
		lpDDSVideo->Release();
		lpDDSVideo = NULL;
	}
	
	DDPIXELFORMAT ddpf;
	setDdPf(&ddpf, &pixb, lpDDSPrimary);
	
	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd); 
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT; 
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = w;
	ddsd.dwHeight = h;
	ddsd.ddpfPixelFormat = ddpf;
	
	if (lpDD->CreateSurface(&ddsd, &lpDDSSystem, NULL) != DD_OK) {
		std::cout << "lpDD->CreateSurface(&ddsd, &lpDDSSystem, NULL) failed" << std::endl;
		goto fail;
	}
	
	if (videoSurface)
		ddsd.ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
	
	if (lpDD->CreateSurface(&ddsd, &lpDDSVideo, NULL) != DD_OK) {
		std::cout << "lpDD->CreateSurface(&ddsd, &lpDDSVideo, NULL) failed" << std::endl;
		goto fail;
	}
	
	if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL) != DD_OK) {
		std::cout << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK, NULL) failed" << std::endl;
		goto fail;
	}
	
	pixb.pixels = ddsd.lpSurface;
	pixb.pitch = pixb.format == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2);
	
	return;
	
fail:
	uninit();
}

void DirectDrawBlitter::uninit() {
	if (lpClipper) {
		lpClipper->Release();
		lpClipper = NULL;
	}

	if (lpDDSPrimary) {
		lpDDSPrimary->Release();
		lpDDSPrimary = NULL;
	}

	if (lpDDSSystem) {
		lpDDSSystem->Release();
		lpDDSSystem = NULL;
	}
	
	if (lpDDSVideo) {
		lpDDSVideo->Release();
		lpDDSVideo = NULL;
	}

	if (lpDD) {
		lpDD->Release();
		lpDD = NULL;
	}

	pixb.pixels = NULL;
}

const BlitterWidget::Rational DirectDrawBlitter::frameTime() const {
	if (vblank && hz60) {
		Rational r = { 1, 60 };
		return r;
	}
	
	return BlitterWidget::frameTime();
}

int DirectDrawBlitter::sync(const bool turbo) {
	if (lpDDSPrimary == NULL)
		return -1;
	
	lpDDSVideo->PageLock(0);
	
	RECT rcRectDest;
	GetWindowRect(winId(), &rcRectDest);
	
	if (!turbo) {
		if (vblank && hz60) {
			if (!(exclusive && flipping))
				lpDD->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);
		} else
			BlitterWidget::sync(turbo);
	}
	
	const bool dontwait = exclusive && flipping && !(vblank && hz60 && !turbo);
	
	HRESULT ddrval = lpDDSBack->Blt(&rcRectDest, lpDDSVideo, NULL, dontwait ? DDBLT_DONOTWAIT : DDBLT_WAIT, NULL);
	
	if (ddrval == DDERR_SURFACELOST) {
		if (!restoreSurfaces())
			ddrval = lpDDSBack->Blt(&rcRectDest, lpDDSVideo, NULL, dontwait ? DDBLT_DONOTWAIT : DDBLT_WAIT, NULL);
	}
	
	lpDDSVideo->PageUnlock(0);
	
	if (ddrval == DD_OK && exclusive && flipping) {
		ddrval = lpDDSPrimary->Flip(NULL, dontwait ? DDFLIP_DONOTWAIT : DDFLIP_WAIT);
	}
	
	if (ddrval != DD_OK && ddrval != DDERR_WASSTILLDRAWING) {
		std::cout << "final blit failed" << std::endl;
		return -1;
	}
	
	return 0;
}

void DirectDrawBlitter::keepAspectRatio(const bool enable) {
	keepRatio = enable;
}

bool DirectDrawBlitter::keepsAspectRatio() {
	return keepRatio;
}

void DirectDrawBlitter::scaleByInteger(const bool enable) {
	integerScaling = enable;
}

bool DirectDrawBlitter::scalesByInteger() {
	return integerScaling;
}

void DirectDrawBlitter::acceptSettings() {
	vblank = vblankBox->isChecked();
	
	if (flipping != flippingBox->isChecked()) {
		flipping = flippingBox->isChecked();
		
		if (exclusive)
			reinit();
	}
	
	if (videoSurface != videoSurfaceBox->isChecked()) {
		videoSurface = videoSurfaceBox->isChecked();
		
		if (lpDDSVideo) {
			lpDDSVideo->Release();
			
			if (initVideoSurface()) {
				lpDDSPrimary->Release();
				lpDDSPrimary = NULL;
			}
		}
	}
}

void DirectDrawBlitter::rejectSettings() {
	vblankBox->setChecked(vblank);
	flippingBox->setChecked(flipping);
	videoSurfaceBox->setChecked(videoSurface);
}

void DirectDrawBlitter::rateChange(int hz) {
	hz60 = hz == 60;
}

void DirectDrawBlitter::paintEvent(QPaintEvent */*event*/) {
	sync(true);
}

/*void DirectDrawBlitter::detectExclusive() {
	if (!lpDD || !parentWidget())
		return;
	
	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd); 
	lpDD->GetDisplayMode(&ddsd);
	
	setExclusive(parentWidget()->width() == ddsd.dwWidth && parentWidget()->height() == ddsd.dwHeight);
}

void DirectDrawBlitter::resizeEvent(QResizeEvent *event) {
	BlitterWidget::resizeEvent(event);
	detectExclusive();
}

void DirectDrawBlitter::moveEvent(QMoveEvent *event) {
	BlitterWidget::moveEvent(event);
	detectExclusive();
}

void DirectDrawBlitter::hideEvent(QHideEvent *event) {
	BlitterWidget::hideEvent(event);
	setExclusive(false);
}*/

void DirectDrawBlitter::setExclusive(const bool exclusive) {
	if (this->exclusive == exclusive)
		return;
	
	this->exclusive = exclusive;
	
	std::cout << "exclusive: " << exclusive << std::endl;
	
	if (flipping)
		reinit();
}

void DirectDrawBlitter::reinit() {
	if (lpDDSSystem) {
		DDSURFACEDESC2 ddsd;
		std::memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd); 
		lpDDSSystem->GetSurfaceDesc(&ddsd);
		
		uninit();
		init();
		setBufferDimensions(ddsd.dwWidth, ddsd.dwHeight);
		resetVideoBuffer();
	}
}
