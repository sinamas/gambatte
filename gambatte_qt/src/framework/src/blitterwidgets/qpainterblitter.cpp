//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "qpainterblitter.h"
#include "../blitterwidget.h"
#include "../swscale.h"
#include "dialoghelpers.h"
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
	, bf_(new QCheckBox(tr("Semi-bilinear filtering"), confWidget_.get()),
	      "qpainterblitter/bf", false)
	{
		confWidget_->setLayout(new QVBoxLayout);
		confWidget_->layout()->setMargin(0);
		confWidget_->layout()->addWidget(bf_.checkBox());

		setAttribute(Qt::WA_OpaquePaintEvent, true);
	}

	virtual void draw() { repaint(); }
	virtual int present() { return 0; }

	virtual void uninit() {
		image0_.reset();
		image1_.reset();
	}

	virtual QWidget * settingsWidget() const { return confWidget_.get(); }
	virtual void acceptSettings() { bf_.accept(); }
	virtual void rejectSettings() const { bf_.reject(); }

protected:
	virtual void consumeBuffer(SetBuffer setInputBuffer) {
		QImage &frontBuf = inBuffer().data == image0_->bits() ? *image1_ : *image0_;
		if (image0_->size() == image1_->size()) {
			// flip
			setInputBuffer(frontBuf.bits(), PixelBuffer::RGB32,
			               frontBuf.bytesPerLine() >> 2);
		} else {
			scaleInputBufferOnto(frontBuf);
		}
	}

	virtual void privSetPaused(bool ) {}

	virtual void setBufferDimensions(unsigned w, unsigned h, SetBuffer setInputBuffer) {
		uninit();
		image0_.reset(new QImage(w, h, QImage::Format_RGB32));
		image1_.reset(new QImage(size(), QImage::Format_RGB32));
		setInputBuffer(image0_->bits(), PixelBuffer::RGB32,
		               image0_->bytesPerLine() >> 2);
	}

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
			scaleInputBufferOnto(*frontImage);
		}
	}

private:
	scoped_ptr<QWidget> const confWidget_;
	scoped_ptr<QImage> image0_;
	scoped_ptr<QImage> image1_;
	PersistCheckBox bf_;

	void scaleInputBufferOnto(QImage &frontBuf) const;
};

void QPainterBlitter::scaleInputBufferOnto(QImage &frontBuf) const {
	if (bf_.value()) {
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
