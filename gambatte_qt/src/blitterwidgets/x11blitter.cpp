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
#include <QPaintEvent>

#include "x11blitter.h"

#include "../scalebuffer.h"
#include "../videobufferreseter.h"

#include <QX11Info>
#include <X11/Xlib.h>
// #include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <stdint.h>

#include <videoblitter.h>

#include <iostream>

class X11SubBlitter {
public:
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) = 0;
	virtual bool failed() = 0;
	virtual const PixelBuffer inBuffer() = 0;
	virtual ~X11SubBlitter() {};
};

class X11ShmBlitter : public X11SubBlitter {
	XShmSegmentInfo shminfo;
	XImage *ximage;
	
public:
	X11ShmBlitter(unsigned int width, unsigned int height);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed();
	const PixelBuffer inBuffer();
	~X11ShmBlitter();
};

X11ShmBlitter::X11ShmBlitter(const unsigned int width, const unsigned int height) {
	shminfo.shmaddr = NULL;
	std::cout << "creating shm ximage...\n";
	ximage = XShmCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(QX11Info::appVisual()), QX11Info::appDepth(), ZPixmap, NULL, &shminfo, width, height);
	
	if (ximage == NULL) {
		std::cout << "failed to create shm ximage\n";
	} else {
		shminfo.shmid = shmget(IPC_PRIVATE, ximage->bytes_per_line * ximage->height, IPC_CREAT | 0777);
		shminfo.shmaddr = ximage->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
		shminfo.readOnly = True;
		XShmAttach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
	}
}

X11ShmBlitter::~X11ShmBlitter() {
	if (shminfo.shmaddr != NULL) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}
	
	if (ximage != NULL)
		XFree(ximage);
}

void X11ShmBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage && shminfo.shmaddr) {
		XShmPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h, False);
	}
}

bool X11ShmBlitter::failed() {
	return ximage == NULL || shminfo.shmaddr == NULL;
}

const PixelBuffer X11ShmBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = QX11Info::appDepth() == 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32;
	pixb.pixels = ximage ? ximage->data : NULL;
	pixb.pitch = ximage ? ximage->width : 0;
	
	return pixb;
}

class X11PlainBlitter : public X11SubBlitter {
	XImage *ximage;
	
public:
	X11PlainBlitter (unsigned int width, unsigned int height);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed();
	const PixelBuffer inBuffer();
	~X11PlainBlitter ();
};

X11PlainBlitter::X11PlainBlitter(const unsigned int width, const unsigned int height) {
	std::cout << "creating ximage...\n";
	ximage = XCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(QX11Info::appVisual()), QX11Info::appDepth(), ZPixmap, 0, NULL, width, height, QX11Info::appDepth(), 0);
	
	if (ximage == NULL) {
		std::cout << "failed to create ximage\n";
	} else {
		ximage->data = new char[ximage->bytes_per_line * ximage->height];
	}
}

X11PlainBlitter::~X11PlainBlitter () {
	if (ximage) {
		delete[] ximage->data;
		XFree(ximage);
	}
}

void X11PlainBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage) {
		XPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h);
	}
}

bool X11PlainBlitter::failed() {
	return ximage == NULL;
}

const PixelBuffer X11PlainBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = QX11Info::appDepth() == 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32;
	pixb.pixels = ximage ? ximage->data : NULL;
	pixb.pitch = ximage ? ximage->width : 0;
	
	return pixb;
}

X11Blitter::X11Blitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent) :
BlitterWidget(QString("X11"), true, parent),
resetVideoBuffer(resetVideoBuffer_in),
subBlitter(NULL),
buffer(NULL),
inWidth(160),
inHeight(144),
scale(0)
{
	/*QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(0,0,0));
	setPalette(pal);
	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);*/
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
}

bool X11Blitter::isUnusable() {
	return !(QX11Info::appDepth() == 16 || QX11Info::appDepth() == 24 || QX11Info::appDepth() == 32);
}

void X11Blitter::init() {
	XSync(QX11Info::display(), 0);
	
	shm = XShmQueryExtension(QX11Info::display());
// 	shm = false;
	std::cout << "shm: " << shm << std::endl;
}

void X11Blitter::uninit() {
	delete subBlitter;
	subBlitter = NULL;
	
	delete[] buffer;
	buffer = NULL;
}

X11Blitter::~X11Blitter() {
	uninit();
}

int X11Blitter::sync(const bool turbo) {
	if (subBlitter->failed())
		return -1;
	
	return BlitterWidget::sync(turbo);
}

void X11Blitter::keepAspectRatio(const bool /*enable*/) {
// 	keepRatio = enable;
}

bool X11Blitter::keepsAspectRatio() {
	return true;
}

void X11Blitter::scaleByInteger(const bool /*enable*/) {
// 	integerScaling = enable;
}

bool X11Blitter::scalesByInteger() {
	return true;
}

/*
void X11Blitter::paintEvent(QPaintEvent *event) {
	if(event)
		event->accept();

	if(xvimage) {
		XvShmPutImage(QX11Info::display(), xvport, winId(), DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0, inWidth*2, inHeight, 0, 0, width(), height(), False);
		XSync(QX11Info::display(),0);
	}
}
*/

void X11Blitter::paintEvent(QPaintEvent *event) {
	if (subBlitter) {
		event->accept();
		const QRect &rect = event->rect();
		subBlitter->blit(winId(), rect.x(), rect.y(), rect.width(), rect.height());
	}
}

void X11Blitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	inWidth = w;
	inHeight = h;
	
	scale = std::min(width() / w, height() / h);
	
	uninit();
	
	if (scale > 1)
		buffer = new char[w * h * (QX11Info::appDepth() == 16 ? 2 : 4)];
	
	if (shm) {
		subBlitter = new X11ShmBlitter(w * scale, h * scale);
		
		if (subBlitter->failed()) {
			shm = false;
			delete subBlitter;
		}
	}
	
	if (!shm)
		subBlitter = new X11PlainBlitter (w * scale, h * scale);
}

const PixelBuffer X11Blitter::inBuffer() {
	if (buffer) {
		PixelBuffer pixb;
		pixb.format = QX11Info::appDepth() == 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32;
		pixb.pixels = buffer;
		pixb.pitch = inWidth;
		
		return pixb;
	}
	
	return subBlitter->inBuffer();
}

void X11Blitter::blit() {
	if (buffer) {
		const PixelBuffer &pb = subBlitter->inBuffer();
		
		if (QX11Info::appDepth() == 16) {
			scaleBuffer(reinterpret_cast<uint16_t*>(buffer), reinterpret_cast<uint16_t*>(pb.pixels), inWidth, inHeight, scale);
		} else {
			scaleBuffer(reinterpret_cast<uint32_t*>(buffer), reinterpret_cast<uint32_t*>(pb.pixels), inWidth, inHeight, scale);
		}
	}
	
	subBlitter->blit(winId(), 0, 0, width(), height());
	XSync(QX11Info::display(), 0);
}

void X11Blitter::resizeEvent(QResizeEvent */*event*/) {
	const unsigned newScale = std::min(width() / inWidth, height() / inHeight);
		
	if (newScale != scale) {
		scale = newScale;
		
		if (subBlitter) {
			setBufferDimensions(inWidth, inHeight);
			resetVideoBuffer();
		}
	}
}
