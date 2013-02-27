/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef XVBLITTER_H
#define XVBLITTER_H

#include "../blitterwidget.h"
#include "scoped_ptr.h"
#include <QComboBox>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvBlitter : public BlitterWidget {
public:
	explicit XvBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	virtual ~XvBlitter();
	virtual void init();
	virtual void uninit();
	virtual bool isUnusable() const;
	virtual long sync();
	virtual void blit();
	virtual QWidget * settingsWidget() const { return confWidget.qwidget(); }
	virtual void acceptSettings();
	virtual void rejectSettings() const;
	virtual QPaintEngine * paintEngine() const { return 0; }

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void setBufferDimensions(unsigned int width, unsigned int height);

private:
	class SubBlitter;
	class ShmBlitter;
	class PlainBlitter;

	class ConfWidget {
	public:
		ConfWidget();
		~ConfWidget();
		void store();
		void restore() const;
		XvPortID basePortId() const;
		unsigned numPortIds() const;
		int formatId() const;
		int numAdapters() const;
		QWidget * qwidget() const { return widget_.get(); }

	private:
		const scoped_ptr<QWidget> widget_;
		QComboBox *const portSelector_;
		unsigned portIndex_;
	};

	class PortGrabber : Uncopyable {
	public:
		PortGrabber();
		~PortGrabber();
		bool grab(XvPortID port, unsigned numPorts);
		void ungrab();
		bool grabbed() const { return grabbed_; }
		XvPortID port() const { return port_; }

	private:
		XvPortID port_;
		bool grabbed_;
	};

	ConfWidget confWidget;
	PortGrabber portGrabber;
	scoped_ptr<SubBlitter> subBlitter;
	GC gc;
	bool initialized;

	void initPort();
	virtual void privSetPaused(bool /*paused*/) {}
};

#endif
