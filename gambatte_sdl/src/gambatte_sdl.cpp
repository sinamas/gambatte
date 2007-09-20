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
#include <cstring>
#include <string>
#include <sstream>

#include "syncfunc.h"
#include "parser.h"
#include "str_to_sdlkey.h"
#include "sdlblitter.h"

static Gambatte gambatte;

struct FatOption {
	virtual Parser::Option* getShort() { return NULL; };
	virtual Parser::Option* getLong() = 0;
	virtual const char* getDesc() const = 0;
	virtual ~FatOption() {}
};

class FsOption : public FatOption {
	class Main : public Parser::Option {
		bool &full;
	protected:
		Main(const char *const s, bool &full) : Option(s), full(full) {}
	public:
		void exec(const char *const */*argv*/, int /*index*/) { full = true; }
	};
	
	struct Short : Main {
		Short(bool &full) : Main("f", full) {}
	};
	
	struct Long : Main {
		Long(bool &full) : Main("full-screen", full) {}
	};
	
	Short sOpt;
	Long lOpt;
	bool full;
	
public:
	FsOption() : sOpt(full), lOpt(full), full(false) {}
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
	const char* getDesc() const { return "\t\tStart in full screen mode\n"; }
	bool startFull() const { return full; }
};

class ScaleOption : public FatOption {
	class Main : public Parser::Option {
		Uint8 &scale;
	protected:
		Main(const char *const s, Uint8 &scale) : Option(s, 1), scale(scale) {}
	public:
		void exec(const char *const *argv, int index) {
			int tmp = std::atoi(argv[index + 1]);
			
			if (tmp < 1 || tmp > 40)
				return;
			
			scale = tmp;
		}
	};
	
	struct Short : Main {
		Short(Uint8 &scale) : Main("s", scale) {}
	};
	
	struct Long : Main {
		Long(Uint8 &scale) : Main("scale", scale) {}
	};
	
	Short sOpt;
	Long lOpt;
	Uint8 scale;
	
public:
	ScaleOption() : sOpt(scale), lOpt(scale), scale(1) {}
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
	const char* getDesc() const { return " N\t\t\tScale video output by an integer factor of N\n"; }
	Uint8 getScale() const { return scale; }
};

class VfOption : public FatOption {
	class Main : public Parser::Option {
	protected:
		Main(const char *const s) : Option(s, 1) {}
	public:
		void exec(const char *const *argv, int index) {
			gambatte.setVideoFilter(std::atoi(argv[index + 1]));
		}
	};
	
	struct Short : Main {
		Short() : Main("v") {}
	};
	
	struct Long : Main {
		Long() : Main("video-filter") {}
	};
	
	Short sOpt;
	Long lOpt;
	std::string s;
	
public:
	VfOption();
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
	const char* getDesc() const { return s.c_str(); }
};

VfOption::VfOption() {
	const std::vector<const FilterInfo*> &finfo = gambatte.filterInfo();
	std::stringstream ss;
	ss << " N\t\tUse video filter number N\n";
	
	for (std::size_t i = 0; i < finfo.size(); ++i) {
		ss << "\t\t\t\t    " << i << " = " << finfo[i]->handle << "\n";
	}
	
	s = ss.str();
}

class YuvOption : public FatOption {
	class Main : public Parser::Option {
		bool &yuv;
	protected:
		Main(const char *const s, bool &yuv) : Option(s), yuv(yuv) {}
	public:
		void exec(const char *const */*argv*/, int /*index*/) { yuv = true; }
	};
	
	struct Short : Main {
		Short(bool &yuv) : Main("y", yuv) {}
	};
	
	struct Long : Main {
		Long(bool &yuv) : Main("yuv-overlay", yuv) {}
	};
	
	Short sOpt;
	Long lOpt;
	bool yuv;
	
public:
	YuvOption() : sOpt(yuv), lOpt(yuv), yuv(false) {}
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
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

class InputOption : public FatOption {
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
	class Main : public Parser::Option {
		InputId *const keys;
		StrToSdlkey strToKey;
		
	protected:
		Main(const char *const s, InputId keys[]) : Option(s, 8), keys(keys) {}
	public:
		void exec(const char *const *argv, int index);
	};
	
	struct Short : Main {
		Short(InputId keys[]) : Main("i", keys) {}
	};
	
	struct Long : Main {
		Long(InputId keys[]) : Main("input", keys) {}
	};
	
	Short sOpt;
	Long lOpt;
	InputId keys[8];
	
public:
	InputOption();
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
	const char* getDesc() const { return " KEYS\t\tUse the 8 given input KEYS for respectively\n\t\t\t\t    START SELECT A B UP DOWN LEFT RIGHT\n"; }
	const InputId* getKeys() const { return keys; }
};

InputOption::InputOption() : sOpt(keys), lOpt(keys) {
	keys[0].key = SDLK_RETURN;
	keys[1].key  = SDLK_RSHIFT;
	keys[2].key  = SDLK_d;
	keys[3].key  = SDLK_c;
	keys[4].key  = SDLK_UP;
	keys[5].key  = SDLK_DOWN;
	keys[6].key  = SDLK_LEFT;
	keys[7].key  = SDLK_RIGHT;
}

void InputOption::Main::exec(const char *const *argv, int index) {
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
			const SDLKey *const k = strToKey(s);
			
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

static void fill_buffer(void *buffer, Uint8 *stream, int len);

static const unsigned SAMPLE_RATE = 48000;
static const unsigned SAMPLES = ((SAMPLE_RATE * 4389) / 262144) + 2;
static unsigned rPos = 0;
static unsigned wPos = 0;
static bool bufAvailable = false;

static const unsigned SND_BUF_SZ = 4096 * 2;

static const char *const usage = "Usage: gambatte_sdl romfile\n";

static void printUsage(std::vector<FatOption*> &v) {
	printf("Usage: gambatte_sdl [OPTION]... romfile\n\n");
	
	for (unsigned i = 0; i < v.size(); ++i) {
		printf("  -%s, --%s%s\n", v[i]->getShort()->getStr(), v[i]->getLong()->getStr(), v[i]->getDesc());
	}
}

int main(int argc, char **argv) {
	printf("Gambatte SDL svn\n");
	
	SdlBlitter blitter;
	InputGetter inputGetter;
	
	std::multimap<SDLKey,bool*> keyMap;
	std::multimap<JoyData,bool*> jbMap;
	std::multimap<JoyData,bool*> jaMap;
	std::multimap<JoyData,bool*> jhMap;
	std::vector<SDL_Joystick*> joysticks;
	std::vector<Uint8> jdevnums;
	
	{
		Parser parser;
		
		std::vector<FatOption*> v;
		FsOption fsOption;
		v.push_back(&fsOption);
		InputOption inputOption;
		v.push_back(&inputOption);
		ScaleOption scaleOption;
		v.push_back(&scaleOption);
		VfOption vfOption;
		v.push_back(&vfOption);
		YuvOption yuvOption;
		v.push_back(&yuvOption);
		
		for (unsigned i = 0; i < v.size(); ++i) {
			parser.add(v[i]->getShort());
			parser.add(v[i]->getLong());
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
			printf("failed to load ROM %s\n", argv[loadIndex]);
			return 1;
		}
		
		if (fsOption.startFull())
			blitter.setStartFull();
		
		blitter.setScale(scaleOption.getScale());
		blitter.setYuv(yuvOption.useYuv());
		
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

	Sint16 *const sndBuffer = new Sint16[SND_BUF_SZ];
	std::memset(sndBuffer, 0, SND_BUF_SZ * sizeof(Sint16));
	uint16_t *const tmpBuf = new uint16_t[SAMPLES * 2];
	Uint8 *keys;

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | (jdevnums.empty() ? 0 : SDL_INIT_JOYSTICK)) < 0) {
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		goto done;
	}
	
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption("Gambatte SDL", NULL);
	
	for (std::size_t i = 0; i < jdevnums.size(); ++i) {
		SDL_Joystick *const j = SDL_JoystickOpen(i);
		
		if (j)
			joysticks.push_back(j);
	}
	
	jdevnums.clear();
	SDL_JoystickEventState(SDL_ENABLE);

	{
		SDL_AudioSpec spec;
		spec.freq = SAMPLE_RATE;
		spec.format = AUDIO_S16SYS;
		spec.channels = 2;
		spec.samples = 1024;
		spec.callback = fill_buffer;
		spec.userdata = sndBuffer;
		SDL_OpenAudio(&spec, NULL);
	}
	
	SDL_PauseAudio(0);

	keys = SDL_GetKeyState(NULL);

	gambatte.setVideoBlitter(&blitter);
	
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
					
					for (std::pair<std::multimap<JoyData,bool*>::iterator,std::multimap<JoyData,bool*>::iterator> range(jaMap.equal_range(jd));
							range.first != range.second; ++range.first) {
						*range.first->second = jd.dir == range.first->first.dir;
					}
					break;
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					jd.dev_num = e.jbutton.which;
					jd.num = e.jbutton.button;
					
					for (std::pair<std::multimap<JoyData,bool*>::iterator,std::multimap<JoyData,bool*>::iterator> range(jbMap.equal_range(jd));
							range.first != range.second; ++range.first) {
						*range.first->second = e.jbutton.state;
					}
					break;
				case SDL_JOYHATMOTION:
					jd.dev_num = e.jhat.which;
					jd.num = e.jhat.hat;
					
					for (std::pair<std::multimap<JoyData,bool*>::iterator,std::multimap<JoyData,bool*>::iterator> range(jaMap.equal_range(jd));
							range.first != range.second; ++range.first) {
						*range.first->second = e.jhat.value & range.first->first.dir;
					}
					break;
				case SDL_KEYDOWN:
					if (e.key.keysym.sym == SDLK_ESCAPE)
						goto done;
					
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
					for (std::pair<std::multimap<SDLKey,bool*>::iterator,std::multimap<SDLKey,bool*>::iterator> range(keyMap.equal_range(e.key.keysym.sym));
							range.first != range.second; ++range.first) {
						*range.first->second = e.key.state;
					}
					
					break;
				case SDL_QUIT:
					goto done;
				}
			}
		}
		
		gambatte.runFor(70224);
		
		if (!keys[SDLK_TAB]) {
			gambatte.fill_buffer(tmpBuf, SAMPLES);
			
			while (!bufAvailable)
				SDL_Delay(1);
			
			SDL_LockAudio();
			
			{
				const unsigned samples1 = std::min(SND_BUF_SZ - wPos >> 1, SAMPLES);
				const unsigned samples2 = SAMPLES - samples1;
				
				std::memcpy(sndBuffer + wPos, tmpBuf, samples1 * 4);
				
				if (samples2)
					std::memcpy(sndBuffer, tmpBuf + samples1 * 2, samples2 * 4);
				
				if ((wPos += SAMPLES * 2) >= SND_BUF_SZ)
					wPos -= SND_BUF_SZ;
				
				bufAvailable = rPos - wPos + (wPos > rPos ? SND_BUF_SZ : 0) >> 1 >= SAMPLES;
			}
			
			SDL_UnlockAudio();
			
			syncFunc();
		} else
			gambatte.fill_buffer(0, 0);
	}

done:
	SDL_PauseAudio(1);
	SDL_CloseAudio();
	
	for (std::size_t i = 0; i < joysticks.size(); ++i)
		SDL_JoystickClose(joysticks[i]);
	
	blitter.uninit();
	
	SDL_Quit();
	delete []sndBuffer;
	delete []tmpBuf;
	
	return 0;
}

static void fill_buffer(void *const buffer, Uint8 *const stream, const int len) {
	const unsigned bytes1 = std::min(static_cast<unsigned>(len), (SND_BUF_SZ - rPos) * 2);
	const unsigned bytes2 = len - bytes1;
	
	std::memcpy(stream, reinterpret_cast<Sint16*>(buffer) + rPos, bytes1);
	
	if (bytes2)
		std::memcpy(stream + bytes1, buffer, bytes2);
	
	if ((rPos += len >> 1) >= SND_BUF_SZ)
		rPos -= SND_BUF_SZ;
	
	bufAvailable = rPos - wPos + (wPos > rPos ? SND_BUF_SZ : 0) >> 1 >= SAMPLES;
}
