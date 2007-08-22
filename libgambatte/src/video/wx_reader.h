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
#ifndef WX_READER_H
#define WX_READER_H

#include "video_event.h"
#include "ly_counter.h"

class WxReader : public VideoEvent {
	uint8_t wx_;
	uint8_t src_;
	
public:
	WxReader();
	
	void doEvent();
	
	uint8_t wx() const {
		return wx_;
	}
	
	void reset() {
		doEvent();
	}
	
	void setSource(const unsigned src) {
		src_ = src;
	}
	
	void schedule(const unsigned scxAnd7, const LyCounter &lyCounter, const unsigned cycleCounter) {
		setTime(lyCounter.nextLineCycle(scxAnd7 + 89 + lyCounter.isDoubleSpeed() * 3, cycleCounter));
	}
};

#endif
