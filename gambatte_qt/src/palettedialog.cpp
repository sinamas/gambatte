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

#include "palettedialog.h"
#include "dialoghelpers.h"
#include <QApplication>
#include <QByteArray>
#include <QColorDialog>
#include <QDataStream>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QListView>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStringListModel>
#include <QVBoxLayout>
#include <algorithm>
#include <cstring>
#include <functional>

namespace {

#define TO5BIT(c8) (((c8) * 0x1F * 2 + 0xFF) / (0xFF*2))
#define PACK15_1(rgb24) (TO5BIT((rgb24) >> 16 & 0xFF) << 10 | TO5BIT((rgb24) >> 8 & 0xFF) << 5 | TO5BIT((rgb24) & 0xFF))
#define PACK15_4(c0, c1, c2, c3) \
	PACK15_1(c0), PACK15_1(c1), PACK15_1(c2), PACK15_1(c3)

static unsigned short const p005[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000)
};

static unsigned short const p006[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000)
};

static unsigned short const p007[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000)
};

static unsigned short const p008[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000)
};

static unsigned short const p012[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static unsigned short const p013[] = {
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF)
};

static unsigned short const p016[] = {
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000)
};

static unsigned short const p017[] = {
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000)
};

/*static unsigned short const p01B[] = {
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000)
};*/

static unsigned short const p100[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000)
};

static unsigned short const p10B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p10D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000)
};

static unsigned short const p110[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p11C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000)
};

static unsigned short const p20B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p20C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static unsigned short const p300[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000)
};

static unsigned short const p304[] = {
	PACK15_4(0xFFFFFF, 0x7BFF00, 0xB57300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p305[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p306[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p308[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000)
};

static unsigned short const p30A[] = {
	PACK15_4(0xB5B5FF, 0xFFFF94, 0xAD5A42, 0x000000),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A)
};

static unsigned short const p30C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static unsigned short const p30D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p30E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p30F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p312[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static unsigned short const p319[] = {
	PACK15_4(0xFFE6C5, 0xCE9C84, 0x846B29, 0x5A3108),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static unsigned short const p31C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p405[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static unsigned short const p406[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF )
};

static unsigned short const p407[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static unsigned short const p500[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static unsigned short const p501[] = {
	PACK15_4(0xFFFF9C, 0x94B5FF, 0x639473, 0x003A3A),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p502[] = {
	PACK15_4(0x6BFF00, 0xFFFFFF, 0xFF524A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static unsigned short const p503[] = {
	PACK15_4(0x52DE00, 0xFF8400, 0xFFFF00, 0xFFFFFF),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static unsigned short const p508[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0x0000FF, 0xFFFFFF, 0xFFFF7B, 0x0084FF)
};

static unsigned short const p509[] = {
	PACK15_4(0xFFFFCE, 0x63EFEF, 0x9C8431, 0x5A5A5A),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p50B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF7B, 0x0084FF, 0xFF0000)
};

static unsigned short const p50C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static unsigned short const p50D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static unsigned short const p50E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p50F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static unsigned short const p510[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p511[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x00FF00, 0x318400, 0x004A00),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p512[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p514[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFF00, 0xFF0000, 0x630000, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static unsigned short const p515[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static unsigned short const p518[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static unsigned short const p51A[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0x7B4A00, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static unsigned short const p51C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

#undef PACK15_4
#undef PACK15_1
#undef TO5BIT

struct GbcPaletteEntry { char const *title; unsigned short const *p; };

static GbcPaletteEntry const gbcDirPalettes[] = {
	{ "GBC - Blue", p518 },
	{ "GBC - Brown", p012 },
	{ "GBC - Dark Blue", p50D },
	{ "GBC - Dark Brown", p319 },
	{ "GBC - Dark Green", p31C },
	{ "GBC - Grayscale", p016 },
	{ "GBC - Green", p005 },
	{ "GBC - Inverted", p013 },
	{ "GBC - Orange", p007 },
	{ "GBC - Pastel Mix", p017 },
	{ "GBC - Red", p510 },
	{ "GBC - Yellow", p51A },
};

static GbcPaletteEntry const gbcTitlePalettes[] = {
	{ "ALLEY WAY", p008 },
	{ "ASTEROIDS/MISCMD", p30E },
	{ "BA.TOSHINDEN", p50F },
	{ "BALLOON KID", p006 },
	{ "BASEBALL", p503 },
	{ "BOY AND BLOB GB1", p512 },
	{ "BOY AND BLOB GB2", p512 },
	{ "BT2RAGNAROKWORLD", p312 },
	{ "DEFENDER/JOUST", p50F },
	{ "DMG FOOTBALL", p30E },
	{ "DONKEY KONG", p306 },
	{ "DONKEYKONGLAND", p50C },
	{ "DONKEYKONGLAND 2", p50C },
	{ "DONKEYKONGLAND 3", p50C },
	{ "DONKEYKONGLAND95", p501 },
	{ "DR.MARIO", p20B },
	{ "DYNABLASTER", p30F },
	{ "F1RACE", p012 },
	{ "G&W GALLERY", p304 },
	{ "GALAGA&GALAXIAN", p013 },
	{ "GAME&WATCH", p012 },
	{ "GAMEBOY GALLERY", p304 },
	{ "GAMEBOY GALLERY2", p304 },
	{ "GBWARS", p500 },
	{ "GOLF", p30E },
	{ "Game and Watch 2", p304 },
	{ "HOSHINOKA-BI", p508 },
	{ "JAMES  BOND  007", p11C },
	{ "KAERUNOTAMENI", p10D },
	{ "KEN GRIFFEY JR", p31C },
	{ "KID ICARUS", p30D },
	{ "KILLERINSTINCT95", p50D },
	{ "KINGOFTHEZOO", p30F },
	{ "KIRAKIRA KIDS", p012 },
	{ "KIRBY BLOCKBALL", p508 },
	{ "KIRBY DREAM LAND", p508 },
	{ "KIRBY'S PINBALL", p308 },
	{ "KIRBY2", p508 },
	{ "LOLO2", p50F },
	{ "MAGNETIC SOCCER", p50E },
	{ "MANSELL", p012 },
	{ "MARIO & YOSHI", p305 },
	{ "MARIO'S PICROSS", p012 },
	{ "MARIOLAND2", p509 },
	{ "MEGA MAN 2", p50F },
	{ "MEGAMAN", p50F },
	{ "MEGAMAN3", p50F },
	{ "METROID2", p514 },
	{ "MILLI/CENTI/PEDE", p31C },
	{ "MOGURANYA", p300 },
	{ "MYSTIC QUEST", p50E },
	{ "NETTOU KOF 95", p50F },
	{ "NEW CHESSMASTER", p30F },
	{ "OTHELLO", p50E },
	{ "PAC-IN-TIME", p51C },
	{ "PICROSS 2", p012 },
	{ "PINOCCHIO", p20C },
	{ "POKEBOM", p30C },
	{ "POKEMON BLUE", p10B },
	{ "POKEMON GREEN", p11C },
	{ "POKEMON RED", p110 },
	{ "POKEMON YELLOW", p007 },
	{ "QIX", p407 },
	{ "RADARMISSION", p100 },
	{ "ROCKMAN WORLD", p50F },
	{ "ROCKMAN WORLD2", p50F },
	{ "ROCKMANWORLD3", p50F },
	{ "SEIKEN DENSETSU", p50E },
	{ "SOCCER", p502 },
	{ "SOLARSTRIKER", p013 },
	{ "SPACE INVADERS", p013 },
	{ "STAR STACKER", p012 },
	{ "STAR WARS", p512 },
	{ "STAR WARS-NOA", p512 },
	{ "STREET FIGHTER 2", p50F },
	{ "SUPER MARIOLAND", p30A },
	{ "SUPER RC PRO-AM", p50F },
	{ "SUPERDONKEYKONG", p501 },
	{ "SUPERMARIOLAND3", p500 },
	{ "TENNIS", p502 },
	{ "TETRIS", p007 },
	{ "TETRIS ATTACK", p405 },
	{ "TETRIS BLAST", p006 },
	{ "TETRIS FLASH", p407 },
	{ "TETRIS PLUS", p31C },
	{ "TETRIS2", p407 },
	{ "THE CHESSMASTER", p30F },
	{ "TOPRANKINGTENNIS", p502 },
	{ "TOPRANKTENNIS", p502 },
	{ "TOY STORY", p30E },
	{ "TRIP WORLD", p500 }, // unofficial
	{ "VEGAS STAKES", p50E },
	{ "WARIO BLAST", p31C },
	{ "WARIOLAND2", p515 },
	{ "WAVERACE", p50B },
	{ "WORLD CUP", p30E },
	{ "X", p016 },
	{ "YAKUMAN", p012 },
	{ "YOSHI'S COOKIE", p406 },
	{ "YOSSY NO COOKIE", p406 },
	{ "YOSSY NO PANEPON", p405 },
	{ "YOSSY NO TAMAGO", p305 },
	{ "ZELDA", p511 },
};

static inline std::size_t gbcDirPalettesSize() { return sizeof gbcDirPalettes / sizeof gbcDirPalettes[0]; }
static inline GbcPaletteEntry const * gbcDirPalettesEnd() { return gbcDirPalettes + gbcDirPalettesSize(); }
static inline std::size_t gbcTitlePalettesSize() { return sizeof gbcTitlePalettes / sizeof gbcTitlePalettes[0]; }
static inline GbcPaletteEntry const * gbcTitlePalettesEnd() { return gbcTitlePalettes + gbcTitlePalettesSize(); }

struct GbcPaletteEntryLess {
	bool operator()(GbcPaletteEntry const &lhs, char const *rhstitle) const {
		return std::strcmp(lhs.title, rhstitle) < 0;
	}
};

static unsigned short const * findGbcDirPal(char const *title) {
	GbcPaletteEntry const *r = std::lower_bound(gbcDirPalettes, gbcDirPalettesEnd(),
	                                            title, GbcPaletteEntryLess());
	return r < gbcDirPalettesEnd() && !std::strcmp(r->title, title)
	     ? r->p
	     : 0;
}

static unsigned short const * findGbcTitlePal(char const *title) {
	GbcPaletteEntry const *r = std::lower_bound(gbcTitlePalettes, gbcTitlePalettesEnd(),
	                                            title, GbcPaletteEntryLess());
	return r < gbcTitlePalettesEnd() && !std::strcmp(r->title, title)
	     ? r->p
	     : 0;
}

static unsigned short const * findGbcPal(char const *title) {
	if (unsigned short const *pal = findGbcDirPal(title))
		return pal;

	return findGbcTitlePal(title);
}

static unsigned long gbcToRgb32(unsigned const rgb15) {
	unsigned long r = rgb15 >> 10 & 0x1F;
	unsigned long g = rgb15 >>  5 & 0x1F;
	unsigned long b = rgb15       & 0x1F;

	return ((r * 13 + g * 2 + b) >> 1) << 16
	     | (g * 3 + b) << 9
	     | (r * 3 + g * 2 + b * 11) >> 1;
}

} // anon ns

ColorPicker::ColorPicker(QRgb color, QWidget *parent)
: QFrame(parent)
, w_(new QWidget(this))
{
	setAcceptDrops(true);
	w_->setAutoFillBackground(true);

	setLayout(new QVBoxLayout);
	layout()->setMargin(0);
	layout()->addWidget(w_);

	setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	setFocusPolicy(Qt::StrongFocus);
	setColor(color);
}

QColor const & ColorPicker::getQColor() const {
	return w_->palette().color(QPalette::Background);
}

void ColorPicker::requestColor() {
	QColor c = QColorDialog::getColor(QColor(color()), this);
	if (c.isValid()) {
		setColor(c);
		emit colorChanged();
	}
}

void ColorPicker::setColor(QColor const &color) {
	QPalette p(w_->palette());
	p.setColor(QPalette::Background, color);
	w_->setPalette(p);
}

void ColorPicker::dragEnterEvent(QDragEnterEvent *e) {
	if (e->mimeData()->hasColor() && e->source() != this)
		e->acceptProposedAction();
}

void ColorPicker::dropEvent(QDropEvent *e) {
	e->setDropAction(Qt::CopyAction);
	e->accept();
	setColor(qvariant_cast<QColor>(e->mimeData()->colorData()));
	emit colorChanged();
}

void ColorPicker::mousePressEvent(QMouseEvent *e) {
	dragStartPosition_ = e->pos();
}

void ColorPicker::mouseMoveEvent(QMouseEvent *e) {
	if ((e->pos() - dragStartPosition_).manhattanLength() < QApplication::startDragDistance())
		return;

	QDrag *const drag = new QDrag(this);
	QMimeData *const mimeData = new QMimeData;
	drag->setMimeData(mimeData);
	mimeData->setColorData(getQColor());
	drag->exec(Qt::CopyAction);
}

void ColorPicker::mouseReleaseEvent(QMouseEvent *e) {
	e->ignore();

	if (e->x() <= width() && e->y() <= height()) {
		e->accept();
		requestColor();
	}
}

void ColorPicker::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();

	if (e->key() == Qt::Key_Space) {
		e->accept();
		requestColor();
	}
}

QRgb ColorPicker::color() const {
	return getQColor().rgb() & 0xFFFFFF;
}

void ColorPicker::setColor(QRgb rgb32) {
	setColor(QColor(rgb32));
}

ColorQuad::ColorQuad(QString const &label, QWidget *parent)
: QGroupBox(label, parent)
{
	setAcceptDrops(true);

	QHBoxLayout *const l = new QHBoxLayout(this);
	for (std::size_t i = 0; i < sizeof picker_ / sizeof *picker_; ++i) {
		picker_[i] = addWidget(l, new ColorPicker);
		connect(picker_[i], SIGNAL(colorChanged()), this, SLOT(pickerChanged()));
	}
}

void ColorQuad::pickerChanged() {
	emit colorChanged();
}

void ColorQuad::dragEnterEvent(QDragEnterEvent *e) {
	if (e->mimeData()->hasFormat("application/x-colorquad") && e->source() != this)
		e->acceptProposedAction();
}

void ColorQuad::dropEvent(QDropEvent *e) {
	e->setDropAction(Qt::CopyAction);
	e->accept();

	QDataStream dataStream(e->mimeData()->data("application/x-colorquad"));
	QRgb color;
	for (std::size_t i = 0; i < sizeof picker_ / sizeof *picker_; ++i) {
		dataStream >> color;
		setColor(i, color);
	}

	pickerChanged();
}

void ColorQuad::mousePressEvent(QMouseEvent *) {
	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	for (std::size_t i = 0; i < sizeof picker_ / sizeof *picker_; ++i)
		dataStream << color(i);

	QDrag *const drag = new QDrag(this);
	QMimeData *const mimeData = new QMimeData;
	drag->setMimeData(mimeData);
	mimeData->setData("application/x-colorquad", itemData);
	drag->exec(Qt::CopyAction);
}

namespace {

class ImmutableStringListModel : public QStringListModel {
public:
	explicit ImmutableStringListModel(QObject *parent = 0)
	: QStringListModel(parent)
	{
	}

	explicit ImmutableStringListModel(QStringList const &strings, QObject *parent = 0)
	: QStringListModel(strings, parent)
	{
	}

	Qt::ItemFlags flags(QModelIndex const &index) const {
		return QStringListModel::flags(index) & ~Qt::ItemIsEditable;
	}
};

static QStringList const makeStaticStringList(bool const hasGlobal) {
	QStringList sl;
	if (hasGlobal)
		sl.append(QObject::tr("Global Palette"));

	sl.append(QObject::tr("Current Scheme"));
	sl.append(QObject::tr("Default Gray"));
	for (std::size_t i = 0; i < gbcDirPalettesSize(); ++i)
		sl.append(gbcDirPalettes[i].title);
	for (std::size_t i = 0; i < gbcTitlePalettesSize(); ++i)
		sl.append(gbcTitlePalettes[i].title);

	return sl;
}

static void setSchemeList(QStringListModel &model, QString const &savedir, bool hasGlobal) {
	QDir dir(savedir, "*.pal",
	         QDir::Name | QDir::IgnoreCase,
	         QDir::Files | QDir::Readable);
	QStringList dirlisting(dir.entryList());
	std::for_each(dirlisting.begin(), dirlisting.end(), 
	              std::bind2nd(std::mem_fun_ref(&QString::chop), 4));
	model.setStringList(makeStaticStringList(hasGlobal) + dirlisting);
}

static QModelIndex schemeIndexOf(QAbstractItemModel const &model, QString const &schemeStr) {
	int const rows = model.rowCount();

	for (int i = 0; i < rows; ++i) {
		if (model.index(i, 0).data().toString() == schemeStr)
			return model.index(i, 0);
	}

	for (int i = 0; i < rows; ++i) {
		if (model.index(i, 0).data().toString() == QObject::tr("Current Scheme"))
			return model.index(i, 0);
	}

	return QModelIndex();
}

} // anon ns

PaletteDialog::PaletteDialog(QString const &savepath,
                             PaletteDialog const *const global,
                             QWidget *const parent)
: QDialog(parent)
, savedir_(savepath + '/')
, global_(global)
, listView_(new QListView(this))
, rmSchemeButton_(new QPushButton(tr("Remove Scheme"), this))
, quads_()
, currentColors_()
, defaultScheme_(global ? tr("Global Palette") : tr("Default Gray"))
, schemeString_(defaultScheme_)
{
	setWindowTitle(global ? tr("Current ROM Palette") : tr("Global Palette"));
	QDir::root().mkpath(savedir_ + "stored/");
	listView_->setModel(new ImmutableStringListModel(this));
	setSchemeList();

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);

	{
		QHBoxLayout *const topLayout = addLayout(mainLayout, new QHBoxLayout);
		QGroupBox *const lframe = addWidget(topLayout, new QGroupBox(tr("Scheme")));
		QVBoxLayout *const frameLayout = new QVBoxLayout(lframe);
		frameLayout->addWidget(listView_);

		QPushButton *const saveButton =
			addWidget(frameLayout, new QPushButton(tr("Save Scheme...")));
		rmSchemeButton_->setParent(0); // tab order reparent
		frameLayout->addWidget(rmSchemeButton_);
		connect(saveButton, SIGNAL(clicked()), this, SLOT(saveScheme()));
		connect(rmSchemeButton_, SIGNAL(clicked()), this, SLOT(rmScheme()));

		QVBoxLayout *vLayout = addLayout(topLayout, new QVBoxLayout);
		quads_[0] = addWidget(vLayout, new ColorQuad(tr("Background")));
		quads_[1] = addWidget(vLayout, new ColorQuad(tr("Sprite 1")));
		quads_[2] = addWidget(vLayout, new ColorQuad(tr("Sprite 2")));
	}

	{
		QHBoxLayout *hLayout = addLayout(mainLayout, new QHBoxLayout,
		                                 Qt::AlignBottom | Qt::AlignRight);
		QPushButton *okButton     = addWidget(hLayout, new QPushButton(tr("OK")));
		QPushButton *cancelButton = addWidget(hLayout, new QPushButton(tr("Cancel")));
		okButton->setDefault(true);
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	}

	for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i) {
		connect(quads_[i], SIGNAL(colorChanged()),
		        listView_->selectionModel(), SLOT(clear()));
	}

	connect(listView_->selectionModel(),
	        SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)),
	        this, SLOT(schemeChanged(QModelIndex const &)));

	if (global) {
		restore();
		store();
	} else {
		QSettings settings;
		settings.beginGroup("palette");
		loadSettings(settings);
		settings.endGroup();
	}
}

PaletteDialog::~PaletteDialog() {
	if (global_) {
		saveToSettingsFile();
	} else {
		QSettings settings;
		settings.beginGroup("palette");
		saveSettings(settings);
		settings.endGroup();
	}
}

void PaletteDialog::saveSettings(QSettings &settings) {
	settings.setValue("slectedScheme", schemeString_);

	for (std::size_t i = 0; i < sizeof currentColors_ / sizeof *currentColors_; ++i)
	for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
		settings.setValue(quads_[i]->title() + QString::number(j), currentColors_[i][j]);
}

void PaletteDialog::loadSettings(QSettings &settings) {
	schemeString_ = settings.value("slectedScheme", defaultScheme_).toString();

	for (std::size_t i = 0; i < sizeof currentColors_ / sizeof *currentColors_; ++i)
	for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j) {
		currentColors_[i][j] = qvariant_cast<QRgb>(
			settings.value(quads_[i]->title() + QString::number(j),
			               QRgb((3 - (j & 3)) * 85 * 0x010101)));
	}

	restore();
	store();
}

void PaletteDialog::saveToSettingsFile() {
	if (!settingsFile_.isEmpty()) {
		if (schemeString_ == defaultScheme_) {
			QDir(savedir_).remove(settingsFile_);
		} else {
			QSettings settings(savedir_ + settingsFile_, QSettings::IniFormat);
			saveSettings(settings);
		}
	}
}

void PaletteDialog::setSchemeList() {
	::setSchemeList(*static_cast<QStringListModel *>(listView_->model()),
	                savedir_ + "stored/", global_);
}

void PaletteDialog::rmScheme() {
	{ QDir(savedir_ + "stored/").remove(listView_->currentIndex().data().toString() + ".pal"); }
	listView_->selectionModel()->clear();
	setSchemeList();
}

void PaletteDialog::saveScheme() {
	bool ok;
	QString const &text = QInputDialog::getText(this, tr("Save Scheme"), tr("Scheme name:"),
	                                            QLineEdit::Normal, QString(), &ok);
	if (!ok)
		return;

	if (text.isEmpty()
			|| makeStaticStringList(true).contains(text)
			|| text.size() > 200
			|| text.contains(QRegExp("[" + QRegExp::escape("<>:\"/\\|?*") + "]"))) {
		QMessageBox::information(this, tr("Invalid scheme name"), tr("Invalid scheme name."));
		return;
	}

	{
		QSettings settings(savedir_ + "stored/" + text + ".pal", QSettings::IniFormat);

		for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
			settings.setValue(quads_[i]->title() + QString::number(j), quads_[i]->color(j));
	}

	setSchemeList();
	listView_->setCurrentIndex(schemeIndexOf(*listView_->model(), text));
}

void PaletteDialog::schemeChanged(QModelIndex const &current) {
	rmSchemeButton_->setEnabled(false);
	if (!current.isValid())
		return;

	QString const &str = current.data().toString();
	if (tr("Global Palette") == str) {
		for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
			quads_[i]->setColor(j, global_->color(i, j));
	} else if (tr("Current Scheme") == str) {
		for (std::size_t i = 0; i < sizeof currentColors_ / sizeof *currentColors_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
			quads_[i]->setColor(j, currentColors_[i][j]);
	} else if (tr("Default Gray") == str) {
		for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
			quads_[i]->setColor(j, (3 - (j & 3)) * 85 * 0x010101);
	} else if (unsigned short const *gbcpal = findGbcPal(str.toAscii().data())) {
		for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
			quads_[i]->setColor(j, gbcToRgb32(gbcpal[i * 4 + j]));
	} else {
		QSettings settings(savedir_ + "stored/" + str + ".pal", QSettings::IniFormat);

		for (std::size_t i = 0; i < sizeof quads_ / sizeof *quads_; ++i)
		for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j) {
			QVariant v = settings.value(quads_[i]->title() + QString::number(j), 0);
			quads_[i]->setColor(j, qvariant_cast<QRgb>(v));
		}

		rmSchemeButton_->setEnabled(true);
	}
}

void PaletteDialog::store() {
	for (std::size_t i = 0; i < sizeof currentColors_ / sizeof *currentColors_; ++i)
	for (std::size_t j = 0; j < sizeof *currentColors_ / sizeof **currentColors_; ++j)
		currentColors_[i][j] = quads_[i]->color(j);

	if (!listView_->currentIndex().isValid()) {
		// obs: will emit currentChanged()
		listView_->setCurrentIndex(schemeIndexOf(*listView_->model(),
		                                         tr("Current Scheme")));
	}

	schemeString_ = listView_->currentIndex().data().toString();
}

void PaletteDialog::restore() {
	listView_->setCurrentIndex(schemeIndexOf(*listView_->model(), schemeString_));
}

void PaletteDialog::externalChange() {
	setSchemeList();
	restore();
	store();
}

void PaletteDialog::setSettingsFile(QString const &filename, QString const &romTitle) {
	saveToSettingsFile();
	settingsFile_ = filename;
	defaultScheme_ = findGbcTitlePal(romTitle.toAscii().data())
	               ? romTitle
	               : tr("Global Palette");
	QSettings settings(savedir_ + settingsFile_, QSettings::IniFormat);
	loadSettings(settings);
}

void PaletteDialog::accept() {
	store();
	QDialog::accept();
}

void PaletteDialog::reject() {
	restore();
// 	QDialog::reject();
	QDialog::accept();
}
