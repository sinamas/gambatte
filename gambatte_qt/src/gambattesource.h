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
#ifndef GAMBATTESOURCE_H
#define GAMBATTESOURCE_H

#include "mediasource.h"
#include <gambatte.h>
#include <cstring>
#include <QObject>

class GambatteSource : public QObject, public MediaSource {
	Q_OBJECT
	
	struct InputGetter : public Gambatte::InputStateGetter {
		Gambatte::InputState is;
		InputGetter() { std::memset(&is, 0, sizeof(is)); }
		const Gambatte::InputState& operator()() { return is; }
	};
	
	class Blitter : public Gambatte::VideoBlitter {
		GambatteSource &gs;
		
	public:
		Gambatte::PixelBuffer pb;
		
		Blitter(GambatteSource &gs) : gs(gs) {}
		void setBufferDimensions(unsigned /*width*/, unsigned /*height*/) {}
		const Gambatte::PixelBuffer inBuffer() { return pb; }
		void blit() { gs.emitBlit(); }
	};
	
	Gambatte::GB gb;
	InputGetter inputGetter;
	Blitter blitter;
	
public:
	GambatteSource();
	
	static const std::vector<ButtonInfo> generateButtonInfos();
	const std::vector<MediaSource::VideoSourceInfo> generateVideoSourceInfos();
	
	void emitBlit() { emit blit(); }
	
	bool load(const char* romfile) { return gb.load(romfile); }
	void reset() { gb.reset(); }
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) { gb.setDmgPaletteColor(palNum, colorNum, rgb32); }
	void setSavedir(const std::string &sdir) { gb.set_savedir(sdir.c_str()); }
	bool isCgb() const { return gb.isCgb(); }
	void selectState(int n) { gb.selectState(n); }
	int currentState() const { return gb.currentState(); }
	void saveState(const char *filepath) { gb.saveState(filepath); }
	void loadState(const char *filepath) { gb.loadState(filepath); }
	
	//overrides
	void buttonPressEvent(unsigned buttonIndex);
	void buttonReleaseEvent(unsigned buttonIndex);
	void setPixelBuffer(void *pixels, PixelFormat format, unsigned pitch);
	void setVideoSource(unsigned videoSourceIndex) { gb.setVideoFilter(videoSourceIndex); }
	unsigned update(qint16 *soundBuf, unsigned samples);
	
public slots:
	void saveState() { gb.saveState(); }
	void loadState() { gb.loadState(); }
	
signals:
	void blit();
	void setTurbo(bool on);
	void togglePause();
	void frameStep();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	void prevStateSlot();
	void nextStateSlot();
};

#endif
