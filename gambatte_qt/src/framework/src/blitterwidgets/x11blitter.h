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
#ifndef X11BLITTER_H
#define X11BLITTER_H

#include "../blitterwidget.h"
#include "persistcheckbox.h"
#include "array.h"
#include <memory>

class X11Blitter : public BlitterWidget {
	class SubBlitter;
	class ShmBlitter;
	class PlainBlitter;
	
	struct VisInfo {
		void *visual;
		unsigned depth;
	};
	
	const std::auto_ptr<QWidget> confWidget;
	std::auto_ptr<SubBlitter> subBlitter;
	PersistCheckBox bf_;
	Array<char> buffer;
	VisInfo visInfo;
	
protected:
	void setBufferDimensions(const unsigned width, const unsigned height);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	
public:
	explicit X11Blitter(VideoBufferLocker vbl, QWidget *parent = 0);
	~X11Blitter();
	void init();
	void uninit();
	bool isUnusable() const;
	long sync();
	void blit();
	QWidget* settingsWidget() const { return confWidget.get(); }
	void acceptSettings();
	void rejectSettings() const;
	
	QPaintEngine* paintEngine() const { return 0; }
};

#endif
