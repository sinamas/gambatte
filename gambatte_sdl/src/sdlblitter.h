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
#ifndef SDLBLITTER_H
#define SDLBLITTER_H

#include <videoblitter.h>
#include <SDL.h>

class SdlBlitter : public VideoBlitter {
	SDL_Surface *screen;
	SDL_Surface *surface;
	SDL_Overlay *overlay;
	Uint32 startFlags;
	Uint8 scale;
	bool yuv;
	
	template<typename T> void swScale();
	
public:
	SdlBlitter(bool startFull = false, Uint8 scale = 1, bool yuv = false);
	~SdlBlitter();
	void setBufferDimensions(unsigned int width, unsigned int height);
	const PixelBuffer inBuffer();
	void blit();
	void toggleFullScreen();
	void setScale(const Uint8 scale) { if (!screen) this->scale = scale; }
	void setStartFull() { startFlags |= SDL_FULLSCREEN; }
	void setYuv(const bool yuv) { if (!screen) this->yuv = yuv; }
	void uninit();
// 	bool failed() const { return screen == NULL; }
};

#endif
