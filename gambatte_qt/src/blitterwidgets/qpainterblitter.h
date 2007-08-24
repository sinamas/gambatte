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
#ifndef QPAINTERBLITTER_H
#define QPAINTERBLITTER_H

#include "../blitterwidget.h"
#include <stdint.h>

class QPainter;
class QImage;
class VideoBufferReseter;

class QPainterBlitter : public BlitterWidget {
	VideoBufferReseter &resetVideoBuffer;
	QImage *image;
	uint32_t *buffer;
	unsigned int inWidth, inHeight;
	unsigned int scale;
	
protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	
public:
	QPainterBlitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent = 0);
	~QPainterBlitter();
	void blit();
	const PixelBuffer inBuffer();
	void keepAspectRatio(bool enable);
	bool keepsAspectRatio();
	void scaleByInteger(bool enable);
	bool scalesByInteger();
	void setBufferDimensions(unsigned int w, unsigned int h);
	void uninit();
};

#endif
