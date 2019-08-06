//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "statesaver.h"
#include "savestate.h"
#include "array.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <vector>
#include <cstring>

namespace {

using namespace gambatte;

struct Saver {
	char const *label;
	void (*save)(std::ofstream &file, SaveState const &state);
	void (*load)(std::ifstream &file, SaveState &state);
	std::size_t labelsize;
};

inline bool operator<(Saver const &l, Saver const &r) {
	return std::strcmp(l.label, r.label) < 0;
}

void put24(std::ofstream &file, unsigned long data) {
	file.put(data >> 16 & 0xFF);
	file.put(data >>  8 & 0xFF);
	file.put(data       & 0xFF);
}

void put32(std::ofstream &file, unsigned long data) {
	file.put(data >> 24 & 0xFF);
	file.put(data >> 16 & 0xFF);
	file.put(data >>  8 & 0xFF);
	file.put(data       & 0xFF);
}

void write(std::ofstream &file, unsigned char data) {
	static char const inf[] = { 0x00, 0x00, 0x01 };
	file.write(inf, sizeof inf);
	file.put(data & 0xFF);
}

void write(std::ofstream &file, unsigned short data) {
	static char const inf[] = { 0x00, 0x00, 0x02 };
	file.write(inf, sizeof inf);
	file.put(data >> 8 & 0xFF);
	file.put(data      & 0xFF);
}

void write(std::ofstream &file, unsigned long data) {
	static char const inf[] = { 0x00, 0x00, 0x04 };
	file.write(inf, sizeof inf);
	put32(file, data);
}

void write(std::ofstream &file, unsigned char const *data, std::size_t size) {
	put24(file, size);
	file.write(reinterpret_cast<char const *>(data), size);
}

void write(std::ofstream &file, bool const *data, std::size_t size) {
	put24(file, size);
	std::for_each(data, data + size,
		std::bind1st(std::mem_fun(&std::ofstream::put), &file));
}

unsigned long get24(std::ifstream &file) {
	unsigned long tmp = file.get() & 0xFF;
	tmp =   tmp << 8 | (file.get() & 0xFF);
	return  tmp << 8 | (file.get() & 0xFF);
}

unsigned long read(std::ifstream &file) {
	unsigned long size = get24(file);
	if (size > 4) {
		file.ignore(size - 4);
		size = 4;
	}

	unsigned long out = 0;
	switch (size) {
	case 4: out = (out | (file.get() & 0xFF)) << 8; // fall through.
	case 3: out = (out | (file.get() & 0xFF)) << 8; // fall through.
	case 2: out = (out | (file.get() & 0xFF)) << 8; // fall through.
	case 1: out =  out | (file.get() & 0xFF);
	}

	return out;
}

inline void read(std::ifstream &file, unsigned char &data) {
	data = read(file) & 0xFF;
}

inline void read(std::ifstream &file, unsigned short &data) {
	data = read(file) & 0xFFFF;
}

inline void read(std::ifstream &file, unsigned long &data) {
	data = read(file);
}

void read(std::ifstream &file, unsigned char *buf, std::size_t bufsize) {
	std::size_t const size = get24(file);
	std::size_t const minsize = std::min(size, bufsize);
	file.read(reinterpret_cast<char*>(buf), minsize);
	file.ignore(size - minsize);

	if (static_cast<unsigned char>(0x100)) {
		for (std::size_t i = 0; i < minsize; ++i)
			buf[i] &= 0xFF;
	}
}

void read(std::ifstream &file, bool *buf, std::size_t bufsize) {
	std::size_t const size = get24(file);
	std::size_t const minsize = std::min(size, bufsize);
	for (std::size_t i = 0; i < minsize; ++i)
		buf[i] = file.get();

	file.ignore(size - minsize);
}

} // anon namespace

namespace gambatte {

class SaverList {
public:
	typedef std::vector<Saver> list_t;
	typedef list_t::const_iterator const_iterator;

	SaverList();
	const_iterator begin() const { return list.begin(); }
	const_iterator end() const { return list.end(); }
	std::size_t maxLabelsize() const { return maxLabelsize_; }

private:
	list_t list;
	std::size_t maxLabelsize_;
};

static void push(SaverList::list_t &list, char const *label,
		void (*save)(std::ofstream &file, SaveState const &state),
		void (*load)(std::ifstream &file, SaveState &state),
		std::size_t labelsize) {
	Saver saver = { label, save, load, labelsize };
	list.push_back(saver);
}

SaverList::SaverList()
: maxLabelsize_(0)
{
#define ADD(label, arg) do { \
	struct Func { \
		static void save(std::ofstream &file, SaveState const &state) { write(file, state.arg); } \
		static void load(std::ifstream &file, SaveState &state) { read(file, state.arg); } \
	}; \
	push(list, label, Func::save, Func::load, sizeof label); \
} while (0)

#define ADDPTR(label, arg) do { \
	struct Func { \
		static void save(std::ofstream &file, SaveState const &state) { \
			write(file, state.arg.get(), state.arg.size()); \
		} \
		static void load(std::ifstream &file, SaveState &state) { \
			read(file, state.arg.ptr, state.arg.size()); \
		} \
	}; \
	push(list, label, Func::save, Func::load, sizeof label); \
} while (0)

#define ADDARRAY(label, arg) do { \
	struct Func { \
		static void save(std::ofstream &file, SaveState const &state) { \
			write(file, state.arg, sizeof state.arg); \
		} \
		static void load(std::ifstream &file, SaveState &state) { \
			read(file, state.arg, sizeof state.arg); \
		} \
	}; \
	push(list, label, Func::save, Func::load, sizeof label); \
} while (0)

	ADD("cc", cpu.cycleCounter);
	ADD("pc", cpu.pc);
	ADD("sp", cpu.sp);
	ADD("a", cpu.a);
	ADD("b", cpu.b);
	ADD("c", cpu.c);
	ADD("d", cpu.d);
	ADD("e", cpu.e);
	ADD("f", cpu.f);
	ADD("h", cpu.h);
	ADD("l", cpu.l);
	ADD("op", cpu.opcode);
	ADD("fetched", cpu.prefetched);
	ADD("skip", cpu.skip);
	ADD("halt", mem.halted);
	ADDPTR("vram", mem.vram);
	ADDPTR("sram", mem.sram);
	ADDPTR("wram", mem.wram);
	ADDPTR("hram", mem.ioamhram);
	ADD("ldivup", mem.divLastUpdate);
	ADD("ltimaup", mem.timaLastUpdate);
	ADD("tmatime", mem.tmatime);
	ADD("serialt", mem.nextSerialtime);
	ADD("lodmaup", mem.lastOamDmaUpdate);
	ADD("minintt", mem.minIntTime);
	ADD("unhaltt", mem.unhaltTime);
	ADD("rombank", mem.rombank);
	ADD("dmasrc", mem.dmaSource);
	ADD("dmadst", mem.dmaDestination);
	ADD("rambank", mem.rambank);
	ADD("odmapos", mem.oamDmaPos);
	ADD("hlthdma", mem.haltHdmaState);
	ADD("ime", mem.IME);
	ADD("sramon", mem.enableRam);
	ADD("rambmod", mem.rambankMode);
	ADD("hdma", mem.hdmaTransfer);
	ADDPTR("bgp", ppu.bgpData);
	ADDPTR("objp", ppu.objpData);
	ADDPTR("sposbuf", ppu.oamReaderBuf);
	ADDPTR("spszbuf", ppu.oamReaderSzbuf);
	ADDARRAY("spattr", ppu.spAttribList);
	ADDARRAY("spbyte0", ppu.spByte0List);
	ADDARRAY("spbyte1", ppu.spByte1List);
	ADD("vcycles", ppu.videoCycles);
	ADD("edM0tim", ppu.enableDisplayM0Time);
	ADD("m0time", ppu.lastM0Time);
	ADD("nm0irq", ppu.nextM0Irq);
	ADD("bgtw", ppu.tileword);
	ADD("bgntw", ppu.ntileword);
	ADD("winypos", ppu.winYPos);
	ADD("xpos", ppu.xpos);
	ADD("endx", ppu.endx);
	ADD("ppur0", ppu.reg0);
	ADD("ppur1", ppu.reg1);
	ADD("bgatrb", ppu.attrib);
	ADD("bgnatrb", ppu.nattrib);
	ADD("ppustat", ppu.state);
	ADD("nsprite", ppu.nextSprite);
	ADD("csprite", ppu.currentSprite);
	ADD("lyc", ppu.lyc);
	ADD("m0lyc", ppu.m0lyc);
	ADD("oldwy", ppu.oldWy);
	ADD("windraw", ppu.winDrawState);
	ADD("wscx", ppu.wscx);
	ADD("wemastr", ppu.weMaster);
	ADD("lcdsirq", ppu.pendingLcdstatIrq);
	ADD("spucntr", spu.cycleCounter);
	ADD("swpcntr", spu.ch1.sweep.counter);
	ADD("swpshdw", spu.ch1.sweep.shadow);
	ADD("swpneg", spu.ch1.sweep.negging);
	ADD("dut1ctr", spu.ch1.duty.nextPosUpdate);
	ADD("dut1pos", spu.ch1.duty.pos);
	ADD("dut1hi", spu.ch1.duty.high);
	ADD("env1ctr", spu.ch1.env.counter);
	ADD("env1vol", spu.ch1.env.volume);
	ADD("len1ctr", spu.ch1.lcounter.counter);
	ADD("len1val", spu.ch1.lcounter.lengthCounter);
	ADD("nr10", spu.ch1.sweep.nr0);
	ADD("nr13", spu.ch1.duty.nr3);
	ADD("nr14", spu.ch1.nr4);
	ADD("c1mastr", spu.ch1.master);
	ADD("dut2ctr", spu.ch2.duty.nextPosUpdate);
	ADD("dut2pos", spu.ch2.duty.pos);
	ADD("dut2hi", spu.ch2.duty.high);
	ADD("env2ctr", spu.ch2.env.counter);
	ADD("env2vol", spu.ch2.env.volume);
	ADD("len2ctr", spu.ch2.lcounter.counter);
	ADD("len2val", spu.ch2.lcounter.lengthCounter);
	ADD("nr23", spu.ch2.duty.nr3);
	ADD("nr24", spu.ch2.nr4);
	ADD("c2mastr", spu.ch2.master);
	ADDPTR("waveram", spu.ch3.waveRam);
	ADD("len3ctr", spu.ch3.lcounter.counter);
	ADD("len3val", spu.ch3.lcounter.lengthCounter);
	ADD("wavectr", spu.ch3.waveCounter);
	ADD("lwavrdt", spu.ch3.lastReadTime);
	ADD("wavepos", spu.ch3.wavePos);
	ADD("wavsmpl", spu.ch3.sampleBuf);
	ADD("nr33", spu.ch3.nr3);
	ADD("nr34", spu.ch3.nr4);
	ADD("c3mastr", spu.ch3.master);
	ADD("lfsrctr", spu.ch4.lfsr.counter);
	ADD("lfsrreg", spu.ch4.lfsr.reg);
	ADD("env4ctr", spu.ch4.env.counter);
	ADD("env4vol", spu.ch4.env.volume);
	ADD("len4ctr", spu.ch4.lcounter.counter);
	ADD("len4val", spu.ch4.lcounter.lengthCounter);
	ADD("nr44", spu.ch4.nr4);
	ADD("c4mastr", spu.ch4.master);
	ADD("rtcbase", rtc.baseTime);
	ADD("rtchalt", rtc.haltTime);
	ADD("rtcdh", rtc.dataDh);
	ADD("rtcdl", rtc.dataDl);
	ADD("rtch", rtc.dataH);
	ADD("rtcm", rtc.dataM);
	ADD("rtcs", rtc.dataS);
	ADD("rtclld", rtc.lastLatchData);

#undef ADD
#undef ADDPTR
#undef ADDARRAY

	// sort list for binary search/std::lower_bound use.
	std::sort(list.begin(), list.end());
	for (const_iterator it = list.begin(); it != list.end(); ++it)
		maxLabelsize_ = std::max(maxLabelsize_, it->labelsize);
}

}

namespace {

struct RgbSum { unsigned long rb, g; };

void addPairs(RgbSum *const sum, uint_least32_t const *const p) {
	sum[0].rb += (p[0] & 0xFF00FF) + (p[3] & 0xFF00FF);
	sum[0].g  += (p[0] & 0x00FF00) + (p[3] & 0x00FF00);
	sum[1].rb += (p[1] & 0xFF00FF) + (p[2] & 0xFF00FF);
	sum[1].g  += (p[1] & 0x00FF00) + (p[2] & 0x00FF00);
}

void blendPairs(RgbSum *const dst, RgbSum const *const sums) {
	dst->rb = sums[1].rb * 8 + (sums[0].rb - sums[1].rb) * 3;
	dst->g  = sums[1].g  * 8 + (sums[0].g  - sums[1].g ) * 3;
}

void writeSnapShot(std::ofstream &file, uint_least32_t const *src, std::ptrdiff_t const pitch) {
	put24(file, src ? StateSaver::ss_width * StateSaver::ss_height * sizeof *src : 0);

	if (src) {
		uint_least32_t buf[StateSaver::ss_width];

		for (unsigned h = StateSaver::ss_height; h--;) {
			for (unsigned x = 0; x < StateSaver::ss_width; ++x) {
				uint_least32_t const *const p = src + x * StateSaver::ss_div;
				RgbSum sum[] = { {0, 0}, {0, 0}, {0, 0}, {0, 0} };

				addPairs(sum    , p            );
				addPairs(sum + 2, p + pitch    );
				addPairs(sum + 2, p + pitch * 2);
				addPairs(sum    , p + pitch * 3);

				blendPairs(sum, sum);
				blendPairs(sum + 1, sum + 2);

				blendPairs(sum, sum);

				buf[x] = ((sum[0].rb & 0xFF00FF00) | (sum[0].g & 0x00FF0000)) >> 8;
			}

			file.write(reinterpret_cast<char const *>(buf), sizeof buf);
			src += pitch * StateSaver::ss_div;
		}
	}
}

SaverList list;

} // anon namespace

bool StateSaver::saveState(SaveState const &state,
		uint_least32_t const *const videoBuf,
		std::ptrdiff_t const pitch, std::string const &filename) {
	std::ofstream file(filename.c_str(), std::ios_base::binary);
	if (!file)
		return false;

	{ static char const ver[] = { 0, 1 }; file.write(ver, sizeof ver); }
	writeSnapShot(file, videoBuf, pitch);

	for (SaverList::const_iterator it = list.begin(); it != list.end(); ++it) {
		file.write(it->label, it->labelsize);
		(*it->save)(file, state);
	}

	return !file.fail();
}

bool StateSaver::loadState(SaveState &state, std::string const &filename) {
	std::ifstream file(filename.c_str(), std::ios_base::binary);
	if (!file || file.get() != 0)
		return false;

	file.ignore();
	file.ignore(get24(file));

	Array<char> const labelbuf(list.maxLabelsize());
	Saver const labelbufSaver = { labelbuf, 0, 0, list.maxLabelsize() };
	SaverList::const_iterator done = list.begin();

	while (file.good() && done != list.end()) {
		file.getline(labelbuf, list.maxLabelsize(), 0);

		SaverList::const_iterator it = done;
		if (std::strcmp(labelbuf, it->label)) {
			it = std::lower_bound(it + 1, list.end(), labelbufSaver);
			if (it == list.end() || std::strcmp(labelbuf, it->label)) {
				file.ignore(get24(file));
				continue;
			}
		} else
			++done;

		(*it->load)(file, state);
	}

	state.cpu.cycleCounter &= 0x7FFFFFFF;
	state.spu.cycleCounter &= 0x7FFFFFFF;

	return true;
}
