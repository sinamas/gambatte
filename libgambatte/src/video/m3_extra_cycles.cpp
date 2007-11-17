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
#include "m3_extra_cycles.h"

#include "../video.h"

M3ExtraCycles::M3ExtraCycles(const LCD &video) :
	video(video)
{}

unsigned M3ExtraCycles::operator()(const unsigned ly) const {
	unsigned cycles = video.spriteMapper.spriteCycles(ly);
	
	cycles += video.scxReader.scxAnd7();
	
	if (video.we.value() && video.wxReader.wx() < 0xA7 && ly >= video.wyReg.value() && (video.weMasterChecker.weMaster() || ly == video.wyReg.value()))
		cycles += 6;
	
	return cycles;
}
