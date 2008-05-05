/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
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
#include "state_osd_elements.h"
#include "bitmap_font.h"
#include "statesaver.h"
#include <fstream>
#include <cstring>

using namespace BitmapFont;

static const char stateLoadedTxt[] = { S,t,a,t,e,SPC,N0,SPC,l,o,a,d,e,d,0 };
static const char stateSavedTxt[] = { S,t,a,t,e,SPC,N0,SPC,s,a,v,e,d,0 };
static const unsigned stateLoadedTxtWidth = getWidth(stateLoadedTxt);
static const unsigned stateSavedTxtWidth = getWidth(stateSavedTxt);

class ShadedTextOsdElment : public OsdElement {
	Gambatte::uint_least32_t *const pixels;
	unsigned life;
	
public:
	ShadedTextOsdElment(unsigned w, const char *txt);
	~ShadedTextOsdElment();
	const Gambatte::uint_least32_t* update();
};

ShadedTextOsdElment::ShadedTextOsdElment(unsigned width, const char *txt) :
OsdElement(MAX_WIDTH, 144 - HEIGHT - HEIGHT / 2, width + 1, HEIGHT + 1),
pixels(new Gambatte::uint_least32_t[(width + 1) * (HEIGHT + 1)]),
life(4 * 60) {
	std::memset(pixels, 0xFF, w() * h() * sizeof(Gambatte::uint_least32_t));
	BitmapFont::print(pixels + w() + 1, w(), 0x000000, txt);
	BitmapFont::print(pixels, w(), 0xFFFFFF, txt);
}

ShadedTextOsdElment::~ShadedTextOsdElment() {
	delete []pixels;
}

const Gambatte::uint_least32_t* ShadedTextOsdElment::update() {
	if (life--)
		return pixels;
	
	return 0;
}

std::auto_ptr<OsdElement> newStateLoadedOsdElement(unsigned stateNo) {
	char txt[sizeof(stateLoadedTxt)];
	
	std::memcpy(txt, stateLoadedTxt, sizeof(stateLoadedTxt));
	utoa(stateNo, txt + 6);
	
	return std::auto_ptr<OsdElement>(new ShadedTextOsdElment(stateLoadedTxtWidth, txt));
}

std::auto_ptr<OsdElement> newStateSavedOsdElement(unsigned stateNo) {
	char txt[sizeof(stateSavedTxt)];
	
	std::memcpy(txt, stateSavedTxt, sizeof(stateSavedTxt));
	utoa(stateNo, txt + 6);
	
	return std::auto_ptr<OsdElement>(new ShadedTextOsdElment(stateSavedTxtWidth, txt));
}

class SaveStateOsdElement : public OsdElement {
	Gambatte::uint_least32_t pixels[StateSaver::SS_WIDTH * StateSaver::SS_HEIGHT];
	unsigned life;
	
public:
	SaveStateOsdElement(const char *fileName, unsigned stateNo);
	const Gambatte::uint_least32_t* update();
};

SaveStateOsdElement::SaveStateOsdElement(const char *fileName, unsigned stateNo) :
OsdElement(stateNo * ((160 - StateSaver::SS_WIDTH) / 10) + ((160 - StateSaver::SS_WIDTH) / 10) / 2, 4, StateSaver::SS_WIDTH, StateSaver::SS_HEIGHT),
life(4 * 60) {
	std::ifstream file(fileName);
	
	if (file.is_open()) {
		file.ignore(5);
		file.read(reinterpret_cast<char*>(pixels), sizeof(pixels));
	} else {
		std::memset(pixels, 0, sizeof(pixels));
		
		{
			using namespace BitmapFont;
			
			static const char txt[] = { E,m,p,t,BitmapFont::y,0 };
			
			print(pixels + 3 + (StateSaver::SS_HEIGHT / 2 - BitmapFont::HEIGHT / 2) * StateSaver::SS_WIDTH, StateSaver::SS_WIDTH, 0x808080, txt);
		}
	}
}

const Gambatte::uint_least32_t* SaveStateOsdElement::update() {
	if (life--)
		return pixels;
	
	return 0;
}

std::auto_ptr<OsdElement> newSaveStateOsdElement(const char *fileName, unsigned stateNo) {
	return std::auto_ptr<OsdElement>(new SaveStateOsdElement(fileName, stateNo));
}
