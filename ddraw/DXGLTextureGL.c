// DXGL
// Copyright (C) 2024-2025 William Feely

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
#include "DXGLQueue.h"
#include "DXGLRendererGL.h"
#include "DXGLTexture.h"
#include "DXGLTextureGL.h"
#include "util.h"
#include "glUtil.h"

// Pixel format constants
#define DXGLPIXELFORMAT_INVALID			-1
#define DXGLPIXELFORMAT_PAL1			0
#define DXGLPIXELFORMAT_PAL2			1
#define DXGLPIXELFORMAT_PAL4			2
#define DXGLPIXELFORMAT_PAL8			3
#define DXGLPIXELFORMAT_RGB332			4
#define DXGLPIXELFORMAT_RGB555			5
#define DXGLPIXELFORMAT_RGB565			6
#define DXGLPIXELFORMAT_RGB888			7
#define DXGLPIXELFORMAT_RGB888_REV		8
#define DXGLPIXELFORMAT_RGBX8888		9
#define DXGLPIXELFORMAT_RGBX8888_REV	10
#define DXGLPIXELFORMAT_RGBA8332		11
#define DXGLPIXELFORMAT_RGBA4444		12
#define DXGLPIXELFORMAT_RGBA1555		13
#define DXGLPIXELFORMAT_RGBA8888		14
#define DXGLPIXELFORMAT_LUM8			15
#define DXGLPIXELFORMAT_ALPHA8			16
#define DXGLPIXELFORMAT_LUM_ALPHA88		17
#define DXGLPIXELFORMAT_Z16				18
#define DXGLPIXELFORMAT_Z24				19
#define DXGLPIXELFORMAT_X8_Z24			20
#define DXGLPIXELFORMAT_X8_Z24_REV		21
#define DXGLPIXELFORMAT_Z32				22
#define DXGLPIXELFORMAT_S8_Z32			23
#define DXGLPIXELFORMAT_S8_Z32_REV		24
#define DXGLPIXELFORMAT_FOURCC_Y8		25
#define DXGLPIXELFORMAT_FOURCC_Y800		26
#define DXGLPIXELFORMAT_FOURCC_GREY		27
#define DXGLPIXELFORMAT_FOURCC_Y16		28
#define DXGLPIXELFORMAT_FOURCC_UYVY		29
#define DXGLPIXELFORMAT_FOURCC_UYNV		30
#define DXGLPIXELFORMAT_FOURCC_Y422		31
#define DXGLPIXELFORMAT_FOURCC_YUY2		32
#define DXGLPIXELFORMAT_FOURCC_YUYV		33
#define DXGLPIXELFORMAT_FOURCC_YUNV		34
#define DXGLPIXELFORMAT_FOURCC_YVYU		35
#define DXGLPIXELFORMAT_FOURCC_RGBG		36
#define DXGLPIXELFORMAT_FOURCC_GRGB		37
#define DXGLPIXELFORMAT_FOURCC_AYUV		38


static const DDSURFACEDESC2 ddsddummycolor =
{
	sizeof(DDSURFACEDESC2),
	DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL,
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{
		sizeof(DDPIXELFORMAT),
		DDPF_RGB | DDPF_ALPHAPIXELS,
		0,
		16,
		0xF00,
		0xF0,
		0xF,
		0xF000,
	},
	{
		DDSCAPS_OFFSCREENPLAIN,
		0,
		0,
		0
	},
	0,
};


// Use EXACTLY one line per entry.  Don't change layout of the list.
static const int START_GLTEXFORMATS = __LINE__;
static const DDPIXELFORMAT GLtexformats[] =
{ // Size					Flags							FOURCC	bits	R/Ymask		G/U/Zmask	B/V/STmask	A/Zmask
	{sizeof(DDPIXELFORMAT),	DDPF_RGB | DDPF_PALETTEINDEXED1,	0,		1,		0,			0,			0,			0},  // 1-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB | DDPF_PALETTEINDEXED2,	0,		2,		0,			0,			0,			0},  // 2-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB | DDPF_PALETTEINDEXED4,	0,		4,		0,			0,			0,			0},  // 4-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB | DDPF_PALETTEINDEXED8,	0,		8,		0,			0,			0,			0},  // 8-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},  // 8 bit 332
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},  // 15 bit 555
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},  // 16 bit 565
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF0000,	0xFF00,		0xFF,		0},  // 24 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF,		0xFF00,		0xFF0000,	0},  // 24 bit 888 RGB
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},  // 32 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF,		0xFF00,		0xFF0000,	0},  // 32 bit 888 RGB
	{sizeof(DDPIXELFORMAT),	DDPF_RGB | DDPF_ALPHAPIXELS,		0,		16,		0xE0,		0x1C,		0x3,		0xFF00},  // 16-bit 8332
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS,		0,		16,		0xF00,		0xF0,		0xF,		0xF000},  // 16-bit 4444
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS,		0,		16,		0x7c00,		0x3E0,		0x1F,		0x8000},  // 16-bit 1555
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS,		0,		32,		0xFF0000,	0xFF00,		0xFF,		0xFF000000},  // 32-bit 8888
	{sizeof(DDPIXELFORMAT), DDPF_LUMINANCE,					0,		8,		0xFF,		0,			0,			0},  // 8-bit luminance
	{sizeof(DDPIXELFORMAT),	DDPF_ALPHA,						0,		8,		0,			0,			0,			0},  // 8-bit alpha
	{sizeof(DDPIXELFORMAT),	DDPF_LUMINANCE | DDPF_ALPHAPIXELS,0,		16,		0xFF,		0,			0,			0xFF00},  // 8-bit luminance alpha
	{sizeof(DDPIXELFORMAT), DDPF_ZBUFFER,					0,		16,		0,			0xFFFF,		0,			0},  // 16 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		24,		0,			0xFFFFFF,	0,			0},  // 24 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFF,	0,			0},  // 24 bit Z buffer, 32-bit space
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFF00,	0,			0},  // 24 bit Z buffer, 32-bit space, reversed
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFFFF,	0,			0},  // 32 bit Z buffer
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFFFFFF00,	0xFF,		0},  // 32 bit Z buffer with stencil
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFF,		0xFFFFFF00,	0},  // 32 bit Z buffer with stencil, reversed
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','8',' ',' '), 0,	0,			0,			0,			0},  // 8-bit grayscale
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','8','0','0'), 0,	0,			0,			0,			0},  // 8-bit grayscale
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('G','R','E','Y'), 0,	0,			0,			0,			0},  // 8-bit grayscale
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','1','6',' '), 0,	0,			0,			0,			0},  // 16-bit grayscale
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U','Y','V','Y'), 0,	0,			0,			0,			0},  // UYVY packed YUV surface
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U','Y','N','V'), 0,	0,			0,			0,			0},  // UYVY packed YUV surface (NVIDIA alias)
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','4','2','2'), 0,	0,			0,			0,			0},  // UYVY packed YUV surface (ADS Tech. alias)
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0,	0,			0,			0,			0},  // YUY2 packed YUV surface
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','Y','V'), 0,	0,			0,			0,			0},  // YUY2 packed YUV surface (dup. of YUY2)
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','N','V'), 0,	0,			0,			0,			0},  // YUY2 packed YUV surface (NVIDIA alias)
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','V','Y','U'), 0,	0,			0,			0,			0},  // YUY2 packed YUV surface
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('R','G','B','G'), 0,	0,			0,			0,			0},  // RGBG packed 16-bit pixelformat
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('G','R','G','B'), 0,	0,			0,			0,			0},  // GRGB packed 16-bit pixelformat
	{sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('A','Y','U','V'), 0,	0,			0,			0,			0},  // AYUV packed YUV surface
};
static const int END_GLTEXFORMATS = __LINE__ - 4;
static int numGLtexformats;


HRESULT DXGLTextureGL_ValidatePixelFormat(LPDDPIXELFORMAT pixelformat, DWORD *texformat)
{
	int i;
	DDPIXELFORMAT compformat;
	*texformat = -1;
	numGLtexformats = END_GLTEXFORMATS - START_GLTEXFORMATS;
	if (pixelformat->dwFlags & DDPF_FOURCC)
	{
		ZeroMemory(&compformat, sizeof(DDPIXELFORMAT));
		compformat.dwSize = sizeof(DDPIXELFORMAT);
		compformat.dwFlags = pixelformat->dwFlags;
		compformat.dwFourCC = pixelformat->dwFourCC;
	}
	else compformat = *pixelformat;
	for (i = 0; i < numGLtexformats; i++)
	{
		if (!memcmp(&GLtexformats[i], pixelformat, sizeof(DDPIXELFORMAT)))
		{
			*texformat = i;
			break;
		}
	}
	if (*texformat == -1) return DDERR_INVALIDPIXELFORMAT;
	else return DD_OK;
}

static LONG SetPitch(LPDDSURFACEDESC ddsd, DWORD width, DWORD bpp)
{
	if (ddsd->dwFlags & DDSD_PITCH) return ddsd->lPitch;  // Hope the app knows what it's doing.
	else
	{
		if (ddsd->dwFlags & DDSD_PIXELFORMAT)
		{
			if (ddsd->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
			{
				switch (ddsd->ddpfPixelFormat.dwFourCC)
				{
				case MAKEFOURCC('Y', '8', ' ', ' '):
				case MAKEFOURCC('Y', '8', '0', '0'):
				case MAKEFOURCC('G', 'R', 'E', 'Y'):
					return NextMultipleOf4(width);
				case MAKEFOURCC('Y', '1', '6', ' '):
				case MAKEFOURCC('U', 'Y', 'V', 'Y'):
				case MAKEFOURCC('U', 'Y', 'N', 'V'):
				case MAKEFOURCC('Y', '4', '2', '2'):
				case MAKEFOURCC('Y', 'U', 'Y', '2'):
				case MAKEFOURCC('Y', 'U', 'Y', 'V'):
				case MAKEFOURCC('Y', 'U', 'N', 'V'):
				case MAKEFOURCC('Y', 'V', 'Y', 'U'):
				case MAKEFOURCC('R', 'G', 'B', 'G'):
				case MAKEFOURCC('G', 'R', 'G', 'B'):
					return NextMultipleOf4(width * 2);
				case MAKEFOURCC('A', 'Y', 'U', 'V'):
				default:
					return NextMultipleOf4(width * 4);
				}

			}
			else
			{
				switch (ddsd->ddpfPixelFormat.dwRGBBitCount)
				{
				case 8:
					return NextMultipleOf4(width);
				case 15:
				case 16:
					return NextMultipleOf4(width * 2);
				case 24:
					return NextMultipleOf4(width * 3);
				case 32:
				default:
					return NextMultipleOf4(width * 4);
				}
			}
		}
		else
		{
			switch(bpp)
			{
			case 8:
				return NextMultipleOf4(width);
			case 15:
			case 16:
				return NextMultipleOf4(width * 2);
			case 24:
				return NextMultipleOf4(width * 3);
			case 32:
			default:
				return NextMultipleOf4(width * 4);
			}
		}
	}
}

HRESULT DXGLTextureGL_Create(LPDDSURFACEDESC2 ddsd, DWORD bpp, glExtensions *ext, DXGLTexture *out)
{
	DDPIXELFORMAT compformat;
	HRESULT error;
	int i;
	int texformat = -1;
	if (!out) return DDERR_INVALIDPARAMS;
	if (!IsWritablePointer(out, sizeof(DXGLTexture), TRUE))
		return DDERR_EXCEPTION;
	if ((ddsd->dwWidth < 1) || (ddsd->dwWidth > 32768) || (ddsd->dwHeight < 1) || (ddsd->dwHeight > 32768))
		return DDERR_INVALIDPARAMS;
	ZeroMemory(out, sizeof(DXGLTexture));
	memcpy(&out->ddformat, &ddsd->ddpfPixelFormat, sizeof(DDPIXELFORMAT));
	if (!ddsd->dwFlags & DDSD_PIXELFORMAT);
	{
		out->ddformat.dwSize = sizeof(DDPIXELFORMAT);
		out->ddformat.dwRGBBitCount = bpp;
		switch (bpp)
		{
		case 8:
			out->ddformat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			out->ddformat.dwRBitMask = 0;
			out->ddformat.dwGBitMask = 0;
			out->ddformat.dwBBitMask = 0;
			break;
		case 15:
			out->ddformat.dwFlags = DDPF_RGB;
			out->ddformat.dwRBitMask = 0x7C00;
			out->ddformat.dwGBitMask = 0x3E0;
			out->ddformat.dwBBitMask = 0x1F;
			out->ddformat.dwRGBBitCount = 16;
			break;
		case 16:
			out->ddformat.dwFlags = DDPF_RGB;
			out->ddformat.dwRBitMask = 0xF800;
			out->ddformat.dwGBitMask = 0x7E0;
			out->ddformat.dwBBitMask = 0x1F;
			break;
		case 24:
			out->ddformat.dwFlags = DDPF_RGB;
			out->ddformat.dwRBitMask = 0xFF0000;
			out->ddformat.dwGBitMask = 0xFF00;
			out->ddformat.dwBBitMask = 0xFF;
			break;
		case 32:
		default:
			out->ddformat.dwFlags = DDPF_RGB;
			out->ddformat.dwRBitMask = 0xFF0000;
			out->ddformat.dwGBitMask = 0xFF00;
			out->ddformat.dwBBitMask = 0xFF;
			break;
		}
	}
	error = DXGLTextureGL_ValidatePixelFormat(&out->ddformat, &texformat);
	if (FAILED(error)) return error;
	out->levels[0].width = ddsd->dwWidth;
	out->levels[0].height = ddsd->dwHeight;
	out->levels[0].pitch = SetPitch(ddsd, ddsd->dwWidth, bpp);
	if (ddsd->dwMipMapCount)
	{
		for (i = 1; i < ddsd->dwMipMapCount; i++)
		{
			out->levels[i].width = out->levels[0].width;
			out->levels[i].height = out->levels[0].height;
			ShrinkMip(&out->levels[i].width, &out->levels[i].height, i);
			out->levels[i].pitch = SetPitch(ddsd, out->levels[i].width, bpp);
		}
	}
	else out->mipcount = 1;
	return DD_OK;
}

void DXGLTextureGL__FinishCreate(DXGLTexture *This, LPDXGLRENDERERGL renderer, glExtensions *ext, glUtil *util)
{
	int i;
	int texformat = -1;
	HRESULT error;
	GLenum glerror;
	ZeroMemory(&This->format.glformat.internalformats, 4 * sizeof(GLint));
	switch (texformat)
	{
	case DXGLPIXELFORMAT_PAL1: // 1-bit palette
		This->useconv = TRUE;
		This->convfunctionupload = 11;
		This->convfunctiondownload = 14;
		This->internalsize = 1;  // Store in R8/LUMINANCE8 texture
		This->blttype = 0x13;
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 1;
		This->colorsizes[1] = 1;
		This->colorsizes[2] = 1;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 1;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_PAL2: // 2-bit palette
		This->useconv = TRUE;
		This->convfunctionupload = 12;
		This->convfunctiondownload = 15;
		This->internalsize = 1;  // Store in R8/LUMINANCE8 texture
		This->blttype = 0x12;
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 3;
		This->colorsizes[1] = 3;
		This->colorsizes[2] = 3;
		This->colorsizes[3] = 3;
		This->colorbits[0] = 2;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_PAL4: // 4-bit palette
		This->useconv = TRUE;
		This->convfunctionupload = 13;
		This->convfunctiondownload = 16;
		This->internalsize = 1;  // Store in R8/LUMINANCE8 texture
		This->blttype = 0x11;
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 15;
		This->colorsizes[1] = 15;
		This->colorsizes[2] = 15;
		This->colorsizes[3] = 15;
		This->colorbits[0] = 4;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_PAL8: // 8-bit palette
		This->blttype = 0x10;
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGB332: // 8-bit RGB332
		This->format.glformat.internalformats[0] = GL_R3_G3_B2;
		This->format.glformat.internalformats[1] = GL_RGB8;
		This->format.glformat.internalformats[2] = GL_RGBA8;
		This->format.glformat.format = GL_RGB;
		This->format.glformat.type = GL_UNSIGNED_BYTE_3_3_2;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 7;
		This->colorsizes[1] = 7;
		This->colorsizes[2] = 3;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 3;
		This->colorbits[1] = 3;
		This->colorbits[2] = 2;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGB555: // 16-bit RGB555
		This->format.glformat.internalformats[0] = GL_RGB5_A1;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 31;
		This->colorsizes[1] = 31;
		This->colorsizes[2] = 31;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 5;
		This->colorbits[1] = 5;
		This->colorbits[2] = 5;
		This->colorbits[3] = 1;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGB565: // 16-bit RGB565
		if (ext->GLEXT_ARB_ES2_compatibility)
		{
			This->format.glformat.internalformats[0] = GL_RGB565;
			This->format.glformat.internalformats[1] = GL_RGB8;
			This->format.glformat.internalformats[2] = GL_RGBA8;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_RGB8;
			This->format.glformat.internalformats[1] = GL_RGBA8;
		}
		This->format.glformat.format = GL_RGB;
		This->format.glformat.type = GL_UNSIGNED_SHORT_5_6_5;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 31;
		This->colorsizes[1] = 63;
		This->colorsizes[2] = 31;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 5;
		This->colorbits[1] = 6;
		This->colorbits[2] = 5;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGB888: // 24-bit RGB888
		This->format.glformat.internalformats[0] = GL_RGB8;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_BGR;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGB888_REV: // 24-bit BGR888
		This->format.glformat.internalformats[0] = GL_RGB8;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_RGB;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 0;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGBX8888: // 32-bit RGB888
		This->format.glformat.internalformats[0] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_INT_8_8_8_8_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGBX8888_REV: // 32-bit BGR888
		This->format.glformat.internalformats[0] = GL_RGBA8;
		This->format.glformat.format = GL_RGBA;
		This->format.glformat.type = GL_UNSIGNED_INT_8_8_8_8_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 0;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGBA8332: // 16-bit RGBA8332
		This->useconv = TRUE;
		This->convfunctionupload = 0;
		This->convfunctiondownload = 1;
		This->internalsize = 4;  // Store in 8888 texture
		This->format.glformat.internalformats[0] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 7;
		This->colorsizes[1] = 7;
		This->colorsizes[2] = 3;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 3;
		This->colorbits[1] = 3;
		This->colorbits[2] = 2;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGBA4444: // 16-bit RGBA4444
		This->format.glformat.internalformats[0] = GL_RGBA4;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 15;
		This->colorsizes[1] = 15;
		This->colorsizes[2] = 15;
		This->colorsizes[3] = 15;
		This->colorbits[0] = 4;
		This->colorbits[1] = 4;
		This->colorbits[2] = 4;
		This->colorbits[3] = 4;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_RGBA1555: // 16-bit RGBA1555
		This->format.glformat.internalformats[0] = GL_RGB5_A1;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colorsizes[0] = 31;
		This->colorsizes[1] = 31;
		This->colorsizes[2] = 31;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 5;
		This->colorbits[1] = 5;
		This->colorbits[2] = 5;
		This->colorbits[3] = 1;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_INVALID:
	case DXGLPIXELFORMAT_RGBA8888: // 32-bit RGBA8888
		This->format.glformat.internalformats[0] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_INT_8_8_8_8_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_LUM8: // 8-bit Luminance
		This->blttype = 0x01;
		if (ext->glver_major >= 3) // && !(This->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_LUMINANCE8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.internalformats[1] = GL_RGB8;
		This->format.glformat.internalformats[2] = GL_RGBA8;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 5;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_Y8:  // 8-bit Y-only
	case DXGLPIXELFORMAT_FOURCC_Y800:
	case DXGLPIXELFORMAT_FOURCC_GREY:
		This->blttype = 0x02;
		if (ext->glver_major >= 3)// && !(This->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		{
			This->format.glformat.internalformats[0] = GL_R8;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_LUMINANCE8;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.internalformats[1] = GL_RGB8;
		This->format.glformat.internalformats[2] = GL_RGBA8;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 5;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_ALPHA8: // 8-bit Alpha
		This->format.glformat.internalformats[0] = GL_ALPHA8;
		This->format.glformat.format = GL_ALPHA;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 6;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 0;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_LUM_ALPHA88: // 16-bit Luminance Alpha
		This->format.glformat.internalformats[0] = GL_LUMINANCE8_ALPHA8;
		This->format.glformat.internalformats[1] = GL_RGBA8;
		This->format.glformat.format = GL_LUMINANCE_ALPHA;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 7;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_Z16: // 16-bit Z buffer
		This->format.glformat.internalformats[0] = GL_DEPTH_COMPONENT16;
		This->format.glformat.format = GL_DEPTH_COMPONENT;
		This->format.glformat.type = GL_UNSIGNED_SHORT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 65535;
		This->colorsizes[1] = 65535;
		This->colorsizes[2] = 65535;
		This->colorsizes[3] = 65535;
		This->colorbits[0] = 16;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_Z24: // 24-bit Z buffer
		This->useconv = TRUE;
		This->convfunctionupload = 17;
		This->convfunctiondownload = 18;
		This->internalsize = 4;
		This->blttype = 0x18;
		This->format.glformat.internalformats[0] = GL_DEPTH_COMPONENT24;
		This->format.glformat.format = GL_DEPTH_COMPONENT;
		This->format.glformat.type = GL_UNSIGNED_INT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 16777215;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_X8_Z24: // 32/24 bit Z buffer
		This->blttype = 0x18;
		This->format.glformat.internalformats[0] = GL_DEPTH_COMPONENT24;
		This->format.glformat.format = GL_DEPTH_COMPONENT;
		This->format.glformat.type = GL_UNSIGNED_INT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 16777215;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_X8_Z24_REV: // 32/24 bit Z buffer reversed
		This->format.glformat.internalformats[0] = GL_DEPTH_COMPONENT24;
		This->format.glformat.format = GL_DEPTH_COMPONENT;
		This->format.glformat.type = GL_UNSIGNED_INT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 16777215;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_Z32: // 32-bit Z buffer
		This->format.glformat.internalformats[0] = GL_DEPTH_COMPONENT32;
		This->format.glformat.format = GL_DEPTH_COMPONENT;
		This->format.glformat.type = GL_UNSIGNED_INT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 4294967295;
		This->colorsizes[1] = 4294967295;
		This->colorsizes[2] = 4294967295;
		This->colorsizes[3] = 4294967295;
		This->colorbits[0] = 32;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_S8_Z32: // 32-bit Z/Stencil buffer, depth LSB
		This->format.glformat.internalformats[0] = GL_DEPTH24_STENCIL8;
		This->format.glformat.format = GL_DEPTH_STENCIL;
		This->format.glformat.type = GL_UNSIGNED_INT_24_8;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 7;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_S8_Z32_REV: // 32-bit Z/Stencil buffer, depth MSB
		This->blttype = 0x18;
		This->format.glformat.internalformats[0] = GL_DEPTH24_STENCIL8;
		This->format.glformat.format = GL_DEPTH_STENCIL;
		This->format.glformat.type = GL_UNSIGNED_INT_24_8;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 7;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_Y16:
		This->blttype = 0x02;
		if (ext->glver_major >= 3) // && !(This->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		{
			This->format.glformat.internalformats[0] = GL_R16;
			This->format.glformat.format = GL_RED;
		}
		else
		{
			This->format.glformat.internalformats[0] = GL_LUMINANCE16;
			This->format.glformat.format = GL_LUMINANCE;
		}
		This->format.glformat.type = GL_UNSIGNED_SHORT;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 4;
		This->colorsizes[0] = 65535;
		This->colorsizes[1] = 65535;
		This->colorsizes[2] = 65535;
		This->colorsizes[3] = 65535;
		This->colorbits[0] = 16;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_UYVY:
	case DXGLPIXELFORMAT_FOURCC_UYNV:
	case DXGLPIXELFORMAT_FOURCC_Y422:
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_RG8;
			This->format.glformat.format = GL_RG;
		}
		else
		{
			This->useconv = TRUE;
			This->convfunctionupload = 9;
			This->convfunctiondownload = 10;
			This->internalsize = 4;
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_RGBA;
		}
		This->blttype = 0x80;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_RECTANGLE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_YUY2:
	case DXGLPIXELFORMAT_FOURCC_YUYV:
	case DXGLPIXELFORMAT_FOURCC_YUNV:
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_RG8;
			This->format.glformat.format = GL_RG;
		}
		else
		{
			This->useconv = TRUE;
			This->convfunctionupload = 9;
			This->convfunctiondownload = 10;
			This->internalsize = 4;
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_RGBA;
		}
		This->blttype = 0x81;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_RECTANGLE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_YVYU:
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_RG8;
			This->format.glformat.format = GL_RG;
		}
		else
		{
			This->useconv = TRUE;
			This->convfunctionupload = 9;
			This->convfunctiondownload = 10;
			This->internalsize = 4;
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_RGBA;
		}
		This->blttype = 0x82;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_RECTANGLE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_RGBG:
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_RG8;
			This->format.glformat.format = GL_RG;
		}
		else
		{
			This->useconv = TRUE;
			This->convfunctionupload = 9;
			This->convfunctiondownload = 10;
			This->internalsize = 4;
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_RGBA;
		}
		This->blttype = 0x20;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_RECTANGLE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_GRGB:
		if (ext->glver_major >= 3)
		{
			This->format.glformat.internalformats[0] = GL_RG8;
			This->format.glformat.format = GL_RG;
		}
		else
		{
			This->useconv = TRUE;
			This->convfunctionupload = 9;
			This->convfunctiondownload = 10;
			This->internalsize = 4;
			This->format.glformat.internalformats[0] = GL_RGBA8;
			This->format.glformat.format = GL_RGBA;
		}
		This->blttype = 0x21;
		This->format.glformat.type = GL_UNSIGNED_BYTE;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_RECTANGLE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	case DXGLPIXELFORMAT_FOURCC_AYUV: // 32-bit AYUV
		This->blttype = 0x83;
		This->format.glformat.internalformats[0] = GL_RGBA8;
		This->format.glformat.format = GL_BGRA;
		This->format.glformat.type = GL_UNSIGNED_INT_8_8_8_8_REV;
		if (!This->gltarget) This->gltarget = GL_TEXTURE_2D;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		This->packsize = 1;
		break;
	}
	glGenTextures(1, &This->glhandle);
	glUtil_SetTexture(util, 15, This);
	This->params.glparam.minfilter = This->params.glparam.magfilter = GL_NEAREST;
	This->params.glparam.wraps = This->params.glparam.wrapt = This->params.glparam.wrapr = GL_CLAMP_TO_EDGE;
	glTexParameteri(This->gltarget, GL_TEXTURE_MIN_FILTER, This->params.glparam.minfilter);
	glTexParameteri(This->gltarget, GL_TEXTURE_MAG_FILTER, This->params.glparam.magfilter);
	glTexParameteri(This->gltarget, GL_TEXTURE_WRAP_S, This->params.glparam.wraps);
	glTexParameteri(This->gltarget, GL_TEXTURE_WRAP_T, This->params.glparam.wrapt);
	glTexParameteri(This->gltarget, GL_TEXTURE_WRAP_R, This->params.glparam.wrapr);
	glTexParameteri(This->gltarget, GL_TEXTURE_MAX_LEVEL, This->mipcount - 1);
	for (i = 0; i < This->mipcount; i++)
	{
		do
		{
			ClearError();
			glTexImage2D(This->gltarget, i, This->format.glformat.internalformats[0],
				This->levels[i].width, This->levels[i].height, 0, This->format.glformat.format,
				This->format.glformat.type, NULL);
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (This->format.glformat.internalformats[1] == 0)
				{
					FIXME("Failed to create texture, cannot find internal format\n");
					break;
				}
				memmove(&This->format.glformat.internalformats[0],
					&This->format.glformat.internalformats[1], 3 * sizeof(GLint));
				This->format.glformat.internalformats[3] = 0;
			}
			else break;
		} while (1);
	}
}

void DXGLTextureGL__Lock(DXGLTexture *This, glExtensions *ext, GLuint miplevel, BYTE **ptr)
{
	if (!This->levels[miplevel].PBO)
	{
		ext->glGenBuffers(1, &This->levels[miplevel].PBO);
		ext->glBindBuffer(GL_PIXEL_PACK_BUFFER, This->levels[miplevel].PBO);
		ext->glBufferData(GL_PIXEL_PACK_BUFFER, This->levels[miplevel].pitch * This->levels[miplevel].height,
			NULL, GL_STREAM_READ);
	}
	else ext->glBindBuffer(GL_PIXEL_PACK_BUFFER, This->levels[miplevel].PBO);
	glBindTexture(This->gltarget, This->glhandle);
	glGetTexImage(This->gltarget, miplevel, This->format.glformat.format, This->format.glformat.type, 0);
	This->levels[miplevel].bufferptr = ext->glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
	*ptr = This->levels[miplevel].bufferptr;
	ext->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void DXGLTextureGL__Unlock(DXGLTexture *This, glExtensions *ext, GLuint miplevel)
{
	if (!This->levels[miplevel].bufferptr) return;
	ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, This->levels[miplevel].PBO);
	ext->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	This->levels[miplevel].bufferptr = NULL;
	glBindTexture(This->gltarget, This->glhandle);
	glTexSubImage2D(This->gltarget, miplevel, 0, 0, This->levels[miplevel].width, This->levels[miplevel].height,
		This->format.glformat.format, This->format.glformat.type, 0);
	ext->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
