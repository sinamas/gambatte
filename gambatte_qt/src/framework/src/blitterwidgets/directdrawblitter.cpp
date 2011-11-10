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
#include "../dwmcontrol.h"
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
	vblank_(new QCheckBox(QString("Wait for vertical blank")), "directdrawblitter/vblank", false),
	flipping_(new QCheckBox(QString("Exclusive full screen")), "directdrawblitter/flipping", false),
	vblankflip_(new QCheckBox(QString("Flip during vertical blank")), "directdrawblitter/vblankflip", true),
	triplebuf_(new QCheckBox("Triple buffering"), "directdrawblitter/triplebuf", false),
	videoSurface_(new QCheckBox(QString("Use video memory surface")), "directdrawblitter/videoSurface", true),
	deviceSelector(new QComboBox),
	lpDD(NULL),
	lpDDSPrimary(NULL),
	lpDDSBack(NULL),
	lpDDSSystem(NULL),
	lpDDSVideo(NULL),
	lpDDSClear(NULL),
	lpClipper(NULL),
	lastblank(0),
	clear(0),
	dhz(600),
	swapInterval(0),
	deviceIndex(0),
	exclusive(false),
	blitted(false)
{
	setAttribute(Qt::WA_PaintOnScreen, true);

	DirectDrawEnumerateExA(enumCallback, this, DDENUM_ATTACHEDSECONDARYDEVICES);

	if (deviceSelector->count() < 1)
		deviceSelector->addItem(QString(), QVariant::fromValue<GUID*>(NULL));

	QSettings settings;
	settings.beginGroup("directdrawblitter");

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

	mainLayout->addWidget(vblank_.checkBox());
	vblank_.checkBox()->setToolTip(tr("Prevents tearing. Does not work well on all systems.\n"
	                                  "Ignored when exclusive full screen or DWM composition is active."));
	mainLayout->addWidget(flipping_.checkBox());
	flipping_.checkBox()->setToolTip(tr("Grabs device for better performance when full screen."));

	{
		QHBoxLayout *l = new QHBoxLayout;
		l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
		l->addWidget(vblankflip_.checkBox());
		vblankflip_.checkBox()->setToolTip(tr("Prevents tearing. Recommended."));
		mainLayout->addLayout(l);
		l = new QHBoxLayout;
		l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
		l->addWidget(triplebuf_.checkBox());
		mainLayout->addLayout(l);
	}

	mainLayout->addWidget(videoSurface_.checkBox());
	confWidget->setLayout(mainLayout);

	vblankflip_.checkBox()->setEnabled(flipping_.checkBox()->isChecked());
	triplebuf_.checkBox()->setEnabled(flipping_.checkBox()->isChecked());
	connect(flipping_.checkBox(), SIGNAL(toggled(bool)), vblankflip_.checkBox(), SLOT(setEnabled(bool)));
	connect(flipping_.checkBox(), SIGNAL(toggled(bool)), triplebuf_.checkBox(), SLOT(setEnabled(bool)));

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
		std::cerr << "lpDDSVideo->BltFast(0, 0, lpDDSSystem, NULL, DDBLTFAST_WAIT) failed" << std::endl;
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

	if (videoSurface_.value())
		videoSurfaceBlit();
	else
		systemSurfaceBlit();

	{
		DDSURFACEDESC2 ddsd;
		ddsd.dwSize = sizeof(ddsd);

		if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
			std::cerr << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
			ddsd.lpSurface = NULL;
		}

		setPixelBuffer(ddsd.lpSurface, inBuffer().pixelFormat,
				inBuffer().pixelFormat == PixelBuffer::RGB16 ? ddsd.lPitch >> 1 : (ddsd.lPitch >> 2));
	}
}

void DirectDrawBlitter::draw() {
	if (exclusive & flipping_.value()/* & ~vblankflip*/)
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

	if (exclusive & flipping_.value()) {
		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = triplebuf_.value() ? 2 : 1;
		ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
	}

	if (lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL) != DD_OK) {
		std::cerr << "lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL) failed" << std::endl;
		lpDDSBack = lpDDSPrimary = NULL;
	} else {
		lpDDSBack = lpDDSPrimary;

		if (exclusive & flipping_.value()) {
			ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;

			if (lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack) != DD_OK) {
				std::cerr << "lpDDSPrimary->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSBack) failed" << std::endl;
				lpDDSBack = NULL;
			}
		}

		if (lpDDSPrimary->SetClipper(lpClipper) != DD_OK)
			std::cerr << "SetClipper failed" << std::endl;
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
		std::cerr << "lpDD->CreateSurface(&ddsd, &lpDDSOut, NULL) failed" << std::endl;
	}
}

void DirectDrawBlitter::initVideoSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSVideo, 0, 0, videoSurface_.value() ? 0 : DDSCAPS_SYSTEMMEMORY);
}

void DirectDrawBlitter::initClearSurface() {
	initSubSurface(lpDD, lpDDSSystem, lpDDSClear, 1, 1, 0);

	if (lpDDSClear) {
		DDSURFACEDESC2 ddsd;
		ddsd.dwSize = sizeof(ddsd);

		if (lpDDSClear->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
			std::cerr << "lpDDSClear->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
			lpDDSClear->Release();
			lpDDSClear = NULL;
		} else {
			std::memset(ddsd.lpSurface, 0, ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
			lpDDSClear->Unlock(NULL);
		}
	}

	clear = 2 + triplebuf_.value();
}

void DirectDrawBlitter::init() {
	if (DirectDrawCreateEx(deviceSelector->itemData(deviceIndex).value<GUID*>(), reinterpret_cast<void**>(&lpDD),
			IID_IDirectDraw7, NULL) != DD_OK) {
		std::cerr << "DirectDrawCreateEx failed" << std::endl;
		goto fail;
	}

	if (lpDD->SetCooperativeLevel(parentWidget()->parentWidget()->winId(),
			(exclusive & flipping_.value()) ? DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN : DDSCL_NORMAL) != DD_OK) {
		std::cerr << "SetCooperativeLevel failed" << std::endl;
		goto fail;
	}

	if (lpDD->CreateClipper(0, &lpClipper, NULL) != DD_OK) {
		std::cerr << "CreateClipper failed" << std::endl;
		goto fail;
	}

	if (lpClipper->SetHWnd(0, winId()) != DD_OK) {
		std::cerr << "SetHWnd failed" << std::endl;
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

		if (videoSurface_.value()) {
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
	PixelBuffer::PixelFormat pixelFormat;
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
		std::cerr << "lpDD->CreateSurface(&ddsd, &lpDDSSystem, NULL) failed" << std::endl;
		goto fail;
	}

	initVideoSurface();
	initClearSurface();

	if (!lpDDSVideo || !lpDDSClear)
		goto fail;

	if (lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) != DD_OK) {
		std::cerr << "lpDDSSystem->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL) failed" << std::endl;
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

		vblank_.checkBox()->setText(text + " and " + QString::number(hz1 * 2) + " Hz modes");
	}

	setSwapInterval(hz1, hz2);
}*/

long DirectDrawBlitter::frameTimeEst() const {
	if (swapInterval)
		return ftEst.est();

	return BlitterWidget::frameTimeEst();
}

/*const BlitterWidget::Rational DirectDrawBlitter::frameTime() const {
	if (vblank_.value() && hz == vblankHz) {
		return Rational(1, hz);
	}

	return BlitterWidget::frameTime();
}*/

void DirectDrawBlitter::setSwapInterval(const unsigned newSwapInterval) {
	if (newSwapInterval != swapInterval) {
		swapInterval = newSwapInterval;
		ftEst.init(swapInterval * 10000000 / dhz);
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
		std::cerr << "lpDDSBack->Blt(rcRectDest, lpDDSSrc, NULL, flags, NULL) failed" << std::endl;

	return ddrval;
}

void DirectDrawBlitter::finalBlit(const DWORD waitflag) {
	if (clear && (exclusive & flipping_.value()) && !blitted) {
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

	if (exclusive & flipping_.value()) {
		if (swapInterval) {
			const unsigned long estft = ftEst.est();
			const usec_t swaplimit = getusecs() - lastblank > estft - estft / 32 ? estft * 2 - 5000000 / dhz : 0;

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
				finalBlit(vblankflip_.value() ? DDBLT_DONOTWAIT : DDBLT_WAIT);

			if (lpDDSPrimary && blitted)
				ddrval = lpDDSPrimary->Flip(NULL,
						(vblankflip_.value() ? DDFLIP_DONOTWAIT : DDFLIP_WAIT) |
						(vblankflip_.value() ? 0 : DDFLIP_NOVSYNC));
		}
	} else {
		if (const unsigned si = swapInterval ? swapInterval : vblank_.value() && !DwmControl::isCompositingEnabled()) {
			const usec_t refreshPeriod = 10000000 / dhz;

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
	const bool oldFlipping = flipping_.value();
	const bool oldTriplebuf = triplebuf_.value();
	const bool oldVideoSurface = videoSurface_.value();
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

void DirectDrawBlitter::rateChange(const int dhz) {
	this->dhz = dhz ? dhz : 600;
	ftEst.init(swapInterval * 10000000 / this->dhz);
}

void DirectDrawBlitter::paintEvent(QPaintEvent */*event*/) {
	sync();
}

void DirectDrawBlitter::setExclusive(const bool exclusive) {
	if (this->exclusive == exclusive)
		return;

	this->exclusive = exclusive;

	// std::cout << "exclusive: " << exclusive << std::endl;

	if (flipping_.value())
		reinit();
}

void DirectDrawBlitter::reinit() {
	if (isVisible()) {
		lockPixelBuffer();
		uninit();
		setPixelBuffer(NULL, PixelBuffer::RGB32, 0);
		init();
		setBufferDimensions(inBuffer().width, inBuffer().height);
		unlockPixelBuffer();
	}
}
