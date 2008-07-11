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
#ifndef BLITTERWIDGET_H
#define BLITTERWIDGET_H

#include "pixelbuffersetter.h"
#include <QWidget>
#include <QString>
#include <memory>

class QHBoxLayout;

class BlitterWidget : public QWidget {
	Q_OBJECT

	class Impl;
	
	Impl *const impl;

public:
	struct Rational {
		unsigned numerator;
		unsigned denominator;
		
		Rational(unsigned num = 1, unsigned den = 60) : numerator(num), denominator(den) {}
	};
	
protected:
	PixelBufferSetter setPixelBuffer;
	
public:
	const QString nameString;
	const bool integerOnlyScaler;
	
	BlitterWidget(PixelBufferSetter setPixelBuffer,
	              const QString &name,
	              bool integerOnlyScaler = false,
	              QWidget *parent = 0);
	virtual ~BlitterWidget();
	
	virtual void init() {}
	virtual void uninit() {}
	virtual void blit() = 0;
	virtual bool isUnusable() const { return false; }
	virtual void setBufferDimensions(unsigned width, unsigned height) = 0;
	virtual void setCorrectedGeometry(int w, int h, int new_w, int new_h) { setGeometry((w - new_w) >> 1, (h - new_h) >> 1, new_w, new_h); }
	virtual void setFrameTime(Rational ft);
	virtual const Rational frameTime() const;
	virtual int sync(bool turbo);
	virtual QWidget* settingsWidget() { return NULL; }
	virtual void setExclusive(bool) {}

// 	public slots:
	virtual void acceptSettings() {}
	virtual void rejectSettings() {}

public slots:
	virtual void rateChange(int /*newHz*/) {}
//	virtual void modeChange() {}
};

#endif
