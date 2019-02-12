//
//   Copyright (C) 2007-2010 by sinamas <sinamas at users.sourceforge.net>
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

#include "memptrs.h"
#include <algorithm>

using namespace gambatte;

MemPtrs::MemPtrs()
: rmem_()
, wmem_()
, romdata_()
, wramdata_()
, vrambankptr_(0)
, rsrambankptr_(0)
, wsrambankptr_(0)
, rambankdata_(0)
, wramdataend_(0)
, oamDmaSrc_(oam_dma_src_off)
{
}

void MemPtrs::reset(unsigned const rombanks, unsigned const rambanks, unsigned const wrambanks) {
	int const num_disabled_ram_areas = 2;
	memchunk_.reset(
		  pre_rom_pad_size()
		+ rombanks * rombank_size()
		+ max_num_vrambanks * vrambank_size()
		+ rambanks * rambank_size()
		+ wrambanks * wrambank_size()
		+ num_disabled_ram_areas * rambank_size());

	romdata_[0] = romdata();
	rambankdata_ = romdata_[0] + rombanks * rombank_size() + max_num_vrambanks * vrambank_size();
	wramdata_[0] = rambankdata_ + rambanks * rambank_size();
	wramdataend_ = wramdata_[0] + wrambanks * wrambank_size();

	std::fill_n(rdisabledRamw(), rambank_size(), 0xFF);

	oamDmaSrc_ = oam_dma_src_off;
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	rmem_[0xC] = wmem_[0xC] = wramdata_[0] - mm_wram_begin;
	rmem_[0xE] = wmem_[0xE] = wramdata_[0] - mm_wram_mirror_begin;
	setRombank(1);
	setRambank(0, 0);
	setVrambank(0);
	setWrambank(1);
}

void MemPtrs::setRombank0(unsigned bank) {
	romdata_[0] = romdata() + bank * rombank_size();
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	disconnectOamDmaAreas();
}

void MemPtrs::setRombank(unsigned bank) {
	romdata_[1] = romdata() + bank * rombank_size() - mm_rom1_begin;
	rmem_[0x7] = rmem_[0x6] = rmem_[0x5] = rmem_[0x4] = romdata_[1];
	disconnectOamDmaAreas();
}

void MemPtrs::setRambank(unsigned const flags, unsigned const rambank) {
	unsigned char *srambankptr = 0;
	if (!(flags & rtc_en)) {
		srambankptr = rambankdata() != rambankdataend()
			? rambankdata_ + rambank * rambank_size()
			: wdisabledRam();
	}

	rsrambankptr_ = (flags & read_en) && srambankptr != wdisabledRam()
		? srambankptr - mm_sram_begin
		: rdisabledRamw() - mm_sram_begin;
	wsrambankptr_ = flags & write_en
		? srambankptr - mm_sram_begin
		: wdisabledRam() - mm_sram_begin;
	rmem_[0xB] = rmem_[0xA] = rsrambankptr_;
	wmem_[0xB] = wmem_[0xA] = wsrambankptr_;
	disconnectOamDmaAreas();
}

void MemPtrs::setWrambank(unsigned bank) {
	wramdata_[1] = wramdata_[0] + (bank & 0x07 ? bank & 0x07 : 1) * wrambank_size();
	rmem_[0xD] = wmem_[0xD] = wramdata_[1] - mm_wram1_begin;
	disconnectOamDmaAreas();
}

void MemPtrs::setOamDmaSrc(OamDmaSrc oamDmaSrc) {
	rmem_[0x3] = rmem_[0x2] = rmem_[0x1] = rmem_[0x0] = romdata_[0];
	rmem_[0x7] = rmem_[0x6] = rmem_[0x5] = rmem_[0x4] = romdata_[1];
	rmem_[0xB] = rmem_[0xA] = rsrambankptr_;
	wmem_[0xB] = wmem_[0xA] = wsrambankptr_;
	rmem_[0xC] = wmem_[0xC] = wramdata_[0] - mm_wram_begin;
	rmem_[0xD] = wmem_[0xD] = wramdata_[1] - mm_wram1_begin;
	rmem_[0xE] = wmem_[0xE] = wramdata_[0] - mm_wram_mirror_begin;

	oamDmaSrc_ = oamDmaSrc;
	disconnectOamDmaAreas();
}

void MemPtrs::disconnectOamDmaAreas() {
	if (isCgb(*this)) {
		switch (oamDmaSrc_) {
		case oam_dma_src_rom:  // fall through
		case oam_dma_src_sram:
		case oam_dma_src_invalid:
			std::fill(rmem_, rmem_ + 8, static_cast<unsigned char *>(0));
			rmem_[0xB] = rmem_[0xA] = 0;
			wmem_[0xB] = wmem_[0xA] = 0;
			break;
		case oam_dma_src_vram:
			break;
		case oam_dma_src_wram:
			rmem_[0xE] = rmem_[0xD] = rmem_[0xC] = 0;
			wmem_[0xE] = wmem_[0xD] = wmem_[0xC] = 0;
			break;
		case oam_dma_src_off:
			break;
		}
	} else {
		switch (oamDmaSrc_) {
		case oam_dma_src_rom:  // fall through
		case oam_dma_src_sram:
		case oam_dma_src_wram:
		case oam_dma_src_invalid:
			std::fill(rmem_, rmem_ + 8, static_cast<unsigned char *>(0));
			rmem_[0xB] = rmem_[0xA] = 0;
			wmem_[0xB] = wmem_[0xA] = 0;
			rmem_[0xE] = rmem_[0xD] = rmem_[0xC] = 0;
			wmem_[0xE] = wmem_[0xD] = wmem_[0xC] = 0;
			break;
		case oam_dma_src_vram:
			break;
		case oam_dma_src_off:
			break;
		}
	}
}
