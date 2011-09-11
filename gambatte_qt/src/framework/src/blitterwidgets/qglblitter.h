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
#include "../dwmcontrol.h"
#include "persistcheckbox.h"
#include "array.h"
#include <memory>

class QGLBlitter : public BlitterWidget {
	class SubWidget;
	
	const DwmControlHwndChange hwndChange_;
	FtEst ftEst;
	const std::auto_ptr<QWidget> confWidget;
	PersistCheckBox vsync_;
	PersistCheckBox bf_;
	Array<quint32> buffer;
	unsigned swapInterval_;
	int dhz;
	std::auto_ptr<SubWidget> subWidget;
	
	void resetSubWidget();
	void privSetPaused(const bool /*paused*/) {}
	
protected:
	void setBufferDimensions(unsigned int width, unsigned int height);
	void resizeEvent(QResizeEvent *event);
	
public:
	explicit QGLBlitter(VideoBufferLocker vbl, DwmControlHwndChange hwndChange, QWidget *parent = 0);
	~QGLBlitter();
	void uninit();
	bool isUnusable() const;
	void setCorrectedGeometry(int w, int h, int new_w, int new_h);
	WId hwnd() const;
	long frameTimeEst() const;
	void blit();
	void draw();
	long sync();
	QWidget* settingsWidget() const { return confWidget.get(); }
	
	void acceptSettings();
	void rejectSettings() const;
	
	void setSwapInterval(unsigned);
	void rateChange(int dhz);
	void compositionEnabledChange();
};

#endif
