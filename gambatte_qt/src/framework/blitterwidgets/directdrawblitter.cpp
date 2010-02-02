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
#include "directdrawblitter.h"
#include <QCheckBox>
#include <QComboBox>
#include <QSettings>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <iostream>
#include <cstring>

Q_DECLARE_METATYPE(GUID*)

BOOL WINAPI DirectDrawBlitter::enumCallback(GUID FAR *lpGUID, char *lpDriverDescription, char */*lpDriverName*/, LPVOID lpContext, HMONITOR) {
	DirectDrawBlitter *const thisptr = static_cast<DirectDrawBlitter*>(lpContext);
	GUID *guidptr = NULL;

	if (lpGUID) {
		thisptr->deviceList.append(*lpGUID);
		guidptr = &thisptr->deviceList.last();
	}

	thisptr->deviceSelector->addItem(lpDriverDescription, QVariant::fromValue(guidptr));

	return true;
}

DirectDrawBlitter::DirectDrawBlitter(VideoBufferLocker vbl, QWidget *parent) :
	BlitterWidget(vbl, "DirectDraw", 2, parent),
	confWidget(new QWidget),
	vblankBox(new QCheckBox(QString("Wait for vertical blank"))),
	flippingBox(new QCheckBox(QString("Exclusive full screen"))),
	triplebufBox(new QCheckBox("Triple buffering")),
	videoSurfaceBox(new QCheckBox(QString("Use video memory surface"))),
	deviceSelector(new QComboBox),
	lpDD(NULL),
	lpDDSPrimary(NULL),
	lpDDSBack(NULL),
	lpDDSSystem(NULL),
	lpDDSVideo(NULL),
	lpDDSClear(NULL),
	lpClipper(NULL),
	pixelFormat(PixelBuffer::RGB32),
	lastblank(0),
	clear(0),
	hz(0),
	swapInterval(0),
	deviceIndex(0),
	inWidth(1),
	inHeight(1),
	vblank(false),
	videoSurface(true),
	exclusive(false),
	flipping(false),
	triplebuf(false),
	blitted(false)
{
	setAttribute(Qt::WA_PaintOnScreen, true);

	DirectDrawEnumerateExA(enumCallback, this, DDENUM_ATTACHEDSECONDARYDEVICES);

	if (deviceSelector->count() < 1)
		deviceSelector->addItem(QString(), QVariant::fromValue<GUID*>(NULL));

	QSettings settings;
	settings.beginGroup("directdrawblitter");
	vblank = settings.value("vblank", vblank).toBool();
	flipping = settings.value("flipping", flipping).toBool();
	triplebuf = settings.value("triplebuf", triplebuf).toBool();
	videoSurface = settings.value("videoSurface", videoSurface).toBool();

	if ((deviceIndex = settings.value("deviceIndex", deviceIndex).toUInt()) >= static_cast<unsigned>(deviceSelector->count()))
		deviceIndex = 0;

	settings.endGroup();

	QVBoxLayout *const mainLayout = new QVBoxLayout;
	mainLayout->setMargin(0);

	if (deviceSelector->count() > 2) {
		QHBoxLayout *const hlayout = new QHBoxLayout;

		hlayout->addWidget(new QLabel(QString(tr("DirectDraw device:"))));
		hlayout->addWidget(deviceSelector);

		mainLayout->addLayout(hlayout);
	}

	mainLayout->addWidget(vblankBox);
	mainLayout->addWidget(flippingBox);

	{
		QHBoxLayout *l = new QHBoxLayout;
		l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
		l->addWidget(triplebufBox);
		mainLayout->addLayout(l);
	}

	mainLayout->addWidget(videoSurfaceBox);
	confWidget->setLayout(mainLayout);

	triplebufBox->setEnabled(false);
	connect(flippingBox, SIGNAL(toggled(bool)), triplebufBox, SLOT(setEnabled(bool)));

	rejectSettings();
}

DirectDrawBlitter::~DirectDrawBlitter() {
	uninit();

	QSettings settings;
	settings.beginGroup("directdrawblitter");
	settings.setValue("vblank", vblank);
	settings.setValue("flipping", flipping);
	settings.setValue("triplebuf", triplebuf);
	settings.setValue("videoSurface", videoSurface);
	settings.setValue("deviceIndex", deviceIndex);
	settings.endGroup();
}

void DirectDrawBlitter::videoSurfaceBlit() {
	HRESULT ddrval = DD_OK;

	for (unsigned n = 2; n-- && lpDDSSystem && lpDDSVideo;) {
		lpDDSSystem->PageLock(0);
		lpDDSVideo->PageLock(0);

		ddrval = lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT);

		lpDDSVideo->PageUnlock(0);
		lpDDSSystem->PageUnlock(0);

		if (ddrval == DDERR_SURFACELOST) {
			restoreSurfaces();
		} else
			break;
	}

	if (ddrval != DD_OK)
		std::cout << "lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT) failed" << std::endl;
}

void DirectDrawBlitter::systemSurfaceBlit() {
	LPDIRECTDRAWSURFACE7 lpDDSTmp = lpDDSSystem;
	lpDDSSystem = lpDDSVideo;
	lpDDSVideo = lpDDSTmp;
}

void DirectDrawBlitter::blit() {
	if (!lpDDSSystem || !lpDDSVideo)
		return;

	lpDDSSystem->Unlock(NULL);

	if (videoSurface)
		videoSurfaceBlit();
	else
		systemSurfaceBlit();

	{
		DDSURFACEDESC2 ddsd;
		ddsd.dwSize = sizeof(ddsd);

		if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
			std::cout << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
			ddsd.lpSurface = NULL;
		}

		setPixelBuffer(ddsd.lpSurface, pixelFormat, pixelFormat == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2));
	}
}

void DirectDrawBlitter::draw() {
	if (exclusive & flipping/* & ~vblankflip*/)
		finalBlit(DDBLT_WAIT);
}

void DirectDrawBlitter::initPrimarySurface() {
	if (!lpClipper)
		return;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (exclusive & flipping) {
		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = triplebuf ? 2 : 1;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
	}

	if (lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL) != DD_OK) {
		std::cout << "lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL) failed" << std::endl;
		lpDDSBack = lpDDSPrimary = NULL;
	} else {
		lpDDSBack = lpDDSPrimary;

		if (exclusive & flipping) {
			ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

			if (lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack) != DD_OK) {
				std::cout << "lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack) failed" << std::endl;
				lpDDSBack = NULL;
			}
		}

		if (lpDDSPrimary->SetClipper(lpClipper) != DD_OK)
			std::cout << "SetClipper failed" << std::endl;
	}
}

static void initSubSurface(IDirectDraw7 *const lpDD, IDirectDrawSurface7 *const lpDDSSystem,
		IDirectDrawSurface7 *&lpDDSOut, const DWORD w, const DWORD h, const DWORD dwCaps) {
	if (!lpDDSSystem)
		return;

	DDSURFACEDESC2 ddsd;
	std::memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	lpDDSSystem->GetSurfaceDesc(&ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | dwCaps;

	if (w)
		ddsd.dwWidth = w;

	if (h)
		ddsd.dwHeight = h;

	if (lpDD->CreateSurface(&ddsd, &lpDDSOut, NULL) != DD_OK) {
		lpDDSOut = NULL;
		std::cout << "lpDD->CreateSurface(&ddsd, &lpDDSOut, NULL) failed" << std::endl;
	}
}

void DirectDrawBlitter::initVideoSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSVideo, 0, 0, videoSurface ? 0 : DDSCAPS_SYSTEMMEMORY);
}

void DirectDrawBlitter::initClearSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSClear, 1, 1, 0);

	if (lpDDSClear) {
		DDSURFACEDESC2 ddsd;
		ddsd.dwSize = sizeof(ddsd);

		if (lpDDSClear->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
			std::cout << "lpDDSClear->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
			lpDDSClear->Release();
			lpDDSClear = NULL;
		} else {
			std::memset(ddsd.lpSurface, 0, ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
			lpDDSClear->Unlock(NULL);
		}
	}

	clear = 2 + triplebuf;
}

void DirectDrawBlitter::init() {
	if (DirectDrawCreateEx(deviceSelector->itemData(deviceIndex).value<GUID*>(), reinterpret_cast<void**>(&lpDD),
			IID_IDirectDraw7, NULL) != DD_OK) {
		std::cout << "DirectDrawCreateEx failed" << std::endl;
		goto fail;
	}

	if (lpDD->SetCooperativeLevel(parentWidget()->parentWidget()->winId(),
			(exclusive & flipping) ? DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN : DDSCL_NORMAL) != DD_OK) {
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

	initPrimarySurface();
	return;

fail:
	uninit();
}

void DirectDrawBlitter::restoreSurfaces() {
	if (!lpDDSPrimary || !lpDDSVideo || !lpDDSClear)
		return;

	lpDDSPrimary->Restore();
	lpDDSVideo->Restore();
	lpDDSClear->Restore();

	if (lpDDSPrimary->IsLost() == DDERR_SURFACELOST || lpDDSVideo->IsLost() == DDERR_SURFACELOST ||
			lpDDSClear->IsLost() == DDERR_SURFACELOST) {
		lpDDSPrimary->Release();
		initPrimarySurface();

		if (videoSurface) {
			lpDDSVideo->Release();
			initVideoSurface();
		}

		lpDDSClear->Release();
		initClearSurface();
	}
}

static void setDdPf(DDPIXELFORMAT *const ddpf, PixelBuffer::PixelFormat *const pixelFormat,
		LPDIRECTDRAWSURFACE7 lpDDSPrimary) {
	bool alpha = false;

	ddpf->dwSize = sizeof(DDPIXELFORMAT);

	if (lpDDSPrimary && lpDDSPrimary->GetPixelFormat(ddpf) == DD_OK && (ddpf->dwFlags & DDPF_RGB) &&
			ddpf->dwRGBBitCount == 16) {
		*pixelFormat = PixelBuffer::RGB16;
	} else {
		*pixelFormat = PixelBuffer::RGB32;
		alpha = ddpf->dwFlags & DDPF_ALPHAPIXELS;
	}

	std::memset(ddpf, 0, sizeof(DDPIXELFORMAT));
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

void DirectDrawBlitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	inWidth = w;
	inHeight = h;
	blitted = false;

	if (!lpDDSPrimary)
		return;

	if (lpDDSSystem) {
		lpDDSSystem->Release();
		lpDDSSystem = NULL;
	}

	if (lpDDSVideo) {
		lpDDSVideo->Release();
		lpDDSVideo = NULL;
	}

	if (lpDDSClear) {
		lpDDSClear->Release();
		lpDDSClear = NULL;
	}

	DDPIXELFORMAT ddpf;
	setDdPf(&ddpf, &pixelFormat, lpDDSPrimary);

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

	initVideoSurface();
	initClearSurface();

	if (!lpDDSVideo || !lpDDSClear)
		goto fail;

	if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
		std::cout << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
		goto fail;
	}

	setPixelBuffer(ddsd.lpSurface, pixelFormat, pixelFormat == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2));

	return;

fail:
	setPixelBuffer(NULL, pixelFormat, 0);
	uninit();
}

void DirectDrawBlitter::uninit() {
	lpDDSBack = NULL;

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

	if (lpDDSClear) {
		lpDDSClear->Release();
		lpDDSClear = NULL;
	}

	if (lpDD) {
		lpDD->Release();
		lpDD = NULL;
	}
}

/*void DirectDrawBlitter::setFrameTime(const long ft) {
	BlitterWidget::setFrameTime(ft);

	const unsigned hz1 = (1000000 + (ft >> 1)) / ft;
	const unsigned hz2 = (1000000 * 2 + (ft >> 1)) / ft;

	{
		QString text("Sync to vertical blank in " + QString::number(hz1));

		if (hz2 != hz1 * 2)
			text += ", " + QString::number(hz2);

		vblankBox->setText(text + " and " + QString::number(hz1 * 2) + " Hz modes");
	}

	setSwapInterval(hz1, hz2);
}*/

long DirectDrawBlitter::frameTimeEst() const {
	if (swapInterval)
		return ftEst.est();

	return BlitterWidget::frameTimeEst();
}

/*const BlitterWidget::Rational DirectDrawBlitter::frameTime() const {
	if (vblank && hz == vblankHz) {
		return Rational(1, hz);
	}

	return BlitterWidget::frameTime();
}*/

void DirectDrawBlitter::setSwapInterval(const unsigned newSwapInterval) {
	if (newSwapInterval != swapInterval) {
		swapInterval = newSwapInterval;
		ftEst.init(hz ? swapInterval * 1000000 / hz : 0);
	}
}

HRESULT DirectDrawBlitter::backBlit(IDirectDrawSurface7 *const lpDDSSrc, RECT *const rcRectDest, const DWORD flags) {
	HRESULT ddrval = DD_OK;

	for (unsigned n = 2; n-- && lpDDSSrc && lpDDSBack;) {
		lpDDSSrc->PageLock(0);
		ddrval = lpDDSBack->Blt(rcRectDest, lpDDSSrc, NULL, flags, NULL);
		lpDDSSrc->PageUnlock(0);

		if (ddrval == DDERR_SURFACELOST) {
			restoreSurfaces();
		} else
			break;
	}

	if (ddrval != DD_OK && ddrval != DDERR_WASSTILLDRAWING)
		std::cout << "lpDDSBack->Blt(rcRectDest, lpDDSSrc, NULL, flags, NULL) failed" << std::endl;

	return ddrval;
}

void DirectDrawBlitter::finalBlit(const DWORD waitflag) {
	if (clear && (exclusive & flipping) && !blitted) {
		lpClipper->SetHWnd(0, parentWidget()->parentWidget()->winId());

		RECT rcRectDest;
		GetWindowRect(parentWidget()->parentWidget()->winId(), &rcRectDest);

		const bool success = backBlit(lpDDSClear, &rcRectDest, waitflag) == DD_OK;

		lpClipper->SetHWnd(0, winId());

		clear -= success;

		if (!success)
			return;
	}

	RECT rcRectDest;
	GetWindowRect(winId(), &rcRectDest);
	blitted |= backBlit(lpDDSVideo, &rcRectDest, waitflag) == DD_OK;
}

long DirectDrawBlitter::sync() {
	if (!lpDDSVideo || !lpDDSSystem || !lpDDSBack)
		return -1;

	HRESULT ddrval = DD_OK;

	if (exclusive & flipping) {
		if (swapInterval) {
			const unsigned long estft = ftEst.est();
			const usec_t swaplimit = getusecs() - lastblank > estft - estft / 32 ? estft * 2 - 500000 / hz : 0;

			if (!blitted)
				finalBlit(DDBLT_WAIT);

			const DWORD flipflags = DDFLIP_WAIT | (swapInterval == 2 ? DDFLIP_INTERVAL2 : 0);

			if (lpDDSPrimary)
				ddrval = lpDDSPrimary->Flip(NULL, flipflags);

			usec_t now = getusecs();

			if (now - lastblank < swaplimit) {
				blitted = false;
				finalBlit(DDBLT_WAIT);

				if (lpDDSPrimary)
					ddrval = lpDDSPrimary->Flip(NULL, flipflags);

				now = getusecs();
			}

			lastblank = now;
		} else {
			if (!blitted)
				finalBlit(vblank ? DDBLT_DONOTWAIT : DDBLT_WAIT);

			if (lpDDSPrimary && blitted)
				ddrval = lpDDSPrimary->Flip(NULL,
						(vblank ? DDFLIP_DONOTWAIT : DDFLIP_WAIT) |
						(vblank ? 0 : DDFLIP_NOVSYNC));
		}
	} else {
		if (const unsigned si = swapInterval ? swapInterval : vblank) {
			const usec_t refreshPeriod = 1000000 / hz;

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
		std::cout << "lpDDSPrimary->Flip failed" << std::endl;

	return 0;
}

void DirectDrawBlitter::acceptSettings() {
	bool needReinit = false;

	if (static_cast<int>(deviceIndex) != deviceSelector->currentIndex()) {
		deviceIndex = deviceSelector->currentIndex();
		needReinit = true;
	}

	vblank = vblankBox->isChecked();

	if (flipping != flippingBox->isChecked()) {
		flipping = flippingBox->isChecked();
		needReinit |= exclusive;
	}

	if (triplebuf != triplebufBox->isChecked()) {
		triplebuf = triplebufBox->isChecked();
		needReinit |= exclusive & flipping;
	}

	if (videoSurface != videoSurfaceBox->isChecked()) {
		videoSurface = videoSurfaceBox->isChecked();

		if (!needReinit && lpDDSVideo) {
			lpDDSVideo->Release();
			initVideoSurface();
		}
	}

	if (needReinit)
		reinit();
}

void DirectDrawBlitter::rejectSettings() const {
	vblankBox->setChecked(vblank);
	flippingBox->setChecked(flipping);
	triplebufBox->setChecked(triplebuf);
	videoSurfaceBox->setChecked(videoSurface);
	deviceSelector->setCurrentIndex(deviceIndex);
}

void DirectDrawBlitter::rateChange(int hz) {
	this->hz = hz;
	ftEst.init(hz ? swapInterval * 1000000 / hz : 0);
}

void DirectDrawBlitter::paintEvent(QPaintEvent */*event*/) {
	sync();
}

void DirectDrawBlitter::setExclusive(const bool exclusive) {
	if (this->exclusive == exclusive)
		return;

	this->exclusive = exclusive;

	// std::cout << "exclusive: " << exclusive << std::endl;

	if (flipping)
		reinit();
}

void DirectDrawBlitter::reinit() {
	if (isVisible()) {
		lockPixelBuffer();
		uninit();
		setPixelBuffer(NULL, PixelBuffer::RGB32, 0);
		init();
		setBufferDimensions(inWidth, inHeight);
		unlockPixelBuffer();
	}
}
