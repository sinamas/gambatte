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
#ifndef BLITTERWIDGET_H
#define BLITTERWIDGET_H

#include <QWidget>
#include <QString>
#include <memory>
#include <usec.h>
#include "mediasource.h"
#include "pixelbuffer.h"
#include "videobufferlocker.h"

class QHBoxLayout;

class FtEst {
	enum { UPSHIFT= 5 };
	enum { UP = 1 << UPSHIFT };

	long frameTime;
	long ft;
	long ftAvg;
	usec_t last;
	unsigned count;

public:
	explicit FtEst(long frameTime = 0) { init(frameTime); }
	void init(long frameTime);
	void update(usec_t t);
	long est() const { return (ftAvg + UP / 2) >> UPSHIFT; }
};

class BlitterWidget : public QWidget {
	PixelBuffer pixbuf;
	VideoBufferLocker vbl;
	const QString nameString_;
	const unsigned maxSwapInterval_;
	bool paused;
	
protected:
	virtual void privSetPaused(const bool paused) { setUpdatesEnabled(paused); }
	virtual void setBufferDimensions(unsigned width, unsigned height) = 0;
	
	// use these if modifying pixel buffer in the sync method, or in an event method.
	void lockPixelBuffer() { vbl.lock(); }
	void unlockPixelBuffer() { vbl.unlock(); }
	
	void setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, unsigned pitch) {
		pixbuf.data = pixels;
		pixbuf.pitch = pitch;
		pixbuf.pixelFormat = format;
	}

public:
	BlitterWidget(VideoBufferLocker, const QString &name,
	              unsigned maxSwapInterval = 0,
	              QWidget *parent = 0);
	virtual ~BlitterWidget() {}
	const QString& nameString() const { return nameString_; }
	unsigned maxSwapInterval() const { return maxSwapInterval_; }
	bool isPaused() const { return paused; }
	void setPaused(const bool paused) { this->paused = paused; privSetPaused(paused); }
	const PixelBuffer& inBuffer() const { return pixbuf; }

	virtual void init() {}
	virtual void uninit() {}
	virtual void blit() = 0;
	virtual void draw() {}
	virtual bool isUnusable() const { return false; }
	
	void setVideoFormat(unsigned width, unsigned height/*, PixelBuffer::PixelFormat pf*/) {
		pixbuf.width = width;
		pixbuf.height = height;
		setBufferDimensions(width, height);
// 		return pf == pixbuf.pixelFormat;
	}
	
	virtual void setCorrectedGeometry(int w, int h, int new_w, int new_h) {
		setGeometry((w - new_w) >> 1, (h - new_h) >> 1, new_w, new_h);
	}
	
	virtual WId hwnd() const { return winId(); }
	virtual long frameTimeEst() const { return 0; }
	virtual long sync() { return 0; }
	virtual void setExclusive(bool) {}
	virtual void setSwapInterval(unsigned) {}
	
	virtual QWidget* settingsWidget() const { return 0; }
	virtual void acceptSettings() {}
	virtual void rejectSettings() const {}

	virtual void rateChange(int /*newHz*/) {}
	virtual void compositionEnabledChange() {}
};

#endif
