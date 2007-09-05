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
#include "video.h"
#include "videoblitter.h"
#include "video/filters/filter.h"
#include "video/filters/catrom2x.h"
#include "video/filters/catrom3x.h"
#include "video/filters/kreed2xsai.h"
#include "video/filters/maxsthq2x.h"
#include "filterinfo.h"

#include "video/basic_add_event.h"

const uint32_t LCD::dmgColorsRgb32[4] = { 3 * 85 * 0x010101, 2 * 85 * 0x010101, 1 * 85 * 0x010101, 0 * 85 * 0x010101 };
const uint32_t LCD::dmgColorsRgb16[4] = { 3 * 10 * 0x0841, 2 * 10 * 0x0841, 1 * 10 * 0x0841, 0 * 10 * 0x0841 };

#ifdef WORDS_BIGENDIAN
const uint32_t LCD::dmgColorsUyvy[4] = {
		0x80008000 | (16 + 73 * 3) * 0x00010001U,
		0x80008000 | (16 + 73 * 2) * 0x00010001U,
		0x80008000 | (16 + 73 * 1) * 0x00010001U,
		0x80008000 | (16 + 73 * 0) * 0x00010001U };
#else
const uint32_t LCD::dmgColorsUyvy[4] = {
		0x00800080 | (16 + 73 * 3) * 0x01000100U,
		0x00800080 | (16 + 73 * 2) * 0x01000100U,
		0x00800080 | (16 + 73 * 1) * 0x01000100U,
		0x00800080 | (16 + 73 * 0) * 0x01000100U };
#endif

void LCD::setDmgPalette(uint32_t *const palette, const uint32_t *const dmgColors, const unsigned data) {
	palette[0] = dmgColors[data & 3];
	palette[1] = dmgColors[(data >> 2) & 3];
	palette[2] = dmgColors[(data >> 4) & 3];
	palette[3] = dmgColors[(data >> 6) & 3];
}

unsigned LCD::gbcToRgb32(const unsigned bgr15) {
	const unsigned r = bgr15 & 0x1F;
	const unsigned g = (bgr15 >> 5) & 0x1F;
	const unsigned b = (bgr15 >> 10) & 0x1F;
	
	return (((r * 13 + g * 2 + b) >> 1) << 16) | ((g * 3 + b) << 9) | ((r * 3 + g * 2 + b * 11) >> 1);
}

unsigned LCD::gbcToRgb16(const unsigned bgr15) {
	const unsigned r = bgr15 & 0x1F;
	const unsigned g = (bgr15 >> 5) & 0x1F;
	const unsigned b = (bgr15 >> 10) & 0x1F;
	
	return (((r * 13 + g * 2 + b + 8) << 7) & 0xF800) | (((g * 3 + b + 1) >> 1) << 5) | ((r * 3 + g * 2 + b * 11 + 8) >> 4);
}

unsigned LCD::gbcToUyvy(const unsigned bgr15) {
	const unsigned r5 = bgr15 & 0x1F;
	const unsigned g5 = (bgr15 >> 5) & 0x1F;
	const unsigned b5 = (bgr15 >> 10) & 0x1F;
	
	// y = (r5 * 926151 + g5 * 1723530 + b5 * 854319) / 510000 + 16;
	// u = (b5 * 397544 - r5 * 68824 - g5 * 328720) / 225930 + 128;
	// v = (r5 * 491176 - g5 * 328720 - b5 * 162456) / 178755 + 128;
	
	const unsigned y = r5 * 116 + g5 * 216 + b5 * 107 + 16 * 64 + 32 >> 6;
	const unsigned u = b5 * 225 - r5 * 39 - g5 * 186 + 128 * 128 + 64 >> 7;
	const unsigned v = r5 * 176 - g5 * 118 - b5 * 58 + 128 * 64 + 32 >> 6;
	
#ifdef WORDS_BIGENDIAN
	return (u << 24) | (y << 16) | (v << 8) | y;
#else
	return (y << 24) | (v << 16) | (y << 8) | u;
#endif
}

LCD::LCD(const uint8_t *const oamram, const uint8_t *const vram_in) :
	vram(vram_in),
	m3EventQueue(11, VideoEventComparer()),
	irqEventQueue(4, VideoEventComparer()),
	vEventQueue(5, VideoEventComparer()),
	m3ExtraCycles(spriteMapper, scxReader, weMasterChecker, wyReg, we, wxReader),
	weMasterChecker(m3EventQueue, wyReg, lyCounter),
	wyReg(lyCounter, weMasterChecker),
	wxReader(m3EventQueue, we.enableChecker(), we.disableChecker()),
	spriteSizeReader(m3EventQueue, spriteMapper, lyCounter),
	scxReader(m3EventQueue, /*wyReg.reader3(),*/ wxReader, we.enableChecker(), we.disableChecker()),
	spriteMapper(spriteSizeReader, scxReader, oamram),
	breakEvent(drawStartCycle, scReadOffset),
	mode3Event(m3EventQueue, vEventQueue, mode0Irq, irqEvent),
	lycIrq(ifReg),
	mode0Irq(lyCounter, lycIrq, m3ExtraCycles, ifReg),
	mode1Irq(ifReg),
	mode2Irq(lyCounter, lycIrq, ifReg),
	irqEvent(irqEventQueue)
{
	vBlitter = NULL;
	filter = 0;
	pb.pixels = 0;
	pb.format = PixelBuffer::RGB32;
	pb.pitch = 0;
	
	filters.push_back(NULL);
	filters.push_back(new Catrom2x);
	filters.push_back(new Catrom3x);
	filters.push_back(new Kreed_2xSaI);
	filters.push_back(new MaxSt_Hq2x);
	
	//for (uint16_t* i = &spritemap[10];i < &spritemap[144*12];i += 12)
	//	i[1] = i[0] = 0;
	// for (uint32_t i = 0;i < 256;i++)
	// 	xflipt[i] = ((i & 0x1) << 7) | ((i & 0x2) << 5) | ((i & 0x4) << 3) | ((i & 0x8) << 1) | ((i & 0x10) >> 1) | ((i & 0x20) >> 3) | ((i & 0x40) >> 5) | ((i & 0x80) >> 7);
	reset(false);
	setVideoFilter(0);
}

LCD::~LCD() {
// 	delete []filter_buffer;
	for (std::size_t i = 0; i < filters.size(); ++i)
		delete filters[i];
}

void LCD::reset(const bool cgb_in) {
	cgb = cgb_in;
	spriteMapper.setCgb(cgb_in);
	setDBuffer();
	ifReg = 0;
	setDoubleSpeed(false);
	enabled = true;
	bgEnable = true;
	spriteEnable = false;
	wdTileMap = bgTileMap = vram + 0x1800;
	bgTileData = vram;
	tileIndexSign = 0;
	/*drawStartCycle = 90;
	scReadOffset = 90; 
	scxAnd7 = 0;
	largeSprites = false;
	we = false;*/
	
	scxReader.setSource(0);
	wxReader.setSource(0);
	wyReg.setSource(0);
	spriteSizeReader.setSource(false);
	we.setSource(false);
// 	weMasterChecker.setSource(false);
	scReader.setScxSource(0);
	scReader.setScySource(0);
	breakEvent.setScxSource(0);
	lycIrq.setM2IrqEnabled(false);
	lycIrq.setLycReg(0);
	mode1Irq.setM1StatIrqEnabled(false);
	
	resetVideoState(0x80, 0x102A0 - (144*456 + 164));
	//setEvent();
}

void LCD::setDoubleSpeed(const bool ds) {
	doubleSpeed = ds;
	lyCounter.setDoubleSpeed(doubleSpeed);
	scxReader.setDoubleSpeed(doubleSpeed);
	wxReader.setDoubleSpeed(doubleSpeed);
	spriteMapper.setDoubleSpeed(doubleSpeed);
	scReader.setDoubleSpeed(doubleSpeed);
	breakEvent.setDoubleSpeed(doubleSpeed);
	lycIrq.setDoubleSpeed(doubleSpeed);
	mode1Irq.setDoubleSpeed(doubleSpeed);
}

template<class T>
static inline void rescheduleIfActive(T &event, const LyCounter &lyCounter, const unsigned cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	if (event.time() != uint32_t(-1)) {
		event.schedule(lyCounter, cycleCounter);
		queue.push(&event);
	}
}

template<class T>
static inline void rescheduleIfActive(T &event, const ScxReader &scxReader, const LyCounter &lyCounter, const unsigned cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	if (event.time() != uint32_t(-1)) {
		event.schedule(scxReader.scxAnd7(), lyCounter, cycleCounter);
		queue.push(&event);
	}
}

template<class T>
static inline void rescheduleIfActive(T &event, const ScxReader &scxReader, const WxReader &wxReader, const LyCounter &lyCounter, const unsigned cycleCounter, event_queue<VideoEvent*,VideoEventComparer> &queue) {
	if (event.time() != uint32_t(-1)) {
		event.schedule(scxReader.scxAnd7(), wxReader.wx(), lyCounter, cycleCounter);
		queue.push(&event);
	}
}

void LCD::resetVideoState(const unsigned statReg, const unsigned cycleCounter) {
	videoCycles = 0;
	lastUpdate = cycleCounter;
	enableDisplayM0Time = cycleCounter + 159;
	winYPos = 0xFF;
	
	m3EventQueue.clear();
	irqEventQueue.clear();
	vEventQueue.clear();
	
	lyCounter.resetLy();
	lyCounter.setTime(lastUpdate + (456U << doubleSpeed));
	vEventQueue.push(&lyCounter);
	
	scxReader.reset();
	wxReader.reset();
	wyReg.reset();
	spriteSizeReader.reset();
	spriteMapper.reset();
	we.reset();
	scReader.reset();
	breakEvent.reset();
	
	mode3Event.reset();
	
	weMasterChecker.reset();
	weMasterChecker.schedule(wyReg.getSource(), we.getSource(), cycleCounter);
	
	if (weMasterChecker.time() != uint32_t(-1)) {
		m3EventQueue.push(&weMasterChecker);
		mode3Event.schedule();
		vEventQueue.push(&mode3Event);
	}
	
	lycIrq.reset();
	mode0Irq.reset();
	mode2Irq.reset();
	
	if ((statReg & 0x40) && lycIrq.lycReg() < 154) {
		lycIrq.schedule(lyCounter, cycleCounter);
		irqEventQueue.push(&lycIrq);
	}
	
	if (statReg & 0x08) {
		mode0Irq.schedule(lyCounter, cycleCounter);
		irqEventQueue.push(&mode0Irq);
	} else if (statReg & 0x20) {
		mode2Irq.schedule(lyCounter, cycleCounter);
		irqEventQueue.push(&mode2Irq);
	}
	
	mode1Irq.schedule(lyCounter, cycleCounter);
	irqEventQueue.push(&mode1Irq);
	
	irqEvent.schedule();
	vEventQueue.push(&irqEvent);
}

// static VideoBlitter *vBlitter = NULL;
// static PixelBuffer pb = { 0, 0, 0 };

void LCD::setVideoBlitter(VideoBlitter *vb) {
	vBlitter = vb;
	if (vBlitter) {
		vBlitter->setBufferDimensions(filter ? filter->info().outWidth : 160, filter ? filter->info().outHeight : 144);
		pb = vBlitter->inBuffer();
// 		memory.update_bgpalette = 1;
// 		memory.update_objpalette = 1;
	}
	
	setDBuffer();
}

void LCD::videoBufferChange() {
	if (vBlitter) {
		pb = vBlitter->inBuffer();
		setDBuffer();
	}
}

void LCD::setVideoFilter(const uint32_t n) {
	const unsigned int oldw = filter ? filter->info().outWidth : 160;
	const unsigned int oldh = filter ? filter->info().outHeight : 144;
	
	if (filter)
		filter->outit();
	
	filter = filters.at(n < filters.size() ? n : 0);

	if (filter) {
		filter->init();
	}

	if (vBlitter && ((oldw != (filter ? filter->info().outWidth : 160)) || (oldh != (filter ? filter->info().outHeight : 144)))) {
		vBlitter->setBufferDimensions(filter ? filter->info().outWidth : 160, filter ? filter->info().outHeight : 144);
		pb = vBlitter->inBuffer();
	}
	
	setDBuffer();
}

std::vector<const FilterInfo*> LCD::filterInfo() const {
	std::vector<const FilterInfo*> v;
	
	static FilterInfo noInfo = { "None", 160, 144 };
	v.push_back(&noInfo);
	
	for (std::size_t i = 1; i < filters.size(); ++i)
		v.push_back(&filters[i]->info());

	return v;
}

unsigned int LCD::videoWidth() const {
	return filter ? filter->info().outWidth : 160;
}

unsigned int LCD::videoHeight() const {
	return filter ? filter->info().outHeight : 144;
}

void LCD::updateScreen(const unsigned cycleCounter) {
	update(cycleCounter);

	if (filter && pb.pixels) {
		switch (pb.format) {
		case PixelBuffer::RGB32:
			filter->filter(static_cast<Rgb32Putter::pixel_t*>(pb.pixels), pb.pitch, Rgb32Putter());
			break;
		case PixelBuffer::RGB16:
			filter->filter(static_cast<Rgb16Putter::pixel_t*>(pb.pixels), pb.pitch, Rgb16Putter());
			break;
		case PixelBuffer::UYVY:
			filter->filter(static_cast<UyvyPutter::pixel_t*>(pb.pixels), pb.pitch, UyvyPutter());
			break;
		}
	}

	if (vBlitter)
		vBlitter->blit();
}

template<typename T>
static void clear(T *buf, const unsigned color, const unsigned dpitch) {
	unsigned lines = 144;
	
	while (lines--) {
		std::fill_n(buf, 160, color);
		buf += dpitch;
	}
}

void LCD::enableChange(const unsigned statReg, const unsigned cycleCounter) {
	update(cycleCounter);
	enabled = !enabled;
	resetVideoState(statReg, cycleCounter);
	
	if (!enabled && dbuffer) {
		const unsigned color = cgb ? (*gbcToFormat)(0xFFFF) : dmgColors[0];
		
		if (!filter && pb.format == PixelBuffer::RGB16)
			clear(static_cast<uint16_t*>(dbuffer), color, dpitch);
		else
			clear(static_cast<uint32_t*>(dbuffer), color, dpitch);
		
// 		updateScreen(cycleCounter);
	}
}

void LCD::preResetCounter(const unsigned cycleCounter) {
	preSpeedChange(cycleCounter);
}

void LCD::postResetCounter(const unsigned oldCC, const unsigned cycleCounter) {
	enableDisplayM0Time = oldCC > enableDisplayM0Time ? 0 : (cycleCounter + enableDisplayM0Time - oldCC);
	lastUpdate = cycleCounter - (oldCC - lastUpdate);
	rescheduleEvents(cycleCounter);
}

void LCD::preSpeedChange(const unsigned cycleCounter) {
	update(cycleCounter);
}

void LCD::rescheduleEvents(const unsigned cycleCounter) {
	vEventQueue.clear();
	m3EventQueue.clear();
	irqEventQueue.clear();
	
	lyCounter.setTime(lastUpdate + (456 - (videoCycles - lyCounter.ly() * 456) << doubleSpeed));
	vEventQueue.push(&lyCounter);
	
	rescheduleIfActive(scxReader, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(spriteSizeReader, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(spriteMapper, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(wxReader, scxReader, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(wyReg.reader1(), lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(wyReg.reader2(), lyCounter, cycleCounter, m3EventQueue);
// 	rescheduleIfActive(wyReg.reader3(), scxReader, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(wyReg.reader4(), lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(we.disableChecker(), scxReader, wxReader, lyCounter, cycleCounter, m3EventQueue);
	rescheduleIfActive(we.enableChecker(), scxReader, wxReader, lyCounter, cycleCounter, m3EventQueue);
	
	if (wyReg.reader3().time() != uint32_t(-1)) {
		wyReg.reader3().schedule(wxReader.getSource(), scxReader, cycleCounter);
		m3EventQueue.push(&wyReg.reader3());
	}
	
	if (weMasterChecker.time() != uint32_t(-1)) {
		weMasterChecker.schedule(wyReg.getSource(), we.getSource(), cycleCounter);
		m3EventQueue.push(&weMasterChecker);
	}
	
	if (scReader.time() != uint32_t(-1)) {
		scReader.schedule(lastUpdate, videoCycles, scReadOffset);
		vEventQueue.push(&scReader);
	}
	
	rescheduleIfActive(breakEvent, lyCounter, cycleCounter, vEventQueue);
	
	if (!m3EventQueue.empty()) {
		mode3Event.schedule();
		vEventQueue.push(&mode3Event);
	}
	
	rescheduleIfActive(lycIrq, lyCounter, cycleCounter, irqEventQueue);
	rescheduleIfActive(mode0Irq, lyCounter, cycleCounter, irqEventQueue);
	mode1Irq.schedule(lyCounter, cycleCounter);
	irqEventQueue.push(&mode1Irq);
	rescheduleIfActive(mode2Irq, lyCounter, cycleCounter, irqEventQueue);
	
	irqEvent.schedule();
	vEventQueue.push(&irqEvent);
}

void LCD::postSpeedChange(const unsigned cycleCounter) {
	setDoubleSpeed(!doubleSpeed);
	
	rescheduleEvents(cycleCounter);
}

bool LCD::isMode0IrqPeriod(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);

	const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

	return /*memory.enable_display && */lyCounter.ly() < 144 && timeToNextLy <= (456U - (169 + doubleSpeed * 3 + 80 + m3ExtraCycles(lyCounter.ly()) + 1 - doubleSpeed) << doubleSpeed) + doubleSpeed && timeToNextLy > 4;
}

bool LCD::isMode2IrqPeriod(const unsigned cycleCounter) {
	if (cycleCounter >= lyCounter.time())
		update(cycleCounter);

	const unsigned nextLy = lyCounter.time() - cycleCounter;

	return /*memory.enable_display && */((lyCounter.ly() < 143 && nextLy <= 5) || (lyCounter.ly() == 153 && nextLy <= 1));
}

bool LCD::isLycIrqPeriod(const unsigned lycReg, const unsigned endCycles, const unsigned cycleCounter) {
	if (cycleCounter >= lyCounter.time())
		update(cycleCounter);

	const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

	if (lycReg)
		return (lyCounter.ly() == lycReg && timeToNextLy > endCycles) || (lyCounter.ly() < 153 && lyCounter.ly() + 1 == lycReg && timeToNextLy <= 1);
	else
		return (lyCounter.ly() == 153 && timeToNextLy <= ((456U - 8U) << doubleSpeed) + 1) || (lyCounter.ly() == 0 && timeToNextLy > endCycles);

}

bool LCD::isMode1IrqPeriod(const unsigned cycleCounter) {
	if (cycleCounter >= lyCounter.time())
		update(cycleCounter);

	const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

	return lyCounter.ly() > 143 && (lyCounter.ly() < 153 || doubleSpeed || timeToNextLy > 4);
}

bool LCD::isHdmaPeriod(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	const unsigned timeToNextLy = lyCounter.time() - cycleCounter;
	
	return /*memory.enable_display && */lyCounter.ly() < 144 && timeToNextLy <= (456U - (169U + doubleSpeed * 3 + 80U + m3ExtraCycles(lyCounter.ly()) + 2 - doubleSpeed) << doubleSpeed) && timeToNextLy > 4;
}

unsigned LCD::nextHdmaTime(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	const unsigned lineCycles = 456 * 2 - ((lyCounter.time() - cycleCounter) << (1 ^ doubleSpeed));
	unsigned line = lyCounter.ly();
	int next = ((169 + doubleSpeed * 3 + 80 + 2 - doubleSpeed) * 2) - lineCycles;
	unsigned m3ExCs;
	
	if (line < 144) {
		m3ExCs = m3ExtraCycles(line) * 2;
		next += m3ExCs;
		if (next <= 0) {
			next += 456 * 2 - m3ExCs;
			++line;
			if (line < 144) {
				m3ExCs = m3ExtraCycles(line) * 2;
				next += m3ExCs;
			}
		}
	}
	
	if (line > 143) {
		m3ExCs = m3ExtraCycles(0) * 2;
		next += (154 - line) * 456 * 2 + m3ExCs;
	}
	
	return cycleCounter + (next >> (1 ^ doubleSpeed));
}

bool LCD::vramAccessible(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);

	bool accessible = true;

	if (enabled && lyCounter.ly() < 144) {
		const unsigned lineCycles = 456 - ((lyCounter.time() - cycleCounter) >> doubleSpeed);
		if (lineCycles > 79 && lineCycles < 80 + 169 + doubleSpeed * 3 + m3ExtraCycles(lyCounter.ly()))
			accessible = false;
	}

	return accessible;
}

bool LCD::cgbpAccessible(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);

	bool accessible = true;

	if (enabled && lyCounter.ly() < 144) {
		const unsigned lineCycles = 456 - ((lyCounter.time() - cycleCounter/* + 4*/) >> doubleSpeed);
		if (lineCycles > 79U + doubleSpeed && lineCycles < 80U + 169U + doubleSpeed * 3 + m3ExtraCycles(lyCounter.ly()) + 4U - doubleSpeed * 2)
			accessible = false;
	}

	return accessible;
}

bool LCD::oamAccessible(const unsigned cycleCounter) {
	bool accessible = true;

	if (enabled) {
		if (cycleCounter >= vEventQueue.top()->time())
			update(cycleCounter);

		const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

		if (lyCounter.ly() < 144) {
			const unsigned lineCycles = 456 - (timeToNextLy >> doubleSpeed);
			if (lineCycles < 80) {
				if (cycleCounter > enableDisplayM0Time)
					accessible = false;
			} else {
				const unsigned m0start = 80 + 169 + doubleSpeed * 3 + m3ExtraCycles(lyCounter.ly());
				if (lineCycles < m0start || (!doubleSpeed && lineCycles >= 452))
					accessible = false;
			}
		}
	}

	return accessible;
}

void LCD::weChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
// 	printf("%u: weChange: %u\n", videoCycles, newValue);
	
	addEvent(weMasterChecker, wyReg.getSource(), newValue, cycleCounter, m3EventQueue);
// 	addEvent(weMasterChecker, lyCounter, cycleCounter, m3EventQueue);
	
	we.setSource(newValue);
	addEvent(we.disableChecker(), scxReader.scxAnd7(), wxReader.wx(), lyCounter, cycleCounter, m3EventQueue);
	addEvent(we.enableChecker(), scxReader.scxAnd7(), wxReader.wx(), lyCounter, cycleCounter, m3EventQueue);
	
	addEvent(mode3Event, vEventQueue);
}

void LCD::wxChange(const unsigned newValue, const unsigned cycleCounter) {
	update(cycleCounter);
// 	printf("%u: wxChange: 0x%X\n", videoCycles, newValue);
	
	wxReader.setSource(newValue);
	addEvent(wxReader, scxReader.scxAnd7(), lyCounter, cycleCounter, m3EventQueue);
	
	if (wyReg.reader3().time() != uint32_t(-1))
		addEvent(wyReg.reader3(), wxReader.getSource(), scxReader, cycleCounter, m3EventQueue);
	
	addEvent(mode3Event, vEventQueue);
}

void LCD::wyChange(const unsigned newValue, const unsigned cycleCounter) {
	update(cycleCounter);
// 	printf("%u: wyChange: 0x%X\n", videoCycles, newValue);
	
	wyReg.setSource(newValue);
	addEvent(wyReg.reader1(), lyCounter, cycleCounter, m3EventQueue);
	addEvent(wyReg.reader2(), lyCounter, cycleCounter, m3EventQueue);
	addEvent(wyReg.reader3(), wxReader.getSource(), scxReader, cycleCounter, m3EventQueue);
	addEvent(wyReg.reader4(), lyCounter, cycleCounter, m3EventQueue);
	
	addEvent(weMasterChecker, newValue, we.getSource(), cycleCounter, m3EventQueue);
	
	addEvent(mode3Event, vEventQueue);
}

void LCD::scxChange(const unsigned newScx, const unsigned cycleCounter) {
	update(cycleCounter);
	//printf("scxChange\n");
	
	scxReader.setSource(newScx);
	addEvent(scxReader, lyCounter, cycleCounter, m3EventQueue);
	
	addEvent(spriteMapper, lyCounter, cycleCounter, m3EventQueue);
	
	if (wyReg.reader3().time() != uint32_t(-1))
		addEvent(wyReg.reader3(), wxReader.getSource(), scxReader, cycleCounter, m3EventQueue);
	
	addEvent(mode3Event, vEventQueue);
	
	
	const unsigned lineCycles = 456 - ((lyCounter.time() - cycleCounter) >> doubleSpeed);
	
	breakEvent.setScxSource(newScx);
	
	if (lineCycles < 82U + doubleSpeed * 4)
		drawStartCycle = 90 + doubleSpeed * 4 + (newScx & 7);
	else
		addEvent(breakEvent, lyCounter, cycleCounter, vEventQueue);
	
	if (lineCycles < 86U + doubleSpeed * 2)
		scReadOffset = std::max(drawStartCycle - (newScx & 7), 90U + doubleSpeed * 4);
	
	scReader.setScxSource(newScx);
	addEvent(scReader, lastUpdate, videoCycles, scReadOffset, vEventQueue);
}

void LCD::scyChange(const unsigned newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	//printf("scyChange\n");
	
	scReader.setScySource(newValue);
	addEvent(scReader, lastUpdate, videoCycles, scReadOffset, vEventQueue);
}

void LCD::spriteSizeChange(const bool newLarge, const unsigned cycleCounter) {
	update(cycleCounter);
	//printf("spriteSizeChange\n");
	
	spriteSizeReader.setSource(newLarge);
	addEvent(spriteSizeReader, lyCounter, cycleCounter, m3EventQueue);
	
	//if (spriteSizeReader.time() < spriteMapper.time()) {
	//	addEvent(spriteMapper, m3EventQueue);
	//}
	
	addEvent(mode3Event, vEventQueue);
}

void LCD::oamChange(const unsigned cycleCounter) {
	update(cycleCounter);
	//printf("oamChange\n");
	
	addEvent(spriteMapper, lyCounter, cycleCounter, m3EventQueue);
	
	addEvent(mode3Event, vEventQueue);
}

void LCD::wdTileMapSelectChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	
// 	printf("%u: wdTileMapSelectChange: 0x%X\n", videoCycles, newValue);
	
	wdTileMap = vram + (0x1800 | newValue * 0x400);
}

void LCD::bgTileMapSelectChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	
	bgTileMap = vram + (0x1800 | newValue * 0x400);
}

void LCD::bgTileDataSelectChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	
// 	printf("%u: bgTileDataSelectChange: 0x%X\n", videoCycles, newValue);
	
	tileIndexSign = (newValue ^ 1) * 0x80;
	bgTileData = vram + (newValue ^ 1) * 0x1000;
}

void LCD::spriteEnableChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	
	spriteEnable = newValue;
}

void LCD::bgEnableChange(const bool newValue, const unsigned cycleCounter) {
	update(cycleCounter);
	
// 	printf("%u: bgEnableChange: %u\n", videoCycles, newValue);
	
	bgEnable = newValue;
}

void LCD::lcdstatChange(const unsigned old, const unsigned data, const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	mode1Irq.setM1StatIrqEnabled(data & 0x10);
	lycIrq.setM2IrqEnabled(data & 0x20);
	
	if (!enabled)
		return;
	
	const bool lycIrqPeriod = isLycIrqPeriod(lycIrq.lycReg(), lycIrq.lycReg() == 153 ? lyCounter.lineTime() - (4 << (doubleSpeed*2)) : 4 ^ (doubleSpeed << 2), cycleCounter);
	
	if (lycIrq.lycReg() < 154 && ((data ^ old) & 0x40)) {
		if (data & 0x40) {
			if (lycIrqPeriod)
				ifReg |= 2;
			
			lycIrq.schedule(lyCounter, cycleCounter);
			irqEventQueue.push(&lycIrq);
		} else {
			if (!doubleSpeed && lycIrq.time() - cycleCounter < 4 && (!(old & 0x20) || lycIrq.lycReg() > 143 || lycIrq.lycReg() == 0))
				ifReg |= 2;
			
			lycIrq.reset();
			irqEventQueue.remove(&lycIrq);
		}
	}
	
	/*if (data & 0x8) {
		if (!(old & 0x8) && !((old & 0x40) && lycIrqPeriod) && isMode0IrqPeriod(cycleCounter))
			ifReg |= 2;
	} else if ((data & 0x20) && !(old & 0x20) && isMode2IrqPeriod(cycleCounter))
		ifReg |= 2;*/
	
	if (((data & 0x10) && !(old & 0x10) || !cgb) && !((old & 0x40) && lycIrqPeriod) && isMode1IrqPeriod(cycleCounter))
		ifReg |= 2;
	
	if ((data ^ old) & 0x08) {
		if (data & 0x08) {
			if (!((old & 0x40) && lycIrqPeriod) && isMode0IrqPeriod(cycleCounter))
				ifReg |= 2;
			
			mode0Irq.schedule(lyCounter, cycleCounter);
			irqEventQueue.push(&mode0Irq);
		} else {
			if (mode0Irq.time() - cycleCounter < 3 && (lycIrq.time() == uint32_t(-1) || lyCounter.ly() != lycIrq.lycReg()))
				ifReg |= 2;
			
			mode0Irq.reset();
			irqEventQueue.remove(&mode0Irq);
		}
	}
	
	if ((data & 0x28) == 0x20) {
		if ((old & 0x28) != 0x20) {
			if (isMode2IrqPeriod(cycleCounter))
				ifReg |= 2;
			
			mode2Irq.schedule(lyCounter, cycleCounter);
			irqEventQueue.push(&mode2Irq);
		}
	} else if ((old & 0x28) == 0x20) {
		mode2Irq.reset();
		irqEventQueue.remove(&mode2Irq);
	}
	
	/*if ((old ^ data) & 0x28) {
		if (mode0Irq.time() - cycleCounter < 3 && (lycIrq.time() == uint32_t(-1) || lyCounter.ly() != lycIrq.lycReg()))
			ifReg |= 2;

		schedule_mode();
	}*/
	
	modifyEvent(irqEvent, vEventQueue);
}

void LCD::lycRegChange(const unsigned data, const unsigned statReg, const unsigned cycleCounter) {
	if (data == lycIrq.lycReg())
		return;
	
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	const unsigned old = lycIrq.lycReg();
	lycIrq.setLycReg(data);
	
	if (!(enabled && (statReg & 0x40)))
		return;
	
	if (!doubleSpeed && lycIrq.time() - cycleCounter < 4 && (!(statReg & 0x20) || old > 143 || old == 0))
		ifReg |= 2;
	
	if (data < 154) {
		if (isLycIrqPeriod(data, data == 153 ? lyCounter.lineTime() - doubleSpeed * 8 : 8, cycleCounter))
			ifReg |= 2;
		
		const unsigned oldTime = lycIrq.time();
		lycIrq.lycRegSchedule(lyCounter, cycleCounter);
		
		if (oldTime == uint32_t(-1)) {
			irqEventQueue.push(&lycIrq);
		} else if (lycIrq.time() > oldTime)
			irqEventQueue.inc(&lycIrq, &lycIrq);
		else
			irqEventQueue.dec(&lycIrq, &lycIrq);
	} else if (old < 154) {
		lycIrq.reset();
		irqEventQueue.remove(&lycIrq);
	}
	
	modifyEvent(irqEvent, vEventQueue);
}

unsigned LCD::nextIrqEvent() const {
	if (!enabled)
		return uint32_t(-1);
	
	if (mode0Irq.time() != uint32_t(-1) && mode3Event.time() < irqEvent.time())
		return mode3Event.time();
	
	return irqEvent.time();
}

unsigned LCD::getIfReg(const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	return ifReg;
}

void LCD::setIfReg(const unsigned ifReg_in, const unsigned cycleCounter) {
	if (cycleCounter >= vEventQueue.top()->time())
		update(cycleCounter);
	
	ifReg = ifReg_in;
}

unsigned LCD::get_stat(const unsigned lycReg, const unsigned cycleCounter) {
	unsigned stat = 0;

	if (enabled) {
		if (cycleCounter >= vEventQueue.top()->time())
			update(cycleCounter);
			
		const unsigned timeToNextLy = lyCounter.time() - cycleCounter;

		if (lyCounter.ly() > 143) {
			if (lyCounter.ly() < 153 || doubleSpeed || timeToNextLy > 4)
				stat = 1;
		} else {
			const unsigned lineCycles = 456 - (timeToNextLy >> doubleSpeed);
			
			if (lineCycles < 80) {
				if (cycleCounter > enableDisplayM0Time)
					stat = 2;
			} else {
				if (lineCycles < 80 + 169 + doubleSpeed * 3 + m3ExtraCycles(lyCounter.ly()))
					stat = 3;
			}
		}

		unsigned lyc;
		
		if (lycReg)
			lyc = lyCounter.ly() == lycReg && timeToNextLy > 4U - (doubleSpeed << 2);
		else
			lyc = (lyCounter.ly() == 153 && (timeToNextLy >> doubleSpeed) <= 456 - 8) || (lyCounter.ly() == 0 && timeToNextLy > 4U - (doubleSpeed << 2));
		
		stat |= lyc << 2;
	}

	return stat;
}

void LCD::do_update(unsigned cycles) {
	if (lyCounter.ly() < 144) {
		const unsigned lineCycles = 456 - (lyCounter.time() - lastUpdate >> doubleSpeed);
		const unsigned xpos = lineCycles < drawStartCycle ? 0 : lineCycles - drawStartCycle;
		
		const unsigned endLineCycles = lineCycles + cycles;
		unsigned endX = endLineCycles < drawStartCycle ? 0 : endLineCycles - drawStartCycle;
		if (endX > 160)
			endX = 160;
		
		if (xpos < endX)
			(this->*draw)(xpos, lyCounter.ly(), endX);
	} else if (lyCounter.ly() == 144) {
		winYPos = 0xFF;
		//scy[0] = scy[1] = memory.fastread(0xFF42);
		//scx[0] = scx[1] = memory.fastread(0xFF43);
		weMasterChecker.unset();
	}
	
	videoCycles += cycles;
	if (videoCycles >= 70224U)
		videoCycles -= 70224U;
}

inline void LCD::event() {
	vEventQueue.top()->doEvent();
	
	if (vEventQueue.top()->time() == uint32_t(-1))
		vEventQueue.pop();
	else
		vEventQueue.modify_root(vEventQueue.top());
}

void LCD::update(const unsigned cycleCounter) {
	if (!enabled)
		return;
		
	for (;;) {
		const unsigned cycles = (std::max(std::min(cycleCounter, vEventQueue.top()->time()), lastUpdate) - lastUpdate) >> doubleSpeed;
		do_update(cycles);
		lastUpdate += cycles << doubleSpeed;
		
		if (cycleCounter >= vEventQueue.top()->time())
			event();
		else
			break;
	}
}

void LCD::setDBuffer() {
	if (filter) {
		dbuffer = filter->inBuffer();
		dpitch = filter->inPitch();
	} else {
		dbuffer = pb.pixels;
		dpitch = pb.pitch;
	}
	
	/*if (filter || pb.bpp == 16)
		draw = memory.cgb ? &LCD::cgb_draw<uint16_t> : &LCD::dmg_draw<uint16_t>;
	else
		draw = memory.cgb ? &LCD::cgb_draw<uint32_t> : &LCD::dmg_draw<uint32_t>;*/
	
	if (!filter && pb.format == PixelBuffer::RGB16) {
		if (cgb)
			draw = &LCD::cgb_draw<uint16_t>;
		else
			draw = &LCD::dmg_draw<uint16_t>;
		
		gbcToFormat = &gbcToRgb16;
		dmgColors = dmgColorsRgb16;
	} else {
		if (cgb)
			draw = &LCD::cgb_draw<uint32_t>;
		else
			draw = &LCD::dmg_draw<uint32_t>;
		
		if (filter || pb.format == PixelBuffer::RGB32) {
			gbcToFormat = &gbcToRgb32;
			dmgColors = dmgColorsRgb32;
		} else {
			gbcToFormat = &gbcToUyvy;
			dmgColors = dmgColorsUyvy;
		}
	}
	
	if (dbuffer == NULL)
		draw = &LCD::null_draw;
}

void LCD::null_draw(unsigned /*xpos*/, const unsigned ypos, const unsigned endX) {
	const bool enableWindow = (weMasterChecker.weMaster() || ypos == wyReg.value()) && we.value() && wyReg.value() <= ypos && wxReader.wx() < 0xA7;
	
	if (enableWindow && winYPos == 0xFF)
		winYPos = /*ypos - wyReg.value()*/ 0;
	
	if (endX == 160) {
		if (enableWindow)
			++winYPos;
	}
}

template<typename T>
void LCD::cgb_draw(unsigned xpos, const unsigned ypos, const unsigned endX) {
	const unsigned effectiveScx = scReader.scx();
	
	const bool enableWindow = (weMasterChecker.weMaster() || ypos == wyReg.value()) && we.value() && wyReg.value() <= ypos && wxReader.wx() < 0xA7;
	
	if (enableWindow && winYPos == 0xFF)
		winYPos = /*ypos - wyReg.value()*/ 0;
		
	T *const bufLine = static_cast<T*>(dbuffer) + ypos * dpitch;
	
	if (!(enableWindow && wxReader.wx() <= (xpos + 7))) {
		const unsigned fby = (scReader.scy() + ypos)/*&0xFF*/;
		const unsigned end = std::min(enableWindow ? wxReader.wx() - 7 : 160U, endX);

		cgb_bg_drawPixels(bufLine, xpos, end, effectiveScx, bgTileMap + (fby & 0xF8) * 4, bgTileData, fby & 7);
	}
	
	if (enableWindow && (endX + 7) > wxReader.wx()) {
		const unsigned start = std::max(wxReader.wx() < 7 ? 0U : (wxReader.wx() - 7), xpos);
		
		cgb_bg_drawPixels(bufLine, start, endX, 0 - (wxReader.wx() - 7), wdTileMap + (winYPos & 0xF8) * 4, bgTileData, winYPos & 7);
	}
		
	if (endX == 160) {
		if (spriteEnable)
			cgb_drawSprites(bufLine, ypos);
			
		if (enableWindow)
			++winYPos;
	}
}

template<typename T>
void LCD::dmg_draw(unsigned xpos, const unsigned ypos, const unsigned endX) {
	const unsigned effectiveScx = scReader.scx();
	
	const bool enableWindow = (weMasterChecker.weMaster() || ypos == wyReg.value()) && we.value() && wyReg.value() <= ypos && wxReader.wx() < 0xA7;
	
	if (enableWindow && winYPos == 0xFF)
		winYPos = /*ypos - wyReg.value()*/ 0;
		
	T *const bufLine = static_cast<T*>(dbuffer) + ypos * dpitch;
	
	if (bgEnable) {
		if (!(enableWindow && wxReader.wx() <= xpos + 7)) {
			const unsigned fby = (scReader.scy() + ypos)/*&0xFF*/;
			const unsigned end = std::min(enableWindow ? wxReader.wx() - 7 : 160U, endX);
			
			bg_drawPixels(bufLine, xpos, end, effectiveScx, bgTileMap + (fby & 0xF8) * 4, bgTileData + (fby & 7) * 2);
		}
		
		if (enableWindow && endX + 7 > wxReader.wx()) {
			const unsigned start = std::max(wxReader.wx() < 7 ? 0U : (wxReader.wx() - 7), xpos);
			
			bg_drawPixels(bufLine, start, endX, 0 - (wxReader.wx() - 7), wdTileMap + (winYPos & 0xF8) * 4, bgTileData + (winYPos & 7) * 2);
		}
	} else
		std::fill_n(bufLine + xpos, endX - xpos, bgPalette[0]);
		
	if (endX == 160) {
		if (spriteEnable)
			drawSprites(bufLine, ypos);
			
		if (enableWindow)
			++winYPos;
	}
}

#define FLIP(u8) ( (((u8) & 0x01) << 7) | (((u8) & 0x02) << 5) | (((u8) & 0x04) << 3) | (((u8) & 0x08) << 1) | \
(((u8) & 0x10) >> 1) | (((u8) & 0x20) >> 3) | (((u8) & 0x40) >> 5) | (((u8) & 0x80) >> 7) )

#define FLIP_ROW(n) FLIP((n)|0x0), FLIP((n)|0x1), FLIP((n)|0x2), FLIP((n)|0x3), FLIP((n)|0x4), FLIP((n)|0x5), FLIP((n)|0x6), FLIP((n)|0x7), \
FLIP((n)|0x8), FLIP((n)|0x9), FLIP((n)|0xA), FLIP((n)|0xB), FLIP((n)|0xC), FLIP((n)|0xD), FLIP((n)|0xE), FLIP((n)|0xF)

static const uint8_t xflipt[0x100] = {
	FLIP_ROW(0x00), FLIP_ROW(0x10), FLIP_ROW(0x20), FLIP_ROW(0x30),
	FLIP_ROW(0x40), FLIP_ROW(0x50), FLIP_ROW(0x60), FLIP_ROW(0x70),
	FLIP_ROW(0x80), FLIP_ROW(0x90), FLIP_ROW(0xA0), FLIP_ROW(0xB0),
	FLIP_ROW(0xC0), FLIP_ROW(0xD0), FLIP_ROW(0xE0), FLIP_ROW(0xF0)
};

#undef FLIP_ROW
#undef FLIP


//shoud work for the window too, if -wx is passed as scx.
//tilemap and tiledata must point to the areas in the first vram bank
//the second vram bank has to be placed immediately after the first one in memory (0x4000 continous bytes that cover both).
//tilemap needs to be offset to the right line

template<typename T>
void LCD::cgb_bg_drawPixels(T * const buffer_line, unsigned xpos, const unsigned end, const unsigned scx, const uint8_t *const tilemap, const uint8_t *const tiledata, const unsigned tileline) {
	const unsigned sign = tileIndexSign;

	while (xpos < end) {
		if ((scx + xpos & 7) || xpos + 7 >= end) {
			const uint8_t *const maptmp = tilemap + (scx + xpos >> 3 & 0x1F);
			const uintptr_t tile = maptmp[0];
			const unsigned attributes = maptmp[0x2000];
			const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
				(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
			
			unsigned byte1 = data[0];
			unsigned byte2 = data[1];
			
			if (attributes & 0x20) {
				byte1 = xflipt[byte1];
				byte2 = xflipt[byte2];
			}
			
			byte2 <<= 1;
			
			const uint32_t *const palette = bgPalette + (attributes & 7) * 4;
			unsigned tmp = 7 - (scx + xpos & 7);
			
			do {
				buffer_line[xpos++] = palette[byte2 >> tmp & 2 | byte1 >> tmp & 1];
			} while (tmp-- && xpos < end);
		}
		
		while (xpos + 7 < end) {
			const uint8_t *const maptmp = tilemap + (scx + xpos >> 3 & 0x1F);
			const uintptr_t tile = maptmp[0];
			const unsigned attributes = maptmp[0x2000];
			const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
				(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
			
			unsigned byte1 = data[0];
			unsigned byte2 = data[1];
			
			if (attributes & 0x20) {
				byte1 = xflipt[byte1];
				byte2 = xflipt[byte2];
			}
			
			byte2 <<= 1;
			
			const uint32_t *const palette = bgPalette + (attributes & 7) * 4;
			T * const buf = buffer_line + xpos;
			
			buf[0] = palette[(byte2 & 0x100 | byte1) >> 7];
			buf[1] = palette[(byte2 & 0x80 | byte1 & 0x40) >> 6];
			buf[2] = palette[(byte2 & 0x40 | byte1 & 0x20) >> 5];
			buf[3] = palette[(byte2 & 0x20 | byte1 & 0x10) >> 4];
			buf[4] = palette[(byte2 & 0x10 | byte1 & 0x8) >> 3];
			buf[5] = palette[(byte2 & 0x8 | byte1 & 0x4) >> 2];
			buf[6] = palette[(byte2 & 0x4 | byte1 & 0x2) >> 1];
			buf[7] = palette[byte2 & 0x2 | byte1 & 0x1];
			
			xpos += 8;
		}
	}
}

static unsigned cgb_prioritizedBG_mask(const unsigned spx, const unsigned bgStart, const unsigned bgEnd, const unsigned scx,
                                       const uint8_t *const tilemap, const uint8_t *const tiledata, const unsigned tileline, const unsigned sign) {
	const unsigned spStart = spx < bgStart + 8 ? bgStart - (spx - 8) : 0;

	unsigned bgbyte;
	
	{
		const unsigned pos = scx + (spx - 8) + spStart;
		const uint8_t *maptmp = tilemap + (pos >> 3 & 0x1F);
		uintptr_t tile = maptmp[0];
		unsigned attributes = maptmp[0x2000];
		
		const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
			(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
		
		bgbyte = (attributes & 0x20) ? xflipt[data[0] | data[1]] : (data[0] | data[1]);
		
		const unsigned offset = pos & 7;
		
		if (offset) {
			bgbyte <<= offset;
			maptmp = tilemap + ((pos >> 3) + 1 & 0x1F);
			tile = maptmp[0];
			attributes = maptmp[0x2000];
			
			const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
				(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
			
			bgbyte |= ((attributes & 0x20) ? xflipt[data[0] | data[1]] : (data[0] | data[1])) >> (8 - offset);
		}
	}
	
	bgbyte >>= spStart;
	const unsigned spEnd = spx > bgEnd ? bgEnd - (spx - 8) : 8;
	const unsigned mask = ~bgbyte | (0xFF >> spEnd);
	
	return mask;
}

static unsigned cgb_toplayerBG_mask(const unsigned spx, const unsigned bgStart, const unsigned bgEnd, const unsigned scx,
                                    const uint8_t *const tilemap, const uint8_t *const tiledata, const unsigned tileline, const unsigned sign) {
	const unsigned spStart = spx < bgStart + 8 ? bgStart - (spx - 8) : 0;

	unsigned bgbyte = 0;

	{
		const unsigned pos = scx + (spx - 8) + spStart;
		const uint8_t *maptmp = tilemap + (pos >> 3 & 0x1F);
		unsigned attributes = maptmp[0x2000];
		
		if (attributes & 0x80) {
			const uintptr_t tile = maptmp[0];
			
			const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
				(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
			
			bgbyte = (attributes & 0x20) ? xflipt[data[0] | data[1]] : (data[0] | data[1]);
		}
		
		const unsigned offset = pos & 7;
		
		if (offset) {
			bgbyte <<= offset;
			maptmp = tilemap + ((pos >> 3) + 1 & 0x1F);
			attributes = maptmp[0x2000];
			
			if (attributes & 0x80) {
				const uintptr_t tile = maptmp[0];
				
				const uint8_t *const data = tiledata + (attributes << 10 & 0x2000) +
					(tile - (tile & sign) * 2 << 4) + ((attributes & 0x40) ? 7 - tileline : tileline) * 2;
				
				bgbyte |= ((attributes & 0x20) ? xflipt[data[0] | data[1]] : (data[0] | data[1])) >> (8 - offset);
			}
		}
	}

	bgbyte >>= spStart;
	const unsigned spEnd = spx > bgEnd ? bgEnd - (spx - 8) : 8;
	const unsigned mask = ~bgbyte | (0xFF >> spEnd);
	
	return mask;
}

template<typename T>
void LCD::cgb_drawSprites(T * const buffer_line, const unsigned ypos) {
	const unsigned scy = (scReader.scy() + ypos)/*&0xFF*/;
	const unsigned wx = wxReader.wx() < 7 ? 0 : wxReader.wx() - 7;
	//const bool enableWindow = (memory.fastread(0xFF40) & 0x20) && (ypos >= memory.fastread(0xFF4A)) && (wx < 160);
	const bool enableWindow = (weMasterChecker.weMaster() || ypos == wyReg.value()) && we.value() && wyReg.value() <= ypos && wxReader.wx() < 0xA7;
	
	for (int i = spriteMapper.spriteMap()[ypos * 12 + 10] - 1; i >= 0; --i) {
		const uint8_t *const spriteInfo = spriteMapper.oamram + (spriteMapper.spriteMap()[ypos*12+i] & 0xFF);
		const unsigned spx = spriteInfo[1];
		
		if (spx < 168 && spx) {
			unsigned spLine = ypos - (spriteInfo[0] - 16);
			unsigned spTile = spriteInfo[2];
			const unsigned attributes = spriteInfo[3];

			if (spriteSizeReader.largeSprites()) { //large sprite
				if (attributes & 0x40) //yflip
					spLine = 15 - spLine;
				
				if (spLine < 8)
					spTile &= 0xFE;
				else {
					spLine -= 8;
					spTile |= 0x01;
				}
			} else {
				if (attributes & 0x40) //yflip
					spLine = 7 - spLine;
			}
			
			const uint8_t *const data = vram + ((attributes * 0x400) & 0x2000) + (spTile << 4) + spLine * 2;

			unsigned byte1 = data[0];
			unsigned byte2 = data[1];
			
			if (attributes & 0x20) {
				byte1 = xflipt[byte1];
				byte2 = xflipt[byte2];
			}

			//(Sprites with priority-bit are still allowed to cover other sprites according to GBdev-faq.)
			if (bgEnable) {
				unsigned mask = 0xFF;
				
				if (attributes & 0x80) {
					if (!(enableWindow && (wx == 0 || spx >= unsigned(wx + 8))))
						mask = cgb_prioritizedBG_mask(spx, 0, enableWindow ? wx : 160, scReader.scx(),
						                              bgTileMap + ((scy & 0xF8) << 2), bgTileData, scy & 7, tileIndexSign);
					if (enableWindow && spx > wx)
						mask &= cgb_prioritizedBG_mask(spx, wx, 160, 0 - wx, wdTileMap + ((winYPos & 0xF8) << 2), bgTileData, winYPos & 7, tileIndexSign);
				} else {
					if (!(enableWindow && (wx == 0 || spx >= unsigned(wx + 8))))
						mask = cgb_toplayerBG_mask(spx, 0, enableWindow ? wx : 160, scReader.scx(),
						                           bgTileMap + ((scy & 0xF8) << 2), bgTileData, scy & 7, tileIndexSign);
					if (enableWindow && spx > wx)
						mask &= cgb_toplayerBG_mask(spx, wx, 160, 0 - wx, wdTileMap + ((winYPos & 0xF8) << 2), bgTileData, winYPos & 7, tileIndexSign);
				}
				
				byte1 &= mask;
				byte2 &= mask;
			}

			byte2 <<= 1;

			const uint32_t *const palette = spPalette + (attributes & 7) * 4;

			if (spx > 7 && spx < 161) {
				T * const buf = buffer_line + (spx - 8);
				unsigned color;

				if ((color = (byte2 & 0x100 | byte1) >> 7))
					buf[0] = palette[color];
				if ((color = byte2 & 0x80 | byte1 & 0x40))
					buf[1] = palette[color >> 6];
				if ((color = byte2 & 0x40 | byte1 & 0x20))
					buf[2] = palette[color >> 5];
				if ((color = byte2 & 0x20 | byte1 & 0x10))
					buf[3] = palette[color >> 4];
				if ((color = byte2 & 0x10 | byte1 & 0x8))
					buf[4] = palette[color >> 3];
				if ((color = byte2 & 0x8 | byte1 & 0x4))
					buf[5] = palette[color >> 2];
				if ((color = byte2 & 0x4 | byte1 & 0x2))
					buf[6] = palette[color >> 1];
				if ((color = byte2 & 0x2 | byte1 & 0x1))
					buf[7] = palette[color];
			} else {
				const unsigned end = spx >= 160 ? 160 : spx;
				unsigned xpos = spx <= 8 ? 0 : (spx - 8);
				unsigned u32temp = 7 - (xpos - (spx - 8));
				
				while (xpos < end) {
					const unsigned color = byte2 >> u32temp & 2 | byte1 >> u32temp & 1;
					
					if (color)
						buffer_line[xpos] = palette[color];
					
					--u32temp,
					++xpos;
				}
			}
		}
	}
}


//shoud work for the window too, if -wx is passed as scx.
//tilemap and tiledata need to be offset to the right line
template<typename T>
void LCD::bg_drawPixels(T * const buffer_line, unsigned xpos, const unsigned end, const unsigned scx, const uint8_t *const tilemap, const uint8_t *const tiledata) {
	const unsigned sign = tileIndexSign;

	while (xpos < end) {
		if ((scx + xpos & 7) || xpos + 7 >= end) {
			const uintptr_t tile = tilemap[scx + xpos >> 3 & 0x1F];
			const uint8_t *const data = tiledata + (tile - (tile & sign) * 2 << 4);
			const unsigned byte1 = data[0];
			const unsigned byte2 = data[1] << 1;
			unsigned tmp = 7 - (scx + xpos & 7);
			
			do {
				buffer_line[xpos++] = bgPalette[byte2 >> tmp & 2 | byte1 >> tmp & 1];
			} while (tmp-- && xpos < end);
		}
		
		while (xpos + 7 < end) {
			const uintptr_t tile = tilemap[scx + xpos >> 3 & 0x1F];
			const uint8_t *const data = tiledata + (tile - (tile & sign) * 2 << 4);
			const unsigned byte1 = data[0];
			const unsigned byte2 = data[1] << 1;
			T * const buf = buffer_line + xpos;
			buf[0] = bgPalette[(byte2 & 0x100 | byte1) >> 7];
			buf[1] = bgPalette[(byte2 & 0x80 | byte1 & 0x40) >> 6];
			buf[2] = bgPalette[(byte2 & 0x40 | byte1 & 0x20) >> 5];
			buf[3] = bgPalette[(byte2 & 0x20 | byte1 & 0x10) >> 4];
			buf[4] = bgPalette[(byte2 & 0x10 | byte1 & 0x8) >> 3];
			buf[5] = bgPalette[(byte2 & 0x8 | byte1 & 0x4) >> 2];
			buf[6] = bgPalette[(byte2 & 0x4 | byte1 & 0x2) >> 1];
			buf[7] = bgPalette[byte2 & 0x2 | byte1 & 0x1];
			xpos += 8;
		}
	}
}

static unsigned prioritizedBG_mask(const unsigned spx, const unsigned bgStart, const unsigned bgEnd, const unsigned scx,
                                   const uint8_t *const tilemap, const uint8_t *const tiledata, const unsigned sign) {
	const unsigned spStart = spx < bgStart + 8 ? bgStart - (spx - 8) : 0;

	unsigned bgbyte;
	
	{
		const unsigned pos = scx + (spx - 8) + spStart;
		uintptr_t tile = tilemap[pos >> 3 & 0x1F];
		const uint8_t *data = tiledata + (tile - (tile & sign) * 2 << 4);
		bgbyte = data[0] | data[1];
		const unsigned offset = pos & 7;
		
		if (offset) {
			bgbyte <<= offset;
			tile = tilemap[(pos >> 3) + 1 & 0x1F];
			data = tiledata + (tile - (tile & sign) * 2 << 4);
			bgbyte |= (data[0] | data[1]) >> (8 - offset);
		}
	}
	
	bgbyte >>= spStart;
	const unsigned spEnd = spx > bgEnd ? bgEnd - (spx - 8) : 8;
	const unsigned mask = ~bgbyte | (0xFF >> spEnd);
	
	return mask;
}

template<typename T>
void LCD::drawSprites(T * const buffer_line, const unsigned ypos) {
	const unsigned scy = (scReader.scy() + ypos)/*&0xFF*/;
	const unsigned wx = wxReader.wx() < 7 ? 0 : wxReader.wx() - 7;
	const bool enableWindow = (weMasterChecker.weMaster() || ypos == wyReg.value()) && we.value() && wyReg.value() <= ypos && wxReader.wx() < 0xA7;
	
	for (int i = spriteMapper.spriteMap()[ypos * 12 + 10] - 1; i >= 0; --i) {
		const uint8_t *const spriteInfo = spriteMapper.oamram + (spriteMapper.spriteMap()[ypos * 12 + i] & 0xFF);
		const unsigned spx = spriteInfo[1];
		
		if (spx < 168 && spx) {
			unsigned spLine = ypos - (spriteInfo[0] - 16);
			unsigned spTile = spriteInfo[2];
			const unsigned attributes = spriteInfo[3];

			if (spriteSizeReader.largeSprites()) {
				if (attributes & 0x40) //yflip
					spLine = 15 - spLine;
				
				if (spLine < 8)
					spTile &= 0xFE;
				else {
					spLine -= 8;
					spTile |= 0x01;
				}
			} else {
				if (attributes & 0x40) //yflip
					spLine = 7 - spLine;
			}
			
			const uint8_t *const data = vram + (spTile << 4) + spLine * 2;

			unsigned byte1 = data[0];
			unsigned byte2 = data[1];
			
			if (attributes & 0x20) {
				byte1 = xflipt[byte1];
				byte2 = xflipt[byte2];
			}

			//(Sprites with priority-bit are still allowed to cover other sprites according to GBdev-faq.)
			if (attributes & 0x80) {
				unsigned mask = 0xFF;
				
				if (bgEnable && !(enableWindow && (wx == 0 || spx >= unsigned(wx + 8))))
					mask = prioritizedBG_mask(spx, 0, enableWindow ? wx : 160, scReader.scx(),
					                          bgTileMap + ((scy & 0xF8) << 2), bgTileData + ((scy & 7) << 1), tileIndexSign);
				if (enableWindow && spx > wx)
					mask &= prioritizedBG_mask(spx, wx, 160, 0 - wx, wdTileMap + ((winYPos & 0xF8) << 2), bgTileData + ((winYPos & 7) << 1), tileIndexSign);
				
				byte1 &= mask;
				byte2 &= mask;
			}

			byte2 <<= 1;

			const uint32_t *const palette = spPalette + ((attributes >> 2) & 4);

			if (spx > 7 && spx < 161) {
				T * const buf = buffer_line + (spx - 8);
				unsigned color;
				
				if ((color = (byte2 & 0x100 | byte1) >> 7))
					buf[0] = palette[color];
				if ((color = byte2 & 0x80 | byte1 & 0x40))
					buf[1] = palette[color >> 6];
				if ((color = byte2 & 0x40 | byte1 & 0x20))
					buf[2] = palette[color >> 5];
				if ((color = byte2 & 0x20 | byte1 & 0x10))
					buf[3] = palette[color >> 4];
				if ((color = byte2 & 0x10 | byte1 & 0x8))
					buf[4] = palette[color >> 3];
				if ((color = byte2 & 0x8 | byte1 & 0x4))
					buf[5] = palette[color >> 2];
				if ((color = byte2 & 0x4 | byte1 & 0x2))
					buf[6] = palette[color >> 1];
				if ((color = byte2 & 0x2 | byte1 & 0x1))
					buf[7] = palette[color];
			} else {
				const unsigned end = spx >= 160 ? 160 : spx;
				unsigned xpos = spx <= 8 ? 0 : (spx - 8);
				unsigned u32temp = 7 - (xpos - (spx - 8));
				
				while (xpos < end) {
					const unsigned color = byte2 >> u32temp & 2 | byte1 >> u32temp & 1;
					
					if (color)
						buffer_line[xpos] = palette[color];
					
					--u32temp,
					++xpos;
				}
			}
		}
	}
}
