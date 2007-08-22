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
#ifndef FULLRESTOGGLER_H
#define FULLRESTOGGLER_H

#include <QObject>
#include <vector>

#include "resinfo.h"

class FullResToggler : public QObject {
	Q_OBJECT

public:
	virtual ~FullResToggler() {};
	virtual unsigned currentResIndex() const = 0;
	virtual unsigned currentRateIndex() const = 0;
	virtual bool isFullRes() const = 0;
	virtual void setMode(unsigned newID, unsigned rate) = 0;
	virtual void setFullRes(bool enable) = 0;
	virtual void emitRate() = 0;
	virtual const std::vector<ResInfo>& resVector() const = 0;

signals:
	void rateChange(int newHz);
//	void modeChange();
};

#endif
