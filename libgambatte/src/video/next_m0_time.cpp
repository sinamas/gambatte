#include "next_m0_time.h"
#include "ppu.h"

void gambatte::NextM0Time::predictNextM0Time(const struct PPU &ppu) {
	predictedNextM0Time_ = ppu.predictedNextXposTime(167);
}
