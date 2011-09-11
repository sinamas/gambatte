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
#ifndef FULLMODETOGGLER_H
#define FULLMODETOGGLER_H

#include <QObject>
#include <QWidget>
#include <QRect>
#include <QString>
#include <vector>

#include "resinfo.h"

class FullModeToggler : public QObject {
	Q_OBJECT

public:
	virtual ~FullModeToggler() {};
	virtual unsigned currentResIndex(unsigned screen) const = 0;
	virtual unsigned currentRateIndex(unsigned screen) const = 0;
	virtual const QRect fullScreenRect(const QWidget *w) const { return w->geometry(); }
	virtual bool isFullMode() const = 0;
	virtual void setMode(unsigned screen, unsigned resIndex, unsigned rateIndex) = 0;
	virtual void setFullMode(bool enable) = 0;
	virtual void emitRate() = 0;
	virtual const std::vector<ResInfo>& modeVector(unsigned screen) const = 0;
	virtual void setScreen(const QWidget *widget) = 0;
	virtual unsigned screen() const = 0;
	virtual unsigned screens() const = 0;
	virtual const QString screenName(unsigned screen) const { return QString::number(screen + 1); }

signals:
	void rateChange(int newHz);
//	void modeChange();
};

#endif
