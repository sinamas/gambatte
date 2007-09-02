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
#ifndef VIDEO_EVENT_H
#define VIDEO_EVENT_H

#include <stdint.h>

class VideoEvent {
	uint32_t time_;
	const uint8_t priority_;
	
protected:
	void setTime(const unsigned time_in) {
		time_ = time_in;
	}
	
public:
	VideoEvent(const unsigned priority_in) :
		priority_(priority_in)
	{}
	
	virtual ~VideoEvent() {}
	virtual void doEvent() = 0;
	
	unsigned priority() const {
		return priority_;
	}
	
	unsigned time() const {
		return time_;
	}
	
	friend class ScxReader;
	friend class WxReader;
};

#endif
