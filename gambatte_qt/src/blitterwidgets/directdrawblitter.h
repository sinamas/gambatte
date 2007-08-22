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
#ifndef DIRECTDRAWBLITTER_H_
#define DIRECTDRAWBLITTER_H_

#include "../blitterwidget.h"

#include <ddraw.h>

class QCheckBox;
class VideoBufferReseter;

class DirectDrawBlitter : public BlitterWidget {
	VideoBufferReseter &resetVideoBuffer;
	QWidget *const confWidget;
	QCheckBox *const vblankBox;
	QCheckBox *const flippingBox;
	QCheckBox *const videoSurfaceBox;
	LPDIRECTDRAW7 lpDD;
	LPDIRECTDRAWSURFACE7 lpDDSPrimary;
	LPDIRECTDRAWSURFACE7 lpDDSBack;
	LPDIRECTDRAWSURFACE7 lpDDSSystem;
	LPDIRECTDRAWSURFACE7 lpDDSVideo;
	LPDIRECTDRAWCLIPPER lpClipper;
	PixelBuffer pixb;
	bool keepRatio;
	bool integerScaling;
	bool vblank;
	bool hz60;
	bool videoSurface;
	bool exclusive;
	bool flipping;
	
	bool initPrimarySurface();
	bool initVideoSurface();
	bool restoreSurfaces();
	void systemSurfaceBlit();
	void videoSurfaceBlit();
	//void detectExclusive();
	void reinit();
	
protected:
	void paintEvent(QPaintEvent *event);
	/*void resizeEvent(QResizeEvent *event);
	void moveEvent(QMoveEvent *event);
	void hideEvent(QHideEvent *event);*/
	
public:
	DirectDrawBlitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent = 0);
	~DirectDrawBlitter();
	void blit();
	const PixelBuffer inBuffer();
	void init();
	void keepAspectRatio(bool enable);
	bool keepsAspectRatio();
	void scaleByInteger(bool enable);
	bool scalesByInteger();
	void setBufferDimensions(unsigned int w, unsigned int h);
	const Rational frameTime() const;
	int sync(bool turbo);
	void uninit();
	void setExclusive(bool exclusive);
	
	QWidget* settingsWidget() { return confWidget; }
	void acceptSettings();
	void rejectSettings();
	
public slots:
//	void modeChange();
	void rateChange(int hz);
};

#endif /*DIRECTDRAWBLITTER_H_*/
