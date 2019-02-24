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

#include "adaptivesleep.h"
#include "audiosink.h"
#include "blitterwrapper.h"
#include "parser.h"
#include "resample/resampler.h"
#include "resample/resamplerinfo.h"
#include "skipsched.h"
#include "str_to_sdlkey.h"
#include "videolink/vfilterinfo.h"
#include <gambatte.h>
#include <pakinfo.h>
#include <SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace gambatte;

class DescOption : public Parser::Option {
public:
	explicit DescOption(char const *s, char c = 0, int nArgs = 0)
	: Option(s, c, nArgs)
	{
	}

	virtual ~DescOption() {}
	virtual std::string const desc() const = 0;
};

class BoolOption : public DescOption {
public:
	BoolOption(char const *desc, char const *s, char c = 0)
	: DescOption(s, c), desc_(desc), isSet_(false)
	{
	}

	virtual void exec(char const *const *, int) { isSet_ = true; }
	virtual std::string const desc() const { return desc_; }
	bool isSet() const { return isSet_; }

private:
	char const *const desc_;
	bool isSet_;
};

class RateOption : public DescOption {
public:
	RateOption()
	: DescOption("sample-rate", 'r', 1)
	, rate_(48000)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		long r = std::atol(argv[index + 1]);
		if (r < 4000 || r > 192000)
			return;

		rate_ = r;
	}

	virtual std::string const desc() const {
		return " N\t\tUse audio sample rate of N Hz\n"
		       "\t\t\t\t    4000 <= N <= 192000, default: 48000\n";
	}

	long rate() const { return rate_; }

private:
	long rate_;
};

class LatencyOption : public DescOption {
public:
	LatencyOption()
	: DescOption("latency", 'l', 1)
	, latency_(133)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		int l = std::atoi(argv[index + 1]);
		if (l < 16 || l > 5000)
			return;

		latency_ = l;
	}

	virtual std::string const desc() const {
		return " N\t\tUse audio buffer latency of N ms\n"
		       "\t\t\t\t    16 <= N <= 5000, default: 133\n";
	}

	int latency() const { return latency_; }

private:
	int latency_;
};

class PeriodsOption : public DescOption {
public:
	PeriodsOption()
	: DescOption("periods", 'p', 1)
	, periods_(4)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		int p = std::atoi(argv[index + 1]);
		if (p < 1 || p > 32)
			return;

		periods_ = p;
	}

	virtual std::string const desc() const {
		return " N\t\tUse N audio buffer periods\n"
		       "\t\t\t\t    1 <= N <= 32, default: 4\n";
	}

	int periods() const { return periods_; }

private:
	int periods_;
};

class ScaleOption : public DescOption {
public:
	ScaleOption()
	: DescOption("scale", 's', 1)
	, scale_(1)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		int s = std::atoi(argv[index + 1]);
		if (s < 1 || s > 40)
			return;

		scale_ = s;
	}

	virtual std::string const desc() const {
		return " N\t\t\tScale video output by an integer factor of N\n";
	}

	int scale() const { return scale_; }

private:
	int scale_;
};

class VfOption : public DescOption {
public:
	VfOption()
	: DescOption("video-filter", 'v', 1)
	, filterNo_(0)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		unsigned long fno = std::strtoul(argv[index + 1], 0, 0);
		if (fno < VfilterInfo::numVfilters())
			filterNo_ = fno;
	}

	virtual std::string const desc() const {
		std::stringstream ss;
		ss << " N\t\tUse video filter number N\n";

		for (std::size_t i = 0; i < VfilterInfo::numVfilters(); ++i)
			ss << "\t\t\t\t    " << i << " = " << VfilterInfo::get(i).handle << '\n';

		return ss.str();
	}

	VfilterInfo const & filter() const { return VfilterInfo::get(filterNo_); }

private:
	std::size_t filterNo_;
};

class ResamplerOption : public DescOption {
public:
	ResamplerOption()
	: DescOption("resampler", 0, 1)
	, resamplerNo_(1)
	{
	}

	virtual void exec(char const *const *argv, int index) {
		unsigned long n = std::strtoul(argv[index + 1], 0, 0);
		if (n < ResamplerInfo::num())
			resamplerNo_ = n;
	}

	virtual std::string const desc() const {
		std::stringstream ss;
		ss << " N\t\tUse audio resampler number N\n";

		for (std::size_t i = 0; i < ResamplerInfo::num(); ++i) {
			ss << "\t\t\t\t    " << i << " = " << ResamplerInfo::get(i).desc;

			if (i == resamplerNo_)
				ss << " [default]";

			ss << '\n';
		}

		return ss.str();
	}

	ResamplerInfo const & resampler() const { return ResamplerInfo::get(resamplerNo_); }

private:
	std::size_t resamplerNo_;
};

struct JoyData {
	enum { dir_centered = SDL_HAT_CENTERED,
	       dir_left     = SDL_HAT_LEFT,
	       dir_right    = SDL_HAT_RIGHT,
	       dir_up       = SDL_HAT_UP,
	       dir_down     = SDL_HAT_DOWN };

	union {
		struct {
			Uint8 dev_num;
			Uint8 num;
		};

		Uint16 id;
	};

	Sint16 dir;
};

static inline bool operator<(JoyData const &l, JoyData const &r) {
	return l.id < r.id;
}

struct InputId {
	enum { type_key, type_jbutton, type_jaxis, type_jhat } type;
	union {
		JoyData jdata;
		SDLKey keydata;
	};

	InputId() : type(type_key), keydata() {}
};

class InputOption : public DescOption {
public:
	InputOption()
	: DescOption("input", 'i', 8)
	{
		ids_[0].keydata = SDLK_RETURN;
		ids_[1].keydata = SDLK_RSHIFT;
		ids_[2].keydata = SDLK_d;
		ids_[3].keydata = SDLK_c;
		ids_[4].keydata = SDLK_UP;
		ids_[5].keydata = SDLK_DOWN;
		ids_[6].keydata = SDLK_LEFT;
		ids_[7].keydata = SDLK_RIGHT;
	}

	virtual void exec(char const *const *argv, int index);

	virtual std::string const desc() const {
		return " KEYS\t\tUse the 8 given input KEYS for respectively\n"
		       "\t\t\t\t    START SELECT A B UP DOWN LEFT RIGHT\n";
	}

	std::pair<InputId, InputGetter::Button> mapping(std::size_t i) const {
		static InputGetter::Button const gbbuts[] = {
			InputGetter::START, InputGetter::SELECT,
			InputGetter::A,     InputGetter::B,
			InputGetter::UP,    InputGetter::DOWN,
			InputGetter::LEFT,  InputGetter::RIGHT,
		};

		return std::make_pair(ids_[i], gbbuts[i]);
	}

	std::size_t numMappings() const { return sizeof ids_ / sizeof *ids_; }

private:
	InputId ids_[8];
};

void InputOption::exec(char const *const *argv, int index) {
	++index;

	for (std::size_t i = 0; i < sizeof ids_ / sizeof *ids_; ++i) {
		char const *s = argv[index + i];

		if (s[0] == 'j' && s[1] == 's') {
			s += 2;
			char const *const send = s + std::strlen(s);
			if (send - s < 3)
				continue;

			int const dev_num = std::atoi(s++);
			if (dev_num < 0 || dev_num > 255)
				continue;

			s += (dev_num > 9) + (dev_num > 99);
			if (send - s < 2)
				continue;

			char const type = *s++;
			int const num = std::atoi(s++);
			if (num < 0 || num > 255)
				continue;

			s += (num > 9) + (num > 99);

			InputId id;
			id.jdata.dev_num = dev_num;
			id.jdata.num = num;

			if (type == 'b') {
				if (send - s != 0)
					continue;

				id.type = InputId::type_jbutton;
			} else {
				if (send - s != 1)
					continue;

				char const dir = *s;

				switch (type) {
				case 'a':
					switch (dir) {
					case '+': id.jdata.dir = JoyData::dir_up; break;
					case '-': id.jdata.dir = JoyData::dir_down; break;
					default: continue;
					}

					id.type = InputId::type_jaxis;
					break;
				case 'h':
					switch (dir) {
					case 'u': id.jdata.dir = JoyData::dir_up; break;
					case 'd': id.jdata.dir = JoyData::dir_down; break;
					case 'l': id.jdata.dir = JoyData::dir_left; break;
					case 'r': id.jdata.dir = JoyData::dir_right; break;
					default: continue;
					}

					id.type = InputId::type_jhat;
					break;
				default: continue;
				}
			}

			ids_[i] = id;
		} else if (SDLKey const *k = strToSdlkey(s)) {
			ids_[i].type = InputId::type_key;
			ids_[i].keydata = *k;
		}
	}
}

class AudioOut {
public:
	struct Status {
		long rate;
		bool low;

		Status(long rate, bool low) : rate(rate), low(low) {}
	};

	AudioOut(long sampleRate, int latency, int periods,
	         ResamplerInfo const &resamplerInfo, std::size_t maxInSamplesPerWrite)
	: resampler_(resamplerInfo.create(2097152, sampleRate, maxInSamplesPerWrite))
	, resampleBuf_(resampler_->maxOut(maxInSamplesPerWrite) * 2)
	, sink_(sampleRate, latency, periods)
	{
	}

	Status write(Uint32 const *data, std::size_t samples) {
		long const outsamples = resampler_->resample(
			resampleBuf_, reinterpret_cast<Sint16 const *>(data), samples);
		AudioSink::Status const &stat = sink_.write(resampleBuf_, outsamples);
		bool low = stat.fromUnderrun + outsamples < (stat.fromOverflow - outsamples) * 2;
		return Status(stat.rate, low);
	}

private:
	scoped_ptr<Resampler> const resampler_;
	Array<Sint16> const resampleBuf_;
	AudioSink sink_;
};

class FrameWait {
public:
	FrameWait() : last_() {}

	void waitForNextFrameTime(usec_t frametime) {
		last_ += asleep_.sleepUntil(last_, frametime);
		last_ += frametime;
	}

private:
	AdaptiveSleep asleep_;
	usec_t last_;
};

class GetInput : public InputGetter {
public:
	unsigned is;

	GetInput() : is(0) {}
	virtual unsigned operator()() { return is; }
};

class SdlIniter : Uncopyable {
public:
	SdlIniter()
	: failed_(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0)
	{
		if (failed_)
			std::fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
	}

	~SdlIniter() {
		SDL_Quit();
	}

	bool isFailed() const { return failed_; }

private:
	bool const failed_;
};

class JsOpen : Uncopyable {
public:
	template<class InputIterator>
	JsOpen(InputIterator begin, InputIterator end) {
		for (InputIterator at = begin; at != end; ++at) {
			if (SDL_Joystick *j = SDL_JoystickOpen(*at))
				opened_.push_back(j);
		}
	}

	~JsOpen() { std::for_each(opened_.begin(), opened_.end(), SDL_JoystickClose); }

private:
	std::vector<SDL_Joystick *> opened_;
};

class GambatteSdl {
public:
	GambatteSdl() { gambatte.setInputGetter(&inputGetter); }
	int exec(int argc, char const *const argv[]);

private:
	typedef std::multimap<SDLKey,  InputGetter::Button> keymap_t;
	typedef std::multimap<JoyData, InputGetter::Button> jmap_t;

	GetInput inputGetter;
	GB gambatte;
	keymap_t keyMap;
	jmap_t jbMap;
	jmap_t jaMap;
	jmap_t jhMap;

	bool handleEvents(BlitterWrapper &blitter);
	int run(long sampleRate, int latency, int periods,
	        ResamplerInfo const &resamplerInfo, BlitterWrapper &blitter);
};

static void printOptionUsage(DescOption const *const o) {
	std::printf("  ");

	if (char c = o->character())
		std::printf("-%c, ", c);
	else
		std::printf("    ");

	std::printf("--%s%s\n", o->str(), o->desc().c_str());
}

static void printUsage(std::vector<DescOption *> const &v) {
	std::puts("Usage: gambatte_sdl [OPTION]... romfile\n");
	std::for_each(v.begin(), v.end(), printOptionUsage);
}

static void printValidInputKeys() {
	std::puts("Valid input KEYS:");
	printStrSdlkeys();
	static char const      jsnam[] = "jsNaM";
	static char const      jsnhm[] = "jsNhM";
	static char const joystick_n[] = "Joystick N";
	static char const     axis_m[] = "axis M";
	static char const      hat_m[] = "hat M";
	std::printf("%s+\t(%s %s +)\n", jsnam, joystick_n, axis_m);
	std::printf("%s-\t(%s %s -)\n", jsnam, joystick_n, axis_m);
	std::printf("jsNbM\t(%s button M)\n", joystick_n);
	std::printf("%sd\t(%s %s down)\n", jsnhm, joystick_n, hat_m);
	std::printf("%sl\t(%s %s left)\n", jsnhm, joystick_n, hat_m);
	std::printf("%sr\t(%s %s right)\n", jsnhm, joystick_n, hat_m);
	std::printf("%su\t(%s %s up)\n", jsnhm, joystick_n, hat_m);
}

static void printControls() {
	std::puts("Controls:");
	std::puts("TAB\t- fast-forward");
	std::puts("Ctrl-f\t- toggle full screen");
	std::puts("Ctrl-r\t- reset");
	std::puts("F5\t- save state");
	std::puts("F6\t- previous state slot");
	std::puts("F7\t- next state slot");
	std::puts("F8\t- load state");
	std::puts("0 to 9\t- select state slot 0 to 9");
	std::puts("");
	std::puts("Default key mapping:");
	std::puts("Up:\tup");
	std::puts("Down:\tdown");
	std::puts("Left:\tleft");
	std::puts("Right:\tright");
	std::puts("A:\td");
	std::puts("B:\tc");
	std::puts("Start:\treturn");
	std::puts("Select:\trshift");
}

int GambatteSdl::exec(int const argc, char const *const argv[]) {
	std::puts("Gambatte SDL"
#ifdef GAMBATTE_SDL_VERSION_STR
	          " (" GAMBATTE_SDL_VERSION_STR ")"
#endif
	);

	std::set<Uint8> jdevnums;
	BoolOption fsOption("\t\tStart in full screen mode\n", "full-screen", 'f');
	LatencyOption latencyOption;
	PeriodsOption periodsOption;
	RateOption rateOption;
	ResamplerOption resamplerOption;
	ScaleOption scaleOption;
	VfOption vfOption;
	BoolOption yuvOption("\t\tUse YUV overlay for (usually faster) scaling\n",
	                     "yuv-overlay", 'y');
	BoolOption gbaCgbOption("\t\t\tGBA CGB mode\n", "gba-cgb");
	BoolOption forceDmgOption("\t\tForce DMG mode\n", "force-dmg");
	BoolOption multicartCompatOption(
		"\tSupport certain multicart ROM images by\n"
		"\t\t\t\tnot strictly respecting ROM header MBC type\n", "multicart-compat");
	InputOption inputOption;
	int loadIndex = 0;

	{
		BoolOption controlsOption("\t\tShow keyboard controls\n", "controls");
		BoolOption lkOption("\t\tList valid input KEYS\n", "list-keys");
		std::vector<DescOption *> v;
		v.push_back(&controlsOption);
		v.push_back(&gbaCgbOption);
		v.push_back(&forceDmgOption);
		v.push_back(&multicartCompatOption);
		v.push_back(&fsOption);
		v.push_back(&inputOption);
		v.push_back(&latencyOption);
		v.push_back(&lkOption);
		v.push_back(&periodsOption);
		v.push_back(&rateOption);
		v.push_back(&resamplerOption);
		v.push_back(&scaleOption);
		v.push_back(&vfOption);
		v.push_back(&yuvOption);

		Parser parser;
		std::for_each(v.begin(), v.end(),
			std::bind1st(std::mem_fun(&Parser::add), &parser));

		for (int i = 1; i < argc; ++i) {
			if (argv[i][0] == '-') {
				if (!(i = parser.parse(argc, argv, i))) {
					printUsage(v);
					return EXIT_FAILURE;
				}
			} else if (!loadIndex) {
				loadIndex = i;
			}
		}

		if (lkOption.isSet())
			printValidInputKeys();
		if (controlsOption.isSet())
			printControls();

		if (!loadIndex) {
			if (!lkOption.isSet() && !controlsOption.isSet())
				printUsage(v);

			return 0;
		}
	}

	for (std::size_t i = 0; i < inputOption.numMappings(); ++i) {
		std::pair<InputId, InputGetter::Button> const m = inputOption.mapping(i);
		if (m.first.type == InputId::type_key) {
			keyMap.insert(std::make_pair(m.first.keydata, m.second));
		} else {
			jmap_t::value_type pair(m.first.jdata, m.second);
			jdevnums.insert(m.first.jdata.dev_num);

			switch (m.first.type) {
			case InputId::type_jbutton: jbMap.insert(pair); break;
			case InputId::type_jaxis:   jaMap.insert(pair); break;
			case InputId::type_jhat:    jhMap.insert(pair); break;
			default: break;
			}
		}
	}

	if (LoadRes const error =
			gambatte.load(argv[loadIndex],
			                gbaCgbOption.isSet()          * GB::GBA_CGB
			              + forceDmgOption.isSet()        * GB::FORCE_DMG
			              + multicartCompatOption.isSet() * GB::MULTICART_COMPAT)) {
		std::printf("failed to load ROM %s: %s\n", argv[loadIndex], to_string(error).c_str());
		return EXIT_FAILURE;
	}

	{
		PakInfo const &pak = gambatte.pakInfo();
		std::puts(gambatte.romTitle().c_str());
		std::printf("GamePak type: %s rambanks: %u rombanks: %u\n",
		            pak.mbc().c_str(), pak.rambanks(), pak.rombanks());
		std::printf("header checksum: %s\n", pak.headerChecksumOk() ? "ok" : "bad");
		std::printf("cgb: %d\n", gambatte.isCgb());
	}

	SdlIniter sdlIniter;
	if (sdlIniter.isFailed())
		return EXIT_FAILURE;

	if (!jdevnums.empty()
			&& SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		std::fprintf(stderr, "Unable to init joysticks: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	JsOpen jsOpen(jdevnums.begin(), jdevnums.end());
	SDL_JoystickEventState(SDL_ENABLE);
	BlitterWrapper blitter(vfOption.filter(),
	                       scaleOption.scale(), yuvOption.isSet(),
	                       fsOption.isSet());
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption("Gambatte SDL", 0);

	return run(rateOption.rate(), latencyOption.latency(), periodsOption.periods(),
	           resamplerOption.resampler(), blitter);
}

bool GambatteSdl::handleEvents(BlitterWrapper &blitter) {
	JoyData jd;
	SDL_Event e;

	while (SDL_PollEvent(&e)) switch (e.type) {
	case SDL_JOYAXISMOTION:
		jd.dev_num = e.jaxis.which;
		jd.num = e.jaxis.axis;
		jd.dir = e.jaxis.value < -8192
		       ? JoyData::dir_down
		       : (e.jaxis.value > 8192 ? JoyData::dir_up : JoyData::dir_centered);

		for (std::pair<jmap_t::iterator, jmap_t::iterator> range =
				jaMap.equal_range(jd); range.first != range.second; ++range.first) {
			if (jd.dir == range.first->first.dir)
				inputGetter.is |= range.first->second;
			else
				inputGetter.is &= ~range.first->second;
		}

		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		jd.dev_num = e.jbutton.which;
		jd.num = e.jbutton.button;

		for (std::pair<jmap_t::iterator, jmap_t::iterator> range =
				jbMap.equal_range(jd); range.first != range.second; ++range.first) {
			if (e.jbutton.state)
				inputGetter.is |= range.first->second;
			else
				inputGetter.is &= ~range.first->second;
		}

		break;
	case SDL_JOYHATMOTION:
		jd.dev_num = e.jhat.which;
		jd.num = e.jhat.hat;

		for (std::pair<jmap_t::iterator, jmap_t::iterator> range =
				jhMap.equal_range(jd); range.first != range.second; ++range.first) {
			if (e.jhat.value & range.first->first.dir)
				inputGetter.is |= range.first->second;
			else
				inputGetter.is &= ~range.first->second;
		}

		break;
	case SDL_KEYDOWN:
		if (e.key.keysym.mod & KMOD_CTRL) {
			switch (e.key.keysym.sym) {
			case SDLK_f: blitter.toggleFullScreen(); break;
			case SDLK_r: gambatte.reset(); break;
			default: break;
			}
		} else {
			switch (e.key.keysym.sym) {
			case SDLK_ESCAPE:
				return true;
			case SDLK_F5:
				gambatte.saveState(blitter.inBuf().pixels, blitter.inBuf().pitch);
				break;
			case SDLK_F6: gambatte.selectState(gambatte.currentState() - 1); break;
			case SDLK_F7: gambatte.selectState(gambatte.currentState() + 1); break;
			case SDLK_F8: gambatte.loadState(); break;
			case SDLK_0: gambatte.selectState(0); break;
			case SDLK_1: gambatte.selectState(1); break;
			case SDLK_2: gambatte.selectState(2); break;
			case SDLK_3: gambatte.selectState(3); break;
			case SDLK_4: gambatte.selectState(4); break;
			case SDLK_5: gambatte.selectState(5); break;
			case SDLK_6: gambatte.selectState(6); break;
			case SDLK_7: gambatte.selectState(7); break;
			case SDLK_8: gambatte.selectState(8); break;
			case SDLK_9: gambatte.selectState(9); break;
			default: break;
			}
		}
		// fallthrough
	case SDL_KEYUP:
		for (std::pair<keymap_t::iterator, keymap_t::iterator> range =
				keyMap.equal_range(e.key.keysym.sym);
				range.first != range.second; ++range.first) {
			if (e.key.state)
				inputGetter.is |= range.first->second;
			else
				inputGetter.is &= ~range.first->second;
		}

		break;
	case SDL_QUIT:
		return true;
	}

	return false;
}

static std::size_t const gb_samples_per_frame = 35112;
static std::size_t const gambatte_max_overproduction = 2064;

static bool isFastForward(Uint8 const *keys) {
	return keys[SDLK_TAB];
}

int GambatteSdl::run(long const sampleRate, int const latency, int const periods,
                     ResamplerInfo const &resamplerInfo, BlitterWrapper &blitter) {
	Array<Uint32> const audioBuf(gb_samples_per_frame + gambatte_max_overproduction);
	AudioOut aout(sampleRate, latency, periods, resamplerInfo, audioBuf.size());
	FrameWait frameWait;
	SkipSched skipSched;
	Uint8 const *const keys = SDL_GetKeyState(0);
	std::size_t bufsamples = 0;
	bool audioOutBufLow = false;

	SDL_PauseAudio(0);

	for (;;) {
		if (handleEvents(blitter))
			return 0;

		BlitterWrapper::Buf const &vbuf = blitter.inBuf();
		std::size_t runsamples = gb_samples_per_frame - bufsamples;
		std::ptrdiff_t const vidFrameDoneSampleCnt = gambatte.runFor(
			vbuf.pixels, vbuf.pitch, audioBuf + bufsamples, runsamples);
		std::size_t const outsamples = vidFrameDoneSampleCnt >= 0
		                             ? bufsamples + vidFrameDoneSampleCnt
		                             : bufsamples + runsamples;
		bufsamples += runsamples;
		bufsamples -= outsamples;

		if (isFastForward(keys)) {
			if (vidFrameDoneSampleCnt >= 0) {
				blitter.draw();
				blitter.present();
			}
		} else {
			bool const blit = vidFrameDoneSampleCnt >= 0
			               && !skipSched.skipNext(audioOutBufLow);
			if (blit)
				blitter.draw();

			AudioOut::Status const &astatus = aout.write(audioBuf, outsamples);
			audioOutBufLow = astatus.low;
			if (blit) {
				usec_t ft = (16743ul - 16743 / 1024) * sampleRate / astatus.rate;
				frameWait.waitForNextFrameTime(ft);
				blitter.present();
			}
		}

		std::memmove(audioBuf, audioBuf + outsamples, bufsamples * sizeof *audioBuf);
	}

	return 0;
}

} // anon namespace

int main(int argc, char **argv) {
	GambatteSdl gambatteSdl;
	return gambatteSdl.exec(argc, argv);
}
