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
#ifndef QGLBLITTER_H
#define QGLBLITTER_H

#include "../blitterwidget.h"
#include <memory>

class QCheckBox;
class VideoDialog;

class QGLBlitter : public BlitterWidget {
	class SubWidget;
	
	SubWidget *subWidget;
	const std::auto_ptr<QWidget> confWidget;
	QCheckBox *const vsyncBox;
	QCheckBox *const bfBox;
	quint32 *buffer;
	unsigned hz;
	unsigned hz1;
	unsigned hz2;
	bool vsync;
	bool bf;
	
	void resetSubWidget();
	
protected:
	void resizeEvent(QResizeEvent *event);
	
public:
	QGLBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent = 0);
	~QGLBlitter();
// 	void init();
	void uninit();
	bool isUnusable() const;
	void setBufferDimensions(unsigned int width, unsigned int height);
	void setCorrectedGeometry(int w, int h, int new_w, int new_h);
	void setFrameTime(Rational ft);
	void blit();
	const Rational frameTime() const;
	int sync(bool turbo);
	QWidget* settingsWidget() { return confWidget.get(); }
	
// // 	public slots:
	void acceptSettings();
	void rejectSettings();
	
public slots:
	void rateChange(int hz);
	
};

#endif
