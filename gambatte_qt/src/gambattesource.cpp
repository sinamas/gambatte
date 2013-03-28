/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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

namespace {

using namespace gambatte;

static InputDialog::Button makeButtonInfo(InputDialog::Button::Action *action, char const *label,
		char const *category, int defaultKey = Qt::Key_unknown, int defaultAltKey = Qt::Key_unknown,
		unsigned char defaultFpp = 0) {
	InputDialog::Button b = { label: label, category: category, defaultKey: defaultKey,
	                          defaultAltKey: defaultAltKey, action: action, defaultFpp: defaultFpp };
	return b;
}

template<void (GambatteSource::*fun)()>
struct CallAct : InputDialog::Button::Action {
	GambatteSource *const source;
	explicit CallAct(GambatteSource *source) : source(source) {}
	virtual void buttonPressed() { (source->*fun)(); }
};

template<bool bval>
struct GbDirAct : InputDialog::Button::Action {
	bool &a, &b;
	explicit GbDirAct(bool &a, bool &b) : a(a), b(b) {}
	virtual void buttonPressed() { a = true; b = bval; }
	virtual void buttonReleased() { a = false; }
};

enum { a_but, b_but, select_but, start_but, right_but, left_but, up_but, down_but };

} // anon ns

GambatteSource::GambatteSource()
: MediaSource(2064)
, inputDialog_(createInputDialog())
, pxformat_(PixelBuffer::RGB32)
, vsrci_(0)
, inputState_()
, dpadUp_(false)
, dpadDown_(false)
, dpadLeft_(false)
, dpadRight_(false)
, dpadUpLast_(false)
, dpadLeftLast_(false)
{
	gb_.setInputGetter(&inputGetter_);
}

InputDialog * GambatteSource::createInputDialog() {
	struct GbButAct : InputDialog::Button::Action {
		bool &a;
		explicit GbButAct(bool &a) : a(a) {}
		virtual void buttonPressed() { a = true; }
		virtual void buttonReleased() { a = false; }
	};

	struct FastForwardAct : InputDialog::Button::Action {
		GambatteSource *const source;
		explicit FastForwardAct(GambatteSource *source) : source(source) {}
		virtual void buttonPressed() { source->emitSetTurbo(true); }
		virtual void buttonReleased() { source->emitSetTurbo(false); }
	};

	std::vector<InputDialog::Button> v;
	v.push_back(makeButtonInfo(new GbDirAct<true>(dpadUp_, dpadUpLast_), "Up", "Game", Qt::Key_Up));
	v.push_back(makeButtonInfo(new GbDirAct<false>(dpadDown_, dpadUpLast_), "Down", "Game", Qt::Key_Down));
	v.push_back(makeButtonInfo(new GbDirAct<true>(dpadLeft_, dpadLeftLast_), "Left", "Game", Qt::Key_Left));
	v.push_back(makeButtonInfo(new GbDirAct<false>(dpadRight_, dpadLeftLast_), "Right", "Game", Qt::Key_Right));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[a_but]), "A", "Game", Qt::Key_D));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[b_but]), "B", "Game", Qt::Key_C));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[start_but]), "Start", "Game", Qt::Key_Return));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[select_but]), "Select", "Game", Qt::Key_Shift));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitPause>(this), "Pause", "Play", Qt::Key_Pause));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitFrameStep>(this), "Frame step", "Play", Qt::Key_F1));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitDecFrameRate>(this),
	                           "Decrease frame rate", "Play", Qt::Key_F2));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitIncFrameRate>(this),
	                           "Increase frame rate", "Play", Qt::Key_F3));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitResetFrameRate>(this),
	                           "Reset frame rate", "Play", Qt::Key_F4));
	v.push_back(makeButtonInfo(new FastForwardAct(this), "Fast forward", "Play", Qt::Key_Tab));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitSaveState>(this), "Save state", "State", Qt::Key_F5));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitLoadState>(this), "Load state", "State", Qt::Key_F8));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitPrevStateSlot>(this),
	                           "Previous state slot", "State", Qt::Key_F6));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitNextStateSlot>(this),
	                           "Next state slot", "State", Qt::Key_F7));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[8 + a_but]), "Turbo A", "Other",
	                           Qt::Key_unknown, Qt::Key_unknown, 1));
	v.push_back(makeButtonInfo(new GbButAct(inputState_[8 + b_but]), "Turbo B", "Other",
	                           Qt::Key_unknown, Qt::Key_unknown, 1));
	v.push_back(makeButtonInfo(new CallAct<&GambatteSource::emitQuit>(this), "Quit", "Other"));

	return new InputDialog(v);
}

std::vector<VideoDialog::VideoSourceInfo> const GambatteSource::generateVideoSourceInfos() {
	std::vector<VideoDialog::VideoSourceInfo> v(VfilterInfo::numVfilters());
	for (std::size_t i = 0; i < v.size(); ++i) {
		VideoDialog::VideoSourceInfo vsi = { label: VfilterInfo::get(i).handle,
			width: VfilterInfo::get(i).outWidth, height: VfilterInfo::get(i).outHeight };

		v[i] = vsi;
	}

	return v;
}

void GambatteSource::keyPressEvent(QKeyEvent const *e) {
	inputDialog_->keyPressEvent(e);
}

void GambatteSource::keyReleaseEvent(QKeyEvent const *e) {
	inputDialog_->keyReleaseEvent(e);
}

void GambatteSource::joystickEvent(SDL_Event const &e) {
	inputDialog_->joystickEvent(e);
}

struct GambatteSource::GbVidBuf {
	uint_least32_t *pixels; std::ptrdiff_t pitch;
	GbVidBuf(uint_least32_t *pixels, std::ptrdiff_t pitch) : pixels(pixels), pitch(pitch) {}
};

GambatteSource::GbVidBuf GambatteSource::setPixelBuffer(
		void *pixels, PixelBuffer::PixelFormat format, std::ptrdiff_t pitch) {
	if (pxformat_ != format && pixels) {
		pxformat_ = format;
		cconvert_.reset();
		cconvert_.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat_),
		                                  VfilterInfo::get(vsrci_).outWidth,
		                                  VfilterInfo::get(vsrci_).outHeight));
	}

	if (VideoLink *gblink = vfilter_ ? vfilter_.get() : cconvert_.get())
		return GbVidBuf(static_cast<uint_least32_t *>(gblink->inBuf()), gblink->inPitch());

	return GbVidBuf(static_cast<uint_least32_t *>(pixels), pitch);
}

static void setGbDir(bool &a, bool &b, bool const aPressed, bool const bPressed, bool const preferA) {
	if (aPressed & bPressed) {
		a =  aPressed & preferA;
		b = (aPressed & preferA) ^ 1;
	} else {
		a = aPressed;
		b = bPressed;
	}
}

static unsigned packedInputState(bool const inputState[], std::size_t const len) {
	unsigned is = 0;
	for (std::size_t i = 0; i < len; ++i)
		is |= inputState[i] << (i & 7);

	return is;
}

static void * getpbdata(PixelBuffer const &pb, unsigned vsrci) {
	return    pb.width  == VfilterInfo::get(vsrci).outWidth
	       && pb.height == VfilterInfo::get(vsrci).outHeight
	     ? pb.data
	     : 0;
}

long GambatteSource::update(PixelBuffer const &pb, qint16 *const soundBuf, long &samples) {
	GbVidBuf const gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	samples -= overupdate;
	if (samples < 0) {
		samples = 0;
		return -1;
	}

	setGbDir(inputState_[up_but], inputState_[down_but], dpadUp_, dpadDown_, dpadUpLast_);
	setGbDir(inputState_[left_but], inputState_[right_but], dpadLeft_, dpadRight_, dpadLeftLast_);
	inputGetter_.is = packedInputState(inputState_, sizeof inputState_ / sizeof inputState_[0]);

	unsigned usamples = samples;
	long const retval = gb_.runFor(gbvidbuf.pixels, gbvidbuf.pitch,
	                               reinterpret_cast<quint32 *>(soundBuf), usamples);
	samples = usamples;

	if (retval >= 0)
		inputDialog_->consumeAutoPress();

	return retval;
}

void GambatteSource::generateVideoFrame(PixelBuffer const &pb) {
	if (void *const pbdata = getpbdata(pb, vsrci_)) {
		setPixelBuffer(pbdata, pb.pixelFormat, pb.pitch);

		if (vfilter_) {
			if (cconvert_) {
				vfilter_->draw(cconvert_->inBuf(), cconvert_->inPitch());
				cconvert_->draw(pbdata, pb.pitch);
			} else
				vfilter_->draw(pbdata, pb.pitch);
		} else if (cconvert_)
			cconvert_->draw(pbdata, pb.pitch);
	}
}

void GambatteSource::setVideoSource(unsigned const videoSourceIndex) {
	if (videoSourceIndex != vsrci_) {
		vsrci_ = videoSourceIndex;
		vfilter_.reset();
		cconvert_.reset();
		vfilter_.reset(VfilterInfo::get(vsrci_).create());
		cconvert_.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(pxformat_),
		                                  VfilterInfo::get(vsrci_).outWidth,
		                                  VfilterInfo::get(vsrci_).outHeight));
	}
}

void GambatteSource::saveState(PixelBuffer const &pb) {
	GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	gb_.saveState(gbvidbuf.pixels, gbvidbuf.pitch);
}

void GambatteSource::saveState(PixelBuffer const &pb, std::string const &filepath) {
	GbVidBuf gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	gb_.saveState(gbvidbuf.pixels, gbvidbuf.pitch, filepath);
}
