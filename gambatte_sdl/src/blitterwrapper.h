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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef BLITTERWRAPPER_H
#define BLITTERWRAPPER_H

#include "gbint.h"
#include "scoped_ptr.h"
#include "sdlblitter.h"
#include "videolink/videolink.h"

class VfilterInfo;
class VideoLink;

class BlitterWrapper {
public:
	struct Buf { gambatte::uint_least32_t *pixels; std::ptrdiff_t pitch; };

	BlitterWrapper(VfilterInfo const &, int scale, bool yuv, bool full);
	Buf const inBuf() const;
	void draw();
	void present() { blitter_.present(); }
	void toggleFullScreen() { blitter_.toggleFullScreen(); }

private:
	SdlBlitter blitter_;
	scoped_ptr<VideoLink> const cconvert_;
	scoped_ptr<VideoLink> const vfilter_;
};

#endif
