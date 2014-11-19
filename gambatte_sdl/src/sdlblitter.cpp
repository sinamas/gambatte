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

#include "sdlblitter.h"
#include "scalebuffer.h"
#include <SDL.h>

struct SdlBlitter::SurfaceDeleter {
	static void del(SDL_Surface *s) { SDL_FreeSurface(s); }
	static void del(SDL_Overlay *o) {
		if (o) {
			SDL_UnlockYUVOverlay(o);
			SDL_FreeYUVOverlay(o);
		}
	}
};

SdlBlitter::SdlBlitter(unsigned inwidth, unsigned inheight,
                       int scale, bool yuv, bool startFull)
: screen_(SDL_SetVideoMode(inwidth * scale, inheight * scale,
                           SDL_GetVideoInfo()->vfmt->BitsPerPixel == 16 ? 16 : 32,
                           SDL_SWSURFACE | (startFull ? SDL_FULLSCREEN : 0)))
, surface_(screen_ && scale > 1 && !yuv
           ? SDL_CreateRGBSurface(SDL_SWSURFACE, inwidth, inheight,
                                  screen_->format->BitsPerPixel, 0, 0, 0, 0)
           : 0)
, overlay_(screen_ && scale > 1 && yuv
           ? SDL_CreateYUVOverlay(inwidth * 2, inheight, SDL_UYVY_OVERLAY, screen_)
           : 0)
{
	if (overlay_)
		SDL_LockYUVOverlay(overlay_.get());
}

SdlBlitter::~SdlBlitter() {
}

SdlBlitter::PixelBuffer SdlBlitter::inBuffer() const {
	PixelBuffer pb = { 0, 0, RGB32 };

	if (overlay_) {
		pb.pixels = overlay_->pixels[0];
		pb.format = UYVY;
		pb.pitch = overlay_->pitches[0] >> 2;
	} else if (SDL_Surface *s = surface_ ? surface_.get() : screen_) {
		pb.pixels = static_cast<char *>(s->pixels) + s->offset;
		pb.format = s->format->BitsPerPixel == 16 ? RGB16 : RGB32;
		pb.pitch = s->pitch / s->format->BytesPerPixel;
	}

	return pb;
}

template<typename T>
inline void SdlBlitter::swScale() {
	T const *src = reinterpret_cast<T *>(static_cast<char *>(surface_->pixels) + surface_->offset);
	T       *dst = reinterpret_cast<T *>(static_cast<char *>(screen_->pixels) + screen_->offset);
	scaleBuffer(src, dst, surface_->w, surface_->h,
	            screen_->pitch / screen_->format->BytesPerPixel, screen_->h / surface_->h);
}

void SdlBlitter::draw() {
	if (surface_ && screen_) {
		if (surface_->format->BitsPerPixel == 16)
			swScale<Uint16>();
		else
			swScale<Uint32>();
	}
}

void SdlBlitter::present() {
	if (!screen_)
		return;

	if (overlay_) {
		SDL_Rect dstr = { 0, 0, Uint16(screen_->w), Uint16(screen_->h) };
		SDL_UnlockYUVOverlay(overlay_.get());
		SDL_DisplayYUVOverlay(overlay_.get(), &dstr);
		SDL_LockYUVOverlay(overlay_.get());
	} else {
		SDL_UpdateRect(screen_, 0, 0, screen_->w, screen_->h);
	}
}

void SdlBlitter::toggleFullScreen() {
	if (screen_) {
		screen_ = SDL_SetVideoMode(screen_->w, screen_->h, screen_->format->BitsPerPixel,
		                           screen_->flags ^ SDL_FULLSCREEN);
	}
}
