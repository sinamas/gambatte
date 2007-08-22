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
#ifndef VIDEO_LYC_IRQ_H
#define VIDEO_LYC_IRQ_H

#include <stdint.h>
#include "ly_counter.h"

class LycIrq : public VideoEvent {
	uint8_t &ifReg;
	
	uint32_t frameTime;
	
	uint8_t lycReg_;
	bool m2IrqEnabled;
	
public:
	LycIrq(uint8_t &ifReg_in);
	
	void doEvent();
	
	unsigned lycReg() const {
		return lycReg_;
	}
	
	void lycRegSchedule(const LyCounter &lyCounter, unsigned cycleCounter);
	
	void reset() {
		setTime(uint32_t(-1));
	}
	
	void schedule(const LyCounter &lyCounter, unsigned cycleCounter);
	
	void setDoubleSpeed(const bool ds) {
		frameTime = 70224 << ds;
	}
	
	void setLycReg(const unsigned lycReg_in) {
		lycReg_ = lycReg_in;
	}
	
	void setM2IrqEnabled(const bool enabled) {
		m2IrqEnabled = enabled;
	}
};

#endif
