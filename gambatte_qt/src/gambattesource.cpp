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
#include "videolink/rgb32conv.h"
#include "videolink/vfilterinfo.h"
#include <algorithm>

using namespace Gambatte;

GambatteSource::GambatteSource() :
MediaSource(2064),
inputDialog_(0),
gbpixels(0),
pxformat(PixelBuffer::RGB32),
gbpitch(160),
vsrci(0),
dpadUp(false),
dpadDown(false),
dpadLeft(false),
dpadRight(false),
dpadUpLast(false),
dpadLeftLast(false) {
	inputDialog_ = createInputDialog();
	gb.setInputGetter(&inputGetter);
	std::fill(inputState, inputState + 8, false);
}

static const InputDialog::Button constructButtonInfo(InputDialog::Button::Action *action, const char *label,
		const char *category, int defaultKey = Qt::Key_unknown, int defaultAltKey = Qt::Key_unknown) {
	InputDialog::Button b = { label: label, category: category, defaultKey: defaultKey,
			defaultAltKey: defaultAltKey, action: action };

	return b;
}

template<void (GambatteSource::*fun)()>
struct CallAct : InputDialog::Button::Action {
	GambatteSource *const source;
	CallAct(GambatteSource *const source) : source(source) {}
	void buttonPressed() { (source->*fun)(); }
};

template<bool bval>
struct GbDirAct : InputDialog::Button::Action {
	volatile bool &a, &b;
	GbDirAct(volatile bool &a, volatile bool &b) : a(a), b(b) {}
	void buttonPressed() { a = true; b = bval; }
	void buttonReleased() { a = false; }
};

enum { A_BUT, B_BUT, SELECT_BUT, START_BUT, RIGHT_BUT, LEFT_BUT, UP_BUT, DOWN_BUT };

InputDialog* GambatteSource::createInputDialog() {
	std::vector<InputDialog::Button> v;
	
	struct GbButAct : InputDialog::Button::Action {
		bool &a;
		GbButAct(bool &a) : a(a) {}
		void buttonPressed() { a = true; }
		void buttonReleased() { a = false; }
	};
	
	struct FastForwardAct : InputDialog::Button::Action {
		GambatteSource *const source;
		FastForwardAct(GambatteSource *const source) : source(source) {}
		void buttonPressed() { source->emitSetTurbo(true); }
		void buttonReleased() { source->emitSetTurbo(false); }
	};
	
	v.reserve(18);
	v.push_back(constructButtonInfo(new GbDirAct<true>(dpadUp, dpadUpLast), "Up", "Game", Qt::Key_Up));
	v.push_back(constructButtonInfo(new GbDirAct<false>(dpadDown, dpadUpLast), "Down", "Game", Qt::Key_Down));
	v.push_back(constructButtonInfo(new GbDirAct<true>(dpadLeft, dpadLeftLast), "Left", "Game", Qt::Key_Left));
	v.push_back(constructButtonInfo(new GbDirAct<false>(dpadRight, dpadLeftLast), "Right", "Game", Qt::Key_Right));
	v.push_back(constructButtonInfo(new GbButAct(inputState[A_BUT]), "A", "Game", Qt::Key_D));
	v.push_back(constructButtonInfo(new GbButAct(inputState[B_BUT]), "B", "Game", Qt::Key_C));
	v.push_back(constructButtonInfo(new GbButAct(inputState[START_BUT]), "Start", "Game", Qt::Key_Return));
	v.push_back(constructButtonInfo(new GbButAct(inputState[SELECT_BUT]), "Select", "Game", Qt::Key_Shift));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitPause>(this), "Pause", "Play", Qt::Key_Pause));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitFrameStep>(this), "Frame step", "Play", Qt::Key_F1));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitDecFrameRate>(this), "Decrease frame rate", "Play", Qt::Key_F2));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitIncFrameRate>(this), "Increase frame rate", "Play", Qt::Key_F3));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitResetFrameRate>(this), "Reset frame rate", "Play", Qt::Key_F4));
	v.push_back(constructButtonInfo(new FastForwardAct(this), "Fast forward", "Play", Qt::Key_Tab));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitSaveState>(this), "Save state", "State", Qt::Key_F5));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitLoadState>(this), "Load state", "State", Qt::Key_F8));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitPrevStateSlot>(this), "Previous state slot", "State", Qt::Key_F6));
	v.push_back(constructButtonInfo(new CallAct<&GambatteSource::emitNextStateSlot>(this), "Next state slot", "State", Qt::Key_F7));
	
	return new InputDialog(v);
}

const std::vector<VideoDialog::VideoSourceInfo> GambatteSource::generateVideoSourceInfos() {
	std::vector<VideoDialog::VideoSourceInfo> v(VfilterInfo::numVfilters());
	
	for (unsigned i = 0; i < v.size(); ++i) {
		const VideoDialog::VideoSourceInfo vsi = { label: VfilterInfo::get(i).handle,
				width: VfilterInfo::get(i).outWidth, height: VfilterInfo::get(i).outHeight };
		
		v[i] = vsi;
	}
	
	return v;
}

void GambatteSource::keyPressEvent(const QKeyEvent *e) {
	inputDialog_->keyPressEvent(e);
}

void GambatteSource::keyReleaseEvent(const QKeyEvent *e) {
	inputDialog_->keyReleaseEvent(e);
}

void GambatteSource::joystickEvent(const SDL_Event &e) {
	inputDialog_->joystickEvent(e);
}

void GambatteSource::setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, unsigned pitch) {
	if (pxformat != format) {
		pxformat = format;
		cconvert.reset();
		cconvert.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat),
				VfilterInfo::get(vsrci).outWidth, VfilterInfo::get(vsrci).outHeight));
	}
	
	if (VideoLink *const gblink = vfilter.get() ? vfilter.get() : cconvert.get()) {
		gbpixels = static_cast<uint_least32_t*>(gblink->inBuf());
		gbpitch  = gblink->inPitch();
	} else {
		gbpixels = static_cast<uint_least32_t*>(pixels);
		gbpitch = pitch;
	}
}

static void setGbDir(bool &a, bool &b, const bool aPressed, const bool bPressed, const bool preferA) {
	if (aPressed & bPressed) {
		a =  aPressed & preferA;
		b = (aPressed & preferA) ^ 1;
	} else {
		a = aPressed;
		b = bPressed;
	}
}

static unsigned packedInputState(const bool inputState[8]) {
	unsigned is = 0;
	
	for (unsigned i = 0; i < 8; ++i)
		is |= inputState[i] << i;
	
	return is;
}

static void* getpbdata(const PixelBuffer &pb, const unsigned vsrci) {
	return pb.width == VfilterInfo::get(vsrci).outWidth && pb.height == VfilterInfo::get(vsrci).outHeight ? pb.data : 0;
}

long GambatteSource::update(const PixelBuffer &pb, qint16 *const soundBuf, long &samples) {
	setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	samples -= overupdate;
	
	if (samples < 0) {
		samples = 0;
		return -1;
	}
	
	setGbDir(inputState[UP_BUT], inputState[DOWN_BUT], dpadUp, dpadDown, dpadUpLast);
	setGbDir(inputState[LEFT_BUT], inputState[RIGHT_BUT], dpadLeft, dpadRight, dpadLeftLast);
	inputGetter.is = packedInputState(inputState);
	
	unsigned usamples = samples;
	const long retval = gb.runFor(gbpixels, gbpitch, reinterpret_cast<uint_least32_t*>(soundBuf), usamples);
	samples = usamples;
	
	return retval;
}

void GambatteSource::generateVideoFrame(const PixelBuffer &pb) {
	if (void *const pbdata = getpbdata(pb, vsrci)) {
		setPixelBuffer(pbdata, pb.pixelFormat, pb.pitch);
	
		if (vfilter.get()) {
			if (cconvert.get()) {
				vfilter->draw(cconvert->inBuf(), cconvert->inPitch());
				cconvert->draw(pbdata, pb.pitch);
			} else
				vfilter->draw(pbdata, pb.pitch);
		} else if (cconvert.get())
			cconvert->draw(pbdata, pb.pitch);
	}
}

void GambatteSource::setVideoSource(unsigned videoSourceIndex) {
	if (videoSourceIndex != vsrci) {
		vsrci = videoSourceIndex;
		vfilter.reset();
		cconvert.reset();
		vfilter.reset(VfilterInfo::get(vsrci).create());
		cconvert.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat),
				VfilterInfo::get(vsrci).outWidth, VfilterInfo::get(vsrci).outHeight));
	}
}

void GambatteSource::saveState(const PixelBuffer &pb) {
	setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	gb.saveState(gbpixels, gbpitch);
}

void GambatteSource::saveState(const PixelBuffer &pb, const char *filepath) {
	setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	gb.saveState(gbpixels, gbpitch, filepath);
}
