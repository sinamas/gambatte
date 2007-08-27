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
#ifndef XVBLITTER_H
#define XVBLITTER_H

#include <QComboBox>

#include "../blitterwidget.h"

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>

class XvSubBlitter;

class XvBlitter : public BlitterWidget {
// 	XShmSegmentInfo shminfo;
	XvSubBlitter *subBlitter;
	XvPortID xvport;
// 	u_int16_t *xvbuffer;
// 	u_int32_t *yuv_table;
// 	XvImage *xvimage;
	QWidget *confWidget;
	QComboBox *portSelector;
	unsigned int inWidth, inHeight;
	int old_w, old_h;
	unsigned portIndex;
	GC gc;
// 	bool init;
	bool keepRatio;
	bool integerScaling;
	bool shm;
	bool portGrabbed;
	bool failed;
	bool initialized;
	
	void initPort();

protected:
	void paintEvent(QPaintEvent *event);
// 	void resizeEvent(QResizeEvent *event);

public:
	XvBlitter(QWidget *parent = 0);
	~XvBlitter();
	void init();
	void uninit();
// 	void init(const unsigned int srcW, const unsigned int srcH);
	bool isUnusable();
	void keepAspectRatio(const bool enable);
	bool keepsAspectRatio();
	void scaleByInteger(const bool enable);
	bool scalesByInteger();
	int sync(bool turbo);
	void setBufferDimensions(const unsigned int width, const unsigned int height);
	const PixelBuffer inBuffer();
	void blit();
	
	QWidget* settingsWidget() { return confWidget; }
	void acceptSettings();
	void rejectSettings();
};

#endif
