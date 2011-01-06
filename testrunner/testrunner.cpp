#include <gambatte.h>
#include <string>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cassert>

static Gambatte::uint_least32_t soundbuf[35112 + 2064];

static const Gambatte::uint_least32_t* tileFromChar(const char c) {
	static const Gambatte::uint_least32_t tiles[0x10 * 8 * 8] = {
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
		_,_,_,O,_,O,_,_,
		_,_,O,_,_,_,O,_,
		_,O,_,_,_,_,_,O,
		_,O,O,O,O,O,O,O,
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

static bool tilesAreEqual(const Gambatte::uint_least32_t *const lhs, const Gambatte::uint_least32_t *const rhs) {
	for (unsigned y = 0; y < 8; ++y)
	for (unsigned x = 0; x < 8; ++x)
		if ((lhs[y * 160 + x] & 0xF8F8F8) != rhs[y * 8 + x])
			return false;
	
	return true;
}

static bool frameBufferMatchesOut(const Gambatte::uint_least32_t *const framebuf, const std::string &out) {
	const Gambatte::uint_least32_t *outTile;
	
	for (std::size_t i = 0; (outTile = tileFromChar(out[i])) != 0; ++i) {
		if (!tilesAreEqual(framebuf + i * 8, outTile))
			return false;
	}
	
	return true;
}

static bool evaluateTestResults(const Gambatte::uint_least32_t *const framebuf, const std::string &file, const char *const outstr) {
	const std::size_t outpos = file.find(outstr);
	
	if (outpos != std::string::npos) {
		if (!frameBufferMatchesOut(framebuf, file.substr(outpos + std::strlen(outstr)))) {
			std::printf("\nFAILED: %s %s\n", file.c_str(), outstr);
			return false;
		}
	}

	return true;
}

static bool runTest(const char *const file, const char *const outstr, const bool forceDmg) {
	Gambatte::uint_least32_t framebuf[160 * 144];
	Gambatte::GB gb;
	
	if (gb.load(file, forceDmg)) {
		std::printf("Failed to load ROM file %s\n", file);
		assert(0);
		return false;
	}
	
	long samplesLeft = 35112 * 15;
	
	while (samplesLeft >= 0) {
		unsigned samples = 35112;
		gb.runFor(framebuf, 160, soundbuf, samples);
		samplesLeft -= samples;
	}
	
	return evaluateTestResults(framebuf, file, outstr);
}

int main(const int argc, char **const argv) {
	int numTestsRun = 0;
	int numTestsSucceeded = 0;
	
	for (int i = 1; i < argc; ++i) {
		const std::string s(argv[i]);
		const char *dmgout = 0;
		const char *cgbout = 0;
		
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
		
		if (cgbout) {
			numTestsSucceeded += runTest(argv[i], cgbout, false);
			++numTestsRun;
		}
		
		if (dmgout) {
#ifdef ENABLE_DMG_TESTS
			numTestsSucceeded += runTest(argv[i], dmgout, true);
			++numTestsRun;
#endif
		}
	}
	
	std::printf("\nRan %d tests.\n", numTestsRun);
	std::printf("%d failures.\n", numTestsRun - numTestsSucceeded);
}
