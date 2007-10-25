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
#ifndef GAMBATTE_H
#define GAMBATTE_H

class CPU;

#include "videoblitter.h"
#include "inputstate.h"
#include "inputstategetter.h"
#include "filterinfo.h"
#include <vector>

class Gambatte {
	CPU *const z80;

public:
	Gambatte();
	~Gambatte();
	bool load(const char* romfile);
	void runFor(unsigned int cycles);
	void reset();
	void setVideoBlitter(VideoBlitter *vb);
	void videoBufferChange();
	unsigned int videoWidth() const;
	unsigned int videoHeight() const;
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32);
	
	void setVideoFilter(unsigned int n);
	std::vector<const FilterInfo*> filterInfo() const;
	void setInputStateGetter(InputStateGetter *getInput);

	void fill_buffer(uint16_t *stream, unsigned samples);
	
	void set_savedir(const char *sdir);
	bool isCgb() const;
};

#endif
