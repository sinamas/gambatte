#ifndef M0_IRQ_H
#define M0_IRQ_H

#include "lcddef.h"
#include "../savestate.h"

namespace gambatte {

class M0Irq {
public:
	M0Irq() : lycReg_(0) {}

	void lcdReset(unsigned lycReg) { lycReg_ = lycReg; }

	void lycRegChange(unsigned lycReg,
	                  unsigned long nextM0IrqTime, unsigned long cc,
	                  bool ds, bool cgb) {
		if (nextM0IrqTime - cc > cgb * 5 + 1U - ds)
			lycReg_ = lycReg;
	}

	void doEvent(unsigned char *ifreg, unsigned ly, unsigned delayedStatReg,
			unsigned statReg, unsigned lycReg) {
		if (((statReg | delayedStatReg) & lcdstat_m0irqen)
				&& (!(delayedStatReg & lcdstat_lycirqen) || ly != lycReg_)) {
			*ifreg |= 2;
		}

		lycReg_ = lycReg;
	}

	void saveState(SaveState &state) const {
		state.ppu.m0lyc = lycReg_;
	}

	void loadState(SaveState const &state) {
		lycReg_ = state.ppu.m0lyc;
	}

private:
	unsigned char lycReg_;
};

}

#endif
