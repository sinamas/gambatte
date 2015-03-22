//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef DIRECT3DBLITTER_H_
#define DIRECT3DBLITTER_H_

#include "../blitterwidget.h"
#include "dialoghelpers.h"
#include "scoped_ptr.h"
#include <d3d9.h>

class QCheckBox;
class QComboBox;

class Direct3DBlitter : public BlitterWidget {
public:
	explicit Direct3DBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	virtual ~Direct3DBlitter();
	virtual void init();
	virtual void uninit();
	virtual void draw();
	virtual int present();
	virtual long frameTimeEst() const;
	virtual void setExclusive(bool exclusive);
	virtual bool isUnusable() const { return !d3d; }
	virtual QWidget * settingsWidget() const { return confWidget.get(); }
	virtual void acceptSettings();
	virtual void rejectSettings() const;

	virtual QPaintEngine * paintEngine () const { return 0; }
	virtual void setSwapInterval(unsigned si);
	virtual void rateChange(int dhz);
	virtual void compositionEnabledChange();

protected:
	virtual void consumeBuffer(SetBuffer setInputBuffer);
	virtual void paintEvent(QPaintEvent *e);
	virtual void resizeEvent(QResizeEvent *e);
	virtual void setBufferDimensions(unsigned w, unsigned h, SetBuffer setInputBuffer);

private:
	FtEst ftEst;
	scoped_ptr<QWidget> const confWidget;
	QComboBox *const adapterSelector;
	PersistCheckBox vblankblit_;
	PersistCheckBox flipping_;
	PersistCheckBox vblankflip_;
	PersistCheckBox triplebuf_;
	PersistCheckBox bf_;
	HMODULE d3d9handle;
	IDirect3D9 *d3d;
	IDirect3DDevice9* device;
	IDirect3DVertexBuffer9 *vertexBuffer;
	IDirect3DTexture9 *stexture;
	IDirect3DTexture9 *vtexture;
	usec_t lastblank;
	unsigned backBufferWidth;
	unsigned backBufferHeight;
	unsigned clear;
	unsigned dhz;
	unsigned swapInterval;
	unsigned adapterIndex;
	bool exclusive;
	bool windowed;
	bool drawn;

	void getPresentParams(D3DPRESENT_PARAMETERS *presentParams) const;
	unsigned textureSizeFromInBufferSize() const;
	void lockTexture(SetBuffer setInputBuffer);
	void setVertexBuffer();
	void setVideoTexture();
	void setFilter();
	void setDeviceState();
	void resetDevice();
	void exclusiveChange();
	void doPresent();
};

#endif /*DIRECT3DBLITTER_H_*/
