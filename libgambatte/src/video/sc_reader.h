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
#ifndef SC_READER_H
#define SC_READER_H

template<typename T, class Comparer> class event_queue;

#include "video_event.h"
#include "video_event_comparer.h"

class ScReader : public VideoEvent {
	unsigned char scx_[2];
	unsigned char scy_[2];
	
	unsigned char scxSrc;
	unsigned char scySrc;
	unsigned char incCycles;
	bool dS;
	
public:
	ScReader();
	
	void doEvent();
	
	void reset();
	
	unsigned scx() const {
		return scx_[0] & ~0x7 | scxSrc & 0x7;
	}
	
	unsigned scy() const {
		return scy_[0];
	}
	
	void schedule(unsigned long lastUpdate, unsigned long videoCycles, unsigned scReadOffset);
	
	void setDoubleSpeed(bool dS_in);
	
	void setScxSource(const unsigned scxSrc_in) {
		scxSrc = scxSrc_in;
	}
	
	void setScySource(const unsigned scySrc_in) {
		scySrc = scySrc_in;
	}
};

void addEvent(ScReader &event, unsigned long lastUpdate, unsigned long videoCycles,
              unsigned scReadOffset, event_queue<VideoEvent*,VideoEventComparer> &queue);

#endif
