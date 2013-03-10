/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include "qpainterblitter.h"
#include "../blitterwidget.h"
#include "../swscale.h"
#include "persistcheckbox.h"
#include "scoped_ptr.h"
#include <QCheckBox>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QSettings>
#include <QVBoxLayout>

namespace {

class QPainterBlitter : public BlitterWidget {
public:
	QPainterBlitter(VideoBufferLocker vbl, QWidget *parent)
	: BlitterWidget(vbl, "QPainter", false, parent)
	, confWidget_(new QWidget)
	, bf_(new QCheckBox(QString("Semi-bilinear filtering"), confWidget_.get()),
	      "qpainterblitter/bf", false)
	{
		confWidget_->setLayout(new QVBoxLayout);
		confWidget_->layout()->setMargin(0);
		confWidget_->layout()->addWidget(bf_.checkBox());

		setAttribute(Qt::WA_OpaquePaintEvent, true);
	}

	virtual void blit();
	virtual void draw() { repaint(); }

	virtual void setBufferDimensions(unsigned const w, unsigned const h) {
		uninit();
		image0_.reset(new QImage(w, h, QImage::Format_RGB32));
		image1_.reset(new QImage(size(), QImage::Format_RGB32));
		setPixelBuffer(image0_->bits(), PixelBuffer::RGB32,
		               image0_->bytesPerLine() >> 2);
	}

	virtual void uninit() {
		image0_.reset();
		image1_.reset();
	}

	virtual QWidget * settingsWidget() const { return confWidget_.get(); }
	virtual void acceptSettings() { bf_.accept(); }
	virtual void rejectSettings() const { bf_.reject(); }

protected:
	virtual void privSetPaused(bool ) {}

	virtual void paintEvent(QPaintEvent *event) {
		if (image0_ && image1_) {
			QImage &frontImage = inBuffer().data == image0_->bits()
			                   ? *image1_
			                   : *image0_;
			QPainter painter(this);
			painter.setClipRegion(event->region());
			painter.drawImage(rect(), frontImage);
		}
	}

	virtual void resizeEvent(QResizeEvent *) {
		if (!image0_ || !image1_)
			return;

		scoped_ptr<QImage> &backImage  =
			inBuffer().data == image0_->bits() ? image0_ : image1_;
		scoped_ptr<QImage> &frontImage =
			inBuffer().data == image0_->bits() ? image1_ : image0_;
		if (size() == backImage->size()) {
			*frontImage = *backImage;
		} else {
			frontImage.reset();
			frontImage.reset(new QImage(size(), QImage::Format_RGB32));
			blit();
		}
	}

private:
	scoped_ptr<QWidget> const confWidget_;
	scoped_ptr<QImage> image0_;
	scoped_ptr<QImage> image1_;
	PersistCheckBox bf_;
};

void QPainterBlitter::blit() {
	QImage &frontBuf = inBuffer().data == image0_->bits() ? *image1_ : *image0_;

	if (image0_->size() == image1_->size()) {
		// flip
		setPixelBuffer(frontBuf.bits(), PixelBuffer::RGB32, frontBuf.bytesPerLine() >> 2);
	} else if (bf_.value()) {
		semiLinearScale<quint32, 0xFF00FF, 0x00FF00, 8>(
			reinterpret_cast<quint32 *>(frontBuf.bits()), frontBuf.bytesPerLine() >> 2,
			frontBuf.width(), frontBuf.height(),
			static_cast<quint32 const *>(inBuffer().data), inBuffer().pitch,
			inBuffer().width, inBuffer().height);
	} else {
		nearestNeighborScale(
			reinterpret_cast<quint32 *>(frontBuf.bits()), frontBuf.bytesPerLine() >> 2,
			frontBuf.width(), frontBuf.height(),
			static_cast<quint32 const *>(inBuffer().data), inBuffer().pitch,
			inBuffer().width, inBuffer().height);
	}
}

} // anon ns

transfer_ptr<BlitterWidget> createQPainterBlitter(VideoBufferLocker vbl, QWidget *parent) {
	return transfer_ptr<BlitterWidget>(new QPainterBlitter(vbl, parent));
}
