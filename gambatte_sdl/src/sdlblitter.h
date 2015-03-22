//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef SDLBLITTER_H
#define SDLBLITTER_H

#include "scoped_ptr.h"
#include <cstddef>

struct SDL_Overlay;
struct SDL_Surface;

class SdlBlitter {
public:
	enum PixelFormat { RGB32, RGB16, UYVY };

	struct PixelBuffer {
		void *pixels;
		std::ptrdiff_t pitch;
		PixelFormat format;
	};

	SdlBlitter(unsigned inwidth, unsigned inheight,
	           int scale, bool yuv, bool full);
	~SdlBlitter();
	PixelBuffer inBuffer() const;
	void draw();
	void present();
	void toggleFullScreen();

private:
	struct SurfaceDeleter;

	SDL_Surface *screen_;
	scoped_ptr<SDL_Surface, SurfaceDeleter> const surface_;
	scoped_ptr<SDL_Overlay, SurfaceDeleter> const overlay_;

	template<typename T> void swScale();
};

#endif
