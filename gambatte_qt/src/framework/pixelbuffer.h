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
	enum PixelFormat { RGB32, RGB16, UYVY };
	
	void *data;
	int pitch; // number of pixels (not bytes) between line N and line N+1
	unsigned width, height;
	PixelFormat pixelFormat;
	PixelBuffer(unsigned width = 0, unsigned height = 0, PixelFormat pixelFormat = RGB32, void *data = 0, int pitch = 0) :
			data(data), pitch(pitch), width(width), height(height), pixelFormat(pixelFormat) {}
};

#endif
