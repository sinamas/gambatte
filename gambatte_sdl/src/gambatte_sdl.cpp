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
#include <gambatte.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <cstring>
#include <string>
#include <sstream>

#include "syncfunc.h"
#include "parser.h"
#include "str_to_sdlkey.h"
#include "sdlblitter.h"

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

class RateOption : public DescOption {
	unsigned rate;
	
public:
	RateOption() : DescOption("sample-rate", 'r', 1), rate(48000) {}
	
	void exec(const char *const *argv, int index) {
		int tmp = std::atoi(argv[index + 1]);
		
		if (tmp < 32000 || tmp > 192000)
			return;
		
		rate = tmp;
	}
	
	const char* getDesc() const { return " N\t\tUse audio sample rate of N Hz\n\t\t\t\t    32000 <= N <= 192000, default: 48000\n"; }
	unsigned getRate() const { return rate; }
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
	const char* getDesc() const { return "\t\tUse YUV Overlay for (usually faster) scaling\n"; }
	bool useYuv() const { return yuv; }
};

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

static unsigned ceiledPowerOf2(unsigned t) {
	--t;
	t |= t >> 1;
	t |= t >> 2;
	t |= t >> 4;
	t |= t >> 8;
	t |= t >> 16;
	++t;
	
	return t;
}

static void fill_buffer(void *buffer, Uint8 *stream, int len);

struct AudioData {
	const unsigned samplesPrFrame;
	
	AudioData(unsigned sampleRate);
	~AudioData();
	void write(const Uint16 *inBuf);
	void read(Uint8 *stream, int len);
	
private:
	const unsigned bufSz;
	SDL_mutex *const mut;
	SDL_cond *const bufReadyCond;
	Sint16 *const buffer;
	unsigned rPos;
	unsigned wPos;
};

AudioData::AudioData(const unsigned srate) :
samplesPrFrame(((srate * 4389) / 262144) + 2),
bufSz(ceiledPowerOf2(samplesPrFrame) * 8),
mut(SDL_CreateMutex()),
bufReadyCond(SDL_CreateCond()),
buffer(new Sint16[bufSz]),
rPos(0),
wPos(0) {
	std::memset(buffer, 0, bufSz * sizeof(Sint16));
	
	SDL_AudioSpec spec;
	spec.freq = srate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = bufSz >> 3;
	spec.callback = fill_buffer;
	spec.userdata = this;
	SDL_OpenAudio(&spec, NULL);
}

AudioData::~AudioData() {
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	delete []buffer;
	SDL_DestroyCond(bufReadyCond);
	SDL_DestroyMutex(mut);
}

void AudioData::write(const Uint16 *const inBuf) {
	SDL_mutexP(mut);
	
	if (rPos - wPos + (wPos > rPos ? bufSz : 0) >> 1 < samplesPrFrame)
		SDL_CondWait(bufReadyCond, mut);
	
	{
		const unsigned samples1 = std::min(bufSz - wPos >> 1, samplesPrFrame);
		const unsigned samples2 = samplesPrFrame - samples1;
		
		std::memcpy(buffer + wPos, inBuf, samples1 * 4);
		
		if (samples2)
			std::memcpy(buffer, inBuf + samples1 * 2, samples2 * 4);
		
		if ((wPos += samplesPrFrame * 2) >= bufSz)
			wPos -= bufSz;
	}
	
	SDL_mutexV(mut);
}

void AudioData::read(Uint8 *const stream, const int len) {
	SDL_mutexP(mut);
	
	const unsigned bytes1 = std::min(static_cast<unsigned>(len), (bufSz - rPos) * 2);
	const unsigned bytes2 = len - bytes1;
	
	std::memcpy(stream, buffer + rPos, bytes1);
	
	if (bytes2)
		std::memcpy(stream + bytes1, buffer, bytes2);
	
	if ((rPos += len >> 1) >= bufSz)
		rPos -= bufSz;
	
	if (rPos - wPos + (wPos > rPos ? bufSz : 0) >> 1 >= samplesPrFrame)
		SDL_CondSignal(bufReadyCond);
	
	SDL_mutexV(mut);
}

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
	
	Uint16 *tmpBuf;
	Gambatte gambatte;
	SdlIniter sdlIniter;
	SdlBlitter blitter;
	InputGetter inputGetter;
	keymap_t keyMap;
	jmap_t jbMap;
	jmap_t jaMap;
	jmap_t jhMap;
	std::vector<SDL_Joystick*> joysticks;
	unsigned sampleRate;
	bool failed;
	
	bool init(int argc, char **argv);
	
public:
	GambatteSdl(int argc, char **argv);
	~GambatteSdl();
	int exec();
};

GambatteSdl::GambatteSdl(int argc, char **argv) : tmpBuf(NULL), sampleRate(48000) {
	failed = init(argc, argv);
}

GambatteSdl::~GambatteSdl() {
	delete []tmpBuf;
	
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
			std::printf("     ");
		
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
		RateOption rateOption;
		v.push_back(&rateOption);
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
		blitter.setScale(scaleOption.getScale());
		blitter.setYuv(yuvOption.useYuv());
		gambatte.setVideoFilter(vfOption.filterNumber());
		
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
	
	AudioData adata(sampleRate);
	tmpBuf = new Uint16[adata.samplesPrFrame * 2];
	
	gambatte.setVideoBlitter(&blitter);
	
	Uint8 *keys = SDL_GetKeyState(NULL);
	
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
					if (e.key.keysym.sym == SDLK_ESCAPE)
						return 0;
					
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
		
		gambatte.runFor(70224);
		
		if (!keys[SDLK_TAB]) {
			gambatte.fill_buffer(tmpBuf, adata.samplesPrFrame);
			
			adata.write(tmpBuf);
			
			syncFunc();
		} else
			gambatte.fill_buffer(0, 0);
	}
	
	return 0;
}

int main(int argc, char **argv) {
	GambatteSdl gambatteSdl(argc, argv);
	
	return gambatteSdl.exec();
}

static void fill_buffer(void *const data, Uint8 *const stream, const int len) {
	reinterpret_cast<AudioData*>(data)->read(stream, len);
}
