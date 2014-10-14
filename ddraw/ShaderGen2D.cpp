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
#include "string.h"
#include "glExtensions.h"
#include "ShaderGen2d.h"
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
Bit 17: Use ROP (DDBLT_ROP, may force integer processing)
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
Bit 28: (DXGL) Use Clipper
Bit 29: (DXGL) Source color key range
Bit 30: (DXGL) Dest. color key range
Bit 31: reserved for DXGL usage
AND the dwFlags by 0xF2FAADFF before packing ROP index bits
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
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0x08000400,
	0x00001001,
	0x88014000
};

const DWORD supported_rops_gl2[8] = {
	0x00008001,
	0x00080000,
	0x00200000,
	0x00000000,
	0x00000000,
	0x00000400,
	0x00001000,
	0x80010000
};

static const char revheader[] =
	"//REV" STR(SHADER2DVERSION) "\n";
static const char version_110[] = "#version 110\n";
static const char version_130[] = "#version 130\n";
static const char ext_shader4[] = "#extension GL_EXT_gpu_shader4 : require";
static const char vertexshader[] = "//2D Vertex Shader\n";
static const char fragshader[] = "//2D Fragment Shader\n";
static const char idheader[] = "//ID: 0x";
static const char linefeed[] = "\n";
static const char mainstart[] = "void main()\n{\n";
static const char mainend[] = "} ";
// Attributes
static const char attr_xy[] = "attribute vec2 xy;\n";
static const char attr_rgb[] = "attribute vec3 rgb;\n";
static const char attr_rgba[] = "attribute vec4 rgba;\n";
static const char attr_srcst[] = "attribute vec2 srcst;\n";
static const char attr_destst[] = "attribute vec2 destst;\n";
static const char attr_stencilst[] = "attribute vec2 stencilst;\n";

// Uniforms
static const char unif_view[] = "uniform vec4 view;\n";
static const char unif_srctex[] = "uniform sampler2D srctex;\n";
static const char unif_desttex[] = "uniform sampler2D desttex;\n";
static const char unif_patterntex[] = "uniform sampler2D patterntex;\n";
static const char unif_stenciltex[] = "uniform sampler2D stenciltex;\n";
static const char unif_ckeysrc[] = "uniform ivec3 ckeysrc;\n";
static const char unif_ckeydest[] = "uniform ivec3 ckeydest;\n";
static const char unif_ckeysrchigh[] = "uniform ivec3 ckeysrchigh;\n";
static const char unif_ckeydesthigh[] = "uniform ivec3 ckeydesthigh;\n";
static const char unif_patternsize[] = "uniform ivec2 patternsize;\n";
static const char unif_colorsizesrc[] = "uniform ivec4 colorsizesrc;\n";
static const char unif_colorsizedest[] = "uniform ivec4 colorsizedest;\n";
static const char unif_fillcolor[] = "uniform ivec4 fillcolor;\n";

// Variables
static const char var_dest[] = "ivec4 dest;\n";
static const char var_pattern[] = "ivec4 pattern;\n";
static const char var_pixel[] = "ivec4 pixel;\n";
static const char var_src[] = "ivec4 src;\n";
static const char var_patternst[] = "vec2 patternst;\n";

// Operations
static const char op_src[] = "src = ivec4(texture2D(srctex,gl_TexCoord[0].st)*vec4(colorsizesrc)+.5);\n";
static const char op_pixel[] = "pixel = ivec4(texture2D(srctex,gl_TexCoord[0].st)*vec4(colorsizedest)+.5);\n";
static const char op_color[] = "pixel = fillcolor;\n";
static const char op_dest[] = "dest = ivec4(texture2D(desttex,gl_TexCoord[1].st)*vec4(colorsizedest)+.5);\n";
static const char op_pattern[] = "patternst = vec2(mod(gl_FragCoord.x,float(patternsize.x))/float(patternsize.x),\n\
mod(gl_FragCoord.y, float(patternsize.y)) / float(patternsize.y));\n\
pattern = ivec4(texture2D(patterntex,patternst)*vec4(colorsizedest)+.5);\n";
static const char op_destout[] = "gl_FragColor = vec4(pixel)/vec4(colorsizedest);\n";
static const char op_vertex[] = "vec4 xyzw = vec4(xy[0],xy[1],0,1);\n\
mat4 proj = mat4(\n\
vec4(2.0 / (view[1] - view[0]), 0, 0, 0),\n\
vec4(0, 2.0 / (view[2] - view[3]), 0, 0),\n\
vec4(0, 0, -2.0, 0),\n\
vec4(-(view[1] + view[0]) / (view[1] - view[0]),\n\
-(view[2] + view[3]) / (view[2] - view[3]), -1 , 1));\n\
gl_Position    = proj * xyzw;\n";
static const char op_vertcolorrgb[] = "gl_FrontColor = vec4(rgb,1.0);\n";
static const char op_texcoord0[] = "gl_TexCoord[0] = vec4(srcst,0.0,1.0);\n";
static const char op_texcoord1[] = "gl_TexCoord[1] = vec4(destst,0.0,1.0);\n";
static const char op_texcoord3[] = "gl_TexCoord[3] = vec4(stencilst,0.0,1.0);\n";
static const char op_ckeysrc[] = "if(src.rgb == ckeysrc) discard;\n";
static const char op_ckeydest[] = "if(dest.rgb != ckeydest) discard;\n";
static const char op_ckeysrcrange[] = "if(!((src.r < ckeysrc.r) || (src.g < ckeysrc.g) || (src.b < ckeysrc.b) ||\
   (src.r > ckeysrchigh.r) || (src.g > ckeysrchigh.g) || (src.b > ckeysrchigh.b))) discard;\n";
static const char op_ckeydestrange[] = "if((dest.r < ckeydest.r) || (dest.g < ckeydest.g) || (dest.b < ckeydest.b) ||\
   (dest.r > ckeydesthigh.r) || (dest.g > ckeydesthigh.g) || (dest.b > ckeydesthigh.b)) discard;\n";
static const char op_clip[] = "if(texture2D(stenciltex, gl_TexCoord[3].st).r < .5) discard;\n";

// Functions

// ROP Operations
static const char *op_ROP[256] = {
"pixel = ivec4(0);\n",//00 BLACKNESS
"pixel = (dest | (pattern | pixel)) ^ colorsizedest;\n",
"pixel = dest & ((pattern | pixel) ^ colorsizedest);\n",
"pixel = (pattern | pixel) ^ colorsizedest;\n",
"pixel = pixel & ((dest | pattern) ^ colorsizedest);\n",
"pixel = (dest | pattern) ^ colorsizedest;\n",
"pixel = (pattern | ((dest ^ pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = (pattern | (dest & pixel)) ^ colorsizedest;\n",
"pixel = pixel & dest & (pattern ^ colorsizedest);\n",
"pixel = (pattern | (dest ^ pixel)) ^ colorsizedest;\n",
"pixel = dest & (pattern ^ colorsizedest);\n",
"pixel = (pattern | (pixel & (dest ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = pixel & (pattern ^ colorsizedest);\n",
"pixel = (pattern | (dest & (pixel ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = (pattern | ((dest | pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = pattern ^ colorsizedest;\n",//0F
"pixel = pattern & ((pixel | dest) ^ colorsizedest);\n",//10
"pixel = (dest | pixel) ^ colorsizedest;\n",//11 NOTSRCERASE
"pixel = (pixel | ((dest ^ pattern) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = (pixel | (dest & pattern)) ^ colorsizedest;\n",
"pixel = (dest | ((pattern ^ pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = (dest | (pattern & pixel)) ^ colorsizedest;\n",
"pixel = pattern ^ pixel ^ (dest & ((pattern & pixel) ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ ((pixel ^ pattern) & (pixel ^ dest));\n",
"pixel = (pixel ^ pattern) & (dest ^ pattern);\n",
"pixel = (pixel ^ colorsizedest) ^ (dest & ((pattern & pixel) ^ colorsizedest));\n",
"pixel = pattern ^ (dest | (pixel & pattern));\n",
"pixel = (pixel ^ colorsizedest) ^ (dest & (pattern ^ pixel));\n",
"pixel = pattern ^ (pixel | (dest & pattern));\n",
"pixel = (dest ^ colorsizedest) ^ (pixel & (dest ^ pattern));\n",
"pixel = pattern ^ (dest | pixel);\n",
"pixel = (pattern & (dest | pixel)) ^ colorsizedest;\n",//1F
"pixel = dest & (pattern & (pixel ^ colorsizedest));\n",//20
"pixel = (pixel | (dest ^ pattern)) ^ colorsizedest;\n",
"pixel = (pixel ^ colorsizedest) & dest;\n",
"pixel = (pixel | (pattern & (dest ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = (pixel ^ pattern) & (pixel ^ dest);\n",
"pixel = (pattern ^ colorsizedest) ^ (dest & ((pixel & pattern) ^ colorsizedest));\n",
"pixel = pixel ^ (dest | (pixel & pattern));\n",
"pixel = pixel ^ (dest | ((pattern ^ pixel) ^ colorsizedest));\n",
"pixel = dest & (pattern ^ pixel);\n",
"pixel = (pattern ^ colorsizedest) ^ pixel ^ (dest | (pattern & pixel));\n",
"pixel = dest & ((pattern & pixel) ^ colorsizedest);\n",
"pixel = (pixel ^ colorsizedest) ^ ((pattern ^ pixel) & (pattern ^ dest));\n",
"pixel = pixel ^ (pattern & (pixel | dest));\n",
"pixel = pattern ^ (pixel | (dest ^ colorsizedest));\n",
"pixel = pattern ^ (pixel | (dest ^ pattern));\n",
"pixel = (pattern & (pixel | (dest ^ pattern)))^ colorsizedest;\n",//2F
"pixel = pattern & (pixel ^ colorsizedest);\n",//30
"pixel = (pixel | (dest & (pattern ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = pixel ^ (dest | pattern | pixel);\n",
"pixel = pixel ^ colorsizedest;\n",//33 NOTSRCCOPY
"pixel = pixel ^ (pattern | (dest & pixel));\n",
"pixel = pixel ^ (pattern | ((dest ^ pixel) ^ colorsizedest));\n",
"pixel = pixel ^ (dest | pattern);\n",
"pixel = (pixel & (dest | pattern)) ^ colorsizedest;\n",
"pixel = pattern ^ (pixel & (dest | pattern));\n",
"pixel = pixel ^ (pattern | (dest ^ colorsizedest));\n",
"pixel = pixel ^ (pattern | (dest ^ pixel));\n",
"pixel = (pixel & (pattern | (dest ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = pattern ^ pixel;\n",
"pixel = pixel ^ (pattern | ((dest | pixel) ^ colorsizedest));\n",
"pixel = pixel ^ (pattern | (dest & (pixel ^ colorsizedest)));\n",
"pixel = (pattern & pixel) ^ colorsizedest;\n",//3F
"pixel = pattern & pixel & (dest ^ colorsizedest);\n",//40
"pixel = (dest | (pattern ^ pixel)) ^ colorsizedest;\n",
"pixel = (pixel ^ dest) & (pattern ^ dest);\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern & ((dest & pixel) ^ colorsizedest));\n",
"pixel = pixel & (dest ^ colorsizedest);\n",//44 SRCERASE
"pixel = (dest | (pattern & (pixel ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = dest ^ (pixel | (pattern & dest));\n",
"pixel = (pattern ^ colorsizedest) ^ (pixel & (dest ^ pattern));\n",
"pixel = pixel & (pattern ^ dest);\n",
"pixel = (pattern ^ colorsizedest) ^ dest ^ (pixel | (pattern & dest));\n",
"pixel = dest ^ (pattern & (pixel | dest));\n",
"pixel = pattern ^ (dest | (pixel ^ colorsizedest));\n",
"pixel = pixel & ((dest & pattern) ^ colorsizedest);\n",
"pixel = (pixel ^ colorsizedest) ^ ((pixel ^ pattern) | (pixel ^ dest));\n",
"pixel = pattern ^ (dest | (pixel ^ pattern));\n",
"pixel = (pattern & (dest | (pixel ^ colorsizedest))) ^ colorsizedest;\n",//4F
"pixel = pattern & (dest ^ colorsizedest);\n",//50
"pixel = (dest | (pixel & (pattern ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = dest ^ (pattern | (pixel & dest));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern & (dest ^ pixel));\n",
"pixel = (dest | ((pattern | pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = dest ^ colorsizedest;\n",//55 DSTINVERT
"pixel = dest ^ (pattern | pixel);\n",
"pixel = (dest & (pattern | pixel)) ^ colorsizedest;\n",
"pixel = pattern ^ (dest & (pattern | pixel));\n",
"pixel = dest ^ (pattern | (pixel ^ colorsizedest));\n",
"pixel = dest ^ pattern;\n",//5A PATINVERT
"pixel = dest ^ (pattern | ((pixel | dest) ^ colorsizedest));\n",
"pixel = dest ^ (pattern | (pixel ^ dest));\n",
"pixel = (dest & (pattern | (pixel ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = dest ^ (pattern | (pixel & (dest ^ colorsizedest)));\n",
"pixel = (dest & pattern) ^ colorsizedest;\n",//5F
"pixel = pattern & (dest ^ pixel);\n",//60
"pixel = (dest ^ colorsizedest) ^ pixel ^ (pattern | (dest & pixel));\n",
"pixel = dest ^ (pixel & (pattern | dest));\n",
"pixel = pixel ^ (dest | (pattern ^ colorsizedest));\n",
"pixel = pixel ^ (dest & (pattern | pixel));\n",
"pixel = dest ^ (pixel | (pattern ^ colorsizedest));\n",
"pixel = pixel ^ dest;\n",//66 SRCINVERT
"pixel = pixel ^ (dest | ((pixel | pattern) ^ colorsizedest));\n",
"pixel = (dest ^ colorsizedest) ^ pixel ^ (pattern | ((dest | pixel) ^ colorsizedest));\n",
"pixel = (pattern ^ colorsizedest) ^ (dest ^ pixel);\n",
"pixel = dest ^ (pattern & pixel);\n",
"pixel = (pattern ^ colorsizedest) ^ pixel ^ (dest & (pattern | pixel));\n",
"pixel = pixel ^ (dest & pattern);\n",
"pixel = (pattern ^ colorsizedest) ^ dest ^ (pixel & (pattern | dest));\n",
"pixel = pixel ^ (dest & (pattern | (pixel ^ colorsizedest)));\n",
"pixel = (pattern & ((pixel ^ dest) ^ colorsizedest)) ^ colorsizedest;\n",//6F
"pixel = pattern & ((dest & pixel) ^ colorsizedest);\n",//70
"pixel = (pixel ^ colorsizedest) ^ ((pixel ^ dest) & (pattern ^ dest));\n",
"pixel = pixel ^ (dest | (pattern ^ pixel));\n",
"pixel = (pixel & (dest | (pattern ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = dest ^ (pixel | (pattern ^ dest));\n",
"pixel = (dest & (pixel | (pattern ^ colorsizedest))) ^ colorsizedest;\n",
"pixel = pixel ^ (dest | (pattern & (pixel ^ colorsizedest)));\n",
"pixel = (pixel & dest) ^ colorsizedest;\n",
"pixel = pattern ^ (dest & pixel);\n",
"pixel = (dest ^ colorsizedest) ^ pixel ^ (pattern & (dest | pixel));\n",
"pixel = dest ^ (pattern & (pixel | (dest ^ colorsizedest)));\n",
"pixel = (pixel & ((dest ^ pattern) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = pixel ^ (pattern & (dest | (pixel ^ colorsizedest)));\n",
"pixel = (dest & ((pattern ^ pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = (pixel ^ pattern) | (pattern ^ dest);\n",
"pixel = (dest & pattern & pixel) ^ colorsizedest;\n",//7F
"pixel = dest & pattern & pixel;\n",//80
"pixel = ((pixel ^ pattern) | (pixel ^ dest)) ^ colorsizedest;\n",
"pixel = dest & ((pattern ^ pixel) ^ colorsizedest);\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern & (dest | (pixel ^ colorsizedest)));\n",
"pixel = pixel & ((dest ^ pattern) ^ colorsizedest);\n",
"pixel = (pattern ^ colorsizedest) ^ (dest & (pixel | (pattern ^ colorsizedest)));\n",
"pixel = dest ^ pixel ^ (pattern & (dest | pixel));\n",
"pixel = (pattern ^ colorsizedest) ^ (dest & pixel);\n",
"pixel = pixel & dest;\n",//88 SRCAND
"pixel = (pixel ^ colorsizedest) ^ (dest | (pattern & (pixel ^ colorsizedest)));\n",
"pixel = dest & (pixel | (pattern ^ colorsizedest));\n",
"pixel = (dest ^ colorsizedest) ^ (pixel | (pattern ^ dest));\n",
"pixel = pixel & (dest | (pattern ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ (dest | (pattern ^ pixel));\n",
"pixel = pixel ^ ((pixel ^ dest) & (pattern ^ dest));\n",
"pixel = (pattern & ((dest & pixel) ^ colorsizedest)) ^ colorsizedest;\n",//8F
"pixel = pattern & ((dest ^ pixel) ^ colorsizedest);\n",//90
"pixel = (pixel ^ colorsizedest) ^ (dest & (pattern | (pixel ^ colorsizedest)));\n",
"pixel = dest ^ pattern ^ (pixel & (dest | pattern));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern & dest);\n",
"pixel = pixel ^ pattern ^ (dest & (pattern | pixel));\n",
"pixel = (dest ^ colorsizedest) ^ (pattern & pixel);\n",
"pixel = dest ^ pattern ^ pixel;\n",
"pixel = pixel ^ pattern ^ (dest | ((pattern | pixel) ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ (dest | ((pattern | pixel) ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ dest;\n",
"pixel = dest ^ (pattern & (pixel ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ (dest & (pattern | pixel));\n",
"pixel = pixel ^ (pattern & (dest ^ colorsizedest));\n",
"pixel = (dest ^ colorsizedest) ^ (pixel & (pattern | dest));\n",
"pixel = dest ^ pixel ^ (pattern | (dest & pixel));\n",
"pixel = (pattern & (dest ^ pixel)) ^ colorsizedest;\n",//9F
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
"pixel = dest;\n",
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
"pixel = dest | (pixel ^ colorsizedest);\n",//BB MERGEPAINT
"",
"",
"",
"",//BF
"pixel = pixel & pattern;\n",//C0 MERGECOPY
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
"",//CC SRCCOPY  pixel=pixel
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
"pixel = pixel | dest;\n",//EE SRCPAINT
"",//EF
"pixel = pattern;\n",//F0 PATCOPY
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
"pixel = dest | pattern | (pixel ^ colorsizedest);\n",//FB PATPAINT
"",
"",
"",
"pixel = colorsizedest;\n",//FF WHITENESS
};

static const char *op_ROP_float[256] = {
"pixel = ivec4(0);\n",//00 BLACKNESS
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
"pixel = colorsizedest - pattern;\n",//0F
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
"pixel = colorsizedest - pixel;\n",// 33 NOTSRCCOPY
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
"pixel = colorsizedest - dest;\n",//55 DSTINVERT
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
"pixel = dest;\n",
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
"",//CC SRCCOPY  pixel=pixel
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
"pixel = pattern;\n",//F0 PATCOPY
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
"pixel = colorsizedest;\n",//FF WHITENESS
};

DWORD PackROPBits(DWORD rop, DWORD flags)
{
	DWORD out = flags & 0xF2FAADFF;
	if (rop & 0x10000) out |= 1 << 9;
	if (rop & 0x20000) out |= 1 << 12;
	if (rop & 0x40000) out |= 1 << 14;
	if (rop & 0x80000) out |= 1 << 16;
	if (rop & 0x100000) out |= 1 << 18;
	if (rop & 0x200000) out |= 1 << 24;
	if (rop & 0x400000) out |= 1 << 26;
	if (rop & 0x800000) out |= 1 << 27;
	return out;
}

DWORD UnpackROPBits(DWORD flags)
{
	DWORD out = 0;
	if (flags & (1 << 9)) out |= 1;
	if (flags & (1 << 12)) out |= 2;
	if (flags & (1 << 14)) out |= 4;
	if (flags & (1 << 16)) out |= 8;
	if (flags & (1 << 18)) out |= 16;
	if (flags & (1 << 24)) out |= 32;
	if (flags & (1 << 26)) out |= 64;
	if (flags & (1 << 27)) out |= 128;
	return out;
}

void ShaderGen2D_Init(ShaderGen2D *gen, glExtensions *ext, ShaderManager *shaderman)
{
	gen->ext = ext;
	gen->shaders = shaderman;
	gen->shadercount = 0;
	gen->genindex = 0;
	gen->maxshaders = 256;
	gen->genshaders2D = (GenShader2D *)malloc(256 * sizeof(GenShader2D));
	ZeroMemory(gen->genshaders2D, 256 * sizeof(GenShader2D));
}

void ShaderGen2D_Delete(ShaderGen2D *gen)
{
	if (!gen->genshaders2D) return;
	for (int i = 0; i < gen->shadercount; i++)
	{
		gen->genshaders2D[i].id = 0;
		if (gen->genshaders2D[i].shader.prog) gen->ext->glDeleteProgram(gen->genshaders2D[i].shader.prog);
		if (gen->genshaders2D[i].shader.fs) gen->ext->glDeleteShader(gen->genshaders2D[i].shader.fs);
		if (gen->genshaders2D[i].shader.vs) gen->ext->glDeleteShader(gen->genshaders2D[i].shader.vs);
		if (gen->genshaders2D[i].shader.fsrc.ptr) String_Free(&gen->genshaders2D[i].shader.fsrc);
		if (gen->genshaders2D[i].shader.vsrc.ptr) String_Free(&gen->genshaders2D[i].shader.vsrc);
	}
	if (gen->genshaders2D) free(gen->genshaders2D);
	gen->genshaders2D = NULL;
	gen->shadercount = 0;
	gen->genindex = 0;
}

void ShaderGen2D_CreateShader2D(ShaderGen2D *gen, int index, DWORD id)
{
	STRING tmp;
	DWORD rop;
	tmp.ptr = NULL;
	BOOL intproc = FALSE;
	BOOL usedest = FALSE;
	gen->genshaders2D[index].shader.vsrc.ptr = NULL;
	gen->genshaders2D[index].shader.fsrc.ptr = NULL;
	char idstring[22];
	_snprintf(idstring, 21, "%0.8I32X\n", id);
	idstring[21] = 0;
	// Create vertex shader
	// Header
	STRING *vsrc = &gen->genshaders2D[index].shader.vsrc;
	String_Append(vsrc, revheader);
	if (id & DDBLT_ROP)
	{
		rop = UnpackROPBits(id);
		if (gen->ext->glver_major >= 3)
		{
			String_Append(vsrc, version_130);
			intproc = TRUE;
		}
		else if (gen->ext->GLEXT_EXT_gpu_shader4)
		{
			String_Append(vsrc, version_110);
			String_Append(vsrc, ext_shader4);
			intproc = TRUE;
		}
		else String_Append(vsrc, version_110);
	}
	else String_Append(vsrc, version_110);
	String_Append(vsrc, idheader);
	String_Append(vsrc, idstring);

	// Attributes
	String_Append(vsrc, attr_xy);
	if (!(id & DDBLT_COLORFILL)) String_Append(vsrc, attr_srcst);
	if (id & DDBLT_ROP)
	{
		if (rop_texture_usage[rop] & 2) usedest = TRUE;
	}
	if (id & DDBLT_KEYDEST) usedest = TRUE;
	if (usedest) String_Append(vsrc, attr_destst);
	if (id & 0x10000000) String_Append(vsrc, attr_stencilst);

	// Uniforms
	String_Append(vsrc, unif_view);

	// Main
	String_Append(vsrc, mainstart);
	String_Append(vsrc, op_vertex);
	if (!(id & DDBLT_COLORFILL)) String_Append(vsrc, op_texcoord0);
	if(usedest) String_Append(vsrc, op_texcoord1);
	if (id & 0x10000000) String_Append(vsrc, op_texcoord3);
	String_Append(vsrc, mainend);
#ifdef _DEBUG
	OutputDebugStringA("2D blitter vertex shader:\n");
	OutputDebugStringA(vsrc->ptr);
	OutputDebugStringA("\nCompiling 2D blitter vertex shader:\n");
	TRACE_STRING("2D blitter vertex shader:\n");
	TRACE_STRING(vsrc->ptr);
	TRACE_STRING("\nCompiling 2D blitter vertex shader:\n");
#endif
	gen->genshaders2D[index].shader.vs = gen->ext->glCreateShader(GL_VERTEX_SHADER);
	GLint srclen = strlen(vsrc->ptr);
	gen->ext->glShaderSource(gen->genshaders2D[index].shader.vs, 1, &vsrc->ptr, &srclen);
	gen->ext->glCompileShader(gen->genshaders2D[index].shader.vs);
	GLint result;
	char *infolog = NULL;
	gen->ext->glGetShaderiv(gen->genshaders2D[index].shader.vs, GL_COMPILE_STATUS, &result);
#ifdef _DEBUG
	GLint loglen;
	if (!result)
	{
		gen->ext->glGetShaderiv(gen->genshaders2D[index].shader.vs, GL_INFO_LOG_LENGTH, &loglen);
		infolog = (char*)malloc(loglen);
		gen->ext->glGetShaderInfoLog(gen->genshaders2D[index].shader.vs, loglen, &result, infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Compilation failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
#endif
	usedest = FALSE;
	// Create fragment shader
	STRING *fsrc = &gen->genshaders2D[index].shader.fsrc;
	String_Append(fsrc, revheader);
	if (id & DDBLT_ROP)
	{
		if (gen->ext->glver_major >= 3)
		{
			String_Append(fsrc, version_130);
			intproc = true;
		}
		else if (gen->ext->GLEXT_EXT_gpu_shader4)
		{
			String_Append(fsrc, version_110);
			String_Append(fsrc, ext_shader4);
			intproc = true;
		}
		else String_Append(fsrc, version_110);
	}
	else String_Append(fsrc, version_110);
	String_Append(fsrc, idheader);
	String_Append(fsrc, idstring);

	// Uniforms
	if (id & DDBLT_COLORFILL) String_Append(fsrc, unif_fillcolor);
	else String_Append(fsrc, unif_srctex);
	if (id & DDBLT_KEYDEST) usedest = TRUE;
	if (id & DDBLT_ROP)
	{
		if (rop_texture_usage[rop] & 2) usedest = TRUE;
		if (rop_texture_usage[rop] & 4)
		{
			String_Append(fsrc, unif_patterntex);
			String_Append(fsrc, unif_patternsize);
		}
	}
	if (usedest) String_Append(fsrc, unif_desttex);
	if (id & 0x10000000) String_Append(fsrc, unif_stenciltex);
	if (id & DDBLT_KEYSRC)
	{
		String_Append(fsrc, unif_ckeysrc);
		if (id & 0x20000000) String_Append(fsrc, unif_ckeysrchigh);
		String_Append(fsrc, unif_colorsizesrc);
	}
	String_Append(fsrc, unif_colorsizedest);
	if (id & DDBLT_KEYDEST)
	{
		String_Append(fsrc, unif_ckeydest);
		if (id & 0x40000000) String_Append(fsrc, unif_ckeydesthigh);
	}

	// Variables
	String_Append(fsrc, var_pixel);
	if (id & DDBLT_KEYSRC) String_Append(fsrc, var_src);
	if (id & DDBLT_ROP)
	{
		if (rop_texture_usage[rop] & 4)
		{
			String_Append(fsrc, var_pattern);
			String_Append(fsrc, var_patternst);
		}
	}
	if (usedest) String_Append(fsrc, var_dest);

	// Main
	String_Append(fsrc, mainstart);
	if (id & 0x10000000) String_Append(fsrc, op_clip);
	if (id & DDBLT_COLORFILL) String_Append(fsrc, op_color);
	else String_Append(fsrc, op_pixel);
	if (id & DDBLT_KEYSRC) String_Append(fsrc, op_src);
	if (usedest) String_Append(fsrc, op_dest);
	if (id & DDBLT_KEYSRC)
	{
		if (id & 0x20000000) String_Append(fsrc, op_ckeysrcrange);
		else String_Append(fsrc, op_ckeysrc);
	}
	if (id & DDBLT_KEYDEST)
	{
		if (id & 0x40000000) String_Append(fsrc, op_ckeydestrange);
		else String_Append(fsrc, op_ckeydest);
	}
	if (id & DDBLT_ROP)
	{
		if (rop_texture_usage[rop] & 4) String_Append(fsrc, op_pattern);
		if (intproc) String_Append(fsrc, op_ROP[rop]);
		else String_Append(fsrc, op_ROP_float[rop]);
	}

	String_Append(fsrc, op_destout);
	String_Append(fsrc, mainend);
#ifdef _DEBUG
	OutputDebugStringA("2D blitter fragment shader:\n");
	OutputDebugStringA(fsrc->ptr);
	OutputDebugStringA("\nCompiling 2D blitter fragment shader:\n");
	TRACE_STRING("2D blitter fragment shader:\n");
	TRACE_STRING(fsrc->ptr);
	TRACE_STRING("\nCompiling 2D blitter fragment shader:\n");
#endif
	gen->genshaders2D[index].shader.fs = gen->ext->glCreateShader(GL_FRAGMENT_SHADER);
	srclen = strlen(fsrc->ptr);
	gen->ext->glShaderSource(gen->genshaders2D[index].shader.fs, 1, &fsrc->ptr, &srclen);
	gen->ext->glCompileShader(gen->genshaders2D[index].shader.fs);
	gen->ext->glGetShaderiv(gen->genshaders2D[index].shader.fs, GL_COMPILE_STATUS, &result);
#ifdef _DEBUG
	if (!result)
	{
		gen->ext->glGetShaderiv(gen->genshaders2D[index].shader.fs, GL_INFO_LOG_LENGTH, &loglen);
		infolog = (char*)malloc(loglen);
		gen->ext->glGetShaderInfoLog(gen->genshaders2D[index].shader.fs, loglen, &result, infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Compilation failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
#endif
	gen->genshaders2D[index].shader.prog = gen->ext->glCreateProgram();
	gen->ext->glAttachShader(gen->genshaders2D[index].shader.prog, gen->genshaders2D[index].shader.vs);
	gen->ext->glAttachShader(gen->genshaders2D[index].shader.prog, gen->genshaders2D[index].shader.fs);
	gen->ext->glLinkProgram(gen->genshaders2D[index].shader.prog);
	gen->ext->glGetProgramiv(gen->genshaders2D[index].shader.prog, GL_LINK_STATUS, &result);
#ifdef _DEBUG
	if (!result)
	{
		gen->ext->glGetProgramiv(gen->genshaders2D[index].shader.prog, GL_INFO_LOG_LENGTH, &loglen);
		infolog = (char*)malloc(loglen);
		gen->ext->glGetProgramInfoLog(gen->genshaders2D[index].shader.prog, loglen, &result, infolog);
		OutputDebugStringA("Program link failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Program link failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
#endif
	gen->genshaders2D[index].shader.attribs[0] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "xy");
	gen->genshaders2D[index].shader.attribs[1] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "rgb");
	gen->genshaders2D[index].shader.attribs[2] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "rgba");
	gen->genshaders2D[index].shader.attribs[3] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "srcst");
	gen->genshaders2D[index].shader.attribs[4] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "destst");
	gen->genshaders2D[index].shader.attribs[5] = gen->ext->glGetAttribLocation(gen->genshaders2D[index].shader.prog, "stencilst");
	gen->genshaders2D[index].shader.uniforms[0] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "view");
	gen->genshaders2D[index].shader.uniforms[1] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "srctex");
	gen->genshaders2D[index].shader.uniforms[2] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "desttex");
	gen->genshaders2D[index].shader.uniforms[3] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "patterntex");
	gen->genshaders2D[index].shader.uniforms[4] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "stenciltex");
	gen->genshaders2D[index].shader.uniforms[5] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "ckeysrc");
	gen->genshaders2D[index].shader.uniforms[6] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "ckeydest");
	gen->genshaders2D[index].shader.uniforms[7] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "ckeysrchigh");
	gen->genshaders2D[index].shader.uniforms[8] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "ckeydesthigh");
	gen->genshaders2D[index].shader.uniforms[9] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "patternsize");
	gen->genshaders2D[index].shader.uniforms[10] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "colorsizesrc");
	gen->genshaders2D[index].shader.uniforms[11] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "colorsizedest");
	gen->genshaders2D[index].shader.uniforms[12] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "fillcolor");

}