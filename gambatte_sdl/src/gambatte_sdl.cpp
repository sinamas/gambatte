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
#include <gambatte.h>
#include <array.h>
#include <resample/resamplerinfo.h>
#include <rateest.h>
#include <SDL.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

#include "audiodata.h"
#include "syncfunc.h"
#include "parser.h"
#include "str_to_sdlkey.h"
#include "sdlblitter.h"

using namespace Gambatte;

struct DescOption : Parser::Option {
	DescOption(const char *s, char c = 0, int nArgs = 0) : Option(s, c, nArgs) {}
	virtual ~DescOption() {}
	virtual const char* getDesc() const = 0;
};

class FsOption : public DescOption {
	bool full;
public:
	FsOption() : DescOption("full-screen", 'f'), full(false) {}
	void exec(const char *const */*argv*/, int /*index*/) { full = true; }
	const char* getDesc() const { return "\t\tStart in full screen mode\n"; }
	bool startFull() const { return full; }
};

class ListKeysOption : public DescOption {
	bool execed;
public:
	ListKeysOption() : DescOption("list-keys"), execed(false) {}
	void exec(const char *const */*argv*/, int /*index*/) { execed = true; }
	const char* getDesc() const { return "\t\tList valid input KEYS\n"; }
	bool isExeced() const { return execed; }
};

class RateOption : public DescOption {
	unsigned rate;
public:
	RateOption() : DescOption("sample-rate", 'r', 1), rate(48000) {}
	
	void exec(const char *const *argv, int index) {
		int tmp = std::atoi(argv[index + 1]);
		
		if (tmp < 4000 || tmp > 192000)
			return;
		
		rate = tmp;
	}
	
	const char* getDesc() const { return " N\t\tUse audio sample rate of N Hz\n\t\t\t\t    4000 <= N <= 192000, default: 48000\n"; }
	unsigned getRate() const { return rate; }
};

class LatencyOption : public DescOption {
	unsigned latency;
public:
	LatencyOption() : DescOption("latency", 'l', 1), latency(133) {}
	
	void exec(const char *const *argv, int index) {
		int tmp = std::atoi(argv[index + 1]);
		
		if (tmp < 16 || tmp > 5000)
			return;
		
		latency = tmp;
	}
	
	const char* getDesc() const { return " N\t\tUse audio buffer latency of N ms\n\t\t\t\t    16 <= N <= 5000, default: 133\n"; }
	unsigned getLatency() const { return latency; }
};

class PeriodsOption : public DescOption {
	unsigned periods;
public:
	PeriodsOption() : DescOption("periods", 'p', 1), periods(4) {}
	
	void exec(const char *const *argv, int index) {
		int tmp = std::atoi(argv[index + 1]);
		
		if (tmp < 1 || tmp > 32)
			return;
		
		periods = tmp;
	}
	
	const char* getDesc() const { return " N\t\tUse N audio buffer periods\n\t\t\t\t    1 <= N <= 32, default: 4\n"; }
	unsigned getPeriods() const { return periods; }
};

class ScaleOption : public DescOption {
	Uint8 scale;
public:
	ScaleOption() : DescOption("scale", 's', 1), scale(1) {}
	
	void exec(const char *const *argv, int index) {
		int tmp = std::atoi(argv[index + 1]);
		
		if (tmp < 1 || tmp > 40)
			return;
		
		scale = tmp;
	}
	
	const char* getDesc() const { return " N\t\t\tScale video output by an integer factor of N\n"; }
	Uint8 getScale() const { return scale; }
};

class VfOption : public DescOption {
	std::string s;
	unsigned filterNr;
public:
	VfOption(const std::vector<const FilterInfo*> &finfo);
	void exec(const char *const *argv, int index) { filterNr = std::atoi(argv[index + 1]); }
	const char* getDesc() const { return s.c_str(); }
	unsigned filterNumber() const { return filterNr; }
};

VfOption::VfOption(const std::vector<const FilterInfo*> &finfo) : DescOption("video-filter", 'v', 1), filterNr(0) {
	std::stringstream ss;
	ss << " N\t\tUse video filter number N\n";
	
	for (std::size_t i = 0; i < finfo.size(); ++i) {
		ss << "\t\t\t\t    " << i << " = " << finfo[i]->handle << "\n";
	}
	
	s = ss.str();
}

class YuvOption : public DescOption {
	bool yuv;
public:
	YuvOption() : DescOption("yuv-overlay", 'y'), yuv(false) {}
	void exec(const char *const */*argv*/, int /*index*/) { yuv = true; }
	const char* getDesc() const { return "\t\tUse YUV overlay for (usually faster) scaling\n"; }
	bool useYuv() const { return yuv; }
};

class ResamplerOption : public DescOption {
	std::string s;
	unsigned resamplerNo;
public:
	ResamplerOption();
	
	void exec(const char *const *argv, int index) {
		const unsigned tmp = std::atoi(argv[index + 1]);
		
		if (tmp < ResamplerInfo::num())
			resamplerNo = tmp;
	}
	
	const char* getDesc() const { return s.c_str(); }
	unsigned resamplerNumber() const { return resamplerNo; }
};

ResamplerOption::ResamplerOption() : DescOption("resampler", 0, 1), resamplerNo(1) {
	std::stringstream ss;
	ss << " N\t\tUse audio resampler number N\n";
	
	for (std::size_t i = 0; i < ResamplerInfo::num(); ++i) {
		ss << "\t\t\t\t    " << i << " = " << ResamplerInfo::get(i).desc;
		
		if (i == resamplerNo)
			ss << " [default]";
		
		ss << "\n";
	}
	
	s = ss.str();
}

struct JoyData {
	enum { CENTERED = SDL_HAT_CENTERED, LEFT = SDL_HAT_LEFT, RIGHT = SDL_HAT_RIGHT, UP = SDL_HAT_UP, DOWN = SDL_HAT_DOWN };
	
	union {
		struct {
			Uint8 dev_num;
			Uint8 num;
		};
		
		Uint16 id;
	};
	
	Sint16 dir;
};

static inline bool operator<(const JoyData &l, const JoyData &r) {
	return l.id < r.id; 
}

class InputOption : public DescOption {
public:
	struct InputId {
		enum { KEY, JOYBUT, JOYAX, JOYHAT } type;
		
		union {
			JoyData jdata;
			SDLKey key;
		};
		
		InputId() : type(KEY) {}
	};
	
private:
	InputId keys[8];
	
public:
	InputOption();
	void exec(const char *const *argv, int index);
	const char* getDesc() const { return " KEYS\t\tUse the 8 given input KEYS for respectively\n\t\t\t\t    START SELECT A B UP DOWN LEFT RIGHT\n"; }
	const InputId* getKeys() const { return keys; }
};

InputOption::InputOption() : DescOption("input", 'i', 8) {
	keys[0].key = SDLK_RETURN;
	keys[1].key  = SDLK_RSHIFT;
	keys[2].key  = SDLK_d;
	keys[3].key  = SDLK_c;
	keys[4].key  = SDLK_UP;
	keys[5].key  = SDLK_DOWN;
	keys[6].key  = SDLK_LEFT;
	keys[7].key  = SDLK_RIGHT;
}

void InputOption::exec(const char *const *argv, int index) {
	++index;
	
	for (unsigned i = 0; i < 8; ++i) {
		const char *s = argv[index + i];
	
		if (s[0] == 'j' && s[1] == 's') {
			s += 2;
			const char *const send = s + std::strlen(s);
			
			if (send - s < 3)
				continue;
			
			const int dev_num = std::atoi(s++);
			
			if (dev_num < 0 || dev_num > 255)
				continue;
			
			s += (dev_num > 9) + (dev_num > 99);
			
			if (send - s < 2)
				continue;
			
			const char type = *s++;
			const int num = std::atoi(s++);
			
			if (num < 0 || num > 255)
				continue;
			
			s += (dev_num > 9) + (dev_num > 99);
			
			InputId id;
			id.jdata.dev_num = dev_num;
			id.jdata.num = num;
			
			if (type == 'b') {
				if (send - s != 0)
					continue;
					
				id.type = InputId::JOYBUT;
			} else {
				if (send - s != 1)
					continue;
				
				const char dir = *s;
				
				switch (type) {
				case 'a':
					switch (dir) {
					case '+': id.jdata.dir = JoyData::UP; break;
					case '-': id.jdata.dir = JoyData::DOWN; break;
					default: continue;
					}
					
					id.type = InputId::JOYAX;
					break;
				case 'h':
					switch (dir) {
					case 'u': id.jdata.dir = JoyData::UP;
					case 'd': id.jdata.dir = JoyData::DOWN;
					case 'l': id.jdata.dir = JoyData::LEFT;
					case 'r': id.jdata.dir = JoyData::RIGHT;
					default: continue;
					}
					
					id.type = InputId::JOYHAT;
					break;
				default: continue;
				}
			}
			
			keys[i] = id;
		} else {
			const SDLKey *const k = strToSdlkey(s);
			
			if (k) {
				keys[i].type = InputId::KEY;
				keys[i].key = *k;
			}
		}
	}
}

class InputGetter : public InputStateGetter {
public:
	InputState is;
	
	InputGetter() { memset(&is, 0, sizeof(is)); }
	
	const InputState& operator()() {
		return is;
	}
};

class SdlIniter {
	bool failed;
public:
	SdlIniter();
	~SdlIniter();
	bool isFailed() const { return failed; }
};

SdlIniter::SdlIniter() : failed(false) {
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
		std::fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		failed = true;
	}
}

SdlIniter::~SdlIniter() {
	SDL_Quit();
}

class GambatteSdl {
	typedef std::multimap<SDLKey,bool*> keymap_t;
	typedef std::multimap<JoyData,bool*> jmap_t;
	
	Array<Sint16> inBuf;
	Array<Sint16> tmpBuf;
	std::auto_ptr<Resampler> resampler;
	GB gambatte;
	SdlIniter sdlIniter;
	SdlBlitter blitter;
	InputGetter inputGetter;
	keymap_t keyMap;
	jmap_t jbMap;
	jmap_t jaMap;
	jmap_t jhMap;
	std::vector<SDL_Joystick*> joysticks;
	unsigned sampleRate;
	unsigned latency;
	unsigned periods;
	bool failed;
	
	bool init(int argc, char **argv);
	
public:
	GambatteSdl(int argc, char **argv);
	~GambatteSdl();
	int exec();
};

GambatteSdl::GambatteSdl(int argc, char **argv) : inBuf((35112 + 2064) * 2), sampleRate(48000) {
	failed = init(argc, argv);
}

GambatteSdl::~GambatteSdl() {
	for (std::size_t i = 0; i < joysticks.size(); ++i)
		SDL_JoystickClose(joysticks[i]);
}

static void printUsage(std::vector<DescOption*> &v) {
	std::printf("Usage: gambatte_sdl [OPTION]... romfile\n\n");
	
	for (std::size_t i = 0; i < v.size(); ++i) {
		std::printf("  ");
		
		if (v[i]->getChar())
			std::printf("-%c, ", v[i]->getChar());
		else
			std::printf("    ");
		
		std::printf("--%s%s\n", v[i]->getStr(), v[i]->getDesc());
	}
}

bool GambatteSdl::init(int argc, char **argv) {
	std::printf("Gambatte SDL svn\n");
	
	if (sdlIniter.isFailed())
		return 1;
	
	std::vector<Uint8> jdevnums;
	
	{
		Parser parser;
		
		std::vector<DescOption*> v;
		FsOption fsOption;
		v.push_back(&fsOption);
		InputOption inputOption;
		v.push_back(&inputOption);
		LatencyOption latencyOption;
		v.push_back(&latencyOption);
		ListKeysOption lkOption;
		v.push_back(&lkOption);
		PeriodsOption periodsOption;
		v.push_back(&periodsOption);
		RateOption rateOption;
		v.push_back(&rateOption);
		ResamplerOption resamplerOption;
		v.push_back(&resamplerOption);
		ScaleOption scaleOption;
		v.push_back(&scaleOption);
		VfOption vfOption(gambatte.filterInfo());
		v.push_back(&vfOption);
		YuvOption yuvOption;
		v.push_back(&yuvOption);
		
		for (std::size_t i = 0; i < v.size(); ++i) {
			parser.add(v[i]);
		}
		
		unsigned loadIndex = 0;
		
		for (int i = 1; i < argc; ++i) {
			if (argv[i][0] == '-') {
				if (!(i = parser.parse(argc, argv, i))) {
					printUsage(v);
					return 1;
				}
			} else if (!loadIndex) {
				loadIndex = i;
			}
		}
		
		if (lkOption.isExeced()) {
			std::printf("Valid input KEYS:\n");
			printStrSdlkeys();
			static const char *const jsnam = "jsNaM";
			static const char *const jsnhm = "jsNhM";
			static const char *const joystick_n = "Joystick N";
			static const char *const axis_m = "axis M";
			static const char *const hat_m = "hat M";
			std::printf("%s+\t(%s %s +)\n", jsnam, joystick_n, axis_m);
			std::printf("%s-\t(%s %s -)\n", jsnam, joystick_n, axis_m);
			std::printf("jsNbM\t(%s button M)\n", joystick_n);
			std::printf("%sd\t(%s %s down)\n", jsnhm, joystick_n, hat_m);
			std::printf("%sl\t(%s %s left)\n", jsnhm, joystick_n, hat_m);
			std::printf("%sr\t(%s %s right)\n", jsnhm, joystick_n, hat_m);
			std::printf("%su\t(%s %s up)\n", jsnhm, joystick_n, hat_m);
			return 1;
		}
		
		if (!loadIndex) {
			printUsage(v);
			return 1;
		}
		
		if (gambatte.load(argv[loadIndex])) {
			std::printf("failed to load ROM %s\n", argv[loadIndex]);
			return 1;
		}
		
		if (fsOption.startFull())
			blitter.setStartFull();
		
		sampleRate = rateOption.getRate();
		latency = latencyOption.getLatency();
		periods = periodsOption.getPeriods();
		blitter.setScale(scaleOption.getScale());
		blitter.setYuv(yuvOption.useYuv());
		gambatte.setVideoFilter(vfOption.filterNumber());
		resampler.reset(ResamplerInfo::get(resamplerOption.resamplerNumber()).create(2097152, sampleRate, 35112));
		
		bool* gbbuts[8] = {
			&inputGetter.is.startButton, &inputGetter.is.selectButton,
			&inputGetter.is.aButton, &inputGetter.is.bButton,
			&inputGetter.is.dpadUp, &inputGetter.is.dpadDown,
			&inputGetter.is.dpadLeft, &inputGetter.is.dpadRight
		};
		
		for (unsigned i = 0; i < 8; ++i) {
			const InputOption::InputId &id = inputOption.getKeys()[i];
			
			if (id.type == InputOption::InputId::KEY)
				keyMap.insert(std::pair<SDLKey,bool*>(id.key, gbbuts[i]));
			else {
				std::pair<JoyData,bool*> pair(id.jdata, gbbuts[i]);
				jdevnums.push_back(id.jdata.dev_num);
				
				switch (id.type) {
				case InputOption::InputId::JOYBUT: jbMap.insert(pair); break;
				case InputOption::InputId::JOYAX: jaMap.insert(pair); break;
				case InputOption::InputId::JOYHAT: jhMap.insert(pair); break;
				default: break;
				}
			}
		}
	}
	
	gambatte.setInputStateGetter(&inputGetter);
	
	if (!jdevnums.empty()) {
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
			std::fprintf(stderr, "Unable to init joysticks: %s\n", SDL_GetError());
			return 1;
		}
	}
	
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption("Gambatte SDL", NULL);
	
	for (std::size_t i = 0; i < jdevnums.size(); ++i) {
		SDL_Joystick *const j = SDL_JoystickOpen(i);
		
		if (j)
			joysticks.push_back(j);
	}
	
	SDL_JoystickEventState(SDL_ENABLE);
	
	return 0;
}

int GambatteSdl::exec() {
	if (failed)
		return 1;
	
	AudioData adata(sampleRate, latency, periods);
	tmpBuf.reset(resampler->maxOut(35112) * 2);
	
	gambatte.setVideoBlitter(&blitter);
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	unsigned samples = 0;
	
	SDL_PauseAudio(0);
	
	for (;;) {
		{
			JoyData jd;
			SDL_Event e;
			
			while (SDL_PollEvent(&e)) {
				switch (e.type) {
				case SDL_JOYAXISMOTION:
					jd.dev_num = e.jaxis.which;
					jd.num = e.jaxis.axis;
					jd.dir = e.jaxis.value < -8192 ? JoyData::DOWN : (e.jaxis.value > 8192 ? JoyData::UP : JoyData::CENTERED);
					
					for (std::pair<jmap_t::iterator,jmap_t::iterator> range(jaMap.equal_range(jd));
					     range.first != range.second; ++range.first) {
						*range.first->second = jd.dir == range.first->first.dir;
					}
					
					break;
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					jd.dev_num = e.jbutton.which;
					jd.num = e.jbutton.button;
					
					for (std::pair<jmap_t::iterator,jmap_t::iterator> range(jbMap.equal_range(jd));
					     range.first != range.second; ++range.first) {
						*range.first->second = e.jbutton.state;
					}
					
					break;
				case SDL_JOYHATMOTION:
					jd.dev_num = e.jhat.which;
					jd.num = e.jhat.hat;
					
					for (std::pair<jmap_t::iterator,jmap_t::iterator> range(jaMap.equal_range(jd));
					     range.first != range.second; ++range.first) {
						*range.first->second = e.jhat.value & range.first->first.dir;
					}
					
					break;
				case SDL_KEYDOWN:
					if (e.key.keysym.mod & KMOD_CTRL) {
						switch (e.key.keysym.sym) {
						case SDLK_f:
							blitter.toggleFullScreen();
							gambatte.videoBufferChange();
							break;
						case SDLK_r:
							gambatte.reset();
							break;
						default: break;
						}
					} else {
						switch (e.key.keysym.sym) {
						case SDLK_ESCAPE: return 0;
						case SDLK_F5: gambatte.saveState(); break;
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
				case SDL_KEYUP:
					for (std::pair<keymap_t::iterator,keymap_t::iterator> range(keyMap.equal_range(e.key.keysym.sym));
					     range.first != range.second; ++range.first) {
						*range.first->second = e.key.state;
					}
					
					break;
				case SDL_QUIT:
					return 0;
				}
			}
		}
		
		samples += gambatte.runFor(reinterpret_cast<Gambatte::uint_least32_t*>(static_cast<Sint16*>(inBuf)) + samples, 35112 - samples);
		samples -= 35112;
		
		if (!keys[SDLK_TAB]) {
			const AudioData::Status &status = adata.write(tmpBuf, resampler->resample(tmpBuf, inBuf, 35112));
			
			long ft = (16743ul - (16743 / 1024)) * sampleRate / status.rate.est;
			
			if (status.fromUnderrun < status.fromOverflow)
				ft >>= 1;
			
			syncfunc(ft);
		}
		
		std::memmove(inBuf, inBuf + 35112 * 2, samples * sizeof(Sint16) * 2);
	}
	
	return 0;
}

int main(int argc, char **argv) {
	GambatteSdl gambatteSdl(argc, argv);
	
	return gambatteSdl.exec();
}
