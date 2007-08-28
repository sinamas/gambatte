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
#include <SDL/SDL.h>

namespace {
struct timeval {
	long tv_sec;
	long tv_usec;
};

void gettimeofday(timeval *const t, void *) {
	const unsigned long tv_msec = SDL_GetTicks();
	
	t->tv_sec = tv_msec / 1000;
	t->tv_usec = (tv_msec % 1000) * 1000;
}

void syncFunc() {
	timeval t;
	gettimeofday(&t, NULL);
	static timeval time = t;
	static long late = 0;
	static unsigned noSleep = 60;
	
	if (time.tv_sec > t.tv_sec || (time.tv_sec == t.tv_sec && time.tv_usec > t.tv_usec)) {
		timeval tmp;
		tmp.tv_sec = 0;
		tmp.tv_usec = time.tv_usec - t.tv_usec;
		if (time.tv_sec != t.tv_sec)
			tmp.tv_usec += 1000000;
		
		if (tmp.tv_usec > late) {
			tmp.tv_usec -= late;
			
			if (tmp.tv_usec >= 1000000) {
				tmp.tv_usec -= 1000000;
				++tmp.tv_sec;
			}
			
			SDL_Delay(tmp.tv_sec * 1000 + (tmp.tv_usec + 500) / 1000);
			
			gettimeofday(&t, NULL);
			late -= (time.tv_sec - t.tv_sec) * 1000000 + time.tv_usec - t.tv_usec >> 1;
// 			printf("late: %d\n", late);
			
			if (late < 0)
				late = 0;
			
			noSleep = 60;
		} else if (noSleep-- == 0) {
			noSleep = 60;
			late = 0;
		}
		
		while (time.tv_sec > t.tv_sec || (time.tv_sec == t.tv_sec && time.tv_usec > t.tv_usec)) {
			gettimeofday(&t, NULL);
		}
		
	} else
		time = t;
	
	time.tv_usec += 16743;
	
	if (time.tv_usec >= 1000000) {
		time.tv_usec -= 1000000;
		++time.tv_sec;
	}
}
}

class SdlBlitter : public VideoBlitter {
	SDL_Surface *screen;
public:
	void setBufferDimensions(unsigned int width, unsigned int height);
	const PixelBuffer inBuffer();
	void blit();
	void toggleFullScreen();
};

void SdlBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	screen = SDL_SetVideoMode(width, height, SDL_GetVideoInfo()->vfmt->BitsPerPixel == 16 ? 16 : 32, SDL_SWSURFACE/*|SDL_FULLSCREEN*/);
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

static void fill_buffer(void *buffer, Uint8 *stream, int len);

// static unsigned bufPos = 0;
// static unsigned samples = 804;

static unsigned rPos = 0;
static unsigned wPos = 0;

class InputGetter : public InputStateGetter {
public:
	const InputState& operator()() {
		return is;
	}
};

static InputGetter inputGetter;

static const unsigned SND_BUF_SZ = 4096;

int main(int argc, char **argv) {
	printf("Gambatte SDL svn\n");
	
	if (argc < 2) {
		printf("Usage: gambatte_sdl romfile\n");
		return 1;
	}
	
	if (gambatte.load(argv[1])) {
		printf("failed to load rom %s\n", argv[1]);
		return 1;
	}

	gambatte.setInputStateGetter(&inputGetter);

	Sint16 *const sndBuffer = new Sint16[SND_BUF_SZ];
	memset(sndBuffer, 0, SND_BUF_SZ * sizeof(Sint16));
	SDL_AudioSpec spec;
	spec.freq = 48000;
	spec.format = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples = 1024;
	spec.callback = fill_buffer;
	spec.userdata = sndBuffer;

	SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
	
	SDL_ShowCursor(SDL_DISABLE);

	SDL_OpenAudio(&spec, NULL);
	SDL_PauseAudio(0);

	const unsigned samples = ((spec.freq * 4389) / 262144) + 2;
	uint16_t *const tmpBuf = new uint16_t[samples * 2];

	Uint8 *keys = SDL_GetKeyState(NULL);

// 	gambatte.setVideoFilter(4);
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
			gambatte.fill_buffer(tmpBuf, samples);
			
			while (rPos - wPos + (wPos > rPos ? SND_BUF_SZ : 0) >> 1 < samples)
				SDL_Delay(1);
			
			SDL_LockAudio();
			
			{
				const unsigned samples1 = std::min(SND_BUF_SZ - wPos >> 1, samples);
				const unsigned samples2 = samples - samples1;
				
				memcpy(sndBuffer + wPos, tmpBuf, samples1 * 4);
				
				if (samples2)
					memcpy(sndBuffer, tmpBuf + samples1 * 2, samples2 * 4);
				
				if ((wPos += samples * 2) >= SND_BUF_SZ)
					wPos -= SND_BUF_SZ;
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
}

static void fill_buffer(void *const buffer, Uint8 *const stream, const int len) {
	const unsigned bytes1 = std::min(static_cast<unsigned>(len), (SND_BUF_SZ - rPos) * 2);
	const unsigned bytes2 = len - bytes1;
	
	memcpy(stream, reinterpret_cast<Sint16*>(buffer) + rPos, bytes1);
	
	if (bytes2)
		memcpy(stream + bytes1, buffer, bytes2);
	
	if ((rPos += len >> 1) >= SND_BUF_SZ)
		rPos -= SND_BUF_SZ;
}
