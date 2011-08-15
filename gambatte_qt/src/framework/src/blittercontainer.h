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
#ifndef BLITTERCONTAINER_H
#define BLITTERCONTAINER_H

#include <QWidget>
#include <QSize>
#include "scalingmethod.h"

class BlitterWidget;

class BlitterContainer : public QWidget {
	BlitterWidget *blitter_;
	QSize aspectRatio_;
	QSize sourceSize_;
	ScalingMethod scalingMethod_;
	bool parentExclusive;
	bool cursorHidden_;
	
	void doLayout(int w, int h);
	void testExclusive();
	void updateLayout() { doLayout(width(), height()); }
	
protected:
	void moveEvent(QMoveEvent *event);
	void resizeEvent(QResizeEvent *event);
	
public:
	explicit BlitterContainer(QWidget *parent = 0);
	~BlitterContainer();
	void hideCursor();
	void showCursor();
	void setAspectRatio(const QSize &);
	void setBlitter(BlitterWidget *blitter);
	void setScalingMethod(ScalingMethod);
	void setSourceSize(const QSize &);
	void parentExclusiveEvent(bool fs) { parentExclusive = fs; testExclusive(); }
	BlitterWidget* blitter() { return blitter_; }
	const QSize& aspectRatio() const { return aspectRatio_; }
	ScalingMethod scalingMethod() const { return scalingMethod_; }
	const QSize& sourceSize() const { return sourceSize_; }
};

#endif
