// DXGL
// Copyright (C) 2012-2015 William Feely

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
#include "BufferObject.h"
#include "glTexture.h"
#include "glUtil.h"
#include "glRenderer.h"
#include <math.h>
#include "ddraw.h"
#include "scalers.h"

extern "C" {

// Use EXACTLY one line per entry.  Don't change layout of the list.
static const int START_TEXFORMATS = __LINE__;
const DDPIXELFORMAT texformats[] = 
{ // Size					Flags							FOURCC	bits	R/Ymask		G/U/Zmask	B/V/STmask	A/Zmask
	{sizeof(DDPIXELFORMAT),	DDPF_PALETTEINDEXED8,			0,		8,		0,			0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF0000,	0xFF00,		0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF,		0xFF00,		0xFF0000,	0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xE0,		0x1C,		0x3,		0xFF00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xF00,		0xF0,		0xF,		0xF000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0x7c00,		0x3E0,		0x1F,		0x8000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		32,		0xFF0000,	0xFF00,		0xFF,		0xFF000000},
	{sizeof(DDPIXELFORMAT), DDPF_LUMINANCE,					0,		8,		0xFF,		0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ALPHA,						0,		8,		0,			0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_LUMINANCE|DDPF_ALPHAPIXELS,0,		16,		0xFF,		0,			0,			0xFF00},
	{sizeof(DDPIXELFORMAT), DDPF_ZBUFFER,					0,		16,		0,			0xFFFF,		0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		24,		0,			0xFFFFFF00,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFF00,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFFFF,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFFFFFF00,	0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFF,		0xFFFFFF00,	0}
};
static const int END_TEXFORMATS = __LINE__ - 4;
int numtexformats;

void ClearError()
{
	do
	{
		if (glGetError() == GL_NO_ERROR) break;
	} while (1);
}

DWORD CalculateMipLevels(DWORD width, DWORD height)
{
	DWORD x, y;
	DWORD levels = 1;
	if ((!width) || (!height)) return 0;
	if ((width == 1) || (height == 1)) return 1;
	x = width;
	y = height;
miploop:
	x = max(1,(DWORD)floorf((float)x / 2.0f));
	y = max(1,(DWORD)floorf((float)y / 2.0f));
	levels++;
	if ((x == 1) || (y == 1)) return levels;
	else goto miploop;
}

void ShrinkMip(DWORD *x, DWORD *y)
{
	*x = max(1, (DWORD)floorf((float)*x / 2.0f));
	*y = max(1, (DWORD)floorf((float)*y / 2.0f));
}

void glTexture_Create(glExtensions *ext, glUtil *util, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei bigx, GLsizei bigy, HGLRC hrc, DWORD screenbpp)
{
	DWORD x, y;
	int i;
	*texture = (glTexture*)malloc(sizeof(glTexture));
	ZeroMemory(*texture, sizeof(glTexture));
	(*texture)->ddsd = *ddsd;
	(*texture)->mipmaps[0].bigx = bigx;
	(*texture)->mipmaps[0].bigy = bigy;
	(*texture)->minfilter = (*texture)->magfilter = GL_NEAREST;
	(*texture)->wraps = (*texture)->wrapt = GL_CLAMP_TO_EDGE;
	if (!(ddsd->dwFlags & DDSD_PIXELFORMAT))
	{
		(*texture)->ddsd.dwFlags &= DDSD_PIXELFORMAT;
		ZeroMemory(&(*texture)->ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
		(*texture)->ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		(*texture)->ddsd.ddpfPixelFormat.dwRGBBitCount = screenbpp;
		switch (screenbpp)
		{
		case 8:
			(*texture)->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB & DDPF_PALETTEINDEXED8;
			(*texture)->ddsd.ddpfPixelFormat.dwRBitMask = 0;
			(*texture)->ddsd.ddpfPixelFormat.dwGBitMask = 0;
			(*texture)->ddsd.ddpfPixelFormat.dwBBitMask = 0;
			break;
		case 15:
			(*texture)->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			(*texture)->ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
			(*texture)->ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
			(*texture)->ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
			(*texture)->ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			break;
		case 16:
			(*texture)->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			(*texture)->ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
			(*texture)->ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
			(*texture)->ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			break;
		case 24:
		case 32:
			(*texture)->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			(*texture)->ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			(*texture)->ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
			(*texture)->ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
			break;
		}
	}
	if (ddsd->dwFlags & DDSD_PITCH)
	{
		if (ddsd->lPitch > NextMultipleOf4(ddsd->dwWidth * (NextMultipleOf8(ddsd->ddpfPixelFormat.dwRGBBitCount) / 8)))
		{
			(*texture)->mipmaps[0].nonidealpitch = TRUE;
			(*texture)->mipmaps[0].pixelpitch = ddsd->lPitch / (NextMultipleOf8(ddsd->ddpfPixelFormat.dwRGBBitCount) / 8);
		}
	}
	else
	{
		(*texture)->ddsd.dwFlags |= DDSD_PITCH;
		(*texture)->ddsd.lPitch = (*texture)->mipmaps[0].pitch = NextMultipleOf4(ddsd->dwWidth * (NextMultipleOf8((*texture)->ddsd.ddpfPixelFormat.dwRGBBitCount) / 8));
	}
	if (ddsd->dwFlags & DDSD_LPSURFACE)
	{
		(*texture)->clientmem = TRUE;
		(*texture)->mipmaps[0].buffer = (char*)ddsd->lpSurface;
	}
	if (!(*texture)->mipmaps[0].buffer)
	{
		(*texture)->mipmaps[0].buffer = (char*)malloc((*texture)->ddsd.lPitch * (*texture)->ddsd.dwHeight);
		ZeroMemory((*texture)->mipmaps[0].buffer, (*texture)->ddsd.lPitch*(*texture)->ddsd.dwHeight);
	}
	if ((bigx != ddsd->dwWidth) || (bigy != ddsd->dwHeight))
	{
		(*texture)->mipmaps[0].bigbuffer = (char *)malloc(NextMultipleOf4((ddsd->ddpfPixelFormat.dwRGBBitCount * bigx) / 8) * bigy);
		ZeroMemory((*texture)->mipmaps[0].bigbuffer, NextMultipleOf4((ddsd->ddpfPixelFormat.dwRGBBitCount * bigx) / 8) * bigy);
	}
	if (!((*texture)->ddsd.dwFlags & DDSD_MIPMAPCOUNT))
	{
		(*texture)->ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
		(*texture)->ddsd.dwMipMapCount = 1;
	}
	if (!(*texture)->ddsd.dwMipMapCount)
		(*texture)->ddsd.dwMipMapCount = 1;
	(*texture)->ext = ext;
	glTexture__SetPixelFormat(*texture);
	x = (*texture)->ddsd.dwWidth;
	y = (*texture)->ddsd.dwHeight;
	for (i = 0; i < (*texture)->ddsd.dwMipMapCount; i++)
	{
		(*texture)->mipmaps[i].level = i;
		if (i == 0)
		{
			(*texture)->mipmaps[i].pitch = (*texture)->ddsd.lPitch;
			(*texture)->mipmaps[i].width = (*texture)->ddsd.dwWidth;
			(*texture)->mipmaps[i].height = (*texture)->ddsd.dwHeight;
		}
		else
		{
			ShrinkMip(&x, &y);
			(*texture)->mipmaps[i].buffer = (char*)malloc(NextMultipleOf4(x)*y);
			(*texture)->mipmaps[i].pitch = NextMultipleOf4(x);
			(*texture)->mipmaps[i].width = x;
			(*texture)->mipmaps[i].height = y;
			ZeroMemory((*texture)->mipmaps[i].buffer,
				(*texture)->mipmaps[i].pitch * (*texture)->mipmaps[i].height);
		}
	}
	(*texture)->util = util;
	glUtil_AddRef(util);
	(*texture)->refcount = 1;
	(*texture)->hrc = hrc;
	glTexture__CreateSimple(*texture);
}

void glTexture__SetPixelFormat(glTexture *This)
{
	int texformat = -1;
	int i;
	GLenum error;
	if (!numtexformats) numtexformats = END_TEXFORMATS - START_TEXFORMATS;
	for(i = 0; i < numtexformats; i++)
	{
		if(!memcmp(&texformats[i],&This->ddsd.ddpfPixelFormat,sizeof(DDPIXELFORMAT)))
		{
			texformat = i;
			break;
		}
	}
	switch(texformat)
	{
	case -1:
	case 0: // 8-bit palette
		if(This->ext->glver_major >= 3)
		{
			This->internalformats[0] = GL_R8;
			This->format = GL_RED;
		}
		else
		{
			This->internalformats[0] = GL_RGBA8;
			This->format = GL_LUMINANCE;
		}
		This->type = GL_UNSIGNED_BYTE;
		This->colororder = 4;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 1: // 8-bit RGB332
		This->internalformats[0] = GL_R3_G3_B2;
		This->internalformats[1] = GL_RGB8;
		This->internalformats[2] = GL_RGBA8;
		This->format = GL_RGB;
		This->type = GL_UNSIGNED_BYTE_3_3_2;
		This->colororder = 1;
		This->colorsizes[0] = 7;
		This->colorsizes[1] = 7;
		This->colorsizes[2] = 3;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 3;
		This->colorbits[1] = 3;
		This->colorbits[2] = 2;
		This->colorbits[3] = 0;
		break;
	case 2: // 16-bit RGB555
		This->internalformats[0] = GL_RGB5_A1;
		This->internalformats[1] = GL_RGBA8;
		This->format = GL_BGRA;
		This->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		This->colororder = 1;
		This->colorsizes[0] = 31;
		This->colorsizes[1] = 31;
		This->colorsizes[2] = 31;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 5;
		This->colorbits[1] = 5;
		This->colorbits[2] = 5;
		This->colorbits[3] = 1;
		break;
	case 3: // 16-bit RGB565
		This->internalformats[0] = GL_RGB565;
		This->internalformats[1] = GL_RGB8;
		This->internalformats[2] = GL_RGBA8;
		This->format = GL_RGB;
		This->type = GL_UNSIGNED_SHORT_5_6_5;
		This->colororder = 1;
		This->colorsizes[0] = 31;
		This->colorsizes[1] = 63;
		This->colorsizes[2] = 31;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 5;
		This->colorbits[1] = 6;
		This->colorbits[2] = 5;
		This->colorbits[3] = 0;
		break;
	case 4: // 24-bit RGB888
		This->internalformats[0] = GL_RGB8;
		This->internalformats[1] = GL_RGBA8;
		This->format = GL_BGR;
		This->type = GL_UNSIGNED_BYTE;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		break;
	case 5: // 32-bit RGB888
		This->internalformats[0] = GL_RGBA8;
		This->format = GL_BGRA;
		This->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		break;
	case 6: // 32-bit BGR888
		This->internalformats[0] = GL_RGBA8;
		This->format = GL_RGBA;
		This->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		This->colororder = 0;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 1;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 0;
		break;
	case 7: // 16-bit RGBA8332
		FIXME("Unusual texture format RGBA8332 not currently supported");
		This->colororder = 1;
		This->colorsizes[0] = 7;
		This->colorsizes[1] = 7;
		This->colorsizes[2] = 3;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 3;
		This->colorbits[1] = 3;
		This->colorbits[2] = 2;
		This->colorbits[3] = 8;
		break;
	case 8: // 16-bit RGBA4444
		This->internalformats[0] = GL_RGBA4;
		This->internalformats[1] = GL_RGBA8;
		This->format = GL_BGRA;
		This->type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		This->colororder = 1;
		This->colorsizes[0] = 15;
		This->colorsizes[1] = 15;
		This->colorsizes[2] = 15;
		This->colorsizes[3] = 15;
		This->colorbits[0] = 4;
		This->colorbits[1] = 4;
		This->colorbits[2] = 4;
		This->colorbits[3] = 4;
		break;
	case 9: // 16-bit RGBA1555
		This->internalformats[0] = GL_RGB5_A1;
		This->internalformats[1] = GL_RGBA8;
		This->format = GL_BGRA;
		This->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		This->colorbits[0] = 5;
		This->colorbits[1] = 5;
		This->colorbits[2] = 5;
		This->colorbits[3] = 1;
		break;
	case 10: // 32-bit RGBA8888
		This->internalformats[0] = GL_RGBA8;
		This->format = GL_BGRA;
		This->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		This->colororder = 1;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 8;
		This->colorbits[2] = 8;
		This->colorbits[3] = 8;
		break;
	case 11: // 8-bit Luminance
		This->internalformats[0] = GL_LUMINANCE8;
		This->internalformats[1] = GL_RGB8;
		This->internalformats[2] = GL_RGBA8;
		This->format = GL_LUMINANCE;
		This->type = GL_UNSIGNED_BYTE;
		This->colororder = 5;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 12: // 8-bit Alpha
		This->internalformats[0] = GL_ALPHA8;
		This->format = GL_ALPHA;
		This->type = GL_UNSIGNED_BYTE;
		This->colororder = 6;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 0;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		break;
	case 13: // 16-bit Luminance Alpha
		This->internalformats[0] = GL_LUMINANCE8_ALPHA8;
		This->internalformats[1] = GL_RGBA8;
		This->format = GL_LUMINANCE_ALPHA;
		This->type = GL_UNSIGNED_BYTE;
		This->colororder = 7;
		This->colorsizes[0] = 255;
		This->colorsizes[1] = 255;
		This->colorsizes[2] = 255;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 8;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		break;
	case 14: // 16-bit Z buffer
		This->internalformats[0] = GL_DEPTH_COMPONENT16;
		This->format = GL_DEPTH_COMPONENT;
		This->type = GL_UNSIGNED_SHORT;
		This->colororder = 4;
		This->colorsizes[0] = 65535;
		This->colorsizes[1] = 65535;
		This->colorsizes[2] = 65535;
		This->colorsizes[3] = 65535;
		This->colorbits[0] = 16;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 15: // 24-bit Z buffer
		This->internalformats[0] = GL_DEPTH_COMPONENT24;
		This->format = GL_DEPTH_COMPONENT;
		This->type = GL_UNSIGNED_INT;
		This->colororder = 4;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 16777215;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 16: // 32/24 bit Z buffer
		This->internalformats[0] = GL_DEPTH_COMPONENT24;
		This->format = GL_DEPTH_COMPONENT;
		This->type = GL_UNSIGNED_INT;
		This->colororder = 4;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 16777215;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 17: // 32-bit Z buffer
		This->internalformats[0] = GL_DEPTH_COMPONENT32;
		This->format = GL_DEPTH_COMPONENT;
		This->type = GL_UNSIGNED_INT;
		This->colororder = 4;
		This->colorsizes[0] = 4294967295;
		This->colorsizes[1] = 4294967295;
		This->colorsizes[2] = 4294967295;
		This->colorsizes[3] = 4294967295;
		This->colorbits[0] = 32;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 0;
		break;
	case 18: // 32-bit Z/Stencil buffer, depth LSB
		This->internalformats[0] = GL_DEPTH24_STENCIL8;
		This->format = GL_DEPTH_STENCIL;
		This->type = GL_UNSIGNED_INT_24_8;
		This->colororder = 7;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		break;
	case 19: // 32-bit Z/Stencil buffer, depth MSB
		This->internalformats[0] = GL_DEPTH24_STENCIL8;
		This->format = GL_DEPTH_STENCIL;
		This->type = GL_UNSIGNED_INT_24_8;
		This->colororder = 7;
		This->colorsizes[0] = 16777215;
		This->colorsizes[1] = 16777215;
		This->colorsizes[2] = 16777215;
		This->colorsizes[3] = 255;
		This->colorbits[0] = 24;
		This->colorbits[1] = 0;
		This->colorbits[2] = 0;
		This->colorbits[3] = 8;
		break;
	}
}

void glTexture__CreateSimple(glTexture *This)
{
	int i;
	GLenum error;
	glGenTextures(1, &This->id);
	glUtil_SetTexture(This->util, 0, This);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, This->minfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, This->magfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, This->wraps);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, This->wrapt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->ddsd.dwMipMapCount - 1);
	for (i = 0; i < This->ddsd.dwMipMapCount; i++)
	{
		do
		{
			ClearError();
			glTexImage2D(GL_TEXTURE_2D, i, This->internalformats[0], This->mipmaps[i].width, This->mipmaps[i].height,
				0, This->format, This->type, This->mipmaps[i].buffer);
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (This->internalformats[1] == 0)
				{
					FIXME("Failed to create texture, cannot find internal format");
					break;
				}
				memmove(&This->internalformats[0], &This->internalformats[1], 7 * sizeof(GLint));
				This->internalformats[7] = 0;
			}
			else break;
		} while (1);
	}
}


void glTexture_AddRef(glTexture *This)
{
	InterlockedIncrement(&This->refcount);
}

void glTexture_Release(glTexture *This, BOOL backend, struct glRenderer *renderer)
{
	ULONG refcount = InterlockedDecrement(&This->refcount);
	if (!refcount)
	{
		if (backend) glTexture__Destroy(This);
		else glRenderer_DeleteTexture(renderer, This);
	}
}

void glTexture__Destroy(glTexture *texture)
{
	if (texture->zbuffer) glTexture_Release(texture->zbuffer, TRUE, NULL);
	if (texture->palette) glTexture_Release(texture->palette, TRUE, NULL);
	if (texture->stencil) glTexture_Release(texture->stencil, TRUE, NULL);
	if (texture->dummycolor) glTexture_Release(texture->dummycolor, TRUE, NULL);
	if (texture->pboPack) BufferObject_Release(texture->pboPack);
	if (texture->pboUnpack) BufferObject_Release(texture->pboUnpack);
	for (int i = 0; i < texture->ddsd.dwMipMapCount; i++)
	{
		if (texture->mipmaps[i].buffer && !texture->clientmem && !i) free(texture->mipmaps[i].buffer);
		if (texture->mipmaps[i].bigbuffer) free(texture->mipmaps[i].bigbuffer);
		if (texture->mipmaps[i].gdibuffer) {}
		if (texture->mipmaps[i].dib.hdc) {}
		if (texture->mipmaps[i].fbo.fbo) glUtil_DeleteFBO(texture->util,&texture->mipmaps[i].fbo);
	}
	if (texture->bigid) glDeleteTextures(1, &texture->bigid);
	glDeleteTextures(1, &texture->id);
	glUtil_Release(texture->util);
	free(texture);
}

void glTexture__Modify(glTexture *texture, const DDSURFACEDESC2 *ddsd, GLsizei bigx, GLsizei bigy, BOOL preservedata)
{
	BOOL resize = FALSE;
	if (texture->mipmaps[0].dib.hdc) glTexture__DeleteDIB(texture, 0);
	if (texture->mipmaps[0].dirty & 2)
	{
		glTexture__Download(texture, 0);
		texture->mipmaps[0].dirty &= ~2;
	}
	if (ddsd->dwFlags & DDSD_WIDTH)
	{
		if (ddsd->dwWidth != texture->ddsd.dwWidth)
		{
			texture->ddsd.dwWidth = texture->mipmaps[0].width = ddsd->dwWidth;
			if (!(ddsd->dwFlags & DDSD_PITCH))
			{
				if (ddsd->dwFlags & DDSD_PIXELFORMAT)
					texture->ddsd.lPitch = NextMultipleOf4(ddsd->dwWidth *
						(NextMultipleOf8(ddsd->ddpfPixelFormat.dwRGBBitCount) / 8));
				else texture->ddsd.lPitch = NextMultipleOf4(ddsd->dwWidth *
						(NextMultipleOf8(texture->ddsd.ddpfPixelFormat.dwRGBBitCount) / 8));
			}
			resize = TRUE;
		}
	}
	if (ddsd->dwFlags & DDSD_PITCH)
	{
		if (ddsd->lPitch != texture->ddsd.lPitch)
		{
			texture->ddsd.lPitch = texture->mipmaps[0].pitch = ddsd->lPitch;
			resize = TRUE;
		}
	}
	if (ddsd->dwFlags & DDSD_HEIGHT)
	{
		if (ddsd->dwHeight != texture->ddsd.dwHeight)
		{
			texture->ddsd.dwHeight = texture->mipmaps[0].height = ddsd->dwHeight;
			resize = TRUE;
		}
	}
	if (ddsd->dwFlags & DDSD_PIXELFORMAT)
	{
		if (memcmp(&ddsd->ddpfPixelFormat, &texture->ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT)))
		{
			texture->ddsd.ddpfPixelFormat = ddsd->ddpfPixelFormat;
			glTexture__SetPixelFormat(texture);
			resize = TRUE;
		}
	}
	if (resize)
	{
		if (texture->clientmem)
		{
			if (ddsd->dwFlags & DDSD_LPSURFACE) texture->mipmaps[0].buffer = (char*)ddsd->lpSurface;
		}
		else
		{
			texture->mipmaps[0].buffer = (char*)realloc(texture->mipmaps[0].buffer, texture->ddsd.lPitch * texture->ddsd.dwHeight);
			if ((bigx != ddsd->dwWidth) || (bigy != ddsd->dwHeight))
				texture->mipmaps[0].bigbuffer = (char*)realloc(texture->mipmaps[0].bigbuffer,
					NextMultipleOf4((texture->ddsd.ddpfPixelFormat.dwRGBBitCount * bigx) / 8) * bigy);
		}
		glTexture__Upload(texture, 0, TRUE, TRUE);
	}
}

void glTexture__Upload(glTexture *This, int level, BOOL checkerror, BOOL realloc)
{
	glTexture__UploadSimple(This, level, checkerror, realloc);
}

void glTexture__UploadSimple(glTexture *This, int level, BOOL checkerror, BOOL realloc)
{
	GLenum error;
	GLsizei width;
	GLsizei height;
	char *data;
	if ((This->mipmaps[level].bigx != This->mipmaps[level].width)
		|| (This->mipmaps[level].bigy != This->mipmaps[level].height))
	{
		width = This->mipmaps[level].bigx;
		height = This->mipmaps[level].bigy;
		glTexture__ScaleUpload(This, level);
		data = This->mipmaps[level].bigbuffer;
	}
	else
	{
		width = This->mipmaps[level].width;
		height = This->mipmaps[level].height;
		data = This->mipmaps[level].buffer;
	}
	if (checkerror)
	{
		do
		{
			ClearError();
			if (This->ext->GLEXT_EXT_direct_state_access)
			{
				if (realloc)This->ext->glTextureImage2DEXT(This->id, GL_TEXTURE_2D, level, This->internalformats[0],
					width, height, 0, This->format, This->type, data);
				else This->ext->glTextureSubImage2DEXT(This->id, GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
			}
			else
			{
				glUtil_SetActiveTexture(This->util, 0);
				glUtil_SetTexture(This->util, 0, This);
				if (realloc)glTexImage2D(GL_TEXTURE_2D, level, This->internalformats[0], width, height, 0, This->format, This->type, data);
				else glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
			}
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (This->internalformats[1] == 0)
				{
					FIXME("Failed to update texture, cannot find internal format.  \
This shouldn't happen because all pixelformats have safe fallbacks.");
					break;
				}
				memmove(&This->internalformats[0], &This->internalformats[1], 7 * sizeof(GLint));
				This->internalformats[7] = 0;
			}
			else break;
		} while (1);
	}
	else
	{
		if (This->ext->GLEXT_EXT_direct_state_access)
		{
			if (realloc)This->ext->glTextureImage2DEXT(This->id, GL_TEXTURE_2D, level, This->internalformats[0],
				width, height, 0, This->format, This->type, data);
			else This->ext->glTextureSubImage2DEXT(This->id, GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
		}
		else
		{
			glUtil_SetActiveTexture(This->util, 0);
			glUtil_SetTexture(This->util, 0, This);
			if (realloc)glTexImage2D(GL_TEXTURE_2D, level, This->internalformats[0], width, height, 0, This->format, This->type, data);
			else glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
		}
	}
	This->mipmaps[level].dirty &= ~1;
}

void glTexture__Download(glTexture *This, int level)
{
	glTexture__DownloadSimple(This, level);
}

void glTexture__DownloadSimple(glTexture *This, int level)
{
	char *data;
	BOOL resize = FALSE;
	if ((This->mipmaps[level].bigx != This->mipmaps[level].width) || (This->mipmaps[level].bigy != This->mipmaps[level].height))
	{
		data = This->mipmaps[level].bigbuffer;
		resize = TRUE;
	}
	else data = This->mipmaps[level].buffer;
	if(This->ext->GLEXT_EXT_direct_state_access) This->ext->glGetTextureImageEXT(This->id,GL_TEXTURE_2D,level,This->format,This->type,data);
	else
	{
		glUtil_SetActiveTexture(This->util, 0);
		glUtil_SetTexture(This->util, 0,This);
		glGetTexImage(GL_TEXTURE_2D,level,This->format,This->type,data);
	}
	if (resize) glTexture__ScaleDownload(This, level);
}

BOOL glTexture__Repair(glTexture *This)
{
	// data should be null to create uninitialized texture or be pointer to top-level
	// buffer to retain texture data
	glTexture newtexture;
	GLenum error;
	char *data;
	if ((This->mipmaps[0].bigx != This->ddsd.dwWidth) || (This->mipmaps[0].bigy != This->ddsd.dwHeight))
		data = This->mipmaps[0].bigbuffer;
	else data = This->mipmaps[0].buffer;
	memcpy(&newtexture, This, sizeof(glTexture));
	if (This->internalformats[1] == 0) return FALSE;
	glGenTextures(1, &newtexture.id);
	glUtil_SetActiveTexture(This->util, 0);
	glUtil_SetTexture(This->util, 0, This);
	glGetTexImage(GL_TEXTURE_2D, 0, This->format, This->type, data);
	if (This->mipmaps[0].dirty) This->mipmaps[0].dirty |= 2;
	glUtil_SetTexture(This->util, 0, &newtexture);
	do
	{
		memmove(&newtexture.internalformats[0], &newtexture.internalformats[1], 7 * sizeof(GLint));
		newtexture.internalformats[7] = 0;
		ClearError();
		glTexImage2D(GL_TEXTURE_2D, 0, newtexture.internalformats[0], newtexture.ddsd.dwWidth, newtexture.ddsd.dwHeight,
			0, newtexture.format, newtexture.type, data);
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			if (newtexture.internalformats[1] == 0)
			{
				FIXME("Failed to repair texture, cannot find internal format");
				break;
			}
		}
		else break;
	} while (1);
	glDeleteTextures(1, &This->id);
	memcpy(This, &newtexture, sizeof(glTexture));
	return TRUE;
}

void glTexture__SetFilter(glTexture *This, int level, GLint mag, GLint min, glExtensions *ext, glUtil *util)
{
	if (This)
	{
		if (!ext) ext = This->ext;
		if (!util) util = This->util;
	}
	else
	{
		if (!ext) return;
		if (!util) return;
	}
	switch (dxglcfg.texfilter)
	{
	default:
	case 0:
		break;
	case 1:
		mag = min = GL_NEAREST;
		break;
	case 2:
		mag = min = GL_LINEAR;
		break;
	case 3:
		mag = GL_NEAREST;
		min = GL_NEAREST_MIPMAP_NEAREST;
		break;
	case 4:
		mag = GL_NEAREST;
		min = GL_NEAREST_MIPMAP_LINEAR;
		break;
	case 5:
		mag = GL_LINEAR;
		min = GL_LINEAR_MIPMAP_NEAREST;
		break;
	case 6:
		mag = GL_LINEAR;
		min = GL_LINEAR_MIPMAP_LINEAR;
		break;
	}
	if (ext->GLEXT_ARB_sampler_objects)
	{
		ext->glSamplerParameteri(util->samplers[level].id, GL_TEXTURE_MAG_FILTER, mag);
		ext->glSamplerParameteri(util->samplers[level].id, GL_TEXTURE_MIN_FILTER, min);
	}
	else
	{
		if (ext->GLEXT_ARB_direct_state_access)
		{
			ext->glTextureParameteri(This->id, GL_TEXTURE_MAG_FILTER, mag);
			ext->glTextureParameteri(This->id, GL_TEXTURE_MIN_FILTER, min);
		}
		else if (ext->GLEXT_EXT_direct_state_access)
		{
			ext->glTextureParameteriEXT(This->id, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
			ext->glTextureParameteriEXT(This->id, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		}
		else
		{
			glUtil_SetTexture(util, level, This);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		}
	}
	if (This)
	{
		This->magfilter = mag;
		This->minfilter = min;
	}
}

void glTexture__ScaleUpload(glTexture *This, int level)
{
	switch (This->ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
	case 8:
		ScaleNearest8(This->mipmaps[level].bigbuffer, This->mipmaps[level].buffer, This->mipmaps[level].bigx, This->mipmaps[level].bigy,
			This->mipmaps[level].width, This->mipmaps[level].height, This->mipmaps[level].pitch, NextMultipleOf4(This->mipmaps[level].bigx));
		break;
	case 15:
	case 16:
		ScaleNearest16(This->mipmaps[level].bigbuffer, This->mipmaps[level].buffer, This->mipmaps[level].bigx, This->mipmaps[level].bigy,
			This->mipmaps[level].width, This->mipmaps[level].height, This->mipmaps[level].pitch / 2, NextMultipleOf2(This->mipmaps[level].bigx));
		break;
	case 24:
		ScaleNearest24(This->mipmaps[level].bigbuffer, This->mipmaps[level].buffer, This->mipmaps[level].bigx, This->mipmaps[level].bigy,
			This->mipmaps[level].width, This->mipmaps[level].height, This->mipmaps[level].pitch, NextMultipleOf4(This->mipmaps[level].bigx * 3));
		break;
	case 32:
		ScaleNearest32(This->mipmaps[level].bigbuffer, This->mipmaps[level].buffer, This->mipmaps[level].bigx, This->mipmaps[level].bigy,
			This->mipmaps[level].width, This->mipmaps[level].height, This->mipmaps[level].pitch, This->mipmaps[level].bigx);
		break;
	}
}

void glTexture__ScaleDownload(glTexture *This, int level)
{
	switch (This->ddsd.ddpfPixelFormat.dwRGBBitCount)
	{
	case 8:
		ScaleNearest8(This->mipmaps[level].buffer, This->mipmaps[level].bigbuffer, This->mipmaps[level].width, This->mipmaps[level].height,
			This->mipmaps[level].bigx, This->mipmaps[level].bigy, NextMultipleOf4(This->mipmaps[level].bigx), This->mipmaps[level].pitch);
		break;
	case 15:
	case 16:
		ScaleNearest16(This->mipmaps[level].buffer, This->mipmaps[level].bigbuffer, This->mipmaps[level].width, This->mipmaps[level].height,
			This->mipmaps[level].bigx, This->mipmaps[level].bigy, NextMultipleOf2(This->mipmaps[level].bigx), This->mipmaps[level].pitch / 2);
		break;
	case 24:
		ScaleNearest8(This->mipmaps[level].buffer, This->mipmaps[level].bigbuffer, This->mipmaps[level].width, This->mipmaps[level].height,
			This->mipmaps[level].bigx, This->mipmaps[level].bigy, NextMultipleOf4(This->mipmaps[level].bigx * 3), This->mipmaps[level].pitch);
		break;
	case 32:
		ScaleNearest8(This->mipmaps[level].buffer, This->mipmaps[level].bigbuffer, This->mipmaps[level].width, This->mipmaps[level].height,
			This->mipmaps[level].bigx, This->mipmaps[level].bigy, This->mipmaps[level].bigx, This->mipmaps[level].pitch);
		break;
	}
}

HRESULT glTexture__Lock(glTexture *This, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int level)
{
	if (This->mipmaps[level].locked) return DDERR_SURFACEBUSY;
	memcpy(ddsd, &This->ddsd, sizeof(DDSURFACEDESC2));
	ddsd->dwWidth = This->mipmaps[level].width;
	ddsd->dwHeight = This->mipmaps[level].height;
	ddsd->lPitch = This->mipmaps[level].pitch;
	This->mipmaps[level].dirty |= 1;
	if (This->mipmaps[level].dirty & 2) glTexture__Download(This, level);
	ddsd->lpSurface = This->mipmaps[level].buffer;
	if (r)
	{
		ULONG_PTR ptr = (ULONG_PTR)ddsd->lpSurface;
		ptr += (r->left * (ddsd->ddpfPixelFormat.dwRGBBitCount / 8));
		ptr += (r->top * (ddsd->lPitch));
		ddsd->lpSurface = (LPVOID)ptr;
	}
	This->mipmaps[level].locked++;
	return DD_OK;
}

void glTexture__Unlock(glTexture *This, RECT *r, int level)
{
	This->mipmaps[level].dirty |= 1;
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
		glTexture__Upload(This, level, FALSE, FALSE);
	if (This->mipmaps[level].locked) This->mipmaps[level].locked--;
}

BOOL glTexture__CreateDIB(glTexture *This, int level)
{
	DWORD colormasks[3];
	This->mipmaps[level].dib.hdc = CreateCompatibleDC(NULL);
	if (This->ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
	{
		This->mipmaps[level].dib.info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
		ZeroMemory(This->mipmaps[level].dib.info, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));
	}
	else
	{
		This->mipmaps[level].dib.info = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + (3 * sizeof(DWORD)));
		ZeroMemory(This->mipmaps[level].dib.info, sizeof(BITMAPINFOHEADER) + (3 * sizeof(DWORD)));
	}
	This->mipmaps[level].dib.info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	This->mipmaps[level].dib.info->bmiHeader.biWidth = This->mipmaps[level].width;
	This->mipmaps[level].dib.info->bmiHeader.biHeight = -(signed)This->mipmaps[level].height;
	This->mipmaps[level].dib.info->bmiHeader.biPlanes = 1;
	This->mipmaps[level].dib.info->bmiHeader.biCompression = BI_RGB;
	This->mipmaps[level].dib.info->bmiHeader.biBitCount = NextMultipleOf8(This->ddsd.ddpfPixelFormat.dwRGBBitCount);
	if (This->ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
	{
		colormasks[0] = This->ddsd.ddpfPixelFormat.dwRBitMask;
		colormasks[1] = This->ddsd.ddpfPixelFormat.dwGBitMask;
		colormasks[2] = This->ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(This->mipmaps[level].dib.info->bmiColors, colormasks, 3 * sizeof(DWORD));
	}
	This->mipmaps[level].dib.info->bmiHeader.biWidth = This->mipmaps[level].pitch /
		(This->mipmaps[level].dib.info->bmiHeader.biBitCount / 8);
	if ((This->ddsd.ddpfPixelFormat.dwRGBBitCount == 16) || (This->ddsd.ddpfPixelFormat.dwRGBBitCount == 32))
		This->mipmaps[level].dib.info->bmiHeader.biCompression = BI_BITFIELDS;
	else This->mipmaps[level].dib.info->bmiHeader.biCompression = BI_RGB;
	This->mipmaps[level].dib.hbitmap = CreateDIBSection(This->mipmaps[level].dib.hdc,
		This->mipmaps[level].dib.info, DIB_RGB_COLORS, (void**)&This->mipmaps[level].dib.pixels, NULL, 0);
	HGDIOBJ temp = SelectObject(This->mipmaps[level].dib.hdc, This->mipmaps[level].dib.hbitmap);
	DeleteObject(temp);
	return TRUE;
}

void glTexture__DeleteDIB(glTexture *This, int level)
{
	DeleteObject(This->mipmaps[level].dib.hbitmap);
	This->mipmaps[level].dib.hbitmap = NULL;
	DeleteDC(This->mipmaps[level].dib.hdc);
	This->mipmaps[level].dib.hdc = NULL;
	free(This->mipmaps[level].dib.info);
	This->mipmaps[level].dib.info = NULL;
}

HDC glTexture__GetDC(glTexture *This, glRenderer *renderer, int level)
{
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 ddsdPal;
	glTexture *pal;
	DWORD *palptr;
	DWORD colors[256];
	int i;
	if (This->mipmaps[level].hdc) return (HDC)INVALID_HANDLE_VALUE;
	if (!This->mipmaps[level].dib.hdc) glTexture__CreateDIB(This, level);
	This->mipmaps[level].hdc = This->mipmaps[level].dib.hdc;
	glTexture__Lock(This, NULL, &ddsd, 0, level);
	if (This->ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		if (This->palette) pal = This->palette;
		else pal = renderer->primary->palette;
		glTexture__Lock(pal, NULL, &ddsdPal, 0, 0);
		palptr = (DWORD*)ddsdPal.lpSurface;
		for (i = 0; i < 256; i++)
			colors[i] = ((palptr[i] & 0x0000FF) << 16) | (palptr[i] & 0x00FF00) | ((palptr[i] & 0xFF0000) >> 16);
		SetDIBColorTable(This->mipmaps[level].dib.hdc, 0, 256, (RGBQUAD*)colors);
		glTexture__Unlock(pal, NULL, 0);
	}
	GdiFlush();
	memcpy(This->mipmaps[level].dib.pixels, ddsd.lpSurface, ddsd.lPitch*ddsd.dwHeight);
	return This->mipmaps[level].hdc;
}

void glTexture__ReleaseDC(glTexture *This, int level)
{
	GetDIBits(This->mipmaps[level].dib.hdc, This->mipmaps[level].dib.hbitmap,0,This->mipmaps[level].height,
		This->mipmaps[level].buffer, This->mipmaps[level].dib.info, DIB_RGB_COLORS);
	This->mipmaps[level].hdc = NULL;
	glTexture__Unlock(This, NULL, level);
}

}