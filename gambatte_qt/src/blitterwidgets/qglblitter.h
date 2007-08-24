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
#include <stdint.h>

class QGLSubWidget;
class QCheckBox;

class QGLBlitter : public BlitterWidget {
	QGLSubWidget *subWidget;
	QWidget *const confWidget;
	QCheckBox *const vsyncBox;
	QCheckBox *const bfBox;
	uint32_t *buffer;
	unsigned hz;
	bool vsync;
	bool bf;
	
	void resetSubWidget();
	
protected:
	void resizeEvent(QResizeEvent *event);
	
public:
	QGLBlitter(QWidget *parent = 0);
	~QGLBlitter();
// 	void init();
	void uninit();
	bool isUnusable();
	void keepAspectRatio(bool enable);
	bool keepsAspectRatio();
	void scaleByInteger(bool enable);
	bool scalesByInteger();
	void setBufferDimensions(unsigned int width, unsigned int height);
	const PixelBuffer inBuffer();
	void blit();
	const Rational frameTime() const;
	int sync(bool turbo);
	QWidget* settingsWidget() { return confWidget; }
	
// // 	public slots:
	void acceptSettings();
	void rejectSettings();
	
public slots:
	void rateChange(int hz);
	
};

#endif
