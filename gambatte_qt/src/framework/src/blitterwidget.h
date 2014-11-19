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

#ifndef BLITTERWIDGET_H
#define BLITTERWIDGET_H

#include "pixelbuffer.h"
#include "uncopyable.h"
#include "usec.h"
#include "videobufferlocker.h"
#include <QSize>
#include <QString>
#include <QWidget>

class FtEst {
public:
	explicit FtEst(long frameTime = 0);
	void update(usec_t t);
	long est() const { return (ftAvg_ + ftavg_scale / 2) >> ftavg_shift; }

private:
	enum { ftavg_shift = 5 };
	enum { ftavg_scale = 1 << ftavg_shift };

	long frameTime_;
	long ftAvg_;
	usec_t last_;
	double t_, c_, tc_, c2_;
};

class BlitterWidget : public QWidget {
protected:
	BlitterWidget(VideoBufferLocker,
	              QString const &name,
	              unsigned maxSwapInterval = 0,
	              QWidget *parent = 0);

	class BufferLock : Uncopyable {
	public:
		explicit BufferLock(BlitterWidget &blitter) : b_(blitter) { b_.vbl_.lock(); }
		~BufferLock() { b_.vbl_.unlock(); }
		BlitterWidget & blitterWidget() { return b_; }
	private:
		BlitterWidget &b_;
	};

	class SetBuffer {
	public:
		SetBuffer(PixelBuffer &pb) : pb_(pb) {}
		SetBuffer(BufferLock &lock) : pb_(lock.blitterWidget().pixbuf_) {}
		void operator()(void *data, PixelBuffer::PixelFormat f, std::ptrdiff_t pitch) const {
			pb_.data = data;
			pb_.pixelFormat = f;
			pb_.pitch = pitch;
		}

	private:
		PixelBuffer &pb_;
	};

	virtual void consumeBuffer(SetBuffer setBuffer) = 0;
	virtual void privSetPaused(bool paused) { setUpdatesEnabled(paused); }
	virtual void setBufferDimensions(unsigned width, unsigned height, SetBuffer ) = 0;

public:
	QString const & nameString() const { return nameString_; }
	unsigned maxSwapInterval() const { return maxSwapInterval_; }
	bool isPaused() const { return paused_; }
	void setPaused(bool paused) { paused_ = paused; privSetPaused(paused); }
	PixelBuffer const & inBuffer() const { return pixbuf_; }
	void consumeInputBuffer() { consumeBuffer(SetBuffer(pixbuf_)); }

	void setVideoFormat(QSize const &size) {
		pixbuf_.width = size.width();
		pixbuf_.height = size.height();
		setBufferDimensions(pixbuf_.width, pixbuf_.height, SetBuffer(pixbuf_));
	}

	virtual ~BlitterWidget() {}
	virtual bool isUnusable() const { return false; }

	// TODO: prefer create/destroy to init/uninit
	virtual void init() {}
	virtual void uninit() = 0;
	virtual void draw() {}
	virtual int present() = 0;
	virtual long frameTimeEst() const { return 0; }

	virtual QWidget * settingsWidget() const { return 0; }
	virtual void acceptSettings() {}
	virtual void rejectSettings() const {}

	virtual void setCorrectedGeometry(int w, int h, int new_w, int new_h) {
		setGeometry((w - new_w) >> 1, (h - new_h) >> 1, new_w, new_h);
	}

	virtual void compositionEnabledChange() {}
	virtual void rateChange(int /*newHz*/) {}
	virtual void setExclusive(bool) {}
	virtual void setSwapInterval(unsigned) {}
	virtual WId hwnd() const { return winId(); }

private:
	VideoBufferLocker const vbl_;
	QString const nameString_;
	unsigned const maxSwapInterval_;
	PixelBuffer pixbuf_;
	bool paused_;
};

#endif
