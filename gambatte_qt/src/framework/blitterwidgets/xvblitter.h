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

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvBlitter : public BlitterWidget {
	class SubBlitter;
	class ShmBlitter;
	class PlainBlitter;
	
// 	XShmSegmentInfo shminfo;
	std::auto_ptr<SubBlitter> subBlitter;
	XvPortID xvport;
// 	u_int16_t *xvbuffer;
// 	u_int32_t *yuv_table;
// 	XvImage *xvimage;
	const std::auto_ptr<QWidget> confWidget;
	QComboBox *const portSelector;
	unsigned int inWidth, inHeight;
	int old_w, old_h;
	unsigned portIndex;
	GC gc;
// 	bool init;
	bool shm;
	bool portGrabbed;
	bool failed;
	bool initialized;
	
	void initPort();
	void privSetPaused(const bool /*paused*/) {}

protected:
	void paintEvent(QPaintEvent *event);
// 	void resizeEvent(QResizeEvent *event);

public:
	XvBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent = 0);
	~XvBlitter();
	void init();
	void uninit();
	bool isUnusable() const;
	long sync(long turbo);
	void setBufferDimensions(const unsigned int width, const unsigned int height);
	void blit();
	
	QWidget* settingsWidget() { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings();
	
	QPaintEngine* paintEngine() const { return NULL; }
};

#endif
