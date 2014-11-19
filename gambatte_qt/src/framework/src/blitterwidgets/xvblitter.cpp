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

#include "xvblitter.h"
#include "../blitterwidget.h"
#include "array.h"
#include "scoped_ptr.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPaintEvent>
#include <QSettings>
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>
#include <sys/shm.h>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace {

enum { color_key = 2110 };
enum { formatid_rgb32 = 3, formatid_uyvy = 0x59565955 };

struct XDeleter {
	template<class T> static void del(T *p) { if (p) { XFree(p); } }
	static void del(XvAdaptorInfo *p) { if (p) { XvFreeAdaptorInfo(p); } }
};

template<class T>
struct x_ptr { typedef scoped_ptr<T, XDeleter> scoped; };

static void * shmfailaddr() { return (void *) -1; }

class SubBlitter {
public:
	virtual ~SubBlitter() {};
	virtual bool failed() const = 0;
	virtual void blit(Drawable drawable, XvPortID port, QSize const &size) = 0;
	virtual void flip() = 0;
	virtual std::ptrdiff_t pitch() const = 0;
	virtual void * pixels() const = 0;
};

class ShmBlitter : public SubBlitter {
public:
	ShmBlitter(XvPortID const port, int const formatid, QSize const &size)
	: shminfo_()
	, xvimage_(XvShmCreateImage(QX11Info::display(), port, formatid, 0,
	                            size.width() << (formatid != formatid_rgb32), size.height(),
	                            &shminfo_))
	{
		shminfo_.shmaddr = 0;
		if (!xvimage_) {
			std::cerr << "XvShmCreateImage failed\n";
			return;
		}

		xvimage_->data = 0;
		shminfo_.readOnly = True;
		shminfo_.shmid = shmget(IPC_PRIVATE, xvimage_->data_size * 2, IPC_CREAT | 0777);
		if (shminfo_.shmid == -1) {
			std::perror("shmget failed");
			return;
		}

		shminfo_.shmaddr = static_cast<char *>(shmat(shminfo_.shmid, 0, 0));
		if (shminfo_.shmaddr == shmfailaddr()) {
			std::perror("shmat failed");
			shminfo_.shmaddr = 0;
			return;
		}

		if (!XShmAttach(QX11Info::display(), &shminfo_)) {
			std::cerr << "XShmAttach failed\n";
			return;
		}

		xvimage_->data = shminfo_.shmaddr;
	}

	virtual ~ShmBlitter() {
		if (xvimage_ && shminfo_.shmid != -1) {
			if (xvimage_->data)
				XShmDetach(QX11Info::display(), &shminfo_);
			if (shminfo_.shmaddr)
				shmdt(shminfo_.shmaddr);
			shmctl(shminfo_.shmid, IPC_RMID, 0);
		}
	}

	virtual bool failed() const { return !xvimage_ || !xvimage_->data; }

	virtual void blit(Drawable drawable, XvPortID port, QSize const &size) {
		if (!xvimage_ || !xvimage_->data)
			return;

		if (XvShmPutImage(QX11Info::display(), port, drawable,
		                  DefaultGC(QX11Info::display(), QX11Info::appScreen()),
		                  xvimage_.get(), 0, 0, xvimage_->width, xvimage_->height,
		                  0, 0, size.width(), size.height(), False) != Success) {
			std::cerr << "XvShmPutImage failed\n";
		}
	}

	virtual void flip() {
		if (xvimage_)
			xvimage_->data = backbuf();
	}

	virtual std::ptrdiff_t pitch() const { return xvimage_ ? xvimage_->pitches[0] >> 2 : 0; }

	virtual void * pixels() const {
		if (char *buf = backbuf())
			return buf + xvimage_->offsets[0];

		return 0;
	}

private:
	XShmSegmentInfo shminfo_;
	x_ptr<XvImage>::scoped const xvimage_;

	char * backbuf() const;
	char * frontbuf() const { return xvimage_ ? xvimage_->data : 0; }
};

char * ShmBlitter::backbuf() const {
	return shminfo_.shmaddr && shminfo_.shmaddr == frontbuf()
	     ? shminfo_.shmaddr + xvimage_->data_size
	     : shminfo_.shmaddr;
}

class PlainBlitter : public SubBlitter {
public:
	PlainBlitter(XvPortID port, int formatid, QSize const &size)
	: xvimage_(XvCreateImage(QX11Info::display(), port, formatid, 0,
	                         size.width() << (formatid != formatid_rgb32), size.height()))
	, data_(xvimage_ ? xvimage_->data_size * 2 : 0)
	{
		if (xvimage_) {
			xvimage_->data = data_;
		} else
			std::cerr << "XvCreateImage failed\n";
	}

	virtual bool failed() const { return !data_; }

	virtual void blit(Drawable drawable, XvPortID port, QSize const &size) {
		if (!xvimage_ || !xvimage_->data)
			return;

		if (XvPutImage(QX11Info::display(), port, drawable,
		               DefaultGC(QX11Info::display(), QX11Info::appScreen()),
		               xvimage_.get(), 0, 0, xvimage_->width, xvimage_->height,
		               0, 0, size.width(), size.height()) != Success) {
			std::cerr << "XvPutImage failed\n";
		}
	}

	virtual void flip() {
		if (xvimage_)
			xvimage_->data = backbuf();
	}

	virtual std::ptrdiff_t pitch() const { return xvimage_ ? xvimage_->pitches[0] >> 2 : 0; }

	virtual void * pixels() const {
		if (char *buf = backbuf())
			return buf + xvimage_->offsets[0];

		return 0;
	}

private:
	x_ptr<XvImage>::scoped const xvimage_;
	Array<char> const data_;

	char * backbuf() const;
	char * frontbuf() const { return xvimage_ ? xvimage_->data : 0; }
};

char * PlainBlitter::backbuf() const {
	return data_ && frontbuf() == data_
	     ? data_ + xvimage_->data_size
	     : data_;
}

static transfer_ptr<SubBlitter> createSubBlitter(XvPortID port, int formatid, QSize const &size) {
	if (XShmQueryExtension(QX11Info::display())) {
		transfer_ptr<SubBlitter> blitter(new ShmBlitter(port, formatid, size));
		if (!blitter->failed())
			return blitter;
	}

	return transfer_ptr<SubBlitter>(new PlainBlitter(port, formatid, size));
}

class XvAdaptorInfos {
public:
	XvAdaptorInfos()
	: num_(0)
	{
		XvAdaptorInfo *infos = 0;
		if (XvQueryAdaptors(QX11Info::display(),
		                    QX11Info::appRootWindow(), &num_, &infos) != Success) {
			std::cerr << "failed to query xv adaptors\n";
			num_ = 0;
		}

		infos_.reset(infos);
	}

	unsigned len() const { return num_; }
	operator XvAdaptorInfo const *() const { return infos_.get(); }

private:
	unsigned num_;
	x_ptr<XvAdaptorInfo>::scoped infos_;
};

class XvPortImageFormats {
public:
	explicit XvPortImageFormats(XvPortID baseId)
	: num_(0), formats_(XvListImageFormats(QX11Info::display(), baseId, &num_))
	{
	}

	int len() const { return num_; }
	operator XvImageFormatValues const *() const { return formats_.get(); }

private:
	int num_;
	x_ptr<XvImageFormatValues>::scoped const formats_;
};

static int findId(XvPortImageFormats const &formats, int const id) {
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

		XvPortImageFormats const formats(adaptors[i].base_id);
		int formatId = formatid_rgb32;
		int j = findId(formats, formatId);
		if (j == formats.len()) {
			formatId = formatid_uyvy;
			j = findId(formats, formatId);
		}

		if (j < formats.len()) {
			QList<QVariant> l;
			l.append(static_cast<uint>(adaptors[i].base_id));
			l.append(static_cast<uint>(std::min<std::size_t>(adaptors[i].num_ports, 0x100)));
			l.append(formats[j].id);
			portSelector.addItem(adaptors[i].name, l);
		}
	}
}

class ConfWidget {
public:
	ConfWidget()
	: widget_(new QWidget)
	, portSelector_(new QComboBox(widget_.get()))
	, portIndex_(0)
	{
		addPorts(*portSelector_);

		portIndex_ = QSettings().value("xvblitter/portIndex", 0).toUInt();
		if (portIndex_ >= unsigned(portSelector_->count()))
			portIndex_ = 0;

		restore();

		widget_->setLayout(new QHBoxLayout);
		widget_->layout()->setMargin(0);
		widget_->layout()->addWidget(new QLabel(QObject::tr("Xv Port:")));
		widget_->layout()->addWidget(portSelector_);
	}

	~ConfWidget() {
		QSettings settings;
		settings.setValue("xvblitter/portIndex", portIndex_);
	}

	void store() { portIndex_ = portSelector_->currentIndex(); }
	void restore() const { portSelector_->setCurrentIndex(portIndex_); }
	XvPortID basePortId() const;
	unsigned numPortIds() const;
	int formatId() const;
	int numAdapters() const { return portSelector_->count(); }
	QWidget * qwidget() const { return widget_.get(); }

private:
	scoped_ptr<QWidget> const widget_;
	QComboBox *const portSelector_;
	unsigned portIndex_;
};

XvPortID ConfWidget::basePortId() const {
	return static_cast<XvPortID>(portSelector_->itemData(portIndex_).toList().front().toUInt());
}

unsigned ConfWidget::numPortIds() const {
	return portSelector_->itemData(portIndex_).toList().at(1).toUInt();
}

int ConfWidget::formatId() const {
	return portSelector_->itemData(portIndex_).toList().back().toInt();
}

class PortGrabber : Uncopyable {
public:
	PortGrabber()
	: port_(0)
	, grabbed_(false)
	{
	}

	~PortGrabber() { ungrab(); }
	bool grab(XvPortID port, unsigned numPorts);
	void ungrab();
	bool grabbed() const { return grabbed_; }
	XvPortID port() const { return port_; }

private:
	XvPortID port_;
	bool grabbed_;
};

bool PortGrabber::grab(XvPortID const basePort, unsigned const numPorts) {
	ungrab();

	for (XvPortID port = basePort; port < basePort + numPorts; ++port) {
		port_ = port;
		if ((grabbed_ = XvGrabPort(QX11Info::display(), port, CurrentTime) == Success))
			return true;
	}

	return false;
}

void PortGrabber::ungrab() {
	if (grabbed_) {
		XvUngrabPort(QX11Info::display(), port_, CurrentTime);
		grabbed_ = false;
	}
}

class XvAttributes {
public:
	explicit XvAttributes(XvPortID const xvport)
	: xvport_(xvport)
	, numAttribs_(0)
	, attribs_(XvQueryPortAttributes(QX11Info::display(), xvport, &numAttribs_))
	{
	}

	void set(char const *const name, int const value) {
		Atom const atom = atomOf(name);
		if (atom != None)
			XvSetPortAttribute(QX11Info::display(), xvport_, atom, value);
	}

private:
	XvPortID const xvport_;
	int numAttribs_;
	x_ptr<XvAttribute>::scoped const attribs_;

	Atom atomOf(char const *const name) const {
		for (int i = 0; i < numAttribs_; ++i) {
			if (std::strcmp(attribs_.get()[i].name, name) == 0)
				return XInternAtom(QX11Info::display(), name, True);
		}

		return None;
	}
};

static GC createGC() {
	XGCValues gcValues;
	gcValues.foreground = gcValues.background = color_key;
	return XCreateGC(QX11Info::display(), QX11Info::appRootWindow(),
	                 GCForeground | GCBackground, &gcValues);
}

class XvBlitter : public BlitterWidget {
public:
	XvBlitter(VideoBufferLocker vbl, QWidget *parent)
	: BlitterWidget(vbl, QString("Xv"), 0, parent)
	, gc_(createGC())
	{
		setAttribute(Qt::WA_NoSystemBackground, true);
		setAttribute(Qt::WA_PaintOnScreen, true);
	}

	virtual ~XvBlitter() { XFreeGC(QX11Info::display(), gc_); }

	virtual void init() {
		XSync(QX11Info::display(), False);
		initPort();
	}

	virtual void uninit() {
		subBlitter_.reset();
		portGrabber_.ungrab();
	}

	virtual bool isUnusable() const { return confWidget_.numAdapters() == 0; }

	virtual int present() {
		if (!portGrabber_.grabbed() || subBlitter_->failed())
			return -1;

		subBlitter_->blit(winId(), portGrabber_.port(), size());
		XSync(QX11Info::display(), False);
		return 0;
	}

	virtual void acceptSettings() {
		confWidget_.store();
		if (subBlitter_
			&& (   portGrabber_.port() <  confWidget_.basePortId()
			    || portGrabber_.port() >= confWidget_.basePortId()
			                            + confWidget_.numPortIds())) {
			initPort();

			{
				BufferLock lock(*this);
				setBufferDimensions(inBuffer().width, inBuffer().height,
				                    SetBuffer(lock));
			}

			repaint();
		}
	}

	virtual void rejectSettings() const { confWidget_.restore(); }
	virtual QWidget * settingsWidget() const { return confWidget_.qwidget(); }
	virtual QPaintEngine * paintEngine() const { return 0; }

protected:
	virtual void consumeBuffer(SetBuffer setInputBuffer) {
		if (portGrabber_.grabbed()) {
			subBlitter_->flip();
			setInputBuffer(subBlitter_->pixels(), inBuffer().pixelFormat,
			               subBlitter_->pitch());
		}
	}

	virtual void paintEvent(QPaintEvent *event) {
		QRect const &rect = event->rect();
		XFillRectangle(QX11Info::display(), winId(), gc_,
		               rect.x(), rect.y(), rect.width(), rect.height());

		if (isPaused() && portGrabber_.grabbed())
			subBlitter_->blit(winId(), portGrabber_.port(), size());
	}

	virtual void setBufferDimensions(unsigned width, unsigned height,
	                                 SetBuffer setInputBuffer);

private:
	GC const gc_;
	ConfWidget confWidget_;
	PortGrabber portGrabber_;
	scoped_ptr<SubBlitter> subBlitter_;

	void initPort();
	virtual void privSetPaused(bool /*paused*/) {}
};

void XvBlitter::setBufferDimensions(unsigned const width, unsigned const height,
                                    SetBuffer setInputBuffer)
{
	int const formatid = confWidget_.formatId();
	subBlitter_.reset();
	subBlitter_ = createSubBlitter(portGrabber_.port(), formatid, QSize(width, height));
	setInputBuffer(subBlitter_->pixels(),
	               formatid == formatid_rgb32 ? PixelBuffer::RGB32 : PixelBuffer::UYVY,
	               subBlitter_->pitch());
}

void XvBlitter::initPort() {
	if (portGrabber_.grab(confWidget_.basePortId(), confWidget_.numPortIds())) {
		XvAttributes attribs(portGrabber_.port());
		attribs.set("XV_AUTOPAINT_COLORKEY", 0);
		attribs.set("XV_COLORKEY", color_key);
	}
}

} // anon ns

transfer_ptr<BlitterWidget> createXvBlitter(VideoBufferLocker vbl, QWidget *parent) {
	return transfer_ptr<BlitterWidget>(new XvBlitter(vbl, parent));
}
