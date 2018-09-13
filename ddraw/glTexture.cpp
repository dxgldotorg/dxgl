// DXGL
// Copyright (C) 2012-2016 William Feely

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
#include "ddraw.h"
#include "BufferObject.h"
#include "timer.h"
#include "glRenderer.h"
#include "glTexture.h"
#include "glUtil.h"
#include "util.h"
#include <math.h>
#include "scalers.h"

extern "C" {

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
static const int START_TEXFORMATS = __LINE__;
const DDPIXELFORMAT texformats[] = 
{ // Size					Flags							FOURCC	bits	R/Ymask		G/U/Zmask	B/V/STmask	A/Zmask
	{sizeof(DDPIXELFORMAT),	DDPF_PALETTEINDEXED8,			0,		8,		0,			0,			0,			0},  // 8-bit paletted
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},  // 8 bit 332
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},  // 15 bit 555
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},  // 16 bit 565
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF0000,	0xFF00,		0xFF,		0},  // 24 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},  // 32 bit 888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF,		0xFF00,		0xFF0000,	0},  // 32 bit 888 BGR
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

HRESULT glTexture_Create(const DDSURFACEDESC2 *ddsd, glTexture **texture, struct glRenderer *renderer, GLint bigwidth, GLint bigheight, BOOL zhasstencil, BOOL backend)
{
	glTexture *newtexture;
	if (!texture) return DDERR_INVALIDPARAMS;
	if (!IsWritablePointer(texture, sizeof(glTexture*), TRUE))
		return DDERR_EXCEPTION;
	if (ddsd->dwSize != sizeof(DDSURFACEDESC2)) return DDERR_INVALIDPARAMS;
	if ((ddsd->dwFlags & DDSD_PIXELFORMAT) && (ddsd->ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT)))
		return DDERR_INVALIDPARAMS;
	newtexture = (glTexture*)malloc(sizeof(glTexture));
	if (!newtexture) return DDERR_OUTOFMEMORY;
	ZeroMemory(newtexture, sizeof(glTexture));
	memcpy(&newtexture->levels[0].ddsd, ddsd, sizeof(DDSURFACEDESC2));
	newtexture->useconv = FALSE;
	if (bigwidth)
	{
		newtexture->bigwidth = bigwidth;
		newtexture->bigheight = bigheight;
		newtexture->bigprimary = TRUE;
	}
	else newtexture->bigprimary = FALSE;
	if (newtexture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
	{
		newtexture->levels[0].ddsd.dwFlags |= DDSD_PIXELFORMAT;
		newtexture->levels[0].ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
		if (!newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBufferBitDepth)
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBufferBitDepth = newtexture->levels[0].ddsd.dwRefreshRate;
		switch (newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBufferBitDepth)
		{
		case 8:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBitMask = 0xFF;
			break;
		case 16:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFF;
			break;
		case 24:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFF;
			break;
		case 32:
		default:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFFFF;
			break;
		}
		newtexture->zhasstencil = zhasstencil;
	}

	if (!(newtexture->levels[0].ddsd.dwFlags & DDSD_PIXELFORMAT))
	{
		ZeroMemory(&newtexture->levels[0].ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
		newtexture->levels[0].ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		newtexture->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount = glRenderer_GetBPP(renderer);
		switch (glRenderer_GetBPP(renderer))
		{
		case 8:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRBitMask = 0;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwGBitMask = 0;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwBBitMask = 0;
			newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth);
			break;
		case 15:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth * 2);
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
			break;
		case 16:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth * 2);
			break;
		case 24:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
			newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth * 3);
			break;
		case 32:
		default:
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
			newtexture->levels[0].ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
			newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth * 4);
			break;
		}
	}
	else newtexture->levels[0].ddsd.lPitch = NextMultipleOf4(newtexture->levels[0].ddsd.dwWidth *
		(newtexture->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount / 8));
	/*if (!(newtexture->levels[0].ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		newtexture->pixelformat = newtexture->levels[0].ddsd.ddpfPixelFormat;
		if (newtexture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
			newtexture->miplevel = newtexture->levels[0].ddsd.dwMipMapCount;
		else texture->miplevel = 1;
		glRenderer_MakeTexture(ddInterface->renderer, texture, fakex, fakey);
	}*/

	newtexture->renderer = renderer;
	newtexture->refcount = 1;
	// Fill in mipmaps
	if (!newtexture->miplevel) newtexture->miplevel = 1;
	if (newtexture->miplevel > 1)
	{
		for (int i = 1; i < newtexture->miplevel; i++)
		{
			memcpy(&newtexture->levels[i].ddsd, &newtexture->levels[0].ddsd, sizeof(DDSURFACEDESC2));
			newtexture->levels[i].ddsd.dwMipMapCount = newtexture->levels[0].ddsd.dwMipMapCount - i;
			newtexture->levels[i].ddsd.dwWidth = max(1, (DWORD)floorf((float)newtexture->levels[i - 1].ddsd.dwWidth / 2.0f));
			newtexture->levels[i].ddsd.dwHeight = max(1, (DWORD)floorf((float)newtexture->levels[i - 1].ddsd.dwHeight / 2.0f));
			newtexture->levels[i].ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
		}
	}
	if (backend) glTexture__FinishCreate(newtexture);
	else glRenderer_MakeTexture(renderer, newtexture);
	*texture = newtexture;
	return DD_OK;
}
ULONG glTexture_AddRef(glTexture *This)
{
	InterlockedIncrement((LONG*)&This->refcount);
	return This->refcount;
}
ULONG glTexture_Release(glTexture *This, BOOL backend)
{
	ULONG ret;
	ret = InterlockedDecrement((LONG*)&This->refcount);
	if (This->refcount == 0)
	{
		if (This->palette) glTexture_Release(This->palette, backend);
		if (This->stencil) glTexture_Release(This->stencil, backend);
		if (This->dummycolor) glTexture_Release(This->dummycolor, backend);
		for (int i = 0; i < This->miplevel; i++)
		{
			if (This->levels[i].buffer) free(This->levels[i].buffer);
			if (This->levels[i].bigbuffer) free(This->levels[i].bigbuffer);
			if (This->levels[i].bitmapinfo) free(This->levels[i].bitmapinfo);
		}
		if (backend) glTexture__Destroy(This);
		else glRenderer_DeleteTexture(This->renderer, This);
	}
	return ret;
}
HRESULT glTexture_Lock(glTexture *This, GLint level, LPRECT r, LPDDSURFACEDESC2 ddsd, DWORD flags, BOOL backend)
{
	if (level > (This->levels[0].ddsd.dwMipMapCount - 1)) return DDERR_INVALIDPARAMS;
	if (!ddsd) return DDERR_INVALIDPARAMS;
	InterlockedIncrement((LONG*)&This->levels[level].locked);
	if (backend)
	{
		if (This->levels[level].dirty & 2) glTexture__Download(This, level);
	}
	else
	{
		if (This->levels[level].dirty & 2) glRenderer_DownloadTexture(This->renderer, This, level);
	}
	This->levels[level].dirty |= 1;
	if (r)
	{
		ULONG_PTR ptr = (ULONG_PTR)This->levels[level].buffer;
		ptr += (r->left * (This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount / 8));
		ptr += (r->top * (This->levels[level].ddsd.lPitch));
		This->levels[level].ddsd.lpSurface = (LPVOID)ptr;
	}
	else This->levels[level].ddsd.lpSurface = This->levels[level].buffer;
	memcpy(ddsd, &This->levels[level].ddsd, sizeof(DDSURFACEDESC2));
	return DD_OK;
}
HRESULT glTexture_Unlock(glTexture *This, GLint level, LPRECT r, BOOL backend)
{
	if (level > (This->levels[0].ddsd.dwMipMapCount - 1)) return DDERR_INVALIDPARAMS;
	InterlockedDecrement((LONG*)&This->levels[level].locked);
	if ((This->miplevel > 1) || dxglcfg.DebugUploadAfterUnlock)
	{
		if (backend) glTexture__Upload(This, level);
		else glRenderer_UploadTexture(This->renderer, This, level);
	}
	return DD_OK;
}
HRESULT glTexture_GetDC(glTexture *This, GLint level, HDC *hdc, glDirectDrawPalette *palette)
{
	HRESULT error;
	DWORD colors[256];
	DWORD colormasks[3];
	LPVOID surface;
	if (This->levels[level].hdc) return DDERR_DCALREADYCREATED;
	if (!This->levels[level].bitmapinfo)
	{
		This->levels[level].bitmapinfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO) + (255 * sizeof(RGBQUAD)));
		ZeroMemory(This->levels[level].bitmapinfo, sizeof(BITMAPINFO) + (255 * sizeof(RGBQUAD)));
		This->levels[level].bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		This->levels[level].bitmapinfo->bmiHeader.biWidth = This->levels[level].ddsd.dwWidth;
		This->levels[level].bitmapinfo->bmiHeader.biHeight = -(signed)This->levels[level].ddsd.dwHeight;
		This->levels[level].bitmapinfo->bmiHeader.biPlanes = 1;
		This->levels[level].bitmapinfo->bmiHeader.biCompression = BI_RGB;
		This->levels[level].bitmapinfo->bmiHeader.biBitCount = (WORD)This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount;
		if (This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
		{
			colormasks[0] = This->levels[level].ddsd.ddpfPixelFormat.dwRBitMask;
			colormasks[1] = This->levels[level].ddsd.ddpfPixelFormat.dwGBitMask;
			colormasks[2] = This->levels[level].ddsd.ddpfPixelFormat.dwBBitMask;
			memcpy(This->levels[level].bitmapinfo->bmiColors, colormasks, 3 * sizeof(DWORD));
		}
	}
	error = glTexture_Lock(This, level, NULL, &This->levels[level].ddsd, 0, FALSE);
	if (error != DD_OK) return error;
	This->levels[level].hdc = CreateCompatibleDC(NULL);
	This->levels[level].bitmapinfo->bmiHeader.biWidth = This->levels[level].ddsd.lPitch /
		(This->levels[level].bitmapinfo->bmiHeader.biBitCount / 8);
	if ((This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount == 8) && palette)
	{
		memcpy(colors, palette->palette, 1024);
		for (int i = 0; i < 256; i++)
			colors[i] = ((colors[i] & 0x0000FF) << 16) | (colors[i] & 0x00FF00) | ((colors[i] & 0xFF0000) >> 16);
		memcpy(This->levels[level].bitmapinfo->bmiColors, colors, 1024);
	}
	if (This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
		This->levels[level].bitmapinfo->bmiHeader.biCompression = BI_BITFIELDS;
	else This->levels[level].bitmapinfo->bmiHeader.biCompression = BI_RGB;
	This->levels[level].hbitmap = CreateDIBSection(This->levels[level].hdc,
		This->levels[level].bitmapinfo, DIB_RGB_COLORS, &surface, NULL, 0);
	memcpy(surface, This->levels[level].ddsd.lpSurface,
		This->levels[level].ddsd.lPitch*This->levels[level].ddsd.dwHeight);
	HGDIOBJ temp = SelectObject(This->levels[level].hdc, This->levels[level].hbitmap);
	DeleteObject(temp);
	*hdc = This->levels[level].hdc;
	return DD_OK;
}
HRESULT glTexture_ReleaseDC(glTexture *This, GLint level, HDC hdc)
{
	if (This->levels[level].hdc != hdc) return DDERR_INVALIDPARAMS;
	GetDIBits(This->levels[level].hdc, This->levels[level].hbitmap, 0,
		This->levels[level].ddsd.dwHeight, This->levels[level].ddsd.lpSurface,
		This->levels[level].bitmapinfo, DIB_RGB_COLORS);
	glTexture_Unlock(This, level, NULL, FALSE);
	DeleteDC(This->levels[level].hdc);
	This->levels[level].hdc = NULL;
	DeleteObject(This->levels[level].hbitmap);
	This->levels[level].hbitmap = NULL;
	return DD_OK;
}
void glTexture_SetPalette(glTexture *This, glTexture *palette, BOOL backend)
{
	if (This->palette) glTexture_Release(This->palette, backend);
	This->palette = palette;
	if (palette) glTexture_AddRef(palette);
}
void glTexture_SetStencil(glTexture *This, glTexture *stencil, BOOL backend)
{
	if (This->stencil) glTexture_Release(This->stencil, backend);
	This->stencil = stencil;
	if (stencil) glTexture_AddRef(stencil);
}
void glTexture_CreateDummyColor(glTexture *This, BOOL backend)
{
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, &ddsddummycolor, sizeof(DDSURFACEDESC2));
	ddsd.dwWidth = This->levels[0].ddsd.dwWidth;
	ddsd.lPitch = NextMultipleOf4(ddsd.dwWidth * 2);
	ddsd.dwHeight = This->levels[0].ddsd.dwHeight;
	glTexture_Create(&ddsd, &This->dummycolor, This->renderer, ddsd.dwWidth, ddsd.dwHeight, FALSE, backend);
}
void glTexture_DeleteDummyColor(glTexture *This, BOOL backend)
{
	if(This->dummycolor) glTexture_Release(This->dummycolor, backend);
	This->dummycolor = NULL;
}
HRESULT glTexture__SetSurfaceDesc(glTexture *This, LPDDSURFACEDESC2 ddsd)
{
	// FIXME:  Implement SetSurfaceDesc fully
	BOOL sizechanged = FALSE;
	if (This->miplevel > 1) return DDERR_UNSUPPORTED; // Not supported by DDraw
	if (ddsd->dwFlags & DDSD_PIXELFORMAT) return DDERR_UNSUPPORTED; //FIXME
	if (ddsd->dwFlags & DDSD_LPSURFACE) return DDERR_UNSUPPORTED; //FIXME
	if (ddsd->dwFlags & DDSD_WIDTH)
	{
		This->levels[0].ddsd.dwWidth = ddsd->dwWidth;
		if (ddsd->dwFlags & DDSD_PITCH) This->levels[0].ddsd.lPitch = ddsd->lPitch;
		else This->levels[0].ddsd.lPitch = NextMultipleOf4(ddsd->dwWidth*(This->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount / 8));
		sizechanged = TRUE;
	}
	if (ddsd->dwFlags & DDSD_HEIGHT)
	{
		This->levels[0].ddsd.dwHeight = ddsd->dwHeight;
		sizechanged = TRUE;
	}
	if (sizechanged) glTexture__Upload2(This, 0,
		This->levels[0].ddsd.dwWidth, This->levels[0].ddsd.dwHeight, FALSE, TRUE, This->renderer->util);
	return DD_OK;
}
void glTexture__Download(glTexture *This, GLint level)
{
	int bpp = This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount;
	int x = This->levels[level].ddsd.dwWidth;
	int y = This->levels[level].ddsd.dwHeight;
	int bigpitch = NextMultipleOf4((bpp / 8)*This->bigwidth);
	int pitch = This->levels[level].ddsd.lPitch;
	int bigx, bigy;
	if (level)
	{
		bigx = This->levels[level].ddsd.dwWidth;
		bigy = This->levels[level].ddsd.dwHeight;
	}
	else
	{
		bigx = This->bigwidth;
		bigy = This->bigheight;
	}
	if ((bigx == x && bigy == y) || !This->levels[level].bigbuffer)
	{
		if (This->renderer->ext->GLEXT_EXT_direct_state_access)
			This->renderer->ext->glGetTextureImageEXT(This->id, GL_TEXTURE_2D, level, This->format, This->type, This->levels[level].buffer);
		else
		{
			glUtil_SetActiveTexture(This->renderer->util, 0);
			glUtil_SetTexture(This->renderer->util, 0, This);
			glGetTexImage(GL_TEXTURE_2D, level, This->format, This->type, This->levels[level].buffer);
		}
	}
	else
	{
		if (This->renderer->ext->GLEXT_EXT_direct_state_access)
			This->renderer->ext->glGetTextureImageEXT(This->id, GL_TEXTURE_2D, level, This->format, This->type, This->levels[level].bigbuffer);
		else
		{
			glUtil_SetActiveTexture(This->renderer->util, 0);
			glUtil_SetTexture(This->renderer->util, 0, This);
			glGetTexImage(GL_TEXTURE_2D, level, This->format, This->type, This->levels[level].bigbuffer);
		}
		switch (bpp)
		{
		case 8:
			ScaleNearest8(This->levels[level].buffer, This->levels[level].bigbuffer,
				x, y, bigx, bigy, bigpitch, pitch);
			break;
		case 15:
		case 16:
			ScaleNearest16(This->levels[level].buffer, This->levels[level].bigbuffer,
				x, y, bigx, bigy, bigpitch / 2, pitch / 2);
			break;
		case 24:
			ScaleNearest24(This->levels[level].buffer, This->levels[level].bigbuffer,
				x, y, bigx, bigy, bigpitch, pitch);
			break;
		case 32:
			ScaleNearest32(This->levels[level].buffer, This->levels[level].bigbuffer,
				x, y, bigx, bigy, bigpitch / 4, pitch / 4);
			break;
		}
	}
	This->levels[level].dirty &= ~2;
}

void glTexture__Upload2(glTexture *This, int level, int width, int height, BOOL checkerror, BOOL dorealloc, glUtil *util)
{
	GLenum error;
	void *data;
	if (dorealloc)
	{
		This->levels[level].ddsd.dwWidth = width;
		This->levels[level].ddsd.dwHeight = height;
		This->bigwidth = width;
		This->bigheight = height;
		This->levels[level].buffer = (char*)realloc(This->levels[level].buffer,
			NextMultipleOf4((This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount *
			This->levels[level].ddsd.dwWidth) / 8) * This->levels[level].ddsd.dwHeight);
		if ((level == 0) && ((This->levels[level].ddsd.dwWidth != This->bigwidth) ||
			(This->levels[level].ddsd.dwHeight != This->bigheight)))
			This->levels[level].bigbuffer = (char *)realloc(This->levels[level].bigbuffer,
				NextMultipleOf4((This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount *
				This->bigwidth) / 8) * This->bigheight);
	}
	if ((level == 0) && ((This->levels[level].ddsd.dwWidth != This->bigwidth) ||
		(This->levels[level].ddsd.dwHeight != This->bigheight)))
		data = This->levels[level].bigbuffer;
	else data = This->levels[level].buffer;
	if (checkerror)
	{
		do
		{
			ClearError();
			if (This->renderer->ext->GLEXT_EXT_direct_state_access)
			{
				if (dorealloc)This->renderer->ext->glTextureImage2DEXT(This->id, GL_TEXTURE_2D, level,
					This->internalformats[0], width, height, 0, This->format, This->type, data);
				else This->renderer->ext->glTextureSubImage2DEXT(This->id, GL_TEXTURE_2D, level,
					0, 0, width, height, This->format, This->type, data);
			}
			else
			{
				glUtil_SetActiveTexture(util, 0);
				glUtil_SetTexture(util, 0, This);
				if (dorealloc) glTexImage2D(GL_TEXTURE_2D, level, This->internalformats[0], width, height, 0, This->format, This->type, data);
				else glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
			}
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (This->internalformats[1] == 0)
				{
					FIXME("Failed to update texture, cannot find internal format");
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
		if (This->renderer->ext->GLEXT_EXT_direct_state_access)
		{
			if (dorealloc)This->renderer->ext->glTextureImage2DEXT(This->id, GL_TEXTURE_2D, level, This->internalformats[0],
				width, height, 0, This->format, This->type, data);
			else This->renderer->ext->glTextureSubImage2DEXT(This->id, GL_TEXTURE_2D, level,
				0, 0, width, height, This->format, This->type, data);
		}
		else
		{

		}
		{
			glUtil_SetActiveTexture(util, 0);
			glUtil_SetTexture(util, 0, This);
			if (dorealloc)glTexImage2D(GL_TEXTURE_2D, level, This->internalformats[0], width, height, 0, This->format, This->type, data);
			else glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, This->format, This->type, data);
		}
	}
	This->levels[level].dirty &= ~1;
}

void glTexture__Upload(glTexture *This, GLint level)
{
	int bpp = This->levels[level].ddsd.ddpfPixelFormat.dwRGBBitCount;
	int x = This->levels[level].ddsd.dwWidth;
	int y = This->levels[level].ddsd.dwHeight;
	int bigpitch = NextMultipleOf4((bpp / 8)*This->bigwidth);
	int pitch = This->levels[level].ddsd.lPitch;
	int bigx, bigy;
	if (level)
	{
		bigx = This->levels[level].ddsd.dwWidth;
		bigy = This->levels[level].ddsd.dwHeight;
	}
	else
	{
		bigx = This->bigwidth;
		bigy = This->bigheight;
	}
	if (bpp == 15) bpp = 16;
	if ((x == bigx && y == bigy) || !This->levels[level].bigbuffer)
	{
		glTexture__Upload2(This,level,
			This->levels[level].ddsd.dwWidth, This->levels[level].ddsd.dwHeight, FALSE, FALSE, This->renderer->util);
	}
	else
	{
		switch (bpp)
		{
		case 8:
			ScaleNearest8(This->levels[level].bigbuffer, This->levels[level].buffer,
				bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 16:
			ScaleNearest16(This->levels[level].bigbuffer, This->levels[level].buffer,
				bigx, bigy, x, y, pitch / 2, bigpitch / 2);
			break;
		case 24:
			ScaleNearest24(This->levels[level].bigbuffer, This->levels[level].buffer,
				bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 32:
			ScaleNearest32(This->levels[level].bigbuffer, This->levels[level].buffer,
				bigx, bigy, x, y, pitch / 4, bigpitch / 4);
			break;
		}
		glTexture__Upload2(This, level,
			bigx, bigy, FALSE, FALSE, This->renderer->util);
	}
}
BOOL glTexture__Repair(glTexture *This, BOOL preserve)
{
	// data should be null to create uninitialized texture or be pointer to top-level
	// buffer to retain texture data
	GLenum error;
	BOOL repairfail = false;
	int i;
	if (This->internalformats[1] == 0) return FALSE;
	glUtil_SetActiveTexture(This->renderer->util, 0);
	if (preserve)
	{
		for (i = 0; i < This->miplevel; i++)
			glTexture__Download(This, i);
	}
	GLvoid *data;
	if (preserve) data = This->levels[0].bigbuffer ? This->levels[0].bigbuffer : This->levels[0].buffer;
	glUtil_SetTexture(This->renderer->util, 0, This);
	for (i = 0; i < This->miplevel; i++)
	{
		do
		{
			memmove(&This->internalformats[0], &This->internalformats[1], 7 * sizeof(GLint));
			This->internalformats[7] = 0;
			ClearError();
			if ((This->levels[0].ddsd.dwWidth != This->bigwidth) || (This->levels[0].ddsd.dwHeight != This->bigheight))
				glTexImage2D(GL_TEXTURE_2D, i, This->internalformats[0], This->bigwidth,
					This->bigheight, 0, This->format, This->type, This->levels[i].bigbuffer);
			else glTexImage2D(GL_TEXTURE_2D, i, This->internalformats[0], This->levels[i].ddsd.dwWidth,
				This->levels[i].ddsd.dwHeight, 0, This->format, This->type, This->levels[i].buffer);
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (This->internalformats[1] == 0)
				{
					FIXME("Failed to repair texture, cannot find internal format");
					repairfail = TRUE;
					break;
				}
			}
			else break;
		} while (1);
		if (repairfail) break;
	}
	if (repairfail) return FALSE;
	return TRUE;
}

void glTexture__SetPrimaryScale(glTexture *This, GLint bigwidth, GLint bigheight, BOOL scaling)
{
	if (This->miplevel > 1) return;  // Function is not for mipmapped texture, primary and attachment only
	if (This->levels[0].dirty & 2) glTexture__Download(This, 0);
	if (This->levels[0].bigbuffer)
	{
		free(This->levels[0].bigbuffer);
		This->levels[0].bigbuffer = NULL;
	}
	if (scaling)
	{

		This->bigwidth = bigwidth;
		This->bigheight = bigheight;
		This->levels[0].bigbuffer = (char *)malloc(NextMultipleOf4((This->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount *
			This->bigwidth) / 8) * This->bigheight);
		glTexture__Upload2(This, 0, bigwidth, bigheight, FALSE, TRUE, This->renderer->util);
	}
	else
	{
		This->bigwidth = This->levels[0].ddsd.dwWidth;
		This->bigheight = This->levels[0].ddsd.dwHeight;
		glTexture__Upload2(This, 0, This->levels[0].ddsd.dwWidth, This->levels[0].ddsd.dwHeight,
			FALSE, TRUE, This->renderer->util);
	}
}

void glTexture__FinishCreate(glTexture *This)
{
	int texformat = -1;
	int i;
	DWORD x, y;
	GLenum error;
	numtexformats = END_TEXFORMATS - START_TEXFORMATS;
	for (i = 0; i < numtexformats; i++)
	{
		if (!memcmp(&texformats[i], &This->levels[0].ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT)))
		{
			texformat = i;
			break;
		}
	}
	ZeroMemory(This->internalformats, 8 * sizeof(GLint));
	switch (texformat)
	{
	case -1:
	case 0: // 8-bit palette
		if (This->renderer->ext->glver_major >= 3)
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
		//This->useconv = TRUE;
		//This->convfunction = 0;
		FIXME("Unusual texture format RGBA8332 not supported");
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
	glGenTextures(1, &This->id);
	glUtil_SetTexture(This->renderer->util, 0, This);
	if ((This->levels[0].ddsd.dwFlags & DDSD_CAPS) && (This->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
		This->minfilter = This->magfilter = GL_NEAREST;
	else
	{
		if (This->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			if (dxglcfg.scalingfilter && (glRenderer_GetBPP(This->renderer) > 8))
				This->minfilter = This->magfilter = GL_LINEAR;
			else This->minfilter = This->magfilter = GL_NEAREST;
		}
		else
		{
			if (dxglcfg.BltScale && (glRenderer_GetBPP(This->renderer) > 8))
				This->minfilter = This->magfilter = GL_LINEAR;
			else This->minfilter = This->magfilter = GL_NEAREST;
		}
	}
	This->wraps = This->wrapt = GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, This->minfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, This->magfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, This->wraps);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, This->wrapt);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->miplevel - 1);
	if ((This->levels[0].ddsd.dwWidth != This->bigwidth) || (This->levels[0].ddsd.dwHeight != This->bigheight))
	{
		x = This->bigwidth;
		y = This->bigheight;
	}
	else
	{
		x = This->levels[0].ddsd.dwWidth;
		y = This->levels[0].ddsd.dwHeight;
	}
	for (i = 0; i < This->miplevel; i++)
	{
		do
		{
			ClearError();
			glTexImage2D(GL_TEXTURE_2D, i, This->internalformats[0], x, y, 0, This->format, This->type, NULL);
			This->levels[i].dirty |= 2;
			ShrinkMip(&x, &y);
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
		This->levels[i].buffer = (char*)malloc(NextMultipleOf4((This->levels[i].ddsd.ddpfPixelFormat.dwRGBBitCount *
			This->levels[i].ddsd.dwWidth) / 8) * This->levels[i].ddsd.dwHeight);
		if ((i == 0) && ((This->levels[i].ddsd.dwWidth != This->bigwidth) || (This->levels[i].ddsd.dwHeight != This->bigheight)))
			This->levels[i].bigbuffer = (char *)malloc(NextMultipleOf4((This->levels[i].ddsd.ddpfPixelFormat.dwRGBBitCount *
				This->bigwidth) / 8) * This->bigheight);
	}
}
void glTexture__Destroy(glTexture *This)
{
	glRenderer__RemoveTextureFromD3D(This->renderer, This);
	glDeleteTextures(1, &This->id);
	free(This);
}

void glTexture__SetFilter(glTexture *This, int level, GLint mag, GLint min, glRenderer *renderer)
{
	// 'This' pointer may be set to NULL to set a sampler level when ARB_sampler_objects is available
	switch (dxglcfg.texfilter)
	{
	default:
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
	if (renderer->ext->GLEXT_ARB_sampler_objects)
	{
		renderer->ext->glSamplerParameteri(renderer->util->samplers[level].id, GL_TEXTURE_MAG_FILTER, mag);
		renderer->ext->glSamplerParameteri(renderer->util->samplers[level].id, GL_TEXTURE_MIN_FILTER, min);
	}
	else
	{
		if (This)
		{
			if (renderer->ext->GLEXT_ARB_direct_state_access)
			{
				renderer->ext->glTextureParameteri(This->id, GL_TEXTURE_MAG_FILTER, mag);
				renderer->ext->glTextureParameteri(This->id, GL_TEXTURE_MIN_FILTER, min);
			}
			else if (renderer->ext->GLEXT_EXT_direct_state_access)
			{
				renderer->ext->glTextureParameteriEXT(This->id, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
				renderer->ext->glTextureParameteriEXT(This->id, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
			}
			else
			{
				glUtil_SetTexture(renderer->util, level, This);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
			}
			This->magfilter = mag;
			This->minfilter = min;
		}
		else
		{
			glUtil_SetTexture(renderer->util, level, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		}
	}
}

}