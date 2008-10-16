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
#include "gambattesource.h"

GambatteSource::GambatteSource() :
MediaSource(2064),
blitter(*this) {
	gb.setInputStateGetter(&inputGetter);
	gb.setVideoBlitter(&blitter);
}

static const MediaSource::ButtonInfo constructButtonInfo(const char *label, const char *category, int defaultKey = Qt::Key_unknown, int defaultAltKey = Qt::Key_unknown) {
	MediaSource::ButtonInfo bi = { label: label,  category: category, defaultKey: defaultKey, defaultAltKey: defaultAltKey};
	
	return bi;
}

const std::vector<MediaSource::VideoSourceInfo> GambatteSource::generateVideoSourceInfos() {
	const std::vector<const Gambatte::FilterInfo*> &fi = gb.filterInfo();
	std::vector<MediaSource::VideoSourceInfo> v(fi.size());
	
	for (unsigned i = 0; i < fi.size(); ++i) {
		const MediaSource::VideoSourceInfo vsi = { label: fi[i]->handle.c_str(), width: fi[i]->outWidth, height: fi[i]->outHeight };
		
		v[i] = vsi;
	}
	
	return v;
}

const MediaSource::SampleRateInfo GambatteSource::generateSampleRateInfo() {
	SampleRateInfo srinfo;
	
	srinfo.rates.push_back(48000);
	srinfo.rates.push_back(44100);
	srinfo.defaultRateIndex = 0;
	srinfo.minCustomRate = 8000;
	srinfo.maxCustomRate = 192000;
	
	return srinfo;
}

enum {
	UP_BUTTON, DOWN_BUTTON, LEFT_BUTTON, RIGHT_BUTTON,
	A_BUTTON, B_BUTTON, START_BUTTON, SELECT_BUTTON,
	PAUSE_BUTTON, FRAME_STEP_BUTTON, DECREASE_FRAME_RATE_BUTTON, INCREASE_FRAME_RATE_BUTTON,
	RESET_FRAME_RATE_BUTTON, FAST_FORWARD_BUTTON, SAVE_STATE_BUTTON, LOAD_STATE_BUTTON,
	PREVIOUS_STATE_SLOT_BUTTON, NEXT_STATE_SLOT_BUTTON,
		
	NUMBER_OF_BUTTONS
};

const std::vector<MediaSource::ButtonInfo> GambatteSource::generateButtonInfos() {
	std::vector<MediaSource::ButtonInfo> v(NUMBER_OF_BUTTONS);
	
	v[UP_BUTTON]                   = constructButtonInfo("Up",                  "Game",  Qt::Key_Up);
	v[DOWN_BUTTON]                 = constructButtonInfo("Down",                "Game",  Qt::Key_Down);
	v[LEFT_BUTTON]                 = constructButtonInfo("Left",                "Game",  Qt::Key_Left);
	v[RIGHT_BUTTON]                = constructButtonInfo("Right",               "Game",  Qt::Key_Right);
	v[A_BUTTON]                    = constructButtonInfo("A",                   "Game",  Qt::Key_D);
	v[B_BUTTON]                    = constructButtonInfo("B",                   "Game",  Qt::Key_C);
	v[START_BUTTON]                = constructButtonInfo("Start",               "Game",  Qt::Key_Return);
	v[SELECT_BUTTON]               = constructButtonInfo("Select",              "Game",  Qt::Key_Shift);
	v[PAUSE_BUTTON]                = constructButtonInfo("Pause",               "Play",  Qt::Key_Pause);
	v[FRAME_STEP_BUTTON]           = constructButtonInfo("Frame step",          "Play",  Qt::Key_F1);
	v[DECREASE_FRAME_RATE_BUTTON]  = constructButtonInfo("Decrease frame rate", "Play",  Qt::Key_F2);
	v[INCREASE_FRAME_RATE_BUTTON]  = constructButtonInfo("Increase frame rate", "Play",  Qt::Key_F3);
	v[RESET_FRAME_RATE_BUTTON]     = constructButtonInfo("Reset frame rate",    "Play",  Qt::Key_F4);
	v[FAST_FORWARD_BUTTON]         = constructButtonInfo("Fast forward",        "Play",  Qt::Key_Tab);
	v[SAVE_STATE_BUTTON]           = constructButtonInfo("Save state",          "State", Qt::Key_F5);
	v[LOAD_STATE_BUTTON]           = constructButtonInfo("Load state",          "State", Qt::Key_F8);
	v[PREVIOUS_STATE_SLOT_BUTTON]  = constructButtonInfo("Previous state slot", "State", Qt::Key_F6);
	v[NEXT_STATE_SLOT_BUTTON]      = constructButtonInfo("Next state slot",     "State", Qt::Key_F7);
	
	return v;
}

void GambatteSource::buttonPressEvent(unsigned buttonIndex) {
	switch (buttonIndex) {
	case UP_BUTTON:
		inputGetter.is.dpadUp = true; inputGetter.is.dpadDown = false; break;
	case DOWN_BUTTON:
		inputGetter.is.dpadDown = true; inputGetter.is.dpadUp = false; break;
	case LEFT_BUTTON:
		inputGetter.is.dpadLeft = true; inputGetter.is.dpadRight = false; break;
	case RIGHT_BUTTON:
		inputGetter.is.dpadRight = true; inputGetter.is.dpadLeft = false; break;
	case A_BUTTON:
		inputGetter.is.aButton = true; break;
	case B_BUTTON:
		inputGetter.is.bButton = true; break;
	case START_BUTTON:
		inputGetter.is.startButton = true; break;
	case SELECT_BUTTON:
		inputGetter.is.selectButton = true; break;
	case PAUSE_BUTTON:
		emit togglePause(); break;
	case FRAME_STEP_BUTTON:
		emit frameStep(); break;
	case DECREASE_FRAME_RATE_BUTTON:
		emit decFrameRate(); break;
	case INCREASE_FRAME_RATE_BUTTON:
		emit incFrameRate(); break;
	case RESET_FRAME_RATE_BUTTON:
		emit resetFrameRate(); break;
	case FAST_FORWARD_BUTTON:
		emit setTurbo(true); break;
	case SAVE_STATE_BUTTON:
		saveState(); break;
	case LOAD_STATE_BUTTON:
		loadState(); break;
	case PREVIOUS_STATE_SLOT_BUTTON:
		emit prevStateSlot(); break;
	case NEXT_STATE_SLOT_BUTTON:
		emit nextStateSlot(); break;
	}
}

void GambatteSource::buttonReleaseEvent(unsigned buttonIndex) {
	switch (buttonIndex) {
	case UP_BUTTON:
		inputGetter.is.dpadUp = false; break;
	case DOWN_BUTTON:
		inputGetter.is.dpadDown = false; break;
	case LEFT_BUTTON:
		inputGetter.is.dpadLeft = false; break;
	case RIGHT_BUTTON:
		inputGetter.is.dpadRight = false; break;
	case A_BUTTON:
		inputGetter.is.aButton = false; break;
	case B_BUTTON:
		inputGetter.is.bButton = false; break;
	case START_BUTTON:
		inputGetter.is.startButton = false; break;
	case SELECT_BUTTON:
		inputGetter.is.selectButton = false; break;
	case FAST_FORWARD_BUTTON:
		emit setTurbo(false); break;
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

unsigned GambatteSource::update(qint16 *sampleBuf, const unsigned samples) {
	return gb.runFor(reinterpret_cast<Gambatte::uint_least32_t*>(sampleBuf), samples);
}
