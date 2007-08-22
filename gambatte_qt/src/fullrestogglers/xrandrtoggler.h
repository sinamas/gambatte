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
#ifndef XRANDRTOGGLER_H
#define XRANDRTOGGLER_H

#include "../fullrestoggler.h"

#include <QObject>
#include <vector>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include "../resinfo.h"

class XRandRToggler : public FullResToggler {
	Q_OBJECT
		
	std::vector<ResInfo> infoVector;
// 	XRRScreenConfiguration *config;
	unsigned originalResIndex;
	unsigned fullResIndex;
	Rotation rotation;
	unsigned originalRateIndex;
	unsigned fullRateIndex;
	bool isFull;
	
public:
	static bool isUsable();
	XRandRToggler();
	~XRandRToggler();
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

#endif
