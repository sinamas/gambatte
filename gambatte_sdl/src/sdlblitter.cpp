/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#include "sdlblitter.h"

#include "scalebuffer.h"

SdlBlitter::SdlBlitter(const bool startFull, const Uint8 scale, const bool yuv) :
screen(NULL),
surface(NULL),
overlay(NULL),
startFlags(SDL_SWSURFACE | (startFull ? SDL_FULLSCREEN : 0)),
scale(scale),
yuv(yuv)
{}

SdlBlitter::~SdlBlitter() {
	if (overlay) {
		SDL_UnlockYUVOverlay(overlay);
		SDL_FreeYUVOverlay(overlay);
	}
	
	if (surface != screen)
		SDL_FreeSurface(surface);
}

void SdlBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	surface = screen = SDL_SetVideoMode(width * scale, height * scale, SDL_GetVideoInfo()->vfmt->BitsPerPixel == 16 ? 16 : 32, screen ? screen->flags : startFlags);
	
	if (scale > 1 && screen) {
		if (yuv) {
			if ((overlay = SDL_CreateYUVOverlay(width * 2, height, SDL_UYVY_OVERLAY, screen)))
				SDL_LockYUVOverlay(overlay);
		} else
			surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, screen->format->BitsPerPixel, 0, 0, 0, 0);
	}
}

const SdlBlitter::PixelBuffer SdlBlitter::inBuffer() const {
	PixelBuffer pb;
	
	if (overlay) {
		pb.pixels = overlay->pixels[0];
		pb.format = UYVY;
		pb.pitch = overlay->pitches[0] >> 2;
	} else if (surface) {
		pb.pixels = (Uint8*)(surface->pixels) + surface->offset;
		pb.format = surface->format->BitsPerPixel == 16 ? RGB16 : RGB32;
		pb.pitch = surface->pitch / surface->format->BytesPerPixel;
	}
	
	return pb;
}

template<typename T>
inline void SdlBlitter::swScale() {
	scaleBuffer<T>((T*)((Uint8*)(surface->pixels) + surface->offset), (T*)((Uint8*)(screen->pixels) + screen->offset), surface->w, surface->h, screen->pitch / screen->format->BytesPerPixel, scale);
}

void SdlBlitter::draw() {
	if (!screen || !surface)
		return;
		
	if (!overlay && surface != screen) {
		if (surface->format->BitsPerPixel == 16)
			swScale<Uint16>();
		else
			swScale<Uint32>();
	}
}

void SdlBlitter::present() {
	if (!screen || !surface)
		return;
		
	if (overlay) {
		SDL_Rect dstr = { 0, 0, screen->w, screen->h };
		SDL_UnlockYUVOverlay(overlay);
		SDL_DisplayYUVOverlay(overlay, &dstr);
		SDL_LockYUVOverlay(overlay);
	} else {
		SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
	}
}

void SdlBlitter::toggleFullScreen() {
	if (screen)
		screen = SDL_SetVideoMode(screen->w, screen->h, screen->format->BitsPerPixel, screen->flags ^ SDL_FULLSCREEN);
}
