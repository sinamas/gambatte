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
#include <QImage>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>

QPainterBlitter::QPainterBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
	BlitterWidget(setPixelBuffer, "QPainter", false, parent),
	confWidget(new QWidget),
	bfBox(new QCheckBox(QString("Bilinear filtering"))),
	buffer(NULL),
	inWidth(160),
	inHeight(144),
	bf(false)
{
	QSettings settings;
	settings.beginGroup("qpainterblitter");
	bf = settings.value("bf", false).toBool();
	settings.endGroup();
	
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(bfBox);
	bfBox->setChecked(bf);
	
	setAttribute(Qt::WA_OpaquePaintEvent, true);
}

QPainterBlitter::~QPainterBlitter() {
	uninit();
	
	QSettings settings;
	settings.beginGroup("qpainterblitter");
	settings.setValue("bf", bf);
	settings.endGroup();
}

void QPainterBlitter::blit() {
	repaint();
}

void QPainterBlitter::paintEvent(QPaintEvent *const event) {
	if (buffer) {
		if (bf)
			linearScale<quint32, 0xFF00FF, 0x00FF00, 8>(buffer, reinterpret_cast<quint32*>(image->bits()), inWidth, inHeight, width(), height(), image->bytesPerLine() >> 2);
		else
			nearestNeighborScale(buffer, reinterpret_cast<quint32*>(image->bits()), inWidth, inHeight, width(), height(), image->bytesPerLine() >> 2);
	}
	
	QPainter painter(this);
	painter.setClipRegion(event->region());
	painter.drawImage(rect(), *image);
}

void QPainterBlitter::resizeEvent(QResizeEvent */*event*/) {
	if (image.get())
		setBufferDimensions(inWidth, inHeight);
}

void QPainterBlitter::setBufferDimensions(const unsigned int w, const unsigned int h) {
	inWidth = w;
	inHeight = h;
	
	uninit();
	image.reset(new QImage(width(), height(), QImage::Format_RGB32));
	
	if (width() != static_cast<int>(w) || height() != static_cast<int>(h)) {
		buffer = new quint32[w * h];
		setPixelBuffer(buffer, MediaSource::RGB32, w);
	} else
		setPixelBuffer(image->bits(), MediaSource::RGB32, image->bytesPerLine() >> 2);
}

void QPainterBlitter::uninit() {
	image.reset();
	
	delete []buffer;
	buffer = NULL;
}

void QPainterBlitter::acceptSettings() {
	bf = bfBox->isChecked();
}

void QPainterBlitter::rejectSettings() {
	bfBox->setChecked(bf);
}
