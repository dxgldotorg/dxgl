// DXGL
// Copyright (C) 2011-2018 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include "common.h"
#include "MultiDD.h"
#include "tests.h"
#include "surfacegen.h"

#ifndef GRADIENT_FILL_RECT_H
#define GRADIENT_FILL_RECT_H 0
#endif

inline int NextMultipleOf4(int number){return ((number+3) & (~3));}

const unsigned char dxgl_logo_bw[] = {
0x82,0x21,0x41,0x42,0x20,0x84,0x11,0x48,0x0a,0x30,0xe4,0x2e,0x92,0x50,0x91,
0x90,0x91,0x97,0x92,0x51,0xe4,0x2e,0x09,0x10,0x11,0x08,0x21,0x04,0x41,0x02,
0x81,0xe1
};

const unsigned char blt_pattern_8[] = {
	0xED, 0xED, 0xED, 0xF8, 0xF8, 0xF8,
	0xED, 0xED, 0xED, 0xF8, 0xF8, 0xF8,
	0xED, 0xED, 0xED, 0xF8, 0xF8, 0xF8,
	0xF8, 0xF8, 0xF8, 0xED, 0xED, 0xED,
	0xF8, 0xF8, 0xF8, 0xED, 0xED, 0xED,
	0xF8, 0xF8, 0xF8, 0xED, 0xED, 0xED
};

const unsigned short blt_pattern_15[] = {
	0x7DFF, 0x7DFF, 0x7DFF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x7DFF, 0x7DFF, 0x7DFF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x7DFF, 0x7DFF, 0x7DFF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x3DEF, 0x3DEF, 0x3DEF, 0x7DFF, 0x7DFF, 0x7DFF,
	0x3DEF, 0x3DEF, 0x3DEF, 0x7DFF, 0x7DFF, 0x7DFF,
	0x3DEF, 0x3DEF, 0x3DEF, 0x7DFF, 0x7DFF, 0x7DFF
};

const unsigned short blt_pattern_16[] = {
	0xFBFF, 0xFBFF, 0xFBFF, 0x7BEF, 0x7BEF, 0x7BEF,
	0xFBFF, 0xFBFF, 0xFBFF, 0x7BEF, 0x7BEF, 0x7BEF,
	0xFBFF, 0xFBFF, 0xFBFF, 0x7BEF, 0x7BEF, 0x7BEF,
	0x7BEF, 0x7BEF, 0x7BEF, 0xFBFF, 0xFBFF, 0xFBFF,
	0x7BEF, 0x7BEF, 0x7BEF, 0xFBFF, 0xFBFF, 0xFBFF,
	0x7BEF, 0x7BEF, 0x7BEF, 0xFBFF, 0xFBFF, 0xFBFF
};

const unsigned char blt_pattern_24[] = {
	0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0x7F, 0xFF
};

const unsigned long blt_pattern_32[] = {
	0xFF7FFF, 0xFF7FFF, 0xFF7FFF, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0xFF7FFF, 0xFF7FFF, 0xFF7FFF, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0xFF7FFF, 0xFF7FFF, 0xFF7FFF, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xFF7FFF, 0xFF7FFF, 0xFF7FFF,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xFF7FFF, 0xFF7FFF, 0xFF7FFF,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xFF7FFF, 0xFF7FFF, 0xFF7FFF
};

const unsigned char back_pattern_8[] = {
	0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07,
	0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07,
	0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07,
	0xF8, 0xF8, 0xF8, 0xF8, 0x07, 0x07, 0x07, 0x07,
	0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0xF8, 0xF8,
	0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0xF8, 0xF8,
	0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0xF8, 0xF8,
	0x07, 0x07, 0x07, 0x07, 0xF8, 0xF8, 0xF8, 0xF8
};

const unsigned short back_pattern_15[] = {
	0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF, 0x6318, 0x6318, 0x6318, 0x6318,
	0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF, 0x6318, 0x6318, 0x6318, 0x6318,
	0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF, 0x6318, 0x6318, 0x6318, 0x6318,
	0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF, 0x6318, 0x6318, 0x6318, 0x6318,
	0x6318, 0x6318, 0x6318, 0x6318, 0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x6318, 0x6318, 0x6318, 0x6318, 0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x6318, 0x6318, 0x6318, 0x6318, 0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF,
	0x6318, 0x6318, 0x6318, 0x6318, 0x3DEF, 0x3DEF, 0x3DEF, 0x3DEF
};

const unsigned short back_pattern_16[] = {
	0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF, 0xC618, 0xC618, 0xC618, 0xC618,
	0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF, 0xC618, 0xC618, 0xC618, 0xC618,
	0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF, 0xC618, 0xC618, 0xC618, 0xC618,
	0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF, 0xC618, 0xC618, 0xC618, 0xC618,
	0xC618, 0xC618, 0xC618, 0xC618, 0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF,
	0xC618, 0xC618, 0xC618, 0xC618, 0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF,
	0xC618, 0xC618, 0xC618, 0xC618, 0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF,
	0xC618, 0xC618, 0xC618, 0xC618, 0x7BEF, 0x7BEF, 0x7BEF, 0x7BEF
};

const unsigned short back_pattern_24[] = {
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
	0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F
};

const unsigned long back_pattern_32[] = {
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F,
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F, 0x7F7F7F
};

static const DWORD rop_codes[256] = {
	0x00000042, 0x00010289, 0x00020C89, 0x000300AA, 0x00040C88, 0x000500A9, 0x00060865, 0x000702C5,
	0x00080F08, 0x00090245, 0x000A0329, 0x000B0B2A, 0x000C0324, 0x000D0B25, 0x000E08A5, 0x000F0001,
	0x00100C85, 0x001100A6, 0x00120868, 0x001302C8, 0x00140869, 0x001502C9, 0x00165CCA, 0x00171D54,
	0x00180D59, 0x00191CC8, 0x001A06C5, 0x001B0768, 0x001C06CA, 0x001D0766, 0x001E01A5, 0x001F0385,
	0x00200F09, 0x00210248, 0x00220326, 0x00230B24, 0x00240D55, 0x00251CC5, 0x002606C8, 0x00271868,
	0x00280369, 0x002916CA, 0x002A0CC9, 0x002B1D58, 0x002C0784, 0x002D060A, 0x002E064A, 0x002F0E2A,
	0x0030032A, 0x00310B28, 0x00320688, 0x00330008, 0x003406C4, 0x00351864, 0x003601A8, 0x00370388,
	0x0038078A, 0x00390604, 0x003A0644, 0x003B0E24, 0x003C004A, 0x003D18A4, 0x003E1B24, 0x003F00EA,
	0x00400F0A, 0x00410249, 0x00420D5D, 0x00431CC4, 0x00440328, 0x00450B29, 0x004606C6, 0x0047076A,
	0x00480368, 0x004916C5, 0x004A0789, 0x004B0605, 0x004C0CC8, 0x004D1954, 0x004E0645, 0x004F0E25,
	0x00500325, 0x00510B26, 0x005206C9, 0x00530764, 0x005408A9, 0x00550009, 0x005601A9, 0x00570389,
	0x00580785, 0x00590609, 0x005A0049, 0x005B18A9, 0x005C0649, 0x005D0E29, 0x005E1B29, 0x005F00E9,
	0x00600365, 0x006116C6, 0x00620786, 0x00630608, 0x00640788, 0x00650606, 0x00660046, 0x006718A8,
	0x006858A6, 0x00690145, 0x006A01E9, 0x006B178A, 0x006C01E8, 0x006D1785, 0x006E1E28, 0x006F0C65,
	0x00700CC5, 0x00711D5C, 0x00720648, 0x00730E28, 0x00740646, 0x00750E26, 0x00761B28, 0x007700E6,
	0x007801E5, 0x00791786, 0x007A1E29, 0x007B0C68, 0x007C1E24, 0x007D0C69, 0x007E0955, 0x007F03C9,
	0x008003E9, 0x00810975, 0x00820C49, 0x00831E04, 0x00840C48, 0x00851E05, 0x008617A6, 0x008701C5,
	0x008800C6, 0x00891B08, 0x008A0E06, 0x008B0666, 0x008C0E08, 0x008D0668, 0x008E1D7C, 0x008F0CE5,
	0x00900C45, 0x00911E08, 0x009217A9, 0x009301C4, 0x009417AA, 0x009501C9, 0x00960169, 0x0097588A,
	0x00981888, 0x00990066, 0x009A0709, 0x009B07A8, 0x009C0704, 0x009D07A6, 0x009E16E6, 0x009F0345,
	0x00A000C9, 0x00A11B05, 0x00A20E09, 0x00A30669, 0x00A41885, 0x00A50065, 0x00A60706, 0x00A707A5,
	0x00A803A9, 0x00A90189, 0x00AA0029, 0x00AB0889, 0x00AC0744, 0x00AD06E9, 0x00AE0B06, 0x00AF0229,
	0x00B00E05, 0x00B10665, 0x00B21974, 0x00B30CE8, 0x00B4070A, 0x00B507A9, 0x00B616E9, 0x00B70348,
	0x00B8074A, 0x00B906E6, 0x00BA0B09, 0x00BB0226, 0x00BC1CE4, 0x00BD0D7D, 0x00BE0269, 0x00BF08C9,
	0x00C000CA, 0x00C11B04, 0x00C21884, 0x00C3006A, 0x00C40E04, 0x00C50664, 0x00C60708, 0x00C707AA,
	0x00C803A8, 0x00C90184, 0x00CA0749, 0x00CB06E4, 0x00CC0020, 0x00CD0888, 0x00CE0B08, 0x00CF0224,
	0x00D00E0A, 0x00D1066A, 0x00D20705, 0x00D307A4, 0x00D41D78, 0x00D50CE9, 0x00D616EA, 0x00D70349,
	0x00D80745, 0x00D906E8, 0x00DA1CE9, 0x00DB0D75, 0x00DC0B04, 0x00DD0228, 0x00DE0268, 0x00DF08C8,
	0x00E003A5, 0x00E10185, 0x00E20746, 0x00E306EA, 0x00E40748, 0x00E506E5, 0x00E61CE8, 0x00E70D79,
	0x00E81D74, 0x00E95CE6, 0x00EA02E9, 0x00EB0849, 0x00EC02E8, 0x00ED0848, 0x00EE0086, 0x00EF0A08,
	0x00F00021, 0x00F10885, 0x00F20B05, 0x00F3022A, 0x00F40B0A, 0x00F50225, 0x00F60265, 0x00F708C5,
	0x00F802E5, 0x00F90845, 0x00FA0089, 0x00FB0A09, 0x00FC008A, 0x00FD0A0A, 0x00FE02A9, 0x00FF0062
};

/*
1 - Source
2 - Dest
4 - Pattern */
const DWORD rop_texture_usage[256] = {
	0, 7, 7, 5, 7, 6, 7, 7, 7, 7, 6, 7, 5, 7, 7, 4,
	7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	5, 7, 7, 1, 7, 7, 7, 7, 7, 7, 7, 7, 5, 7, 7, 5,
	7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	6, 7, 7, 7, 7, 2, 7, 7, 7, 7, 6, 7, 7, 7, 7, 6,
	7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7,
	6, 7, 7, 7, 7, 6, 7, 7, 7, 7, 2, 7, 7, 7, 7, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7,
	5, 7, 7, 5, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7, 7, 5,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7,
	4, 7, 7, 5, 7, 6, 7, 7, 7, 7, 6, 7, 5, 7, 7, 0
};

BOOL IsRopCodeSupported(DWORD rop, DWORD *ropcaps)
{
	DWORD rop32 = rop & 0xFFFFFF;
	DWORD ropindex = (rop >> 16) & 0xFF;
	DWORD ropword = ropindex >> 5;
	DWORD ropbit = ropindex & 0x1F;
	if (rop_codes[ropindex] != rop32) return FALSE;
	if ((ropcaps[ropword] >> ropbit) & 1)
	{
		return TRUE;
	}
	else return FALSE;
}

void DrawPalette(DDSURFACEDESC2 ddsd, unsigned char *buffer)  // Palette test
{
	DWORD x,y;
	DWORD color;
	unsigned short *buffer16 = (unsigned short*) buffer;
	unsigned long *buffer32 = (unsigned long*) buffer;
	switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
		case 8:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth; x++)
				{
					buffer[x+(ddsd.lPitch*y)] = (unsigned char)((x/(ddsd.dwWidth/16.)) + 16*floor((y/(ddsd.dwHeight/16.))));
				}
			}
			break;
		case 16:
			if((ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
				ddsd.ddpfPixelFormat.dwBBitMask) == 0x7FFF)
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.))))/2;
					}
				}
			}
			else if((ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
				ddsd.ddpfPixelFormat.dwBBitMask) == 0xFFFF)
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/256.)) + 256*floor((y/(ddsd.dwHeight/256.))));
					}
				}
			}
			else if ((ddsd.ddpfPixelFormat.dwRBitMask | ddsd.ddpfPixelFormat.dwGBitMask |
				ddsd.ddpfPixelFormat.dwBBitMask) == 0xFF)
			{
				for (y = 0; y < ddsd.dwHeight; y++)
				{
					for (x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x + ((ddsd.lPitch/2)*y)] = (unsigned short)((x / (ddsd.dwWidth / 16.)) + 16 * floor((y / (ddsd.dwHeight / 16.))));
					}
				}
			}
			else
			{
				for(y = 0; y < ddsd.dwHeight; y++)
				{
					for(x = 0; x < ddsd.dwWidth; x++)
					{
						buffer16[x+((ddsd.lPitch/2)*y)] = (unsigned short)((x/(ddsd.dwWidth/64.)) + 64*floor((y/(ddsd.dwHeight/64.))));
					}
				}
			}
			break;
		case 24:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth*3; x+=3)
				{
					color = (DWORD)(((x/3)/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.))));
					buffer[x+(ddsd.lPitch*y)] = color & 0xFF;
					buffer[(x+1)+(ddsd.lPitch*y)] = (color >> 8) & 0xFF;
					buffer[(x+2)+(ddsd.lPitch*y)] = (color >> 16) & 0xFF;
				}
			}
			return;
		case 32:
			for(y = 0; y < ddsd.dwHeight; y++)
			{
				for(x = 0; x < ddsd.dwWidth; x++)
				{
					buffer32[x+((ddsd.lPitch/4)*y)] = (unsigned long)((x/(ddsd.dwWidth/4096.)) + 4096*floor((y/(ddsd.dwHeight/4096.))));
				}
			}
			return;
		default:
			return;
	}
}

void DrawDitheredColor(DDSURFACEDESC2 *ddsd, unsigned char *buffer, DWORD color, BOOL invert)
{
	DWORD *dwordptr = (DWORD*)buffer;
	WORD *wordptr = (WORD*)buffer;
	DWORD ptr;
	BOOL pixel;
	DWORD x, y;
	switch (ddsd->ddpfPixelFormat.dwRGBBitCount)
	{
	default:
	case 8:
		for (y = 0; y < ddsd->dwHeight; y++)
		{
			ptr = ddsd->lPitch * y;
			if (y & 1) pixel = TRUE;
			else pixel = FALSE;
			if (invert) pixel ^= 1;
			for (x = 0; x < ddsd->dwWidth; x++)
			{
				if (pixel) buffer[ptr] = (unsigned char)color;
				pixel ^= 1;
				ptr++;
			}
		}
		break;
	case 15:
	case 16:
		for (y = 0; y < ddsd->dwHeight; y++)
		{
			ptr = (ddsd->lPitch / 2) * y;
			if (y & 1) pixel = TRUE;
			else pixel = FALSE;
			if (invert) pixel ^= 1;
			for (x = 0; x < ddsd->dwWidth; x++)
			{
				if (pixel) wordptr[ptr] = (WORD)color;
				pixel ^= 1;
				ptr++;
			}
		}
		break;
	case 24:
		for (y = 0; y < ddsd->dwHeight; y++)
		{
			ptr = ddsd->lPitch * y;
			if (y & 1) pixel = TRUE;
			else pixel = FALSE;
			if (invert) pixel ^= 1;
			for (x = 0; x < ddsd->dwWidth; x++)
			{
				if (pixel) buffer[ptr] = (unsigned char)(color & 255);
				ptr++;
				if (pixel) buffer[ptr] = (unsigned char)((color >> 8) & 255);
				ptr++;
				if (pixel) buffer[ptr] = (unsigned char)((color >> 16) & 255);
				pixel ^= 1;
				ptr++;
			}
		}
		break;
	case 32:
		for (y = 0; y < ddsd->dwHeight; y++)
		{
			ptr = (ddsd->lPitch / 4) * y;
			if (y & 1) pixel = TRUE;
			else pixel = FALSE;
			if (invert) pixel ^= 1;
			for (x = 0; x < ddsd->dwWidth; x++)
			{
				if (pixel) dwordptr[ptr] = (DWORD)color;
				pixel ^= 1;
				ptr++;
			}
		}
		break;
	}
}

void DrawGradient(HDC hdc, int left, int right, int top, int bottom, DWORD color, bool usegdi)
{
	int x;
	int r,g,b;
	RECT rect;
	HBRUSH brushcolor;
	TRIVERTEX vertex[2];
	GRADIENT_RECT grect;
	if(usegdi && gradientavailable)
	{
		vertex[0].x = left;
		vertex[1].x = right;
		vertex[0].y = top;
		vertex[1].y = bottom;
		vertex[0].Red = vertex[0].Green = vertex[0].Blue = 0;
		vertex[1].Red = (color & 0xff) << 8;
		vertex[1].Green = ((color >> 8) & 0xff) << 8;
		vertex[1].Blue = ((color >> 16) & 0xff) << 8;
		grect.UpperLeft = 0;
		grect.LowerRight = 1;
		_GradientFill(hdc,vertex,2,&grect,1,GRADIENT_FILL_RECT_H);
	}
	else
	{
		for(x = left; x < right; x++)
		{
			r = (x*(color & 0xff)) / (right-left);
			g = (x*((color >> 8) & 0xff)) / (right-left);
			b = (x*((color >> 16) & 0xff)) / (right-left);
			rect.left = x;
			rect.right = x+1;
			rect.top = top;
			rect.bottom = bottom;
			brushcolor = CreateSolidBrush(RGB(r,g,b));
			FillRect(hdc,&rect,brushcolor);
			DeleteObject(brushcolor);
		}
	}
}

void DrawGradients(DDSURFACEDESC2 ddsd, unsigned char *buffer, HWND hwnd, LPDIRECTDRAWPALETTE palette, int type, DWORD color) // Gradients
{
	HDC hdcwin = GetDC(hwnd);
	DWORD colors[256];
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*) malloc(sizeof(BITMAPINFOHEADER) + 1024);
	if(!bmi) return;
	ZeroMemory(bmi,sizeof(BITMAPINFOHEADER) + 1024);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		palette->GetEntries(0,0,256,(LPPALETTEENTRY)colors);
		for(int i = 0; i < 256; i++)
			colors[i] = ((colors[i]&0x0000FF)<<16) | (colors[i]&0x00FF00) | ((colors[i]&0xFF0000)>>16);
		memcpy(bmi->bmiColors,colors,1024);
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		{
			bitmode = BI_BITFIELDS;
			bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
			bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
			bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(bmi->bmiColors,bitmasks,3*sizeof(DWORD));
		}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = ddsd.lPitch / (ddsd.ddpfPixelFormat.dwRGBBitCount / 8);
	bmi->bmiHeader.biHeight = 0-ddsd.dwHeight;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bitmap = CreateDIBSection(hdcwin,bmi,DIB_RGB_COLORS,(void**)&bits,NULL,0);
	HDC hdcmem = CreateCompatibleDC(hdcwin);
	HGDIOBJ hbmold = SelectObject(hdcmem,bitmap);
	SetBkMode(hdcmem,TRANSPARENT);
	switch(type)
	{
	case 1:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight / 7,0x0000FF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,ddsd.dwHeight / 7, 2*(ddsd.dwHeight/7),0x00FF00,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,2*(ddsd.dwHeight/7),3*(ddsd.dwHeight/7),0xFF0000,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,3*(ddsd.dwHeight/7),4*(ddsd.dwHeight/7),0xFFFF00,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,4*(ddsd.dwHeight/7),5*(ddsd.dwHeight/7),0xFF00FF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,5*(ddsd.dwHeight/7),6*(ddsd.dwHeight/7),0x00FFFF,false);
		DrawGradient(hdcmem,0,ddsd.dwWidth,6*(ddsd.dwHeight/7),ddsd.dwHeight,0xFFFFFF,false);
		break;
	default:
		DrawGradient(hdcmem,0,ddsd.dwWidth,0,ddsd.dwHeight,color,false);
	}
	for(unsigned int i = 0; i < ddsd.dwHeight; i++)
	{
		memcpy(buffer+(ddsd.lPitch*i),bits+(NextMultipleOf4(ddsd.lPitch)*i),ddsd.lPitch);
	}
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
	ReleaseDC(hwnd,hdcwin);
	free(bmi);
}

const TCHAR ArialName[] = _T("Arial");
const TCHAR TimesName[] = _T("Times New Roman");
const TCHAR CourierName[] = _T("Courier New");
const TCHAR SSerifName[] = _T("MS Sans Serif");
const TCHAR SymbolName[] = _T("Symbol");
const TCHAR WingdingName[] = _T("Wingdings");

const TCHAR space[] = _T(" ");

const TCHAR regularname[] = _T(" Regular");
const TCHAR italicname[] = _T(" Italic");
const TCHAR boldname[] = _T(" Bold");

void DrawTextBlock(HDC hDC, DWORD x, DWORD &y, DWORD bold, BOOL italic, LPCTSTR font)
{
	TCHAR str[256];
	TCHAR num[32];
	TEXTMETRIC tm;
	HFONT DefaultFont;
	HFONT newfont;
	for(int i = -8; i > -25; i-=2)
	{
		newfont = CreateFont(i,0,0,0,bold,italic,FALSE,FALSE,DEFAULT_CHARSET,
			OUT_OUTLINE_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,font);
		DefaultFont = (HFONT)SelectObject(hDC,newfont);
		_tcscpy(str,font);
		_tcscat(str,space);
		_itot(-i,num,10);
		_tcscat(str,num);
		if(!italic && bold == FW_NORMAL) _tcscat(str,regularname);
		if(bold == FW_BOLD) _tcscat(str,boldname);
		if(italic) _tcscat(str,italicname);
		TextOut(hDC,x,y,str,_tcslen(str));
		GetTextMetrics(hDC,&tm);
		y += tm.tmHeight;
		newfont = (HFONT)SelectObject(hDC,DefaultFont);
		DeleteObject(newfont);
		if(i == -14) i -= 2;
		if(i == -18) i -= 4;
	}
}

POINT AngleToPoint(int x, int y, int width, int height, double angle)
{
	POINT ret;
	ret.x = x + (int)((double)width*sin(angle));
	ret.y = y + (int)((double)height*cos(angle));
	return ret;
}

void CreatePolygon(POINT *array, int sides, int x, int y, int width, int height)
{
	double angle = 0.;
	for(int i = 0; i < sides; i++)
	{
		angle = ((atan(1.)*8.)/(double)sides)*(double)i;
		array[i] = AngleToPoint(x,y,width,height,angle);
	}
}

void CreatePattern(HDC hdc, HBRUSH *brush, HPEN *pen, HBITMAP *bmp, int number)
{
	switch(number)
	{
	case 0:
	default:
		*pen = CreatePen(PS_SOLID,1,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,0,0));
		break;
	case 1:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,255,0));
		break;
	case 2:
		*pen = CreatePen(PS_SOLID,3,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,0,255));
		break;
	case 3:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,255,255));
		break;
	case 4:
		*pen = CreatePen(PS_SOLID,1,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,0,255));
		break;
	case 5:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,255,0));
		break;
	case 6:
		*pen = CreatePen(PS_SOLID,3,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(255,255,255));
		break;
	case 7:
		*pen = CreatePen(PS_SOLID,2,RGB(255,255,255));
		*brush = CreateSolidBrush(RGB(0,0,255));
		break;
	case 8:
		SetBkColor(hdc,RGB(0,0,255));
		*pen = CreatePen(PS_DOT,1,RGB(255,255,0));
		*brush = CreateHatchBrush(HS_BDIAGONAL,RGB(0,255,255));
		break;
	case 9:
		SetBkColor(hdc,RGB(255,0,0));
		*pen = CreatePen(PS_DASH,1,RGB(0,255,255));
		*brush = CreateHatchBrush(HS_CROSS,RGB(255,0,255));
		break;
	case 10:
		SetBkColor(hdc,RGB(0,255,0));
		*pen = CreatePen(PS_DASHDOT,1,RGB(255,0,255));
		*brush = CreateHatchBrush(HS_DIAGCROSS,RGB(255,255,0));
		break;
	case 11:
		SetBkColor(hdc,RGB(0,0,255));
		*pen = CreatePen(PS_DASHDOTDOT,1,RGB(0,255,0));
		*brush = CreateHatchBrush(HS_FDIAGONAL,RGB(0,255,0));
		break;
	case 12:
		SetBkColor(hdc,RGB(0,255,0));
		*pen = CreatePen(PS_DOT,1,RGB(0,0,255));
		*brush = CreateHatchBrush(HS_HORIZONTAL,RGB(255,0,0));
		break;
	case 13:
		SetBkColor(hdc,RGB(0,0,0));
		*pen = CreatePen(PS_DASH,1,RGB(255,0,0));
		*brush = CreateHatchBrush(HS_VERTICAL,RGB(0,0,255));
		break;
	case 14:
		SetBkColor(hdc,RGB(255,255,255));
		*bmp = CreateBitmap(16,16,1,1,dxgl_logo_bw);
		*pen = CreatePen(PS_SOLID,2,RGB(192,192,192));
		*brush = CreatePatternBrush(*bmp);
		break;
	case 15:
		*bmp = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_DXGLINV));
		*pen = CreatePen(PS_SOLID,2,RGB(192,192,192));
		*brush = CreatePatternBrush(*bmp);
		break;
	}
}
void DestroyPattern(HBRUSH *brush, HPEN *pen, HBITMAP *bmp, int number)
{
	if(number >= 14) DeleteObject(*bmp);
	DeleteObject(*pen);
	DeleteObject(*brush);
	*bmp = NULL;
	*brush = NULL;
	*pen = NULL;
}

void DrawBitmap(HDC hDC, int x, int y, int w, int h, LPCTSTR bmp, DWORD method)
{
	BITMAP bminfo;
	HBITMAP bitmap = LoadBitmap(GetModuleHandle(NULL),bmp);
	HDC hdcmem = CreateCompatibleDC(hDC);
	HBITMAP hbmold = (HBITMAP)SelectObject(hdcmem,bitmap);
	GetObject(bitmap,sizeof(bminfo),&bminfo);
	if((w == bminfo.bmWidth) && (h == bminfo.bmHeight)) BitBlt(hDC,x,y,w,h,hdcmem,0,0,method);
	else StretchBlt(hDC,x,y,w,h,hdcmem,0,0,bminfo.bmWidth,bminfo.bmHeight,method);
	SelectObject(hdcmem,hbmold);
	DeleteDC(hdcmem);
	DeleteObject(bitmap);
}

void DrawGDIPatterns(DDSURFACEDESC2 ddsd, HDC hDC, int type)
{
	int i;
	DWORD x,x2;
	DWORD y,y2;
	int r,g,b;
	HPEN pen;
	HBRUSH brush;
	HBITMAP bmp;
	HANDLE tmphandle, tmphandle2;
	POINT points[18];
	RECT rect;
	switch(type)
	{
	case 0: // Text
		y = y2 = 16;
		SetBkColor(hDC,RGB(0,0,0));
		SetTextColor(hDC,RGB(255,0,0));
		DrawTextBlock(hDC,0,y,FW_NORMAL,FALSE,ArialName);
		SetTextColor(hDC,RGB(0,255,0));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,TRUE,ArialName);
		SetTextColor(hDC,RGB(0,0,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,FALSE,ArialName);
		SetTextColor(hDC,RGB(0,255,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,TRUE,ArialName);
		if(ddsd.dwWidth > 216)
		{
			y2 = y;
			y = 16;
			SetTextColor(hDC,RGB(255,0,255));
			DrawTextBlock(hDC,216,y,FW_NORMAL,FALSE,TimesName);
			SetTextColor(hDC,RGB(255,255,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_NORMAL,TRUE,TimesName);
			SetTextColor(hDC,RGB(255,255,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_BOLD,FALSE,TimesName);
			SetTextColor(hDC,RGB(255,127,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,216,y,FW_BOLD,TRUE,TimesName);
		}
		y = y2;
		SetTextColor(hDC,RGB(0,127,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,FALSE,CourierName);
		SetTextColor(hDC,RGB(127,255,0));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_NORMAL,TRUE,CourierName);
		SetTextColor(hDC,RGB(127,0,255));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,FALSE,CourierName);
		SetTextColor(hDC,RGB(255,0,127));
		if(ddsd.dwHeight > y) DrawTextBlock(hDC,0,y,FW_BOLD,TRUE,CourierName);
		y = y2;
		if(ddsd.dwWidth > 380)
		{
			SetTextColor(hDC,RGB(0,255,127));
			DrawTextBlock(hDC,380,y,FW_NORMAL,FALSE,SSerifName);
			SetTextColor(hDC,RGB(127,127,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_NORMAL,TRUE,SSerifName);
			SetTextColor(hDC,RGB(255,127,127));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_BOLD,FALSE,SSerifName);
			SetTextColor(hDC,RGB(255,127,255));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,380,y,FW_BOLD,TRUE,SSerifName);
		}
		y = 16;
		if(ddsd.dwWidth > 560)
		{
			SetTextColor(hDC,RGB(191,0,0));
			DrawTextBlock(hDC,560,y,FW_NORMAL,FALSE,SymbolName);
			SetTextColor(hDC,RGB(0,191,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_NORMAL,TRUE,SymbolName);
			SetTextColor(hDC,RGB(0,0,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_BOLD,FALSE,SymbolName);
			SetTextColor(hDC,RGB(0,191,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,560,y,FW_BOLD,TRUE,SymbolName);
		}
		if((ddsd.dwWidth > 720) && (ddsd.dwHeight > y))
		{
			SetTextColor(hDC,RGB(191,0,191));
			DrawTextBlock(hDC,720,y,FW_NORMAL,FALSE,WingdingName);
			SetTextColor(hDC,RGB(191,191,0));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_NORMAL,TRUE,WingdingName);
			SetTextColor(hDC,RGB(191,191,191));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_BOLD,FALSE,WingdingName);
			SetTextColor(hDC,RGB(127,127,127));
			if(ddsd.dwHeight > y) DrawTextBlock(hDC,720,y,FW_BOLD,TRUE,WingdingName);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 0:  Text"),15);
		break;
	case 1: // Lines
		r = g = b = 0;
		points[0].x = ddsd.dwWidth/2;
		points[0].y = ddsd.dwHeight/2;
		points[1].y = 0;
		for(x = 1; x <= 10; x++)
		{
			r = (int)(x * 25.5);
			points[1].x = (LONG)(ddsd.dwWidth * (x/10.));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		for(y = 0; y <= 10; y++)
		{
			r = (int)((10-y) * 25.5);
			g = (int)(y * 25.5);
			points[1].y = (LONG)(ddsd.dwHeight * (y/10.));
			pen = CreatePen(PS_SOLID,y,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		for(x = 10; x > 0; x--)
		{
			g = (int)(x * 25.5);
			b = (int)((10-x) * 25.5);
			points[1].x = (LONG)(ddsd.dwWidth * (x/10.));
			pen = CreatePen(PS_SOLID,x,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
			if(x == 0) break;
		}
		SetBkColor(hDC,RGB(0,0,0));
		for(y = 10; y >= 0; y--)
		{
			r = g = (int)((10-y) * 25.5);
			points[1].y = (LONG)(ddsd.dwHeight * (y/10.));
			switch(y)
			{
			case 1:
			case 6:
			default:
				pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
				break;
			case 2:
			case 7:
				pen = CreatePen(PS_DASH,0,RGB(r,g,b));
				break;
			case 3:
			case 8:
				pen = CreatePen(PS_DOT,0,RGB(r,g,b));
				break;
			case 4:
			case 9:
				pen = CreatePen(PS_DASHDOT,0,RGB(r,g,b));
				break;
			case 5:
			case 10:
				pen = CreatePen(PS_DASHDOTDOT,0,RGB(r,g,b));
				break;
			}
			tmphandle = SelectObject(hDC,pen);
			Polyline(hDC,points,2);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
			if(y == 0) break;
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 1:  Lines"),16);
		break;
	case 2: // Beziers
		points[0].x = 0;
		points[1].x = ddsd.dwWidth / 4;
		points[2].x = 3*(ddsd.dwWidth/4);
		points[3].x = ddsd.dwWidth;
		points[0].y = points[3].y = ddsd.dwHeight / 2;
		r = g = b = 0;
		for(i = 0; i < 50; i++)
		{
			r = (int)(i*5.1);
			points[1].y = (LONG)((ddsd.dwHeight/2.)+(i*(ddsd.dwHeight/30.)));
			points[2].y = (LONG)((ddsd.dwHeight/2.)-(i*(ddsd.dwHeight/30.)));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			PolyBezier(hDC,points,4);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		r = 0;
		for(i = 0; i < 50; i++)
		{
			g = b = (int)(i*5.1);
			points[1].y = (LONG)((ddsd.dwHeight/2.)-(i*(ddsd.dwHeight/30.)));
			points[2].y = (LONG)((ddsd.dwHeight/2.)+(i*(ddsd.dwHeight/30.)));
			pen = CreatePen(PS_SOLID,0,RGB(r,g,b));
			tmphandle = SelectObject(hDC,pen);
			PolyBezier(hDC,points,4);
			SelectObject(hDC,tmphandle);
			DeleteObject(pen);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 2:  Beziers"),18);
		break;
	case 3: // polygons
		x2 = (int)(ddsd.dwWidth / 8.1);
		y2 = (int)(ddsd.dwHeight / 8.1);
		for(i = 0; i < 16; i++)
		{
			x = (int)(((i&3) * (ddsd.dwWidth/4.))+(ddsd.dwWidth/8));
			y = (int)(((i>>2) * (ddsd.dwHeight/4.))+(ddsd.dwHeight/8));
			CreatePolygon(points,i+3,x,y,x2,y2);
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			Polygon(hDC,points,i+3);
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 3:  Polygons"),19);
		break;
	case 4: // rectangles
		for(i = 0; i < 16; i++)
		{
			rect.left = (int)((double)(i&3)*(double)ddsd.dwWidth/4.);
			rect.right = rect.left+(int)(ddsd.dwWidth/4.)-8;
			rect.top = (int)((double)(i>>2)*(double)ddsd.dwHeight/4);
			rect.bottom = rect.top+(int)(ddsd.dwHeight/4.)-8;
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			if(i <= 7) FillRect(hDC,&rect,brush);
			else RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,i+8,24-i);
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 4:  Rectangles"),21);
		break;
	case 5: // ellipses
		for(i = 0; i < 16; i++)
		{
			rect.left = (int)((double)(i&3)*(double)ddsd.dwWidth/4.);
			rect.right = rect.left+(int)(ddsd.dwWidth/4.)-8;
			rect.top = (int)((double)(i>>2)*(double)ddsd.dwHeight/4);
			rect.bottom = rect.top+(int)(ddsd.dwHeight/4.)-8;
			x = (rect.right-rect.left)/4;
			y = (rect.bottom-rect.top)/4;
			CreatePattern(hDC,&brush,&pen,&bmp,i);
			tmphandle = SelectObject(hDC,pen);
			tmphandle2 = SelectObject(hDC,brush);
			switch(i)
			{
			case 0:
			default:
				Ellipse(hDC,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 1:
				Ellipse(hDC,rect.left,rect.top+y,rect.right,rect.bottom-y);
				break;
			case 2:
				Ellipse(hDC,rect.left+x,rect.top,rect.right-x,rect.bottom);
				break;
			case 3:
				Ellipse(hDC,rect.left+x,rect.top+y,rect.right-x,rect.bottom-y);
				break;
			case 4:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left+(2*x),rect.top,rect.left+(2*x),rect.bottom);
				break;
			case 5:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top+(2*y),rect.right,rect.top+(2*y));
				break;
			case 6:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 7:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 8:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+(2*x),rect.bottom);
				break;
			case 9:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.left,rect.bottom);
				break;
			case 10:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left,rect.top,rect.right,rect.bottom);
				break;
			case 11:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 12:
				RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,ddsd.dwWidth/5,ddsd.dwHeight/5);
				break;
			case 13:
				RoundRect(hDC,rect.left,rect.top,rect.right,rect.bottom,ddsd.dwWidth/8,ddsd.dwHeight/8);
				break;
			case 14:
				Chord(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.right,rect.top,rect.left+x,rect.bottom);
				break;
			case 15:
				Pie(hDC,rect.left,rect.top,rect.right,rect.bottom,rect.left+x,rect.bottom,rect.right,rect.top);
				break;
			}
			SelectObject(hDC,tmphandle2);
			SelectObject(hDC,tmphandle);
			DestroyPattern(&brush,&pen,&bmp,i);
		}
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 5:  Ellipses"),19);
		break;
	case 6: // gradients
		DrawGradient(hDC,0,ddsd.dwWidth,0,ddsd.dwHeight / 7,0x0000FF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,ddsd.dwHeight / 7, 2*(ddsd.dwHeight/7),0x00FF00,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,2*(ddsd.dwHeight/7),3*(ddsd.dwHeight/7),0xFF0000,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,3*(ddsd.dwHeight/7),4*(ddsd.dwHeight/7),0xFFFF00,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,4*(ddsd.dwHeight/7),5*(ddsd.dwHeight/7),0xFF00FF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,5*(ddsd.dwHeight/7),6*(ddsd.dwHeight/7),0x00FFFF,gradientavailable);
		DrawGradient(hDC,0,ddsd.dwWidth,6*(ddsd.dwHeight/7),ddsd.dwHeight,0xFFFFFF,gradientavailable);
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 6:  Gradients"),20);
		break;
	case 7: // bitmaps
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)(ddsd.dwHeight/6.)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),SRCCOPY);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)(ddsd.dwHeight/6.)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),SRCCOPY);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),0,(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),SRCCOPY);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),0,(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),SRCCOPY);
		SetBkColor(hDC,RGB(0,255,255));
		brush = CreateHatchBrush(HS_DIAGCROSS,RGB(128,0,128));
		tmphandle = SelectObject(hDC,brush);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)(ddsd.dwHeight/2.)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),MERGECOPY);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)(ddsd.dwHeight/2.)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),MERGECOPY);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),(int)(ddsd.dwHeight/3.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),MERGECOPY);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),(int)(ddsd.dwHeight/3.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),MERGECOPY);
		SelectObject(hDC,tmphandle);
		DeleteObject(brush);
		rect.left = 0;
		rect.right = ddsd.dwWidth;
		rect.top = (int)((ddsd.dwHeight/3.)*2.);
		rect.bottom = ddsd.dwHeight;
		FillRect(hDC,&rect,(HBRUSH) (COLOR_WINDOW+1));
		DrawBitmap(hDC,(int)(ddsd.dwWidth/8.)-8,(int)((5./6.)*ddsd.dwHeight)-8,
			16,16,MAKEINTRESOURCE(IDB_DXGLINV),SRCINVERT);
		DrawBitmap(hDC,(int)((ddsd.dwWidth/4.)+(ddsd.dwWidth/8.))-32,(int)((5./6.)*ddsd.dwHeight)-32,
			64,64,MAKEINTRESOURCE(IDB_DXGLINV64),SRCINVERT);
		DrawBitmap(hDC,(int)(ddsd.dwWidth/2.),(int)((ddsd.dwHeight/3.)*2.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV),SRCINVERT);
		DrawBitmap(hDC,(int)(.75*ddsd.dwWidth),(int)((ddsd.dwHeight/3.)*2.),(int)(ddsd.dwWidth/4.),
			(int)(ddsd.dwHeight/3.),MAKEINTRESOURCE(IDB_DXGLINV64),SRCINVERT);
		SetBkColor(hDC,RGB(0,0,255));
		SetTextColor(hDC,RGB(255,255,255));
		TextOut(hDC,0,0,_T("Screen 7:  Bitmaps"),18);
		break;
	}
}

void DrawROPPatternSurface(MultiDirectDrawSurface *surface, int bpp, int ddver)
{
	DDSURFACEDESC2 ddsd;
	if (ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	surface->GetSurfaceDesc(&ddsd);
	surface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
	switch (bpp)
	{
	case 8:
	default:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char*)ddsd.lpSurface + (i*ddsd.lPitch), &blt_pattern_8[6 * i], 6);
		break;
	case 15:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &blt_pattern_15[6 * i], 12);
		break;
	case 16:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &blt_pattern_16[6 * i], 12);
		break;
	case 24:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &blt_pattern_24[18 * i], 18);
		break;
	case 32:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &blt_pattern_32[6 * i], 24);
		break;
	}
	surface->Unlock(NULL);
}

void DrawROPPatterns(MultiDirectDrawSurface *primary, DDSPRITE *sprites, int backbuffers, int ddver, int bpp, DWORD *ropcaps, 
	HWND hwnd, LPDIRECTDRAWPALETTE palette)
{
	HICON ico_dxglsm;
	HICON ico_x16;
	HDC hdc;
	DDSURFACEDESC2 ddsd;
	int bltx, blty;
	RECT bltrect;
	DDBLTFX bltfx;
	ZeroMemory(&bltfx, sizeof(DDBLTFX));
	bltfx.dwSize = sizeof(DDBLTFX);
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	if (ddver > 3)ddsd.dwSize = sizeof(DDSURFACEDESC2);
	else ddsd.dwSize = sizeof(DDSURFACEDESC);
	ico_dxglsm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DXGLSM), IMAGE_ICON, 16, 16, 0);
	ico_x16 = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_X16), IMAGE_ICON, 16, 16, 0);
	bltrect.left = bltrect.top = 0;
	bltrect.bottom = bltrect.right = 16;
	bltfx.dwFillColor = 0;
	sprites[2].surface->Blt(&bltrect, NULL, NULL, DDBLT_COLORFILL, &bltfx);
	sprites[2].surface->GetDC(&hdc);
	DrawIconEx(hdc, 0, 0, ico_dxglsm, 16, 16, 0, NULL, DI_NORMAL);
	sprites[2].surface->ReleaseDC(hdc);
	sprites[3].surface->GetDC(&hdc);
	DrawIconEx(hdc, 0, 0, ico_x16, 16, 16, 0, NULL, DI_NORMAL);
	sprites[3].surface->ReleaseDC(hdc);
	DestroyIcon(ico_x16);
	sprites[4].surface->GetSurfaceDesc(&ddsd);
	sprites[4].surface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
	switch (bpp)
	{
	case 8:
	default:
		for (int i = 0; i < 8; i++)
			memcpy((unsigned char*)ddsd.lpSurface + (i*ddsd.lPitch), &back_pattern_8[8 * i], 8);
		break;
	case 15:
		for (int i = 0; i < 8; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &back_pattern_15[8 * i], 16);
		break;
	case 16:
		for (int i = 0; i < 8; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &back_pattern_16[8 * i], 16);
		break;
	case 24:
		for (int i = 0; i < 8; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &back_pattern_24[24 * i], 24);
		break;
	case 32:
		for (int i = 0; i < 8; i++)
			memcpy((unsigned char *)ddsd.lpSurface + (i*ddsd.lPitch), &back_pattern_32[8 * i], 32);
		break;
	}
	sprites[4].surface->Unlock(NULL);
	DrawROPPatternSurface(sprites[5].surface, bpp, ddver);
	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			bltx = x * 8;
			blty = y * 8;
			bltrect.left = bltx;
			bltrect.right = bltx + 8;
			bltrect.top = blty;
			bltrect.bottom = blty + 8;
			sprites[0].surface->Blt(&bltrect, sprites[4].surface, NULL, DDBLT_WAIT, NULL);
			sprites[1].surface->Blt(&bltrect, sprites[4].surface, NULL, DDBLT_WAIT, NULL);
		}
	}
	for (int i = 0; i < 256; i++)
	{
		bltx = (i & 0xF) * 16;
		blty = (i >> 4) * 16;
		bltrect.left = bltx;
		bltrect.right = bltx + 16;
		bltrect.top = blty;
		bltrect.bottom = blty + 16;
		if (IsRopCodeSupported(rop_codes[i], ropcaps))
		{
			if (rop_texture_usage[i] & 4) bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)sprites[5].surface->GetSurface();
			else bltfx.lpDDSPattern = NULL;
			bltfx.dwROP = rop_codes[i];
			sprites[0].surface->Blt(&bltrect, sprites[2].surface, NULL, DDBLT_ROP | DDBLT_WAIT, &bltfx);
		}
		else
		{
			sprites[0].surface->Blt(&bltrect, sprites[3].surface, NULL, DDBLT_WAIT, NULL);
		}
	}
	HDC hdcwin = GetDC(hwnd);
	DWORD colors[256];
	int bitmode = BI_RGB;
	DWORD bitmasks[3];
	LPBYTE bits;
	BITMAPINFO *bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER)+1024);
	if (!bmi) return;
	ZeroMemory(bmi, sizeof(BITMAPINFOHEADER)+1024);
	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		palette->GetEntries(0, 0, 256, (LPPALETTEENTRY)colors);
		for (int i = 0; i < 256; i++)
			colors[i] = ((colors[i] & 0x0000FF) << 16) | (colors[i] & 0x00FF00) | ((colors[i] & 0xFF0000) >> 16);
		memcpy(bmi->bmiColors, colors, 1024);
	}
	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
	{
		bitmode = BI_BITFIELDS;
		bitmasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
		bitmasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
		bitmasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(bmi->bmiColors, bitmasks, 3 * sizeof(DWORD));
	}
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = 16;
	bmi->bmiHeader.biHeight = -16;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biCompression = bitmode;
	bmi->bmiHeader.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	HBITMAP bmpicon = CreateDIBSection(hdcwin, bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
	HDC hdcicon = CreateCompatibleDC(hdcwin);
	HGDIOBJ hbmiconold = SelectObject(hdcicon, bmpicon);
	DrawIconEx(hdcicon, 0, 0, ico_dxglsm, 16, 16, 0, NULL, DI_NORMAL);
	DestroyIcon(ico_dxglsm);
	bmi->bmiHeader.biWidth = 6;
	bmi->bmiHeader.biHeight = -6;
	HBITMAP bmppattern = CreateDIBSection(hdcwin, bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
	int bmpwidth = NextMultipleOf4((bmi->bmiHeader.biBitCount*bmi->bmiHeader.biWidth) / 8);
	switch (bpp)
	{
	case 8:
	default:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char*)bits + (i*bmpwidth), &blt_pattern_8[6 * i], 6);
		break;
	case 15:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)bits + (i*bmpwidth), &blt_pattern_15[6 * i], 12);
		break;
	case 16:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)bits + (i*bmpwidth), &blt_pattern_16[6 * i], 12);
		break;
	case 24:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)bits + (i*bmpwidth), &blt_pattern_24[18 * i], 18);
		break;
	case 32:
		for (int i = 0; i < 6; i++)
			memcpy((unsigned char *)bits + (i*bmpwidth), &blt_pattern_32[6 * i], 24);
		break;
	}
	HBRUSH hbrpattern = CreatePatternBrush(bmppattern);
	HDC hdcblt;
	sprites[1].surface->GetDC(&hdcblt);
	HGDIOBJ hbrold = SelectObject(hdcblt, hbrpattern);
	for (int i = 0; i < 256; i++)
	{
		bltx = (i & 0xF) * 16;
		blty = (i >> 4) * 16;
		BitBlt(hdcblt, bltx, blty, 16, 16, hdcicon, 0, 0, rop_codes[i]);
	}
	SelectObject(hdcblt, hbrold);
	sprites[1].surface->ReleaseDC(hdcblt);
	primary->GetDC(&hdcblt);
	SetBkColor(hdcblt, RGB(0, 0, 255));
	SetTextColor(hdcblt, RGB(255, 255, 255));
	TextOut(hdcblt, 0, 0, _T("Screen 0:  DDraw Blt"),20);
	primary->ReleaseDC(hdcblt);
	primary->BltFast(0, 16, sprites[0].surface, NULL, DDBLTFAST_WAIT);
	if (backbuffers)
	{
		MultiDirectDrawSurface *back;
		DDSCAPS2 ddscaps;
		ZeroMemory(&ddscaps, sizeof(DDSCAPS2));
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		primary->GetAttachedSurface(&ddscaps, &back);
		back->GetDC(&hdcblt);
		SetBkColor(hdcblt, RGB(0, 0, 255));
		SetTextColor(hdcblt, RGB(255, 255, 255));
		TextOut(hdcblt, 0, 0, _T("Screen 1:  GDI Blt"), 18);
		back->ReleaseDC(hdcblt);
		back->BltFast(0, 16, sprites[1].surface, NULL, DDBLTFAST_WAIT);
		back->Release();
	}
	free(bmi);
}

void DrawRotatedBlt(MultiDirectDrawSurface *primary, DDSPRITE *sprites)
{
HDC hdc;
DDBLTFX bltfx;
ZeroMemory(&bltfx, sizeof(DDBLTFX));
bltfx.dwSize = sizeof(DDBLTFX);
sprites[0].surface->GetDC(&hdc);
DrawBitmap(hdc, 0, 0, 64, 64, MAKEINTRESOURCE(IDB_DXGLINV64), SRCCOPY);
sprites[0].surface->ReleaseDC(hdc);
RECT r;
r.left = 0;
r.right = 64;
r.top = 0;
r.bottom = 64;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 64;
r.right = 128;
bltfx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 128;
r.right = 192;
bltfx.dwDDFX = DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 192;
r.right = 256;
bltfx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.top = 64;
r.bottom = 128;
r.left = 0;
r.right = 64;
bltfx.dwDDFX = DDBLTFX_ROTATE90;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 64;
r.right = 128;
bltfx.dwDDFX = DDBLTFX_ROTATE90 | DDBLTFX_MIRRORLEFTRIGHT;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 128;
r.right = 192;
bltfx.dwDDFX = DDBLTFX_ROTATE90 | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 192;
r.right = 256;
bltfx.dwDDFX = DDBLTFX_ROTATE90 | DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.top = 128;
r.bottom = 192;
r.left = 0;
r.right = 64;
bltfx.dwDDFX = DDBLTFX_ROTATE180;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 64;
r.right = 128;
bltfx.dwDDFX = DDBLTFX_ROTATE180 | DDBLTFX_MIRRORLEFTRIGHT;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 128;
r.right = 192;
bltfx.dwDDFX = DDBLTFX_ROTATE180 | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 192;
r.right = 256;
bltfx.dwDDFX = DDBLTFX_ROTATE180 | DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.top = 192;
r.bottom = 256;
r.left = 0;
r.right = 64;
bltfx.dwDDFX = DDBLTFX_ROTATE270;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 64;
r.right = 128;
bltfx.dwDDFX = DDBLTFX_ROTATE270 | DDBLTFX_MIRRORLEFTRIGHT;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 128;
r.right = 192;
bltfx.dwDDFX = DDBLTFX_ROTATE270 | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
r.left = 192;
r.right = 256;
bltfx.dwDDFX = DDBLTFX_ROTATE270 | DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
primary->Blt(&r, sprites[0].surface, NULL, DDBLT_DDFX, &bltfx);
}

void DrawColorKeyCompPatterns(DDSURFACEDESC2 ddsd, unsigned char *buffer, int bpp, int index)
{
	int x, y;
	DWORD i;
	if (index)
	{
		switch (bpp)
		{
		case 8:
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					i = x + ((15 - y) << 4);
					buffer[x + (ddsd.lPitch*y)] = i & 0xFF;
				}
			}
			break;
		case 15:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 32; x++)
				{
					switch (y)
					{
					case 7:
						i = x << 10;
						break;
					case 6:	
						i = x << 5;
						break;
					case 5:
						i = x;
						break;
					case 4:
						i = x + (x << 5);
						break;
					case 3:
						i = x + (x << 10);
						break;
					case 2:
						i = (x << 5) + (x << 10);
						break;
					case 1:
					default:
						i = x + (x << 5) + (x << 10);
						break;
					}
					buffer[(x * 2) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 2) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
				}
			}
			break;
		case 16:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 64; x++)
				{
					switch (y)
					{
					case 7:
						i = (x >> 1) << 11;
						break;
					case 6:
						i = x << 5;
						break;
					case 5:
						i = x >> 1;
						break;
					case 4:
						i = (x >> 1) + (x << 5);
						break;
					case 3:
						i = (x >> 1) + ((x >> 1) << 11);
						break;
					case 2:
						i = (x << 5) + ((x >> 1) << 11);
						break;
					case 1:
					default:
						i = (x >> 1) + (x << 5) + ((x >> 1) << 11);
						break;
					}
					buffer[(x * 2) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 2) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
				}
			}
			break;
		case 24:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 256; x++)
				{
					switch (y)
					{
					case 7:
						i = x << 16;
						break;
					case 6:
						i = x << 8;
						break;
					case 5:
						i = x;
						break;
					case 4:
						i = x + (x << 8);
						break;
					case 3:
						i = x + (x << 16);
						break;
					case 2:
						i = (x << 8) + (x << 16);
						break;
					case 1:
					default:
						i = x + (x << 8) + (x << 16);
						break;
					}
					buffer[(x * 3) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 3) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
					buffer[((x * 3) + (ddsd.lPitch*(y - 1))) + 2] = (char)(i >> 16);
				}
			}
			break;
		case 32:
		default:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 256; x++)
				{
					switch (y)
					{
					case 7:
						i = x << 16;
						break;
					case 6:
						i = x << 8;
						break;
					case 5:
						i = x;
						break;
					case 4:
						i = x + (x << 8);
						break;
					case 3:
						i = x + (x << 16);
						break;
					case 2:
						i = (x << 8) + (x << 16);
						break;
					case 1:
					default:
						i = x + (x << 8) + (x << 16);
						break;
					}
					buffer[(x * 4) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 2] = (char)(i >> 16);
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 3] = 0;
				}
			}
			break;
		}
	}
	else
	{
		switch (bpp)
		{
		case 8:
			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 16; x++)
				{
					i = 255 - (x + ((15 - y) << 4));
					buffer[x + (ddsd.lPitch*y)] = i & 0xFF;
				}
			}
			break;
		case 15:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 32; x++)
				{
					switch (y)
					{
					case 1:
						i = (31 - x) << 10;
						break;
					case 2:
						i = (31 - x) << 5;
						break;
					case 3:
						i = (31 - x);
						break;
					case 5:
						i = (31 - x) + ((31 - x) << 5);
						break;
					case 4:
						i = (31 - x) + ((31 - x) << 10);
						break;
					case 6:
						i = ((31 - x) << 5) + ((31 - x) << 10);
						break;
					case 7:
					default:
						i = (31 - x) + ((31 - x) << 5) + ((31 - x) << 10);
						break;
					}
					buffer[(x * 2) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 2) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
				}
			}
			break;
		case 16:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 64; x++)
				{
					switch (y)
					{
					case 1:
						i = ((63 - x) >> 1) << 11;
						break;
					case 2:
						i = (63 - x) << 5;
						break;
					case 3:
						i = (63 - x) >> 1;
						break;
					case 5:
						i = ((63 - x) >> 1) + ((63 - x) << 5);
						break;
					case 4:
						i = ((63 - x) >> 1) + (((63 - x) >> 1) << 11);
						break;
					case 6:
						i = ((63 - x) << 5) + (((63 - x) >> 1) << 11);
						break;
					case 7:
					default:
						i = ((63 - x) >> 1) + ((63 - x) << 5) + (((63 - x) >> 1) << 11);
						break;
					}
					buffer[(x * 2) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 2) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
				}
			}
			break;
		case 24:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 256; x++)
				{
					switch (y)
					{
					case 1:
						i = (255 - x) << 16;
						break;
					case 2:
						i = (255 - x) << 8;
						break;
					case 3:
						i = (255 - x);
						break;
					case 5:
						i = (255 - x) + ((255 - x) << 8);
						break;
					case 4:
						i = (255 - x) + ((255 - x) << 16);
						break;
					case 6:
						i = ((255 - x) << 8) + ((255 - x) << 16);
						break;
					case 7:
					default:
						i = (255 - x) + ((255 - x) << 8) + ((255 - x) << 16);
						break;
					}
					buffer[(x * 3) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 3) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
					buffer[((x * 3) + (ddsd.lPitch*(y - 1))) + 2] = (char)(i >> 16);
				}
			}
			break;
		case 32:
		default:
			for (y = 1; y < 8; y++)
			{
				for (x = 0; x < 256; x++)
				{
					switch (y)
					{
					case 1:
						i = (255 - x) << 16;
						break;
					case 2:
						i = (255 - x) << 8;
						break;
					case 3:
						i = (255 - x);
						break;
					case 5:
						i = (255 - x) + ((255 - x) << 8);
						break;
					case 4:
						i = (255 - x) + ((255 - x) << 16);
						break;
					case 6:
						i = ((255 - x) << 8) + ((255 - x) << 16);
						break;
					case 7:
					default:
						i = (255 - x) + ((255 - x) << 8) + ((255 - x) << 16);
						break;
					}
					buffer[(x * 4) + (ddsd.lPitch*(y - 1))] = (char)i;
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 1] = (char)(i >> 8);
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 2] = (char)(i >> 16);
					buffer[((x * 4) + (ddsd.lPitch*(y - 1))) + 3] = 0;
				}
			}
			break;
		}
	}
}

BOOL TextOutShadow(HDC hDC, int x, int y, LPCTSTR string, int count, COLORREF shadow)
{
	int bkmode;
	BOOL ret;
	COLORREF color;
	int x2, y2;
	x2 = x + 1;
	y2 = y + 1;
	bkmode = GetBkMode(hDC);
	if (bkmode == OPAQUE) TextOut(hDC, x, y, string, count); // Ensure background is painted
	bkmode = SetBkMode(hDC, TRANSPARENT);
	color = SetTextColor(hDC, shadow);
	TextOut(hDC, x2, y2, string, count);
	SetTextColor(hDC, color);
	ret = TextOut(hDC, x, y, string, count);
	SetBkMode(hDC, bkmode);
	return ret;
}

static const LPTSTR strFormatTestTitle = _T("Surface format test");
static const LPTSTR strFormatTestKeys1 = _T("UP/DOWN: src type PGUP/DN: dest type ");
static const LPTSTR strFormatTestKeys2 = _T("LEFT/RIGHT: pattern TAB: render method");
static const LPTSTR strFormatTestKeys3 = _T("SPACE: show/hide help/info");
static const LPTSTR strFormatTestStatus1 = _T("PATTERN: ");
static const LPTSTR strFormatTestStatus2 = _T("METHOD: ");
static const LPTSTR strFormatTestPatterns[] =
{
	_T("Unknown "),
	_T("Palettes "),
	_T("Gradients "),
};
static const LPTSTR StrFormatTestMethods[] =
{
	_T("Unknown"),
	_T("Blt Sysmem"),
	_T("Blt Vidmem"),
	_T("D3D Quad"),
	_T("Overlay")
};
static const int START_SURFACEFORMATS = __LINE__;
static const LPTSTR strSurfaceFormats[] =
{
	_T("Primary surface"), // -1
	_T("Same as primary"), // 0
	_T("8-bit Palette"),
	_T("8-bit 332"),
	_T("15-bit 555"),
	_T("16-bit 565"),
	_T("24-bit 888"),
	_T("24-bit 888 RGB"),
	_T("32-bit 888"),
	_T("32-bit 888 RGB"),
	_T("16-bit 8332"),
	_T("16-bit 4444"),
	_T("16-bit 1555"),
	_T("32-bit 8888"),
	_T("8-bit luminance"),
	_T("8-bit alpha"),
	_T("8-bit lum/alpha"),
	_T("16-bit Zbuffer"),
	_T("24-bit Zbuffer"),
	_T("24-bit Z, 32bit"),
	_T("32-bit Zbuffer"),
	_T("32-bit Z/stencil"),
	_T("32-bit Z/st.rev")
};
static const int END_SURFACEFORMATS = __LINE__ - 4;
const int numsurfaceformats = END_SURFACEFORMATS - START_SURFACEFORMATS;

const DDPIXELFORMAT surfaceformats[] =
{
	{0,						0,								0,		0,		0,			0,			0,			0},  // reserved
	{sizeof(DDPIXELFORMAT),	DDPF_PALETTEINDEXED8,			0,		8,		0,			0,			0,			0},  // 8-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},  // 8 bit 332
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},  // 15 bit 555
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},  // 16 bit 565
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF0000,	0xFF00,		0xFF,		0},  // 24 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF,		0xFF00,		0xFF0000,	0},  // 24 bit 888 RGB
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},  // 32 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF,		0xFF00,		0xFF0000,	0},  // 32 bit 888 RGB
	{sizeof(DDPIXELFORMAT),	DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xE0,		0x1C,		0x3,		0xFF00},  // 16-bit 8332
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xF00,		0xF0,		0xF,		0xF000},  // 16-bit 4444
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0x7c00,		0x3E0,		0x1F,		0x8000},  // 16-bit 1555
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		32,		0xFF0000,	0xFF00,		0xFF,		0xFF000000},  // 32-bit 8888
	{sizeof(DDPIXELFORMAT), DDPF_LUMINANCE,					0,		8,		0xFF,		0,			0,			0},  // 8-bit luminance
	{sizeof(DDPIXELFORMAT),	DDPF_ALPHA,						0,		8,		0,			0,			0,			0},  // 8-bit alpha
	{sizeof(DDPIXELFORMAT),	DDPF_LUMINANCE|DDPF_ALPHAPIXELS,0,		16,		0xFF,		0,			0,			0xFF00},  // 8-bit luminance alpha
	{sizeof(DDPIXELFORMAT), DDPF_ZBUFFER,					0,		16,		0,			0xFFFF,		0,			0},  // 16 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		24,		0,			0xFFFFFF00,	0,			0},  // 24 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFF00,	0,			0},  // 24 bit Z buffer, 32-bit space
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFFFF,	0,			0},  // 32 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFFFFFF00,	0xFF,		0},  // 32 bit Z buffer with stencil
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFF,		0xFFFFFF00,	0}   // 32 bit Z buffer with stencil, reversed
};

static const LPTSTR strErrorMessages[] =
{
	_T("Unknown error: "),
	_T("Error creating src surf: "),
	_T("Error locking src surf: "),
	_T("Error getting hdc: "),
	_T("Error creating dest surf: "),
	_T("Error blitting src to pri: "),
	_T("Error blitting src to dest: "),
	_T("Error blitting dest to pri: "),
	_T("Error initializing Direct3D"),
	_T("Overlays not available yet")
};

void DrawFormatTestHUD(MultiDirectDrawSurface *surface, int srcformat, int destformat, int showhud,
	int testpattern, int testmethod, int x, int y, int errorlocation, HRESULT error)
{
	HDC hdc;
	HRESULT err;
	COLORREF oldcolor;
	COLORREF oldbkcolor;
	HFONT DefaultFont;
	HFONT newfont;
	RECT r;
	TCHAR buffer[256];
	TCHAR number[34];
	int oldbk;
	SIZE charsize;
	int rows, cols;
	int formatrows;
	int posx, posy;
	int formatposy;
	int formatfirst, formatlast;
	int i;
	err = surface->GetDC(&hdc);
	if (FAILED(err)) return;
	if (y < 350)
	{
		newfont = CreateFont(-8, -8, 0, 0, 0, 0, 0, 0, OEM_CHARSET, OUT_DEVICE_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Terminal"));
	}
	else if ((x > 1024) && (y > 600))
	{
		newfont = CreateFont(-16, -12, 0, 0, 0, 0, 0, 0, OEM_CHARSET, OUT_DEVICE_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Terminal"));
	}
	else
	{
		newfont = CreateFont(-12, -8, 0, 0, 0, 0, 0, 0, OEM_CHARSET, OUT_DEVICE_PRECIS,
			CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Terminal"));
	}
	DefaultFont = (HFONT)SelectObject(hdc, newfont);
	GetTextExtentPoint(hdc, _T("A"), 1, &charsize);
	rows = y / charsize.cy;
	cols = x / charsize.cx;
	r.left = 0;
	r.right = 128;
	r.top = 0;
	r.bottom = 16;
	oldcolor = SetTextColor(hdc, RGB(255, 255, 255));
	oldbkcolor = SetBkColor(hdc, RGB(0, 0, 255));
	if (showhud == 2) oldbk = SetBkMode(hdc, TRANSPARENT);
	else oldbk = SetBkMode(hdc, OPAQUE);
	if (showhud)
	{
		TextOutShadow(hdc, 0, 0, strFormatTestTitle, _tcslen(strFormatTestTitle), RGB(0, 0, 192));
		TextOutShadow(hdc, 0, charsize.cy, strFormatTestKeys1, _tcslen(strFormatTestKeys1), RGB(0, 0, 192));
		if (cols < (_tcslen(strFormatTestKeys1) + _tcslen(strFormatTestKeys2)))
		{
			posx = 0;
			posy = 2 * charsize.cy;
		}
		else
		{
			posx = _tcslen(strFormatTestKeys1) * charsize.cx;
			posy = charsize.cy;
		}
		TextOutShadow(hdc, posx, posy, strFormatTestKeys2, _tcslen(strFormatTestKeys2), RGB(0, 0, 192));
		posx = 0;
		posy += charsize.cy;
		TextOutShadow(hdc, posx, posy, strFormatTestKeys3, _tcslen(strFormatTestKeys3), RGB(0, 0, 192));
		posy += charsize.cy;
		_tcscpy(buffer, strFormatTestStatus1);
		if (testpattern < 0) testpattern = 0;
		if (testpattern > 2) testpattern = 0;
		_tcscat(buffer, strFormatTestPatterns[testpattern]);
		_tcscat(buffer, strFormatTestStatus2);
		if (testmethod < 0) testmethod = 0;
		if (testmethod > 4) testmethod = 0;
		_tcscat(buffer, StrFormatTestMethods[testmethod]);
		TextOutShadow(hdc, posx, posy, buffer, _tcslen(buffer), RGB(0, 0, 192));
		// List source formats
		formatposy = posy + charsize.cy;
		posy = formatposy;
		SetBkMode(hdc, TRANSPARENT);
		formatrows = rows - (formatposy / charsize.cy) - 1;
		if (formatrows > numsurfaceformats)
		{
			formatfirst = 0;
			formatlast = numsurfaceformats - 1;
		}
		else
		{
			if (srcformat < formatrows / 2)
			{
				formatfirst = 0;
				formatlast = formatrows;
			}
			else if (srcformat+2 > (numsurfaceformats - (formatrows / 2)))
			{
				formatlast = numsurfaceformats - 1;
				formatfirst = formatlast - (formatrows);
			}
			else
			{
				formatfirst = srcformat - (formatrows / 2);
				formatlast = srcformat + (formatrows / 2) + (formatrows % 2);
			}
		}
		for (i = formatfirst; i < formatlast; i++)
		{
			if (i == srcformat)
			{
				posx = 0;
				SetTextColor(hdc, RGB(255, 255, 255));
				TextOutShadow(hdc, posx, posy, _T("-->"), 3, RGB(0, 0, 192));
				posx = 3 * charsize.cx;
				TextOutShadow(hdc, posx, posy, strSurfaceFormats[i + 1], _tcslen(strSurfaceFormats[i + 1]), RGB(0, 0, 192));
			}
			else
			{
				SetTextColor(hdc, RGB(192, 192, 192));
				posx = 3 * charsize.cx;
				TextOut(hdc, posx, posy, strSurfaceFormats[i + 1], _tcslen(strSurfaceFormats[i + 1]));
			}
			posy += charsize.cy;
		}
		// List destination formats
		posy = formatposy;
		SetBkMode(hdc, TRANSPARENT);
		formatrows = rows - (formatposy / charsize.cy) - 1;
		if (formatrows >= numsurfaceformats)
		{
			formatfirst = -1;
			formatlast = numsurfaceformats - 1;
		}
		else
		{
			if (destformat < formatrows / 2)
			{
				formatfirst = 0;
				formatlast = formatrows;
			}
			else if (destformat + 2 > (numsurfaceformats - (formatrows / 2)))
			{
				formatlast = numsurfaceformats - 1;
				formatfirst = formatlast - (formatrows);
			}
			else
			{
				formatfirst = destformat - (formatrows / 2);
				formatlast = destformat + (formatrows / 2) + (formatrows % 2);
			}
		}
		for (i = formatfirst; i < formatlast; i++)
		{
			if (i == destformat)
			{
				posx = (cols / 2) * charsize.cx;
				SetTextColor(hdc, RGB(255, 255, 255));
				TextOutShadow(hdc, posx, posy, _T("-->"), 3, RGB(0, 0, 192));
				posx = ((cols / 2) + 3) * charsize.cx;
				TextOutShadow(hdc, posx, posy, strSurfaceFormats[i + 1], _tcslen(strSurfaceFormats[i + 1]), RGB(0, 0, 192));
			}
			else
			{
				SetTextColor(hdc, RGB(192, 192, 192));
				posx = ((cols / 2) + 3) * charsize.cx;
				TextOut(hdc, posx, posy, strSurfaceFormats[i + 1], _tcslen(strSurfaceFormats[i + 1]));
			}
			posy += charsize.cy;
		}
	}
	// Display error if present
	if (error != 0)
	{
		SetBkMode(hdc, OPAQUE);
		SetBkColor(hdc, RGB(255, 0, 0));
		SetTextColor(hdc, RGB(255, 255, 255));
		if (errorlocation < 0) errorlocation = 0;
		if (errorlocation > 9) errorlocation = 0;
		posx = 0;
		posy = charsize.cy * (rows - 1);
		_tcscpy(buffer, strErrorMessages[errorlocation]);
		if ((errorlocation != 8) && (errorlocation != 9))
		{
			_tcscat(buffer, _T("0x"));
			_itot(error, number, 16);
			_tcscat(buffer, number);
		}
		TextOutShadow(hdc, posx, posy, buffer, _tcslen(buffer), RGB(192, 0, 0));
	}
	SelectObject(hdc, DefaultFont);
	DeleteObject(newfont);
	SetTextColor(hdc, oldcolor);
	SetBkColor(hdc, oldbkcolor);
	SetBkMode(hdc, oldbk);
	surface->ReleaseDC(hdc);
}