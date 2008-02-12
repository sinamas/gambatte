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
	qint16 *sampleBuffer;
	
public:
	GambatteSource();
	
	static const std::vector<std::string> generateButtonLabels();
	static const std::vector<int> generateButtonDefaults();
	const std::vector<MediaSource::VideoSourceInfo> generateVideoSourceInfos();
	static const std::vector<int> generateSampleRates();
	
	void emitBlit() { emit blit(); }
	
	bool load(const char* romfile) { return gb.load(romfile); }
	void reset() { gb.reset(); }
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) { gb.setDmgPaletteColor(palNum, colorNum, rgb32); }
	void setSavedir(const std::string &sdir) { gb.set_savedir(sdir.c_str()); }
	bool isCgb() const { return gb.isCgb(); }
	
	//overrides
	void buttonPressEvent(unsigned buttonIndex);
	void buttonReleaseEvent(unsigned buttonIndex);
	void setPixelBuffer(void *pixels, PixelFormat format, unsigned pitch);
	void setSampleBuffer(qint16 *sampleBuffer) { this->sampleBuffer = sampleBuffer; }
	void setVideoSource(unsigned videoSourceIndex) { gb.setVideoFilter(videoSourceIndex); }
	void update(unsigned samples);
	
signals:
	void blit();
	void toggleTurbo();
};

#endif
