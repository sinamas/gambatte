#include <gambatte.h>
#include <png.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cassert>

static gambatte::uint_least32_t soundbuf[35112 + 2064];

static void readPng(gambatte::uint_least32_t *const out, const char *const filename) {
	const struct PngContext {
		png_structp png;
		png_infop info;
		png_infop endinfo;

		PngContext() :
			png(png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0)),
			info(png ? png_create_info_struct(png) : 0),
			endinfo(png ? png_create_info_struct(png) : 0)
		{
			assert(png);
			assert(info);
			assert(endinfo);
		}

		~PngContext() {
			png_destroy_read_struct(&png, &info, &endinfo);
		}
	} pngCtx;

	const struct FileCtx {
		std::FILE *f;
		FileCtx(const char *filename) : f(std::fopen(filename, "rb")) { assert(f); }
		~FileCtx() { std::fclose(f); }
	} fileCtx(filename);

	if (setjmp(png_jmpbuf(pngCtx.png)))
		std::abort();

	png_init_io(pngCtx.png, fileCtx.f);
	png_read_png(pngCtx.png, pngCtx.info, 0, 0);

	assert(png_get_image_height(pngCtx.png, pngCtx.info) == 144);
	assert(png_get_rowbytes(pngCtx.png, pngCtx.info) == 160 * 4);

	png_bytep *const rows = png_get_rows(pngCtx.png, pngCtx.info);
	
	for (std::size_t y = 0; y < 144; ++y)
	for (std::size_t x = 0; x < 160; ++x)
		out[y * 160 + x] = rows[y][x * 4] << 16 | rows[y][x * 4 + 1] << 8 | rows[y][x * 4 + 2];
}

static const gambatte::uint_least32_t* tileFromChar(const char c) {
	static const gambatte::uint_least32_t tiles[0x10 * 8 * 8] = {
		#define _ 0xF8F8F8
		#define O 0x000000
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,_,O,_,_,_,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,_,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,O,_,
		_,_,_,_,_,O,_,_,
		_,_,_,_,O,_,_,_,
		_,_,_,O,_,_,_,_,
		_,_,_,O,_,_,_,_,
		
		_,_,_,_,_,_,_,_,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,_,_,_,_,_,_,O,
		_,_,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,_,_,_,O,_,_,_,
		_,_,O,_,_,_,O,_,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,
		
		_,_,_,_,_,_,_,_,
		_,_,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,O,
		_,_,O,O,O,O,O,_,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,_,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,_,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		
		_,_,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,O,O,O,O,O,O,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_,
		_,O,_,_,_,_,_,_
		
		#undef O
		#undef _
	};
	
	const unsigned num = isdigit(c) ? c - '0' : toupper(c) - 'A' + 0xA;
	return num < 0x10 ? tiles + num * 8*8 : 0;
}

static bool tilesAreEqual(const gambatte::uint_least32_t *const lhs, const gambatte::uint_least32_t *const rhs) {
	for (unsigned y = 0; y < 8; ++y)
	for (unsigned x = 0; x < 8; ++x)
		if ((lhs[y * 160 + x] & 0xF8F8F8) != rhs[y * 8 + x])
			return false;
	
	return true;
}

static bool frameBufferMatchesOut(const gambatte::uint_least32_t *const framebuf, const std::string &out) {
	const gambatte::uint_least32_t *outTile;
	
	for (std::size_t i = 0; (outTile = tileFromChar(out[i])) != 0; ++i) {
		if (!tilesAreEqual(framebuf + i * 8, outTile))
			return false;
	}
	
	return true;
}

static bool frameBufsEqual(const gambatte::uint_least32_t lhs[], const gambatte::uint_least32_t rhs[]) {
	for (std::size_t i = 0; i < 160 * 144; ++i) {
		if ((lhs[i] ^ rhs[i]) & 0xFCFCFC)
			return false;
	}

	return true;
}

static bool evaluateStrTestResults(const gambatte::uint_least32_t *const framebuf, const std::string &file, const char *const outstr) {
	const std::size_t outpos = file.find(outstr);
	
	if (outpos != std::string::npos) {
		if (!frameBufferMatchesOut(framebuf, file.substr(outpos + std::strlen(outstr)))) {
			std::printf("\nFAILED: %s %s\n", file.c_str(), outstr);
			return false;
		}
	}

	return true;
}

static void runTestRom(gambatte::uint_least32_t framebuf[], const char *const file, const bool forceDmg) {
	gambatte::GB gb;
	
	if (gb.load(file, forceDmg)) {
		std::printf("Failed to load ROM file %s\n", file);
		std::abort();
	}
	
	long samplesLeft = 35112 * 15;
	
	while (samplesLeft >= 0) {
		unsigned samples = 35112;
		gb.runFor(framebuf, 160, soundbuf, samples);
		samplesLeft -= samples;
	}
}

static bool runStrTest(const char *const romfile, const bool forceDmg, const char *const outstr) {
	gambatte::uint_least32_t framebuf[160 * 144];
	runTestRom(framebuf, romfile, forceDmg);
	return evaluateStrTestResults(framebuf, romfile, outstr);
}

static bool runPngTest(const char *const romfile, const bool forceDmg, const char *const pngfile) {
	gambatte::uint_least32_t framebuf[160 * 144];
	runTestRom(framebuf, romfile, forceDmg);
	
	gambatte::uint_least32_t pngbuf[160 * 144];
	readPng(pngbuf, pngfile);
	
	if (!frameBufsEqual(framebuf, pngbuf)) {
		std::printf("\nFAILED: %s %s\n", romfile, pngfile);
		return false;
	}
	
	return true;
}

static bool fileExists(const std::string &filename) {
	if (std::FILE *const file = std::fopen(filename.c_str(), "rb")) {
		std::fclose(file);
		return true;
	}
	
	return false;
}

static const std::string extensionStripped(const std::string &s) {
	const std::size_t pos = s.rfind('.');
	return pos != std::string::npos ? s.substr(0, pos) : s;
}

int main(const int argc, char **const argv) {
	int numTestsRun = 0;
	int numTestsSucceeded = 0;
	
	for (int i = 1; i < argc; ++i) {
		const std::string &s = extensionStripped(argv[i]);
		const char *dmgout = 0;
		const char *cgbout = 0;
		const char *dmgpng = 0;
		const char *cgbpng = 0;
		
		if (s.find("dmg08_cgb_out") != std::string::npos) {
			dmgout = cgbout = "dmg08_cgb_out";
		} else {
			if (s.find("dmg08_out") != std::string::npos) {
				dmgout = "dmg08_out";
				
				if (s.find("cgb_out") != std::string::npos)
					cgbout = "cgb_out";
			} else if (s.find("_out") != std::string::npos)
				cgbout = "_out";
		}
		
		if (fileExists(s + "_dmg08_cgb.png")) {
			dmgpng = cgbpng = "_dmg08_cgb.png";
		} else {
			if (fileExists(s + "_dmg08.png"))
				dmgpng = "_dmg08.png";
			
			if (fileExists(s + "_cgb.png"))
				cgbpng = "_cgb.png";
		}
		
		if (cgbout) {
			numTestsSucceeded += runStrTest(argv[i], false, cgbout);
			++numTestsRun;
		}
		
		if (dmgout) {
			numTestsSucceeded += runStrTest(argv[i],  true, dmgout);
			++numTestsRun;
		}
		
		if (cgbpng) {
			numTestsSucceeded += runPngTest(argv[i], false, (s + cgbpng).c_str());
			++numTestsRun;
		}
		
		if (dmgpng) {
			numTestsSucceeded += runPngTest(argv[i],  true, (s + dmgpng).c_str());
			++numTestsRun;
		}
	}
	
	std::printf("\nRan %d tests.\n", numTestsRun);
	std::printf("%d failures.\n", numTestsRun - numTestsSucceeded);
}
