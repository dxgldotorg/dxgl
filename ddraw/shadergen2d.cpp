// DXGL
// Copyright (C) 2013 William Feely

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
#include "shadergen2d.h"
#include "../common/version.h"

/* Bits in 2D shader ID:
Bit 0:	Use destination alpha (DDBLT_ALPHADEST)
Bit 1:  Use dest. alpha constant (DDBLT_ALPHADESTCONSTOVERRIDE)
Bit 2:  Reverse dest. alpha (DDLBT_ALPHADESTNEG)
Bit 3:  Use dest. alpha surface (DDBLT_ALPHADESTSURFACEOVERRIDE)
Bit 4:  Use alpha edge blend (DDBLT_ALPHAEDGEBLEND)
Bit 5:  Use source alpha (DDBLT_ALPHASRC)
Bit 6:  Use soure alpha constant (DDBLT_ALPHASRCCONSTOVERRIDE)
Bit 7:  Reverse source alpha (DDBLT_ALPHASRCSURFACEOVERRIDE)
Bit 8:  Use source alpha surface (DDBLT_ALPHASRCSURFACEOVERRIDE)
Bit 9:  ROP index bit 0
Bit 10: Color fill (DDBLT_COLORFILL)
Bit 11: Use DDBLTFX (DDBLT_DDFX)
Bit 12: ROP index bit 1
Bit 13: Use destination color key (DDBLT_KEYDEST)
Bit 14: ROP index bit 2
Bit 15: Use source color key (DDBLT_KEYSRC)
Bit 16: ROP index bit 3
Bit 17: Use ROP (DDBLT_ROP, forces integer processing)
Bit 18: ROP index bit 4
Bit 19: Z-buffer blit (DDBLT_ZBUFFER)
Bit 20: Use dest. Z constant (DDBLT_ZBUFFERDESTCONSTOVERRIDE)
Bit 21: Use dest. Z surface (DDBLT_ZBUFFERDESTOVERRIDE)
Bit 22: Use source Z constant (DDBLT_ZBUFFERSRCCONSTOVERRIDE)
Bit 23: Use source Z surface (DDBLT_ZBUFFERSRCOVERRIDE)
Bit 24: ROP index bit 5
Bit 25:	Depth fill (DDBLT_DEPTHFILL)
Bit 26: ROP index bit 6
Bit 27: ROP index bit 7
Bit 28: reserved
Bit 29: reserved
Bit 30: reserved
Bit 31: reserved
AND the dwFlags by 0x02FAADFF before packing ROP index bits
*/

const DWORD valid_rop_codes[256] = {
	0x00000042,0x00010289,0x00020C89,0x000300AA,0x00040C88,0x000500A9,0x00060865,0x000702C5,
	0x00080F08,0x00090245,0x000A0329,0x000B0B2A,0x000C0324,0x000D0B25,0x000E08A5,0x000F0001,
	0x00100C85,0x001100A6,0x00120868,0x001302C8,0x00140869,0x001502C9,0x00165CCA,0x00171D54,
	0x00180D59,0x00191CC8,0x001A06C5,0x001B0768,0x001C06CA,0x001D0766,0x001E01A5,0x001F0385,
	0x00200F09,0x00210248,0x00220326,0x00230B24,0x00240D55,0x00251CC5,0x002606C8,0x00271868,
	0x00280369,0x002916CA,0x002A0CC9,0x002B1D58,0x002C0784,0x002D060A,0x002E064A,0x002F0E2A,
	0x0030032A,0x00310B28,0x00320688,0x00330008,0x003406C4,0x00351864,0x003601A8,0x00370388,
	0x0038078A,0x00390604,0x003A0644,0x003B0E24,0x003C004A,0x003D18A4,0x003E1B24,0x003F00EA,
	0x00400F0A,0x00410249,0x00420D5D,0x00431CC4,0x00440328,0x00450B29,0x004606C6,0x0047076A,
	0x00480368,0x004916C5,0x004A0789,0x004B0605,0x004C0CC8,0x004D1954,0x004E0645,0x004F0E25,
	0x00500325,0x00510B26,0x005206C9,0x00530764,0x005408A9,0x00550009,0x005601A9,0x00570389,
	0x00580785,0x00590609,0x005A0049,0x005B18A9,0x005C0649,0x005D0E29,0x005E1B29,0x005F00E9,
	0x00600365,0x006116C6,0x00620786,0x00630608,0x00640788,0x00650606,0x00660046,0x006718A8,
	0x006858A6,0x00690145,0x006A01E9,0x006B178A,0x006C01E8,0x006D1785,0x006E1E28,0x006F0C65,
	0x00700CC5,0x00711D5C,0x00720648,0x00730E28,0x00740646,0x00750E26,0x00761B28,0x007700E6,
	0x007801E5,0x00791786,0x007A1E29,0x007B0C68,0x007C1E24,0x007D0C69,0x007E0955,0x007F03C9,
	0x008003E9,0x00810975,0x00820C49,0x00831E04,0x00840C48,0x00851E05,0x008617A6,0x008701C5,
	0x008800C6,0x00891B08,0x008A0E06,0x008B0666,0x008C0E08,0x008D0668,0x008E1D7C,0x008F0CE5,
	0x00900C45,0x00911E08,0x009217A9,0x009301C4,0x009417AA,0x009501C9,0x00960169,0x0097588A,
	0x00981888,0x00990066,0x009A0709,0x009B07A8,0x009C0704,0x009D07A6,0x009E16E6,0x009F0345,
	0x00A000C9,0x00A11B05,0x00A20E09,0x00A30669,0x00A41885,0x00A50065,0x00A60706,0x00A707A5,
	0x00A803A9,0x00A90189,0x00AA0029,0x00AB0889,0x00AC0744,0x00AD06E9,0x00AE0B06,0x00AF0229,
	0x00B00E05,0x00B10665,0x00B21974,0x00B30CE8,0x00B4070A,0x00B507A9,0x00B616E9,0x00B70348,
	0x00B8074A,0x00B906E6,0x00BA0B09,0x00BB0226,0x00BC1CE4,0x00BD0D7D,0x00BE0269,0x00BF08C9,
	0x00C000CA,0x00C11B04,0x00C21884,0x00C3006A,0x00C40E04,0x00C50664,0x00C60708,0x00C707AA,
	0x00C803A8,0x00C90184,0x00CA0749,0x00CB06E4,0x00CC0020,0x00CD0888,0x00CE0B08,0x00CF0224,
	0x00D00E0A,0x00D1066A,0x00D20705,0x00D307A4,0x00D41D78,0x00D50CE9,0x00D616EA,0x00D70349,
	0x00D80745,0x00D906E8,0x00DA1CE9,0x00DB0D75,0x00DC0B04,0x00DD0228,0x00DE0268,0x00DF08C8,
	0x00E003A5,0x00E10185,0x00E20746,0x00E306EA,0x00E40748,0x00E506E5,0x00E61CE8,0x00E70D79,
	0x00E81D74,0x00E95CE6,0x00EA02E9,0x00EB0849,0x00EC02E8,0x00ED0848,0x00EE0086,0x00EF0A08,
	0x00F00021,0x00F10885,0x00F20B05,0x00F3022A,0x00F40B0A,0x00F50225,0x00F60265,0x00F708C5,
	0x00F802E5,0x00F90845,0x00FA0089,0x00FB0A09,0x00FC008A,0x00FD0A0A,0x00FE02A9,0x00FF0062
};

/*
1 - Source
2 - Dest
4 - Pattern */
const DWORD rop_texture_usage[256] = {
	0,7,7,5,7,6,7,7,7,7,6,7,5,7,7,4,
	7,3,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	7,7,3,7,7,7,7,7,7,7,7,7,7,7,7,7,
	5,7,7,1,7,7,7,7,7,7,7,7,5,7,7,5,
	7,7,7,7,3,7,7,7,7,7,7,7,7,7,7,7,
	6,7,7,7,7,2,7,7,7,7,6,7,7,7,7,6,
	7,7,7,7,7,7,3,7,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,3,7,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,3,7,7,7,7,7,7,7,
	7,7,7,7,7,7,7,7,7,3,7,7,7,7,7,7,
	6,7,7,7,7,6,7,7,7,7,2,7,7,7,7,6,
	7,7,7,7,7,7,7,7,7,7,7,3,7,7,7,7,
	5,7,7,5,7,7,7,7,7,7,7,7,1,7,7,5,
	7,7,7,7,7,7,7,7,7,7,7,7,7,3,7,7,
	7,7,7,7,7,7,7,7,7,7,7,7,7,7,3,7,
	4,7,7,5,7,6,7,7,7,7,6,7,5,7,7,0
};

const DWORD supported_rops[8] = {
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00001000,
	0x00000000
};

static const char header[] =
	"//REV" STR(SHADER2DVERSION) "\n\
#version 110\n";
static const char vertexshader[] = "//2D Vertex Shader\n";
static const char fragshader[] = "//2D Fragment Shader\n";
static const char idheader[] = "//ID: 0x";
static const char linefeed[] = "\n";
static const char mainstart[] = "void main()\n{\n";
static const char mainend[] = "} ";
// Attributes
static const char attr_srcxy[] = "attribute vec2 srcxy;\n";
static const char attr_destxy[] = "attribute vec2 destxy;\n";
static const char attr_patternxy[] = "attribute vec2 patternxy;\n";

// Uniforms
static const char var_srctex[] = "uniform sampler2d srctex;";
static const char var_desttex[] = "uniform sampler2d desttex;";
static const char var_patterntex[] = "uniform sampler2d patterntex;";

// Variables
static const char var_src[] = "ivec4 src;\n";
static const char var_dest[] = "ivec4 dest;\n";
static const char var_pattern[] = "ivec4 pattern;\n";

// Operations
static const char op_src[] = "src = ivec4(texture2D(src,gl_TexCoord[0].st)*255.5);\n";
static const char op_dest[] = "dest = ivec4(texture2D(dest,gl_TexCoord[1].st)*255.5);\n";
static const char op_pattern[] = "pattern = ivec4(texture2D(pattern,gl_TexCoord[2].st)*255.5);\n";
static const char op_destout[] = "gl_FragColor = vec4(dest)/255.5;\n";


// Functions

// ROP Operations
static const char *op_ROP[256] = {
"dest = ivec4(0);",//00 BLACKNESS
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//0F
"",//10
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//1F
"",//20
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//2F
"",//30
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//3F
"",//40
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//4F
"",//50
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//5F
"",//60
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//6F
"",//70
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//7F
"",//80
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//8F
"",//90
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//9F
"",//A0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//AF
"",//B0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//BF
"",//C0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"dest = src;\n",//CC SRCCOPY
"",
"",
"",//CF
"",//D0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//DF
"",//E0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",//EF
"",//F0
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"dest = ivec4(255);\n",//FF WHITENESS
};