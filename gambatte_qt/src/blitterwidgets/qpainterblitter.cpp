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
#include "qpainterblitter.h"

#include "../scalebuffer.h"
#include "../videobufferreseter.h"

#include <QPainter>
#include <QImage>
#include <QPaintEvent>

#include <algorithm>

QPainterBlitter::QPainterBlitter(VideoBufferReseter &resetVideoBuffer_in, QWidget *parent) :
	BlitterWidget("QPainter", true, parent),
	resetVideoBuffer(resetVideoBuffer_in),
	image(NULL),
	buffer(NULL),
	inWidth(160),
	inHeight(144),
	scale(0)
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

QPainterBlitter::~QPainterBlitter() {
	uninit();
}

void QPainterBlitter::blit() {
	repaint();
}

const PixelBuffer QPainterBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = PixelBuffer::RGB32;	
	pixb.pixels = image->bits();
	pixb.pitch = image->bytesPerLine() >> 2;
	
	if (buffer) {
		pixb.pixels = buffer;
		pixb.pitch = inWidth;
	}
	
	return pixb;
}

void QPainterBlitter::paintEvent(QPaintEvent *const event) {
	if (buffer) {
		scaleBuffer(buffer, reinterpret_cast<uint32_t*>(image->bits()), inWidth, inHeight, scale);
	}
	
	QPainter painter(this);
	painter.setClipRegion(event->region());
	painter.drawImage(rect(), *image);
}

void QPainterBlitter::resizeEvent(QResizeEvent */*event*/) {
	const unsigned newScale = std::min(width() / inWidth, height() / inHeight);
		
	if (newScale != scale) {
		scale = newScale;
		
		if (image) {
			setBufferDimensions(inWidth, inHeight);
			resetVideoBuffer();
		}
	}
}

void QPainterBlitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	inWidth = w;
	inHeight = h;
	
	scale = std::min(width() / w, height() / h);
	
	uninit();
	
	if (scale > 1)
		buffer = new uint32_t[w * h];
	
	image = new QImage(w * scale, h * scale, QImage::Format_RGB32);
}

void QPainterBlitter::uninit() {
	delete image;
	image = NULL;
	
	delete[] buffer;
	buffer = NULL;
}

void QPainterBlitter::keepAspectRatio(const bool /*enable*/) {}

bool QPainterBlitter::keepsAspectRatio() {
	return true;
}

void QPainterBlitter::scaleByInteger(const bool /*enable*/) {}

bool QPainterBlitter::scalesByInteger() {
	return true;
}
