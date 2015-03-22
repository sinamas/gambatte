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

#ifndef BLITTERCONTAINER_H
#define BLITTERCONTAINER_H

#include "scalingmethod.h"
#include <QSize>
#include <QWidget>

class BlitterWidget;

class BlitterContainer : public QWidget {
public:
	explicit BlitterContainer(QWidget *parent = 0);
	void hideCursor();
	void showCursor();
	void setAspectRatio(QSize const &);
	void setBlitter(BlitterWidget *blitter);
	void setScalingMethod(ScalingMethod);
	void setSourceSize(QSize const &);
	void parentExclusiveEvent(bool fs) { parentExclusive_ = fs; testExclusive(); }
	BlitterWidget * blitter() { return blitter_; }
	QSize const sourceSize() const { return sourceSize_; }

protected:
	virtual void moveEvent(QMoveEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	BlitterWidget *blitter_;
	QSize aspectRatio_;
	QSize sourceSize_;
	ScalingMethod scalingMethod_;
	bool parentExclusive_;
	bool cursorHidden_;

	QSize const correctedSize() const;
	void updateLayout();
	void testExclusive();
};

#endif
