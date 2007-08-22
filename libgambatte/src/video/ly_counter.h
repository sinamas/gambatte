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
#ifndef LY_COUNTER_H
#define LY_COUNTER_H

#include <stdint.h>
#include "video_event.h"

class LyCounter : public VideoEvent {
	uint32_t lineTime_;
	uint8_t ly_;
	bool ds;
	
public:
	LyCounter();
	
	void doEvent();
	
	bool isDoubleSpeed() const {
		return ds;
	}
	
	unsigned lineTime() const {
		return lineTime_;
	}
	
	unsigned ly() const {
		return ly_;
	}
	
	unsigned nextLineCycle(unsigned lineCycle, unsigned cycleCounter) const;
	unsigned nextFrameCycle(unsigned frameCycle, unsigned cycleCounter) const;
	
	void resetLy() {
		ly_ = 0;
	}
	
	void setDoubleSpeed(bool ds_in);
	
	void setTime(const unsigned int time) {
		VideoEvent::setTime(time);
	}
};

#endif
