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

#ifndef DIRECTDRAWBLITTER_H_
#define DIRECTDRAWBLITTER_H_

#include "../blitterwidget.h"
#include "dialoghelpers.h"
#include "scoped_ptr.h"
#include <QList>
#include <ddraw.h>

class QComboBox;

class DirectDrawBlitter : public BlitterWidget {
public:
	explicit DirectDrawBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	virtual ~DirectDrawBlitter();
	virtual void draw();
	virtual void init();
	virtual long frameTimeEst() const;
	virtual int present();
	virtual void uninit();
	virtual void setExclusive(bool exclusive);

	virtual QWidget * settingsWidget() const { return confWidget.get(); }
	virtual void acceptSettings();
	virtual void rejectSettings() const;

	virtual QPaintEngine * paintEngine () const { return 0; }

	virtual void rateChange(int dhz);
	virtual void setSwapInterval(unsigned si);

protected:
	virtual void consumeBuffer(SetBuffer setInputBuffer);
	virtual void paintEvent(QPaintEvent *event);
	virtual void setBufferDimensions(unsigned w, unsigned h, SetBuffer setInputBuffer);

private:
	FtEst ftEst;
	scoped_ptr<QWidget> const confWidget;
	QComboBox *const deviceSelector;
	PersistCheckBox vblank_;
	PersistCheckBox flipping_;
	PersistCheckBox vblankflip_;
	PersistCheckBox triplebuf_;
	PersistCheckBox videoSurface_;
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

	static BOOL WINAPI enumCallback(GUID FAR *, char *, char *, LPVOID, HMONITOR);
	void initPrimarySurface();
	void initVideoSurface();
	void initClearSurface();
	void restoreSurfaces();
	void systemSurfaceBlit();
	void videoSurfaceBlit();
	HRESULT backBlit(IDirectDrawSurface7 *lpDDSSrc, RECT *rcRectDest, DWORD flags);
	void finalBlit(DWORD waitFlag);
	void reinit();
};

#endif /*DIRECTDRAWBLITTER_H_*/
