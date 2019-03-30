/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#pragma once

#include "../videolink.h"
#include "../vfilterinfo.h"
#include "array.h"
#include "gbint.h"

class MaxStHq3x : public VideoLink {
public:
	enum { out_width  = VfilterInfo::in_width  * 3 };
	enum { out_height = VfilterInfo::in_height * 3 };

	MaxStHq3x();
	virtual void * inBuf() const;
	virtual std::ptrdiff_t inPitch() const;
	virtual void draw(void *dst, std::ptrdiff_t dstpitch);

private:
	SimpleArray<gambatte::uint_least32_t> const buffer_;
};
