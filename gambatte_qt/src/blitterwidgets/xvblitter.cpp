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
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QPaintEvent>

#include "xvblitter.h"

#include <iostream>

#include <QX11Info>
#include <X11/Xlib.h>
// #include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvSubBlitter {
public:
	virtual void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height) = 0;
	virtual bool failed() = 0;
	virtual const PixelBuffer inBuffer() = 0;
	virtual ~XvSubBlitter() {};
};

class XvShmBlitter : public XvSubBlitter {
	XShmSegmentInfo shminfo;
	XvImage *xvimage;
	
public:
	XvShmBlitter(XvPortID xvport, unsigned int width, unsigned int height);
	void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	bool failed();
	const PixelBuffer inBuffer();
	~XvShmBlitter();
};

XvShmBlitter::XvShmBlitter(const XvPortID xvport, const unsigned int width, const unsigned int height) {
	std::cout << "creating xvimage...\n";
	xvimage = XvShmCreateImage(QX11Info::display(), xvport, 0x59565955, NULL, width * 2, height, &shminfo);
	
	if (xvimage == NULL) {
		std::cout << "failed to create xvimage\n";
	} else {
		shminfo.shmid = shmget(IPC_PRIVATE, xvimage->data_size, IPC_CREAT | 0777);
		shminfo.shmaddr = xvimage->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
		shminfo.readOnly = True;
		XShmAttach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
	}
}

XvShmBlitter::~XvShmBlitter() {
	if (shminfo.shmaddr != NULL) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}
	
	if (xvimage != NULL)
		XFree(xvimage);
}

void XvShmBlitter::blit(const Drawable drawable, const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage && shminfo.shmaddr) {
		if (XvShmPutImage(QX11Info::display(), xvport, drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0, xvimage->width, xvimage->height, 0, 0, width, height, False) != Success) {
			std::cout << "failed to put xvimage\n";
// 			failed = true;
		}
	}
}

bool XvShmBlitter::failed() {
	return xvimage == NULL || shminfo.shmaddr == NULL;
}

const PixelBuffer XvShmBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = PixelBuffer::UYVY;
	pixb.pixels = xvimage ? xvimage->data + xvimage->offsets[0] : NULL;
	pixb.pitch = xvimage ? xvimage->pitches[0] >> 2 : 0;
	
	return pixb;
}

class XvPlainBlitter : public XvSubBlitter {
	XvImage *xvimage;
	
public:
	XvPlainBlitter(XvPortID xvport, unsigned int width, unsigned int height);
	void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	bool failed();
	const PixelBuffer inBuffer();
	~XvPlainBlitter();
};

XvPlainBlitter::XvPlainBlitter(const XvPortID xvport, const unsigned int width, const unsigned int height) {
	std::cout << "creating xvimage...\n";
	xvimage = XvCreateImage(QX11Info::display(), xvport, 0x59565955, NULL, width * 2, height);
	
	if (xvimage == NULL) {
		std::cout << "failed to create xvimage\n";
	} else {
		xvimage->data = new char[xvimage->data_size];
	}
}

XvPlainBlitter::~XvPlainBlitter() {
	if (xvimage) {
		if (xvimage->data)
			delete[] xvimage->data;
		
		XFree(xvimage);
	}
}

void XvPlainBlitter::blit(const Drawable drawable, const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage) {
		if (XvPutImage(QX11Info::display(), xvport, drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0, xvimage->width, xvimage->height, 0, 0, width, height) != Success) {
			std::cout << "failed to put xvimage\n";
// 			failed = true;
		}
	}
}

bool XvPlainBlitter::failed() {
	return xvimage == NULL;
}

const PixelBuffer XvPlainBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = PixelBuffer::UYVY;
	pixb.pixels = xvimage ? xvimage->data + xvimage->offsets[0] : NULL;
	pixb.pitch = xvimage ? xvimage->pitches[0] >> 2 : 0;
	
	return pixb;
}

static void addPorts(QComboBox *portSelector) {
	unsigned int num_adaptors;
	XvAdaptorInfo *adaptor_info;
	
	if (XvQueryAdaptors(QX11Info::display(), QX11Info::appRootWindow(), &num_adaptors, &adaptor_info) == Success) {
		for (unsigned int i = 0; i < num_adaptors; ++i) {
			if (!(adaptor_info[i].type & XvImageMask))
				continue;
			
			int numImages;
			XvImageFormatValues *const formats = XvListImageFormats(QX11Info::display(), adaptor_info[i].base_id, &numImages);
			
			for (int j = 0; j < numImages; ++j) {
				if (formats[j].id == 0x59565955) {
					portSelector->addItem(adaptor_info[i].name, static_cast<qulonglong>(adaptor_info[i].base_id));
					break;
				}
			}
			
			XFree(formats);
		}
		
		XvFreeAdaptorInfo(adaptor_info);
	} else {
		std::cout << "failed to query xv adaptors\n";
	}
}

XvBlitter::XvBlitter(QWidget *parent) :
		BlitterWidget(QString("Xv"), parent),
		subBlitter(NULL),
// 		xvbuffer(NULL),
// 		yuv_table(NULL),
// 		xvimage(NULL),
		inWidth(160),
		inHeight(144),
		old_w(0),
		old_h(0),
		keepRatio(true),
		integerScaling(false),
		portGrabbed(false),
		failed(true),
		initialized(false)
{

	/*QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(2110));
	setPalette(pal);
	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);*/
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_PaintOnScreen, true);
	
	{
		XGCValues gcValues;
		gcValues.foreground = gcValues.background = 2110;
		gc = XCreateGC(QX11Info::display(), QX11Info::appRootWindow(), GCForeground | GCBackground, &gcValues);
	}
	
	confWidget = new QWidget();
	QHBoxLayout *layout = new QHBoxLayout;
	layout->setMargin(0);
	layout->addWidget(new QLabel(QString(tr("Xv Port:"))));
	portSelector = new QComboBox;
	addPorts(portSelector);
	layout->addWidget(portSelector);
	confWidget->setLayout(layout);
	
	QSettings settings;
	settings.beginGroup("xvblitter");
	
	if ((portIndex = settings.value("portIndex", 0).toUInt()) >= static_cast<unsigned>(portSelector->count()))
		portIndex = 0;
	
	settings.endGroup();
	rejectSettings();
}

bool XvBlitter::isUnusable() {
	return portSelector->count() == 0;
}

/*static bool getBestPort(const unsigned int num_adaptors, const XvAdaptorInfo *const adaptor_info, XvPortID *const port_out) {
	for (unsigned int i = 0; i < num_adaptors; ++i) {
		if (!(adaptor_info[i].type & XvImageMask))
			continue;
		
		int numImages;
		XvImageFormatValues *const formats = XvListImageFormats(QX11Info::display(), adaptor_info[i].base_id, &numImages);
		
		for (int j = 0; j < numImages; ++j) {
			if (formats[j].id == 0x59565955 &&
			    XvGrabPort(QX11Info::display(), adaptor_info[i].base_id, CurrentTime) == Success) {
				*port_out = adaptor_info[i].base_id;
				XFree(formats);
				
				return false;
			}
		}
		
		XFree(formats);
	}
	
	return true;
}*/

void XvBlitter::init() {
	/*if (yuv_table == NULL) {
		yuv_table = new u_int32_t[0x10000];
		u_int32_t r, g, b, y, u, v;
		for (u_int32_t i = 0;i < 0x10000;++i) {
			r = (i & 0xF800) >> 8;
			g = (i & 0x07E0) >> 3;
			b = (i & 0x001F) << 3;

			y = (r * 299 * 219 + g * 587 * 219 + b * 114 * 219 + (255 * 1000) / 2) / (255 * 1000) + 16;
			u = (b * 886 * 112 -  r * 299 * 112 - g * 587 * 112 + (886 * 255) / 2 + 128 * 886 * 255) / (886 * 255);
			v = (r * 701 * 112 - b * 114 * 112 - g * 587 * 112 + (701 * 255) / 2 + 128 * 701 * 255) / (701 * 255);

			yuv_table[i] = (y << 24) | (v << 16) | (y << 8) | u;
		}
	}*/
	
	XSync(QX11Info::display(), 0);
	
	shm = XShmQueryExtension(QX11Info::display());
	std::cout << "shm: " << shm << std::endl;
	
	initPort();
	
	initialized = true;
}

void XvBlitter::uninit() {
// 	delete []xvbuffer;
// 	xvbuffer = NULL;
// 	delete []yuv_table;
// 	yuv_table = NULL;

	if (subBlitter) {
		delete subBlitter;
		subBlitter = NULL;
	}
	
	if (portGrabbed) {
		XvUngrabPort(QX11Info::display(), xvport, CurrentTime);
		portGrabbed = false;
	}
	
	failed = true;
	initialized = false;
}

XvBlitter::~XvBlitter() {
	uninit();
	XFreeGC(QX11Info::display(), gc);
	delete confWidget;
	
	QSettings settings;
	settings.beginGroup("xvblitter");
	settings.setValue("portIndex", portIndex);
	settings.endGroup();
}

int XvBlitter::sync(const bool turbo) {
	if (failed || subBlitter->failed())
		return -1;
	
	return BlitterWidget::sync(turbo);
}

void XvBlitter::keepAspectRatio(const bool enable) {
	keepRatio = enable;
}

bool XvBlitter::keepsAspectRatio() {
	return keepRatio;
}

void XvBlitter::scaleByInteger(const bool enable) {
	integerScaling = enable;
}

bool XvBlitter::scalesByInteger() {
	return integerScaling;
}

void XvBlitter::paintEvent(QPaintEvent *event) {
	event->accept();
	const QRect &rect = event->rect();
	XFillRectangle(QX11Info::display(), winId(), gc, rect.x(), rect.y(), rect.width(), rect.height());
	
	if (!failed)
		subBlitter->blit(winId(), xvport, width(), height());
}

void XvBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	inWidth = width;
	inHeight = height;
	
	if (subBlitter) {
		delete subBlitter;
		subBlitter = NULL;
	}
	
	if (shm) {
		subBlitter = new XvShmBlitter(xvport, width, height);
		
		if (subBlitter->failed()) {
			shm = false;
			delete subBlitter;
		}
	}
	
	if (!shm)
		subBlitter = new XvPlainBlitter(xvport, width, height);

// 	delete []xvbuffer;
// 	xvbuffer = new u_int16_t[width*height];
	
	old_w = old_h = 0;
}

const PixelBuffer XvBlitter::inBuffer() {
	/*PixelBuffer pixb;
	pixb.format = PixelBuffer::RGB16;
	pixb.pixels = xvbuffer;
	pixb.pitch = inWidth;
	return pixb;*/
	
	return subBlitter->inBuffer();
}

void XvBlitter::blit() {
	if (!failed) {
		/*{
			u_int32_t *yuv_pixel = (u_int32_t*)(subBlitter->inBuffer().pixels);
			const u_int16_t *s = xvbuffer;
			unsigned n = inWidth * inHeight;
			
			while (n--) {
				*yuv_pixel++ = yuv_table[*s++];
			}
		}*/
		
		subBlitter->blit(winId(), xvport, width(), height());
		XSync(QX11Info::display(), 0);
	}
}

/*void XvBlitter::resizeEvent(QResizeEvent *event) {
// 	std::cout << "resizeEvent\n";
	if (keepRatio) {
// 		static int old_w=0;
// 		static int old_h=0;
		if (width() != old_w || height() != old_h) {
			if (integerScaling) {
				unsigned int scale = std::min((width() / inWidth), (height() / inHeight));
				old_w = inWidth * scale;
				old_h = inHeight * scale;
				setGeometry(x() + (width() - old_w) / 2, y() + (height() - old_h) / 2, old_w, old_h);
			} else {
				if ((static_cast<double>(width()) / static_cast<double>(height())) > (10.0 / 9.0)) {
					old_w = height() * 10.0 / 9.0 + 0.5;
					old_h = height();
					if (old_w < width())
						setGeometry(x() + (width() - old_w) / 2, y(), old_w, old_h);
				} else {
					old_w = width();
					old_h = old_w * 0.9 + 0.5;
					if (old_h < height())
						setGeometry(x(), y() + (height() - old_h) / 2, old_w, old_h);
				}
			}
		}
	}
}*/

void XvBlitter::initPort() {
	xvport = portSelector->itemData(portIndex).toULongLong();
	failed = !(portGrabbed = (XvGrabPort(QX11Info::display(), xvport, CurrentTime) == Success));
	
	if (!failed) {		
		Atom atom = XInternAtom(QX11Info::display(), "XV_AUTOPAINT_COLORKEY", True);
		
		if (atom != None)
			XvSetPortAttribute(QX11Info::display(), xvport, atom, 0);
		
		atom = XInternAtom(QX11Info::display(), "XV_COLORKEY", True);
		
		if (atom != None)
			XvSetPortAttribute(QX11Info::display(), xvport, atom, 2110);
	}
}

void XvBlitter::acceptSettings() {
	portIndex = portSelector->currentIndex();
	
	if (initialized) {
		if (portGrabbed) {
			XvUngrabPort(QX11Info::display(), xvport, CurrentTime);
			portGrabbed = false;
		}
		
		initPort();
		setBufferDimensions(inWidth, inHeight);
	}
}

void XvBlitter::rejectSettings() {
	portSelector->setCurrentIndex(portIndex);
}
