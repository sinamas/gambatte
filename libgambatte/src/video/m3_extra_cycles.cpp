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

#include "sprite_mapper.h"
#include "scx_reader.h"
#include "we_master_checker.h"
#include "wy.h"
#include "we.h"
#include "wx_reader.h"

M3ExtraCycles::M3ExtraCycles(const SpriteMapper &spriteMapper_in,
                             const ScxReader &scxReader_in,
                             const WeMasterChecker &weMasterChecker_in,
                             const Wy &wyReg_in,
                             const We &we_in,
                             const WxReader &wxReader_in) :
	spriteMapper(spriteMapper_in),
	scxReader(scxReader_in),
	weMasterChecker(weMasterChecker_in),
	wyReg(wyReg_in),
	we(we_in),
	wxReader(wxReader_in)
{}

unsigned M3ExtraCycles::operator()(const unsigned ly) const {
	unsigned cycles = spriteMapper.spriteMap()[ly * 12 + 11];
	
	cycles += scxReader.scxAnd7();
	
	if ((weMasterChecker.weMaster() || ly == wyReg.value()) && we.value() && wyReg.value() <= ly && wxReader.wx() < 0xA7)
		cycles += 6;
	
	return cycles;
}
