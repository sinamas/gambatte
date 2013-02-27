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
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPaintEvent>
#include <QSettings>
#include <QX11Info>
// Qt headers and xlib headers do not get along, so Qt headers must preceed xlib headers :(
#include "xvblitter.h" // should be on top
#include "array.h"
#include "uncopyable.h"
#include <sys/shm.h>
#include <cstring>
#include <iostream>

class XvBlitter::SubBlitter {
public:
	virtual ~SubBlitter() {};
	virtual bool failed() const = 0;
	virtual void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height) = 0;
	virtual void flip() = 0;
	virtual std::ptrdiff_t pitch() const = 0;
	virtual void * pixels() const = 0;
};

class XvBlitter::ShmBlitter : public SubBlitter {
public:
	ShmBlitter(XvPortID xvport, int formatid, unsigned int width, unsigned int height);
	virtual ~ShmBlitter();
	virtual bool failed() const { return !shminfo.shmaddr; }
	virtual void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	virtual void flip();
	virtual std::ptrdiff_t pitch() const { return xvimage ? xvimage->pitches[0] >> 2 : 0; }
	virtual void * pixels() const;

private:
	XShmSegmentInfo shminfo;
	XvImage *const xvimage;

	char * backbuf() const;
	char * frontbuf() const { return xvimage ? xvimage->data : 0; }
};

XvBlitter::ShmBlitter::ShmBlitter(const XvPortID xvport, const int formatid,
                                  const unsigned int width, const unsigned int height)
: shminfo()
, xvimage(XvShmCreateImage(QX11Info::display(), xvport, formatid, 0, width << (formatid != 3), height, &shminfo))
{
	if (xvimage) {
		shminfo.shmid = shmget(IPC_PRIVATE, xvimage->data_size * 2, IPC_CREAT | 0777);
		shminfo.shmaddr = xvimage->data = static_cast<char*>(shmat(shminfo.shmid, 0, 0));
		shminfo.readOnly = True;
		XShmAttach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
	} else {
		std::cerr << "XvShmCreateImage failed\n";
		shminfo.shmaddr = 0;
	}
}

XvBlitter::ShmBlitter::~ShmBlitter() {
	if (shminfo.shmaddr) {
		XShmDetach(QX11Info::display(), &shminfo);
		XSync(QX11Info::display(), 0);
		shmdt(shminfo.shmaddr);
		shmctl(shminfo.shmid, IPC_RMID, 0);
	}

	if (xvimage)
		XFree(xvimage);
}

char * XvBlitter::ShmBlitter::backbuf() const {
	return shminfo.shmaddr && frontbuf() == shminfo.shmaddr
	     ? shminfo.shmaddr + xvimage->data_size
	     : shminfo.shmaddr;
}

void XvBlitter::ShmBlitter::blit(const Drawable drawable,
		const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage && xvimage->data) {
		if (XvShmPutImage(QX11Info::display(), xvport, drawable,
		                  DefaultGC(QX11Info::display(), QX11Info::appScreen()), xvimage, 0, 0,
		                  xvimage->width, xvimage->height, 0, 0, width, height, False) != Success) {
			std::cerr << "XvShmPutImage failed\n";
		}
	}
}

void XvBlitter::ShmBlitter::flip() {
	if (xvimage)
		xvimage->data = backbuf();
}

void* XvBlitter::ShmBlitter::pixels() const {
	if (char *buf = backbuf())
		return buf + xvimage->offsets[0];

	return 0;
}

class XvBlitter::PlainBlitter : public SubBlitter {
public:
	PlainBlitter(XvPortID xvport, int formatid, unsigned int width, unsigned int height);
	virtual ~PlainBlitter();
	virtual bool failed() const { return !data; }
	virtual void blit(Drawable drawable, XvPortID xvport, unsigned width, unsigned height);
	virtual void flip();
	virtual std::ptrdiff_t pitch() const { return xvimage ? xvimage->pitches[0] >> 2 : 0; }
	virtual void * pixels() const;

private:
	XvImage *const xvimage;
	Array<char> const data;

	char * backbuf() const;
	char * frontbuf() const { return xvimage ? xvimage->data : 0; }
};

XvBlitter::PlainBlitter::PlainBlitter(const XvPortID xvport, const int formatid,
                                      const unsigned int width, const unsigned int height)
: xvimage(XvCreateImage(QX11Info::display(), xvport, formatid, 0, width << (formatid != 3), height))
, data(xvimage ? xvimage->data_size * 2 : 0)
{
	if (xvimage) {
		xvimage->data = data;
	} else
		std::cerr << "XvCreateImage failed\n";
}

XvBlitter::PlainBlitter::~PlainBlitter() {
	if (xvimage)
		XFree(xvimage);
}

char * XvBlitter::PlainBlitter::backbuf() const {
	return data && frontbuf() == data
	     ? data + xvimage->data_size
	     : data;
}

void XvBlitter::PlainBlitter::blit(const Drawable drawable,
		const XvPortID xvport, const unsigned width, const unsigned height) {
	if (xvimage && xvimage->data) {
		if (XvPutImage(QX11Info::display(), xvport, drawable,
		               DefaultGC(QX11Info::display(), QX11Info::appScreen()),
		               xvimage, 0, 0, xvimage->width, xvimage->height, 0, 0, width, height) != Success) {
			std::cerr << "XvPutImage failed\n";
		}
	}
}

void XvBlitter::PlainBlitter::flip() {
	if (xvimage)
		xvimage->data = backbuf();
}

void* XvBlitter::PlainBlitter::pixels() const {
	if (char *buf = backbuf())
		return buf + xvimage->offsets[0];

	return 0;
}

namespace {

class XvAdaptorInfos : Uncopyable {
public:
	XvAdaptorInfos() : num_(0), infos_(0) {
		if (XvQueryAdaptors(QX11Info::display(), QX11Info::appRootWindow(), &num_, &infos_) != Success) {
			std::cerr << "failed to query xv adaptors\n";
			num_ = 0;
		}
	}

	~XvAdaptorInfos() {
		if (infos_)
			XvFreeAdaptorInfo(infos_);
	}

	unsigned len() const { return num_; }
	operator const XvAdaptorInfo*() const { return infos_; }

private:
	unsigned num_;
	XvAdaptorInfo *infos_;
};

class XvPortImageFormats : Uncopyable {
public:
	explicit XvPortImageFormats(const XvPortID baseId)
	: num_(0), formats_(XvListImageFormats(QX11Info::display(), baseId, &num_))
	{
	}

	~XvPortImageFormats() {
		if (formats_)
			XFree(formats_);
	}

	int len() const { return num_; }
	operator const XvImageFormatValues*() const { return formats_; }

private:
	int num_;
	XvImageFormatValues *const formats_;
};

static int findId(const XvPortImageFormats &formats, const int id) {
	int i = 0;

	while (i < formats.len() && formats[i].id != id)
		++i;

	return i;
}

static void addPorts(QComboBox &portSelector) {
	XvAdaptorInfos adaptors;

	for (unsigned i = 0; i < adaptors.len(); ++i) {
		if (!(adaptors[i].type & XvImageMask))
			continue;

		const XvPortImageFormats formats(adaptors[i].base_id);

		int formatId = 0x3;
		int j = findId(formats, formatId);

		if (j == formats.len()) {
			formatId = 0x59565955;
			j = findId(formats, formatId);
		}

		if (j < formats.len()) {
			QList<QVariant> l;
			l.append(static_cast<uint>(adaptors[i].base_id));
			l.append(static_cast<uint>(std::min<unsigned long>(adaptors[i].num_ports, 0x100)));
			l.append(formats[j].id);
			portSelector.addItem(adaptors[i].name, l);
		}
	}
}

}

XvBlitter::ConfWidget::ConfWidget()
: widget_(new QWidget)
, portSelector_(new QComboBox(widget_.get()))
, portIndex_(0)
{
	addPorts(*portSelector_);

	if ((portIndex_ = QSettings().value("xvblitter/portIndex", 0).toUInt()) >= unsigned(portSelector_->count()))
		portIndex_ = 0;

	restore();

	widget_->setLayout(new QHBoxLayout);
	widget_->layout()->setMargin(0);
	widget_->layout()->addWidget(new QLabel(QString(tr("Xv Port:"))));
	widget_->layout()->addWidget(portSelector_);
}

XvBlitter::ConfWidget::~ConfWidget() {
	QSettings settings;
	settings.setValue("xvblitter/portIndex", portIndex_);
}

void XvBlitter::ConfWidget::store() {
	portIndex_ = portSelector_->currentIndex();
}

void XvBlitter::ConfWidget::restore() const {
	portSelector_->setCurrentIndex(portIndex_);
}

XvPortID XvBlitter::ConfWidget::basePortId() const {
	return static_cast<XvPortID>(portSelector_->itemData(portIndex_).toList().front().toUInt());
}

unsigned XvBlitter::ConfWidget::numPortIds() const {
	return portSelector_->itemData(portIndex_).toList().at(1).toUInt();
}

int XvBlitter::ConfWidget::formatId() const {
	return portSelector_->itemData(portIndex_).toList().back().toInt();
}

int XvBlitter::ConfWidget::numAdapters() const {
	return portSelector_->count();
}

XvBlitter::PortGrabber::PortGrabber()
: port_(0)
, grabbed_(false)
{
}

XvBlitter::PortGrabber::~PortGrabber() {
	ungrab();
}

bool XvBlitter::PortGrabber::grab(const XvPortID basePort, const unsigned numPorts) {
	ungrab();

	for (XvPortID port = basePort; port < basePort + numPorts; ++port) {
		port_ = port;

		if ((grabbed_ = XvGrabPort(QX11Info::display(), port, CurrentTime) == Success))
			return true;
	}

	return false;
}

void XvBlitter::PortGrabber::ungrab() {
	if (grabbed_) {
		XvUngrabPort(QX11Info::display(), port_, CurrentTime);
		grabbed_ = false;
	}
}

XvBlitter::XvBlitter(VideoBufferLocker vbl, QWidget *parent)
: BlitterWidget(vbl, QString("Xv"), 0, parent)
, initialized(false)
{
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_PaintOnScreen, true);

	XGCValues gcValues;
	gcValues.foreground = gcValues.background = 2110;
	gc = XCreateGC(QX11Info::display(), QX11Info::appRootWindow(), GCForeground | GCBackground, &gcValues);
}

bool XvBlitter::isUnusable() const {
	return confWidget.numAdapters() == 0;
}

void XvBlitter::init() {
	XSync(QX11Info::display(), 0);
	initPort();
	initialized = true;
}

void XvBlitter::uninit() {
	initialized = false;
	subBlitter.reset();
	portGrabber.ungrab();
}

XvBlitter::~XvBlitter() {
	XFreeGC(QX11Info::display(), gc);
}

long XvBlitter::sync() {
	if (!portGrabber.grabbed() || subBlitter->failed())
		return -1;

	subBlitter->blit(winId(), portGrabber.port(), width(), height());
	XSync(QX11Info::display(), 0);

	return 0;
}

void XvBlitter::paintEvent(QPaintEvent *event) {
	const QRect &rect = event->rect();
	XFillRectangle(QX11Info::display(), winId(), gc, rect.x(), rect.y(), rect.width(), rect.height());

	if (isPaused() && portGrabber.grabbed())
		subBlitter->blit(winId(), portGrabber.port(), width(), height());
}

void XvBlitter::setBufferDimensions(const unsigned width, const unsigned height) {
	const int formatid = confWidget.formatId();
	bool shm = XShmQueryExtension(QX11Info::display());
	subBlitter.reset();

	if (shm) {
		subBlitter.reset(new ShmBlitter(portGrabber.port(), formatid, width, height));

		if (subBlitter->failed()) {
			shm = false;
			subBlitter.reset();
		}
	}

	if (!shm)
		subBlitter.reset(new PlainBlitter(portGrabber.port(), formatid, width, height));

	setPixelBuffer(subBlitter->pixels(),
	               formatid == 3 ? PixelBuffer::RGB32 : PixelBuffer::UYVY,
	               subBlitter->pitch());
}

void XvBlitter::blit() {
	if (portGrabber.grabbed()) {
		subBlitter->flip();
		setPixelBuffer(subBlitter->pixels(), inBuffer().pixelFormat, subBlitter->pitch());
	}
}

namespace {

class XvAttributes : Uncopyable {
public:
	explicit XvAttributes(const XvPortID xvport)
	: xvport_(xvport)
	, numAttribs_(0)
	, attribs_(XvQueryPortAttributes(QX11Info::display(), xvport, &numAttribs_))
	{
	}

	~XvAttributes() {
		if (attribs_)
			XFree(attribs_);
	}

	void set(const char *const name, const int value) {
		const Atom atom = atomOf(name);
		if (atom != None)
			XvSetPortAttribute(QX11Info::display(), xvport_, atom, value);
	}

private:
	const XvPortID xvport_;
	int numAttribs_;
	XvAttribute *const attribs_;

	Atom atomOf(const char *const name) const {
		for (int i = 0; i < numAttribs_; ++i) {
			if (std::strcmp(attribs_[i].name, name) == 0)
				return XInternAtom(QX11Info::display(), name, True);
		}

		return None;
	}
};

}

void XvBlitter::initPort() {
	if (portGrabber.grab(confWidget.basePortId(), confWidget.numPortIds())) {
		XvAttributes attribs(portGrabber.port());
		attribs.set("XV_AUTOPAINT_COLORKEY", 0);
		attribs.set("XV_COLORKEY", 2110);
	}
}

void XvBlitter::acceptSettings() {
	confWidget.store();

	if (initialized
			&& (   portGrabber.port() <  confWidget.basePortId()
			    || portGrabber.port() >= confWidget.basePortId() + confWidget.numPortIds())) {
		initPort();
		lockPixelBuffer();
		setBufferDimensions(inBuffer().width, inBuffer().height);
		unlockPixelBuffer();
		repaint();
	}
}

void XvBlitter::rejectSettings() const {
	confWidget.restore();
}
