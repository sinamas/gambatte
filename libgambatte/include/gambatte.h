/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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

#include "inputgetter.h"
#include "int.h"

namespace Gambatte {
enum { BG_PALETTE = 0, SP1_PALETTE = 1, SP2_PALETTE = 2 };

class GB {
	CPU *const z80;
	int stateNo;

	void loadState(const char *filepath, bool osdMessage);

public:
	GB();
	~GB();
	bool load(const char* romfile, bool forceDmg = false);
	
	/** Emulates until at least 'samples' stereo sound samples are produced in the supplied buffer,
	  * or until a video frame has been drawn.
	  *
	  * There are 35112 stereo sound samples in a video frame.
	  * May run for uptil 2064 stereo samples too long.
	  * A stereo sample consists of two native endian 2s complement 16-bit PCM samples,
	  * with the left sample preceding the right one. Usually casting soundBuf to/from
	  * short* is OK and recommended. The reason for not using a short* in the interface
	  * is to avoid implementation-defined behaviour without compromising performance.
	  *
	  * Returns early when a new video frame has finished drawing in the video buffer,
	  * such that the caller may update the video output before the frame is overwritten.
	  * The return value indicates whether a new video frame has been drawn, and the
	  * exact time (in number of samples) at which it was drawn.
	  *
	  * @param videoBuf 160x144 RGB32 (native endian) video frame buffer or 0
	  * @param pitch distance in number of pixels (not bytes) from the start of one line to the next in videoBuf.
	  * @param soundBuf buffer with space >= samples + 2064
	  * @param samples in: number of stereo samples to produce, out: actual number of samples produced
	  * @return sample number at which the video frame was produced. -1 means no frame was produced.
	  */
	int runFor(Gambatte::uint_least32_t *videoBuf, unsigned pitch,
			Gambatte::uint_least32_t *soundBuf, unsigned &samples);
	
	void reset();
	
	/** @param palNum 0 <= palNum < 3. One of BG_PALETTE, SP1_PALETTE and SP2_PALETTE.
	  * @param colorNum 0 <= colorNum < 4
	  */
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32);
	
	void setInputGetter(InputGetter *getInput);
	
	void setSaveDir(const char *sdir);
	bool isCgb() const;
	void saveState(const Gambatte::uint_least32_t *videoBuf, unsigned pitch);
	void loadState();
	void saveState(const Gambatte::uint_least32_t *videoBuf, unsigned pitch, const char *filepath);
	void loadState(const char *filepath);
	void selectState(int n);
	int currentState() const { return stateNo; }
};
}

#endif
