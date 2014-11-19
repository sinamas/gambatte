//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#include "str_to_sdlkey.h"
#include <cstring>
#include <map>
#include <utility>

namespace {

struct StrLess {
	bool operator()(char const *l, char const *r) const {
		return std::strcmp(l, r) < 0;
	}
};

typedef std::map<char const *, SDLKey, StrLess> map_t;

static void initStrKeyMap(map_t &m) {
	m.insert(std::make_pair("backspace", SDLK_BACKSPACE));
	m.insert(std::make_pair("tab", SDLK_TAB));
	m.insert(std::make_pair("clear", SDLK_CLEAR));
	m.insert(std::make_pair("return", SDLK_RETURN));
	m.insert(std::make_pair("pause", SDLK_PAUSE));
	m.insert(std::make_pair("escape", SDLK_ESCAPE));
	m.insert(std::make_pair("space", SDLK_SPACE));
	m.insert(std::make_pair("!", SDLK_EXCLAIM));
	m.insert(std::make_pair("quotedbl", SDLK_QUOTEDBL));
	m.insert(std::make_pair("hash", SDLK_HASH));
	m.insert(std::make_pair("$", SDLK_DOLLAR));
	m.insert(std::make_pair("ampersand", SDLK_AMPERSAND));
	m.insert(std::make_pair("quote", SDLK_QUOTE));
	m.insert(std::make_pair("leftparen", SDLK_LEFTPAREN));
	m.insert(std::make_pair("rightparen", SDLK_RIGHTPAREN));
	m.insert(std::make_pair("asterisk", SDLK_ASTERISK));
	m.insert(std::make_pair("+", SDLK_PLUS));
	m.insert(std::make_pair(",", SDLK_COMMA));
	m.insert(std::make_pair("-", SDLK_MINUS));
	m.insert(std::make_pair("period", SDLK_PERIOD));
	m.insert(std::make_pair("slash", SDLK_SLASH));
	m.insert(std::make_pair("0", SDLK_0));
	m.insert(std::make_pair("1", SDLK_1));
	m.insert(std::make_pair("2", SDLK_2));
	m.insert(std::make_pair("3", SDLK_3));
	m.insert(std::make_pair("4", SDLK_4));
	m.insert(std::make_pair("5", SDLK_5));
	m.insert(std::make_pair("6", SDLK_6));
	m.insert(std::make_pair("7", SDLK_7));
	m.insert(std::make_pair("8", SDLK_8));
	m.insert(std::make_pair("9", SDLK_9));
	m.insert(std::make_pair(":", SDLK_COLON));
	m.insert(std::make_pair("semicolon", SDLK_SEMICOLON));
	m.insert(std::make_pair("less", SDLK_LESS));
	m.insert(std::make_pair("=", SDLK_EQUALS));
	m.insert(std::make_pair("greater", SDLK_GREATER));
	m.insert(std::make_pair("question", SDLK_QUESTION));
	m.insert(std::make_pair("@", SDLK_AT));
	m.insert(std::make_pair("[", SDLK_LEFTBRACKET));
	m.insert(std::make_pair("backslash", SDLK_BACKSLASH));
	m.insert(std::make_pair("]", SDLK_RIGHTBRACKET));
	m.insert(std::make_pair("^", SDLK_CARET));
	m.insert(std::make_pair("_", SDLK_UNDERSCORE));
	m.insert(std::make_pair("backquote", SDLK_BACKQUOTE));
	m.insert(std::make_pair("a", SDLK_a));
	m.insert(std::make_pair("b", SDLK_b));
	m.insert(std::make_pair("c", SDLK_c));
	m.insert(std::make_pair("d", SDLK_d));
	m.insert(std::make_pair("e", SDLK_e));
	m.insert(std::make_pair("f", SDLK_f));
	m.insert(std::make_pair("g", SDLK_g));
	m.insert(std::make_pair("h", SDLK_h));
	m.insert(std::make_pair("i", SDLK_i));
	m.insert(std::make_pair("j", SDLK_j));
	m.insert(std::make_pair("k", SDLK_k));
	m.insert(std::make_pair("l", SDLK_l));
	m.insert(std::make_pair("m", SDLK_m));
	m.insert(std::make_pair("n", SDLK_n));
	m.insert(std::make_pair("o", SDLK_o));
	m.insert(std::make_pair("p", SDLK_p));
	m.insert(std::make_pair("q", SDLK_q));
	m.insert(std::make_pair("r", SDLK_r));
	m.insert(std::make_pair("s", SDLK_s));
	m.insert(std::make_pair("t", SDLK_t));
	m.insert(std::make_pair("u", SDLK_u));
	m.insert(std::make_pair("v", SDLK_v));
	m.insert(std::make_pair("w", SDLK_w));
	m.insert(std::make_pair("x", SDLK_x));
	m.insert(std::make_pair("y", SDLK_y));
	m.insert(std::make_pair("z", SDLK_z));
	m.insert(std::make_pair("delete", SDLK_DELETE));
	m.insert(std::make_pair("world_0", SDLK_WORLD_0));
	m.insert(std::make_pair("world_1", SDLK_WORLD_1));
	m.insert(std::make_pair("world_2", SDLK_WORLD_2));
	m.insert(std::make_pair("world_3", SDLK_WORLD_3));
	m.insert(std::make_pair("world_4", SDLK_WORLD_4));
	m.insert(std::make_pair("world_5", SDLK_WORLD_5));
	m.insert(std::make_pair("world_6", SDLK_WORLD_6));
	m.insert(std::make_pair("world_7", SDLK_WORLD_7));
	m.insert(std::make_pair("world_8", SDLK_WORLD_8));
	m.insert(std::make_pair("world_9", SDLK_WORLD_9));
	m.insert(std::make_pair("world_10", SDLK_WORLD_10));
	m.insert(std::make_pair("world_11", SDLK_WORLD_11));
	m.insert(std::make_pair("world_12", SDLK_WORLD_12));
	m.insert(std::make_pair("world_13", SDLK_WORLD_13));
	m.insert(std::make_pair("world_14", SDLK_WORLD_14));
	m.insert(std::make_pair("world_15", SDLK_WORLD_15));
	m.insert(std::make_pair("world_16", SDLK_WORLD_16));
	m.insert(std::make_pair("world_17", SDLK_WORLD_17));
	m.insert(std::make_pair("world_18", SDLK_WORLD_18));
	m.insert(std::make_pair("world_19", SDLK_WORLD_19));
	m.insert(std::make_pair("world_20", SDLK_WORLD_20));
	m.insert(std::make_pair("world_21", SDLK_WORLD_21));
	m.insert(std::make_pair("world_22", SDLK_WORLD_22));
	m.insert(std::make_pair("world_23", SDLK_WORLD_23));
	m.insert(std::make_pair("world_24", SDLK_WORLD_24));
	m.insert(std::make_pair("world_25", SDLK_WORLD_25));
	m.insert(std::make_pair("world_26", SDLK_WORLD_26));
	m.insert(std::make_pair("world_27", SDLK_WORLD_27));
	m.insert(std::make_pair("world_28", SDLK_WORLD_28));
	m.insert(std::make_pair("world_29", SDLK_WORLD_29));
	m.insert(std::make_pair("world_30", SDLK_WORLD_30));
	m.insert(std::make_pair("world_31", SDLK_WORLD_31));
	m.insert(std::make_pair("world_32", SDLK_WORLD_32));
	m.insert(std::make_pair("world_33", SDLK_WORLD_33));
	m.insert(std::make_pair("world_34", SDLK_WORLD_34));
	m.insert(std::make_pair("world_35", SDLK_WORLD_35));
	m.insert(std::make_pair("world_36", SDLK_WORLD_36));
	m.insert(std::make_pair("world_37", SDLK_WORLD_37));
	m.insert(std::make_pair("world_38", SDLK_WORLD_38));
	m.insert(std::make_pair("world_39", SDLK_WORLD_39));
	m.insert(std::make_pair("world_40", SDLK_WORLD_40));
	m.insert(std::make_pair("world_41", SDLK_WORLD_41));
	m.insert(std::make_pair("world_42", SDLK_WORLD_42));
	m.insert(std::make_pair("world_43", SDLK_WORLD_43));
	m.insert(std::make_pair("world_44", SDLK_WORLD_44));
	m.insert(std::make_pair("world_45", SDLK_WORLD_45));
	m.insert(std::make_pair("world_46", SDLK_WORLD_46));
	m.insert(std::make_pair("world_47", SDLK_WORLD_47));
	m.insert(std::make_pair("world_48", SDLK_WORLD_48));
	m.insert(std::make_pair("world_49", SDLK_WORLD_49));
	m.insert(std::make_pair("world_50", SDLK_WORLD_50));
	m.insert(std::make_pair("world_51", SDLK_WORLD_51));
	m.insert(std::make_pair("world_52", SDLK_WORLD_52));
	m.insert(std::make_pair("world_53", SDLK_WORLD_53));
	m.insert(std::make_pair("world_54", SDLK_WORLD_54));
	m.insert(std::make_pair("world_55", SDLK_WORLD_55));
	m.insert(std::make_pair("world_56", SDLK_WORLD_56));
	m.insert(std::make_pair("world_57", SDLK_WORLD_57));
	m.insert(std::make_pair("world_58", SDLK_WORLD_58));
	m.insert(std::make_pair("world_59", SDLK_WORLD_59));
	m.insert(std::make_pair("world_60", SDLK_WORLD_60));
	m.insert(std::make_pair("world_61", SDLK_WORLD_61));
	m.insert(std::make_pair("world_62", SDLK_WORLD_62));
	m.insert(std::make_pair("world_63", SDLK_WORLD_63));
	m.insert(std::make_pair("world_64", SDLK_WORLD_64));
	m.insert(std::make_pair("world_65", SDLK_WORLD_65));
	m.insert(std::make_pair("world_66", SDLK_WORLD_66));
	m.insert(std::make_pair("world_67", SDLK_WORLD_67));
	m.insert(std::make_pair("world_68", SDLK_WORLD_68));
	m.insert(std::make_pair("world_69", SDLK_WORLD_69));
	m.insert(std::make_pair("world_70", SDLK_WORLD_70));
	m.insert(std::make_pair("world_71", SDLK_WORLD_71));
	m.insert(std::make_pair("world_72", SDLK_WORLD_72));
	m.insert(std::make_pair("world_73", SDLK_WORLD_73));
	m.insert(std::make_pair("world_74", SDLK_WORLD_74));
	m.insert(std::make_pair("world_75", SDLK_WORLD_75));
	m.insert(std::make_pair("world_76", SDLK_WORLD_76));
	m.insert(std::make_pair("world_77", SDLK_WORLD_77));
	m.insert(std::make_pair("world_78", SDLK_WORLD_78));
	m.insert(std::make_pair("world_79", SDLK_WORLD_79));
	m.insert(std::make_pair("world_80", SDLK_WORLD_80));
	m.insert(std::make_pair("world_81", SDLK_WORLD_81));
	m.insert(std::make_pair("world_82", SDLK_WORLD_82));
	m.insert(std::make_pair("world_83", SDLK_WORLD_83));
	m.insert(std::make_pair("world_84", SDLK_WORLD_84));
	m.insert(std::make_pair("world_85", SDLK_WORLD_85));
	m.insert(std::make_pair("world_86", SDLK_WORLD_86));
	m.insert(std::make_pair("world_87", SDLK_WORLD_87));
	m.insert(std::make_pair("world_88", SDLK_WORLD_88));
	m.insert(std::make_pair("world_89", SDLK_WORLD_89));
	m.insert(std::make_pair("world_90", SDLK_WORLD_90));
	m.insert(std::make_pair("world_91", SDLK_WORLD_91));
	m.insert(std::make_pair("world_92", SDLK_WORLD_92));
	m.insert(std::make_pair("world_93", SDLK_WORLD_93));
	m.insert(std::make_pair("world_94", SDLK_WORLD_94));
	m.insert(std::make_pair("world_95", SDLK_WORLD_95));
	m.insert(std::make_pair("kp_0", SDLK_KP0));
	m.insert(std::make_pair("kp_1", SDLK_KP1));
	m.insert(std::make_pair("kp_2", SDLK_KP2));
	m.insert(std::make_pair("kp_3", SDLK_KP3));
	m.insert(std::make_pair("kp_4", SDLK_KP4));
	m.insert(std::make_pair("kp_5", SDLK_KP5));
	m.insert(std::make_pair("kp_6", SDLK_KP6));
	m.insert(std::make_pair("kp_7", SDLK_KP7));
	m.insert(std::make_pair("kp_8", SDLK_KP8));
	m.insert(std::make_pair("kp_9", SDLK_KP9));
	m.insert(std::make_pair("kp_period", SDLK_KP_PERIOD));
	m.insert(std::make_pair("kp_divide", SDLK_KP_DIVIDE));
	m.insert(std::make_pair("kp_multiply", SDLK_KP_MULTIPLY));
	m.insert(std::make_pair("kp_-", SDLK_KP_MINUS));
	m.insert(std::make_pair("kp_+", SDLK_KP_PLUS));
	m.insert(std::make_pair("kp_enter", SDLK_KP_ENTER));
	m.insert(std::make_pair("kp_=", SDLK_KP_EQUALS));
	m.insert(std::make_pair("up", SDLK_UP));
	m.insert(std::make_pair("down", SDLK_DOWN));
	m.insert(std::make_pair("right", SDLK_RIGHT));
	m.insert(std::make_pair("left", SDLK_LEFT));
	m.insert(std::make_pair("insert", SDLK_INSERT));
	m.insert(std::make_pair("home", SDLK_HOME));
	m.insert(std::make_pair("end", SDLK_END));
	m.insert(std::make_pair("pageup", SDLK_PAGEUP));
	m.insert(std::make_pair("pagedown", SDLK_PAGEDOWN));
	m.insert(std::make_pair("f1", SDLK_F1));
	m.insert(std::make_pair("f2", SDLK_F2));
	m.insert(std::make_pair("f3", SDLK_F3));
	m.insert(std::make_pair("f4", SDLK_F4));
	m.insert(std::make_pair("f5", SDLK_F5));
	m.insert(std::make_pair("f6", SDLK_F6));
	m.insert(std::make_pair("f7", SDLK_F7));
	m.insert(std::make_pair("f8", SDLK_F8));
	m.insert(std::make_pair("f9", SDLK_F9));
	m.insert(std::make_pair("f10", SDLK_F10));
	m.insert(std::make_pair("f11", SDLK_F11));
	m.insert(std::make_pair("f12", SDLK_F12));
	m.insert(std::make_pair("f13", SDLK_F13));
	m.insert(std::make_pair("f14", SDLK_F14));
	m.insert(std::make_pair("f15", SDLK_F15));
	m.insert(std::make_pair("numlock", SDLK_NUMLOCK));
	m.insert(std::make_pair("capslock", SDLK_CAPSLOCK));
	m.insert(std::make_pair("scrollock", SDLK_SCROLLOCK));
	m.insert(std::make_pair("rshift", SDLK_RSHIFT));
	m.insert(std::make_pair("lshift", SDLK_LSHIFT));
	m.insert(std::make_pair("rctrl", SDLK_RCTRL));
	m.insert(std::make_pair("lctrl", SDLK_LCTRL));
	m.insert(std::make_pair("ralt", SDLK_RALT));
	m.insert(std::make_pair("lalt", SDLK_LALT));
	m.insert(std::make_pair("rmeta", SDLK_RMETA));
	m.insert(std::make_pair("lmeta", SDLK_LMETA));
	m.insert(std::make_pair("lsuper", SDLK_LSUPER));
	m.insert(std::make_pair("rsuper", SDLK_RSUPER));
	m.insert(std::make_pair("mode", SDLK_MODE));
	m.insert(std::make_pair("compose", SDLK_COMPOSE));
	m.insert(std::make_pair("help", SDLK_HELP));
	m.insert(std::make_pair("printscreen", SDLK_PRINT));
	m.insert(std::make_pair("sysrq", SDLK_SYSREQ));
	m.insert(std::make_pair("break", SDLK_BREAK));
	m.insert(std::make_pair("menu", SDLK_MENU));
	m.insert(std::make_pair("power", SDLK_POWER));
	m.insert(std::make_pair("euro", SDLK_EURO));
	m.insert(std::make_pair("undo", SDLK_UNDO));
}

static map_t makeStrKeyMap() {
	map_t m;
	initStrKeyMap(m);
	return m;
}

static map_t const m = makeStrKeyMap();

} // anon ns

void printStrSdlkeys() {
	for (map_t::const_iterator it = m.begin(); it != m.end(); ++it)
		printf("%s\n", it->first);
}

SDLKey const * strToSdlkey(char const *str) {
	map_t::const_iterator it = m.find(str);
	return it == m.end() ? 0 : &it->second;
}
