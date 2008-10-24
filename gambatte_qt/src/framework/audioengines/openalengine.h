/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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
#ifndef OPENALENGINE_H
#define OPENALENGINE_H

#include "../audioengine.h"

#ifdef Q_WS_MAC
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/alc.h>
#include <AL/al.h>
#endif

#include <QtGlobal>

template<typename T> class RingBuffer;

class OpenAlEngine : public AudioEngine {
	qint16 *buf;
	ALCdevice *device;
	ALCcontext *context;
	ALuint source;
	unsigned buffers;
	unsigned bufPos;
	
	void deleteProcessedBufs() const;
	int doInit(int rate, unsigned latency);
	
public:
	OpenAlEngine();
	~OpenAlEngine();
	void uninit();
	int write(void *buffer, unsigned samples);
	const BufferState bufferState() const;
};

#endif
