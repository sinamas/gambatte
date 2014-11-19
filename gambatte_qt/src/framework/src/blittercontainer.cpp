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

#include "blittercontainer.h"
#include "blitterwidget.h"

BlitterContainer::BlitterContainer(QWidget *parent)
: QWidget(parent)
, blitter_()
, aspectRatio_(64, 48)
, sourceSize_(64, 48)
, scalingMethod_(scaling_unrestricted)
, parentExclusive_(false)
, cursorHidden_(false)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(0, 0, 0));
	setPalette(pal);
	setAutoFillBackground(true);
	setMouseTracking(true);
}

// custom impl becuase QSize::scale does not round to nearest
static QSize const aspectCorrected(QSize const &ar, int w, int h) {
	if (w * ar.height() > h * ar.width()) {
		w = (h * ar.width() + (ar.height() >> 1)) / ar.height();
	} else {
		h = (w * ar.height() + (ar.width() >> 1)) / ar.width();
	}

	return QSize(w, h);
}

static QSize const integerScaled(QSize const &source, QSize const &target) {
	int const scale = std::min(target.width() / source.width(),
	                           target.height() / source.height());
	return QSize(source.width() * scale, source.height() * scale);
}

QSize const BlitterContainer::correctedSize() const {
	switch (scalingMethod_) {
	case scaling_unrestricted: break;
	case scaling_keep_ratio:   return aspectCorrected(aspectRatio_, width(), height());
	case scaling_integer:      return integerScaled(sourceSize_, size());
	}

	return size();
}

void BlitterContainer::updateLayout() {
	if (blitter_) {
		QSize const corrected = correctedSize();
		blitter_->setCorrectedGeometry(width(), height(),
		                               corrected.width(), corrected.height());
	}
}

void BlitterContainer::testExclusive() {
	if (blitter_)
		blitter_->setExclusive(parentExclusive_ && pos() == QPoint(0, 0));
}

void BlitterContainer::setBlitter(BlitterWidget *blitter) {
	if (blitter_ != blitter) {
		blitter_ = blitter;
		updateLayout();
		testExclusive();
	}
}

void BlitterContainer::hideCursor() {
	if (!cursorHidden_) {
		setCursor(Qt::BlankCursor);
		cursorHidden_ = true;
	}
}

void BlitterContainer::showCursor() {
	if (cursorHidden_) {
		unsetCursor();
		cursorHidden_ = false;
	}
}

void BlitterContainer::setAspectRatio(QSize const &ar) {
	if (aspectRatio_ != ar) {
		aspectRatio_ = ar;
		updateLayout();
	}
}

void BlitterContainer::setScalingMethod(ScalingMethod st) {
	if (scalingMethod_ != st) {
		scalingMethod_ = st;
		updateLayout();
	}
}

void BlitterContainer::setSourceSize(QSize const &ss) {
	if (sourceSize_ != ss) {
		sourceSize_ = ss;
		updateLayout();
	}
}

void BlitterContainer::moveEvent(QMoveEvent *) {
	testExclusive();
}

void BlitterContainer::resizeEvent(QResizeEvent *) {
	updateLayout();
}
