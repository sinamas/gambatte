/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#ifndef BLITTERWRAPPER_H
#define BLITTERWRAPPER_H

#include "sdlblitter.h"
#include "gbint.h"
#include "videolink/videolink.h"
#include <memory>

class BlitterWrapper {
	SdlBlitter blitter;
	std::auto_ptr<VideoLink> cconvert;
	std::auto_ptr<VideoLink> vfilter;
	unsigned vsrci;
	
public:
	struct Buf { gambatte::uint_least32_t* pixels; unsigned pitch; };
	BlitterWrapper() : vsrci(0) {}
	const Buf inBuf() const;
	void draw();
	void present() { blitter.present(); }
	void toggleFullScreen() { blitter.toggleFullScreen(); }
	void setScale(const Uint8 scale) { blitter.setScale(scale); }
	void setStartFull() { blitter.setStartFull(); }
	void setYuv(const bool yuv) { blitter.setYuv(yuv); }
	void setVideoFilter(unsigned n) { vsrci = n; }
	
	void init();
};

#endif
