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
	ShmBlitter(unsigned int width, unsigned int height, const VisInfo &visInfo);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~ShmBlitter();
};

X11Blitter::ShmBlitter::ShmBlitter(const unsigned int width, const unsigned int height, const VisInfo &visInfo) {
	shminfo.shmaddr = NULL;
	std::cout << "creating shm ximage...\n";
	ximage = XShmCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(visInfo.visual), visInfo.depth, ZPixmap, NULL, &shminfo, width, height);
	
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
	PlainBlitter (unsigned int width, unsigned int height, const VisInfo &visInfo);
	void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~PlainBlitter ();
};

X11Blitter::PlainBlitter::PlainBlitter(const unsigned int width, const unsigned int height, const VisInfo &visInfo) {
	std::cout << "creating ximage...\n";
	ximage = XCreateImage(QX11Info::display(), reinterpret_cast<Visual*>(visInfo.visual), visInfo.depth, ZPixmap, 0, NULL, width, height, visInfo.depth <= 16 ? 16 : 32, 0);
	
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
			if (vinfos->depth == 24 && vinfos->red_mask == 0xFF0000 && vinfos->green_mask == 0x00FF00 && vinfos->blue_mask == 0x0000FF ||
					vinfos->depth <= 16 && vinfos->red_mask == 0xF800 && vinfos->green_mask == 0x07E0 && vinfos->blue_mask == 0x001F) {
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

X11Blitter::X11Blitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
BlitterWidget(setPixelBuffer, QString("X11"), false, parent),
confWidget(new QWidget),
bfBox(new QCheckBox(QString("Bilinear filtering"))),
buffer(NULL),
inWidth(160),
inHeight(144),
bf(false)
{
	visInfo.visual = NULL;
	visInfo.depth = 0;
		
	QSettings settings;
	settings.beginGroup("x11blitter");
	bf = settings.value("bf", false).toBool();
	settings.endGroup();
	
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(bfBox);
	bfBox->setChecked(bf);
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	
	if (XVisualInfo *v = getVisualPtr()) {
		visInfo.visual = v->visual;
		visInfo.depth = v->depth;
		XFree(v);
	}
}

bool X11Blitter::isUnusable() const {
	return visInfo.visual == NULL;
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
	
	QSettings settings;
	settings.beginGroup("x11blitter");
	settings.setValue("bf", bf);
	settings.endGroup();
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
	
	uninit();
	
	if (shm) {
		subBlitter.reset(new ShmBlitter(width(), height(), visInfo));
		
		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset(); // predestruct previous object to ensure resource availability.
		}
	}
	
	if (!shm)
		subBlitter.reset(new PlainBlitter(width(), height(), visInfo));
	
	if (width() != static_cast<int>(w) || height() != static_cast<int>(h)) {
		buffer = new char[w * h * (visInfo.depth <= 16 ? 2 : 4)];
		setPixelBuffer(buffer, visInfo.depth <= 16 ? MediaSource::RGB16 : MediaSource::RGB32, w);
	} else
		setPixelBuffer(subBlitter->pixels(), visInfo.depth <= 16 ? MediaSource::RGB16 : MediaSource::RGB32, subBlitter->pitch());
}

void X11Blitter::blit() {
	if (buffer) {
		if (visInfo.depth <= 16) {
			if (bf) {
				linearScale<quint16, 0xF81F, 0x07E0, 6>(reinterpret_cast<quint16*>(buffer),
				                                        static_cast<quint16*>(subBlitter->pixels()),
				                                        inWidth, inHeight, width(), height(), subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint16*>(buffer),
				                     static_cast<quint16*>(subBlitter->pixels()),
				                     inWidth, inHeight, width(), height(), subBlitter->pitch());
			}
		} else {
			if (bf) {
				linearScale<quint32, 0xFF00FF, 0x00FF00, 8>(reinterpret_cast<quint32*>(buffer),
				                                            static_cast<quint32*>(subBlitter->pixels()),
				                                            inWidth, inHeight, width(), height(), subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint32*>(buffer),
				                     static_cast<quint32*>(subBlitter->pixels()),
				                     inWidth, inHeight, width(), height(), subBlitter->pitch());
			}
		}
	}
	
	subBlitter->blit(winId(), 0, 0, width(), height());
	XSync(QX11Info::display(), 0);
}

void X11Blitter::resizeEvent(QResizeEvent */*event*/) {
	if (subBlitter.get())
		setBufferDimensions(inWidth, inHeight);
}

void X11Blitter::acceptSettings() {
	bf = bfBox->isChecked();
}

void X11Blitter::rejectSettings() {
	bfBox->setChecked(bf);
}
