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
#include <QList>

#include "xvblitter.h"

#include <iostream>

#include <QX11Info>
#include <X11/Xlib.h>
// #include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvBlitter::SubBlitter {
public:
	virtual void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height) = 0;
	virtual bool failed() const = 0;
	virtual void* pixels() const = 0;
	virtual unsigned pitch() const = 0;
	virtual ~SubBlitter() {};
};

class XvBlitter::ShmBlitter : public SubBlitter {
	XShmSegmentInfo shminfo;
	XvImage *xvimage;
	
public:
	ShmBlitter(XvPortID xvport, int formatid, unsigned int width, unsigned int height);
	void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~ShmBlitter();
};

XvBlitter::ShmBlitter::ShmBlitter(const XvPortID xvport, const int formatid, const unsigned int width, const unsigned int height) {
	std::cout << "creating shm xvimage...\n";
	xvimage = XvShmCreateImage(QX11Info::display(), xvport, formatid, NULL, width << (formatid != 3), height, &shminfo);
	
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

XvBlitter::ShmBlitter::~ShmBlitter() {
	if (shminfo.shmaddr != NULL) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}
	
	if (xvimage != NULL)
		XFree(xvimage);
}

void XvBlitter::ShmBlitter::blit(const Drawable drawable, const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage && shminfo.shmaddr) {
		if (XvShmPutImage(QX11Info::display(), xvport, drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0, xvimage->width, xvimage->height, 0, 0, width, height, False) != Success) {
			std::cout << "failed to put xvimage\n";
// 			failed = true;
		}
	}
}

bool XvBlitter::ShmBlitter::failed() const {
	return xvimage == NULL || shminfo.shmaddr == NULL;
}

void* XvBlitter::ShmBlitter::pixels() const {
	return xvimage ? xvimage->data + xvimage->offsets[0] : NULL;
}

unsigned XvBlitter::ShmBlitter::pitch() const {
	return xvimage ? xvimage->pitches[0] >> 2 : 0;
}

class XvBlitter::PlainBlitter : public SubBlitter {
	XvImage *xvimage;
	
public:
	PlainBlitter(XvPortID xvport, int formatid, unsigned int width, unsigned int height);
	void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	bool failed() const;
	void* pixels() const;
	unsigned pitch() const;
	~PlainBlitter();
};

XvBlitter::PlainBlitter::PlainBlitter(const XvPortID xvport, const int formatid, const unsigned int width, const unsigned int height) {
	std::cout << "creating xvimage...\n";
	xvimage = XvCreateImage(QX11Info::display(), xvport, formatid, NULL, width << (formatid != 3), height);
	
	if (xvimage == NULL) {
		std::cout << "failed to create xvimage\n";
	} else {
		xvimage->data = new char[xvimage->data_size];
	}
}

XvBlitter::PlainBlitter::~PlainBlitter() {
	if (xvimage) {
		if (xvimage->data)
			delete[] xvimage->data;
		
		XFree(xvimage);
	}
}

void XvBlitter::PlainBlitter::blit(const Drawable drawable, const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage) {
		if (XvPutImage(QX11Info::display(), xvport, drawable, DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0, xvimage->width, xvimage->height, 0, 0, width, height) != Success) {
			std::cout << "failed to put xvimage\n";
// 			failed = true;
		}
	}
}

bool XvBlitter::PlainBlitter::failed() const {
	return xvimage == NULL;
}

void* XvBlitter::PlainBlitter::pixels() const {
	return xvimage ? xvimage->data + xvimage->offsets[0] : NULL;
}

unsigned XvBlitter::PlainBlitter::pitch() const {
	return xvimage ? xvimage->pitches[0] >> 2 : 0;
}

static int search(const XvImageFormatValues *const formats, const int numFormats, const int id) {
	int i = 0;
	
	while (i < numFormats && formats[i].id != id)
		++i;
	
	return i;
}

static void addPorts(QComboBox *portSelector) {
	unsigned int num_adaptors;
	XvAdaptorInfo *adaptor_info;
	
	if (XvQueryAdaptors(QX11Info::display(), QX11Info::appRootWindow(), &num_adaptors, &adaptor_info) == Success) {
		for (unsigned int i = 0; i < num_adaptors; ++i) {
			if (!(adaptor_info[i].type & XvImageMask))
				continue;
			
			int numFormats;
			XvImageFormatValues *const formats = XvListImageFormats(QX11Info::display(), adaptor_info[i].base_id, &numFormats);
			
			int formatId = 0x3;
			int j = search(formats, numFormats, formatId);
			
			if (j == numFormats) {
				formatId = 0x59565955;
				j = search(formats, numFormats, formatId);
			}
			
			if (j < numFormats) {
				QList<QVariant> l;
				l.append(static_cast<uint>(adaptor_info[i].base_id));
				l.append(formats[j].id);
				portSelector->addItem(adaptor_info[i].name, l);
			}
			
			XFree(formats);
		}
		
		XvFreeAdaptorInfo(adaptor_info);
	} else {
		std::cout << "failed to query xv adaptors\n";
	}
}

XvBlitter::XvBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
		BlitterWidget(setPixelBuffer, QString("Xv"), parent),
// 		xvbuffer(NULL),
// 		yuv_table(NULL),
// 		xvimage(NULL),
		confWidget(new QWidget()),
		portSelector(new QComboBox),
		inWidth(160),
		inHeight(144),
		old_w(0),
		old_h(0),
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
	
	QHBoxLayout *layout = new QHBoxLayout;
	layout->setMargin(0);
	layout->addWidget(new QLabel(QString(tr("Xv Port:"))));
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

bool XvBlitter::isUnusable() const {
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

	subBlitter.reset();
	
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

void XvBlitter::paintEvent(QPaintEvent *event) {
	event->accept();
	const QRect &rect = event->rect();
	XFillRectangle(QX11Info::display(), winId(), gc, rect.x(), rect.y(), rect.width(), rect.height());
	
	if (!failed) {
		subBlitter->blit(winId(), xvport, width(), height());
	}
}

void XvBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	inWidth = width;
	inHeight = height;
	
	const int formatid = portSelector->itemData(portIndex).toList().back().toInt();
	
	subBlitter.reset(); // predestruct previous object to ensure resource availability.
	
	if (shm) {
		subBlitter.reset(new ShmBlitter(xvport, formatid, width, height));
		
		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset();
		}
	}
	
	if (!shm)
		subBlitter.reset(new PlainBlitter(xvport, formatid, width, height));

// 	delete []xvbuffer;
// 	xvbuffer = new u_int16_t[width*height];
	
	setPixelBuffer(subBlitter->pixels(), formatid == 3 ? MediaSource::RGB32 : MediaSource::UYVY, subBlitter->pitch());
	
	old_w = old_h = 0;
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

static void setAttrib(XvAttribute *const attribs, const int nattr, const char *const name, const int value, const XvPortID xvport) {
	Atom atom;
	
	for (int i = 0; i < nattr; ++i) {
		if (!strcmp(attribs[i].name, name)) {
			if ((atom = XInternAtom(QX11Info::display(), name, True)) != None)
				XvSetPortAttribute(QX11Info::display(), xvport, atom, value);
			
			break;
		}
	}
}

void XvBlitter::initPort() {
	xvport = static_cast<XvPortID>(portSelector->itemData(portIndex).toList().front().toUInt());
	failed = !(portGrabbed = (XvGrabPort(QX11Info::display(), xvport, CurrentTime) == Success));
	
	if (!failed) {
		int nattr = 0;
		XvAttribute *const attribs = XvQueryPortAttributes(QX11Info::display(), xvport, &nattr);
		setAttrib(attribs, nattr, "XV_AUTOPAINT_COLORKEY", 0, xvport);
		setAttrib(attribs, nattr, "XV_COLORKEY", 2110, xvport);
		XFree(attribs);
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
		repaint();
// 		setBufferDimensions(inWidth, inHeight);
	}
}

void XvBlitter::rejectSettings() {
	portSelector->setCurrentIndex(portIndex);
}
