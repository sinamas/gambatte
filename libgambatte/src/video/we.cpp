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
#include "we.h"

#include <stdint.h>

We::WeEnableChecker::WeEnableChecker(bool &we_in, const bool &src_in) :
	VideoEvent(8),
	we(we_in),
	src(src_in)
{}

void We::WeEnableChecker::doEvent() {
	we = src;
	
	setTime(uint32_t(-1));
}

We::WeDisableChecker::WeDisableChecker(bool &we_in, const bool &src_in) :
	VideoEvent(9),
	we(we_in),
	src(src_in)
{}

void We::WeDisableChecker::doEvent() {
	we = we && src;
	
	setTime(uint32_t(-1));
}

We::We() :
	enableChecker_(we_, src_),
	disableChecker_(we_, src_)
{
	setSource(false);
	reset();
}

void We::reset() {
	enableChecker_.doEvent();
	disableChecker_.doEvent();
}
