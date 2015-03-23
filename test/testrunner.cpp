#include "gambatte.h"
#include "transfer_ptr.h"
#include <png.h>
#include <algorithm>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>

namespace {

struct FileDeleter { static void del(std::FILE *f) { if (f) std::fclose(f); } };
typedef transfer_ptr<std::FILE, FileDeleter> file_ptr;

unsigned const gb_width = 160, gb_height = 144;
std::size_t const samples_per_frame = 35112;
std::size_t const audiobuf_size = samples_per_frame + 2064;
std::size_t const framebuf_size = gb_width * gb_height;

static void readPng(gambatte::uint_least32_t out[], std::FILE &file) {
	struct PngContext {
		png_structp png;
		png_infop info;
		png_infop endinfo;

		PngContext()
		: png(png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))
		, info(png ? png_create_info_struct(png) : 0)
		, endinfo(png ? png_create_info_struct(png) : 0)
		{
			assert(png);
			assert(info);
			assert(endinfo);
		}

		~PngContext() {
			png_destroy_read_struct(&png, &info, &endinfo);
		}
	} const pngCtx;

	if (setjmp(png_jmpbuf(pngCtx.png)))
		std::abort();

	png_init_io(pngCtx.png, &file);
	png_read_png(pngCtx.png, pngCtx.info, 0, 0);

	assert(png_get_image_height(pngCtx.png, pngCtx.info) == gb_height);
	assert(png_get_rowbytes(pngCtx.png, pngCtx.info) == gb_width * 4);

	png_bytep const *const rows = png_get_rows(pngCtx.png, pngCtx.info);

	for (std::size_t y = 0; y < gb_height; ++y)
	for (std::size_t x = 0; x < gb_width; ++x) {
		out[y * gb_width + x] = rows[y][x * 4] << 16
			| rows[y][x * 4 + 1] << 8
			| rows[y][x * 4 + 2];
	}
}

static gambatte::uint_least32_t const * tileFromChar(char const c) {
	static gambatte::uint_least32_t const tiles[0x10 * 8 * 8] = {
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

	unsigned num = isdigit(c) ? c - '0' : toupper(c) - 'A' + 0xA;
	return num < 0x10 ? tiles + num * 8*8 : 0;
}

static bool tilesAreEqual(
		gambatte::uint_least32_t const lhs[],
		gambatte::uint_least32_t const rhs[]) {
	for (unsigned y = 0; y < 8; ++y)
	for (unsigned x = 0; x < 8; ++x)
		if ((lhs[y * gb_width + x] & 0xF8F8F8) != rhs[y * 8 + x])
			return false;

	return true;
}

static bool frameBufferMatchesOut(gambatte::uint_least32_t const framebuf[], std::string const &out) {
	gambatte::uint_least32_t const *outTile;

	for (std::size_t i = 0; (outTile = tileFromChar(out[i])) != 0; ++i) {
		if (!tilesAreEqual(framebuf + i * 8, outTile))
			return false;
	}

	return true;
}

static bool frameBufsEqual(
		gambatte::uint_least32_t const lhs[],
		gambatte::uint_least32_t const rhs[]) {
	for (std::size_t i = 0; i < framebuf_size; ++i) {
		if ((lhs[i] ^ rhs[i]) & 0xFCFCFC)
			return false;
	}

	return true;
}

static bool evaluateStrTestResults(
		gambatte::uint_least32_t const audiobuf[],
		gambatte::uint_least32_t const framebuf[],
		std::string const &file,
		std::string const &outstr) {
	std::size_t const outpos = file.find(outstr);
	assert(outpos != std::string::npos);

	if (file.compare(outpos + outstr.size(), 6, "audio0") == 0) {
		if (std::count(audiobuf, audiobuf + samples_per_frame, audiobuf[0]) == samples_per_frame)
			return true;
	} else if (file.compare(outpos + outstr.size(), 6, "audio1") == 0) {
		if (std::count(audiobuf, audiobuf + samples_per_frame, audiobuf[0]) != samples_per_frame)
			return true;
	} else {
		if (frameBufferMatchesOut(framebuf, file.substr(outpos + outstr.size())))
			return true;
	}

	std::printf("\nFAILED: %s %s\n", file.c_str(), outstr.c_str());
	return false;
}

static void runTestRom(
		gambatte::uint_least32_t framebuf[],
		gambatte::uint_least32_t audiobuf[],
		std::string const &file,
		bool const forceDmg) {
	gambatte::GB gb;

	if (gb.load(file, forceDmg)) {
		std::fprintf(stderr, "Failed to load ROM image file %s\n", file.c_str());
		std::abort();
	}

	std::putchar(gb.isCgb() ? 'c' : 'd');

	long samplesLeft = samples_per_frame * 15;

	while (samplesLeft >= 0) {
		std::size_t samples = samples_per_frame;
		gb.runFor(framebuf, gb_width, audiobuf, samples);
		samplesLeft -= samples;
	}
}

static bool runStrTest(std::string const &romfile, bool forceDmg, std::string const &outstr) {
	gambatte::uint_least32_t audiobuf[audiobuf_size];
	gambatte::uint_least32_t framebuf[framebuf_size];
	runTestRom(framebuf, audiobuf, romfile, forceDmg);
	return evaluateStrTestResults(audiobuf, framebuf, romfile, outstr);
}

static bool runPngTest(std::string const &romfile, bool forceDmg, std::FILE &pngfile) {
	gambatte::uint_least32_t audiobuf[audiobuf_size];
	gambatte::uint_least32_t framebuf[framebuf_size];
	runTestRom(framebuf, audiobuf, romfile, forceDmg);

	gambatte::uint_least32_t pngbuf[framebuf_size];
	readPng(pngbuf, pngfile);

	if (!frameBufsEqual(framebuf, pngbuf)) {
		std::printf("\nFAILED: %s png\n", romfile.c_str());
		return false;
	}

	return true;
}

static std::string extensionStripped(std::string const &s) {
	return s.substr(0, s.rfind('.'));
}

static file_ptr openFile(std::string const &filename) {
	return file_ptr(std::fopen(filename.c_str(), "rb"));
}

} // anon ns

int main(int const argc, char *argv[]) {
	int numTestsRun = 0;
	int numTestsSucceeded = 0;

	for (int i = 1; i < argc; ++i) {
		std::string const s = extensionStripped(argv[i]);
		char const *dmgout = 0;
		char const *cgbout = 0;

		if (s.find("dmg08_cgb04c_out") != std::string::npos) {
			dmgout = cgbout = "dmg08_cgb04c_out";
		} else {
			if (s.find("dmg08_out") != std::string::npos) {
				dmgout = "dmg08_out";

				if (s.find("cgb04c_out") != std::string::npos)
					cgbout = "cgb04c_out";
			} else if (s.find("_out") != std::string::npos)
				cgbout = "_out";
		}
		if (cgbout) {
			numTestsSucceeded += runStrTest(argv[i], false, cgbout);
			++numTestsRun;
		}
		if (dmgout) {
			numTestsSucceeded += runStrTest(argv[i],  true, dmgout);
			++numTestsRun;
		}

		if (file_ptr png = openFile(s + "_dmg08_cgb04c.png")) {
			numTestsSucceeded += runPngTest(argv[i], false, *png);
			numTestsSucceeded += runPngTest(argv[i],  true, *png);
			numTestsRun += 2;
		} else {
			if (file_ptr p = openFile(s + "_cgb04c.png")) {
				numTestsSucceeded += runPngTest(argv[i], false, *p);
				++numTestsRun;
			}
			if (file_ptr p = openFile(s + "_dmg08.png")) {
				numTestsSucceeded += runPngTest(argv[i],  true, *p);
				++numTestsRun;
			}
		}
	}

	std::printf("\n\nRan %d tests.\n", numTestsRun);
	std::printf("%d failures.\n", numTestsRun - numTestsSucceeded);
}
