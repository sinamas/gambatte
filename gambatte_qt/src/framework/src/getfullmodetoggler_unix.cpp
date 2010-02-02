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
#include "getfullmodetoggler.h"

#include "fullmodetogglers/nulltoggler.h"
// #include "fullmodetogglers/xf86vidmodetoggler.h"
#include "fullmodetogglers/xrandrtoggler.h"
#include "fullmodetogglers/xrandr12toggler.h"

std::auto_ptr<FullModeToggler> getFullModeToggler(WId /*winId*/) {
	if (XRandR12Toggler::isUsable())
		return std::auto_ptr<FullModeToggler>(new XRandR12Toggler);
	
	if (XRandRToggler::isUsable())
		return std::auto_ptr<FullModeToggler>(new XRandRToggler);
	
// 	if (Xf86VidModeToggler::isUsable())
// 		return std::auto_ptr<FullModeToggler>(new Xf86VidModeToggler(winId));
	
	return std::auto_ptr<FullModeToggler>(new NullToggler);
}
