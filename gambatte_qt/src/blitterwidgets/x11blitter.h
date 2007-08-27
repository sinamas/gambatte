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
#ifndef X11BLITTER_H
#define X11BLITTER_H

#include "../blitterwidget.h"

class X11SubBlitter;
class VideoBufferReseter;

class X11Blitter : public BlitterWidget {
	VideoBufferReseter &resetVideoBuffer;
	X11SubBlitter *subBlitter;
	char *buffer;
	unsigned int inWidth, inHeight;
	unsigned int scale;
// 	bool init;
// 	bool keepRatio;
// 	bool integerScaling;
	bool shm;
// 	bool failed;
	
protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	
public:
	X11Blitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent = 0);
	~X11Blitter();
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
};

#endif
