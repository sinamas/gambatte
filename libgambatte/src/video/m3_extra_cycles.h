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
#ifndef VIDEO_M3_EXTRA_CYCLES_H
#define VIDEO_M3_EXTRA_CYCLES_H

class SpriteMapper;
class ScxReader;
class WeMasterChecker;
class Wy;
class We;
class WxReader;

class M3ExtraCycles {
	const SpriteMapper &spriteMapper;
	const ScxReader &scxReader;
	const WeMasterChecker &weMasterChecker;
	const Wy &wyReg;
	const We &we;
	const WxReader &wxReader;
	
public:
	M3ExtraCycles(const SpriteMapper &spriteMapper_in,
	              const ScxReader &scxReader_in,
	              const WeMasterChecker &weMasterChecker_in,
	              const Wy &wyReg_in,
	              const We &we_in,
	              const WxReader &wxReader_in);
	
	unsigned operator()(unsigned ly) const;
};

#endif
