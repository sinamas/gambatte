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
#ifndef VIDEOLINK_H
#define VIDEOLINK_H

class VideoLink {
	void *const inbuf;
	const unsigned inpitch;
protected:
	VideoLink(void *inbuf, unsigned inpitch) : inbuf(inbuf), inpitch(inpitch) {}
public:
	void* inBuf() const { return inbuf; }
	unsigned inPitch() const { return inpitch; }
	virtual ~VideoLink() {}
	virtual void draw(void *dst, unsigned dstpitch) = 0;
};

#endif
