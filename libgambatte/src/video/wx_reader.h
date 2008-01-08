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

template<typename T, class Comparer> class event_queue;
class M3ExtraCycles;

#include "video_event.h"
#include "video_event_comparer.h"
#include "ly_counter.h"

class WxReader : public VideoEvent {
	event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue;
	VideoEvent &weEnableChecker;
	VideoEvent &weDisableChecker;
	M3ExtraCycles &m3ExtraCycles;

	unsigned char wx_;
	unsigned char src_;
	bool dS;
	
	void rescheduleEvent(VideoEvent& event, unsigned long diff);
	
public:
	WxReader(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
	         VideoEvent &weEnableChecker_in,
	         VideoEvent &weDisableChecker_in,
	         M3ExtraCycles &m3ExtraCycles);
	
	void doEvent();
	
	unsigned getSource() const {
		return src_;
	}
	
	unsigned char wx() const {
		return wx_;
	}
	
	void reset() {
		doEvent();
	}
	
	void setDoubleSpeed(const bool dS_in) {
		dS = dS_in;
	}
	
	void setSource(const unsigned src) {
		src_ = src;
	}
	
	void schedule(const unsigned scxAnd7, const LyCounter &lyCounter, const unsigned long cycleCounter) {
		setTime(lyCounter.nextLineCycle(scxAnd7 + 82 + lyCounter.isDoubleSpeed() * 3 + (src_ < wx_ ? src_ : wx_), cycleCounter));
		//setTime(lyCounter.nextLineCycle(scxAnd7 + 89 + lyCounter.isDoubleSpeed() * 3, cycleCounter));
	}
};

void addEvent(WxReader &event, unsigned scxAnd7, const LyCounter &lyCounter,
		unsigned long cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
