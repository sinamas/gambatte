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
#ifndef ALSAENGINE_H
#define ALSAENGINE_H

#include "../audioengine.h"
#include <alsa/asoundlib.h>

class AlsaEngine : public AudioEngine {
	snd_pcm_t *pcm_handle;
	unsigned bufSize;
	
public:
	AlsaEngine();
	~AlsaEngine();
	int init();
	void uninit();
	int write(void *buffer, unsigned samples);
	const BufferState bufferState() const;
};

#endif
