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
#include "blittercontainer.h"

#include "blitterwidget.h"
#include "fullrestoggler.h"
#include <QResizeEvent>

BlitterContainer::BlitterContainer(const FullResToggler &resToggler_in, QWidget *parent) :
	QWidget(parent),
	resToggler(resToggler_in),
	blitter(NULL)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(0, 0, 0));
	setPalette(pal);
// 	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);
}

BlitterContainer::~BlitterContainer()
{}

void BlitterContainer::doLayout(const int w, const int h) {
	if (!blitter)
		return;
	
	if (blitter->keepsAspectRatio() && !blitter->selfScaling) {
		if (blitter->scalesByInteger()) {
			const int scale = std::min(w / minimumWidth(), h / minimumHeight());
			const int new_w = minimumWidth() * scale;
			const int new_h = minimumHeight() * scale;
			blitter->setGeometry(w - new_w >> 1, h - new_h >> 1, new_w, new_h);
		} else {
			if (w * 9 > h * 10) {
				const int new_w = (h * 20 + 9) / 18;
				blitter->setGeometry(w - new_w >> 1, 0, new_w, h);
			} else {
				const int new_h = (w * 9 + 5) / 10;
				blitter->setGeometry(0, h - new_h >> 1, w, new_h);
			}
		}
	} else {
		blitter->setGeometry(0, 0, w, h);
	}
	
	if (!resToggler.resVector().empty()) {
		const ResInfo &res = resToggler.resVector()[resToggler.currentResIndex()];
		blitter->setExclusive(res.w == static_cast<unsigned>(w) && res.h == static_cast<unsigned>(h));
	}
}

void BlitterContainer::setBlitter(BlitterWidget *const blitter_in) {
	if (blitter)
		blitter->setParent(0);
	
	blitter = blitter_in;
	
	if (blitter) {
		blitter->setParent(this);
		updateLayout();
	}
}

void BlitterContainer::resizeEvent(QResizeEvent *const event) {
	doLayout(event->size().width(), event->size().height());
}

void BlitterContainer::hideEvent(QHideEvent *const event) {
	if (blitter)
		blitter->setExclusive(false);
	
	QWidget::hideEvent(event);
}
