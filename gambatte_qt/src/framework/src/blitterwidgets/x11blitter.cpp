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

#include "x11blitter.h"
#include "../blitterwidget.h"
#include "../swscale.h"
#include "array.h"
#include "dialoghelpers.h"
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
#include <algorithm>
#include <cstdio>
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

class Image {
public:
	virtual ~Image() {}
	virtual void blit(Drawable , QRect const &rect) = 0;
	virtual std::ptrdiff_t pitch() const = 0;
	virtual void * pixels() const = 0;
	virtual QSize const size() const = 0;
};

static void * shmfailaddr() { return (void *) -1; }

static transfer_ptr<Image> createShmImage(QSize const &size, Visual *visual, unsigned depth) {
	class ShmImage : public Image {
	public:
		ShmImage(QSize const &size, Visual *visual, unsigned depth)
		: shminfo_()
		, ximage_(XShmCreateImage(QX11Info::display(), visual, depth, ZPixmap,
		                          0, &shminfo_, size.width(), size.height()))
		{
			if (!ximage_) {
				std::cerr << "XShmCreateImage failed\n";
				return;
			}

			ximage_->data = 0;
			shminfo_.readOnly = True;
			shminfo_.shmid = shmget(IPC_PRIVATE, imageDataSize(*ximage_), IPC_CREAT | 0777);
			if (shminfo_.shmid == -1) {
				std::perror("shmget failed");
				return;
			}

			shminfo_.shmaddr = static_cast<char *>(shmat(shminfo_.shmid, 0, 0));
			if (shminfo_.shmaddr == shmfailaddr()) {
				std::perror("shmat failed");
				return;
			}

			if (!XShmAttach(QX11Info::display(), &shminfo_)) {
				std::cerr << "XShmAttach failed\n";
				return;
			}

			ximage_->data = shminfo_.shmaddr;
		}

		virtual ~ShmImage() {
			if (ximage_ && shminfo_.shmid != -1) {
				if (ximage_->data)
					XShmDetach(QX11Info::display(), &shminfo_);
				if (shminfo_.shmaddr != shmfailaddr())
					shmdt(shminfo_.shmaddr);
				shmctl(shminfo_.shmid, IPC_RMID, 0);
			}
		}

		bool good() const { return ximage_ && ximage_->data; }

		virtual void blit(Drawable drawable, QRect const &rect) {
			XShmPutImage(QX11Info::display(), drawable,
			             DefaultGC(QX11Info::display(), QX11Info::appScreen()),
			             ximage_.get(), rect.x(), rect.y(), rect.x(), rect.y(),
			             rect.width(), rect.height(), False);
		}

		virtual std::ptrdiff_t pitch() const {
			int bytesPerPixel = std::max((ximage_->bits_per_pixel + 7) >> 3, 1);
			return ximage_->bytes_per_line / bytesPerPixel;
		}

		virtual void * pixels() const { return ximage_->data; }
		virtual QSize const size() const { return QSize(ximage_->width, ximage_->height); }

	private:
		XShmSegmentInfo shminfo_;
		x_ptr<XImage>::scoped const ximage_;
	};

	ShmImage *shmImage;
	transfer_ptr<Image> image(shmImage = new ShmImage(size, visual, depth));
	return shmImage->good()
	     ? image
	     : transfer_ptr<Image>();
}

static transfer_ptr<Image> createPlainImage(QSize const &size, Visual *visual, unsigned depth) {
	class PlainImage : public Image {
	public:
		PlainImage(QSize const &size, Visual *visual, unsigned depth)
		: ximage_(XCreateImage(QX11Info::display(), visual, depth, ZPixmap, 0, 0,
		                       size.width(), size.height(), depth <= 16 ? 16 : 32, 0))
		, data_(ximage_ ? imageDataSize(*ximage_) : 0)
		{
			if (!ximage_) {
				std::cerr << "XCreateImage failed\n";
				return;
			}

			ximage_->data = data_;
		}

		bool good() const { return data_; }

		virtual void blit(Drawable drawable, QRect const &rect) {
			XPutImage(QX11Info::display(), drawable,
			          DefaultGC(QX11Info::display(), QX11Info::appScreen()),
			          ximage_.get(), rect.x(), rect.y(), rect.x(), rect.y(),
			          rect.width(), rect.height());
		}

		virtual std::ptrdiff_t pitch() const { return ximage_->width; }
		virtual void * pixels() const { return ximage_->data; }
		virtual QSize const size() const { return QSize(ximage_->width, ximage_->height); }

	private:
		x_ptr<XImage>::scoped const ximage_;
		Array<char> const data_;
	};

	PlainImage *plainImage;
	transfer_ptr<Image> image(plainImage = new PlainImage(size, visual, depth));
	return plainImage->good()
	     ? image
	     : transfer_ptr<Image>();
}

struct VisualInfo {
	Visual *visual;
	unsigned depth;

	VisualInfo(XVisualInfo const *xvisualInfo)
	: visual(xvisualInfo ? xvisualInfo->visual : 0)
	, depth( xvisualInfo ? xvisualInfo->depth  : 0)
	{
	}
};

static transfer_ptr<Image> createImage(QSize const &size, VisualInfo const &vinfo, bool shm) {
	if (shm) {
		if (transfer_ptr<Image> image = createShmImage(size, vinfo.visual, vinfo.depth))
			return image;
	}

	return createPlainImage(size, vinfo.visual, vinfo.depth);
}

template<typename T, T rbmask, T gmask, unsigned rbdistance>
static void copyImage(Image const &dst, Image const &src, bool bilinearFilter) {
	QSize const dstSize = dst.size();
	QSize const srcSize = src.size();

	if (bilinearFilter) {
		semiLinearScale<T, rbmask, gmask, rbdistance>(
			static_cast<T *>(dst.pixels()), dst.pitch(),
			dstSize.width(), dstSize.height(),
			static_cast<T const *>(src.pixels()), src.pitch(),
			srcSize.width(), srcSize.height());
	} else {
		nearestNeighborScale(static_cast<T *>(dst.pixels()), dst.pitch(),
		                     dstSize.width(), dstSize.height(),
		                     static_cast<T const *>(src.pixels()), src.pitch(),
		                     srcSize.width(), srcSize.height());
	}
}

static void copyImage(Image const &dst, Image const &src, unsigned depth, bool bilinearFilter) {
	if (depth <= 16)
		copyImage<quint16, 0xF81F, 0x07E0, 6>(dst, src, bilinearFilter);
	else
		copyImage<quint32, 0xFF00FF, 0x00FF00, 8>(dst, src, bilinearFilter);
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
	return vinfo.depth <= 16
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
	, visualInfo_(getVisualInfo().get())
	, confWidget_(new QWidget)
	, bf_(new QCheckBox(tr("Semi-bilinear filtering"), confWidget_.get()),
	      "x11blitter/bf", false)
	{
		confWidget_->setLayout(new QVBoxLayout);
		confWidget_->layout()->setMargin(0);
		confWidget_->layout()->addWidget(bf_.checkBox());

		setAttribute(Qt::WA_OpaquePaintEvent, true);
		setAttribute(Qt::WA_PaintOnScreen, true);
	}

	virtual void init() { XSync(QX11Info::display(), False); }

	virtual void uninit() {
		image1_.reset();
		image0_.reset();
	}

	virtual bool isUnusable() const { return !visualInfo_.visual; }

	virtual int present() {
		if (!image0_ || !image1_)
			return -1;

		Image &frontImage = inBuffer().data == image0_->pixels() ? *image1_ : *image0_;
		frontImage.blit(winId(), rect());
		XSync(QX11Info::display(), False);
		return 0;
	}

	virtual QWidget * settingsWidget() const { return confWidget_.get(); }
	virtual void acceptSettings() { bf_.accept(); }
	virtual void rejectSettings() const { bf_.reject(); }
	virtual QPaintEngine * paintEngine() const { return 0; }

protected:
	virtual void consumeBuffer(SetBuffer setInputBuffer);

	virtual void setBufferDimensions(unsigned w, unsigned h, SetBuffer setInputBuffer) {
		uninit();

		bool shm = XShmQueryExtension(QX11Info::display());
		image0_ = createImage(QSize(w, h), visualInfo_, shm);
		image1_ = createImage(size(), visualInfo_, shm);
		setInputBuffer(image0_ ? image0_->pixels() : 0,
		               visualInfo_.depth <= 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32,
		               image0_ ? image0_->pitch() : 0);
	}

	virtual void paintEvent(QPaintEvent *event) {
		if (image0_ && image1_) {
			QRect const &rect = event->rect();
			Image &frontImage = inBuffer().data == image0_->pixels() ? *image1_ : *image0_;
			event->accept();
			frontImage.blit(winId(), rect);
		}
	}

	virtual void resizeEvent(QResizeEvent *) {
		if (!image0_ || !image1_)
			return;

		scoped_ptr<Image> &backImage  = inBuffer().data == image0_->pixels() ? image0_ : image1_;
		scoped_ptr<Image> &frontImage = inBuffer().data == image0_->pixels() ? image1_ : image0_;
		frontImage.reset();
		frontImage = createImage(size(), visualInfo_, XShmQueryExtension(QX11Info::display()));
		if (isPaused()) {
			bool const bilinearFilter = bf_.value() && size() != backImage->size();
			copyImage(*frontImage, *backImage, visualInfo_.depth, bilinearFilter);
		}
	}

private:
	VisualInfo const visualInfo_;
	scoped_ptr<QWidget> const confWidget_;
	scoped_ptr<Image> image0_;
	scoped_ptr<Image> image1_;
	PersistCheckBox bf_;
};

void X11Blitter::consumeBuffer(SetBuffer setInputBuffer) {
	if (!image0_ || !image1_)
		return;

	Image const &backImage  = inBuffer().data == image0_->pixels() ? *image0_ : *image1_;
	Image const &frontImage = inBuffer().data == image0_->pixels() ? *image1_ : *image0_;
	if (backImage.size() == frontImage.size()) {
		// flip
		setInputBuffer(frontImage.pixels(), inBuffer().pixelFormat, frontImage.pitch());
	} else
		copyImage(frontImage, backImage, visualInfo_.depth, bf_.value());
}

} // anon ns

transfer_ptr<BlitterWidget> createX11Blitter(VideoBufferLocker vbl, QWidget *parent) {
	return transfer_ptr<BlitterWidget>(new X11Blitter(vbl, parent));
}
