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
#ifndef X11BLITTER_H
#define X11BLITTER_H

#include "../blitterwidget.h"
#include "array.h"
#include "persistcheckbox.h"
#include "scoped_ptr.h"

class X11Blitter : public BlitterWidget {
public:
	explicit X11Blitter(VideoBufferLocker vbl, QWidget *parent = 0);
	virtual ~X11Blitter();
	virtual void init();
	virtual void uninit();
	virtual bool isUnusable() const;
	virtual long sync();
	virtual void blit();
	virtual QWidget * settingsWidget() const { return confWidget.get(); }
	virtual void acceptSettings();
	virtual void rejectSettings() const;
	virtual QPaintEngine * paintEngine() const { return 0; }

protected:
	virtual void setBufferDimensions(unsigned width, unsigned height);
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	class SubBlitter;
	class ShmBlitter;
	class PlainBlitter;

	struct VisInfo {
		void *visual;
		unsigned depth;
	};

	const scoped_ptr<QWidget> confWidget;
	scoped_ptr<SubBlitter> subBlitter;
	PersistCheckBox bf_;
	Array<char> buffer;
	VisInfo visInfo;
};

#endif
