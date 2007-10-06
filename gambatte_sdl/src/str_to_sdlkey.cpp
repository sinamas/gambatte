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

#include "str_to_sdlkey.h"

#include <map>
#include <cstring>

struct StrLess {
	bool operator()(const char *const l, const char *const r) const {
		return std::strcmp(l, r) < 0;
	}
};

typedef std::map<const char*,SDLKey,StrLess> map_t;

static map_t m;

static void init() {
	m.insert(std::pair<const char*,SDLKey>("backspace", SDLK_BACKSPACE));
	m.insert(std::pair<const char*,SDLKey>("tab", SDLK_TAB));
	m.insert(std::pair<const char*,SDLKey>("clear", SDLK_CLEAR));
	m.insert(std::pair<const char*,SDLKey>("return", SDLK_RETURN));
	m.insert(std::pair<const char*,SDLKey>("pause", SDLK_PAUSE));
	m.insert(std::pair<const char*,SDLKey>("escape", SDLK_ESCAPE));
	m.insert(std::pair<const char*,SDLKey>("space", SDLK_SPACE));
	m.insert(std::pair<const char*,SDLKey>("!", SDLK_EXCLAIM));
	m.insert(std::pair<const char*,SDLKey>("quotedbl", SDLK_QUOTEDBL));
	m.insert(std::pair<const char*,SDLKey>("hash", SDLK_HASH));
	m.insert(std::pair<const char*,SDLKey>("$", SDLK_DOLLAR));
	m.insert(std::pair<const char*,SDLKey>("ampersand", SDLK_AMPERSAND));
	m.insert(std::pair<const char*,SDLKey>("quote", SDLK_QUOTE));
	m.insert(std::pair<const char*,SDLKey>("leftparen", SDLK_LEFTPAREN));
	m.insert(std::pair<const char*,SDLKey>("rightparen", SDLK_RIGHTPAREN));
	m.insert(std::pair<const char*,SDLKey>("asterisk", SDLK_ASTERISK));
	m.insert(std::pair<const char*,SDLKey>("+", SDLK_PLUS));
	m.insert(std::pair<const char*,SDLKey>(",", SDLK_COMMA));
	m.insert(std::pair<const char*,SDLKey>("-", SDLK_MINUS));
	m.insert(std::pair<const char*,SDLKey>("period", SDLK_PERIOD));
	m.insert(std::pair<const char*,SDLKey>("slash", SDLK_SLASH));
	m.insert(std::pair<const char*,SDLKey>("0", SDLK_0));
	m.insert(std::pair<const char*,SDLKey>("1", SDLK_1));
	m.insert(std::pair<const char*,SDLKey>("2", SDLK_2));
	m.insert(std::pair<const char*,SDLKey>("3", SDLK_3));
	m.insert(std::pair<const char*,SDLKey>("4", SDLK_4));
	m.insert(std::pair<const char*,SDLKey>("5", SDLK_5));
	m.insert(std::pair<const char*,SDLKey>("6", SDLK_6));
	m.insert(std::pair<const char*,SDLKey>("7", SDLK_7));
	m.insert(std::pair<const char*,SDLKey>("8", SDLK_8));
	m.insert(std::pair<const char*,SDLKey>("9", SDLK_9));
	m.insert(std::pair<const char*,SDLKey>(":", SDLK_COLON));
	m.insert(std::pair<const char*,SDLKey>("semicolon", SDLK_SEMICOLON));
	m.insert(std::pair<const char*,SDLKey>("less", SDLK_LESS));
	m.insert(std::pair<const char*,SDLKey>("=", SDLK_EQUALS));
	m.insert(std::pair<const char*,SDLKey>("greater", SDLK_GREATER));
	m.insert(std::pair<const char*,SDLKey>("question", SDLK_QUESTION));
	m.insert(std::pair<const char*,SDLKey>("@", SDLK_AT));
	m.insert(std::pair<const char*,SDLKey>("[", SDLK_LEFTBRACKET));
	m.insert(std::pair<const char*,SDLKey>("backslash", SDLK_BACKSLASH));
	m.insert(std::pair<const char*,SDLKey>("]", SDLK_RIGHTBRACKET));
	m.insert(std::pair<const char*,SDLKey>("^", SDLK_CARET));
	m.insert(std::pair<const char*,SDLKey>("_", SDLK_UNDERSCORE));
	m.insert(std::pair<const char*,SDLKey>("backquote", SDLK_BACKQUOTE));
	m.insert(std::pair<const char*,SDLKey>("a", SDLK_a));
	m.insert(std::pair<const char*,SDLKey>("b", SDLK_b));
	m.insert(std::pair<const char*,SDLKey>("c", SDLK_c));
	m.insert(std::pair<const char*,SDLKey>("d", SDLK_d));
	m.insert(std::pair<const char*,SDLKey>("e", SDLK_e));
	m.insert(std::pair<const char*,SDLKey>("f", SDLK_f));
	m.insert(std::pair<const char*,SDLKey>("g", SDLK_g));
	m.insert(std::pair<const char*,SDLKey>("h", SDLK_h));
	m.insert(std::pair<const char*,SDLKey>("i", SDLK_i));
	m.insert(std::pair<const char*,SDLKey>("j", SDLK_j));
	m.insert(std::pair<const char*,SDLKey>("k", SDLK_k));
	m.insert(std::pair<const char*,SDLKey>("l", SDLK_l));
	m.insert(std::pair<const char*,SDLKey>("m", SDLK_m));
	m.insert(std::pair<const char*,SDLKey>("n", SDLK_n));
	m.insert(std::pair<const char*,SDLKey>("o", SDLK_o));
	m.insert(std::pair<const char*,SDLKey>("p", SDLK_p));
	m.insert(std::pair<const char*,SDLKey>("q", SDLK_q));
	m.insert(std::pair<const char*,SDLKey>("r", SDLK_r));
	m.insert(std::pair<const char*,SDLKey>("s", SDLK_s));
	m.insert(std::pair<const char*,SDLKey>("t", SDLK_t));
	m.insert(std::pair<const char*,SDLKey>("u", SDLK_u));
	m.insert(std::pair<const char*,SDLKey>("v", SDLK_v));
	m.insert(std::pair<const char*,SDLKey>("w", SDLK_w));
	m.insert(std::pair<const char*,SDLKey>("x", SDLK_x));
	m.insert(std::pair<const char*,SDLKey>("y", SDLK_y));
	m.insert(std::pair<const char*,SDLKey>("z", SDLK_z));
	m.insert(std::pair<const char*,SDLKey>("delete", SDLK_DELETE));
	m.insert(std::pair<const char*,SDLKey>("world_0", SDLK_WORLD_0));
	m.insert(std::pair<const char*,SDLKey>("world_1", SDLK_WORLD_1));
	m.insert(std::pair<const char*,SDLKey>("world_2", SDLK_WORLD_2));
	m.insert(std::pair<const char*,SDLKey>("world_3", SDLK_WORLD_3));
	m.insert(std::pair<const char*,SDLKey>("world_4", SDLK_WORLD_4));
	m.insert(std::pair<const char*,SDLKey>("world_5", SDLK_WORLD_5));
	m.insert(std::pair<const char*,SDLKey>("world_6", SDLK_WORLD_6));
	m.insert(std::pair<const char*,SDLKey>("world_7", SDLK_WORLD_7));
	m.insert(std::pair<const char*,SDLKey>("world_8", SDLK_WORLD_8));
	m.insert(std::pair<const char*,SDLKey>("world_9", SDLK_WORLD_9));
	m.insert(std::pair<const char*,SDLKey>("world_10", SDLK_WORLD_10));
	m.insert(std::pair<const char*,SDLKey>("world_11", SDLK_WORLD_11));
	m.insert(std::pair<const char*,SDLKey>("world_12", SDLK_WORLD_12));
	m.insert(std::pair<const char*,SDLKey>("world_13", SDLK_WORLD_13));
	m.insert(std::pair<const char*,SDLKey>("world_14", SDLK_WORLD_14));
	m.insert(std::pair<const char*,SDLKey>("world_15", SDLK_WORLD_15));
	m.insert(std::pair<const char*,SDLKey>("world_16", SDLK_WORLD_16));
	m.insert(std::pair<const char*,SDLKey>("world_17", SDLK_WORLD_17));
	m.insert(std::pair<const char*,SDLKey>("world_18", SDLK_WORLD_18));
	m.insert(std::pair<const char*,SDLKey>("world_19", SDLK_WORLD_19));
	m.insert(std::pair<const char*,SDLKey>("world_20", SDLK_WORLD_20));
	m.insert(std::pair<const char*,SDLKey>("world_21", SDLK_WORLD_21));
	m.insert(std::pair<const char*,SDLKey>("world_22", SDLK_WORLD_22));
	m.insert(std::pair<const char*,SDLKey>("world_23", SDLK_WORLD_23));
	m.insert(std::pair<const char*,SDLKey>("world_24", SDLK_WORLD_24));
	m.insert(std::pair<const char*,SDLKey>("world_25", SDLK_WORLD_25));
	m.insert(std::pair<const char*,SDLKey>("world_26", SDLK_WORLD_26));
	m.insert(std::pair<const char*,SDLKey>("world_27", SDLK_WORLD_27));
	m.insert(std::pair<const char*,SDLKey>("world_28", SDLK_WORLD_28));
	m.insert(std::pair<const char*,SDLKey>("world_29", SDLK_WORLD_29));
	m.insert(std::pair<const char*,SDLKey>("world_30", SDLK_WORLD_30));
	m.insert(std::pair<const char*,SDLKey>("world_31", SDLK_WORLD_31));
	m.insert(std::pair<const char*,SDLKey>("world_32", SDLK_WORLD_32));
	m.insert(std::pair<const char*,SDLKey>("world_33", SDLK_WORLD_33));
	m.insert(std::pair<const char*,SDLKey>("world_34", SDLK_WORLD_34));
	m.insert(std::pair<const char*,SDLKey>("world_35", SDLK_WORLD_35));
	m.insert(std::pair<const char*,SDLKey>("world_36", SDLK_WORLD_36));
	m.insert(std::pair<const char*,SDLKey>("world_37", SDLK_WORLD_37));
	m.insert(std::pair<const char*,SDLKey>("world_38", SDLK_WORLD_38));
	m.insert(std::pair<const char*,SDLKey>("world_39", SDLK_WORLD_39));
	m.insert(std::pair<const char*,SDLKey>("world_40", SDLK_WORLD_40));
	m.insert(std::pair<const char*,SDLKey>("world_41", SDLK_WORLD_41));
	m.insert(std::pair<const char*,SDLKey>("world_42", SDLK_WORLD_42));
	m.insert(std::pair<const char*,SDLKey>("world_43", SDLK_WORLD_43));
	m.insert(std::pair<const char*,SDLKey>("world_44", SDLK_WORLD_44));
	m.insert(std::pair<const char*,SDLKey>("world_45", SDLK_WORLD_45));
	m.insert(std::pair<const char*,SDLKey>("world_46", SDLK_WORLD_46));
	m.insert(std::pair<const char*,SDLKey>("world_47", SDLK_WORLD_47));
	m.insert(std::pair<const char*,SDLKey>("world_48", SDLK_WORLD_48));
	m.insert(std::pair<const char*,SDLKey>("world_49", SDLK_WORLD_49));
	m.insert(std::pair<const char*,SDLKey>("world_50", SDLK_WORLD_50));
	m.insert(std::pair<const char*,SDLKey>("world_51", SDLK_WORLD_51));
	m.insert(std::pair<const char*,SDLKey>("world_52", SDLK_WORLD_52));
	m.insert(std::pair<const char*,SDLKey>("world_53", SDLK_WORLD_53));
	m.insert(std::pair<const char*,SDLKey>("world_54", SDLK_WORLD_54));
	m.insert(std::pair<const char*,SDLKey>("world_55", SDLK_WORLD_55));
	m.insert(std::pair<const char*,SDLKey>("world_56", SDLK_WORLD_56));
	m.insert(std::pair<const char*,SDLKey>("world_57", SDLK_WORLD_57));
	m.insert(std::pair<const char*,SDLKey>("world_58", SDLK_WORLD_58));
	m.insert(std::pair<const char*,SDLKey>("world_59", SDLK_WORLD_59));
	m.insert(std::pair<const char*,SDLKey>("world_60", SDLK_WORLD_60));
	m.insert(std::pair<const char*,SDLKey>("world_61", SDLK_WORLD_61));
	m.insert(std::pair<const char*,SDLKey>("world_62", SDLK_WORLD_62));
	m.insert(std::pair<const char*,SDLKey>("world_63", SDLK_WORLD_63));
	m.insert(std::pair<const char*,SDLKey>("world_64", SDLK_WORLD_64));
	m.insert(std::pair<const char*,SDLKey>("world_65", SDLK_WORLD_65));
	m.insert(std::pair<const char*,SDLKey>("world_66", SDLK_WORLD_66));
	m.insert(std::pair<const char*,SDLKey>("world_67", SDLK_WORLD_67));
	m.insert(std::pair<const char*,SDLKey>("world_68", SDLK_WORLD_68));
	m.insert(std::pair<const char*,SDLKey>("world_69", SDLK_WORLD_69));
	m.insert(std::pair<const char*,SDLKey>("world_70", SDLK_WORLD_70));
	m.insert(std::pair<const char*,SDLKey>("world_71", SDLK_WORLD_71));
	m.insert(std::pair<const char*,SDLKey>("world_72", SDLK_WORLD_72));
	m.insert(std::pair<const char*,SDLKey>("world_73", SDLK_WORLD_73));
	m.insert(std::pair<const char*,SDLKey>("world_74", SDLK_WORLD_74));
	m.insert(std::pair<const char*,SDLKey>("world_75", SDLK_WORLD_75));
	m.insert(std::pair<const char*,SDLKey>("world_76", SDLK_WORLD_76));
	m.insert(std::pair<const char*,SDLKey>("world_77", SDLK_WORLD_77));
	m.insert(std::pair<const char*,SDLKey>("world_78", SDLK_WORLD_78));
	m.insert(std::pair<const char*,SDLKey>("world_79", SDLK_WORLD_79));
	m.insert(std::pair<const char*,SDLKey>("world_80", SDLK_WORLD_80));
	m.insert(std::pair<const char*,SDLKey>("world_81", SDLK_WORLD_81));
	m.insert(std::pair<const char*,SDLKey>("world_82", SDLK_WORLD_82));
	m.insert(std::pair<const char*,SDLKey>("world_83", SDLK_WORLD_83));
	m.insert(std::pair<const char*,SDLKey>("world_84", SDLK_WORLD_84));
	m.insert(std::pair<const char*,SDLKey>("world_85", SDLK_WORLD_85));
	m.insert(std::pair<const char*,SDLKey>("world_86", SDLK_WORLD_86));
	m.insert(std::pair<const char*,SDLKey>("world_87", SDLK_WORLD_87));
	m.insert(std::pair<const char*,SDLKey>("world_88", SDLK_WORLD_88));
	m.insert(std::pair<const char*,SDLKey>("world_89", SDLK_WORLD_89));
	m.insert(std::pair<const char*,SDLKey>("world_90", SDLK_WORLD_90));
	m.insert(std::pair<const char*,SDLKey>("world_91", SDLK_WORLD_91));
	m.insert(std::pair<const char*,SDLKey>("world_92", SDLK_WORLD_92));
	m.insert(std::pair<const char*,SDLKey>("world_93", SDLK_WORLD_93));
	m.insert(std::pair<const char*,SDLKey>("world_94", SDLK_WORLD_94));
	m.insert(std::pair<const char*,SDLKey>("world_95", SDLK_WORLD_95));
	m.insert(std::pair<const char*,SDLKey>("kp_0", SDLK_KP0));
	m.insert(std::pair<const char*,SDLKey>("kp_1", SDLK_KP1));
	m.insert(std::pair<const char*,SDLKey>("kp_2", SDLK_KP2));
	m.insert(std::pair<const char*,SDLKey>("kp_3", SDLK_KP3));
	m.insert(std::pair<const char*,SDLKey>("kp_4", SDLK_KP4));
	m.insert(std::pair<const char*,SDLKey>("kp_5", SDLK_KP5));
	m.insert(std::pair<const char*,SDLKey>("kp_6", SDLK_KP6));
	m.insert(std::pair<const char*,SDLKey>("kp_7", SDLK_KP7));
	m.insert(std::pair<const char*,SDLKey>("kp_8", SDLK_KP8));
	m.insert(std::pair<const char*,SDLKey>("kp_9", SDLK_KP9));
	m.insert(std::pair<const char*,SDLKey>("kp_period", SDLK_KP_PERIOD));
	m.insert(std::pair<const char*,SDLKey>("kp_divide", SDLK_KP_DIVIDE));
	m.insert(std::pair<const char*,SDLKey>("kp_multiply", SDLK_KP_MULTIPLY));
	m.insert(std::pair<const char*,SDLKey>("kp_-", SDLK_KP_MINUS));
	m.insert(std::pair<const char*,SDLKey>("kp_+", SDLK_KP_PLUS));
	m.insert(std::pair<const char*,SDLKey>("kp_enter", SDLK_KP_ENTER));
	m.insert(std::pair<const char*,SDLKey>("kp_=", SDLK_KP_EQUALS));
	m.insert(std::pair<const char*,SDLKey>("up", SDLK_UP));
	m.insert(std::pair<const char*,SDLKey>("down", SDLK_DOWN));
	m.insert(std::pair<const char*,SDLKey>("right", SDLK_RIGHT));
	m.insert(std::pair<const char*,SDLKey>("left", SDLK_LEFT));
	m.insert(std::pair<const char*,SDLKey>("insert", SDLK_INSERT));
	m.insert(std::pair<const char*,SDLKey>("home", SDLK_HOME));
	m.insert(std::pair<const char*,SDLKey>("end", SDLK_END));
	m.insert(std::pair<const char*,SDLKey>("pageup", SDLK_PAGEUP));
	m.insert(std::pair<const char*,SDLKey>("pagedown", SDLK_PAGEDOWN));
	m.insert(std::pair<const char*,SDLKey>("f1", SDLK_F1));
	m.insert(std::pair<const char*,SDLKey>("f2", SDLK_F2));
	m.insert(std::pair<const char*,SDLKey>("f3", SDLK_F3));
	m.insert(std::pair<const char*,SDLKey>("f4", SDLK_F4));
	m.insert(std::pair<const char*,SDLKey>("f5", SDLK_F5));
	m.insert(std::pair<const char*,SDLKey>("f6", SDLK_F6));
	m.insert(std::pair<const char*,SDLKey>("f7", SDLK_F7));
	m.insert(std::pair<const char*,SDLKey>("f8", SDLK_F8));
	m.insert(std::pair<const char*,SDLKey>("f9", SDLK_F9));
	m.insert(std::pair<const char*,SDLKey>("f10", SDLK_F10));
	m.insert(std::pair<const char*,SDLKey>("f11", SDLK_F11));
	m.insert(std::pair<const char*,SDLKey>("f12", SDLK_F12));
	m.insert(std::pair<const char*,SDLKey>("f13", SDLK_F13));
	m.insert(std::pair<const char*,SDLKey>("f14", SDLK_F14));
	m.insert(std::pair<const char*,SDLKey>("f15", SDLK_F15));
	m.insert(std::pair<const char*,SDLKey>("numlock", SDLK_NUMLOCK));
	m.insert(std::pair<const char*,SDLKey>("capslock", SDLK_CAPSLOCK));
	m.insert(std::pair<const char*,SDLKey>("scrollock", SDLK_SCROLLOCK));
	m.insert(std::pair<const char*,SDLKey>("rshift", SDLK_RSHIFT));
	m.insert(std::pair<const char*,SDLKey>("lshift", SDLK_LSHIFT));
	m.insert(std::pair<const char*,SDLKey>("rctrl", SDLK_RCTRL));
	m.insert(std::pair<const char*,SDLKey>("lctrl", SDLK_LCTRL));
	m.insert(std::pair<const char*,SDLKey>("ralt", SDLK_RALT));
	m.insert(std::pair<const char*,SDLKey>("lalt", SDLK_LALT));
	m.insert(std::pair<const char*,SDLKey>("rmeta", SDLK_RMETA));
	m.insert(std::pair<const char*,SDLKey>("lmeta", SDLK_LMETA));
	m.insert(std::pair<const char*,SDLKey>("lsuper", SDLK_LSUPER));
	m.insert(std::pair<const char*,SDLKey>("rsuper", SDLK_RSUPER));
	m.insert(std::pair<const char*,SDLKey>("mode", SDLK_MODE));
	m.insert(std::pair<const char*,SDLKey>("compose", SDLK_COMPOSE));
	m.insert(std::pair<const char*,SDLKey>("help", SDLK_HELP));
	m.insert(std::pair<const char*,SDLKey>("printscreen", SDLK_PRINT));
	m.insert(std::pair<const char*,SDLKey>("sysrq", SDLK_SYSREQ));
	m.insert(std::pair<const char*,SDLKey>("break", SDLK_BREAK));
	m.insert(std::pair<const char*,SDLKey>("menu", SDLK_MENU));
	m.insert(std::pair<const char*,SDLKey>("power", SDLK_POWER));
	m.insert(std::pair<const char*,SDLKey>("euro", SDLK_EURO));
	m.insert(std::pair<const char*,SDLKey>("undo", SDLK_UNDO));
}

void printStrSdlkeys() {
	if (m.empty())
		init();
	
	for (map_t::iterator it = m.begin(); it != m.end(); ++it) {
		printf("%s\n", it->first);
	}
}

const SDLKey* strToSdlkey(const char *const str) {
	if (m.empty())
		init();
	
	map_t::iterator it = m.find(str);
	
	return it == m.end() ? 0 : &it->second;
}
