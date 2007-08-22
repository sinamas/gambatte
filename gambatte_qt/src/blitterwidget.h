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
#ifndef BLITTERWIDGET_H
#define BLITTERWIDGET_H

#include <QWidget>
#include <videoblitter.h>
#include <QString>

class QHBoxLayout;

class BlitterWidget : public QWidget, public VideoBlitter {
	Q_OBJECT

public:
	struct Rational {
		unsigned numerator;
		unsigned denominator;
	};
	
	const QString nameString;
	const bool integerOnlyScaler;
	const bool selfScaling;
	
	BlitterWidget(const QString &name, bool integerOnlyScaler_in = false, bool selfScaling_in = false, QWidget *parent = 0) :
		QWidget(parent),
		nameString(name),
		integerOnlyScaler(integerOnlyScaler_in),
		selfScaling(selfScaling_in)
	{}
	
	virtual void init() {}
	virtual void uninit() {}
	virtual bool isUnusable() { return false; }
	virtual void keepAspectRatio(const bool enable) = 0;
	virtual bool keepsAspectRatio() = 0;
	virtual void scaleByInteger(const bool enable) = 0;
	virtual bool scalesByInteger() = 0;
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
