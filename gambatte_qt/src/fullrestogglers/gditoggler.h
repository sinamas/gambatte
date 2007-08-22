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

#ifndef GDITOGGLER_H_
#define GDITOGGLER_H_

#include "../fullrestoggler.h"

#include <QObject>
#include <vector>

#include "../resinfo.h"

class GdiToggler : public FullResToggler {
	Q_OBJECT
		
	std::vector<ResInfo> infoVector;
	unsigned originalResIndex;
	unsigned originalRateIndex;
	unsigned fullResIndex;
	unsigned fullRateIndex;
	bool isFull;
	
public:
	GdiToggler();
	~GdiToggler();
	unsigned currentResIndex() const { return fullResIndex; }
	unsigned currentRateIndex() const { return fullRateIndex; }
	bool isFullRes() const;
	void setMode(unsigned resIndex, unsigned rateIndex);
	void setFullRes(bool enable);
	void emitRate();
	const std::vector<ResInfo>& resVector() const { return infoVector; }
	
signals:
	void rateChange(int newHz);
//	void modeChange();
};

#endif /*GDITOGGLER_H_*/
