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
#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <ctime>

class Rtc {
private:
	uint8_t *activeData;
	void (Rtc::*activeSet)(uint8_t);
	std::time_t baseTime;
	std::time_t haltTime;
	uint8_t index;
	uint8_t dataDh;
	uint8_t dataDl;
	uint8_t dataH;
	uint8_t dataM;
	uint8_t dataS;
	bool enabled;
	bool lastLatchData;
	
	void doLatch();
	void doSwapActive();
	void setDh(uint8_t new_dh);
	void setDl(uint8_t new_lowdays);
	void setH(uint8_t new_hours);
	void setM(uint8_t new_minutes);
	void setS(uint8_t new_seconds);
	
public:
	Rtc();
	
	const uint8_t* getActive() const {
		return activeData;
	}
	
	std::time_t getBaseTime() const {
		return baseTime;
	}
	
	void setBaseTime(const std::time_t baseTime) {
		this->baseTime = baseTime;
		doLatch();
	}
	
	void latch(const uint8_t data) {
		if (!lastLatchData && data == 1)
			doLatch();
		
		lastLatchData = data;
	}
	
	void reset();
	
	void setEnabled(const bool enabled) {
		this->enabled = enabled;
		
		doSwapActive();
	}
	
	void swapActive(uint8_t index) {
		index &= 0xF;
		index -= 8;
		
		this->index = index;
		
		doSwapActive();
	}
	
	void write(const uint8_t data) {
// 		if (activeSet)
		(this->*activeSet)(data);
		*activeData = data;
	}
};

#endif
