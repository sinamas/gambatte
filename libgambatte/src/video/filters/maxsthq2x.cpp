/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   Copyright (C) 2003 MaxSt                                              *
 *   maxst@hiend3d.com                                                     *
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
#include "maxsthq2x.h"
#include "filterinfo.h"
#include <cstring>

typedef unsigned (*pixelop)(const unsigned w[]);

static pixelop lut[0x400];

static inline unsigned Interp2(const unsigned c1, const unsigned c2, const unsigned c3) {
	const unsigned lowbits = (c1 * 2 & 0x020202) + (c2 & 0x030303) + (c3 & 0x030303) & 0x030303;
	
	return (c1 * 2 + c2 + c3 - lowbits) >> 2;
}

static inline unsigned Interp1(const unsigned c1, const unsigned c2) {
	return Interp2(c1, c1, c2);
}

static inline unsigned Interp5(const unsigned c1, const unsigned c2) {
	return c1 + c2 - ((c1 ^ c2) & 0x010101) >> 1;
}

static inline unsigned Interp6(const unsigned c1, const unsigned c2, const unsigned c3) {
	const unsigned lowbits = (c1 * 4 & 0x040404) + (c1 & 0x070707) + (c2 * 2 & 0x060606) + (c3 & 0x070707) & 0x070707;
	
	return ((c1 * 5 + c2 * 2 + c3) - lowbits) >> 3;
}

static inline unsigned Interp7(const unsigned c1, const unsigned c2, const unsigned c3) {
	const unsigned lowbits = ((c1 * 2 & 0x020202) + (c1 & 0x030303)) * 2 + (c2 & 0x070707) + (c3 & 0x070707) & 0x070707;
	
	return ((c1 * 6 + c2 + c3) - lowbits) >> 3;
}

static inline unsigned Interp9(unsigned c1, const unsigned c2, const unsigned c3) {
	c1 <<= 1;
	
	const unsigned rb = (c1 & 0x01FE01FE) + ((c2 & 0xFF00FF) + (c3 & 0xFF00FF)) * 3;
	const unsigned g = (c1 & 0x0001FE00) + ((c2 & 0x00FF00) + (c3 & 0x00FF00)) * 3;
	
	return ((rb & 0x07F807F8) | (g & 0x0007F800)) >> 3;
}

static inline unsigned Interp10(const unsigned c1, const unsigned c2, const unsigned c3) {
	const unsigned rb = (c1 & 0xFF00FF) * 14 + (c2 & 0xFF00FF) + (c3 & 0xFF00FF);
	const unsigned g = (c1 & 0x00FF00) * 14 + (c2 & 0x00FF00) + (c3 & 0x00FF00);
	
	return ((rb & 0x0FF00FF0) | (g & 0x000FF000)) >> 4;
}

static inline unsigned pixel00_0(const unsigned w[]) { return w[5]; }
static inline unsigned pixel00_10(const unsigned w[]) { return Interp1(w[5], w[1]); }
static inline unsigned pixel00_11(const unsigned w[]) { return Interp1(w[5], w[4]); }
static inline unsigned pixel00_12(const unsigned w[]) { return Interp1(w[5], w[2]); }
static inline unsigned pixel00_20(const unsigned w[]) { return Interp2(w[5], w[4], w[2]); }
static inline unsigned pixel00_21(const unsigned w[]) { return Interp2(w[5], w[1], w[2]); }
static inline unsigned pixel00_22(const unsigned w[]) { return Interp2(w[5], w[1], w[4]); }
static inline unsigned pixel00_60(const unsigned w[]) { return Interp6(w[5], w[2], w[4]); }
static inline unsigned pixel00_61(const unsigned w[]) { return Interp6(w[5], w[4], w[2]); }
static inline unsigned pixel00_70(const unsigned w[]) { return Interp7(w[5], w[4], w[2]); }
static inline unsigned pixel00_90(const unsigned w[]) { return Interp9(w[5], w[4], w[2]); }
static inline unsigned pixel00_100(const unsigned w[]) { return Interp10(w[5], w[4], w[2]); }

#define pixel01_0 pixel00_0
static inline unsigned pixel01_10(const unsigned w[]) { return Interp1(w[5], w[3]); }
#define pixel01_11 pixel00_12
static inline unsigned pixel01_12(const unsigned w[]) { return Interp1(w[5], w[6]); }
static inline unsigned pixel01_20(const unsigned w[]) { return Interp2(w[5], w[2], w[6]); }
static inline unsigned pixel01_21(const unsigned w[]) { return Interp2(w[5], w[3], w[6]); }
static inline unsigned pixel01_22(const unsigned w[]) { return Interp2(w[5], w[3], w[2]); }
static inline unsigned pixel01_60(const unsigned w[]) { return Interp6(w[5], w[6], w[2]); }
static inline unsigned pixel01_61(const unsigned w[]) { return Interp6(w[5], w[2], w[6]); }
static inline unsigned pixel01_70(const unsigned w[]) { return Interp7(w[5], w[2], w[6]); }
static inline unsigned pixel01_90(const unsigned w[]) { return Interp9(w[5], w[2], w[6]); }
static inline unsigned pixel01_100(const unsigned w[]) { return Interp10(w[5], w[2], w[6]); }

#define pixel10_0 pixel00_0
static inline unsigned pixel10_10(const unsigned w[]) { return Interp1(w[5], w[7]); }
static inline unsigned pixel10_11(const unsigned w[]) { return Interp1(w[5], w[8]); }
#define pixel10_12 pixel00_11
static inline unsigned pixel10_20(const unsigned w[]) { return Interp2(w[5], w[8], w[4]); }
static inline unsigned pixel10_21(const unsigned w[]) { return Interp2(w[5], w[7], w[4]); }
static inline unsigned pixel10_22(const unsigned w[]) { return Interp2(w[5], w[7], w[8]); }
static inline unsigned pixel10_60(const unsigned w[]) { return Interp6(w[5], w[4], w[8]); }
static inline unsigned pixel10_61(const unsigned w[]) { return Interp6(w[5], w[8], w[4]); }
static inline unsigned pixel10_70(const unsigned w[]) { return Interp7(w[5], w[8], w[4]); }
static inline unsigned pixel10_90(const unsigned w[]) { return Interp9(w[5], w[8], w[4]); }
static inline unsigned pixel10_100(const unsigned w[]) { return Interp10(w[5], w[8], w[4]); }

#define pixel11_0 pixel00_0
static inline unsigned pixel11_10(const unsigned w[]) { return Interp1(w[5], w[9]); }
#define pixel11_11 pixel01_12
#define pixel11_12 pixel10_11
static inline unsigned pixel11_20(const unsigned w[]) { return Interp2(w[5], w[6], w[8]); }
static inline unsigned pixel11_21(const unsigned w[]) { return Interp2(w[5], w[9], w[8]); }
static inline unsigned pixel11_22(const unsigned w[]) { return Interp2(w[5], w[9], w[6]); }
static inline unsigned pixel11_60(const unsigned w[]) { return Interp6(w[5], w[8], w[6]); }
static inline unsigned pixel11_61(const unsigned w[]) { return Interp6(w[5], w[6], w[8]); }
static inline unsigned pixel11_70(const unsigned w[]) { return Interp7(w[5], w[6], w[8]); }
static inline unsigned pixel11_90(const unsigned w[]) { return Interp9(w[5], w[6], w[8]); }
static inline unsigned pixel11_100(const unsigned w[]) { return Interp10(w[5], w[6], w[8]); }

static inline bool Diff(const unsigned int w1, const unsigned int w2) {
	const unsigned rdiff = (w1 >> 16) - (w2 >> 16);
	const unsigned gdiff = (w1 >> 8 & 0xFF) - (w2 >> 8 & 0xFF);
	const unsigned bdiff = (w1 & 0xFF) - (w2 & 0xFF);
	
	return rdiff + gdiff + bdiff + 0xC0U > 0xC0U * 2    ||
		rdiff - bdiff + 0x1CU > 0x1CU * 2            ||
		gdiff * 2 - rdiff - bdiff + 0x30U > 0x30U * 2;
}

static unsigned pixel00_cond0(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_0(w);
	} else {
		return pixel00_20(w);
	}
}

static unsigned pixel00_cond1(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_10(w);
	} else {
		return pixel00_20(w);
	}
}

static unsigned pixel00_cond2(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel00_12(w);
	} else {
		return pixel00_61(w);
	}
}

static unsigned pixel00_cond3(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_10(w);
	} else {
		return pixel00_90(w);
	}
}

static unsigned pixel00_cond4(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel00_11(w);
	} else {
		return pixel00_60(w);
	}
}

static unsigned pixel00_cond5(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_10(w);
	} else {
		return pixel00_70(w);
	}
}

static unsigned pixel00_cond6(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_0(w);
	} else {
		return pixel00_90(w);
	}
}

static unsigned pixel00_cond7(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel00_0(w);
	} else {
		return pixel00_100(w);
	}
}

static unsigned pixel01_cond0(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_0(w);
	} else {
		return pixel01_20(w);
	}
}

static unsigned pixel01_cond1(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_10(w);
	} else {
		return pixel01_20(w);
	}
}

static unsigned pixel01_cond2(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel01_12(w);
	} else {
		return pixel01_61(w);
	}
}

static unsigned pixel01_cond3(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_10(w);
	} else {
		return pixel01_90(w);
	}
}

static unsigned pixel01_cond4(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel01_11(w);
	} else {
		return pixel01_60(w);
	}
}

static unsigned pixel01_cond5(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_10(w);
	} else {
		return pixel01_70(w);
	}
}

static unsigned pixel01_cond6(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_0(w);
	} else {
		return pixel01_90(w);
	}
}

static unsigned pixel01_cond7(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel01_0(w);
	} else {
		return pixel01_100(w);
	}
}

static unsigned pixel10_cond0(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_0(w);
	} else {
		return pixel10_20(w);
	}
}

static unsigned pixel10_cond1(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_10(w);
	} else {
		return pixel10_20(w);
	}
}

static unsigned pixel10_cond2(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel10_12(w);
	} else {
		return pixel10_61(w);
	}
}

static unsigned pixel10_cond3(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_10(w);
	} else {
		return pixel10_90(w);
	}
}

static unsigned pixel10_cond4(const unsigned w[]) {
	if (Diff(w[4], w[2])) {
		return pixel10_11(w);
	} else {
		return pixel10_60(w);
	}
}

static unsigned pixel10_cond5(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_10(w);
	} else {
		return pixel10_70(w);
	}
}

static unsigned pixel10_cond6(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_0(w);
	} else {
		return pixel10_90(w);
	}
}

static unsigned pixel10_cond7(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel10_0(w);
	} else {
		return pixel10_100(w);
	}
}

static unsigned pixel11_cond0(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_0(w);
	} else {
		return pixel11_20(w);
	}
}

static unsigned pixel11_cond1(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_10(w);
	} else {
		return pixel11_20(w);
	}
}

static unsigned pixel11_cond2(const unsigned w[]) {
	if (Diff(w[2], w[6])) {
		return pixel11_12(w);
	} else {
		return pixel11_61(w);
	}
}

static unsigned pixel11_cond3(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_10(w);
	} else {
		return pixel11_90(w);
	}
}

static unsigned pixel11_cond4(const unsigned w[]) {
	if (Diff(w[8], w[4])) {
		return pixel11_11(w);
	} else {
		return pixel11_60(w);
	}
}

static unsigned pixel11_cond5(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_10(w);
	} else {
		return pixel11_70(w);
	}
}

static unsigned pixel11_cond6(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_0(w);
	} else {
		return pixel11_90(w);
	}
}

static unsigned pixel11_cond7(const unsigned w[]) {
	if (Diff(w[6], w[8])) {
		return pixel11_0(w);
	} else {
		return pixel11_100(w);
	}
}

static void buildLut() {
	lut[106 * 4] = lut[30 * 4] = lut[62 * 4] = lut[110 * 4] = lut[126 * 4] = lut[238 * 4] = lut[190 * 4] = lut[250 * 4] = lut[222 * 4] = lut[254 * 4] = pixel00_10;
	
	lut[163 * 4] = lut[135 * 4] = lut[67 * 4] = lut[99 * 4] = lut[71 * 4] = lut[195 * 4] = lut[83 * 4] = lut[211 * 4] = lut[103 * 4] = lut[227 * 4] = lut[199 * 4] = lut[87 * 4] = lut[167 * 4] = lut[115 * 4] = lut[147 * 4] = lut[231 * 4] = lut[243 * 4] = lut[151 * 4] = lut[215 * 4] = lut[247 * 4] = pixel00_11;
	
	lut[141 * 4] = lut[45 * 4] = lut[25 * 4] = lut[29 * 4] = lut[57 * 4] = lut[153 * 4] = lut[89 * 4] = lut[217 * 4] = lut[185 * 4] = lut[61 * 4] = lut[157 * 4] = lut[121 * 4] = lut[173 * 4] = lut[93 * 4] = lut[201 * 4] = lut[189 * 4] = lut[221 * 4] = lut[233 * 4] = lut[249 * 4] = lut[253 * 4] = pixel00_12;
	
	lut[165 * 4] = lut[49 * 4] = lut[69 * 4] = lut[53 * 4] = lut[177 * 4] = lut[197 * 4] = lut[101 * 4] = lut[81 * 4] = lut[180 * 4] = lut[225 * 4] = lut[209 * 4] = lut[85 * 4] = lut[113 * 4] = lut[212 * 4] = lut[240 * 4] = lut[229 * 4] = lut[181 * 4] = lut[116 * 4] = lut[244 * 4] = pixel00_20;
	
	lut[140 * 4] = lut[172 * 4] = lut[76 * 4] = lut[24 * 4] = lut[108 * 4] = lut[204 * 4] = lut[28 * 4] = lut[152 * 4] = lut[56 * 4] = lut[248 * 4] = lut[216 * 4] = lut[120 * 4] = lut[184 * 4] = lut[156 * 4] = lut[60 * 4] = lut[92 * 4] = lut[232 * 4] = lut[124 * 4] = lut[188 * 4] = lut[220 * 4] = lut[252 * 4] = pixel00_21;
	
	lut[162 * 4] = lut[166 * 4] = lut[50 * 4] = lut[66 * 4] = lut[54 * 4] = lut[178 * 4] = lut[70 * 4] = lut[194 * 4] = lut[98 * 4] = lut[214 * 4] = lut[86 * 4] = lut[210 * 4] = lut[198 * 4] = lut[226 * 4] = lut[102 * 4] = lut[114 * 4] = lut[150 * 4] = lut[118 * 4] = lut[230 * 4] = lut[242 * 4] = lut[246 * 4] = pixel00_22;
	
	lut[27 * 4 + 1] = lut[210 * 4 + 1] = lut[211 * 4 + 1] = lut[155 * 4 + 1] = lut[219 * 4 + 1] = lut[187 * 4 + 1] = lut[243 * 4 + 1] = lut[250 * 4 + 1] = lut[123 * 4 + 1] = lut[251 * 4 + 1] = pixel01_10;
	
	lut[53 * 4 + 1] = lut[180 * 4 + 1] = lut[28 * 4 + 1] = lut[29 * 4 + 1] = lut[156 * 4 + 1] = lut[60 * 4 + 1] = lut[92 * 4 + 1] = lut[124 * 4 + 1] = lut[188 * 4 + 1] = lut[61 * 4 + 1] = lut[157 * 4 + 1] = lut[220 * 4 + 1] = lut[181 * 4 + 1] = lut[93 * 4 + 1] = lut[116 * 4 + 1] = lut[189 * 4 + 1] = lut[125 * 4 + 1] = lut[244 * 4 + 1] = lut[252 * 4 + 1] = lut[253 * 4 + 1] = pixel01_11;
	
	lut[166 * 4 + 1] = lut[135 * 4 + 1] = lut[70 * 4 + 1] = lut[198 * 4 + 1] = lut[71 * 4 + 1] = lut[102 * 4 + 1] = lut[78 * 4 + 1] = lut[110 * 4 + 1] = lut[103 * 4 + 1] = lut[230 * 4 + 1] = lut[199 * 4 + 1] = lut[79 * 4 + 1] = lut[167 * 4 + 1] = lut[206 * 4 + 1] = lut[46 * 4 + 1] = lut[231 * 4 + 1] = lut[238 * 4 + 1] = lut[47 * 4 + 1] = lut[111 * 4 + 1] = lut[239 * 4 + 1] = pixel01_12;
	
	lut[165 * 4 + 1] = lut[69 * 4 + 1] = lut[140 * 4 + 1] = lut[197 * 4 + 1] = lut[101 * 4 + 1] = lut[172 * 4 + 1] = lut[141 * 4 + 1] = lut[76 * 4 + 1] = lut[225 * 4 + 1] = lut[45 * 4 + 1] = lut[108 * 4 + 1] = lut[204 * 4 + 1] = lut[77 * 4 + 1] = lut[232 * 4 + 1] = lut[105 * 4 + 1] = lut[229 * 4 + 1] = lut[173 * 4 + 1] = lut[201 * 4 + 1] = lut[233 * 4 + 1] = pixel01_20;
	
	lut[162 * 4 + 1] = lut[163 * 4 + 1] = lut[138 * 4 + 1] = lut[66 * 4 + 1] = lut[139 * 4 + 1] = lut[170 * 4 + 1] = lut[67 * 4 + 1] = lut[194 * 4 + 1] = lut[98 * 4 + 1] = lut[107 * 4 + 1] = lut[106 * 4 + 1] = lut[75 * 4 + 1] = lut[99 * 4 + 1] = lut[226 * 4 + 1] = lut[195 * 4 + 1] = lut[202 * 4 + 1] = lut[43 * 4 + 1] = lut[203 * 4 + 1] = lut[227 * 4 + 1] = lut[234 * 4 + 1] = lut[235 * 4 + 1] = pixel01_21;
	
	lut[49 * 4 + 1] = lut[177 * 4 + 1] = lut[81 * 4 + 1] = lut[24 * 4 + 1] = lut[209 * 4 + 1] = lut[113 * 4 + 1] = lut[152 * 4 + 1] = lut[56 * 4 + 1] = lut[25 * 4 + 1] = lut[248 * 4 + 1] = lut[216 * 4 + 1] = lut[120 * 4 + 1] = lut[184 * 4 + 1] = lut[57 * 4 + 1] = lut[153 * 4 + 1] = lut[89 * 4 + 1] = lut[240 * 4 + 1] = lut[217 * 4 + 1] = lut[185 * 4 + 1] = lut[121 * 4 + 1] = lut[249 * 4 + 1] = pixel01_22;
	
	lut[216 * 4 + 2] = lut[75 * 4 + 2] = lut[203 * 4 + 2] = lut[217 * 4 + 2] = lut[219 * 4 + 2] = lut[221 * 4 + 2] = lut[207 * 4 + 2] = lut[95 * 4 + 2] = lut[222 * 4 + 2] = lut[223 * 4 + 2] = pixel10_10;
	
	lut[172 * 4 + 2] = lut[45 * 4 + 2] = lut[56 * 4 + 2] = lut[184 * 4 + 2] = lut[57 * 4 + 2] = lut[60 * 4 + 2] = lut[58 * 4 + 2] = lut[62 * 4 + 2] = lut[188 * 4 + 2] = lut[185 * 4 + 2] = lut[61 * 4 + 2] = lut[59 * 4 + 2] = lut[173 * 4 + 2] = lut[186 * 4 + 2] = lut[46 * 4 + 2] = lut[189 * 4 + 2] = lut[190 * 4 + 2] = lut[47 * 4 + 2] = lut[63 * 4 + 2] = lut[191 * 4 + 2] = pixel10_11;
	
	lut[101 * 4 + 2] = lut[225 * 4 + 2] = lut[98 * 4 + 2] = lut[99 * 4 + 2] = lut[226 * 4 + 2] = lut[102 * 4 + 2] = lut[114 * 4 + 2] = lut[118 * 4 + 2] = lut[103 * 4 + 2] = lut[227 * 4 + 2] = lut[230 * 4 + 2] = lut[242 * 4 + 2] = lut[229 * 4 + 2] = lut[115 * 4 + 2] = lut[116 * 4 + 2] = lut[231 * 4 + 2] = lut[119 * 4 + 2] = lut[244 * 4 + 2] = lut[246 * 4 + 2] = lut[247 * 4 + 2] = pixel10_12;
	
	lut[165 * 4 + 2] = lut[162 * 4 + 2] = lut[49 * 4 + 2] = lut[163 * 4 + 2] = lut[166 * 4 + 2] = lut[53 * 4 + 2] = lut[177 * 4 + 2] = lut[50 * 4 + 2] = lut[135 * 4 + 2] = lut[180 * 4 + 2] = lut[54 * 4 + 2] = lut[51 * 4 + 2] = lut[178 * 4 + 2] = lut[23 * 4 + 2] = lut[150 * 4 + 2] = lut[167 * 4 + 2] = lut[181 * 4 + 2] = lut[147 * 4 + 2] = lut[151 * 4 + 2] = pixel10_20;
	
	lut[69 * 4 + 2] = lut[197 * 4 + 2] = lut[81 * 4 + 2] = lut[66 * 4 + 2] = lut[209 * 4 + 2] = lut[85 * 4 + 2] = lut[67 * 4 + 2] = lut[70 * 4 + 2] = lut[194 * 4 + 2] = lut[214 * 4 + 2] = lut[86 * 4 + 2] = lut[210 * 4 + 2] = lut[198 * 4 + 2] = lut[71 * 4 + 2] = lut[195 * 4 + 2] = lut[83 * 4 + 2] = lut[212 * 4 + 2] = lut[211 * 4 + 2] = lut[199 * 4 + 2] = lut[87 * 4 + 2] = lut[215 * 4 + 2] = pixel10_21;
	
	lut[140 * 4 + 2] = lut[141 * 4 + 2] = lut[138 * 4 + 2] = lut[24 * 4 + 2] = lut[139 * 4 + 2] = lut[142 * 4 + 2] = lut[28 * 4 + 2] = lut[152 * 4 + 2] = lut[25 * 4 + 2] = lut[31 * 4 + 2] = lut[27 * 4 + 2] = lut[30 * 4 + 2] = lut[29 * 4 + 2] = lut[156 * 4 + 2] = lut[153 * 4 + 2] = lut[154 * 4 + 2] = lut[15 * 4 + 2] = lut[155 * 4 + 2] = lut[157 * 4 + 2] = lut[158 * 4 + 2] = lut[159 * 4 + 2] = pixel10_22;
	
	lut[86 * 4 + 3] = lut[120 * 4 + 3] = lut[124 * 4 + 3] = lut[118 * 4 + 3] = lut[126 * 4 + 3] = lut[125 * 4 + 3] = lut[119 * 4 + 3] = lut[123 * 4 + 3] = lut[95 * 4 + 3] = lut[127 * 4 + 3] = pixel11_10;
	
	lut[197 * 4 + 3] = lut[225 * 4 + 3] = lut[194 * 4 + 3] = lut[198 * 4 + 3] = lut[226 * 4 + 3] = lut[195 * 4 + 3] = lut[202 * 4 + 3] = lut[203 * 4 + 3] = lut[227 * 4 + 3] = lut[230 * 4 + 3] = lut[199 * 4 + 3] = lut[234 * 4 + 3] = lut[229 * 4 + 3] = lut[206 * 4 + 3] = lut[201 * 4 + 3] = lut[231 * 4 + 3] = lut[207 * 4 + 3] = lut[233 * 4 + 3] = lut[235 * 4 + 3] = lut[239 * 4 + 3] = pixel11_11;
	
	lut[177 * 4 + 3] = lut[180 * 4 + 3] = lut[152 * 4 + 3] = lut[184 * 4 + 3] = lut[156 * 4 + 3] = lut[153 * 4 + 3] = lut[154 * 4 + 3] = lut[155 * 4 + 3] = lut[188 * 4 + 3] = lut[185 * 4 + 3] = lut[157 * 4 + 3] = lut[158 * 4 + 3] = lut[181 * 4 + 3] = lut[186 * 4 + 3] = lut[147 * 4 + 3] = lut[189 * 4 + 3] = lut[187 * 4 + 3] = lut[151 * 4 + 3] = lut[159 * 4 + 3] = lut[191 * 4 + 3] = pixel11_12;
	
	lut[165 * 4 + 3] = lut[162 * 4 + 3] = lut[140 * 4 + 3] = lut[163 * 4 + 3] = lut[166 * 4 + 3] = lut[172 * 4 + 3] = lut[141 * 4 + 3] = lut[138 * 4 + 3] = lut[135 * 4 + 3] = lut[45 * 4 + 3] = lut[139 * 4 + 3] = lut[170 * 4 + 3] = lut[142 * 4 + 3] = lut[43 * 4 + 3] = lut[15 * 4 + 3] = lut[167 * 4 + 3] = lut[173 * 4 + 3] = lut[46 * 4 + 3] = lut[47 * 4 + 3] = pixel11_20;
	
	lut[49 * 4 + 3] = lut[53 * 4 + 3] = lut[50 * 4 + 3] = lut[24 * 4 + 3] = lut[54 * 4 + 3] = lut[51 * 4 + 3] = lut[28 * 4 + 3] = lut[56 * 4 + 3] = lut[25 * 4 + 3] = lut[31 * 4 + 3] = lut[27 * 4 + 3] = lut[30 * 4 + 3] = lut[29 * 4 + 3] = lut[57 * 4 + 3] = lut[60 * 4 + 3] = lut[58 * 4 + 3] = lut[23 * 4 + 3] = lut[62 * 4 + 3] = lut[61 * 4 + 3] = lut[59 * 4 + 3] = lut[63 * 4 + 3] = pixel11_21;
	
	lut[69 * 4 + 3] = lut[101 * 4 + 3] = lut[76 * 4 + 3] = lut[66 * 4 + 3] = lut[108 * 4 + 3] = lut[77 * 4 + 3] = lut[67 * 4 + 3] = lut[70 * 4 + 3] = lut[98 * 4 + 3] = lut[107 * 4 + 3] = lut[106 * 4 + 3] = lut[75 * 4 + 3] = lut[99 * 4 + 3] = lut[71 * 4 + 3] = lut[102 * 4 + 3] = lut[78 * 4 + 3] = lut[105 * 4 + 3] = lut[110 * 4 + 3] = lut[103 * 4 + 3] = lut[79 * 4 + 3] = lut[111 * 4 + 3] = pixel11_22;
	
	lut[139 * 4] = lut[31 * 4] = lut[107 * 4] = lut[27 * 4] = lut[75 * 4] = lut[203 * 4] = lut[155 * 4] = lut[59 * 4] = lut[79 * 4] = lut[91 * 4] = lut[219 * 4] = lut[123 * 4] = lut[95 * 4] = lut[235 * 4] = lut[159 * 4] = lut[251 * 4] = lut[223 * 4] = pixel00_cond0;
	lut[138 * 4] = pixel00_cond1;
	lut[77 * 4] = lut[105 * 4] = lut[125 * 4] = pixel00_cond2;
	lut[170 * 4] = lut[142 * 4] = pixel00_cond3;
	lut[51 * 4] = lut[23 * 4] = lut[119 * 4] = pixel00_cond4;
	lut[58 * 4] = lut[202 * 4] = lut[78 * 4] = lut[154 * 4] = lut[90 * 4] = lut[158 * 4] = lut[234 * 4] = lut[122 * 4] = lut[94 * 4] = lut[218 * 4] = lut[186 * 4] = lut[206 * 4] = lut[46 * 4] = pixel00_cond5;
	lut[43 * 4] = lut[15 * 4] = lut[207 * 4] = lut[187 * 4] = pixel00_cond6;
	lut[47 * 4] = lut[111 * 4] = lut[63 * 4] = lut[239 * 4] = lut[127 * 4] = lut[191 * 4] = lut[255 * 4] = pixel00_cond7;
	
	lut[54 * 4 + 1] = lut[31 * 4 + 1] = lut[214 * 4 + 1] = lut[86 * 4 + 1] = lut[30 * 4 + 1] = lut[62 * 4 + 1] = lut[118 * 4 + 1] = lut[158 * 4 + 1] = lut[87 * 4 + 1] = lut[94 * 4 + 1] = lut[126 * 4 + 1] = lut[95 * 4 + 1] = lut[222 * 4 + 1] = lut[63 * 4 + 1] = lut[246 * 4 + 1] = lut[254 * 4 + 1] = lut[127 * 4 + 1] = pixel01_cond0;
	lut[50 * 4 + 1] = pixel01_cond1;
	lut[142 * 4 + 1] = lut[15 * 4 + 1] = lut[207 * 4 + 1] = pixel01_cond2;
	lut[51 * 4 + 1] = lut[178 * 4 + 1] = pixel01_cond3;
	lut[85 * 4 + 1] = lut[212 * 4 + 1] = lut[221 * 4 + 1] = pixel01_cond4;
	lut[58 * 4 + 1] = lut[83 * 4 + 1] = lut[154 * 4 + 1] = lut[114 * 4 + 1] = lut[90 * 4 + 1] = lut[242 * 4 + 1] = lut[59 * 4 + 1] = lut[122 * 4 + 1] = lut[218 * 4 + 1] = lut[91 * 4 + 1] = lut[186 * 4 + 1] = lut[115 * 4 + 1] = lut[147 * 4 + 1] = pixel01_cond5;
	lut[23 * 4 + 1] = lut[150 * 4 + 1] = lut[190 * 4 + 1] = lut[119 * 4 + 1] = pixel01_cond6;
	lut[151 * 4 + 1] = lut[159 * 4 + 1] = lut[215 * 4 + 1] = lut[191 * 4 + 1] = lut[223 * 4 + 1] = lut[247 * 4 + 1] = lut[255 * 4 + 1] = pixel01_cond7;
	
	lut[108 * 4 + 2] = lut[248 * 4 + 2] = lut[107 * 4 + 2] = lut[106 * 4 + 2] = lut[120 * 4 + 2] = lut[124 * 4 + 2] = lut[110 * 4 + 2] = lut[234 * 4 + 2] = lut[121 * 4 + 2] = lut[122 * 4 + 2] = lut[126 * 4 + 2] = lut[250 * 4 + 2] = lut[123 * 4 + 2] = lut[252 * 4 + 2] = lut[111 * 4 + 2] = lut[254 * 4 + 2] = lut[127 * 4 + 2] = pixel10_cond0;
	lut[76 * 4 + 2] = pixel10_cond1;
	lut[113 * 4 + 2] = lut[240 * 4 + 2] = lut[243 * 4 + 2] = pixel10_cond2;
	lut[204 * 4 + 2] = lut[77 * 4 + 2] = pixel10_cond3;
	lut[170 * 4 + 2] = lut[43 * 4 + 2] = lut[187 * 4 + 2] = pixel10_cond4;
	lut[92 * 4 + 2] = lut[202 * 4 + 2] = lut[78 * 4 + 2] = lut[89 * 4 + 2] = lut[90 * 4 + 2] = lut[220 * 4 + 2] = lut[79 * 4 + 2] = lut[94 * 4 + 2] = lut[218 * 4 + 2] = lut[91 * 4 + 2] = lut[93 * 4 + 2] = lut[206 * 4 + 2] = lut[201 * 4 + 2] = pixel10_cond5;
	lut[232 * 4 + 2] = lut[105 * 4 + 2] = lut[125 * 4 + 2] = lut[238 * 4 + 2] = pixel10_cond6;
	lut[233 * 4 + 2] = lut[249 * 4 + 2] = lut[235 * 4 + 2] = lut[253 * 4 + 2] = lut[251 * 4 + 2] = lut[239 * 4 + 2] = lut[255 * 4 + 2] = pixel10_cond7;
	
	lut[209 * 4 + 3] = lut[214 * 4 + 3] = lut[248 * 4 + 3] = lut[216 * 4 + 3] = lut[210 * 4 + 3] = lut[211 * 4 + 3] = lut[217 * 4 + 3] = lut[220 * 4 + 3] = lut[242 * 4 + 3] = lut[218 * 4 + 3] = lut[219 * 4 + 3] = lut[250 * 4 + 3] = lut[222 * 4 + 3] = lut[249 * 4 + 3] = lut[215 * 4 + 3] = lut[251 * 4 + 3] = lut[223 * 4 + 3] = pixel11_cond0;
	lut[81 * 4 + 3] = pixel11_cond1;
	lut[178 * 4 + 3] = lut[150 * 4 + 3] = lut[190 * 4 + 3] = pixel11_cond2;
	lut[85 * 4 + 3] = lut[113 * 4 + 3] = pixel11_cond3;
	lut[204 * 4 + 3] = lut[232 * 4 + 3] = lut[238 * 4 + 3] = pixel11_cond4;
	lut[83 * 4 + 3] = lut[92 * 4 + 3] = lut[114 * 4 + 3] = lut[89 * 4 + 3] = lut[90 * 4 + 3] = lut[121 * 4 + 3] = lut[87 * 4 + 3] = lut[122 * 4 + 3] = lut[94 * 4 + 3] = lut[91 * 4 + 3] = lut[115 * 4 + 3] = lut[93 * 4 + 3] = lut[116 * 4 + 3] = pixel11_cond5;
	lut[212 * 4 + 3] = lut[240 * 4 + 3] = lut[221 * 4 + 3] = lut[243 * 4 + 3] = pixel11_cond6;
	lut[244 * 4 + 3] = lut[252 * 4 + 3] = lut[246 * 4 + 3] = lut[254 * 4 + 3] = lut[253 * 4 + 3] = lut[247 * 4 + 3] = lut[255 * 4 + 3] = pixel11_cond7;
	
	
	lut[0 * 4] = lut[1 * 4] = lut[4 * 4] = lut[32 * 4] = lut[128 * 4] = lut[5 * 4] = lut[132 * 4] = lut[160 * 4] = lut[33 * 4] = lut[129 * 4] = lut[36 * 4] = lut[133 * 4] = lut[164 * 4] = lut[161 * 4] = lut[37 * 4] = lut[165 * 4];
	
	lut[2 * 4] = lut[34 * 4] = lut[130 * 4] = lut[162 * 4];
	
	lut[16 * 4] = lut[17 * 4] = lut[48 * 4] = lut[49 * 4];
	
	lut[64 * 4] = lut[65 * 4] = lut[68 * 4] = lut[69 * 4];
	
	lut[8 * 4] = lut[12 * 4] = lut[136 * 4] = lut[140 * 4];
	
	lut[3 * 4] = lut[35 * 4] = lut[131 * 4] = lut[163 * 4];
	
	lut[6 * 4] = lut[38 * 4] = lut[134 * 4] = lut[166 * 4];
	
	lut[20 * 4] = lut[21 * 4] = lut[52 * 4] = lut[53 * 4];
	
	lut[144 * 4] = lut[145 * 4] = lut[176 * 4] = lut[177 * 4];
	
	lut[192 * 4] = lut[193 * 4] = lut[196 * 4] = lut[197 * 4];
	
	lut[96 * 4] = lut[97 * 4] = lut[100 * 4] = lut[101 * 4];
	
	lut[40 * 4] = lut[44 * 4] = lut[168 * 4] = lut[172 * 4];
	
	lut[9 * 4] = lut[13 * 4] = lut[137 * 4] = lut[141 * 4];
	
	lut[18 * 4] = lut[50 * 4];
	
	lut[80 * 4] = lut[81 * 4];
	
	lut[72 * 4] = lut[76 * 4];
	
	lut[10 * 4] = lut[138 * 4];
	
	lut[7 * 4] = lut[39 * 4] = lut[135 * 4];
	
	lut[148 * 4] = lut[149 * 4] = lut[180 * 4];
	
	lut[224 * 4] = lut[228 * 4] = lut[225 * 4];
	
	lut[41 * 4] = lut[169 * 4] = lut[45 * 4];
	
	lut[22 * 4] = lut[54 * 4];
	
	lut[208 * 4] = lut[209 * 4];
	
	lut[104 * 4] = lut[108 * 4];
	
	lut[11 * 4] = lut[139 * 4];
	
	lut[19 * 4] = lut[51 * 4];
	
	lut[146 * 4] = lut[178 * 4];
	
	lut[84 * 4] = lut[85 * 4];
	
	lut[112 * 4] = lut[113 * 4];
	
	lut[200 * 4] = lut[204 * 4];
	
	lut[73 * 4] = lut[77 * 4];
	
	lut[42 * 4] = lut[170 * 4];
	
	lut[14 * 4] = lut[142 * 4];
	
	lut[26 * 4] = lut[31 * 4];
	
	lut[82 * 4] = lut[214 * 4];
	
	lut[88 * 4] = lut[248 * 4];
	
	lut[74 * 4] = lut[107 * 4];
	
	lut[55 * 4] = lut[23 * 4];
	
	lut[182 * 4] = lut[150 * 4];
	
	lut[213 * 4] = lut[212 * 4];
	
	lut[241 * 4] = lut[240 * 4];
	
	lut[236 * 4] = lut[232 * 4];
	
	lut[109 * 4] = lut[105 * 4];
	
	lut[171 * 4] = lut[43 * 4];
	
	lut[143 * 4] = lut[15 * 4];
	
	lut[205 * 4] = lut[201 * 4];
	
	lut[174 * 4] = lut[46 * 4];
	
	lut[179 * 4] = lut[147 * 4];
	
	lut[117 * 4] = lut[116 * 4];
	
	lut[237 * 4] = lut[233 * 4];
	
	lut[175 * 4] = lut[47 * 4];
	
	lut[183 * 4] = lut[151 * 4];
	
	lut[245 * 4] = lut[244 * 4];
	
	
	lut[0 * 4 + 1] = lut[1 * 4 + 1] = lut[4 * 4 + 1] = lut[32 * 4 + 1] = lut[128 * 4 + 1] = lut[5 * 4 + 1] = lut[132 * 4 + 1] = lut[160 * 4 + 1] = lut[33 * 4 + 1] = lut[129 * 4 + 1] = lut[36 * 4 + 1] = lut[133 * 4 + 1] = lut[164 * 4 + 1] = lut[161 * 4 + 1] = lut[37 * 4 + 1] = lut[165 * 4 + 1];
	
	lut[2 * 4 + 1] = lut[34 * 4 + 1] = lut[130 * 4 + 1] = lut[162 * 4 + 1];
	
	lut[16 * 4 + 1] = lut[17 * 4 + 1] = lut[48 * 4 + 1] = lut[49 * 4 + 1];
	
	lut[64 * 4 + 1] = lut[65 * 4 + 1] = lut[68 * 4 + 1] = lut[69 * 4 + 1];
	
	lut[8 * 4 + 1] = lut[12 * 4 + 1] = lut[136 * 4 + 1] = lut[140 * 4 + 1];
	
	lut[3 * 4 + 1] = lut[35 * 4 + 1] = lut[131 * 4 + 1] = lut[163 * 4 + 1];
	
	lut[6 * 4 + 1] = lut[38 * 4 + 1] = lut[134 * 4 + 1] = lut[166 * 4 + 1];
	
	lut[20 * 4 + 1] = lut[21 * 4 + 1] = lut[52 * 4 + 1] = lut[53 * 4 + 1];
	
	lut[144 * 4 + 1] = lut[145 * 4 + 1] = lut[176 * 4 + 1] = lut[177 * 4 + 1];
	
	lut[192 * 4 + 1] = lut[193 * 4 + 1] = lut[196 * 4 + 1] = lut[197 * 4 + 1];
	
	lut[96 * 4 + 1] = lut[97 * 4 + 1] = lut[100 * 4 + 1] = lut[101 * 4 + 1];
	
	lut[40 * 4 + 1] = lut[44 * 4 + 1] = lut[168 * 4 + 1] = lut[172 * 4 + 1];
	
	lut[9 * 4 + 1] = lut[13 * 4 + 1] = lut[137 * 4 + 1] = lut[141 * 4 + 1];
	
	lut[18 * 4 + 1] = lut[50 * 4 + 1];
	
	lut[80 * 4 + 1] = lut[81 * 4 + 1];
	
	lut[72 * 4 + 1] = lut[76 * 4 + 1];
	
	lut[10 * 4 + 1] = lut[138 * 4 + 1];
	
	lut[7 * 4 + 1] = lut[39 * 4 + 1] = lut[135 * 4 + 1];
	
	lut[148 * 4 + 1] = lut[149 * 4 + 1] = lut[180 * 4 + 1];
	
	lut[224 * 4 + 1] = lut[228 * 4 + 1] = lut[225 * 4 + 1];
	
	lut[41 * 4 + 1] = lut[169 * 4 + 1] = lut[45 * 4 + 1];
	
	lut[22 * 4 + 1] = lut[54 * 4 + 1];
	
	lut[208 * 4 + 1] = lut[209 * 4 + 1];
	
	lut[104 * 4 + 1] = lut[108 * 4 + 1];
	
	lut[11 * 4 + 1] = lut[139 * 4 + 1];
	
	lut[19 * 4 + 1] = lut[51 * 4 + 1];
	
	lut[146 * 4 + 1] = lut[178 * 4 + 1];
	
	lut[84 * 4 + 1] = lut[85 * 4 + 1];
	
	lut[112 * 4 + 1] = lut[113 * 4 + 1];
	
	lut[200 * 4 + 1] = lut[204 * 4 + 1];
	
	lut[73 * 4 + 1] = lut[77 * 4 + 1];
	
	lut[42 * 4 + 1] = lut[170 * 4 + 1];
	
	lut[14 * 4 + 1] = lut[142 * 4 + 1];
	
	lut[26 * 4 + 1] = lut[31 * 4 + 1];
	
	lut[82 * 4 + 1] = lut[214 * 4 + 1];
	
	lut[88 * 4 + 1] = lut[248 * 4 + 1];
	
	lut[74 * 4 + 1] = lut[107 * 4 + 1];
	
	lut[55 * 4 + 1] = lut[23 * 4 + 1];
	
	lut[182 * 4 + 1] = lut[150 * 4 + 1];
	
	lut[213 * 4 + 1] = lut[212 * 4 + 1];
	
	lut[241 * 4 + 1] = lut[240 * 4 + 1];
	
	lut[236 * 4 + 1] = lut[232 * 4 + 1];
	
	lut[109 * 4 + 1] = lut[105 * 4 + 1];
	
	lut[171 * 4 + 1] = lut[43 * 4 + 1];
	
	lut[143 * 4 + 1] = lut[15 * 4 + 1];
	
	lut[205 * 4 + 1] = lut[201 * 4 + 1];
	
	lut[174 * 4 + 1] = lut[46 * 4 + 1];
	
	lut[179 * 4 + 1] = lut[147 * 4 + 1];
	
	lut[117 * 4 + 1] = lut[116 * 4 + 1];
	
	lut[237 * 4 + 1] = lut[233 * 4 + 1];
	
	lut[175 * 4 + 1] = lut[47 * 4 + 1];
	
	lut[183 * 4 + 1] = lut[151 * 4 + 1];
	
	lut[245 * 4 + 1] = lut[244 * 4 + 1];
	
	
	lut[0 * 4 + 2] = lut[1 * 4 + 2] = lut[4 * 4 + 2] = lut[32 * 4 + 2] = lut[128 * 4 + 2] = lut[5 * 4 + 2] = lut[132 * 4 + 2] = lut[160 * 4 + 2] = lut[33 * 4 + 2] = lut[129 * 4 + 2] = lut[36 * 4 + 2] = lut[133 * 4 + 2] = lut[164 * 4 + 2] = lut[161 * 4 + 2] = lut[37 * 4 + 2] = lut[165 * 4 + 2];
	
	lut[2 * 4 + 2] = lut[34 * 4 + 2] = lut[130 * 4 + 2] = lut[162 * 4 + 2];
	
	lut[16 * 4 + 2] = lut[17 * 4 + 2] = lut[48 * 4 + 2] = lut[49 * 4 + 2];
	
	lut[64 * 4 + 2] = lut[65 * 4 + 2] = lut[68 * 4 + 2] = lut[69 * 4 + 2];
	
	lut[8 * 4 + 2] = lut[12 * 4 + 2] = lut[136 * 4 + 2] = lut[140 * 4 + 2];
	
	lut[3 * 4 + 2] = lut[35 * 4 + 2] = lut[131 * 4 + 2] = lut[163 * 4 + 2];
	
	lut[6 * 4 + 2] = lut[38 * 4 + 2] = lut[134 * 4 + 2] = lut[166 * 4 + 2];
	
	lut[20 * 4 + 2] = lut[21 * 4 + 2] = lut[52 * 4 + 2] = lut[53 * 4 + 2];
	
	lut[144 * 4 + 2] = lut[145 * 4 + 2] = lut[176 * 4 + 2] = lut[177 * 4 + 2];
	
	lut[192 * 4 + 2] = lut[193 * 4 + 2] = lut[196 * 4 + 2] = lut[197 * 4 + 2];
	
	lut[96 * 4 + 2] = lut[97 * 4 + 2] = lut[100 * 4 + 2] = lut[101 * 4 + 2];
	
	lut[40 * 4 + 2] = lut[44 * 4 + 2] = lut[168 * 4 + 2] = lut[172 * 4 + 2];
	
	lut[9 * 4 + 2] = lut[13 * 4 + 2] = lut[137 * 4 + 2] = lut[141 * 4 + 2];
	
	lut[18 * 4 + 2] = lut[50 * 4 + 2];
	
	lut[80 * 4 + 2] = lut[81 * 4 + 2];
	
	lut[72 * 4 + 2] = lut[76 * 4 + 2];
	
	lut[10 * 4 + 2] = lut[138 * 4 + 2];
	
	lut[7 * 4 + 2] = lut[39 * 4 + 2] = lut[135 * 4 + 2];
	
	lut[148 * 4 + 2] = lut[149 * 4 + 2] = lut[180 * 4 + 2];
	
	lut[224 * 4 + 2] = lut[228 * 4 + 2] = lut[225 * 4 + 2];
	
	lut[41 * 4 + 2] = lut[169 * 4 + 2] = lut[45 * 4 + 2];
	
	lut[22 * 4 + 2] = lut[54 * 4 + 2];
	
	lut[208 * 4 + 2] = lut[209 * 4 + 2];
	
	lut[104 * 4 + 2] = lut[108 * 4 + 2];
	
	lut[11 * 4 + 2] = lut[139 * 4 + 2];
	
	lut[19 * 4 + 2] = lut[51 * 4 + 2];
	
	lut[146 * 4 + 2] = lut[178 * 4 + 2];
	
	lut[84 * 4 + 2] = lut[85 * 4 + 2];
	
	lut[112 * 4 + 2] = lut[113 * 4 + 2];
	
	lut[200 * 4 + 2] = lut[204 * 4 + 2];
	
	lut[73 * 4 + 2] = lut[77 * 4 + 2];
	
	lut[42 * 4 + 2] = lut[170 * 4 + 2];
	
	lut[14 * 4 + 2] = lut[142 * 4 + 2];
	
	lut[26 * 4 + 2] = lut[31 * 4 + 2];
	
	lut[82 * 4 + 2] = lut[214 * 4 + 2];
	
	lut[88 * 4 + 2] = lut[248 * 4 + 2];
	
	lut[74 * 4 + 2] = lut[107 * 4 + 2];
	
	lut[55 * 4 + 2] = lut[23 * 4 + 2];
	
	lut[182 * 4 + 2] = lut[150 * 4 + 2];
	
	lut[213 * 4 + 2] = lut[212 * 4 + 2];
	
	lut[241 * 4 + 2] = lut[240 * 4 + 2];
	
	lut[236 * 4 + 2] = lut[232 * 4 + 2];
	
	lut[109 * 4 + 2] = lut[105 * 4 + 2];
	
	lut[171 * 4 + 2] = lut[43 * 4 + 2];
	
	lut[143 * 4 + 2] = lut[15 * 4 + 2];
	
	lut[205 * 4 + 2] = lut[201 * 4 + 2];
	
	lut[174 * 4 + 2] = lut[46 * 4 + 2];
	
	lut[179 * 4 + 2] = lut[147 * 4 + 2];
	
	lut[117 * 4 + 2] = lut[116 * 4 + 2];
	
	lut[237 * 4 + 2] = lut[233 * 4 + 2];
	
	lut[175 * 4 + 2] = lut[47 * 4 + 2];
	
	lut[183 * 4 + 2] = lut[151 * 4 + 2];
	
	lut[245 * 4 + 2] = lut[244 * 4 + 2];
	
	
	lut[0 * 4 + 3] = lut[1 * 4 + 3] = lut[4 * 4 + 3] = lut[32 * 4 + 3] = lut[128 * 4 + 3] = lut[5 * 4 + 3] = lut[132 * 4 + 3] = lut[160 * 4 + 3] = lut[33 * 4 + 3] = lut[129 * 4 + 3] = lut[36 * 4 + 3] = lut[133 * 4 + 3] = lut[164 * 4 + 3] = lut[161 * 4 + 3] = lut[37 * 4 + 3] = lut[165 * 4 + 3];
	
	lut[2 * 4 + 3] = lut[34 * 4 + 3] = lut[130 * 4 + 3] = lut[162 * 4 + 3];
	
	lut[16 * 4 + 3] = lut[17 * 4 + 3] = lut[48 * 4 + 3] = lut[49 * 4 + 3];
	
	lut[64 * 4 + 3] = lut[65 * 4 + 3] = lut[68 * 4 + 3] = lut[69 * 4 + 3];
	
	lut[8 * 4 + 3] = lut[12 * 4 + 3] = lut[136 * 4 + 3] = lut[140 * 4 + 3];
	
	lut[3 * 4 + 3] = lut[35 * 4 + 3] = lut[131 * 4 + 3] = lut[163 * 4 + 3];
	
	lut[6 * 4 + 3] = lut[38 * 4 + 3] = lut[134 * 4 + 3] = lut[166 * 4 + 3];
	
	lut[20 * 4 + 3] = lut[21 * 4 + 3] = lut[52 * 4 + 3] = lut[53 * 4 + 3];
	
	lut[144 * 4 + 3] = lut[145 * 4 + 3] = lut[176 * 4 + 3] = lut[177 * 4 + 3];
	
	lut[192 * 4 + 3] = lut[193 * 4 + 3] = lut[196 * 4 + 3] = lut[197 * 4 + 3];
	
	lut[96 * 4 + 3] = lut[97 * 4 + 3] = lut[100 * 4 + 3] = lut[101 * 4 + 3];
	
	lut[40 * 4 + 3] = lut[44 * 4 + 3] = lut[168 * 4 + 3] = lut[172 * 4 + 3];
	
	lut[9 * 4 + 3] = lut[13 * 4 + 3] = lut[137 * 4 + 3] = lut[141 * 4 + 3];
	
	lut[18 * 4 + 3] = lut[50 * 4 + 3];
	
	lut[80 * 4 + 3] = lut[81 * 4 + 3];
	
	lut[72 * 4 + 3] = lut[76 * 4 + 3];
	
	lut[10 * 4 + 3] = lut[138 * 4 + 3];
	
	lut[7 * 4 + 3] = lut[39 * 4 + 3] = lut[135 * 4 + 3];
	
	lut[148 * 4 + 3] = lut[149 * 4 + 3] = lut[180 * 4 + 3];
	
	lut[224 * 4 + 3] = lut[228 * 4 + 3] = lut[225 * 4 + 3];
	
	lut[41 * 4 + 3] = lut[169 * 4 + 3] = lut[45 * 4 + 3];
	
	lut[22 * 4 + 3] = lut[54 * 4 + 3];
	
	lut[208 * 4 + 3] = lut[209 * 4 + 3];
	
	lut[104 * 4 + 3] = lut[108 * 4 + 3];
	
	lut[11 * 4 + 3] = lut[139 * 4 + 3];
	
	lut[19 * 4 + 3] = lut[51 * 4 + 3];
	
	lut[146 * 4 + 3] = lut[178 * 4 + 3];
	
	lut[84 * 4 + 3] = lut[85 * 4 + 3];
	
	lut[112 * 4 + 3] = lut[113 * 4 + 3];
	
	lut[200 * 4 + 3] = lut[204 * 4 + 3];
	
	lut[73 * 4 + 3] = lut[77 * 4 + 3];
	
	lut[42 * 4 + 3] = lut[170 * 4 + 3];
	
	lut[14 * 4 + 3] = lut[142 * 4 + 3];
	
	lut[26 * 4 + 3] = lut[31 * 4 + 3];
	
	lut[82 * 4 + 3] = lut[214 * 4 + 3];
	
	lut[88 * 4 + 3] = lut[248 * 4 + 3];
	
	lut[74 * 4 + 3] = lut[107 * 4 + 3];
	
	lut[55 * 4 + 3] = lut[23 * 4 + 3];
	
	lut[182 * 4 + 3] = lut[150 * 4 + 3];
	
	lut[213 * 4 + 3] = lut[212 * 4 + 3];
	
	lut[241 * 4 + 3] = lut[240 * 4 + 3];
	
	lut[236 * 4 + 3] = lut[232 * 4 + 3];
	
	lut[109 * 4 + 3] = lut[105 * 4 + 3];
	
	lut[171 * 4 + 3] = lut[43 * 4 + 3];
	
	lut[143 * 4 + 3] = lut[15 * 4 + 3];
	
	lut[205 * 4 + 3] = lut[201 * 4 + 3];
	
	lut[174 * 4 + 3] = lut[46 * 4 + 3];
	
	lut[179 * 4 + 3] = lut[147 * 4 + 3];
	
	lut[117 * 4 + 3] = lut[116 * 4 + 3];
	
	lut[237 * 4 + 3] = lut[233 * 4 + 3];
	
	lut[175 * 4 + 3] = lut[47 * 4 + 3];
	
	lut[183 * 4 + 3] = lut[151 * 4 + 3];
	
	lut[245 * 4 + 3] = lut[244 * 4 + 3];
}

// void hq2x_32( const unsigned char * pIn, unsigned char * pOut, int Xres, int Yres, int BpL ) {
template<class PixelPutter>
static void filter(typename PixelPutter::pixel_t *pOut, const unsigned dstPitch, PixelPutter putPixel,
                   const uint32_t *pIn, const unsigned Xres, const unsigned Yres)
{
	//   +----+----+----+
	//   |    |    |    |
	//   | w1 | w2 | w3 |
	//   +----+----+----+
	//   |    |    |    |
	//   | w4 | w5 | w6 |
	//   +----+----+----+
	//   |    |    |    |
	//   | w7 | w8 | w9 |
	//   +----+----+----+
	
	for (unsigned j = 0; j < Yres; j++) {
		int  prevline, nextline;
		
		if (j > 0)      prevline = -Xres;
		else prevline = 0;
		if (j < Yres - 1) nextline = Xres;
		else nextline = 0;
		
		unsigned  w[10];
		
		w[3] = w[2] = *(pIn + prevline);
		w[6] = w[5] = *(pIn);
		w[9] = w[8] = *(pIn + nextline);
		
		for (unsigned i = 0; i < Xres; i++) {
			w[1] = w[2];
			w[4] = w[5];
			w[7] = w[8];
			
			w[2] = w[3];
			w[5] = w[6];
			w[8] = w[9];
			
			if (i < Xres - 1) {
				w[3] = *(pIn + prevline + 1);
				w[6] = *(pIn + 1);
				w[9] = *(pIn + nextline + 1);
			}
			
			unsigned pattern = 0;
			
			{
				unsigned flag = 1;
				
				const unsigned r1 = w[5] >> 16;
				const unsigned g1 = w[5] >> 8 & 0xFF;
				const unsigned b1 = w[5] & 0xFF;
				
				for (unsigned k = 1; k < 10; ++k) {
					if (k == 5) continue;
					
					if (w[k] != w[5]) {
						const unsigned rdiff = r1 - (w[k] >> 16);
						const unsigned gdiff = g1 - (w[k] >> 8 & 0xFF);
						const unsigned bdiff = b1 - (w[k] & 0xFF);
						
						if (rdiff + gdiff + bdiff + 0xC0U > 0xC0U * 2  ||
						    rdiff - bdiff + 0x1CU > 0x1CU * 2          ||
						    gdiff * 2 - rdiff - bdiff + 0x30U > 0x30U * 2)
							pattern |= flag;
					}
					
					flag <<= 1;
				}
			}
			
			{
// 				printf("pattern: %u\n", pattern);
				pixelop* op = lut + pattern * 4;
				
				putPixel(pOut, (*op++)(w));
				putPixel(pOut + 1, (*op++)(w));
				putPixel(pOut + dstPitch, (*op++)(w));
				putPixel(pOut + dstPitch + 1, (*op)(w));
			}
			
			++pIn;
			pOut += 2;
		}
		pOut += dstPitch;
	}
}

MaxSt_Hq2x::MaxSt_Hq2x() {
	buffer = NULL;
	buildLut();
}

MaxSt_Hq2x::~MaxSt_Hq2x() {
	delete []buffer;
}

void MaxSt_Hq2x::init() {
	delete []buffer;
	buffer = new uint32_t[144 * 160];
}

void MaxSt_Hq2x::outit() {
	delete []buffer;
	buffer = NULL;
}

const FilterInfo& MaxSt_Hq2x::info() {
	static FilterInfo fInfo = { "MaxSt's Hq2x", 160 * 2, 144 * 2 };
	return fInfo;
}

uint32_t* MaxSt_Hq2x::inBuffer() {
	return buffer;
}

unsigned MaxSt_Hq2x::inPitch() {
	return 160;
}

void MaxSt_Hq2x::filter(Rgb32Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb32Putter putPixel) {
	::filter(dbuffer, pitch, putPixel, buffer, 160, 144);
}

void MaxSt_Hq2x::filter(Rgb16Putter::pixel_t *const dbuffer, const unsigned pitch, Rgb16Putter putPixel) {
	::filter(dbuffer, pitch, putPixel, buffer, 160, 144);
}

void MaxSt_Hq2x::filter(UyvyPutter::pixel_t *const dbuffer, const unsigned pitch, UyvyPutter putPixel) {
	::filter(dbuffer, pitch, putPixel, buffer, 160, 144);
}
