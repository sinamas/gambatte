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
#ifndef WE_H
#define WE_H

#include "video_event.h"
#include "ly_counter.h"

class We {
	class WeEnableChecker : public VideoEvent {
		bool &we;
		const bool &src;
		
	public:
		WeEnableChecker(bool &we_in, const bool &src_in);
		
		void doEvent();
		
		void schedule(const unsigned scxAnd7, const unsigned wx, const LyCounter &lyCounter, const unsigned cycleCounter) {
			setTime(lyCounter.nextLineCycle(scxAnd7 + 82 + wx + lyCounter.isDoubleSpeed() * 3, cycleCounter));
		}
	};
	
	class WeDisableChecker : public VideoEvent {
		bool &we;
		const bool &src;
		
	public:
		WeDisableChecker(bool &we_in, const bool &src_in);
		
		void doEvent();
		
		void schedule(const unsigned scxAnd7, const unsigned wx, const LyCounter &lyCounter, const unsigned cycleCounter) {
			setTime(lyCounter.nextLineCycle(scxAnd7 + 88 + wx + lyCounter.isDoubleSpeed() * 3, cycleCounter));
		}
	};
	
	
	WeEnableChecker enableChecker_;
	WeDisableChecker disableChecker_;
	
	bool we_;
	bool src_;
	
public:
	We();
	
	WeDisableChecker& disableChecker() {
		return disableChecker_;
	}
	
	WeEnableChecker& enableChecker() {
		return enableChecker_;
	}
	
	bool getSource() const {
		return src_;
	}
	
	void reset();
	
	void setSource(const bool src) {
		src_ = src;
	}
	
	bool value() const {
		return we_;
	}
};

#endif
