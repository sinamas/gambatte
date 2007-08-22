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
#ifndef NULLTOGGLER_H
#define NULLTOGGLER_H

#include "../fullrestoggler.h"

class NullToggler : public FullResToggler {
	const std::vector<ResInfo> nullVector;
	bool fullRes;
	
public:
	NullToggler() : fullRes(false) {}
	
	unsigned currentResIndex() const { return 0; }
	unsigned currentRateIndex() const { return 0; }
	bool isFullRes() const { return fullRes; }
	void setMode(unsigned /*newID*/, unsigned /*rate*/) {}
	void setFullRes(const bool enable) { fullRes = enable; }
	void emitRate() {}
	const std::vector<ResInfo>& resVector() const { return nullVector; }
};

#endif
