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
#ifndef QPAINTERBLITTER_H
#define QPAINTERBLITTER_H

#include "../blitterwidget.h"
#include "persistcheckbox.h"
#include <memory>
#include <QImage>

class QPainter;
class QCheckBox;

class QPainterBlitter : public BlitterWidget {
	const std::auto_ptr<QWidget> confWidget;
	std::auto_ptr<QImage> image;
	std::auto_ptr<QImage> image2;
	PersistCheckBox bf_;
	union { quint32 *buffer; QImage *backImage; };
	
protected:
	void privSetPaused(bool) {}
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	
public:
	explicit QPainterBlitter(VideoBufferLocker vbl, QWidget *parent = 0);
	~QPainterBlitter();
	void blit();
	void draw();
	void setBufferDimensions(unsigned int w, unsigned int h);
	void uninit();
	QWidget* settingsWidget() const { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings() const;
};

#endif
