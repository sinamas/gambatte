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
#include "blittercontainer.h"
#include "blitterwidget.h"
#include <QResizeEvent>

BlitterContainer::BlitterContainer(QWidget *parent) :
	QWidget(parent),
	blitter_(NULL),
	aspectRatio_(64, 48),
	sourceSize_(64, 48),
	scalingMethod_(UNRESTRICTED),
	parentExclusive(false),
	cursorHidden_(false)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(0, 0, 0));
	setPalette(pal);
// 	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);
	setMouseTracking(true);
}

BlitterContainer::~BlitterContainer()
{}

void BlitterContainer::doLayout(const int w, const int h) {
	if (!blitter_)
		return;

	if (scalingMethod_ == UNRESTRICTED) {
		blitter_->setCorrectedGeometry(w, h, w, h);
	} else if (scalingMethod_ == KEEP_RATIO) {
		const QSize &ar = aspectRatio_;

		if (w * ar.height() > h * ar.width()) {
			const int new_w = (h * ar.width() + (ar.height() >> 1)) / ar.height();
			blitter_->setCorrectedGeometry(w, h, new_w, h);
		} else {
			const int new_h = (w * ar.height() + (ar.width() >> 1)) / ar.width();
			blitter_->setCorrectedGeometry(w, h, w, new_h);
		}
	} else {
		const QSize &src = sourceSize_;
		const int scale = std::min(w / src.width(), h / src.height());
		const int new_w = src.width() * scale;
		const int new_h = src.height() * scale;
		blitter_->setCorrectedGeometry(w, h, new_w, new_h);
	}
}

void BlitterContainer::testExclusive() {
	if (blitter_)
		blitter_->setExclusive(parentExclusive && pos() == QPoint(0,0));
}

void BlitterContainer::setBlitter(BlitterWidget *const blitter_in) {
	if (blitter_ != blitter_in) {
// 		if (blitter_)
// 			blitter_->setParent(0);
	
		blitter_ = blitter_in;
	
// 		if (blitter_)
// 			blitter_->setParent(this);
		
		updateLayout();
		testExclusive();
	}
}

void BlitterContainer::hideCursor() {
	if (!cursorHidden_)
		setCursor(Qt::BlankCursor);

	cursorHidden_ = true;
}

void BlitterContainer::showCursor() {
	if (cursorHidden_)
		unsetCursor();

	cursorHidden_ = false;
}

void BlitterContainer::setAspectRatio(const QSize &ar) {
	if (aspectRatio_ != ar) {
		aspectRatio_ = ar;
		updateLayout();
	}
}

void BlitterContainer::setScalingMethod(const ScalingMethod st) {
	if (scalingMethod_ != st) {
		scalingMethod_ = st;
		updateLayout();
	}
}

void BlitterContainer::setSourceSize(const QSize &ss) {
	if (sourceSize_ != ss) {
		sourceSize_ = ss;
		updateLayout();
	}
}

void BlitterContainer::moveEvent(QMoveEvent */*e*/) {
	testExclusive();
}

void BlitterContainer::resizeEvent(QResizeEvent *const event) {
	doLayout(event->size().width(), event->size().height());
}
