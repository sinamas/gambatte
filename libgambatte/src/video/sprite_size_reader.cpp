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
#include "sprite_size_reader.h"

#include <stdint.h>
#include "sprite_mapper.h"
#include "basic_add_event.h"

SpriteSizeReader::SpriteSizeReader(event_queue<VideoEvent*,VideoEventComparer> &m3EventQueue_in,
                                   SpriteMapper &spriteMapper_in, const LyCounter &lyCounter_in) :
	VideoEvent(0),
	m3EventQueue(m3EventQueue_in),
	spriteMapper(spriteMapper_in),
	lyCounter(lyCounter_in)
{
	setSource(false);
	reset();
}

void SpriteSizeReader::doEvent() {
	largeSprites_ = src_;
	
	addEvent(spriteMapper, lyCounter, time(), m3EventQueue);
	
	setTime(uint32_t(-1));
}
