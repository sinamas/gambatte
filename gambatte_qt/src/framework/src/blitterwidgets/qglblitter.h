/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#ifndef QGLBLITTER_H
#define QGLBLITTER_H

#include "../blitterwidget.h"
#include "../dwmcontrol.h"
#include "array.h"
#include "persistcheckbox.h"
#include "scoped_ptr.h"

class QGLBlitter : public BlitterWidget {
public:
	QGLBlitter(VideoBufferLocker vbl, DwmControlHwndChange hwndChange, QWidget *parent = 0);
	virtual ~QGLBlitter();
	virtual void uninit();
	virtual bool isUnusable() const;
	virtual void setCorrectedGeometry(int w, int h, int new_w, int new_h);
	virtual WId hwnd() const;
	virtual long frameTimeEst() const;
	virtual void blit();
	virtual void draw();
	virtual long sync();
	virtual QWidget * settingsWidget() const { return confWidget_.get(); }

	virtual void acceptSettings();
	virtual void rejectSettings() const;

	virtual void setSwapInterval(unsigned);
	virtual void rateChange(int dhz);
	virtual void compositionEnabledChange();

protected:
	virtual void setBufferDimensions(unsigned width, unsigned height);
	virtual void resizeEvent(QResizeEvent *event);

private:
	class SubWidget;

	DwmControlHwndChange const hwndChange_;
	scoped_ptr<QWidget> const confWidget_;
	FtEst ftEst_;
	PersistCheckBox vsync_;
	PersistCheckBox bf_;
	Array<quint32> buffer_;
	unsigned swapInterval_;
	int dhz_;
	scoped_ptr<SubWidget> subWidget_;

	void resetSubWidget();
	virtual void privSetPaused(bool ) {}
};

#endif
