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
#include "videolink/rgb32conv.h"
#include "videolink/vfilterinfo.h"
#include <cstring>

using namespace gambatte;

GambatteSource::GambatteSource()
: MediaSource(2064),
  inputDialog_(createInputDialog()),
  pxformat(PixelBuffer::RGB32),
  vsrci(0),
  dpadUp(false),
  dpadDown(false),
  dpadLeft(false),
  dpadRight(false),
  dpadUpLast(false),
  dpadLeftLast(false)
{
	gb.setInputGetter(&inputGetter);
	std::memset(inputState, 0, sizeof inputState);
}

namespace {
static const InputDialog::Button makeButtonInfo(InputDialog::Button::Action *action, const char *label,
		const char *category, int defaultKey = Qt::Key_unknown, int defaultAltKey = Qt::Key_unknown, int defaultFpp = 0) {
	const InputDialog::Button b = { label: label, category: category, defaultKey: defaultKey,
	                          defaultAltKey: defaultAltKey, action: action, defaultFpp: defaultFpp };
	return b;
}

template<void (GambatteSource::*fun)()>
struct CallAct : InputDialog::Button::Action {
	GambatteSource *const source;
	explicit CallAct(GambatteSource *const source) : source(source) {}
	virtual void buttonPressed() { (source->*fun)(); }
};

template<bool bval>
struct GbDirAct : InputDialog::Button::Action {
	volatile bool &a, &b;
	explicit GbDirAct(volatile bool &a, volatile bool &b) : a(a), b(b) {}
	virtual void buttonPressed() { a = true; b = bval; }
	virtual void buttonReleased() { a = false; }
};

enum { A_BUT, B_BUT, SELECT_BUT, START_BUT, RIGHT_BUT, LEFT_BUT, UP_BUT, DOWN_BUT };
}

InputDialog* GambatteSource::createInputDialog() {
	std::vector<InputDialog::Button> v;
	
	struct GbButAct : InputDialog::Button::Action {
		bool &a;
		explicit GbButAct(bool &a) : a(a) {}
		virtual void buttonPressed() { a = true; }
		virtual void buttonReleased() { a = false; }
	};
	
	struct FastForwardAct : InputDialog::Button::Action {
		GambatteSource *const source;
		explicit FastForwardAct(GambatteSource *const source) : source(source) {}
		virtual void buttonPressed() { source->emitSetTurbo(true); }
		virtual void buttonReleased() { source->emitSetTurbo(false); }
	};
	
	v.push_back(makeButtonInfo(new GbDirAct<true>(dpadUp, dpadUpLast), "Up", "Game", Qt::Key_Up));
	v.push_back(makeButtonInfo(new GbDirAct<false>(dpadDown, dpadUpLast), "Down", "Game", Qt::Key_Down));
	v.push_back(makeButtonInfo(new GbDirAct<true>(dpadLeft, dpadLeftLast), "Left", "Game", Qt::Key_Left));
	v.push_back(makeButtonInfo(new GbDirAct<false>(dpadRight, dpadLeftLast), "Right", "Game", Qt::Key_Right));
	v.push_back(makeButtonInfo(new GbButAct(inputState[A_BUT]), "A", "Game", Qt::Key_D));
	v.push_back(makeButtonInfo(new GbButAct(inputState[B_BUT]), "B", "Game", Qt::Key_C));
	v.push_back(makeButtonInfo(new GbButAct(inputState[START_BUT]), "Start", "Game", Qt::Key_Return));
	v.push_back(makeButtonInfo(new GbButAct(inputState[SELECT_BUT]), "Select", "Game", Qt::Key_Shift));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitPause>(this), "Pause", "Play", Qt::Key_Pause));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitFrameStep>(this), "Frame step", "Play", Qt::Key_F1));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitDecFrameRate>(this), "Decrease frame rate", "Play", Qt::Key_F2));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitIncFrameRate>(this), "Increase frame rate", "Play", Qt::Key_F3));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitResetFrameRate>(this), "Reset frame rate", "Play", Qt::Key_F4));
	v.push_back(makeButtonInfo(new FastForwardAct(this), "Fast forward", "Play", Qt::Key_Tab));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitSaveState>(this), "Save state", "State", Qt::Key_F5));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitLoadState>(this), "Load state", "State", Qt::Key_F8));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitPrevStateSlot>(this), "Previous state slot", "State", Qt::Key_F6));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitNextStateSlot>(this), "Next state slot", "State", Qt::Key_F7));
	v.push_back(makeButtonInfo(new GbButAct(inputState[8 + A_BUT]), "Turbo A", "Other", Qt::Key_unknown, Qt::Key_unknown, 1));
	v.push_back(makeButtonInfo(new GbButAct(inputState[8 + B_BUT]), "Turbo B", "Other", Qt::Key_unknown, Qt::Key_unknown, 1));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitQuit>(this), "Quit", "Other"));
	
	return new InputDialog(v);
}

const std::vector<VideoDialog::VideoSourceInfo> GambatteSource::generateVideoSourceInfos() {
	std::vector<VideoDialog::VideoSourceInfo> v(VfilterInfo::numVfilters());
	
	for (std::size_t i = 0; i < v.size(); ++i) {
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

struct GambatteSource::GbVidBuf {
	uint_least32_t *pixels; long pitch;
	GbVidBuf(uint_least32_t *pixels, long pitch) : pixels(pixels), pitch(pitch) {}
};

const GambatteSource::GbVidBuf GambatteSource::setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, unsigned pitch) {
	if (pxformat != format && pixels) {
		pxformat = format;
		cconvert.reset();
		cconvert.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat),
				VfilterInfo::get(vsrci).outWidth, VfilterInfo::get(vsrci).outHeight));
	}
	
	if (VideoLink *const gblink = vfilter.get() ? vfilter.get() : cconvert.get())
		return GbVidBuf(static_cast<uint_least32_t*>(gblink->inBuf()), gblink->inPitch());
	
	return GbVidBuf(static_cast<uint_least32_t*>(pixels), pitch);
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

static unsigned packedInputState(const bool inputState[], const std::size_t len) {
	unsigned is = 0;
	
	for (std::size_t i = 0; i < len; ++i)
		is |= inputState[i] << (i & 7);
	
	return is;
}

static void* getpbdata(const PixelBuffer &pb, const unsigned vsrci) {
	return pb.width == VfilterInfo::get(vsrci).outWidth && pb.height == VfilterInfo::get(vsrci).outHeight ? pb.data : 0;
}

long GambatteSource::update(const PixelBuffer &pb, qint16 *const soundBuf, long &samples) {
	const GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	samples -= overupdate;
	
	if (samples < 0) {
		samples = 0;
		return -1;
	}
	
	setGbDir(inputState[UP_BUT], inputState[DOWN_BUT], dpadUp, dpadDown, dpadUpLast);
	setGbDir(inputState[LEFT_BUT], inputState[RIGHT_BUT], dpadLeft, dpadRight, dpadLeftLast);
	inputGetter.is = packedInputState(inputState, (sizeof inputState) / (sizeof inputState[0]));
	
	unsigned usamples = samples;
	const long retval = gb.runFor(gbvidbuf.pixels, gbvidbuf.pitch, reinterpret_cast<uint_least32_t*>(soundBuf), usamples);
	samples = usamples;
	
	if (retval >= 0)
		inputDialog_->consumeAutoPress();
	
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
	const GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	gb.saveState(gbvidbuf.pixels, gbvidbuf.pitch);
}

void GambatteSource::saveState(const PixelBuffer &pb, const std::string &filepath) {
	const GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci), pb.pixelFormat, pb.pitch);
	gb.saveState(gbvidbuf.pixels, gbvidbuf.pitch, filepath);
}
