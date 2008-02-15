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

#include <QX11Info>
#include <X11/Xlib.h>
// #include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <stdint.h>

#include <videoblitter.h>

#include <iostream>

class X11Blitter::SubBlitter {
public:
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) = 0;
	virtual bool failed() const = 0;
	virtual void* pixels() const = 0;
	virtual unsigned pitch() const = 0;
	virtual ~SubBlitter() {};
};

class X11Blitter::ShmBlitter : public SubBlitter {
	XShmSegmentInfo shminfo;
	XImage *ximage;
	
public:
	ShmBlitter(unsigned int width, unsigned int height);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~ShmBlitter();
};

X11Blitter::ShmBlitter::ShmBlitter(const unsigned int width, const unsigned int height) {
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

X11Blitter::ShmBlitter::~ShmBlitter() {
	if (shminfo.shmaddr != NULL) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}
	
	if (ximage != NULL)
		XFree(ximage);
}

void X11Blitter::ShmBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage && shminfo.shmaddr) {
		XShmPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h, False);
	}
}

bool X11Blitter::ShmBlitter::failed() const {
	return ximage == NULL || shminfo.shmaddr == NULL;
}

void* X11Blitter::ShmBlitter::pixels() const {
	return ximage ? ximage->data : NULL;
}

unsigned X11Blitter::ShmBlitter::pitch() const {
	return ximage ? ximage->width : 0;
}

class X11Blitter::PlainBlitter : public X11Blitter::SubBlitter {
	XImage *ximage;
	
public:
	PlainBlitter (unsigned int width, unsigned int height);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~PlainBlitter ();
};

X11Blitter::PlainBlitter::PlainBlitter(const unsigned int width, const unsigned int height) {
	std::cout << "creating ximage...\n";
	ximage = XCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(QX11Info::appVisual()), QX11Info::appDepth(), ZPixmap, 0, NULL, width, height, QX11Info::appDepth() == 16 ? 16 : 32, 0);
	
	if (ximage == NULL) {
		std::cout << "failed to create ximage\n";
	} else {
		ximage->data = new char[ximage->bytes_per_line * ximage->height];
	}
}

X11Blitter::PlainBlitter::~PlainBlitter () {
	if (ximage) {
		delete[] ximage->data;
		XFree(ximage);
	}
}

void X11Blitter::PlainBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage) {
		XPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h);
	}
}

bool X11Blitter::PlainBlitter::failed() const {
	return ximage == NULL;
}

void* X11Blitter::PlainBlitter::pixels() const {
	return ximage ? ximage->data : NULL;
}

unsigned X11Blitter::PlainBlitter::pitch() const {
	return ximage ? ximage->width : 0;
}

X11Blitter::X11Blitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
BlitterWidget(setPixelBuffer, QString("X11"), true, parent),
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

bool X11Blitter::isUnusable() const {
	return !(QX11Info::appDepth() == 16 || QX11Info::appDepth() == 24 || QX11Info::appDepth() == 32);
}

void X11Blitter::init() {
	XSync(QX11Info::display(), 0);
	
	shm = XShmQueryExtension(QX11Info::display());
// 	shm = false;
	std::cout << "shm: " << shm << std::endl;
}

void X11Blitter::uninit() {
	subBlitter.reset();
	
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
	if (subBlitter.get()) {
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
	
	if (shm) {
		subBlitter.reset(new ShmBlitter(w * scale, h * scale));
		
		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset(); // predestruct previous object to ensure resource availability.
		}
	}
	
	if (!shm)
		subBlitter.reset(new PlainBlitter (w * scale, h * scale));
	
	if (scale > 1) {
		buffer = new char[w * h * (QX11Info::appDepth() == 16 ? 2 : 4)];
		setPixelBuffer(buffer, QX11Info::appDepth() == 16 ? MediaSource::RGB16 : MediaSource::RGB32, w);
	} else
		setPixelBuffer(subBlitter->pixels(), QX11Info::appDepth() == 16 ? MediaSource::RGB16 : MediaSource::RGB32, subBlitter->pitch());
}

void X11Blitter::blit() {
	if (buffer) {
		if (QX11Info::appDepth() == 16) {
			scaleBuffer(reinterpret_cast<quint16*>(buffer), reinterpret_cast<quint16*>(subBlitter->pixels()), inWidth, inHeight, subBlitter->pitch(), scale);
		} else {
			scaleBuffer(reinterpret_cast<quint32*>(buffer), reinterpret_cast<quint32*>(subBlitter->pixels()), inWidth, inHeight, subBlitter->pitch(), scale);
		}
	}
	
	subBlitter->blit(winId(), 0, 0, width(), height());
	XSync(QX11Info::display(), 0);
}

void X11Blitter::resizeEvent(QResizeEvent */*event*/) {
	const unsigned newScale = std::min(width() / inWidth, height() / inHeight);
		
	if (newScale != scale) {
		scale = newScale;
		
		if (subBlitter.get())
			setBufferDimensions(inWidth, inHeight);
	}
}
