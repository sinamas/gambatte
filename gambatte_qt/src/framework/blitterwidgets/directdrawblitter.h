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
#ifndef DIRECTDRAWBLITTER_H_
#define DIRECTDRAWBLITTER_H_

#include "../blitterwidget.h"
#include <QList>
#include <ddraw.h>
#include <memory>

class QCheckBox;
class QComboBox;

class DirectDrawBlitter : public BlitterWidget {
	FtEst ftEst;
	const std::auto_ptr<QWidget> confWidget;
	QCheckBox *const vblankBox;
	QCheckBox *const flippingBox;
	QCheckBox *const vblankflipBox;
	QCheckBox *const triplebufBox;
	QCheckBox *const videoSurfaceBox;
	QComboBox *const deviceSelector;
	LPDIRECTDRAW7 lpDD;
	LPDIRECTDRAWSURFACE7 lpDDSPrimary;
	LPDIRECTDRAWSURFACE7 lpDDSBack;
	LPDIRECTDRAWSURFACE7 lpDDSSystem;
	LPDIRECTDRAWSURFACE7 lpDDSVideo;
	LPDIRECTDRAWSURFACE7 lpDDSClear;
	LPDIRECTDRAWCLIPPER lpClipper;
	QList<GUID> deviceList;
	MediaSource::PixelFormat pixelFormat;
	usec_t lastblank;
	unsigned clear;
	unsigned hz;
	unsigned swapInterval;
	unsigned deviceIndex;
	unsigned inWidth;
	unsigned inHeight;
	bool vblank;
	bool videoSurface;
	bool exclusive;
	bool flipping;
	bool vblankflip;
	bool triplebuf;
	bool blitted;

	static BOOL WINAPI enumCallback(GUID FAR *, char*, char*, LPVOID, HMONITOR);
	void initPrimarySurface();
	void initVideoSurface();
	void initClearSurface();
	void restoreSurfaces();
	void systemSurfaceBlit();
	void videoSurfaceBlit();
	void setSwapInterval(unsigned hz1, unsigned hz2);
	void setSwapInterval();
	HRESULT backBlit(IDirectDrawSurface7 *lpDDSSrc, RECT *rcRectDest, DWORD flags);
	void finalBlit(DWORD waitFlag);
	//void detectExclusive();
	void reinit();

protected:
	void paintEvent(QPaintEvent *event);
	/*void resizeEvent(QResizeEvent *event);
	void moveEvent(QMoveEvent *event);
	void hideEvent(QHideEvent *event);*/

public:
	DirectDrawBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent = 0);
	~DirectDrawBlitter();
	void blit();
	void init();
	void setBufferDimensions(unsigned int w, unsigned int h);
	void setFrameTime(long ft);
	const Estimate frameTimeEst() const;
	long sync(long turbo);
	void uninit();
	void setExclusive(bool exclusive);

	QWidget* settingsWidget() { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings();

	QPaintEngine* paintEngine () const { return NULL; }

public slots:
//	void modeChange();
	void rateChange(int hz);
};

#endif /*DIRECTDRAWBLITTER_H_*/
