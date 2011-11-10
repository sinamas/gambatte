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
#include "x11blitter.h"
#include "../swscale.h"
#include <QPaintEvent>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
// #include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>
#include <iostream>

class X11Blitter::SubBlitter {
public:
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) = 0;
	virtual void flip() = 0;
	virtual bool failed() const = 0;
	virtual void* pixels() const = 0;
	virtual unsigned pitch() const = 0;
	virtual ~SubBlitter() {};
};

class X11Blitter::ShmBlitter : public SubBlitter, Uncopyable {
	XShmSegmentInfo shminfo;
	XImage *ximage;
	const bool doubleBuffer;
public:
	ShmBlitter(unsigned int width, unsigned int height, const VisInfo &visInfo, bool db);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	void flip();
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~ShmBlitter();
};

X11Blitter::ShmBlitter::ShmBlitter(const unsigned width, const unsigned height, const VisInfo &visInfo, const bool db) : doubleBuffer(db) {
	shminfo.shmaddr = NULL;
	ximage = XShmCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(visInfo.visual), visInfo.depth, ZPixmap, NULL, &shminfo, width, height);
	
	if (ximage == NULL) {
		std::cerr << "failed to create shm ximage\n";
	} else {
		shminfo.shmid = shmget(IPC_PRIVATE, ximage->bytes_per_line * ximage->height << db, IPC_CREAT | 0777);
		shminfo.shmaddr = ximage->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
		shminfo.readOnly = True;
		XShmAttach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
	}
}

X11Blitter::ShmBlitter::~ShmBlitter() {
	if (ximage) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
		
		XFree(ximage);
	}
}

void X11Blitter::ShmBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage && ximage->data) {
		XShmPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h, False);
	}
}

void X11Blitter::ShmBlitter::flip() {
	if (ximage)
		ximage->data = static_cast<char*>(pixels());
}

bool X11Blitter::ShmBlitter::failed() const {
	return ximage == NULL || ximage->data == NULL;
}

void* X11Blitter::ShmBlitter::pixels() const {
	if (!ximage)
		return 0;
	
	if (!doubleBuffer)
		return ximage->data;

	return ximage->data == shminfo.shmaddr ? shminfo.shmaddr + ximage->bytes_per_line * ximage->height : shminfo.shmaddr;
}

unsigned X11Blitter::ShmBlitter::pitch() const {
	return ximage ? ximage->width : 0;
}

class X11Blitter::PlainBlitter : public X11Blitter::SubBlitter, Uncopyable {
	XImage *const ximage;
	char *data;
	
	bool doubleBuffer() const { return data; }
public:
	PlainBlitter(unsigned int width, unsigned int height, const VisInfo &visInfo, bool db);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	void flip();
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~PlainBlitter();
};

X11Blitter::PlainBlitter::PlainBlitter(const unsigned int width, const unsigned int height, const VisInfo &visInfo, const bool db)
: ximage(XCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(visInfo.visual),
			visInfo.depth, ZPixmap, 0, NULL, width, height, visInfo.depth <= 16 ? 16 : 32, 0)),
  data(0)
{
	if (ximage) {
		ximage->data = new char[ximage->bytes_per_line * ximage->height << db];
		
		if (db)
			data = ximage->data;
	} else {
		std::cerr << "failed to create ximage\n";
	}
}

X11Blitter::PlainBlitter::~PlainBlitter () {
	if (ximage) {
		if (doubleBuffer()) {
			delete[] data;
		} else
			delete[] ximage->data;
		
		XFree(ximage);
	}
}

void X11Blitter::PlainBlitter::blit(const Drawable drawable, const unsigned x, const unsigned y, const unsigned w, const unsigned h) {
	if (ximage) {
		XPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), ximage, x, y, x, y, w, h);
	}
}

void X11Blitter::PlainBlitter::flip() {
	if (ximage) {
		ximage->data = static_cast<char*>(pixels());
	}
}

bool X11Blitter::PlainBlitter::failed() const {
	return ximage == NULL;
}

void* X11Blitter::PlainBlitter::pixels() const {
	if (!ximage)
		return 0;
	
	if (!doubleBuffer())
		return ximage->data;
	
	return ximage->data == data ? data + ximage->bytes_per_line * ximage->height : data;
}

unsigned X11Blitter::PlainBlitter::pitch() const {
	return ximage ? ximage->width : 0;
}

static XVisualInfo* getVisualPtr(unsigned depth, int c_class, unsigned long rmask, unsigned long gmask, unsigned long bmask) {
	XVisualInfo vinfo_template;
	vinfo_template.screen = QX11Info::appScreen();
	vinfo_template.depth = depth;
	vinfo_template.c_class = c_class;
	vinfo_template.red_mask = rmask;
	vinfo_template.green_mask = gmask;
	vinfo_template.blue_mask = bmask;
	
	int nitems = 0;
	XVisualInfo *vinfos = XGetVisualInfo(QX11Info::display(),
	                                     VisualScreenMask|(depth ? VisualDepthMask : 0)|VisualClassMask|VisualRedMaskMask|VisualGreenMaskMask|VisualBlueMaskMask,
	                                     &vinfo_template, &nitems);
	
	if (nitems > 0) {
		return vinfos;
	}
	
	return NULL;
}

static XVisualInfo* getVisualPtr() {
	{
		XVisualInfo vinfo_template;
		vinfo_template.visualid = XVisualIDFromVisual(XDefaultVisual(QX11Info::display(), QX11Info::appScreen()));
		
		int nitems = 0;
		XVisualInfo *vinfos = XGetVisualInfo(QX11Info::display(), VisualIDMask, &vinfo_template, &nitems);
		
		if (nitems > 0) {
			if ((vinfos->depth == 24 && vinfos->red_mask == 0xFF0000 && vinfos->green_mask == 0x00FF00 && vinfos->blue_mask == 0x0000FF) ||
					(vinfos->depth <= 16 && vinfos->red_mask == 0xF800 && vinfos->green_mask == 0x07E0 && vinfos->blue_mask == 0x001F)) {
				return vinfos;
			}
			
			XFree(vinfos);
		}
	}
	
	if (XVisualInfo *visual = getVisualPtr(24, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	
	if (XVisualInfo *visual = getVisualPtr(16, TrueColor, 0xF800, 0x07E0, 0x001F))
		return visual;
	
	if (XVisualInfo *visual = getVisualPtr(0, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	
	if (XVisualInfo *visual = getVisualPtr(0, TrueColor, 0xF800, 0x07E0, 0x001F))
		return visual;
	
	if (XVisualInfo *visual = getVisualPtr(0, DirectColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	
	if (XVisualInfo *visual = getVisualPtr(0, DirectColor, 0xF800, 0x07E0, 0x001F))
		return visual;
	
	return NULL;
}

X11Blitter::X11Blitter(VideoBufferLocker vbl, QWidget *parent) :
BlitterWidget(vbl, QString("X11"), false, parent),
confWidget(new QWidget),
bf_(new QCheckBox(QString("Semi-bilinear filtering")), "x11blitter/bf", false)
{
	visInfo.visual = NULL;
	visInfo.depth = 0;
	
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(bf_.checkBox());
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	
	if (XVisualInfo *v = getVisualPtr()) {
		visInfo.visual = v->visual;
		visInfo.depth = v->depth;
		XFree(v);
	}
}

X11Blitter::~X11Blitter() {
}

bool X11Blitter::isUnusable() const {
	return visInfo.visual == NULL;
}

void X11Blitter::init() {
	XSync(QX11Info::display(), 0);
}

void X11Blitter::uninit() {
	subBlitter.reset();
	buffer.reset();
}

long X11Blitter::sync() {
	if (subBlitter->failed())
		return -1;
	
	subBlitter->blit(winId(), 0, 0, width(), height());
	XSync(QX11Info::display(), 0);
	
	return 0;
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
	uninit();
	
	const bool scale = width() != static_cast<int>(w) || height() != static_cast<int>(h);
	bool shm = XShmQueryExtension(QX11Info::display());
	std::cout << "shm: " << shm << std::endl;
	
	if (shm) {
		subBlitter.reset(new ShmBlitter(width(), height(), visInfo, !scale));
		
		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset(); // predestruct previous object to ensure resource availability.
		}
	}
	
	if (!shm)
		subBlitter.reset(new PlainBlitter(width(), height(), visInfo, !scale));
	
	if (scale) {
		buffer.reset(w * h * (visInfo.depth <= 16 ? 2 : 4));
		setPixelBuffer(buffer, visInfo.depth <= 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32, w);
	} else
		setPixelBuffer(subBlitter->pixels(), visInfo.depth <= 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32, subBlitter->pitch());
}

void X11Blitter::blit() {
	if (buffer) {
		if (visInfo.depth <= 16) {
			if (bf_.value()) {
				semiLinearScale<quint16, 0xF81F, 0x07E0, 6>(
						reinterpret_cast<quint16*>(buffer.get()), static_cast<quint16*>(subBlitter->pixels()),
						inBuffer().width, inBuffer().height, width(), height(), subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint16*>(buffer.get()),
				                     static_cast<quint16*>(subBlitter->pixels()),
				                     inBuffer().width, inBuffer().height, width(), height(), subBlitter->pitch());
			}
		} else {
			if (bf_.value()) {
				semiLinearScale<quint32, 0xFF00FF, 0x00FF00, 8>(
						reinterpret_cast<quint32*>(buffer.get()), static_cast<quint32*>(subBlitter->pixels()),
						inBuffer().width, inBuffer().height, width(), height(), subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint32*>(buffer.get()),
				                     static_cast<quint32*>(subBlitter->pixels()),
				                     inBuffer().width, inBuffer().height, width(), height(), subBlitter->pitch());
			}
		}
	} else {
		subBlitter->flip();
		setPixelBuffer(subBlitter->pixels(), inBuffer().pixelFormat, subBlitter->pitch());
	}
}

void X11Blitter::resizeEvent(QResizeEvent */*event*/) {
	if (subBlitter.get()) {
		lockPixelBuffer();
		setBufferDimensions(inBuffer().width, inBuffer().height);
		unlockPixelBuffer();
	}
}

void X11Blitter::acceptSettings() {
	bf_.accept();
}

void X11Blitter::rejectSettings() const {
	bf_.reject();
}
