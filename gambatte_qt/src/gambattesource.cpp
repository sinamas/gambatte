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
#include "gambattesource.h"

GambatteSource::GambatteSource() :
blitter(*this),
sampleBuffer(NULL) {
	gb.setInputStateGetter(&inputGetter);
	gb.setVideoBlitter(&blitter);
}

const std::vector<std::string> GambatteSource::generateButtonLabels() {
	std::vector<std::string> v(9);
	
	v[0] = "Up";
	v[1] = "Down";
	v[2] = "Left";
	v[3] = "Right";
	v[4] = "A";
	v[5] = "B";
	v[6] = "Start";
	v[7] = "Select";
	v[8] = "Fast forward";
	
	return v;
}

const std::vector<int> GambatteSource::generateButtonDefaults() {
	std::vector<int> v(9);
	
	v[0] = Qt::Key_Up;
	v[1] = Qt::Key_Down;
	v[2] = Qt::Key_Left;
	v[3] = Qt::Key_Right;
	v[4] = Qt::Key_D;
	v[5] = Qt::Key_C;
	v[6] = Qt::Key_Return;
	v[7] = Qt::Key_Shift;
	v[8] = Qt::Key_Tab;
	
	return v;
}

const std::vector<MediaSource::VideoSourceInfo> GambatteSource::generateVideoSourceInfos() {
	const std::vector<const Gambatte::FilterInfo*> &fi = gb.filterInfo();
	std::vector<MediaSource::VideoSourceInfo> v(fi.size());
	
	for (unsigned i = 0; i < fi.size(); ++i) {
		const MediaSource::VideoSourceInfo vsi = { handle: fi[i]->handle, width: fi[i]->outWidth, height: fi[i]->outHeight };
		
		v[i] = vsi;
	}
	
	return v;
}

const std::vector<int> GambatteSource::generateSampleRates() {
	std::vector<int> v(2);
	
	v[0] = 48000;
	v[1] = 44100;
	
	return v;
}

void GambatteSource::buttonPressEvent(unsigned buttonIndex) {
	switch (buttonIndex) {
	case 0: inputGetter.is.dpadUp = true; inputGetter.is.dpadDown = false; break;
	case 1: inputGetter.is.dpadDown = true; inputGetter.is.dpadUp = false; break;
	case 2: inputGetter.is.dpadLeft = true; inputGetter.is.dpadRight = false; break;
	case 3: inputGetter.is.dpadRight = true; inputGetter.is.dpadLeft = false; break;
	case 4: inputGetter.is.aButton = true; break;
	case 5: inputGetter.is.bButton = true; break;
	case 6: inputGetter.is.startButton = true; break;
	case 7: inputGetter.is.selectButton = true; break;
	case 8: emit setTurbo(true); break;
	}
}

void GambatteSource::buttonReleaseEvent(unsigned buttonIndex) {
	switch (buttonIndex) {
	case 0: inputGetter.is.dpadUp = false; break;
	case 1: inputGetter.is.dpadDown = false; break;
	case 2: inputGetter.is.dpadLeft = false; break;
	case 3: inputGetter.is.dpadRight = false; break;
	case 4: inputGetter.is.aButton = false; break;
	case 5: inputGetter.is.bButton = false; break;
	case 6: inputGetter.is.startButton = false; break;
	case 7: inputGetter.is.selectButton = false; break;
	case 8: emit setTurbo(false); break;
	}
}

void GambatteSource::setPixelBuffer(void *pixels, PixelFormat format, unsigned pitch) {
	blitter.pb.pixels = pixels;
	blitter.pb.pitch = pitch;
	
	switch (format) {
	case RGB32: blitter.pb.format = Gambatte::PixelBuffer::RGB32; break;
	case RGB16: blitter.pb.format = Gambatte::PixelBuffer::RGB16; break;
	case UYVY: blitter.pb.format = Gambatte::PixelBuffer::UYVY; break;
	}
	
	gb.videoBufferChange();
}

void GambatteSource::update(const unsigned samples) {
	gb.runFor(70224);
	gb.fill_buffer(reinterpret_cast<quint16*>(sampleBuffer), samples);
}
