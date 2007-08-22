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
#include "wy.h"

#include "we_master_checker.h"

#include <cstdio>

Wy::WyReader1::WyReader1(uint8_t &wy, const uint8_t &src, const LyCounter &lyCounter, const WeMasterChecker &weMasterChecker) :
	VideoEvent(3),
	lyCounter(lyCounter),
	weMasterChecker(weMasterChecker),
	wy(wy),
	src(src)
{}

void Wy::WyReader1::doEvent() {
	if (src >= lyCounter.ly() && /*wy >= lyCounter.ly()*/ !weMasterChecker.weMaster()) {
		wy = src;
	}
	
	setTime(uint32_t(-1));
}

Wy::WyReader2::WyReader2(uint8_t &wy_in, const uint8_t &src_in, const LyCounter &lyCounter_in) :
	VideoEvent(4),
	lyCounter(lyCounter_in),
	wy(wy_in),
	src(src_in)
{}

void Wy::WyReader2::doEvent() {
	if (wy == lyCounter.ly() + 1 - lyCounter.isDoubleSpeed() && src > wy)
		wy = src;
	
	setTime(uint32_t(-1));
}

Wy::WyReader3::WyReader3(uint8_t &wy_in, const uint8_t &src_in, const LyCounter &lyCounter_in) :
	VideoEvent(5),
	lyCounter(lyCounter_in),
	wy(wy_in),
	src(src_in)
{}

void Wy::WyReader3::doEvent() {
	if (src == lyCounter.ly() && wy > lyCounter.ly())
		wy = src;
	
	setTime(uint32_t(-1));
}

Wy::WyReader4::WyReader4(uint8_t &wy_in, const uint8_t &src_in) :
	VideoEvent(6),
	wy(wy_in),
	src(src_in)
{}

void Wy::WyReader4::doEvent() {
	wy = src;
	
	setTime(uint32_t(-1));
}

Wy::Wy(const LyCounter &lyCounter, const WeMasterChecker &weMasterChecker) :
	reader1_(wy_, src_, lyCounter, weMasterChecker),
	reader2_(wy_, src_, lyCounter),
	reader3_(wy_, src_, lyCounter),
	reader4_(wy_, src_)
{
	setSource(0);
	reset();
}

void Wy::reset() {
	reader1_.doEvent();
	reader2_.doEvent();
	reader3_.doEvent();
	reader4_.doEvent();
}
