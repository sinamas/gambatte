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
#include "../swscale.h"
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>

QPainterBlitter::QPainterBlitter(VideoBufferLocker vbl, QWidget *parent) :
	BlitterWidget(vbl, "QPainter", false, parent),
	confWidget(new QWidget),
	bf_(new QCheckBox(QString("Semi-bilinear filtering"), confWidget.get()), "qpainterblitter/bf", false),
	buffer(0)
{
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(bf_.checkBox());

	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

QPainterBlitter::~QPainterBlitter() {
	uninit();
}

void QPainterBlitter::blit() {
	if (image2.get()) {
		backImage = backImage == image2.get() ? image.get() : image2.get();
		setPixelBuffer(backImage->bits(), PixelBuffer::RGB32, backImage->bytesPerLine() >> 2);
	} else {
		if (bf_.value()) {
			semiLinearScale<quint32, 0xFF00FF, 0x00FF00, 8>(buffer, reinterpret_cast<quint32*>(image->bits()),
					inBuffer().width, inBuffer().height, image->width(), image->height(), image->bytesPerLine() >> 2);
		} else {
			nearestNeighborScale(buffer, reinterpret_cast<quint32*>(image->bits()),
					inBuffer().width, inBuffer().height, image->width(), image->height(), image->bytesPerLine() >> 2);
		}
	}
}

void QPainterBlitter::draw() {
	repaint();
}

void QPainterBlitter::paintEvent(QPaintEvent *const event) {
	if (!image.get())
		return;
	
	QImage *const frontImage = image2.get() && backImage == image.get() ? image2.get() : image.get();
	QPainter painter(this);
	painter.setClipRegion(event->region());
	painter.drawImage(rect(), *frontImage);
}

void QPainterBlitter::resizeEvent(QResizeEvent */*event*/) {
	if (image.get()) {
		lockPixelBuffer();
		setBufferDimensions(inBuffer().width, inBuffer().height);
		unlockPixelBuffer();
	}
}

void QPainterBlitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	uninit();
	image.reset(new QImage(width(), height(), QImage::Format_RGB32));

	if (width() != static_cast<int>(w) || height() != static_cast<int>(h)) {
		buffer = new quint32[w * h];
		setPixelBuffer(buffer, PixelBuffer::RGB32, w);
	} else {
		image2.reset(new QImage(w, h, QImage::Format_RGB32));
		backImage = image.get();
		setPixelBuffer(backImage->bits(), PixelBuffer::RGB32, backImage->bytesPerLine() >> 2);
	}
}

void QPainterBlitter::uninit() {
	image.reset();
	
	if (image2.get()) {
		image2.reset();
	} else {
		delete[] buffer;
	}
	
	buffer = NULL;
}

void QPainterBlitter::acceptSettings() {
	bf_.accept();
}

void QPainterBlitter::rejectSettings() const {
	bf_.reject();
}
