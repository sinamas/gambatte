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
#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

class AudioEngine {
public:
	struct BufferState {
		unsigned fromUnderrun;
		unsigned fromOverflow;
	};
	
	virtual ~AudioEngine() {}
	virtual int init() = 0;
	virtual void uninit() {}
	virtual int write(void *buffer, unsigned samples) = 0;
	virtual const BufferState bufferState() const { static const BufferState s = { 0xFFFFFFFF, 0xFFFFFFFF }; return s; }
	virtual void pause() {}
};

#endif
