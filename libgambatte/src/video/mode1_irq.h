/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#ifndef VIDEO_MODE1_IRQ_H
#define VIDEO_MODE1_IRQ_H

#include "ly_counter.h"

class Mode1Irq : public VideoEvent {
	unsigned char &ifReg;
	unsigned long frameTime;
	unsigned char flags;
	
public:
	Mode1Irq(unsigned char &ifReg_in);
	
	void doEvent();
	
	void reset() {
		setTime(DISABLED_TIME);
	}
	
	void schedule(const LyCounter &lyCounter, unsigned long cycleCounter);
	
	void setDoubleSpeed(const bool ds) {
		frameTime = 70224 << ds;
	}
	
	void setM1StatIrqEnabled(const bool enabled) {
		flags = enabled * 2 | 1;
	}
};

#endif
