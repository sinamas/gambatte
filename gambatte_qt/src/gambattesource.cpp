//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "gambattesource.h"
#include "videolink/rgb32conv.h"
#include "videolink/vfilterinfo.h"

namespace {

using namespace gambatte;

struct ButtonInfo {
	char const *label;
	char const *category;
	int defaultKey;
	int defaultAltKey;
	unsigned char defaultFpp;
};

static ButtonInfo const buttonInfoGbUp = { "Up", "Game", Qt::Key_Up, 0, 0 };
static ButtonInfo const buttonInfoGbDown = { "Down", "Game", Qt::Key_Down, 0, 0 };
static ButtonInfo const buttonInfoGbLeft = { "Left", "Game", Qt::Key_Left, 0, 0 };
static ButtonInfo const buttonInfoGbRight = { "Right", "Game", Qt::Key_Right, 0, 0 };
static ButtonInfo const buttonInfoGbA = { "A", "Game", Qt::Key_D, 0, 0 };
static ButtonInfo const buttonInfoGbB = { "B", "Game", Qt::Key_C, 0, 0 };
static ButtonInfo const buttonInfoGbStart = { "Start", "Game", Qt::Key_Return, 0, 0 };
static ButtonInfo const buttonInfoGbSelect = { "Select", "Game", Qt::Key_Shift, 0, 0 };
static ButtonInfo const buttonInfoPause = { "Pause", "Play", Qt::Key_Pause, 0, 0 };
static ButtonInfo const buttonInfoFrameStep = { "Frame step", "Play", Qt::Key_F1, 0, 0 };
static ButtonInfo const buttonInfoDecFrameRate = { "Decrease frame rate", "Play", Qt::Key_F2, 0, 0 };
static ButtonInfo const buttonInfoIncFrameRate = { "Increase frame rate", "Play", Qt::Key_F3, 0, 0 };
static ButtonInfo const buttonInfoResetFrameRate = { "Reset frame rate", "Play", Qt::Key_F4, 0, 0 };
static ButtonInfo const buttonInfoFastFwd = { "Fast forward", "Play", Qt::Key_Tab, 0, 0 };
static ButtonInfo const buttonInfoSaveState = { "Save state", "State", Qt::Key_F5, 0, 0 };
static ButtonInfo const buttonInfoLoadState = { "Load state", "State", Qt::Key_F8, 0, 0 };
static ButtonInfo const buttonInfoPrevState = { "Previous state slot", "State", Qt::Key_F6, 0, 0 };
static ButtonInfo const buttonInfoNextState = { "Next state slot", "State", Qt::Key_F7, 0, 0 };
static ButtonInfo const buttonInfoGbATurbo = { "Turbo A", "Other", 0, 0, 1 };
static ButtonInfo const buttonInfoGbBTurbo = { "Turbo B", "Other", 0, 0, 1 };
static ButtonInfo const buttonInfoQuit = { "Quit", "Other", 0, 0, 0 };

// separate non-template base to avoid obj code bloat
class ButtonBase : public InputDialog::Button {
public:
	// pass by ptr to disallow passing temporaries
	explicit ButtonBase(ButtonInfo const *info) : info_(info) {}
	virtual QString const label() const { return info_->label; }
	virtual QString const category() const { return info_->category; }
	virtual int defaultKey() const { return info_->defaultKey; }
	virtual int defaultAltKey() const { return info_->defaultAltKey; }
	virtual unsigned char defaultFpp() const { return info_->defaultFpp; }

private:
	ButtonInfo const *info_;
};

template<class T>
static void addButton(auto_vector<InputDialog::Button> &v,
		ButtonInfo const *info, void (T::*onPressed)(), T *t) {
	class Button : public ButtonBase {
	public:
		Button(ButtonInfo const *info, void (T::*onPressed)(), T *t)
		: ButtonBase(info), onPressed_(onPressed), t_(t)
		{
		}

		virtual void pressed() { (t_->*onPressed_)(); }
		virtual void released() {}

	private:
		void (T::*onPressed_)();
		T *t_;
	};

	v.push_back(new Button(info, onPressed, t));
}

template<class Action>
static void addButton(auto_vector<InputDialog::Button> &v, ButtonInfo const *info, Action action) {
	class Button : public ButtonBase {
	public:
		Button(ButtonInfo const *info, Action action)
		: ButtonBase(info), action_(action)
		{
		}

		virtual void pressed() { action_.pressed(); }
		virtual void released() { action_.released(); }

	private:
		Action action_;
	};

	v.push_back(new Button(info, action));
}

template<void (GambatteSource::*setPressed)(bool)>
struct CallSetterAct {
	GambatteSource *source;
	explicit CallSetterAct(GambatteSource *source) : source(source) {}
	void  pressed() const { (source->*setPressed)(true); }
	void released() const { (source->*setPressed)(false); }
};

template<bool button_id>
class GbDirAct {
public:
	GbDirAct(bool &buttonPressed, bool &lastPressedButtonId)
	: buttonPressed_(buttonPressed), lastPressedButtonId_(lastPressedButtonId)
	{
	}

	void  pressed() const { buttonPressed_ = true; lastPressedButtonId_ = button_id; }
	void released() const { buttonPressed_ = false; }

private:
	bool &buttonPressed_, &lastPressedButtonId_;
};

struct SetPressedAct {
	bool &v;
	explicit SetPressedAct(bool &v) : v(v) {}
	void pressed() const { v = true; }
	void released() const { v = false; }
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
	auto_vector<InputDialog::Button> v;
	addButton(v, &buttonInfoGbUp, GbDirAct<true>(dpadUp_, dpadUpLast_));
	addButton(v, &buttonInfoGbDown, GbDirAct<false>(dpadDown_, dpadUpLast_));
	addButton(v, &buttonInfoGbLeft, GbDirAct<true>(dpadLeft_, dpadLeftLast_));
	addButton(v, &buttonInfoGbRight, GbDirAct<false>(dpadRight_, dpadLeftLast_));
	addButton(v, &buttonInfoGbA, SetPressedAct(inputState_[a_but]));
	addButton(v, &buttonInfoGbB, SetPressedAct(inputState_[b_but]));
	addButton(v, &buttonInfoGbStart, SetPressedAct(inputState_[start_but]));
	addButton(v, &buttonInfoGbSelect, SetPressedAct(inputState_[select_but]));
	addButton(v, &buttonInfoPause, &GambatteSource::emitPause, this);
	addButton(v, &buttonInfoFrameStep, &GambatteSource::emitFrameStep, this);
	addButton(v, &buttonInfoDecFrameRate, &GambatteSource::emitDecFrameRate, this);
	addButton(v, &buttonInfoIncFrameRate, &GambatteSource::emitIncFrameRate, this);
	addButton(v, &buttonInfoResetFrameRate, &GambatteSource::emitResetFrameRate, this);
	addButton(v, &buttonInfoFastFwd, CallSetterAct<&GambatteSource::emitSetTurbo>(this));
	addButton(v, &buttonInfoSaveState, &GambatteSource::emitSaveState, this);
	addButton(v, &buttonInfoLoadState, &GambatteSource::emitLoadState, this);
	addButton(v, &buttonInfoPrevState, &GambatteSource::emitPrevStateSlot, this);
	addButton(v, &buttonInfoNextState, &GambatteSource::emitNextStateSlot, this);
	addButton(v, &buttonInfoGbATurbo, SetPressedAct(inputState_[8 + a_but]));
	addButton(v, &buttonInfoGbBTurbo, SetPressedAct(inputState_[8 + b_but]));
	addButton(v, &buttonInfoQuit, &GambatteSource::emitQuit, this);

	InputDialog *dialog = new InputDialog(v);
	QObject *o = dialog;
	o->setParent(this);
	return dialog;
}

std::vector<VideoDialog::VideoSourceInfo> const GambatteSource::generateVideoSourceInfos() {
	std::vector<VideoDialog::VideoSourceInfo> v(VfilterInfo::numVfilters());
	for (std::size_t i = 0; i < v.size(); ++i) {
		VfilterInfo const &vfi = VfilterInfo::get(i);
		VideoDialog::VideoSourceInfo vsi =
			{ vfi.handle, QSize(vfi.outWidth, vfi.outHeight) };
		v[i] = vsi;
	}

	return v;
}

void GambatteSource::keyPressEvent(QKeyEvent const *e) {
	inputDialog_->keyPress(e);
}

void GambatteSource::keyReleaseEvent(QKeyEvent const *e) {
	inputDialog_->keyRelease(e);
}

void GambatteSource::joystickEvent(SDL_Event const &e) {
	inputDialog_->joystickEvent(e);
}

struct GambatteSource::GbVidBuf {
	uint_least32_t *pixels;
	std::ptrdiff_t pitch;

	GbVidBuf(uint_least32_t *pixels, std::ptrdiff_t pitch)
	: pixels(pixels), pitch(pitch)
	{
	}
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

static void setGbDir(bool &a, bool &b,
		bool const aPressed, bool const bPressed, bool const preferA) {
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

static void * getpbdata(PixelBuffer const &pb, std::size_t vsrci) {
	return    pb.width  == VfilterInfo::get(vsrci).outWidth
	       && pb.height == VfilterInfo::get(vsrci).outHeight
	     ? pb.data
	     : 0;
}

template<class T>
static T * ptr_cast(void *p) { return static_cast<T *>(p); }

std::ptrdiff_t GambatteSource::update(
		PixelBuffer const &pb, qint16 *const soundBuf, std::size_t &samples) {
	GbVidBuf const gbvidbuf = setPixelBuffer(getpbdata(pb, vsrci_), pb.pixelFormat, pb.pitch);
	if (samples < overUpdate) {
		samples = 0;
		return -1;
	}

	setGbDir(inputState_[up_but], inputState_[down_but],
	         dpadUp_, dpadDown_, dpadUpLast_);
	setGbDir(inputState_[left_but], inputState_[right_but],
	         dpadLeft_, dpadRight_, dpadLeftLast_);
	inputGetter_.is = packedInputState(inputState_, sizeof inputState_ / sizeof inputState_[0]);

	samples -= overUpdate;
	std::ptrdiff_t const vidFrameSampleNo =
		gb_.runFor(gbvidbuf.pixels, gbvidbuf.pitch,
		           ptr_cast<quint32>(soundBuf), samples);
	if (vidFrameSampleNo >= 0)
		inputDialog_->consumeAutoPress();

	return vidFrameSampleNo;
}

void GambatteSource::generateVideoFrame(PixelBuffer const &pb) {
	if (void *const pbdata = getpbdata(pb, vsrci_)) {
		setPixelBuffer(pbdata, pb.pixelFormat, pb.pitch);
		if (vfilter_) {
			void          *dstbuf   = cconvert_ ? cconvert_->inBuf()   : pbdata;
			std::ptrdiff_t dstpitch = cconvert_ ? cconvert_->inPitch() : pb.pitch;
			vfilter_->draw(dstbuf, dstpitch);
		}

		if (cconvert_)
			cconvert_->draw(pbdata, pb.pitch);
	}
}

void GambatteSource::setVideoSource(std::size_t const videoSourceIndex) {
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
