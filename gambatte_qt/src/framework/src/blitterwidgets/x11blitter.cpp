/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#include "../blitterwidget.h"
#include "../swscale.h"
#include "array.h"
#include "persistcheckbox.h"
#include "scoped_ptr.h"
#include <QCheckBox>
#include <QPaintEvent>
#include <QSettings>
#include <QVBoxLayout>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <iostream>

namespace {

static std::size_t imageDataSize(XImage const &ximage) {
	return std::size_t(ximage.height) * ximage.bytes_per_line;
}

struct XDeleter { template<class T> static void del(T *p) { if (p) { XFree(p); } } };

template<class T>
struct x_ptr {
	typedef scoped_ptr<T, XDeleter> scoped;
	typedef transfer_ptr<T, XDeleter> transfer;
};

struct VisualInfo {
	Visual *visual;
	unsigned depth;
};

class SubBlitter {
public:
	virtual ~SubBlitter() {};
	virtual bool failed() const = 0;
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) = 0;
	virtual void flip() = 0;
	virtual std::ptrdiff_t pitch() const = 0;
	virtual void * pixels() const = 0;
};

class ShmBlitter : public SubBlitter {
public:
	ShmBlitter(unsigned width, unsigned height, const VisualInfo &visInfo, bool db);
	virtual ~ShmBlitter();
	virtual bool failed() const { return !shminfo.shmaddr; }
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	virtual void flip();
	virtual std::ptrdiff_t pitch() const { return ximage ? ximage->width : 0; }
	virtual void * pixels() const { return backBuffer(); }

private:
	XShmSegmentInfo shminfo;
	x_ptr<XImage>::scoped const ximage;
	bool const doubleBuffer;

	char * backBuffer() const;
	char * frontBuffer() const { return ximage ? ximage->data : 0; }
};

ShmBlitter::ShmBlitter(const unsigned width, const unsigned height,
                       const VisualInfo &visInfo, const bool db)
: shminfo()
, ximage(XShmCreateImage(QX11Info::display(), visInfo.visual, visInfo.depth,
                         ZPixmap, 0, &shminfo, width, height))
, doubleBuffer(db)
{
	if (ximage) {
		shminfo.shmid = shmget(IPC_PRIVATE, imageDataSize(*ximage) << db, IPC_CREAT | 0777);
		shminfo.shmaddr = ximage->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
		shminfo.readOnly = True;
		XShmAttach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
	} else {
		std::cerr << "failed to create shm ximage\n";
		shminfo.shmaddr = 0;
	}
}

ShmBlitter::~ShmBlitter() {
	if (ximage) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}
}

char * ShmBlitter::backBuffer() const {
	if (doubleBuffer) {
		return shminfo.shmaddr && frontBuffer() == shminfo.shmaddr
		     ? shminfo.shmaddr + imageDataSize(*ximage)
		     : shminfo.shmaddr;
	}

	return frontBuffer();
}

void ShmBlitter::blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) {
	if (ximage && ximage->data) {
		XShmPutImage(QX11Info::display(), drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()),
		             ximage.get(), x, y, x, y, w, h, False);
	}
}

void ShmBlitter::flip() {
	if (ximage)
		ximage->data = backBuffer();
}

class PlainBlitter : public SubBlitter {
public:
	PlainBlitter(unsigned width, unsigned height, const VisualInfo &visInfo, bool db);
	virtual bool failed() const { return !data; }
	virtual void blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h);
	virtual void flip();
	virtual std::ptrdiff_t pitch() const { return ximage ? ximage->width : 0; }
	virtual void * pixels() const { return backBuffer(); }

private:
	x_ptr<XImage>::scoped const ximage;
	Array<char> const data;

	bool doubleBuffer() const { return ximage && data.size() >= imageDataSize(*ximage) * 2; }
	char * backBuffer() const;
	char * frontBuffer() const { return ximage ? ximage->data : 0;  }
};

PlainBlitter::PlainBlitter(const unsigned width, const unsigned height,
                           const VisualInfo &visInfo, const bool db)
: ximage(XCreateImage(QX11Info::display(), visInfo.visual, visInfo.depth,
                      ZPixmap, 0, 0, width, height, visInfo.depth <= 16 ? 16 : 32, 0))
, data(ximage ? imageDataSize(*ximage) << db : 0)
{
	if (ximage) {
		ximage->data = data;
	} else {
		std::cerr << "failed to create ximage\n";
	}
}

char * PlainBlitter::backBuffer() const {
	if (doubleBuffer()) {
		return data && frontBuffer() == data
		     ? data + imageDataSize(*ximage)
		     : data;
	}

	return frontBuffer();
}

void PlainBlitter::blit(Drawable drawable, unsigned x, unsigned y, unsigned w, unsigned h) {
	if (ximage && ximage->data) {
		XPutImage(QX11Info::display(), drawable,
		          DefaultGC(QX11Info::display(), QX11Info::appScreen()),
		          ximage.get(), x, y, x, y, w, h);
	}
}

void PlainBlitter::flip() {
	if (ximage)
		ximage->data = backBuffer();
}

static x_ptr<XVisualInfo>::transfer getVisualInfo(
		unsigned depth, int c_class,
		unsigned long rmask, unsigned long gmask, unsigned long bmask) {
	XVisualInfo vinfo_template;
	vinfo_template.screen = QX11Info::appScreen();
	vinfo_template.depth = depth;
	vinfo_template.c_class = c_class;
	vinfo_template.red_mask = rmask;
	vinfo_template.green_mask = gmask;
	vinfo_template.blue_mask = bmask;

	int nitems = 0;
	x_ptr<XVisualInfo>::transfer vinfos(
		XGetVisualInfo(QX11Info::display(),
		               (   VisualScreenMask
		                 | (depth ? VisualDepthMask : 0)
		                 | VisualClassMask
		                 | VisualRedMaskMask
		                 | VisualGreenMaskMask
		                 | VisualBlueMaskMask),
		               &vinfo_template,
		               &nitems));
	if (nitems > 0)
		return vinfos;

	return x_ptr<XVisualInfo>::transfer();
}

static bool isRgb24(XVisualInfo const &vinfo) {
	return vinfo.depth == 24
	    && vinfo.red_mask   == 0xFF0000
	    && vinfo.green_mask == 0x00FF00
	    && vinfo.blue_mask  == 0x0000FF;
}

static bool isRgb16(XVisualInfo const &vinfo) {
	return vinfo.depth <= 16 // ?
	    && vinfo.red_mask   == 0xF800
	    && vinfo.green_mask == 0x07E0
	    && vinfo.blue_mask  == 0x001F;
}

static x_ptr<XVisualInfo>::transfer getVisualInfo() {
	typedef x_ptr<XVisualInfo>::transfer vptr;

	{
		XVisualInfo vinfo_template;
		vinfo_template.visualid =
			XVisualIDFromVisual(XDefaultVisual(QX11Info::display(),
			                                   QX11Info::appScreen()));
		int nitems = 0;
		vptr vinfos(XGetVisualInfo(QX11Info::display(), VisualIDMask,
		                           &vinfo_template, &nitems));
		if (nitems > 0 && (isRgb24(vinfos.get()[0]) || isRgb16(vinfos.get()[0])))
			return vinfos;
	}

	if (vptr visual = getVisualInfo(24, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	if (vptr visual = getVisualInfo(16, TrueColor, 0xF800, 0x07E0, 0x001F))
		return visual;
	if (vptr visual = getVisualInfo(0, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	if (vptr visual = getVisualInfo(0, TrueColor, 0xF800, 0x07E0, 0x001F))
		return visual;
	if (vptr visual = getVisualInfo(0, DirectColor, 0xFF0000, 0x00FF00, 0x0000FF))
		return visual;
	if (vptr visual = getVisualInfo(0, DirectColor, 0xF800, 0x07E0, 0x001F))
		return visual;

	return vptr();
}

class X11Blitter : public BlitterWidget {
public:
	X11Blitter(VideoBufferLocker vbl, QWidget *parent)
	: BlitterWidget(vbl, QString("X11"), false, parent)
	, confWidget(new QWidget)
	, bf_(new QCheckBox(QString("Semi-bilinear filtering")),
	      "x11blitter/bf", false)
	, visInfo()
	{
		confWidget->setLayout(new QVBoxLayout);
		confWidget->layout()->setMargin(0);
		confWidget->layout()->addWidget(bf_.checkBox());

		setAttribute(Qt::WA_OpaquePaintEvent, true);
		setAttribute(Qt::WA_PaintOnScreen, true);

		if (x_ptr<XVisualInfo>::transfer v = getVisualInfo()) {
			visInfo.visual = v->visual;
			visInfo.depth = v->depth;
		}
	}

	virtual void init() { XSync(QX11Info::display(), False); }

	virtual void uninit() {
		subBlitter.reset();
		buffer.reset();
	}

	virtual bool isUnusable() const { return !visInfo.visual; }

	virtual long sync() {
		if (subBlitter->failed())
			return -1;

		subBlitter->blit(winId(), 0, 0, width(), height());
		XSync(QX11Info::display(), False);
		return 0;
	}

	virtual void blit();
	virtual QWidget * settingsWidget() const { return confWidget.get(); }
	virtual void acceptSettings() { bf_.accept(); }
	virtual void rejectSettings() const { bf_.reject(); }
	virtual QPaintEngine * paintEngine() const { return 0; }

protected:
	virtual void setBufferDimensions(unsigned width, unsigned height);

	virtual void paintEvent(QPaintEvent *event) {
		if (subBlitter) {
			const QRect &rect = event->rect();
			event->accept();
			subBlitter->blit(winId(), rect.x(), rect.y(), rect.width(), rect.height());
		}
	}

	virtual void resizeEvent(QResizeEvent *) {
		if (subBlitter) {
			lockPixelBuffer();
			setBufferDimensions(inBuffer().width, inBuffer().height);
			unlockPixelBuffer();
		}
	}

private:
	scoped_ptr<QWidget> const confWidget;
	scoped_ptr<SubBlitter> subBlitter;
	PersistCheckBox bf_;
	Array<char> buffer;
	VisualInfo visInfo;
};

void X11Blitter::setBufferDimensions(const unsigned w, const unsigned h) {
	uninit();

	const bool scale = width() != static_cast<int>(w) || height() != static_cast<int>(h);
	bool shm = XShmQueryExtension(QX11Info::display());

	if (shm) {
		subBlitter.reset(new ShmBlitter(width(), height(), visInfo, !scale));

		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset();
		}
	}

	if (!shm)
		subBlitter.reset(new PlainBlitter(width(), height(), visInfo, !scale));

	if (scale) {
		buffer.reset(w * h * (visInfo.depth <= 16 ? 2 : 4));
		setPixelBuffer(buffer, visInfo.depth <= 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32, w);
	} else {
		setPixelBuffer(subBlitter->pixels(),
		               visInfo.depth <= 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32,
		               subBlitter->pitch());
	}
}

void X11Blitter::blit() {
	if (buffer) {
		if (visInfo.depth <= 16) {
			if (bf_.value()) {
				semiLinearScale<quint16, 0xF81F, 0x07E0, 6>(
					reinterpret_cast<quint16*>(buffer.get()),
					static_cast<quint16*>(subBlitter->pixels()),
					inBuffer().width, inBuffer().height, width(), height(),
					subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint16*>(buffer.get()),
				                     static_cast<quint16*>(subBlitter->pixels()),
				                     inBuffer().width, inBuffer().height, width(), height(),
				                     subBlitter->pitch());
			}
		} else {
			if (bf_.value()) {
				semiLinearScale<quint32, 0xFF00FF, 0x00FF00, 8>(
					reinterpret_cast<quint32*>(buffer.get()),
					static_cast<quint32*>(subBlitter->pixels()),
					inBuffer().width, inBuffer().height, width(), height(),
					subBlitter->pitch());
			} else {
				nearestNeighborScale(reinterpret_cast<quint32*>(buffer.get()),
				                     static_cast<quint32*>(subBlitter->pixels()),
				                     inBuffer().width, inBuffer().height, width(), height(),
				                     subBlitter->pitch());
			}
		}
	} else {
		subBlitter->flip();
		setPixelBuffer(subBlitter->pixels(), inBuffer().pixelFormat, subBlitter->pitch());
	}
}

} // anon ns

transfer_ptr<BlitterWidget> createX11Blitter(VideoBufferLocker vbl, QWidget *parent) {
	return transfer_ptr<BlitterWidget>(new X11Blitter(vbl, parent));
}
