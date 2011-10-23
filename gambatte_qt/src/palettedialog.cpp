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
#include "palettedialog.h"
#include <QColor>
#include <QPalette>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>
#include <QListView>
#include <QStringListModel>
#include <QDir>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <algorithm>
#include <cstring>

namespace {

#define TO5BIT(c8) (((c8) * 0x1F * 2 + 0xFF) / (0xFF*2))
#define PACK15_1(rgb24) (TO5BIT((rgb24) >> 16 & 0xFF) << 10 | TO5BIT((rgb24) >> 8 & 0xFF) << 5 | TO5BIT((rgb24) & 0xFF))
#define PACK15_4(c0, c1, c2, c3) \
	PACK15_1(c0), PACK15_1(c1), PACK15_1(c2), PACK15_1(c3)

static const unsigned short p005[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000)
};

static const unsigned short p006[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000)
};

static const unsigned short p007[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000)
};

static const unsigned short p008[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000)
};

static const unsigned short p012[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p013[] = {
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF),
	PACK15_4(0x000000, 0x008484, 0xFFDE00, 0xFFFFFF)
};

static const unsigned short p016[] = {
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000),
	PACK15_4(0xFFFFFF, 0xA5A5A5, 0x525252, 0x000000)
};

static const unsigned short p017[] = {
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000),
	PACK15_4(0xFFFFA5, 0xFF9494, 0x9494FF, 0x000000)
};

static const unsigned short p01B[] = {
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFCE00, 0x9C6300, 0x000000)
};

static const unsigned short p100[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000)
};

static const unsigned short p10B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p10D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000)
};

static const unsigned short p110[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p11C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000)
};

static const unsigned short p20B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p20C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static const unsigned short p300[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000)
};

static const unsigned short p304[] = {
	PACK15_4(0xFFFFFF, 0x7BFF00, 0xB57300, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p305[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p306[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p308[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000)
};

static const unsigned short p30A[] = {
	PACK15_4(0xB5B5FF, 0xFFFF94, 0xAD5A42, 0x000000),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A),
	PACK15_4(0x000000, 0xFFFFFF, 0xFF8484, 0x943A3A)
};

static const unsigned short p30C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000)
};

static const unsigned short p30D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p30E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p30F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p312[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p319[] = {
	PACK15_4(0xFFE6C5, 0xCE9C84, 0x846B29, 0x5A3108),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p31C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p405[] = {
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x52FF00, 0xFF4200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p406[] = {
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF9C00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF )
};

static const unsigned short p407[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF00, 0xFF0000, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p500[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p501[] = {
	PACK15_4(0xFFFF9C, 0x94B5FF, 0x639473, 0x003A3A),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p502[] = {
	PACK15_4(0x6BFF00, 0xFFFFFF, 0xFF524A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p503[] = {
	PACK15_4(0x52DE00, 0xFF8400, 0xFFFF00, 0xFFFFFF),
	PACK15_4(0xFFFFFF, 0xFFFFFF, 0x63A5FF, 0x0000FF),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000)
};

static const unsigned short p508[] = {
	PACK15_4(0xA59CFF, 0xFFFF00, 0x006300, 0x000000),
	PACK15_4(0xFF6352, 0xD60000, 0x630000, 0x000000),
	PACK15_4(0x0000FF, 0xFFFFFF, 0xFFFF7B, 0x0084FF)
};

static const unsigned short p509[] = {
	PACK15_4(0xFFFFCE, 0x63EFEF, 0x9C8431, 0x5A5A5A),
	PACK15_4(0xFFFFFF, 0xFF7300, 0x944200, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p50B[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFFF7B, 0x0084FF, 0xFF0000)
};

static const unsigned short p50C[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFC542, 0xFFD600, 0x943A00, 0x4A0000),
	PACK15_4(0xFFFFFF, 0x5ABDFF, 0xFF0000, 0x0000FF)
};

static const unsigned short p50D[] = {
	PACK15_4(0xFFFFFF, 0x8C8CDE, 0x52528C, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000)
};

static const unsigned short p50E[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p50F[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p510[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p511[] = {
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x00FF00, 0x318400, 0x004A00),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p512[] = {
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p514[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFF00, 0xFF0000, 0x630000, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p515[] = {
	PACK15_4(0xFFFFFF, 0xADAD84, 0x42737B, 0x000000),
	PACK15_4(0xFFFFFF, 0xFFAD63, 0x843100, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

static const unsigned short p518[] = {
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p51A[] = {
	PACK15_4(0xFFFFFF, 0xFFFF00, 0x7B4A00, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000),
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x008400, 0x000000)
};

static const unsigned short p51C[] = {
	PACK15_4(0xFFFFFF, 0x7BFF31, 0x0063C5, 0x000000),
	PACK15_4(0xFFFFFF, 0xFF8484, 0x943A3A, 0x000000),
	PACK15_4(0xFFFFFF, 0x63A5FF, 0x0000FF, 0x000000)
};

#undef PACK15_4
#undef PACK15_1
#undef TO5BIT

struct GbcPaletteEntry { const char *title; const unsigned short *p; };

static const GbcPaletteEntry gbcDirPalettes[] = {
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

static const GbcPaletteEntry gbcTitlePalettes[] = {
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

static inline std::size_t gbcDirPalettesSize() { return (sizeof gbcDirPalettes) / (sizeof gbcDirPalettes[0]); }
static inline const struct GbcPaletteEntry * gbcDirPalettesEnd() { return gbcDirPalettes + gbcDirPalettesSize(); }
static inline std::size_t gbcTitlePalettesSize() { return (sizeof gbcTitlePalettes) / (sizeof gbcTitlePalettes[0]); }
static inline const struct GbcPaletteEntry * gbcTitlePalettesEnd() { return gbcTitlePalettes + gbcTitlePalettesSize(); }

struct GbcPaletteEntryLess {
	bool operator()(const GbcPaletteEntry &lhs, const char *const rhstitle) const {
		return std::strcmp(lhs.title, rhstitle) < 0;
	}
};

static const unsigned short * findGbcDirPal(const char *const title) {
	const GbcPaletteEntry *const r = std::lower_bound(gbcDirPalettes, gbcDirPalettesEnd(), title, GbcPaletteEntryLess());
	return r < gbcDirPalettesEnd() && !std::strcmp(r->title, title) ? r->p : 0;
}

static const unsigned short * findGbcTitlePal(const char *const title) {
	const GbcPaletteEntry *const r = std::lower_bound(gbcTitlePalettes, gbcTitlePalettesEnd(), title, GbcPaletteEntryLess());
	return r < gbcTitlePalettesEnd() && !std::strcmp(r->title, title) ? r->p : 0;
}

static const unsigned short * findGbcPal(const char *const title) {
	if (const unsigned short *const pal = findGbcDirPal(title))
		return pal;
	
	return findGbcTitlePal(title);
}

static unsigned long gbcToRgb32(const unsigned rgb15) {
	const unsigned long r = rgb15 >> 10 & 0x1F;
	const unsigned long g = rgb15 >>  5 & 0x1F;
	const unsigned long b = rgb15       & 0x1F;
	
	return ((r * 13 + g * 2 + b) >> 1) << 16 | (g * 3 + b) << 9 | (r * 3 + g * 2 + b * 11) >> 1;
}

}

ColorPicker::ColorPicker(QRgb color, QWidget *parent)
: QFrame(parent), w(new QWidget)
{
	setAcceptDrops(true);
	w->setAutoFillBackground(true);
	
	setLayout(new QVBoxLayout);
	layout()->setMargin(0);
	layout()->addWidget(w);
	
	setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	setFocusPolicy(Qt::StrongFocus);
	setColor(color);
}

const QColor& ColorPicker::getQColor() const {
	return w->palette().color(QPalette::Background);
}

void ColorPicker::requestColor() {
	QColor c = QColorDialog::getColor(QColor(getColor()), this);
	
	if (c.isValid()) {
		setColor(c);
		emit colorChanged();
	}
}

void ColorPicker::setColor(const QColor &color) {
	QPalette p(w->palette());
	p.setColor(QPalette::Background, color);
	w->setPalette(p);
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
	dragStartPosition = e->pos();
}

void ColorPicker::mouseMoveEvent(QMouseEvent *e) {
	if ((e->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
		return;
	
	QDrag *const drag = new QDrag(this);
	QMimeData *const mimeData = new QMimeData;
	mimeData->setColorData(getQColor());
	drag->setMimeData(mimeData);
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

QRgb ColorPicker::getColor() const {
	return getQColor().rgb() & 0xFFFFFF;
}

void ColorPicker::setColor(QRgb rgb32) {
	setColor(QColor(rgb32));
}

ColorQuad::ColorQuad(const QString &label, QWidget *parent)
: QGroupBox(label, parent)
{
	setAcceptDrops(true);
	setLayout(new QHBoxLayout);
	
	for (int i = 0; i < 4; ++i) {
		layout()->addWidget(picker[i] = new ColorPicker);
		connect(picker[i], SIGNAL(colorChanged()), this, SLOT(pickerChanged()));
	}
	
// 	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
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
	
	for (int i = 0; i < 4; ++i) {
		dataStream >> color;
		setColor(i, color);
	}
	
	pickerChanged();
}

void ColorQuad::mousePressEvent(QMouseEvent */*e*/) {
	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	
	for (int i = 0; i < 4; ++i)
		dataStream << getColor(i);
	
	QMimeData *const mimeData = new QMimeData;
	mimeData->setData("application/x-colorquad", itemData);
	
	QDrag *const drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(Qt::CopyAction);
}

namespace {

class ImmutableStringListModel : public QStringListModel {
public:
	ImmutableStringListModel(QObject *parent = 0) : QStringListModel(parent) {}
	ImmutableStringListModel(const QStringList &strings, QObject *parent = 0) : QStringListModel(strings, parent) {}
	Qt::ItemFlags flags(const QModelIndex &index) const { return QStringListModel::flags(index) & ~Qt::ItemIsEditable; }
};

static const QStringList makeStaticStringList(const bool hasGlobal) {
	QStringList sl;
	
	if (hasGlobal)
		sl.append("Global Palette");
	
	sl.append("Current Scheme");
	sl.append("Default Gray");
	
	for (std::size_t i = 0; i < gbcDirPalettesSize(); ++i)
		sl.append(gbcDirPalettes[i].title);
	
	for (std::size_t i = 0; i < gbcTitlePalettesSize(); ++i)
		sl.append(gbcTitlePalettes[i].title);
	
	return sl;
}

static void setSchemeList(const QString &savedir, const bool hasGlobal, QStringListModel *const model) {
	QDir dir(savedir, "*.pal", QDir::Name | QDir::IgnoreCase, QDir::Files | QDir::Readable);
	QStringList dirlisting(dir.entryList());
	
	for (QStringList::iterator it = dirlisting.begin(); it != dirlisting.end(); ++it)
		it->chop(4);
	
	model->setStringList(makeStaticStringList(hasGlobal) + dirlisting);
}

static const QModelIndex schemeIndexOf(const QAbstractItemModel *const model, const QString &schemeStr) {
	const int rows = model->rowCount();
	
	for (int i = 0; i < rows; ++i) {
		if (model->index(i, 0).data().toString() == schemeStr)
			return model->index(i, 0);
	}
	
	for (int i = 0; i < rows; ++i) {
		if (model->index(i, 0).data().toString() == "Current Scheme")
			return model->index(i, 0);
	}
	
	return QModelIndex();
}

}

PaletteDialog::PaletteDialog(const QString &savepath, const PaletteDialog *global, QWidget *parent)
: QDialog(parent),
  global(global),
  listView(new QListView),
  rmSchemeButton(new QPushButton("Remove Scheme")),
  defaultScheme(global ? "Global Palette" : "Default Gray"),
  schemeString(defaultScheme)
{
	std::memset(currentColors, 0, sizeof currentColors);
	setWindowTitle(global ? "Current ROM Palette" : "Global Palette");
	
	QBoxLayout *const mainLayout = new QVBoxLayout;
	
	{
		QBoxLayout *const topLayout = new QHBoxLayout;
		
		{
			QGroupBox *const lframe = new QGroupBox("Scheme");
			QBoxLayout *const frameLayout = new QVBoxLayout;
			savedir = savepath + "/";
			QDir::root().mkpath(savedir + "stored/");
			listView->setModel(new ImmutableStringListModel(this));
			setSchemeList();
			frameLayout->addWidget(listView);
			
			{
				QPushButton *const saveButton = new QPushButton("Save Scheme...");
				connect(saveButton, SIGNAL(clicked()), this, SLOT(saveScheme()));
				frameLayout->addWidget(saveButton);
			}
			
			connect(rmSchemeButton, SIGNAL(clicked()), this, SLOT(rmScheme()));
			frameLayout->addWidget(rmSchemeButton);
			
			lframe->setLayout(frameLayout);
			topLayout->addWidget(lframe);
		}
		
		{
			QBoxLayout *const vLayout = new QVBoxLayout;
			vLayout->addWidget(quads[0] = new ColorQuad("Background"));
			vLayout->addWidget(quads[1] = new ColorQuad("Sprite 1"));
			vLayout->addWidget(quads[2] = new ColorQuad("Sprite 2"));
			topLayout->addLayout(vLayout);
// 			topLayout->setAlignment(vLayout, Qt::AlignTop);
		}
		
		mainLayout->addLayout(topLayout);
	}
	
	{
		QPushButton *const okButton = new QPushButton(tr("OK"));
		QPushButton *const cancelButton = new QPushButton(tr("Cancel"));
		
		okButton->setDefault(true);
		
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
		
		QBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(okButton);
		hLayout->addWidget(cancelButton);
		mainLayout->addLayout(hLayout);
		mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	}
	
	setLayout(mainLayout);
	
	for (int i = 0; i < 3; ++i)
		connect(quads[i], SIGNAL(colorChanged()), listView->selectionModel(), SLOT(clear()));
	
	connect(listView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(schemeChanged(const QModelIndex&, const QModelIndex&)));
	
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
	if (global) {
		saveToSettingsFile();
	} else {
		QSettings settings;
		settings.beginGroup("palette");
		saveSettings(settings);
		settings.endGroup();
	}
}

void PaletteDialog::saveSettings(QSettings &settings) {
	settings.setValue("slectedScheme", schemeString);
	
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			settings.setValue(quads[i]->title() + QString::number(j), currentColors[i][j]);
}

void PaletteDialog::loadSettings(QSettings &settings) {
	schemeString = settings.value("slectedScheme", defaultScheme).toString();
	
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			currentColors[i][j] = qvariant_cast<QRgb>(settings.value(quads[i]->title() + QString::number(j), (3 - (j & 3)) * 85 * 0x010101));
	
	restore();
	store();
}

void PaletteDialog::saveToSettingsFile() {
	if (!settingsFile.isEmpty()) {
		if (schemeString == defaultScheme) {
			QDir(savedir).remove(settingsFile);
		} else {
			QSettings settings(savedir + settingsFile, QSettings::IniFormat);
			saveSettings(settings);
		}
	}
}

void PaletteDialog::setSchemeList() {
	::setSchemeList(savedir + "stored/", global, reinterpret_cast<QStringListModel*>(listView->model()));
}

void PaletteDialog::rmScheme() {
	{
		QDir(savedir + "stored/").remove(listView->currentIndex().data().toString() + ".pal");
	}
	
	listView->selectionModel()->clear();
	setSchemeList();
}

void PaletteDialog::saveScheme() {
	bool ok;
	const QString &text = QInputDialog::getText(this, "Save Scheme", "Scheme name:",
	                                            QLineEdit::Normal, QString(), &ok);
	
	if (!ok)
		return;
	
	if (text.isEmpty() || makeStaticStringList(true).contains(text)
			|| text.size() > 200 || text.contains(QRegExp("[" + QRegExp::escape("<>:\"/\\|?*") + "]"))) {
		QMessageBox::information(this, "Invalid scheme name", "Invalid scheme name.");
		return;
	}
	
	{
		QSettings settings(savedir + "stored/" + text + ".pal", QSettings::IniFormat);
		
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				settings.setValue(quads[i]->title() + QString::number(j), quads[i]->getColor(j));
	}
	
	setSchemeList();
	
	listView->setCurrentIndex(schemeIndexOf(listView->model(), text));
}

void PaletteDialog::schemeChanged(const QModelIndex &current, const QModelIndex &/*previous*/) {
	rmSchemeButton->setEnabled(false);
	
	if (!current.isValid())
		return;
	
	const QString &str = current.data().toString();
	
	if ("Global Palette" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, global->getColor(i, j));
	} else if ("Current Scheme" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, currentColors[i][j]);
	} else if ("Default Gray" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, (3 - (j & 3)) * 85 * 0x010101);
	} else if (const unsigned short *const gbcpal = findGbcPal(str.toAscii().data())) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, gbcToRgb32(gbcpal[i * 4 + j]));
	} else {
		QSettings settings(savedir + "stored/" + str + ".pal", QSettings::IniFormat);
		
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, qvariant_cast<QRgb>(settings.value(quads[i]->title() + QString::number(j), 0)));
		
		rmSchemeButton->setEnabled(true);
	}
}

void PaletteDialog::store() {
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			currentColors[i][j] = quads[i]->getColor(j);
	
	if (!listView->currentIndex().isValid())
		listView->setCurrentIndex(schemeIndexOf(listView->model(), "Current Scheme")); //obs: will emit currentChanged()
	
	schemeString = listView->currentIndex().data().toString();
}

void PaletteDialog::restore() {
	listView->setCurrentIndex(schemeIndexOf(listView->model(), schemeString));
}

void PaletteDialog::externalChange() {
	setSchemeList();
	restore();
	store();
}

void PaletteDialog::setSettingsFile(const QString &filename, const QString &romTitle) {
	saveToSettingsFile();
	
	settingsFile = filename;
	defaultScheme = findGbcTitlePal(romTitle.toAscii().data()) ? romTitle : QString("Global Palette");
	
	QSettings settings(savedir + settingsFile, QSettings::IniFormat);
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
