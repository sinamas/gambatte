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
#ifndef PIXELBUFFER_H
#define PIXELBUFFER_H

struct PixelBuffer {
	/**
	  * @enum RGB32 Native endian RGB with 8 bits pr color. rmask: 0xff0000, gmask: 0x00ff00, bmask: 0x0000ff
	  *
	  * @enum RGB16 Native endian RGB with 5. 6. and 5 bits for red, green and blue respectively.
	  *             rmask: 0xf800, gmask: 0x07e0 , bmask: 0x001f
	  *
	  * @enum UYVY Big endian UYVY, 8 bits pr field. Normally two horizontal neighbour pixels share U and V,
	  *            but this expects video at 2x width to avoid chroma loss. One pixel is made up of
	  *            U, Y, V and Y (the same value) again for a total of 32 bits pr pixel.
	  *            umask: 0xff000000, ymask: 0x00ff00ff, vmask: 0x0000ff00 (big endian)
	  *            umask: 0x000000ff, ymask: 0xff00ff00, vmask: 0x00ff0000 (little endian)
	  */
	enum PixelFormat { RGB32, RGB16, UYVY };
	
	void *data;
	int pitch; // number of pixels (not bytes) between line N and line N+1
	unsigned width, height;
	PixelFormat pixelFormat;
	explicit PixelBuffer(unsigned width = 0, unsigned height = 0, PixelFormat pixelFormat = RGB32, void *data = 0, int pitch = 0)
	: data(data), pitch(pitch), width(width), height(height), pixelFormat(pixelFormat)
	{
	}
};

#endif
