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
#include <string>
#include <sstream>

#include "syncfunc.h"
#include "parser.h"

class SdlBlitter : public VideoBlitter {
	SDL_Surface *screen;
	unsigned startFlags;
	
public:
	SdlBlitter() : screen(NULL), startFlags(SDL_SWSURFACE) {}
	void setBufferDimensions(unsigned int width, unsigned int height);
	const PixelBuffer inBuffer();
	void blit();
	void toggleFullScreen();
	void setStartFull() { startFlags |= SDL_FULLSCREEN; }
};

void SdlBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	screen = SDL_SetVideoMode(width, height, SDL_GetVideoInfo()->vfmt->BitsPerPixel == 16 ? 16 : 32, startFlags);
}

const PixelBuffer SdlBlitter::inBuffer() {
	PixelBuffer pb;
	pb.pixels = (Uint8*)(screen->pixels) + screen->offset;
	pb.format = screen->format->BitsPerPixel == 16 ? PixelBuffer::RGB16 : PixelBuffer::RGB32;
	pb.pitch = screen->pitch / screen->format->BytesPerPixel;
	return pb;
}

void SdlBlitter::blit() {
	SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
}

void SdlBlitter::toggleFullScreen() {
	screen = SDL_SetVideoMode(screen->w, screen->h, screen->format->BitsPerPixel, screen->flags ^ SDL_FULLSCREEN);
}

static Gambatte gambatte;
static SdlBlitter blitter;
static InputState is;

struct FatOption {
	virtual Parser::Option* getShort() { return NULL; };
	virtual Parser::Option* getLong() = 0;
	virtual const char* getDesc() const = 0;
	virtual ~FatOption() {}
};

class FsOption : public FatOption {
	class Main : public Parser::Option {
	protected:
		Main(const char *const s) : Option(s) {}
	public:
		void exec(const char *const */*argv*/, int /*index*/) {
			blitter.setStartFull();
		}
	};
	
	struct Short : Main {
		Short() : Main("-f") {}
	};
	
	struct Long : Main {
		Long() : Main("--full-screen") {}
	};
	
	Short sOpt;
	Long lOpt;
	static const char *const desc;
	
public:
	Parser::Option* getShort() { return &sOpt; }
	Parser::Option* getLong() { return &lOpt; }
	const char* getDesc() const { return desc; }
};

const char *const FsOption::desc = "\t\tStart in full screen mode\n";

class VfOption : public FatOption {
	class Main : public Parser::Option {
	protected:
		Main(const char *const s) : Option(s, 1) {}
	public:
		void exec(const char *const *argv, int index) {
			gambatte.setVideoFilter(atoi(argv[index + 1]));
		}
	};
	
	struct Short : Main {
		Short() : Main("-v") {}
	};
	
	struct Long : Main {
		Long() : Main("--video-filter") {}
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



static void fill_buffer(void *buffer, Uint8 *stream, int len);

// static unsigned bufPos = 0;
// static unsigned samples = 804;

static const unsigned SAMPLE_RATE = 48000;
static const unsigned SAMPLES = ((SAMPLE_RATE * 4389) / 262144) + 2;
static unsigned rPos = 0;
static unsigned wPos = 0;
static bool bufAvailable = false;

class InputGetter : public InputStateGetter {
public:
	const InputState& operator()() {
		return is;
	}
};

static InputGetter inputGetter;

static const unsigned SND_BUF_SZ = 4096 * 2;

static const char *const usage = "Usage: gambatte_sdl romfile\n";

static void printUsage(std::vector<FatOption*> &v) {
	printf("Usage: gambatte_sdl [OPTION]... romfile\n\n");
	
	for (unsigned i = 0; i < v.size(); ++i) {
		printf("  %s, %s%s\n", v[i]->getShort()->getStr(), v[i]->getLong()->getStr(), v[i]->getDesc());
	}
}

int main(int argc, char **argv) {
	printf("Gambatte SDL svn\n");
	
	Parser parser;
	
	{
		static std::vector<FatOption*> v;
		static FsOption fsOption;
		v.push_back(&fsOption);
		static VfOption vfOption;
		v.push_back(&vfOption);
		
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
	}

	gambatte.setInputStateGetter(&inputGetter);

	Sint16 *const sndBuffer = new Sint16[SND_BUF_SZ];
	memset(sndBuffer, 0, SND_BUF_SZ * sizeof(Sint16));
	SDL_AudioSpec spec;
	spec.freq = SAMPLE_RATE;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = 1024;
	spec.callback = fill_buffer;
	spec.userdata = sndBuffer;

	SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
	
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_SetCaption("Gambatte SDL", NULL);

	SDL_OpenAudio(&spec, NULL);
	SDL_PauseAudio(0);

	uint16_t *const tmpBuf = new uint16_t[SAMPLES * 2];

	Uint8 *keys = SDL_GetKeyState(NULL);

	gambatte.setVideoBlitter(&blitter);
	
	for (;;) {
		SDL_PumpEvents();
		
		if (keys[SDLK_ESCAPE])
			break;
		
		if (keys[SDLK_LCTRL] | keys[SDLK_RCTRL]) {
			if (keys[SDLK_f]) {
				blitter.toggleFullScreen();
				gambatte.videoBufferChange();
			}
			
			if (keys[SDLK_r]) {
				gambatte.reset();
			}
		}
		
		is.startButton = keys[SDLK_RETURN];
		is.selectButton = keys[SDLK_RSHIFT];
		is.bButton = keys[SDLK_c];
		is.aButton = keys[SDLK_d];
		is.dpadDown = keys[SDLK_DOWN];
		is.dpadUp = keys[SDLK_UP];
		is.dpadLeft = keys[SDLK_LEFT];
		is.dpadRight = keys[SDLK_RIGHT];
		
		gambatte.runFor(70224);
		
		if (!keys[SDLK_TAB]) {
			gambatte.fill_buffer(tmpBuf, SAMPLES);
			
			while (!bufAvailable)
				SDL_Delay(1);
			
			SDL_LockAudio();
			
			{
				const unsigned samples1 = std::min(SND_BUF_SZ - wPos >> 1, SAMPLES);
				const unsigned samples2 = SAMPLES - samples1;
				
				memcpy(sndBuffer + wPos, tmpBuf, samples1 * 4);
				
				if (samples2)
					memcpy(sndBuffer, tmpBuf + samples1 * 2, samples2 * 4);
				
				if ((wPos += SAMPLES * 2) >= SND_BUF_SZ)
					wPos -= SND_BUF_SZ;
				
				bufAvailable = rPos - wPos + (wPos > rPos ? SND_BUF_SZ : 0) >> 1 >= SAMPLES;
			}
			
			SDL_UnlockAudio();
			
			syncFunc();
		}
	}

	SDL_PauseAudio(1);
	SDL_CloseAudio();
	SDL_Quit();
	delete []sndBuffer;
	delete []tmpBuf;
	
	return 0;
}

static void fill_buffer(void *const buffer, Uint8 *const stream, const int len) {
	const unsigned bytes1 = std::min(static_cast<unsigned>(len), (SND_BUF_SZ - rPos) * 2);
	const unsigned bytes2 = len - bytes1;
	
	memcpy(stream, reinterpret_cast<Sint16*>(buffer) + rPos, bytes1);
	
	if (bytes2)
		memcpy(stream + bytes1, buffer, bytes2);
	
	if ((rPos += len >> 1) >= SND_BUF_SZ)
		rPos -= SND_BUF_SZ;
	
	bufAvailable = rPos - wPos + (wPos > rPos ? SND_BUF_SZ : 0) >> 1 >= SAMPLES;
}
