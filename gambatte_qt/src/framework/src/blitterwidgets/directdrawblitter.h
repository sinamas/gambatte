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
#include "persistcheckbox.h"
#include <QList>
#include <ddraw.h>
#include <memory>

class QCheckBox;
class QComboBox;

class DirectDrawBlitter : public BlitterWidget {
	FtEst ftEst;
	const std::auto_ptr<QWidget> confWidget;
	PersistCheckBox vblank_;
	PersistCheckBox flipping_;
	PersistCheckBox vblankflip_;
	PersistCheckBox triplebuf_;
	PersistCheckBox videoSurface_;
	QComboBox *const deviceSelector;
	LPDIRECTDRAW7 lpDD;
	LPDIRECTDRAWSURFACE7 lpDDSPrimary;
	LPDIRECTDRAWSURFACE7 lpDDSBack;
	LPDIRECTDRAWSURFACE7 lpDDSSystem;
	LPDIRECTDRAWSURFACE7 lpDDSVideo;
	LPDIRECTDRAWSURFACE7 lpDDSClear;
	LPDIRECTDRAWCLIPPER lpClipper;
	QList<GUID> deviceList;
	usec_t lastblank;
	unsigned clear;
	unsigned dhz;
	unsigned swapInterval;
	unsigned deviceIndex;
	bool exclusive;
	bool blitted;

	static BOOL WINAPI enumCallback(GUID FAR *, char*, char*, LPVOID, HMONITOR);
	void initPrimarySurface();
	void initVideoSurface();
	void initClearSurface();
	void restoreSurfaces();
	void systemSurfaceBlit();
	void videoSurfaceBlit();
	HRESULT backBlit(IDirectDrawSurface7 *lpDDSSrc, RECT *rcRectDest, DWORD flags);
	void finalBlit(DWORD waitFlag);
	//void detectExclusive();
	void reinit();

protected:
	void paintEvent(QPaintEvent *event);
	/*void resizeEvent(QResizeEvent *event);
	void moveEvent(QMoveEvent *event);
	void hideEvent(QHideEvent *event);*/
	void setBufferDimensions(unsigned int w, unsigned int h);

public:
	explicit DirectDrawBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	~DirectDrawBlitter();
	void blit();
	void draw();
	void init();
	long frameTimeEst() const;
	long sync();
	void uninit();
	void setExclusive(bool exclusive);

	QWidget* settingsWidget() const { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings() const;

	QPaintEngine* paintEngine () const { return NULL; }

	void rateChange(int dhz);
	void setSwapInterval(unsigned si);
};

#endif /*DIRECTDRAWBLITTER_H_*/
