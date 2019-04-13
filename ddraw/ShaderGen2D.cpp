// DXGL
// Copyright (C) 2013-2019 William Feely

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

// Contains some GLSL code derived from Nervous Systems ffmpeg-opengl YUV feature,
// original code from:
// https://github.com/nervous-systems/ffmpeg-opengl/blob/feature/yuv/vf_genericshader.c
// Original code was released under the Unlicense and thus dedicated to the public
// domain, as evidenced by the following repository license:
// https://github.com/nervous-systems/ffmpeg-opengl/blob/feature/yuv/LICENSE

#include "common.h"
#include "string.h"
#include "glExtensions.h"
#include "ShaderGen2d.h"
#include "../common/version.h"
extern "C" {extern DXGLCFG dxglcfg; }


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
Bits 32-39: Texture type input
Bits 40-47: Texture type output
AND the dwFlags by 0xF2FAADFF before packing ROP index bits

Texture types:
0x00: Standard RGBA
0x01: Luminance-only (write to red)
0x02: Luminance-only (Calculate Y on Blt)
0x10: 8-bit palette
0x11: 4-bit palette
0x12: 2-bit palette
0x13: 1-bit palette
0x14: 4-bit palette index (future)
0x15: 2-bit palette index (future)
0x16: 1-bit palette index (future)
0x18: 24-bit Depth
0x19: 24-bit Depth, 8-bit Stencil
0x20: 16-bit RGBG
0x21: 16-bit GRGB
0x80: UYVY / UYNV / Y422
0x81: YUY2 / YUYV / YUNV
0x82: YVYU
0x83: AYUV
0xC0: (first entry for compressed)			 (future)
*/

static const char revheader[] =
	"//REV" STR(SHADER2DVERSION) "\n";
static const char version_110[] = "#version 110\n";
static const char version_130[] = "#version 130\n";
static const char ext_shader4[] = "#extension GL_EXT_gpu_shader4 : require\n";
static const char ext_texrect[] = "#extension GL_ARB_texture_rectangle : require\n";
static const char vertexshader[] = "//2D Vertex Shader\n";
static const char fragshader[] = "//2D Fragment Shader\n";
static const char idheader[] = "//ID: 0x";
static const char linefeed[] = "\n";
static const char mainstart[] = "void main()\n{\n";
static const char mainend[] = "} ";

// Constants
static const char const_bt601_coeff[] = 
"const mat3 bt601_coeff = mat3(1.164,1.164,1.164,0.0,-0.392,2.017,1.596,-0.813,0.0);\n\
const vec3 yuv_offsets = vec3(-0.0625, -0.5, -0.5);\n";
static const char const_bt601_coeff_inv[] =
"const mat3 bt601_coeff_inv = mat3(0.2569,-0.1483,.4394,.5044,-.2911,-.3679,.0979,.4394,-.0715);\n\
const vec3 yuv_offsets_inv = vec3(0.0625, 0.5, 0.5);\n";

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
static const char unif_srctexrect[] = "uniform sampler2DRect srctex;\n";
static const char unif_desttex[] = "uniform sampler2D desttex;\n";
static const char unif_desttexrect[] = "uniform sampler2DRect desttex;\n";
static const char unif_patterntex[] = "uniform sampler2D patterntex;\n";
static const char unif_stenciltex[] = "uniform sampler2D stenciltex;\n";
static const char unif_srcpal[] = "uniform sampler2D srcpal;\n";
static const char unif_destpal[] = "uniform sampler2D destpal;\n";
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
static const char op_srcrect[] = "src = ivec4(texture2DRect(srctex,gl_TexCoord[0].st)*vec4(colorsizesrc)+.5);\n";
static const char op_pixel[] = "pixel = ivec4(texture2D(srctex,gl_TexCoord[0].st)*vec4(colorsizedest)+.5);\n";
static const char op_pixelrect[] = "pixel = ivec4(texture2DRect(srctex,gl_TexCoord[0].st)*vec4(colorsizedest)+.5);\n";
static const char op_palpixel[] = "vec4 myindex = texture2D(srctex, gl_TexCoord[0].xy);\n\
vec2 index = vec2(((myindex.x*(255.0/256.0))+(0.5/256.0)),0.5);\n\
pixel = ivec4(texture2D(srcpal, index)*vec4(colorsizedest)+.5);\n";
static const char op_palpixelrect[] = "vec4 myindex = texture2DRect(srctex, gl_TexCoord[0].xy);\n\
vec2 index = vec2(((myindex.x*(255.0/256.0))+(0.5/256.0)),0.5);\n\
pixel = ivec4(texture2D(srcpal, index)*vec4(colorsizedest)+.5);\n";
static const char op_pixelmul256[] = "pixel = ivec4(vec4(256.0)*texture2D(srctex,gl_TexCoord[0].st)*vec4(colorsizedest)+.5);\n";
static const char op_pixeluyvy[] = "pixel = ivec4(readuyvy(srctex)*vec4(colorsizedest)+.5);\n";
static const char op_pixelyuyv[] = "pixel = ivec4(readyuyv(srctex)*vec4(colorsizedest)+.5);\n";
static const char op_pixelyvyu[] = "pixel = ivec4(readyvyu(srctex)*vec4(colorsizedest)+.5);\n";
static const char op_lumpixel[] = "pixel = ivec4(vec4(texture2D(srctex,gl_TexCoord[0].st).rrr,1.0)*vec4(colorsizedest)+.5);\n";
static const char op_color[] = "pixel = fillcolor;\n";
static const char op_dest[] = "dest = ivec4(texture2D(desttex,gl_TexCoord[1].st)*vec4(colorsizedest)+.5);\n";
static const char op_pattern[] = "patternst = vec2(mod(gl_FragCoord.x,float(patternsize.x))/float(patternsize.x),\n\
mod(gl_FragCoord.y, float(patternsize.y)) / float(patternsize.y));\n\
pattern = ivec4(texture2D(patterntex,patternst)*vec4(colorsizedest)+.5);\n";
static const char op_destoutdestblend[] = "gl_FragColor = (vec4(pixel)/vec4(colorsizedest)) * texture2D(desttex,gl_TexCoord[1].st);\n";
static const char op_destout[] = "gl_FragColor = vec4(pixel)/vec4(colorsizedest);\n";
static const char op_destoutyuvrgb[] = "gl_FragColor = yuvatorgba(vec4(pixel)/vec4(colorsizedest));\n";
static const char op_destoutrgbyuv[] = "gl_FragColor = rgbatoyuva(vec4(pixel)/vec4(colorsizedest));\n";
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
static const char func_yuvatorgba[] =
"vec4 yuvatorgba(vec4 yuva)\n\
{\n\
	return vec4(vec3(bt601_coeff * (yuva.rgb + yuv_offsets)),yuva.a);\n\
}\n\n";
static const char func_rgbatoyuva[] =
"vec4 rgbatoyuva(vec4 rgba)\n\
{\n\
	return vec4(vec3((bt601_coeff_inv * rgba.rgb) + yuv_offsets_inv),rgba.a);\n\
}\n\n";
static const char func_readuyvy_nearest[] = 
"vec4 readuyvy(sampler2DRect texture)\n\
{\n\
	float x = floor(gl_TexCoord[0].s);\n\
	float y = floor(gl_TexCoord[0].t);\n\
	float x2 = floor(gl_TexCoord[0].s/2.0)*2.0;\n\
	return vec4(texture2DRect(texture,vec2(x,y)).g,texture2DRect(texture,vec2(x2,y)).r,texture2DRect(texture,vec2(x2+1.0,y)).r,1.0);\n\
}\n\n";
static const char func_readyuyv_nearest[] =
"vec4 readyuyv(sampler2DRect texture)\n\
{\n\
	float x = floor(gl_TexCoord[0].s);\n\
	float y = floor(gl_TexCoord[0].t);\n\
	float x2 = floor(gl_TexCoord[0].s/2.0)*2.0;\n\
	return vec4(texture2DRect(texture,vec2(x,y)).r,texture2DRect(texture,vec2(x2,y)).g,texture2DRect(texture,vec2(x2+1.0,y)).g,1.0);\n\
}\n\n";
static const char func_readyvyu_nearest[] =
"vec4 readyvyu(sampler2DRect texture)\n\
{\n\
	float x = floor(gl_TexCoord[0].s);\n\
	float y = floor(gl_TexCoord[0].t);\n\
	float x2 = floor(gl_TexCoord[0].s/2.0)*2.0;\n\
	return vec4(texture2DRect(texture,vec2(x,y)).r,texture2DRect(texture,vec2(x2+1.0,y)).g,texture2DRect(texture,vec2(x2,y)).g,1.0);\n\
}\n\n";


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
"pixel = dest & pattern;\n",//A0
"pixel = (pattern ^ colorsizedest) ^ (dest | (pixel & (pattern ^ colorsizedest)));\n",
"pixel = dest & (pattern | (pixel ^ colorsizedest));\n",
"pixel = (dest ^ colorsizedest) ^ (pattern | (pixel ^ dest));\n",
"pixel = (pattern ^ colorsizedest) ^ (dest | ((pixel | pattern) ^ colorsizedest));\n",
"pixel = (pattern ^ colorsizedest) ^ dest;\n",
"pixel = dest ^ (pixel & (pattern ^ colorsizedest));\n",
"pixel = (pattern ^ colorsizedest) ^ (dest & (pixel | pattern));\n",
"pixel = dest & (pattern | pixel);\n",
"pixel = (dest ^ colorsizedest) ^ (pattern | pixel);\n",
"pixel = dest;\n",
"pixel = dest | ((pattern | pixel) ^ colorsizedest);\n",
"pixel = pixel ^ (pattern & (dest ^ pixel));\n",
"pixel = (dest ^ colorsizedest) ^ (pattern | (pixel & dest));\n",
"pixel = dest | (pixel & (pattern ^ colorsizedest));\n",
"pixel = dest | (pattern ^ colorsizedest);\n",//AF
"pixel = pattern & (dest | (pixel ^ colorsizedest));\n",//B0
"pixel = (pattern ^ colorsizedest) ^ (dest | (pixel ^ pattern));\n",
"pixel = pixel ^ ((pixel ^ pattern) | (pixel ^ dest));\n",
"pixel = (pixel & ((dest & pattern) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = pattern ^ (pixel & (dest ^ colorsizedest));\n",
"pixel = (dest ^ colorsizedest) ^ (pattern & (pixel | dest));\n",
"pixel = dest ^ pattern ^ (pixel | (dest & pattern));\n",
"pixel = (pixel & (dest ^ pattern)) ^ colorsizedest;\n",
"pixel = pattern ^ (pixel & (dest ^ pattern));\n",
"pixel = (dest ^ colorsizedest) ^ (pixel | (pattern & dest));\n",
"pixel = dest | (pattern & (pixel ^ colorsizedest));\n",
"pixel = dest | (pixel ^ colorsizedest);\n",//BB MERGEPAINT
"pixel = pixel ^ (pattern &	((dest & pixel) ^ colorsizedest));\n",
"pixel = ((pixel ^ dest) & (pattern ^ dest)) ^ colorsizedest;\n",
"pixel = dest | (pattern ^ pixel);\n",
"pixel = dest | ((pattern & pixel) ^ colorsizedest);\n",//BF
"pixel = pixel & pattern;\n",//C0 MERGECOPY
"pixel = (pixel ^ colorsizedest) ^ (pattern | (dest & (pixel ^ colorsizedest)));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern | ((dest | pixel) ^ colorsizedest));\n",
"pixel = (pattern ^ colorsizedest) ^ pixel;\n",
"pixel = pixel & (pattern | (dest ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern | (dest ^ pixel));\n",
"pixel = pixel ^ (dest & (pattern ^ colorsizedest));\n",
"pixel = (pattern ^ colorsizedest) ^ (pixel & (dest | pattern));\n",
"pixel = pixel & (dest | pattern);\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern | dest);\n",
"pixel = dest ^ (pattern & (pixel ^ dest));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern | (dest & pixel));\n",
"//ROP is a no-op for srccopy\n",//CC SRCCOPY  pixel=pixel
"pixel = pixel | ((dest | pattern) ^ colorsizedest);\n",
"pixel = pixel | (dest & (pattern ^ colorsizedest));\n",
"pixel = pixel | (pattern ^ colorsizedest);\n",//CF
"pixel = pattern & (pixel | (dest ^ colorsizedest));\n",//D0
"pixel = (pattern ^ colorsizedest) ^ (pixel | (dest ^ pattern));\n",
"pixel = pattern ^ (dest & (pixel ^ colorsizedest));\n",
"pixel = (pixel ^ colorsizedest) ^ (pattern & (dest | pixel));\n",
"pixel = pixel ^ ((pixel ^ pattern) & (dest ^ pattern));\n",
"pixel = (dest & ((pattern & pixel) ^ colorsizedest)) ^ colorsizedest;\n",
"pixel = pixel ^ pattern ^ (dest | (pattern & pixel));\n",
"pixel = (dest & (pattern ^ pixel)) ^ colorsizedest;\n",
"pixel = pattern ^ (dest & (pixel ^ pattern));\n",
"pixel = (pixel ^ colorsizedest) ^ (dest | (pattern & pixel));\n",
"pixel = dest ^ (pattern & ((pixel & dest) ^ colorsizedest));\n",
"pixel = ((pixel ^ pattern) & (pixel ^ dest)) ^ colorsizedest;\n",
"pixel = pixel | (pattern & (dest ^ colorsizedest));\n",
"pixel = pixel | (dest ^ colorsizedest);\n",
"pixel = pixel | (dest ^ pattern);\n",
"pixel = pixel | ((dest & pattern) ^ colorsizedest);\n",//DF
"pixel = pattern & (dest | pixel);\n",//E0
"pixel = (pattern ^ colorsizedest) ^ (dest | pixel);\n",
"pixel = dest ^ (pixel & (pattern ^ dest));\n",
"pixel = (pattern ^ colorsizedest) ^ (pixel | (dest & pattern));\n",
"pixel = pixel ^ (dest & (pattern ^ pixel));\n",
"pixel = (pattern ^ colorsizedest) ^ (dest | (pixel & pattern));\n",
"pixel = pixel ^ (dest & ((pattern & pixel) ^ colorsizedest));\n",
"pixel = ((pixel ^ pattern) & (dest ^ pattern)) ^ colorsizedest;\n",
"pixel = pixel ^ ((pixel ^ pattern) & (pixel ^ dest));\n",
"pixel = (dest ^ colorsizedest) ^ pixel ^ (pattern & ((pixel & dest) ^ colorsizedest));\n",
"pixel = dest | (pattern & pixel);\n",
"pixel = dest | ((pattern ^ pixel) ^ colorsizedest);\n",
"pixel = pixel | (dest & pattern);\n",
"pixel = pixel | ((dest ^ pattern) ^ colorsizedest);\n",
"pixel = pixel | dest;\n",//EE SRCPAINT
"pixel = pixel | dest | (pattern ^ colorsizedest);\n",//EF
"pixel = pattern;\n",//F0 PATCOPY
"pixel = pattern | ((dest | pixel) ^ colorsizedest);\n",
"pixel = pattern | (dest & (pixel ^ colorsizedest));\n",
"pixel = pattern | (pixel ^ colorsizedest);\n",
"pixel = pattern | (pixel & (dest ^ colorsizedest));\n",
"pixel = pattern | (dest ^ colorsizedest);\n",
"pixel = pattern | (dest ^ pixel);\n",
"pixel = pattern | ((pixel & dest) ^ colorsizedest);\n",
"pixel = pattern | (dest & pixel);\n",
"pixel = pattern | ((dest ^ pixel) ^ colorsizedest);\n",
"pixel = dest | pattern;\n",
"pixel = dest | pattern | (pixel ^ colorsizedest);\n",//FB PATPAINT
"pixel = pattern | pixel;\n",
"pixel = pattern | pixel | (dest ^ colorsizedest);\n",
"pixel = pattern | dest | pixel;\n",
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

__int64 PackROPBits(DWORD rop, __int64 flags)
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

DWORD UnpackROPBits(__int64 flags)
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

void ShaderGen2D_CreateShader2D(ShaderGen2D *gen, int index, __int64 id)
{
	STRING tmp;
	DWORD rop;
	tmp.ptr = NULL;
	BOOL intproc = FALSE;
	BOOL usedest = FALSE;
	unsigned char srctype = (id >> 32) & 0xFF;
	unsigned char srctype2;
	unsigned char desttype = (id >> 40) & 0xFF;
	if (srctype == desttype) srctype2 = 0;
	else srctype2 = srctype;
	gen->genshaders2D[index].shader.vsrc.ptr = NULL;
	gen->genshaders2D[index].shader.fsrc.ptr = NULL;
	char idstring[30];
	_snprintf(idstring, 29, "%0.16I64X\n", id);
	idstring[29] = 0;
	// Create vertex shader
	// Header
	STRING *vsrc = &gen->genshaders2D[index].shader.vsrc;
	String_Append(vsrc, revheader);
	if (id & DDBLT_ROP)
	{
		rop = UnpackROPBits(id);
		if ((gen->ext->glver_major >= 3) && !dxglcfg.DebugNoGLSL130)
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
		if ((gen->ext->glver_major >= 3) && !dxglcfg.DebugNoGLSL130)
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
	switch (srctype)
	{
	default:
		break;
	case 0x20:
	case 0x21:
	case 0x80:
	case 0x81:
	case 0x82:
		String_Append(fsrc, ext_texrect);
		break;
	}
	String_Append(fsrc, idheader);
	String_Append(fsrc, idstring);

	// Constants
	switch (srctype)
	{
	case 0:
	default:
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
		if ((desttype >= 0x80) && (desttype <= 0x83)) break;
		String_Append(fsrc, const_bt601_coeff);
		break;			
	}

	switch (desttype)
	{
	case 0:
	default:
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
		if ((srctype >= 0x80) && (srctype <= 0x83)) break;
		String_Append(fsrc, const_bt601_coeff_inv);
		break;
	}

	// Uniforms
	if (id & DDBLT_COLORFILL) String_Append(fsrc, unif_fillcolor);
	else
	{
		switch (srctype)
		{
		case 0x20:
		case 0x21:
		case 0x80:
		case 0x81:
		case 0x82:
			String_Append(fsrc, unif_srctexrect);
			break;
		case 0x83:
		default:
			String_Append(fsrc, unif_srctex);
			break;
		}
	}
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
	switch (srctype2)
	{
	default:
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		String_Append(fsrc, unif_srcpal);
		break;
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

	// Functions
	switch (srctype)
	{
	case 0x20:
		break;
	case 0x21:
		break;
	case 0x80:
		String_Append(fsrc, func_readuyvy_nearest);
		if ((desttype >= 0x80) && (desttype <= 0x83)) break;
		String_Append(fsrc, func_yuvatorgba);
		break;
	case 0x81:
		String_Append(fsrc, func_readyuyv_nearest);
		if ((desttype >= 0x80) && (desttype <= 0x83)) break;
		String_Append(fsrc, func_yuvatorgba);
		break;
	case 0x82:
		String_Append(fsrc, func_readyvyu_nearest);
		if ((desttype >= 0x80) && (desttype <= 0x83)) break;
		String_Append(fsrc, func_yuvatorgba);
		break;
	case 0x83:
		if ((desttype >= 0x80) && (desttype <= 0x83)) break;
		String_Append(fsrc, func_yuvatorgba);
		break;
	default:
		break;
	}

	switch (desttype)
	{
	case 0x20:
		break;
	case 0x21:
		break;
	case 0x80:
		break;
	case 0x81:
		break;
	case 0x82:
		break;
	case 0x83:
		if ((srctype >= 0x80) && (srctype <= 0x83)) break;
		String_Append(fsrc, func_rgbatoyuva);
		break;
	default:
		break;
	}

	// Main
	String_Append(fsrc, mainstart);
	if (id & 0x10000000) String_Append(fsrc, op_clip);
	if (id & DDBLT_COLORFILL) String_Append(fsrc, op_color);
	else
	{
		switch (srctype)
		{
		case 0x00:  // Classic RGB
		case 0x83:  // AYUV
		default:
			String_Append(fsrc, op_pixel);
			break;
		case 0x01:
		case 0x02:
			String_Append(fsrc, op_lumpixel);
			break;
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			if((desttype >= 0x10) && (desttype <= 0x13)) String_Append(fsrc, op_pixel);
			else String_Append(fsrc, op_palpixel);
			break;
		case 0x18:
		case 0x19:
			String_Append(fsrc, op_pixelmul256);
			break;
		case 0x20:
			break;
		case 0x21:
			break;
		case 0x80:
			String_Append(fsrc, op_pixeluyvy);
			break;
		case 0x81:
			break;
		case 0x82:
			break;
		}
	}
	if (id & DDBLT_KEYSRC) String_Append(fsrc, op_src);
	if (usedest) String_Append(fsrc, op_dest);
	if (id & DDBLT_KEYSRC)
	{
		if (id & 0x20000000) String_Append(fsrc, op_ckeysrcrange);
		else String_Append(fsrc, op_ckeysrc);
	}
	if (id & DDBLT_KEYDEST)
	{
		if (!dxglcfg.DebugBlendDestColorKey)
		{
			if (id & 0x40000000) String_Append(fsrc, op_ckeydestrange);
			else String_Append(fsrc, op_ckeydest);
		}
	}
	if (id & DDBLT_ROP)
	{
		if (rop_texture_usage[rop] & 4) String_Append(fsrc, op_pattern);
		if (intproc) String_Append(fsrc, op_ROP[rop]);
		else String_Append(fsrc, op_ROP_float[rop]);
	}
	if (dxglcfg.DebugBlendDestColorKey && (id & DDBLT_KEYDEST))
		String_Append(fsrc, op_destoutdestblend);
	else
	{
		switch (srctype)
		{
		case 0x00:
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
			switch (desttype)
			{
			case 0:
			default:
				String_Append(fsrc, op_destout);
				break;
			case 0x83:
				String_Append(fsrc, op_destoutrgbyuv);
				break;
			}
			break;
		case 0x80:
			if ((desttype >= 0x80) && (desttype <= 0x83))
				String_Append(fsrc, op_destout);
			else String_Append(fsrc, op_destoutyuvrgb);
			break;
		case 0x83:
			if ((desttype >= 0x80) && (desttype <= 0x83))
				String_Append(fsrc, op_destout);
			else String_Append(fsrc, op_destoutyuvrgb);
			break;
		default:
			String_Append(fsrc, op_destout);
			break;
		}
	}
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
	gen->genshaders2D[index].shader.uniforms[13] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "srcpal");
	gen->genshaders2D[index].shader.uniforms[14] = gen->ext->glGetUniformLocation(gen->genshaders2D[index].shader.prog, "destpal");

	gen->genshaders2D[index].id = id;
}