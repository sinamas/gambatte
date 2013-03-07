/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef QPAINTERBLITTER_H
#define QPAINTERBLITTER_H

#include "../blitterwidget.h"
#include "persistcheckbox.h"
#include "scoped_ptr.h"

class QImage;

class QPainterBlitter : public BlitterWidget {
public:
	explicit QPainterBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	virtual ~QPainterBlitter();
	virtual void blit();
	virtual void draw();
	virtual void setBufferDimensions(unsigned w, unsigned h);
	virtual void uninit();
	virtual QWidget * settingsWidget() const { return confWidget_.get(); }
	virtual void acceptSettings();
	virtual void rejectSettings() const;

protected:
	virtual void privSetPaused(bool ) {}
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	scoped_ptr<QWidget> const confWidget_;
	scoped_ptr<QImage> image0_;
	scoped_ptr<QImage> image1_;
	PersistCheckBox bf_;
};

#endif
