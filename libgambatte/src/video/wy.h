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
#ifndef WY_H
#define WY_H

class WeMasterChecker;
class ScxReader;
template<typename T, class Comparer> class event_queue;

#include "video_event.h"
#include "video_event_comparer.h"
#include "ly_counter.h"
#include "m3_extra_cycles.h"

class Wy {
public:
	class WyReader1 : public VideoEvent {
		Wy &wy;
		const WeMasterChecker &weMasterChecker;
		
	public:
		WyReader1(Wy &wy, const WeMasterChecker &weMasterChecker);
		
		void doEvent();
		
		void schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
			setTime(lyCounter.nextLineCycle(448 + lyCounter.isDoubleSpeed() * 4, cycleCounter));
		}
	};
	
	class WyReader2 : public VideoEvent {
		Wy &wy;
		
	public:
		WyReader2(Wy &wy);
		
		void doEvent();
		
		void schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
			setTime(lyCounter.isDoubleSpeed() ? lyCounter.time() : lyCounter.nextLineCycle(452, cycleCounter));
		}
	};
	
	class WyReader3 : public VideoEvent {
		Wy &wy;
		
	public:
		WyReader3(Wy &wy);
		
		void doEvent();
		void schedule(unsigned wxSrc, const ScxReader &scxReader, unsigned long cycleCounter);
		
		//void schedule(const unsigned scxAnd7, const LyCounter &lyCounter, const unsigned cycleCounter) {
		//	setTime(lyCounter.nextLineCycle(scxAnd7 + 85 + lyCounter.isDoubleSpeed() * 6, cycleCounter));
		//}
	};
	
	class WyReader4 : public VideoEvent {
		Wy &wy;
		
	public:
		WyReader4(Wy &wy);
		
		void doEvent();
		
		void schedule(const LyCounter &lyCounter, const unsigned long cycleCounter) {
			setTime(lyCounter.nextFrameCycle(lyCounter.isDoubleSpeed() * 4, cycleCounter));
		}
	};
	
	friend class WyReader1;
	friend class WyReader2;
	friend class WyReader3;
	friend class WyReader4;
	
private:
	const LyCounter &lyCounter;
	M3ExtraCycles &m3ExtraCycles;
	WyReader1 reader1_;
	WyReader2 reader2_;
	WyReader3 reader3_;
	WyReader4 reader4_;
	
	unsigned char wy_;
	unsigned char src_;
	
	void set(const unsigned char value) {
		if (wy_ != value)
			m3ExtraCycles.invalidateCache();
		
		wy_ = value;
	}
	
public:
	Wy(const LyCounter &lyCounter, const WeMasterChecker &weMasterChecker, M3ExtraCycles &m3ExtraCycles);
	
	WyReader1& reader1() {
		return reader1_;
	}
	
	WyReader2& reader2() {
		return reader2_;
	}
	
	WyReader3& reader3() {
		return reader3_;
	}
	
	WyReader4& reader4() {
		return reader4_;
	}
	
	unsigned getSource() const {
		return src_;
	}
	
	void reset();
	
	void setSource(const unsigned src) {
		src_ = src;
	}
	
	//void setValue(const unsigned val) {
	//	wy_ = val;
	//}
	
	unsigned value() const {
		return wy_;
	}
	
	void weirdAssWeMasterEnableOnWyLineCase() {
		set(wy_ + 1);
	}
};

void addEvent(Wy::WyReader3 &event, unsigned wxSrc, const ScxReader &scxReader,
		unsigned long cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
