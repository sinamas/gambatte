/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef XVBLITTER_H
#define XVBLITTER_H

#include <QComboBox>
#include <memory>

#include "../blitterwidget.h"
#include "uncopyable.h"

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvBlitter : public BlitterWidget {
	class SubBlitter;
	class ShmBlitter;
	class PlainBlitter;
	
	class ConfWidget {
		const std::auto_ptr<QWidget> widget_;
		QComboBox *const portSelector_;
		unsigned portIndex_;
		
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
	};
	
	class PortGrabber : Uncopyable {
		XvPortID port_;
		bool grabbed_;
	public:
		PortGrabber();
		~PortGrabber();
		bool grab(XvPortID port, unsigned numPorts);
		void ungrab();
		bool grabbed() const { return grabbed_; }
		XvPortID port() const { return port_; }
	};
	
	ConfWidget confWidget;
	PortGrabber portGrabber;
	std::auto_ptr<SubBlitter> subBlitter;
	GC gc;
	bool initialized;
	
	void initPort();
	void privSetPaused(const bool /*paused*/) {}

protected:
	void paintEvent(QPaintEvent *event);
// 	void resizeEvent(QResizeEvent *event);

public:
	explicit XvBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	~XvBlitter();
	void init();
	void uninit();
	bool isUnusable() const;
	long sync();
	void setBufferDimensions(const unsigned int width, const unsigned int height);
	void blit();
	
	QWidget* settingsWidget() const { return confWidget.qwidget(); }
	void acceptSettings();
	void rejectSettings() const;
	
	QPaintEngine* paintEngine() const { return NULL; }
};

#endif
