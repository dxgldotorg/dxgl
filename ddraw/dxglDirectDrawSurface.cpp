// DXGL
// Copyright (C) 2011-2022 William Feely

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
#include "scalers.h"
#include "ddraw.h"
#include "glTexture.h"
#include "glUtil.h"
#include "util.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "dxglDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirectDrawPalette.h"
#include "glDirectDrawClipper.h"
#include "glDirectDrawGammaControl.h"
#include "glRenderer.h"
#include <string>
using namespace std;
#include "ShaderGen3D.h"
#include <math.h>

float primaryscalex;
float primaryscaley;

dxglDirectDrawSurface7Vtbl dxglDirectDrawSurface7_impl =
{
	dxglDirectDrawSurface7_QueryInterface,
	dxglDirectDrawSurface7_AddRef,
	dxglDirectDrawSurface7_Release,
	dxglDirectDrawSurface7_AddAttachedSurface,
	dxglDirectDrawSurface7_AddOverlayDirtyRect,
	dxglDirectDrawSurface7_Blt,
	dxglDirectDrawSurface7_BltBatch,
	dxglDirectDrawSurface7_BltFast,
	dxglDirectDrawSurface7_DeleteAttachedSurface,
	dxglDirectDrawSurface7_EnumAttachedSurfaces,
	dxglDirectDrawSurface7_EnumOverlayZOrders,
	dxglDirectDrawSurface7_Flip,
	dxglDirectDrawSurface7_GetAttachedSurface,
	dxglDirectDrawSurface7_GetBltStatus,
	dxglDirectDrawSurface7_GetCaps,
	dxglDirectDrawSurface7_GetClipper,
	dxglDirectDrawSurface7_GetColorKey,
	dxglDirectDrawSurface7_GetDC,
	dxglDirectDrawSurface7_GetFlipStatus,
	dxglDirectDrawSurface7_GetOverlayPosition,
	dxglDirectDrawSurface7_GetPalette,
	dxglDirectDrawSurface7_GetPixelFormat,
	dxglDirectDrawSurface7_GetSurfaceDesc,
	dxglDirectDrawSurface7_Initialize,
	dxglDirectDrawSurface7_IsLost,
	dxglDirectDrawSurface7_Lock,
	dxglDirectDrawSurface7_ReleaseDC,
	dxglDirectDrawSurface7_Restore,
	dxglDirectDrawSurface7_SetClipper,
	dxglDirectDrawSurface7_SetColorKey,
	dxglDirectDrawSurface7_SetOverlayPosition,
	dxglDirectDrawSurface7_SetPalette,
	dxglDirectDrawSurface7_Unlock,
	dxglDirectDrawSurface7_UpdateOverlay,
	dxglDirectDrawSurface7_UpdateOverlayDisplay,
	dxglDirectDrawSurface7_UpdateOverlayZOrder,
	dxglDirectDrawSurface7_GetDDInterface,
	dxglDirectDrawSurface7_PageLock,
	dxglDirectDrawSurface7_PageUnlock,
	dxglDirectDrawSurface7_SetSurfaceDesc,
	dxglDirectDrawSurface7_SetPrivateData,
	dxglDirectDrawSurface7_GetPrivateData,
	dxglDirectDrawSurface7_FreePrivateData,
	dxglDirectDrawSurface7_GetUniquenessValue,
	dxglDirectDrawSurface7_ChangeUniquenessValue,
	dxglDirectDrawSurface7_SetPriority,
	dxglDirectDrawSurface7_GetPriority,
	dxglDirectDrawSurface7_SetLOD,
	dxglDirectDrawSurface7_GetLOD
};

void ShrinkMip(DWORD *x, DWORD *y, int level)
{
	int i;
	for (i = 0; i < level; i++)
	{
		*x = max(1, (DWORD)floorf((float)*x / 2.0f));
		*y = max(1, (DWORD)floorf((float)*y / 2.0f));
	}
}

// DDRAW7 routines
HRESULT dxglDirectDrawSurface7_Create(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, glDirectDrawPalette *palettein,
	glTexture *parenttex, int version, dxglDirectDrawSurface7 *glDDS7)
{
	HRESULT error;
	DDSURFACEDESC2 ddsdBigSurface;
	BOOL makeBigSurface = FALSE;
	DWORD buffercount;
	DWORD mipcount;
	DWORD complexcount;
	dxglDirectDrawSurface7 *surfaceptr;
	dxglDirectDrawSurface7 *bigsurface;
	glTexture *textureptr;
	TRACE_ENTER(9,14,lpDD7,14,lpDDSurfaceDesc2,14,palettein,14,parenttex,11,version,14,glDDS7);
	ZeroMemory(glDDS7, sizeof(dxglDirectDrawSurface7));
	glDDS7->lpVtbl = &dxglDirectDrawSurface7_impl;
	glDDS7->version = version;
	dxglDirectDrawSurface1_Create(glDDS7, &glDDS7->dds1);
	dxglDirectDrawSurface2_Create(glDDS7, &glDDS7->dds2);
	dxglDirectDrawSurface3_Create(glDDS7, &glDDS7->dds3);
	dxglDirectDrawSurface4_Create(glDDS7, &glDDS7->dds4);
	glDirect3DTexture2_Create(glDDS7, &glDDS7->d3dt2);
	glDirect3DTexture1_Create(glDDS7, &glDDS7->d3dt1);
	glDirectDrawGammaControl_Create(glDDS7, &glDDS7->gammacontrol);
	glDDS7->ddInterface = (glDirectDraw7 *)lpDD7;
	glDDS7->ddsd = *lpDDSurfaceDesc2;
	LONG sizes[6];
	int i, x, y;
	float xscale, yscale;
	DWORD winver, winvermajor, winverminor;
	glDirectDraw7_GetSizes(glDDS7->ddInterface, sizes);

	// Check if requested surface is Primary
	if (glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		// Width, height, and pixel format forbidden for primary surfaces.
		if (((glDDS7->ddsd.dwFlags & DDSD_WIDTH) || (glDDS7->ddsd.dwFlags & DDSD_HEIGHT)
			|| (glDDS7->ddsd.dwFlags & DDSD_PIXELFORMAT)) && !(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);

		// Check for incompatible surface types
		if ((glDDS7->ddsd.dwFlags & DDSD_CAPS))
		{
			if ((glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) ||
				(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) ||
				(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) ||
				(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE) ||
				(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER) ||
				(glDDS7->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
				TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		}
		
		// Check fullscreen
		if (glDirectDraw7_GetFullscreen(glDDS7->ddInterface))
		{
			glDDS7->ddsd.dwWidth = sizes[2];
			glDDS7->ddsd.dwHeight = sizes[3];
			memcpy(&ddsdBigSurface, &glDDS7->ddsd, sizeof(DDSURFACEDESC2));
			ddsdBigSurface.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
			if (dxglcfg.primaryscale)
			{
				if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
					(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
				{
					if (glDDS7->ddsd.dwWidth <= 400) xscale = 2.0f;
					else xscale = 1.0f;
					if (glDDS7->ddsd.dwHeight <= 300) yscale = 2.0f;
					else yscale = 1.0f;
				}
				else
				{
					xscale = dxglcfg.postsizex;
					yscale = dxglcfg.postsizey;
				}
				primaryscalex = 1.0f;
				primaryscaley = 1.0f;
				switch (dxglcfg.primaryscale)
				{
				case 1: // Scale to window size
				default:
					ddsdBigSurface.dwWidth = (DWORD)((float)sizes[0] / xscale);
					ddsdBigSurface.dwHeight = (DWORD)((float)sizes[1] / yscale);
					primaryscalex = (float)ddsdBigSurface.dwWidth / (float)glDDS7->ddsd.dwWidth;
					primaryscaley = (float)ddsdBigSurface.dwHeight / (float)glDDS7->ddsd.dwHeight;
					break;
				case 2: // Scale to integer auto
					for (i = 1; i < 100; i++)
					{
						if ((glDDS7->ddsd.dwWidth * i) > (DWORD)((float)sizes[0] / xscale))
						{
							ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * i;
							break;
						}
					}
					for (i = 1; i < 100; i++)
					{
						if ((glDDS7->ddsd.dwHeight * i) > (DWORD)((float)sizes[1] / yscale))
						{
							ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * i;
							break;
						}
					}
					primaryscalex = (float)(ddsdBigSurface.dwWidth / glDDS7->ddsd.dwWidth);
					primaryscaley = (float)(ddsdBigSurface.dwHeight / glDDS7->ddsd.dwHeight);
					break;
				case 3: // 1.5x scale
					ddsdBigSurface.dwWidth = (DWORD)((float)glDDS7->ddsd.dwWidth * 1.5f);
					ddsdBigSurface.dwHeight = (DWORD)((float)glDDS7->ddsd.dwHeight * 1.5f);
					primaryscalex = 1.5f;
					primaryscaley = 1.5f;
					break;
				case 4: // 2x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 2;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 2;
					primaryscalex = 2.0f;
					primaryscaley = 2.0f;
					break;
				case 5: // 2.5x scale
					ddsdBigSurface.dwWidth = (DWORD)((float)glDDS7->ddsd.dwWidth * 2.5f);
					ddsdBigSurface.dwHeight = (DWORD)((float)glDDS7->ddsd.dwHeight * 2.5f);
					primaryscalex = 2.5f;
					primaryscaley = 2.5f;
					break;
				case 6: // 3x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 3;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 3;
					primaryscalex = 3.0f;
					primaryscaley = 3.0f;
					break;
				case 7: // 4x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 4;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 4;
					primaryscalex = 4.0f;
					primaryscaley = 4.0f;
					break;
				case 8: // 5x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 5;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 5;
					primaryscalex = 5.0f;
					primaryscaley = 5.0f;
					break;
				case 9: // 6x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 6;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 6;
					primaryscalex = 6.0f;
					primaryscaley = 6.0f;
					break;
				case 10: // 7x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 7;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 7;
					primaryscalex = 7.0f;
					primaryscaley = 7.0f;
					break;
				case 11: // 8x scale
					ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth * 8;
					ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight * 8;
					primaryscalex = 8.0f;
					primaryscaley = 8.0f;
					break;
				case 12: // Custom scale
					ddsdBigSurface.dwWidth = (DWORD)((float)glDDS7->ddsd.dwWidth * dxglcfg.primaryscalex);
					ddsdBigSurface.dwHeight = (DWORD)((float)glDDS7->ddsd.dwHeight * dxglcfg.primaryscaley);
					primaryscalex = dxglcfg.primaryscalex;
					primaryscaley = dxglcfg.primaryscaley;
					break;
				}
			}
			else
			{
				ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth;
				ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight;
			}
			glDDS7->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			ddsdBigSurface.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
		else
		{
			winver = GetVersion();
			winvermajor = (DWORD)(LOBYTE(LOWORD(winver)));
			winverminor = (DWORD)(HIBYTE(LOWORD(winver)));
			if ((winvermajor > 4) || ((winvermajor == 4) && (winverminor >= 1)))
			{
				ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
				ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
				glDDS7->ddInterface->renderer->xoffset = GetSystemMetrics(SM_XVIRTUALSCREEN);
				glDDS7->ddInterface->renderer->yoffset = GetSystemMetrics(SM_YVIRTUALSCREEN);
			}
			else
			{
				ddsdBigSurface.dwWidth = glDDS7->ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
				ddsdBigSurface.dwHeight = glDDS7->ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
			}
			glDDS7->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
	}
	else if (!((glDDS7->ddsd.dwFlags & DDSD_WIDTH) && (glDDS7->ddsd.dwFlags & DDSD_HEIGHT)))
	TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);

	// Not yet implemented: System memory surface
	/*	if(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	{
		BITMAPINFO info;
		ZeroMemory(&info,sizeof(BITMAPINFO));
		if(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			info.bmiHeader.biWidth = glDDS7->fakex;
			info.bmiHeader.biHeight = -(signed)glDDS7->fakey;
			info.bmiHeader.biCompression = BI_RGB;
			info.bmiHeader.biSizeImage = 0;
			info.bmiHeader.biXPelsPerMeter = 0;
			info.bmiHeader.biYPelsPerMeter = 0;
			info.bmiHeader.biClrImportant = 0;
			info.bmiHeader.biClrUsed = 0;
			info.bmiHeader.biBitCount = (WORD)glDirectDraw7_GetBPPMultipleOf8(glDDS7->ddInterface);
			*glDDS7->bitmapinfo = info;
		}
		else
		{
			if(glDDS7->ddsd.dwFlags & DDSD_PIXELFORMAT) glDDS7->surfacetype=2;
			else
			{
				info.bmiHeader.biWidth = glDDS7->fakex;
				info.bmiHeader.biHeight = -(signed)glDDS7->fakey;
				info.bmiHeader.biCompression = BI_RGB;
				info.bmiHeader.biSizeImage = 0;
				info.bmiHeader.biXPelsPerMeter = 0;
				info.bmiHeader.biYPelsPerMeter = 0;
				info.bmiHeader.biClrImportant = 0;
				info.bmiHeader.biClrUsed = 0;
				info.bmiHeader.biBitCount = (WORD)glDirectDraw7_GetBPPMultipleOf8(glDDS7->ddInterface);
				*glDDS7->bitmapinfo = info;
			}
		}
	}
	else
	{
		glDDS7->bitmapinfo->bmiHeader.biSizeImage = 0;
		glDDS7->bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
		glDDS7->bitmapinfo->bmiHeader.biYPelsPerMeter = 0;
		glDDS7->bitmapinfo->bmiHeader.biClrImportant = 0;
		glDDS7->bitmapinfo->bmiHeader.biClrUsed = 0;
		glDDS7->bitmapinfo->bmiHeader.biCompression = BI_RGB;
		glDDS7->bitmapinfo->bmiHeader.biBitCount = 0;
	}
	glDDS7->surfacetype=2;
	glDDS7->bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	glDDS7->bitmapinfo->bmiHeader.biWidth = glDDS7->ddsd.dwWidth;
	glDDS7->bitmapinfo->bmiHeader.biHeight = -(signed)glDDS7->ddsd.dwHeight;
	glDDS7->bitmapinfo->bmiHeader.biPlanes = 1; */

	// Calculate mipmap levels
	if ((glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && !(glDDS7->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if (!(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (!(glDDS7->ddsd.dwFlags & DDSD_MIPMAPCOUNT))
		{
			glDDS7->ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
			glDDS7->ddsd.dwMipMapCount = CalculateMipLevels(glDDS7->ddsd.dwWidth, glDDS7->ddsd.dwHeight);
		}
	}

	// Calculate complex surface count
	if (glDDS7->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) buffercount = glDDS7->ddsd.dwBackBufferCount + 1;
	else buffercount = 1;
	if (glDDS7->ddsd.dwFlags & DDSD_MIPMAPCOUNT) complexcount = buffercount * glDDS7->ddsd.dwMipMapCount;
	else complexcount = buffercount;

	// Get start of texture structures
	textureptr = (glTexture*)&glDDS7[complexcount];

	// Base surface is top of miplevel
	glDDS7->miplevel = 0;

	// Calculate mipmap count
	if (glDDS7->ddsd.dwFlags & DDSD_MIPMAPCOUNT) mipcount = glDDS7->ddsd.dwMipMapCount;
	else mipcount = 1;

	// Fill out complex surface structure
	// Pre-populate child surfaces
	for (i = 1; i < complexcount; i++)
		memcpy(&glDDS7[i], glDDS7, sizeof(dxglDirectDrawSurface7));
	// Populate unique child surface parameters
	for (y = 0; y < buffercount; y++)
	{
		for (x = 0; x < mipcount; x++)
		{
			surfaceptr = &glDDS7[x + (y * mipcount)];
			// Set mip level
			surfaceptr->miplevel = x;
			// Set flip index
			surfaceptr->flipindex = y;
			if (x > 0)  // Mipmap sublevels
			{
				ShrinkMip(&surfaceptr->ddsd.dwWidth, &surfaceptr->ddsd.dwHeight, x);
				surfaceptr->ddsd.dwMipMapCount -= x;
				surfaceptr->ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
			}
			// Set parameters for flip surfaces
			switch (y)
			{
			case 0:
				if(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
					surfaceptr->ddsd.ddsCaps.dwCaps |= DDSCAPS_VISIBLE; // Visible surface
				if ((buffercount > 1) && (x == 0)) surfaceptr->ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
				break;
			case 1:
				surfaceptr->ddsd.dwBackBufferCount = 0; // Attached surfaces do not show backbuffer count
				surfaceptr->ddsd.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
				surfaceptr->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
				surfaceptr->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				surfaceptr->ddsd.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;  // First flip is a backbuffer
				break;
			default:
				surfaceptr->ddsd.dwBackBufferCount = 0; // Attached surfaces do not show backbuffer count
				surfaceptr->ddsd.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
				surfaceptr->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_PRIMARYSURFACE;
				surfaceptr->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				break;
			}
			// Set parent to top of complex structure
			surfaceptr->parent = glDDS7;
			// Set texture pointer to buffer's storage
			surfaceptr->texture = &textureptr[y];
			// Initialize refrence count
			surfaceptr->refcount7 = 1;
			// Set parent on legacy interfaces
			surfaceptr->dds1.glDDS7 = surfaceptr;
			surfaceptr->dds2.glDDS7 = surfaceptr;
			surfaceptr->dds3.glDDS7 = surfaceptr;
			surfaceptr->dds4.glDDS7 = surfaceptr;
			surfaceptr->d3dt2.glDDS7 = surfaceptr;
			surfaceptr->d3dt1.glDDS7 = surfaceptr;
		}
	}

	/* // Is glDDS7 a mipmap sublevel?  Commented out due to new surface format
	if (glDDS7->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
	{
		if (!glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE)
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		glDDS7->texture = parenttex;
		glTexture_AddRef(glDDS7->texture);
	}
	else
	{*/
		for (i = 0; i < buffercount; i++)
		{
			surfaceptr = &glDDS7[i * mipcount];
			error = glTexture_Create(&surfaceptr->ddsd, surfaceptr->texture, surfaceptr->ddInterface->renderer, /*surfaceptr->hasstencil,*/ FALSE, 0);
			if (error != DD_OK)
			{
				while (i > 0)
				{
					// Delete already created textures in case of failure
					i--;
					glRenderer_DeleteTexture(glDDS7->ddInterface->renderer,&textureptr[i]);
				}
				TRACE_EXIT(23, error);
				return error;
			}
			// Set primary capability for DrawScreen
			if (glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
				surfaceptr->texture->levels[0].ddsd.ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE;
			surfaceptr->texture->freeonrelease = FALSE;
		}
	//}
	for (i = 0; i < complexcount; i++)
	{
		// Set pitch of surface
		if (!(glDDS7[i].ddsd.dwFlags & DDSD_PITCH))
		{
			glDDS7[i].ddsd.dwFlags |= DDSD_PITCH;
			glDDS7[i].ddsd.lPitch = glDDS7[i].texture->levels[glDDS7[i].miplevel].ddsd.lPitch;
		}
		// Set pixel format
		if (!(glDDS7[i].ddsd.dwFlags & DDSD_PIXELFORMAT))
		{
			glDDS7[i].ddsd.dwFlags |= DDSD_PIXELFORMAT;
			memcpy(&glDDS7[i].ddsd.ddpfPixelFormat, &glDDS7[i].texture->levels[glDDS7[i].miplevel].ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
		}
		if (!glTexture_ValidatePixelFormat(&glDDS7[i].ddsd.ddpfPixelFormat))
		{
			// Clean up textures
			for (i = 0; i < complexcount; i++)
				glRenderer_DeleteTexture(glDDS7->ddInterface->renderer, &textureptr[i]);
			TRACE_EXIT(23, DDERR_INVALIDPIXELFORMAT);
			return DDERR_INVALIDPIXELFORMAT;
		}
	}
	// Create/set primary palette
	if (glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (glDDS7->ddInterface->primarybpp == 8)
		{
			if (!palettein)
			{
				glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&glDDS7->palette);
				dxglDirectDrawSurface7_SetPaletteNoDraw(glDDS7, (LPDIRECTDRAWPALETTE)glDDS7->palette);
			}
			else
			{
				dxglDirectDrawSurface7_SetPaletteNoDraw(glDDS7, (LPDIRECTDRAWPALETTE)palettein);
			}
		}
		// Create big version texture
		if (dxglcfg.primaryscale)
		{
			error = glDirectDraw7_CreateSurface2(glDDS7->ddInterface, &ddsdBigSurface,
				(LPDIRECTDRAWSURFACE7*)&bigsurface, NULL, FALSE, 7);
			if (SUCCEEDED(error))
			{
				if ((glDDS7->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && glDDS7->ddsd.dwBackBufferCount)
				{
					for (y = 0; y < buffercount; y++)
					{
						bigsurface[y * mipcount].texture->bigparent = glDDS7[y * mipcount].texture;
						glDDS7[y * mipcount].texture->bigtexture = bigsurface[y * mipcount].texture;
						for (x = 0; x < mipcount; x++)
						{
							glDDS7[x + (y * mipcount)].bigsurface = &bigsurface[x + (y * mipcount)];
							bigsurface[x + (y * mipcount)].bigparent = &glDDS7[x + (y * mipcount)];
						}
					}
				}
				else
				{
					glDDS7->bigsurface = bigsurface;
					bigsurface->bigparent = glDDS7;
					bigsurface->texture->bigparent = glDDS7->texture;
					glDDS7->texture->bigtexture = bigsurface->texture;
				}
			}
		}
	}

	/* No longer used for new format
	if ((glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && glDDS7->ddsd.dwMipMapCount)
	{
		DDSURFACEDESC2 newdesc = glDDS7->ddsd;
		newdesc.dwWidth = max(1, (DWORD)floorf((float)glDDS7->ddsd.dwWidth / 2.0f));
		newdesc.dwHeight = max(1, (DWORD)floorf((float)glDDS7->ddsd.dwHeight / 2.0f));
		newdesc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
		newdesc.dwMipMapCount = glDDS7->ddsd.dwMipMapCount - 1;
		HRESULT miperror;
		if(newdesc.dwMipMapCount)
			dxglDirectDrawSurface7_Create(lpDD7, &newdesc, &miperror, glDDS7->palette,
				glDDS7->texture, miplevel + 1, version, NULL,&glDDS7->miptexture);
	}*/

/*	if(glDDS7->ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
	{
		glDDS7->colormasks[0] = glDDS7->ddsd.ddpfPixelFormat.dwRBitMask;
		glDDS7->colormasks[1] = glDDS7->ddsd.ddpfPixelFormat.dwGBitMask;
		glDDS7->colormasks[2] = glDDS7->ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(glDDS7->bitmapinfo->bmiColors,glDDS7->colormasks,3*sizeof(DWORD));
	}
	if(!glDDS7->bitmapinfo->bmiHeader.biBitCount)
		glDDS7->bitmapinfo->bmiHeader.biBitCount = (WORD)glDDS7->ddsd.ddpfPixelFormat.dwRGBBitCount;*/
	glDDS7->refcount7 = 1;
	glDDS7->refcount4 = 0;
	glDDS7->refcount3 = 0;
	glDDS7->refcount2 = 0;
	glDDS7->refcount1 = 0;
	glDDS7->refcountgamma = 0;
	glDDS7->refcountcolor = 0;
	//glDDS7->mulx = (float)glDDS7->fakex / (float)glDDS7->ddsd.dwWidth;
	//glDDS7->muly = (float)glDDS7->fakey / (float)glDDS7->ddsd.dwHeight;
	/* No longer needed with new format
	glDDS7->backbuffer = NULL;
	if(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
	{
		if(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
		{
			if((glDDS7->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && (glDDS7->ddsd.dwBackBufferCount > 0))
			{
				if(!(glDDS7->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
					glDDS7->ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
				DDSURFACEDESC2 ddsdBack;
				memcpy(&ddsdBack,&glDDS7->ddsd, glDDS7->ddsd.dwSize);
				ddsdBack.dwBackBufferCount--;
				ddsdBack.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
				ddsdBack.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				dxglDirectDrawSurface7_Create((LPDIRECTDRAW7)glDDS7->ddInterface, &ddsdBack, error,
					glDDS7->palette, parenttex, miplevel, version, front ? front : glDDS7, &glDDS7->backbuffer);
			}
			else if (glDDS7->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
			{
				glDDS7->backbufferwraparound = front;
			}
			else *error = DDERR_INVALIDPARAMS;
		}
	}*/

	// Set parent interface for texture interfaces
	switch (version)
	{
	case 1:
	case 2:
	case 3:
	default:
		glDDS7->textureparent = (IUnknown*)&glDDS7->dds1;
		break;
	case 4:
		glDDS7->textureparent = (IUnknown*)&glDDS7->dds4;
		break;
	case 7:
		glDDS7->textureparent = (IUnknown*)glDDS7;
		break;
	}
	TRACE_EXIT(23,error);
	return error;
}

void dxglDirectDrawSurface7_Delete(dxglDirectDrawSurface7 *This)
{
	int i;
	TRACE_ENTER(1, 14, This);
	dxglDirectDrawSurface7_AddRef(This);
	if (This->overlaydest) dxglDirectDrawSurface7_DeleteOverlay(This->overlaydest, This);
	if (This->overlays)
	{
		for (i = 0; i < This->overlaycount; i++)
			glTexture_Release(This->overlays[i].texture, FALSE);
		free(This->overlays);
		dxglDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, FALSE, NULL, -1);
	}
	if (This->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		for (i = 0; i <= This->ddsd.dwBackBufferCount; i++)
		{
			glTexture_Release(This[i].texture, FALSE);
		}
	}
	else
	{
		if (This->texture) glTexture_Release(This->texture, FALSE);
	}
	//if(This->bitmapinfo) free(This->bitmapinfo);
	if (This->palette)
	{
		if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			This->palette->surface = NULL;
			This->palette->timer = NULL;
		}
		glDirectDrawPalette_Release(This->palette);
	}
	//if(This->backbuffer) dxglDirectDrawSurface7_Release(This->backbuffer);
	if(This->clipper) glDirectDrawClipper_Release(This->clipper);
	//if(This->buffer) free(This->buffer);
	//if(This->bigbuffer) free(This->bigbuffer);
	if (This->zbuffer)
	{
		if (This->zbuffer->attachparent == This) This->zbuffer->attachparent = NULL;
		if (This->zbuffer->attachcount) This->zbuffer->attachcount--;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = NULL;
		This->zbuffer_iface->Release();
	}
	if (This->bigsurface) dxglDirectDrawSurface7_Release(This->bigsurface);
	//if(This->miptexture) dxglDirectDrawSurface7_Release(This->miptexture);
	if (This->device) glDirect3DDevice7_Release(This->device); 
	if (This->device1) glDirect3DDevice7_Destroy(This->device1);
	glDirectDraw7_DeleteSurface(This->ddInterface, This);
	if (This->creator) This->creator->Release();
	glRenderer_FreePointer(This->ddInterface->renderer, This);
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI dxglDirectDrawSurface7_QueryInterface(dxglDirectDrawSurface7 *This, REFIID riid, void** ppvObj)
{
	HRESULT ret;
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDrawSurface7))
	{
		dxglDirectDrawSurface7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface4)
	{
		dxglDirectDrawSurface7_AddRef4(This);
		*ppvObj = &This->dds4;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		dxglDirectDrawSurface7_AddRef3(This);
		*ppvObj = &This->dds3;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		dxglDirectDrawSurface7_AddRef2(This);
		*ppvObj = &This->dds2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface)
	{
		dxglDirectDrawSurface7_AddRef1(This);
		*ppvObj = &This->dds1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture2)
	{
		glDirect3DTexture2_AddRef(&This->d3dt2);
		*ppvObj = &This->d3dt2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture)
	{
		glDirect3DTexture1_AddRef(&This->d3dt1);
		*ppvObj = &This->d3dt1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if (riid == IID_IDirectDrawGammaControl)
	{
		dxglDirectDrawSurface7_AddRefGamma(This);
		*ppvObj = &This->gammacontrol;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if (riid == IID_IDirectDrawColorControl)
	{
		DEBUG("FIXME:  Add color control\n");
		TRACE_EXIT(23, E_NOINTERFACE);
		return E_NOINTERFACE;
	}
	if ((riid == IID_IDirect3DHALDevice) || (riid == IID_IDirect3DRGBDevice) ||
		(riid == IID_IDirect3DRampDevice) || (riid == IID_IDirect3DRefDevice))
	{

		if(!This->device1)
		{
			glDirect3D7 *tmpd3d;
			glDirectDraw7_QueryInterface(This->ddInterface,IID_IDirect3D7,(void**)&tmpd3d);
			if(!tmpd3d) TRACE_RET(HRESULT,23,E_NOINTERFACE);
			ret = glDirect3DDevice7_Create(riid, tmpd3d, This, (IUnknown*)&This->dds1, 1, &This->device1);
			if (FAILED(ret))
			{
				if(This->device1) glDirect3DDevice7_Destroy(This->device1);
				glDirect3D7_Release(tmpd3d);
				TRACE_EXIT(23, ret);
				return ret;
			}
			*ppvObj = This->device1->glD3DDev1;
			glDirect3DDevice1_AddRef(This->device1->glD3DDev1);
			glDirect3DDevice7_InitDX5(This->device1);
			glDirect3DDevice7_InitDX2(This->device1);
			glDirect3D7_Release(tmpd3d);
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else
		{
			*ppvObj = This->device1->glD3DDev1;
			glDirect3DDevice1_AddRef(This->device1->glD3DDev1);
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
	}
	if (This->version == 7)
	{
		TRACE_EXIT(23, E_NOINTERFACE);
		return E_NOINTERFACE;
	}
	else
	{
		TRACE_EXIT(23, DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
}
ULONG WINAPI dxglDirectDrawSurface7_AddRef(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount7);
	TRACE_EXIT(8,This->refcount7);
	return This->refcount7;
}
ULONG WINAPI dxglDirectDrawSurface7_Release(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	if (This->refcount7 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount7);
	ret = This->refcount7;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8,ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRef4(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount4);
	TRACE_EXIT(8, This->refcount4);
	return This->refcount4;
}
ULONG WINAPI dxglDirectDrawSurface7_Release4(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount4 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount4);
	ret = This->refcount4;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRef3(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount3);
	TRACE_EXIT(8, This->refcount3);
	return This->refcount3;
}
ULONG WINAPI dxglDirectDrawSurface7_Release3(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount3 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount3);
	ret = This->refcount3;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRef2(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount2);
	TRACE_EXIT(8, This->refcount2);
	return This->refcount2;
}
ULONG WINAPI dxglDirectDrawSurface7_Release2(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount2 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount2);
	ret = This->refcount2;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRef1(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount1);
	TRACE_EXIT(8, This->refcount1);
	return This->refcount1;
}
ULONG WINAPI dxglDirectDrawSurface7_Release1(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount1 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount1);
	ret = This->refcount1;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRefGamma(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcountgamma);
	TRACE_EXIT(8, This->refcountgamma);
	return This->refcountgamma;
}
ULONG WINAPI dxglDirectDrawSurface7_ReleaseGamma(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountgamma == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcountgamma);
	ret = This->refcountgamma;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI dxglDirectDrawSurface7_AddRefColor(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcountcolor);
	TRACE_EXIT(8, This->refcountcolor);
	return This->refcountcolor;
}
ULONG WINAPI dxglDirectDrawSurface7_ReleaseColor(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountcolor == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcountcolor);
	ret = This->refcountcolor;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		dxglDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}

HRESULT WINAPI dxglDirectDrawSurface7_AddAttachedSurface(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = dxglDirectDrawSurface7_AddAttachedSurface2(This, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}

HRESULT dxglDirectDrawSurface7_AddAttachedSurface2(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface, IUnknown *iface)
{
	TRACE_ENTER(3,14,This,14,lpDDSAttachedSurface,iface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSAttachedSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->zbuffer) TRACE_RET(HRESULT,23,DDERR_SURFACEALREADYATTACHED);
	dxglDirectDrawSurface7 *attached = (dxglDirectDrawSurface7 *)lpDDSAttachedSurface;
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 ddsdBigSurface;
	HRESULT error;
	int i;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	dxglDirectDrawSurface7_GetSurfaceDesc(attached, &ddsd);
	if((ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER) || (ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
	{
		if (This->zbuffer)
		{
			if (This->zbuffer->texture->dummycolor)
			{
				glTexture_DeleteDummyColor(This->zbuffer->texture, FALSE);
			}
		}
		This->zbuffer = attached;
		This->zbuffer_iface = iface;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = This;
		This->zbuffer->attachcount++;
		if (This->zbuffer && dxglcfg.primaryscale && (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
		{
			if (This->bigsurface && !This->zbuffer->bigsurface)
			{
				memcpy(&ddsdBigSurface, &This->zbuffer->ddsd, sizeof(DDSURFACEDESC2));
				ddsdBigSurface.dwWidth = This->bigsurface->ddsd.dwWidth;
				ddsdBigSurface.dwHeight = This->bigsurface->ddsd.dwHeight;
				error = glDirectDraw7_CreateSurface2(This->ddInterface, &ddsdBigSurface,
					(LPDIRECTDRAWSURFACE7*)&This->zbuffer->bigsurface, NULL, FALSE, 7);
				if (SUCCEEDED(error))
				{
					if ((This->zbuffer->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
						&& This->zbuffer->ddsd.dwBackBufferCount)
					{
						for (i = 0; i < This->zbuffer->ddsd.dwBackBufferCount + 1; i++)
						{
							This->zbuffer->bigsurface[i].texture->bigparent = This->zbuffer[i].texture;
							This->zbuffer[i].texture->bigtexture = This->zbuffer->bigsurface[i].texture;
							This->zbuffer[i].bigsurface = &This->zbuffer->bigsurface[i];
							This->zbuffer->bigsurface[i].bigparent = &This->zbuffer[i];
						}
					}
					else
					{
						This->zbuffer->bigsurface->texture->bigparent = This->zbuffer->texture;
						This->zbuffer->texture->bigtexture = This->zbuffer->bigsurface->texture;
						This->zbuffer->bigsurface->bigparent = This->zbuffer;
					}
				}
			}
			glRenderer_MakeTexturePrimary(This->ddInterface->renderer, This->zbuffer->texture, This->texture, TRUE);
		}
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_CANNOTATTACHSURFACE);
}
HRESULT WINAPI dxglDirectDrawSurface7_AddOverlayDirtyRect(dxglDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI dxglDirectDrawSurface7_Blt(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	HRESULT error;
	RECT tmprect;
	dxglDirectDrawSurface7 *pattern;
	dxglDirectDrawSurface7 *dest;
	dxglDirectDrawSurface7 *src;
	BltCommand cmd;
	TRACE_ENTER(6, 14, This, 26, lpDestRect, 14, lpDDSrcSurface, 26, lpSrcRect, 9, dwFlags, 14, lpDDBltFx);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if ((dwFlags & DDBLT_DEPTHFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 32, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_DDFX) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (This->bigsurface) dest = This->bigsurface;
	else dest = This;
	ZeroMemory(&cmd, sizeof(BltCommand));
	cmd.dest = dest->texture;
	cmd.destlevel = dest->miplevel;
	if (lpDestRect)
	{
		if (This->bigsurface)
		{
			cmd.destrect.left = (LONG)((float)lpDestRect->left * primaryscalex);
			cmd.destrect.top = (LONG)((float)lpDestRect->top * primaryscaley);
			cmd.destrect.right = (LONG)((float)lpDestRect->right * primaryscalex);
			cmd.destrect.bottom = (LONG)((float)lpDestRect->bottom * primaryscaley);
		}
		else cmd.destrect = *lpDestRect;
		if(!glDirectDraw7_GetFullscreen(dest->ddInterface) && (dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			OffsetRect(&cmd.destrect, 0 - dest->ddInterface->renderer->xoffset, 0 - dest->ddInterface->renderer->yoffset);
	}
	else cmd.destrect = nullrect;
	if (lpSrcRect) cmd.srcrect = *lpSrcRect;
	else cmd.srcrect = nullrect;
	if (lpDDSrcSurface)
	{
		cmd.src = ((dxglDirectDrawSurface7*)lpDDSrcSurface)->texture;
		cmd.srclevel = ((dxglDirectDrawSurface7*)lpDDSrcSurface)->miplevel;
	}
	cmd.flags = dwFlags;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			pattern = (dxglDirectDrawSurface7*)lpDDBltFx->lpDDSPattern;
			cmd.pattern = pattern->texture;
			cmd.patternlevel = pattern->miplevel;
		}
	}
	if (dwFlags & DDBLT_KEYSRCOVERRIDE)
	{
		if(!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if(dwFlags & DDBLT_KEYSRC) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	if (dwFlags & DDBLT_KEYDESTOVERRIDE)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (dwFlags & DDBLT_KEYDEST) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	if (dwFlags & DDBLT_KEYSRC)
	{
		if (!lpDDSrcSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (!(((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwFlags & DDSD_CKSRCBLT))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue !=
			((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
			cmd.flags |= 0x20000000;
	}
	if (dwFlags & DDBLT_KEYSRCOVERRIDE)
	{
		if (lpDDBltFx->ddckSrcColorkey.dwColorSpaceHighValue !=
			lpDDBltFx->ddckSrcColorkey.dwColorSpaceLowValue) cmd.flags |= 0x20000000;
		cmd.flags |= DDBLT_KEYSRC;
		memcpy(&cmd.srckey, &lpDDBltFx->ddckSrcColorkey, sizeof(DDCOLORKEY));
	}
	if (dwFlags & DDBLT_KEYDEST)
	{
		if (!(This->ddsd.dwFlags & DDSD_CKDESTBLT)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (This->ddsd.ddckCKDestBlt.dwColorSpaceHighValue != This->ddsd.ddckCKDestBlt.dwColorSpaceLowValue)
			cmd.flags |= 0x40000000;
	}
	if (dwFlags & DDBLT_KEYDESTOVERRIDE)
	{
		if (lpDDBltFx->ddckDestColorkey.dwColorSpaceHighValue !=
			lpDDBltFx->ddckDestColorkey.dwColorSpaceLowValue) cmd.flags |= 0x40000000;
		cmd.flags |= DDBLT_KEYDEST;
		memcpy(&cmd.destkey, &lpDDBltFx->ddckDestColorkey, sizeof(DDCOLORKEY));
	}
	if (lpDDSrcSurface == (LPDIRECTDRAWSURFACE7)This) src = dest;
	else src = (dxglDirectDrawSurface7 *)lpDDSrcSurface;
	if (This->clipper)
	{
		if (!This->clipper->hWnd)
		{
			if (!This->clipper->clipsize) TRACE_RET(HRESULT, 23, DDERR_NOCLIPLIST);
			if (This->clipper->dirty)
			{
				glRenderer_UpdateClipper(This->ddInterface->renderer, &This->clipper->texture, This->clipper->indices,
					This->clipper->vertices, This->clipper->clipsize, This->ddsd.dwWidth, This->ddsd.dwHeight);
				This->clipper->dirty = false;
			}
			dwFlags |= 0x10000000;
		}
	}
	if (This->clipper && !(This->clipper->hWnd)) cmd.flags |= 0x10000000;
	if (lpDDBltFx) cmd.bltfx = *lpDDBltFx;
	if (dwFlags & DDBLT_DEPTHFILL)
	{
		if (!(dest->ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (dest->attachparent)
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, dest->attachparent->texture, dest->attachparent->miplevel));
		}
		else
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, NULL, 0));
		}
	}
	if (dest == src)
	{
		tmprect.left = tmprect.top = 0;
		if (lpSrcRect)
		{
			tmprect.right = lpSrcRect->right - lpSrcRect->left;
			tmprect.bottom = lpSrcRect->bottom - lpSrcRect->top;
		}
		else
		{
			tmprect.right = src->ddsd.dwWidth;
			tmprect.bottom = src->ddsd.dwHeight;
		}
		error = glDirectDraw7_SetupTempSurface(This->ddInterface, tmprect.right, tmprect.bottom);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		error = dxglDirectDrawSurface7_Blt(This->ddInterface->tmpsurface, &tmprect, lpDDSrcSurface, lpSrcRect, 0, NULL);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		if (dwFlags & DDBLT_KEYSRC)
		{
			if (This->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue !=
				This->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
				dxglDirectDrawSurface7_SetColorKey(This->ddInterface->tmpsurface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE,
					&((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
			else dxglDirectDrawSurface7_SetColorKey(This->ddInterface->tmpsurface, DDCKEY_SRCBLT,
				&((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
		}
		TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(dest, lpDestRect, (LPDIRECTDRAWSURFACE7)This->ddInterface->tmpsurface,
			&tmprect, dwFlags, lpDDBltFx));
	}
	else TRACE_RET(HRESULT, 23, glRenderer_Blt(This->ddInterface->renderer, &cmd));
}
HRESULT WINAPI dxglDirectDrawSurface7_BltBatch(dxglDirectDrawSurface7 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_BltBatch: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_BltFast(dxglDirectDrawSurface7 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->clipper) TRACE_RET(HRESULT, 23, DDERR_BLTFASTCANTCLIP);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpDDSrcSurface->GetSurfaceDesc(&ddsd);
	RECT dest;
	dest.left = dwX;
	dest.top = dwY;
	if(lpSrcRect)
	{
		dest.right = dwX + (lpSrcRect->right-lpSrcRect->left);
		dest.bottom = dwY + (lpSrcRect->bottom-lpSrcRect->top);
	}
	else
	{
		dest.right = dwX + ((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwWidth;
		dest.bottom = dwY + ((dxglDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwHeight;
	}
	DWORD flags = 0;
	if(dwTrans & DDBLTFAST_WAIT) flags |= DDBLT_WAIT;
	if(dwTrans & DDBLTFAST_DESTCOLORKEY) flags |= DDBLT_KEYDEST;
	if(dwTrans & DDBLTFAST_SRCCOLORKEY) flags |= DDBLT_KEYSRC;
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Blt(This,&dest,lpDDSrcSurface,lpSrcRect,flags,NULL));
}
HRESULT WINAPI dxglDirectDrawSurface7_DeleteAttachedSurface(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(lpDDSAttachedSurface == (LPDIRECTDRAWSURFACE7)This->zbuffer)
	{
		if (This->zbuffer && dxglcfg.primaryscale && (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			glRenderer_MakeTexturePrimary(This->ddInterface->renderer, This->zbuffer->texture, NULL, FALSE);
		if (This->zbuffer->attachparent == This) This->zbuffer->attachparent = NULL;
		if (This->zbuffer->attachcount) This->zbuffer->attachcount--;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = NULL;
		This->zbuffer_iface->Release();
		This->zbuffer = NULL;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_SURFACENOTATTACHED);
}

dxglDirectDrawSurface7 *GetNextFlip(dxglDirectDrawSurface7 *This)
{
	DWORD index = This->flipindex + 1;
	if (index > This->parent->ddsd.dwBackBufferCount) index = 0;
	if (This->parent->ddsd.dwFlags & DDSD_MIPMAPCOUNT)
		return &This->parent[(index * This->parent->ddsd.dwMipMapCount) + This->miplevel];
	else return &This->parent[index];
}

HRESULT WINAPI dxglDirectDrawSurface7_EnumAttachedSurfaces(dxglDirectDrawSurface7 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT enumret = DDENUMRET_OK;
	dxglDirectDrawSurface7 *target;
	if (This->parent->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		target = GetNextFlip(This);
		dxglDirectDrawSurface7_AddRef(target);
		enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)target, &target->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (This->zbuffer)
	{
		dxglDirectDrawSurface7_AddRef(This->zbuffer);
		enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)This->zbuffer, &This->zbuffer->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (This->parent->ddsd.dwFlags & DDSD_MIPMAPCOUNT)
	{
		if (This->miplevel < This->ddsd.dwMipMapCount - 1)
		{
			dxglDirectDrawSurface7_AddRef(&This[1]);
			enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)&This[1], &This[1].ddsd, lpContext);
		}
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_EnumOverlayZOrders(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
{
	int i;
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpfnCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags > 1) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (This->overlaycount == 0) TRACE_RET(HRESULT, 23, DD_OK);
	if (dwFlags == 1)
	{
		for (i = This->overlaycount; i > 0; i--)
		{
			if (lpfnCallback((LPDIRECTDRAWSURFACE7)This->overlays[i].surface,
				&((dxglDirectDrawSurface7*)This->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	else
	{
		for (i = This->overlaycount; i > 0; i--)
		{
			if (lpfnCallback((LPDIRECTDRAWSURFACE7)This->overlays[i].surface,
				&((dxglDirectDrawSurface7*)This->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	TRACE_RET(HRESULT, 23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_Flip(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	LONG_PTR surfaceoverride2 = (LONG_PTR)lpDDSurfaceTargetOverride;
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glTexture *previous;
	HRESULT ret = dxglDirectDrawSurface7_Flip2(This,lpDDSurfaceTargetOverride,dwFlags,&previous);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	if (This->bigsurface)
	{
		if (lpDDSurfaceTargetOverride)
		{
			surfaceoverride2 = ((LONG_PTR)lpDDSurfaceTargetOverride - (LONG_PTR)This)
				+ (LONG_PTR)This->bigsurface;
		}
		else surfaceoverride2 = NULL;
		dxglDirectDrawSurface7_Flip(This->bigsurface, (LPDIRECTDRAWSURFACE7)surfaceoverride2, dwFlags);
	}
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(This->ddInterface->lastsync)
		{
			This->swapinterval++;
			This->ddInterface->lastsync = false;
		}
		dxglDirectDrawSurface7_RenderScreen(This,This->texture,This->swapinterval,previous,TRUE,NULL,0);
	}
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)
	{
		if (This->overlaydest) dxglDirectDrawSurface7_UpdateOverlayTexture(This->overlaydest, This, This->texture);
	}
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT dxglDirectDrawSurface7_Flip2(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags, glTexture **previous)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & 0xF8FFFFC0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	if(!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && ((dwFlags & DDFLIP_ODD) || (dwFlags & DDFLIP_EVEN))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD i;
	dxglDirectDrawSurface7 *tmp;
	glTexture *texptr;
	glTexture *palette;
	if(previous) *previous = This->texture;
	if(dwFlags & DDFLIP_NOVSYNC) This->swapinterval=0;
	else
	{
		if(dwFlags & DDFLIP_INTERVAL3) This->swapinterval=3;
		else if(dwFlags & DDFLIP_INTERVAL2) This->swapinterval=2;
		else if(dwFlags & DDFLIP_INTERVAL4) This->swapinterval=4;
		else This->swapinterval=1;
	}
	int flips = 1;
	if(lpDDSurfaceTargetOverride)
	{
		BOOL success = FALSE;
		if(lpDDSurfaceTargetOverride == (LPDIRECTDRAWSURFACE7)This) TRACE_RET(HRESULT,23,DD_OK);
		tmp = This;
		for(i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = GetNextFlip(tmp);
			if(lpDDSurfaceTargetOverride == (LPDIRECTDRAWSURFACE7)tmp)
			{
				success = TRUE;
				i++;
				break;
			}
		}
		if(!success) TRACE_RET(HRESULT,23,DDERR_NOTFLIPPABLE);
		for(DWORD x = 0; x < i; x++)
		{
			if(x == i-1) {TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip2(This,NULL,dwFlags,NULL));}
			else dxglDirectDrawSurface7_Flip2(This,NULL,0,NULL);
		}
	}
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		// Get pointer to texture structure
		texptr = (glTexture*)&This[This->ddsd.dwBackBufferCount + 1];

		// Back up palette and remove from prior level
		palette = texptr[This->flipcount].palette;
		if (palette)
		{
			glTexture_AddRef(palette);
			glTexture_SetPalette(&texptr[This->flipcount], NULL, FALSE);
		}

		// Increment flip count
		This->flipcount += flips;
		if (This->flipcount > This->ddsd.dwBackBufferCount)
			This->flipcount -= (This->ddsd.dwBackBufferCount + 1);

		// Flip all mip surfaces if mipmap
		if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
		{
			for (i = 0; i <= This->ddsd.dwBackBufferCount; i++)
			{
				for (DWORD x = 0; x < This->ddsd.dwMipMapCount; x++)
				{
					This[(i * This->ddsd.dwMipMapCount) + x].texture = 
						&texptr[(i + This->flipcount) % This->ddsd.dwBackBufferCount + 1];
				}
			}
		}
		// Flip all surfaces for non-mipmap
		else
		{
			for (i = 0; i <= This->ddsd.dwBackBufferCount; i++)
			{
				This[i].texture = &texptr[(i + This->flipcount) % (This->ddsd.dwBackBufferCount + 1)];
			}
		}
		// Restore texture
		if (palette)
		{
			glTexture_SetPalette(&texptr[This->flipcount], palette, FALSE);
			glTexture_Release(palette, FALSE);
		}

		TRACE_EXIT(23, DD_OK);
		return DD_OK;

		/*glTexture **textures = new glTexture * [This->ddsd.dwBackBufferCount + 1];
		textures[0] = This->texture;
		tmp = This;
		for(i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = GetNextFlip(tmp);
			textures[i+1] = tmp->texture;
		}
		glTexture *tmptex = textures[0];
		memmove(textures,&textures[1],This->ddsd.dwBackBufferCount*sizeof(glTexture*));
		textures[This->ddsd.dwBackBufferCount] = tmptex;
		tmp = This;
		dxglDirectDrawSurface7_SetTexture(This, textures[0]);
		if(This->palette && (This->texture->palette != This->palette->texture))
			glTexture_SetPalette(This->texture, This->palette->texture, FALSE);
		if(This->clipper && (This->texture->stencil != This->clipper->texture))
			glTexture_SetStencil(This->texture, This->clipper->texture, FALSE);
		for(DWORD i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->backbuffer;
			dxglDirectDrawSurface7_SetTexture(tmp, textures[i+1]);
			if (tmp->palette && (tmp->texture->palette != tmp->palette->texture))
				glTexture_SetPalette(tmp->texture, tmp->palette->texture, FALSE);
			if (tmp->clipper && (tmp->texture->stencil != tmp->clipper->texture))
				glTexture_SetStencil(tmp->texture, tmp->clipper->texture, FALSE);
		}
		delete[] textures;*/
	}
	else TRACE_RET(HRESULT,23,DDERR_NOTFLIPPABLE);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetAttachedSurface(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSCAPS2 ddsComp;
	dxglDirectDrawSurface7 *target;
	ZeroMemory(&ddsComp, sizeof(DDSCAPS2));
	unsigned __int64 comp1,comp2;
	if (This->parent->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		target = GetNextFlip(This);
		dxglDirectDrawSurface7_GetCaps(target, &ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)target;
			dxglDirectDrawSurface7_AddRef(target);
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	if(This->zbuffer)
	{
		dxglDirectDrawSurface7_GetCaps(This->zbuffer, &ddsComp);
		memcpy(&comp1,lpDDSCaps,sizeof(unsigned __int64));
		memcpy(&comp2,&ddsComp,sizeof(unsigned __int64));
		if((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)This->zbuffer;
			dxglDirectDrawSurface7_AddRef(This->zbuffer);
			TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
	}
	if((This->parent->ddsd.dwFlags & DDSD_MIPMAPCOUNT) && (This->ddsd.dwMipMapCount > 1))
	{
		dxglDirectDrawSurface7_GetCaps(&This[1], &ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)&This[1];
			dxglDirectDrawSurface7_AddRef(&This[1]);
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}

	TRACE_EXIT(23,DDERR_NOTFOUND);
	return DDERR_NOTFOUND;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetBltStatus(dxglDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT islost = dxglDirectDrawSurface7_IsLost(This);
	if (islost == DDERR_SURFACELOST) return DDERR_SURFACELOST;
	// Async rendering not yet implemented
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetCaps(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpDDSCaps,&This->ddsd.ddsCaps,sizeof(DDSCAPS2));
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetClipper(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetClipper: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetColorKey(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDColorKey) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags == DDCKEY_SRCBLT)
	{
		if(This->ddsd.dwFlags & DDSD_CKSRCBLT)
		{
			memcpy(lpDDColorKey,&This->ddsd.ddckCKSrcBlt,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTBLT)
	{
		if(This->ddsd.dwFlags & DDSD_CKDESTBLT)
		{
			memcpy(lpDDColorKey,&This->ddsd.ddckCKDestBlt,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_SRCOVERLAY)
	{
		if(This->ddsd.dwFlags & DDSD_CKSRCOVERLAY)
		{
			memcpy(lpDDColorKey,&This->ddsd.ddckCKSrcOverlay,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTOVERLAY)
	{
		if(This->ddsd.dwFlags & DDSD_CKDESTOVERLAY)
		{
			memcpy(lpDDColorKey,&This->ddsd.ddckCKDestOverlay,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetDC(dxglDirectDrawSurface7 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lphDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirectDrawPalette *pal = NULL;
	HRESULT error;
	if (This->ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		if (This->palette) pal = This->palette;
		else pal = This->ddInterface->primary->palette;
	}
	error = glTexture_GetDC(This->texture, This->miplevel, lphDC, pal);
	if (SUCCEEDED(error)) { TRACE_VAR("*lphDC", 14, *lphDC); }
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetFlipStatus(dxglDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
	FIXME("dxglDirectDrawSurface7_GetFlipStatus: stub\n");
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetOverlayPosition(dxglDirectDrawSurface7 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
		TRACE_RET(HRESULT, 23, DDERR_NOTAOVERLAYSURFACE);
	if (!This->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_OVERLAYNOTVISIBLE);
	if (!This->overlayset) TRACE_RET(HRESULT, 23, DDERR_NOOVERLAYDEST);
	if (!lplX && !lplY) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lplX) *lplX = This->overlaypos.x;
	if (lplY) *lplY = This->overlaypos.y;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetPalette(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT err;
	if(This->palette)
	{
		glDirectDrawPalette_AddRef(This->palette);
		*lplpDDPalette = (LPDIRECTDRAWPALETTE)This->palette;
		err = DD_OK;
	}
	else
	{
		err = DDERR_NOPALETTEATTACHED;
		*lplpDDPalette = NULL;
		TRACE_VAR("*lplpDDPalette",14,*lplpDDPalette);
	}
	TRACE_EXIT(23,err);
	return err;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetPixelFormat(dxglDirectDrawSurface7 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDPixelFormat) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lpDDPixelFormat = This->ddsd.ddpfPixelFormat;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_GetSurfaceDesc(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&lpDDSurfaceDesc->dwSize+1,&This->ddsd.dwSize+1,lpDDSurfaceDesc->dwSize-sizeof(DWORD)); // copy skipping first DWORD dwSize
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_Initialize(dxglDirectDrawSurface7 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI dxglDirectDrawSurface7_IsLost(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->hRC == This->ddInterface->renderer->hRC) {TRACE_RET(HRESULT,23,DD_OK);}
	else TRACE_RET(HRESULT,23,DDERR_SURFACELOST);
}

HRESULT WINAPI dxglDirectDrawSurface7_Lock(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC2)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	HRESULT error = glTexture_Lock(This->texture, This->miplevel, lpDestRect, lpDDSurfaceDesc, dwFlags, FALSE);
	if (SUCCEEDED(error))
	{
		This->locked++;
		This->ddsd.lpSurface = lpDDSurfaceDesc->lpSurface;
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface7_ReleaseDC(dxglDirectDrawSurface7 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!hDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error = glTexture_ReleaseDC(This->texture, This->miplevel, hDC);
	if (This->bigsurface) This->bigsurface->texture->levels[This->miplevel].dirty |= 4;
	if (((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		if (This->ddInterface->lastsync)
		{
			dxglDirectDrawSurface7_RenderScreen(This, This->texture, 1, NULL, TRUE, NULL, 0);
			This->ddInterface->lastsync = false;
		}
		else dxglDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23,error);
	return error;
}
void dxglDirectDrawSurface7_Restore2(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
/*	LONG sizes[6];
	float xscale, yscale;
	if(This->hRC != This->ddInterface->renderer->hRC)
	{
		glDirectDraw7_GetSizes(This->ddInterface, sizes);
		if(This->ddInterface->fullscreen)
		{
			This->ddsd.dwWidth = sizes[2];
			This->ddsd.dwHeight = sizes[3];
			if(dxglcfg.primaryscale)
			{
				if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
					(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
				{
					if (This->ddsd.dwWidth <= 400) xscale = 2.0f;
					else xscale = 1.0f;
					if (This->ddsd.dwHeight <= 300) yscale = 2.0f;
					else yscale = 1.0f;
				}
				else
				{
					xscale = dxglcfg.postsizex;
					yscale = dxglcfg.postsizey;
				}
				This->fakex = (DWORD)((float)sizes[0] / xscale);
				This->fakey = (DWORD)((float)sizes[1] / yscale);
			}
			else
			{
				This->fakex = This->ddsd.dwWidth;
				This->fakey = This->ddsd.dwHeight;
			}
			This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
		else
		{
			This->fakex = This->ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
			This->fakey = This->ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
			This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
		if(This->backbuffer) dxglDirectDrawSurface7_Restore2(This->backbuffer);
		if(This->zbuffer) dxglDirectDrawSurface7_Restore2(This->zbuffer);
		if(This->paltex) glRenderer_MakeTexture(This->ddInterface->renderer,This->paltex,256,1);
		glRenderer_MakeTexture(This->ddInterface->renderer,This->texture,This->fakex,This->fakey);
	}*/
	TRACE_EXIT(0,0);
}
HRESULT WINAPI dxglDirectDrawSurface7_Restore(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	//LONG sizes[6];
	//float xscale, yscale;
	if(!This->ddInterface->renderer) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
/*	if(This->hRC != This->ddInterface->renderer->hRC)
	{
		if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			glDirectDraw7_GetSizes(This->ddInterface, sizes);
			if(This->ddInterface->fullscreen)
			{
				This->ddsd.dwWidth = sizes[2];
				This->ddsd.dwHeight = sizes[3];
				if(dxglcfg.primaryscale)
				{
					if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
						(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
					{
						if (This->ddsd.dwWidth <= 400) xscale = 2.0f;
						else xscale = 1.0f;
						if (This->ddsd.dwHeight <= 300) yscale = 2.0f;
						else yscale = 1.0f;
					}
					else
					{
						xscale = dxglcfg.postsizex;
						yscale = dxglcfg.postsizey;
					}
					This->fakex = (DWORD)((float)sizes[0] / xscale);
					This->fakey = (DWORD)((float)sizes[1] / yscale);
				}
				else
				{
					This->fakex = This->ddsd.dwWidth;
					This->fakey = This->ddsd.dwHeight;
				}
				This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
			else
			{
				This->fakex = This->ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
				This->fakey = This->ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
			if(This->backbuffer) dxglDirectDrawSurface7_Restore2(This->backbuffer);
			if(This->zbuffer) dxglDirectDrawSurface7_Restore2(This->zbuffer);
		}
		else
		{
			if(This->backbuffer) dxglDirectDrawSurface7_Restore(This->backbuffer);
			if(This->zbuffer) dxglDirectDrawSurface7_Restore(This->zbuffer);
		}
		if(This->paltex) glRenderer_MakeTexture(This->ddInterface->renderer,This->paltex,256,1);
		glRenderer_MakeTexture(This->ddInterface->renderer,This->texture,This->fakex,This->fakey);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else */TRACE_RET(HRESULT,23,DD_OK);
}
HRESULT WINAPI dxglDirectDrawSurface7_SetClipper(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->clipper)
	{
		This->clipper->dirty = true;
		glDirectDrawClipper_Release(This->clipper);
	}
	This->clipper = (glDirectDrawClipper *)lpDDClipper;
	if (This->clipper)
	{
		glDirectDrawClipper_AddRef(This->clipper);
		This->clipper->dirty = true;
		if (!This->clipper->texture.initialized) glDirectDrawClipper_CreateTexture(This->clipper, This->texture, This->ddInterface->renderer);
		glTexture_SetStencil(This->texture, &This->clipper->texture, FALSE);
	}
	else
	{
		glTexture_SetStencil(This->texture, NULL, FALSE);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_SetColorKey(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (dwFlags & DDCKEY_SRCBLT)
	{
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKSRCBLT;
			memcpy(&This->ddsd.ddckCKSrcBlt, lpDDColorKey, sizeof(DDCOLORKEY));
		}
		else This->ddsd.dwFlags &= ~DDSD_CKSRCBLT;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_DESTBLT)
	{
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKDESTBLT;
			memcpy(&This->ddsd.ddckCKDestBlt, lpDDColorKey, sizeof(DDCOLORKEY));
		}
		else This->ddsd.dwFlags &= ~DDSD_CKDESTBLT;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_SRCOVERLAY)
	{
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
			memcpy(&This->ddsd.ddckCKSrcOverlay, lpDDColorKey, sizeof(DDCOLORKEY));
		}
		else This->ddsd.dwFlags &= ~DDSD_CKSRCOVERLAY;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_DESTOVERLAY)
	{
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
			memcpy(&This->ddsd.ddckCKDestOverlay, lpDDColorKey, sizeof(DDCOLORKEY));
		}
		else This->ddsd.dwFlags &= ~DDSD_CKDESTOVERLAY;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
		if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			dxglDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, FALSE, 0, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_SetOverlayPosition(dxglDirectDrawSurface7 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
		TRACE_RET(HRESULT, 23, DDERR_NOTAOVERLAYSURFACE);
	if (!This->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_OVERLAYNOTVISIBLE);
	if (!This->overlayset) TRACE_RET(HRESULT, 23, DDERR_NOOVERLAYDEST);
	This->overlaypos.x = lX;
	This->overlaypos.y = lY;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_SetPalette(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if ((LPDIRECTDRAWPALETTE)This->palette != lpDDPalette)
	{
		if (This->palette)
		{
			This->palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
			This->palette->surface = NULL;
			This->palette->timer = NULL;
			glTexture_SetPalette(This->texture, NULL, FALSE);
			glDirectDrawPalette_Release(This->palette);
			if (!lpDDPalette) glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&This->palette);
		}
		if (lpDDPalette)
		{
			This->palette = (glDirectDrawPalette *)lpDDPalette;
			glDirectDrawPalette_AddRef(This->palette);
			if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
			{
				This->palette->flags |= DDPCAPS_PRIMARYSURFACE;
				This->palette->surface = (LPDIRECTDRAWSURFACE7)This;
				This->palette->timer = &This->ddInterface->renderer->timer;
			}
			else
			{
				This->palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
				This->palette->surface = NULL;
				This->palette->timer = NULL;
			}
		}
	}
	if (This->palette)
	{
		if (!This->palette->texture.initialized) glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
		glTexture_SetPalette(This->texture, &This->palette->texture, FALSE);
	}
	else
	{
		glTexture_SetPalette(This->texture, NULL, FALSE);
	}
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (!dxglcfg.DebugNoPaletteRedraw)
		{
			if(DXGLTimer_CheckLastDraw(&This->ddInterface->renderer->timer,dxglcfg.HackPaletteDelay))
				dxglDirectDrawSurface7_RenderScreen(This, This->texture, dxglcfg.HackPaletteVsync, NULL, FALSE, NULL, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

HRESULT WINAPI dxglDirectDrawSurface7_SetPaletteNoDraw(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2, 14, This, 14, lpDDPalette);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if ((LPDIRECTDRAWPALETTE)This->palette != lpDDPalette)
	{
		if (This->palette)
		{
			This->palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
			This->palette->surface = NULL;
			This->palette->timer = NULL;
			glDirectDrawPalette_Release(This->palette);
			if (!lpDDPalette) glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&This->palette);
		}
		if (lpDDPalette)
		{
			This->palette = (glDirectDrawPalette *)lpDDPalette;
			glDirectDrawPalette_AddRef(This->palette);
			if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
			{
				This->palette->flags |= DDPCAPS_PRIMARYSURFACE;
				This->palette->surface = (LPDIRECTDRAWSURFACE7)This;
				This->palette->timer = &This->ddInterface->renderer->timer;
			}
			else
			{
				This->palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
				This->palette->surface = NULL;
				This->palette->timer = NULL;
			}
		}
	}
	if (This->palette)
	{
		if (!This->palette->texture.initialized) glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
		glTexture_SetPalette(This->texture, &This->palette->texture, FALSE);
	}
	else
	{
		glTexture_SetPalette(This->texture, NULL, FALSE);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI dxglDirectDrawSurface7_Unlock(dxglDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!This->locked)
	{
		This->texture->levels[This->miplevel].dirty |= 1;
		if (This->bigsurface) This->bigsurface->texture->levels[This->miplevel].dirty |= 4;
		TRACE_RET(HRESULT, 23, DDERR_NOTLOCKED);
	}
	This->locked--;
	glTexture_Unlock(This->texture, This->miplevel, lpRect, FALSE);
	if (This->bigsurface) This->bigsurface->texture->levels[This->miplevel].dirty |= 4;
	This->ddsd.lpSurface = NULL;
	if(((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		{
			if(This->ddInterface->lastsync)
			{
				dxglDirectDrawSurface7_RenderScreen(This,This->texture,1,NULL,TRUE,NULL,0);
				This->ddInterface->lastsync = false;
			}
			else dxglDirectDrawSurface7_RenderScreen(This,This->texture,0,NULL,TRUE,NULL,0);
		}
	if ((This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && This->overlayenabled)
	{
		if (This->ddInterface->lastsync)
		{
			dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture, 1, NULL, TRUE, NULL, 0);
			This->ddInterface->lastsync = false;
		}
		else dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlay(dxglDirectDrawSurface7 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) 
		TRACE_RET(HRESULT,23,DDERR_NOTAOVERLAYSURFACE);
	if (!(((dxglDirectDrawSurface7 *)lpDDDestSurface)->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
		TRACE_RET(HRESULT, 23, DDERR_INVALIDSURFACETYPE);
	if ((dwFlags & DDOVER_SHOW) && (dwFlags & DDOVER_HIDE))
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if((dwFlags & DDOVER_DDFX) && !lpDDOverlayFx)
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDOverlayFx)
	{
		if (lpDDOverlayFx->dwSize != sizeof(DDOVERLAYFX))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if(!lpDDOverlayFx->dwFlags)
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	if(dwFlags & 0xFF040000) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpSrcRect)
	{
		if((lpSrcRect->left < 0) ||	(lpSrcRect->right > This->ddsd.dwWidth) ||
			(lpSrcRect->top < 0) ||	(lpSrcRect->bottom > This->ddsd.dwHeight) ||
			(lpSrcRect->right < lpSrcRect->left) || (lpSrcRect->bottom < lpSrcRect->top))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	if (lpDestRect)
	{
		if((lpDestRect->right < lpDestRect->left) || (lpDestRect->bottom < lpDestRect->top))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	}
	OVERLAY newoverlay;
	ZeroMemory(&newoverlay, sizeof(OVERLAY));
	if (lpSrcRect) newoverlay.srcrect = *lpSrcRect;
	else newoverlay.srcrect = nullrect;
	if (lpDestRect) newoverlay.destrect = *lpDestRect;
	else newoverlay.destrect = nullrect;
	newoverlay.surface = This;
	newoverlay.texture = This->texture;
	if (lpDDOverlayFx) newoverlay.fx = *lpDDOverlayFx;
	newoverlay.flags = dwFlags;
	if (dwFlags & DDOVER_SHOW)
	{
		This->overlayenabled = TRUE;
		newoverlay.enabled = TRUE;
	}
	if (dwFlags & DDOVER_HIDE)
	{
		This->overlayenabled = FALSE;
		newoverlay.enabled = FALSE;
	}
	This->overlaydest = (dxglDirectDrawSurface7 *)lpDDDestSurface;
	dxglDirectDrawSurface7_AddOverlay((dxglDirectDrawSurface7 *)lpDDDestSurface, &newoverlay);
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlayDisplay(dxglDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlayZOrder(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

extern "C" void glDirectDrawSurface7_RenderScreen2(LPDIRECTDRAWSURFACE7 surface, int vsync, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	dxglDirectDrawSurface7_RenderScreen((dxglDirectDrawSurface7*)surface, ((dxglDirectDrawSurface7*)surface)->texture, vsync, NULL, settime, overlays, overlaycount);
}

void dxglDirectDrawSurface7_RenderScreen(dxglDirectDrawSurface7 *This, glTexture *texture, int vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	TRACE_ENTER(3,14,This,14,texture,14,vsync);
	if(texture->bigtexture) glRenderer_DrawScreen(This->ddInterface->renderer, texture->bigtexture, texture->palette, vsync, previous, settime, overlays, overlaycount);
	else glRenderer_DrawScreen(This->ddInterface->renderer, texture, texture->palette, vsync, previous, settime, overlays, overlaycount);
	TRACE_EXIT(0,0);
}
// ddraw 2+ api
HRESULT WINAPI dxglDirectDrawSurface7_GetDDInterface(dxglDirectDrawSurface7 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7_AddRef(This->ddInterface);
	*lplpDD = This->ddInterface;
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_PageLock(dxglDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	This->pagelocked++;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface7_PageUnlock(dxglDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->pagelocked) TRACE_RET(HRESULT,23,DDERR_NOTPAGELOCKED);
	This->pagelocked--;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}


// ddraw 3+ api
void MergeDDSD(LPDDSURFACEDESC2 ddsdDest, LPDDSURFACEDESC2 ddsdSrc)
{
	if (ddsdSrc->dwFlags & DDSD_CKDESTBLT)
	{
		ddsdDest->dwFlags |= DDSD_CKDESTBLT;
		memcpy(&ddsdDest->ddckCKDestBlt, &ddsdSrc->ddckCKDestBlt, sizeof(DDCOLORKEY));
	}
	if (ddsdSrc->dwFlags & DDSD_CKDESTOVERLAY)
	{
		ddsdDest->dwFlags |= DDSD_CKDESTOVERLAY;
		memcpy(&ddsdDest->ddckCKDestOverlay, &ddsdSrc->ddckCKDestOverlay, sizeof(DDCOLORKEY));
	}
	if (ddsdSrc->dwFlags & DDSD_CKSRCBLT)
	{
		ddsdDest->dwFlags |= DDSD_CKSRCBLT;
		memcpy(&ddsdDest->ddckCKSrcBlt, &ddsdSrc->ddckCKSrcBlt, sizeof(DDCOLORKEY));
	}
	if (ddsdSrc->dwFlags & DDSD_CKSRCOVERLAY)
	{
		ddsdDest->dwFlags |= DDSD_CKSRCOVERLAY;
		memcpy(&ddsdDest->ddckCKSrcOverlay, &ddsdSrc->ddckCKSrcOverlay, sizeof(DDCOLORKEY));
	}
	if (ddsdSrc->dwFlags & DDSD_WIDTH)
	{
		ddsdDest->dwFlags |= DDSD_WIDTH;
		ddsdDest->dwWidth = ddsdSrc->dwWidth;
	}
	if (ddsdSrc->dwFlags & DDSD_HEIGHT)
	{
		ddsdDest->dwFlags |= DDSD_HEIGHT;
		ddsdDest->dwHeight = ddsdSrc->dwHeight;
	}
	if (ddsdSrc->dwFlags & DDSD_PITCH)
	{
		ddsdDest->dwFlags |= DDSD_PITCH;
		ddsdDest->lPitch = ddsdSrc->lPitch;
	}
}

HRESULT WINAPI dxglDirectDrawSurface7_SetSurfaceDesc(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd2,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpddsd2) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(lpddsd2->dwSize != sizeof(DDSURFACEDESC2)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (This->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_SURFACEBUSY);
	// Handle currently unsupported changes
	if (lpddsd2->dwFlags & DDSD_ALPHABITDEPTH) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (lpddsd2->dwFlags & DDSD_MIPMAPCOUNT) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (lpddsd2->dwFlags & DDSD_REFRESHRATE) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (lpddsd2->dwFlags & DDSD_ZBUFFERBITDEPTH) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (lpddsd2->dwFlags & DDSD_CAPS)
	{
		if (lpddsd2->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (lpddsd2->ddsCaps.dwCaps & DDSCAPS_MIPMAP) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (lpddsd2->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (lpddsd2->ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	}
	// Certain types of target unsupported
	if (This->ddsd.dwFlags & DDSD_MIPMAPCOUNT) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	if (This->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	// Mismatched complex surfaces are unsupported
	if (lpddsd2->dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		if(lpddsd2->dwBackBufferCount != This->ddsd.dwBackBufferCount)
			TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	}
	// Pixel format not currently supported by glTexture.c
	if (lpddsd2->dwFlags & DDSD_PIXELFORMAT) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
	// Surface pointer not currently supported by glTexture.c
	if (lpddsd2->dwFlags & DDSD_LPSURFACE) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);

	// Merge surface description with input
	MergeDDSD(&This->ddsd, lpddsd2);
	
	// Apply surface description to texture
	glRenderer_SetTextureSurfaceDesc(This->ddInterface->renderer, This->texture, lpddsd2);

	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
// ddraw 4+ api
HRESULT WINAPI dxglDirectDrawSurface7_SetPrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,This,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_SetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetPrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,This,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_FreePrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag)
{
	TRACE_ENTER(2,14,This,24,&guidTag);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_FreePrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetUniquenessValue(dxglDirectDrawSurface7 *This, LPDWORD lpValue)
{
	TRACE_ENTER(2,14,This,14,lpValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_ChangeUniquenessValue(dxglDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_ChangeUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
// ddraw 7 api
HRESULT WINAPI dxglDirectDrawSurface7_SetPriority(dxglDirectDrawSurface7 *This, DWORD dwPriority)
{
	TRACE_ENTER(2,14,This,8,dwPriority);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_SetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetPriority(dxglDirectDrawSurface7 *This, LPDWORD lpdwPriority)
{
	TRACE_ENTER(2,14,This,14,lpdwPriority);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_SetLOD(dxglDirectDrawSurface7 *This, DWORD dwMaxLOD)
{
	TRACE_ENTER(2,14,This,8,dwMaxLOD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_SetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_GetLOD(dxglDirectDrawSurface7 *This, LPDWORD lpdwMaxLOD)
{
	TRACE_ENTER(2,14,This,14,lpdwMaxLOD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI dxglDirectDrawSurface7_Unlock2(dxglDirectDrawSurface7 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	// FIXME:  Get rect from pointer
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Unlock(This, (LPRECT)lpSurfaceData));
}

HRESULT dxglDirectDrawSurface7_GetHandle(dxglDirectDrawSurface7 *This, glDirect3DDevice7 *glD3DDev7, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,glD3DDev7,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!glD3DDev7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->handle)
	{
		if(This->device != glD3DDev7) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		*lpHandle = This->handle;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	This->device = glD3DDev7;
	This->handle = glDirect3DDevice7_AddTexture(This->device, This);
	if(This->handle == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	glDirect3DDevice7_AddRef(This->device);
	*lpHandle = This->handle;
	TRACE_VAR("*lpHandle",9,*lpHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT dxglDirectDrawSurface7_Load(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *src)
{
	TRACE_ENTER(2,14,This,14,src);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!src) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (src == This) TRACE_RET(HRESULT, 23, DD_OK);
	dxglDirectDrawSurface7_Blt(This,NULL,(LPDIRECTDRAWSURFACE7)src,NULL,DDBLT_WAIT,NULL);
	if (src->ddsd.dwFlags & DDSD_CKSRCBLT)
		dxglDirectDrawSurface7_SetColorKey(This, DDCKEY_SRCBLT, &src->ddsd.ddckCKSrcBlt);
	This->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT dxglDirectDrawSurface7_GetGammaRamp(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_GetGammaRamp: stub\n");
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT dxglDirectDrawSurface7_SetGammaRamp(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("dxglDirectDrawSurface7_SetGammaRamp: stub\n");
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT dxglDirectDrawSurface7_AddOverlay(dxglDirectDrawSurface7 *This, OVERLAY *overlay)
{
	OVERLAY *tmpptr;
	int i;
	if (This->overlaycount + 1 > This->maxoverlays)
	{
		if (!This->overlays)
		{
			This->overlays = (OVERLAY*)malloc(16 * sizeof(OVERLAY));
			if (!This->overlays) return DDERR_OUTOFMEMORY;
			This->maxoverlays = 16;
		}
		else
		{
			if (This->maxoverlays == 256) return DDERR_OUTOFCAPS;
			This->maxoverlays += 16;
			tmpptr = (OVERLAY*)realloc(This->overlays, This->maxoverlays * sizeof(OVERLAY));
			if (!tmpptr) return DDERR_OUTOFMEMORY;
			This->overlays = tmpptr;
		}
	}
	for (i = 0; i < This->overlaycount; i++)
	{
		if (This->overlays[i].surface == overlay->surface)
		{
			glTexture_Release(This->overlays[i].texture, FALSE);
			This->overlays[i].destrect = overlay->destrect;
			This->overlays[i].srcrect = overlay->srcrect;
			This->overlays[i].flags = overlay->flags;
			This->overlays[i].fx = overlay->fx;
			This->overlays[i].surface = overlay->surface;
			This->overlays[i].texture = overlay->texture;
			if (overlay->flags & DDOVER_SHOW) This->overlays[i].enabled = TRUE;
			if (overlay->flags & DDOVER_HIDE) This->overlays[i].enabled = FALSE;
			memcpy(&This->overlays[i], overlay, sizeof(OVERLAY));
			glTexture_AddRef(This->overlays[i].texture);
			if (This->ddInterface->lastsync)
			{
				dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					1, NULL, TRUE, This->overlays, This->overlaycount);
				This->ddInterface->lastsync = false;
			}
			else dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
				0, NULL, TRUE, This->overlays, This->overlaycount);
			return DD_OK;
		}
	}
	This->overlays[This->overlaycount] = *overlay;
	glTexture_AddRef(This->overlays[This->overlaycount].texture);
	This->overlaycount++;
	if (This->ddInterface->lastsync)
	{
		dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
			1, NULL, TRUE, This->overlays, This->overlaycount);
		This->ddInterface->lastsync = false;
	}
	else dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
		0, NULL, TRUE, This->overlays, This->overlaycount);
	return DD_OK;
}

HRESULT dxglDirectDrawSurface7_DeleteOverlay(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *surface)
{
	int i;
	for (i = 0; i < This->overlaycount; i++)
	{
		if (This->overlays[i].surface == surface)
		{
			glTexture_Release(This->overlays[i].texture, FALSE);
			This->overlaycount--;
			memmove(&This->overlays[i], &This->overlays[i + 1], (This->overlaycount - i) * sizeof(OVERLAY));
			if (surface->overlayenabled)
			{
				if (This->ddInterface->lastsync)
				{
					dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
						1, NULL, TRUE, This->overlays, This->overlaycount);
					This->ddInterface->lastsync = false;
				}
				else dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					0, NULL, TRUE, This->overlays, This->overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
}

HRESULT dxglDirectDrawSurface7_UpdateOverlayTexture(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *surface, glTexture *texture)
{
	int i;
	for (i = 0; i < This->overlaycount; i++)
	{
		if (This->overlays[i].surface == surface)
		{
			glTexture_Release(This->overlays[i].texture, FALSE);
			This->overlays[i].texture = texture;
			glTexture_AddRef(This->overlays[i].texture);
			if (surface->overlayenabled)
			{
				if (This->ddInterface->lastsync)
				{
					dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
						1, NULL, TRUE, This->overlays, This->overlaycount);
					This->ddInterface->lastsync = false;
				}
				else dxglDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					0, NULL, TRUE, This->overlays, This->overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
}

void dxglDirectDrawSurface7_SetTexture(dxglDirectDrawSurface7 *This, glTexture *newtexture)
{
	This->texture = newtexture;
}

// DDRAW1 wrapper
dxglDirectDrawSurface1Vtbl glDirectDrawSurface1_impl =
{
	dxglDirectDrawSurface1_QueryInterface,
	dxglDirectDrawSurface1_AddRef,
	dxglDirectDrawSurface1_Release,
	dxglDirectDrawSurface1_AddAttachedSurface,
	dxglDirectDrawSurface1_AddOverlayDirtyRect,
	dxglDirectDrawSurface1_Blt,
	dxglDirectDrawSurface1_BltBatch,
	dxglDirectDrawSurface1_BltFast,
	dxglDirectDrawSurface1_DeleteAttachedSurface,
	dxglDirectDrawSurface1_EnumAttachedSurfaces,
	dxglDirectDrawSurface1_EnumOverlayZOrders,
	dxglDirectDrawSurface1_Flip,
	dxglDirectDrawSurface1_GetAttachedSurface,
	dxglDirectDrawSurface1_GetBltStatus,
	dxglDirectDrawSurface1_GetCaps,
	dxglDirectDrawSurface1_GetClipper,
	dxglDirectDrawSurface1_GetColorKey,
	dxglDirectDrawSurface1_GetDC,
	dxglDirectDrawSurface1_GetFlipStatus,
	dxglDirectDrawSurface1_GetOverlayPosition,
	dxglDirectDrawSurface1_GetPalette,
	dxglDirectDrawSurface1_GetPixelFormat,
	dxglDirectDrawSurface1_GetSurfaceDesc,
	dxglDirectDrawSurface1_Initialize,
	dxglDirectDrawSurface1_IsLost,
	dxglDirectDrawSurface1_Lock,
	dxglDirectDrawSurface1_ReleaseDC,
	dxglDirectDrawSurface1_Restore,
	dxglDirectDrawSurface1_SetClipper,
	dxglDirectDrawSurface1_SetColorKey,
	dxglDirectDrawSurface1_SetOverlayPosition,
	dxglDirectDrawSurface1_SetPalette,
	dxglDirectDrawSurface1_Unlock,
	dxglDirectDrawSurface1_UpdateOverlay,
	dxglDirectDrawSurface1_UpdateOverlayDisplay,
	dxglDirectDrawSurface1_UpdateOverlayZOrder
};
HRESULT dxglDirectDrawSurface1_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface1 *glDDS1)
{
	TRACE_ENTER(2,14,gl_DDS7, 14, glDDS1);
	glDDS1->lpVtbl = &glDirectDrawSurface1_impl;
	glDDS1->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface1_QueryInterface(dxglDirectDrawSurface1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		dxglDirectDrawSurface1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI dxglDirectDrawSurface1_AddRef(dxglDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_AddRef1(This->glDDS7));
}
ULONG WINAPI dxglDirectDrawSurface1_Release(dxglDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_Release1(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface1_AddAttachedSurface(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = dxglDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface1_AddOverlayDirtyRect(dxglDirectDrawSurface1 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI dxglDirectDrawSurface1_Blt(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,This,26,lpDestRect,14,lpDDSrcSurface,14,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			dxglDirectDrawSurface1 *pattern = (dxglDirectDrawSurface1*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, 
				(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI dxglDirectDrawSurface1_BltBatch(dxglDirectDrawSurface1 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface1_BltFast(dxglDirectDrawSurface1 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_BltFast(This->glDDS7, dwX, dwY,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwTrans));
}
HRESULT WINAPI dxglDirectDrawSurface1_DeleteAttachedSurface(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = dxglDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface1_EnumAttachedSurfaces(dxglDirectDrawSurface1 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7,context,EnumSurfacesCallback1));
}
HRESULT WINAPI dxglDirectDrawSurface1_EnumOverlayZOrders(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI dxglDirectDrawSurface1_Flip(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetAttachedSurface(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	dxglDirectDrawSurface7 *attachedsurface;
	dxglDirectDrawSurface1 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = dxglDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		dxglDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface,(void **)&attached1);
		dxglDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface1_GetBltStatus(dxglDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetBltStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetCaps(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	ZeroMemory(&ddsCaps1, sizeof(DDSCAPS2));
	error = dxglDirectDrawSurface7_GetCaps(This->glDDS7,&ddsCaps1);
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface1_GetClipper(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_GetClipper(This->glDDS7,lplpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetColorKey(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetDC(dxglDirectDrawSurface1 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetDC(This->glDDS7,lphDC));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetFlipStatus(dxglDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetFlipStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetOverlayPosition(dxglDirectDrawSurface1 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetPalette(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPalette(This->glDDS7,lplpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetPixelFormat(dxglDirectDrawSurface1 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPixelFormat(This->glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI dxglDirectDrawSurface1_GetSurfaceDesc(dxglDirectDrawSurface1 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_GetSurfaceDesc(This->glDDS7,&ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface1_Initialize(dxglDirectDrawSurface1 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI dxglDirectDrawSurface1_IsLost(dxglDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface1_Lock(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface1_ReleaseDC(dxglDirectDrawSurface1 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI dxglDirectDrawSurface1_Restore(dxglDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface1_SetClipper(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface1_SetColorKey(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface1_SetOverlayPosition(dxglDirectDrawSurface1 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI dxglDirectDrawSurface1_SetPalette(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetPalette(This->glDDS7,lpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface1_Unlock(dxglDirectDrawSurface1 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Unlock2(This->glDDS7,lpSurfaceData));
}
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlay(dxglDirectDrawSurface1 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface1*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlayDisplay(dxglDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlayZOrder(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7, dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}

// DDRAW2 wrapper
dxglDirectDrawSurface2Vtbl glDirectDrawSurface2_impl =
{
	dxglDirectDrawSurface2_QueryInterface,
	dxglDirectDrawSurface2_AddRef,
	dxglDirectDrawSurface2_Release,
	dxglDirectDrawSurface2_AddAttachedSurface,
	dxglDirectDrawSurface2_AddOverlayDirtyRect,
	dxglDirectDrawSurface2_Blt,
	dxglDirectDrawSurface2_BltBatch,
	dxglDirectDrawSurface2_BltFast,
	dxglDirectDrawSurface2_DeleteAttachedSurface,
	dxglDirectDrawSurface2_EnumAttachedSurfaces,
	dxglDirectDrawSurface2_EnumOverlayZOrders,
	dxglDirectDrawSurface2_Flip,
	dxglDirectDrawSurface2_GetAttachedSurface,
	dxglDirectDrawSurface2_GetBltStatus,
	dxglDirectDrawSurface2_GetCaps,
	dxglDirectDrawSurface2_GetClipper,
	dxglDirectDrawSurface2_GetColorKey,
	dxglDirectDrawSurface2_GetDC,
	dxglDirectDrawSurface2_GetFlipStatus,
	dxglDirectDrawSurface2_GetOverlayPosition,
	dxglDirectDrawSurface2_GetPalette,
	dxglDirectDrawSurface2_GetPixelFormat,
	dxglDirectDrawSurface2_GetSurfaceDesc,
	dxglDirectDrawSurface2_Initialize,
	dxglDirectDrawSurface2_IsLost,
	dxglDirectDrawSurface2_Lock,
	dxglDirectDrawSurface2_ReleaseDC,
	dxglDirectDrawSurface2_Restore,
	dxglDirectDrawSurface2_SetClipper,
	dxglDirectDrawSurface2_SetColorKey,
	dxglDirectDrawSurface2_SetOverlayPosition,
	dxglDirectDrawSurface2_SetPalette,
	dxglDirectDrawSurface2_Unlock,
	dxglDirectDrawSurface2_UpdateOverlay,
	dxglDirectDrawSurface2_UpdateOverlayDisplay,
	dxglDirectDrawSurface2_UpdateOverlayZOrder,
	dxglDirectDrawSurface2_GetDDInterface,
	dxglDirectDrawSurface2_PageLock,
	dxglDirectDrawSurface2_PageUnlock
};

HRESULT dxglDirectDrawSurface2_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface2 *glDDS2)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS2);
	glDDS2->lpVtbl = &glDirectDrawSurface2_impl;
	glDDS2->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI dxglDirectDrawSurface2_QueryInterface(dxglDirectDrawSurface2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		dxglDirectDrawSurface2_AddRef(This);
		*ppvObj = This;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI dxglDirectDrawSurface2_AddRef(dxglDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_AddRef2(This->glDDS7));
}
ULONG WINAPI dxglDirectDrawSurface2_Release(dxglDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_Release2(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface2_AddAttachedSurface(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = dxglDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface2_AddOverlayDirtyRect(dxglDirectDrawSurface2 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI dxglDirectDrawSurface2_Blt(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,This,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			dxglDirectDrawSurface2 *pattern = (dxglDirectDrawSurface2*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI dxglDirectDrawSurface2_BltBatch(dxglDirectDrawSurface2 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_BltFast(dxglDirectDrawSurface2 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,4,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI dxglDirectDrawSurface2_DeleteAttachedSurface(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = dxglDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface2_EnumAttachedSurfaces(dxglDirectDrawSurface2 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI dxglDirectDrawSurface2_EnumOverlayZOrders(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI dxglDirectDrawSurface2_Flip(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetAttachedSurface(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	dxglDirectDrawSurface7 *attachedsurface;
	dxglDirectDrawSurface2 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = dxglDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		dxglDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface2,(void **)&attached1);
		dxglDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE2)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface2_GetBltStatus(dxglDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetCaps(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = dxglDirectDrawSurface7_GetCaps(This->glDDS7, &ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface2_GetClipper(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetClipper(This->glDDS7, lplpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetColorKey(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetColorKey(This->glDDS7, dwFlags, lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetDC(dxglDirectDrawSurface2 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetDC(This->glDDS7, lphDC));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetFlipStatus(dxglDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetFlipStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetOverlayPosition(dxglDirectDrawSurface2 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetPalette(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetPixelFormat(dxglDirectDrawSurface2 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetSurfaceDesc(dxglDirectDrawSurface2 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, &ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface2_Initialize(dxglDirectDrawSurface2 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI dxglDirectDrawSurface2_IsLost(dxglDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface2_Lock(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface2_ReleaseDC(dxglDirectDrawSurface2 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI dxglDirectDrawSurface2_Restore(dxglDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface2_SetClipper(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface2_SetColorKey(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface2_SetOverlayPosition(dxglDirectDrawSurface2 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI dxglDirectDrawSurface2_SetPalette(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface2_Unlock(dxglDirectDrawSurface2 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Unlock2(This->glDDS7, lpSurfaceData));
}
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlay(dxglDirectDrawSurface2 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlay(This->glDDS7, lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface2*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlayDisplay(dxglDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlayZOrder(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI dxglDirectDrawSurface2_GetDDInterface(dxglDirectDrawSurface2 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = dxglDirectDrawSurface7_GetDDInterface(This->glDDS7, (void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw,lplpDD);
	glDirectDraw7_Release(glDD7);
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_RET(HRESULT,23,ret);
}
HRESULT WINAPI dxglDirectDrawSurface2_PageLock(dxglDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface2_PageUnlock(dxglDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}

// DDRAW3 wrapper
dxglDirectDrawSurface3Vtbl glDirectDrawSurface3_impl =
{
	dxglDirectDrawSurface3_QueryInterface,
	dxglDirectDrawSurface3_AddRef,
	dxglDirectDrawSurface3_Release,
	dxglDirectDrawSurface3_AddAttachedSurface,
	dxglDirectDrawSurface3_AddOverlayDirtyRect,
	dxglDirectDrawSurface3_Blt,
	dxglDirectDrawSurface3_BltBatch,
	dxglDirectDrawSurface3_BltFast,
	dxglDirectDrawSurface3_DeleteAttachedSurface,
	dxglDirectDrawSurface3_EnumAttachedSurfaces,
	dxglDirectDrawSurface3_EnumOverlayZOrders,
	dxglDirectDrawSurface3_Flip,
	dxglDirectDrawSurface3_GetAttachedSurface,
	dxglDirectDrawSurface3_GetBltStatus,
	dxglDirectDrawSurface3_GetCaps,
	dxglDirectDrawSurface3_GetClipper,
	dxglDirectDrawSurface3_GetColorKey,
	dxglDirectDrawSurface3_GetDC,
	dxglDirectDrawSurface3_GetFlipStatus,
	dxglDirectDrawSurface3_GetOverlayPosition,
	dxglDirectDrawSurface3_GetPalette,
	dxglDirectDrawSurface3_GetPixelFormat,
	dxglDirectDrawSurface3_GetSurfaceDesc,
	dxglDirectDrawSurface3_Initialize,
	dxglDirectDrawSurface3_IsLost,
	dxglDirectDrawSurface3_Lock,
	dxglDirectDrawSurface3_ReleaseDC,
	dxglDirectDrawSurface3_Restore,
	dxglDirectDrawSurface3_SetClipper,
	dxglDirectDrawSurface3_SetColorKey,
	dxglDirectDrawSurface3_SetOverlayPosition,
	dxglDirectDrawSurface3_SetPalette,
	dxglDirectDrawSurface3_Unlock,
	dxglDirectDrawSurface3_UpdateOverlay,
	dxglDirectDrawSurface3_UpdateOverlayDisplay,
	dxglDirectDrawSurface3_UpdateOverlayZOrder,
	dxglDirectDrawSurface3_GetDDInterface,
	dxglDirectDrawSurface3_PageLock,
	dxglDirectDrawSurface3_PageUnlock,
	dxglDirectDrawSurface3_SetSurfaceDesc
};

HRESULT dxglDirectDrawSurface3_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface3 *glDDS3)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS3);
	glDDS3->lpVtbl = &glDirectDrawSurface3_impl;
	glDDS3->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI dxglDirectDrawSurface3_QueryInterface(dxglDirectDrawSurface3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		dxglDirectDrawSurface3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI dxglDirectDrawSurface3_AddRef(dxglDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if (!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_AddRef3(This->glDDS7));
}
ULONG WINAPI dxglDirectDrawSurface3_Release(dxglDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_Release3(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface3_AddAttachedSurface(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = dxglDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface3_AddOverlayDirtyRect(dxglDirectDrawSurface3 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI dxglDirectDrawSurface3_Blt(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,This,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			dxglDirectDrawSurface3 *pattern = (dxglDirectDrawSurface3*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, 
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI dxglDirectDrawSurface3_BltBatch(dxglDirectDrawSurface3 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_BltFast(dxglDirectDrawSurface3 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI dxglDirectDrawSurface3_DeleteAttachedSurface(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = dxglDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface3_EnumAttachedSurfaces(dxglDirectDrawSurface3 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI dxglDirectDrawSurface3_EnumOverlayZOrders(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI dxglDirectDrawSurface3_Flip(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetAttachedSurface(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	dxglDirectDrawSurface7 *attachedsurface;
	dxglDirectDrawSurface3 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = dxglDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		dxglDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface3,(void **)&attached1);
		dxglDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE3)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface3_GetBltStatus(dxglDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetCaps(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = dxglDirectDrawSurface7_GetCaps(This->glDDS7, &ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface3_GetClipper(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetClipper(This->glDDS7,lplpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetColorKey(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetDC(dxglDirectDrawSurface3 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetDC(This->glDDS7,lphDC));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetFlipStatus(dxglDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetFlipStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetOverlayPosition(dxglDirectDrawSurface3 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetPalette(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetPixelFormat(dxglDirectDrawSurface3 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetSurfaceDesc(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, &ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface3_Initialize(dxglDirectDrawSurface3 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI dxglDirectDrawSurface3_IsLost(dxglDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface3_Lock(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = dxglDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface3_ReleaseDC(dxglDirectDrawSurface3 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI dxglDirectDrawSurface3_Restore(dxglDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface3_SetClipper(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface3_SetColorKey(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface3_SetOverlayPosition(dxglDirectDrawSurface3 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI dxglDirectDrawSurface3_SetPalette(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface3_Unlock(dxglDirectDrawSurface3 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Unlock2(This->glDDS7, lpSurfaceData));
}
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlay(dxglDirectDrawSurface3 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface3*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlayDisplay(dxglDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlayZOrder(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI dxglDirectDrawSurface3_GetDDInterface(dxglDirectDrawSurface3 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = dxglDirectDrawSurface7_GetDDInterface(This->glDDS7, (void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw,lplpDD);
	glDirectDraw7_Release(glDD7);
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface3_PageLock(dxglDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,14,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_PageUnlock(dxglDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface3_SetSurfaceDesc(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetSurfaceDesc(This->glDDS7,(LPDDSURFACEDESC2)lpddsd,dwFlags));
}

// DDRAW4 wrapper
dxglDirectDrawSurface4Vtbl glDirectDrawSurface4_impl =
{
	dxglDirectDrawSurface4_QueryInterface,
	dxglDirectDrawSurface4_AddRef,
	dxglDirectDrawSurface4_Release,
	dxglDirectDrawSurface4_AddAttachedSurface,
	dxglDirectDrawSurface4_AddOverlayDirtyRect,
	dxglDirectDrawSurface4_Blt,
	dxglDirectDrawSurface4_BltBatch,
	dxglDirectDrawSurface4_BltFast,
	dxglDirectDrawSurface4_DeleteAttachedSurface,
	dxglDirectDrawSurface4_EnumAttachedSurfaces,
	dxglDirectDrawSurface4_EnumOverlayZOrders,
	dxglDirectDrawSurface4_Flip,
	dxglDirectDrawSurface4_GetAttachedSurface,
	dxglDirectDrawSurface4_GetBltStatus,
	dxglDirectDrawSurface4_GetCaps,
	dxglDirectDrawSurface4_GetClipper,
	dxglDirectDrawSurface4_GetColorKey,
	dxglDirectDrawSurface4_GetDC,
	dxglDirectDrawSurface4_GetFlipStatus,
	dxglDirectDrawSurface4_GetOverlayPosition,
	dxglDirectDrawSurface4_GetPalette,
	dxglDirectDrawSurface4_GetPixelFormat,
	dxglDirectDrawSurface4_GetSurfaceDesc,
	dxglDirectDrawSurface4_Initialize,
	dxglDirectDrawSurface4_IsLost,
	dxglDirectDrawSurface4_Lock,
	dxglDirectDrawSurface4_ReleaseDC,
	dxglDirectDrawSurface4_Restore,
	dxglDirectDrawSurface4_SetClipper,
	dxglDirectDrawSurface4_SetColorKey,
	dxglDirectDrawSurface4_SetOverlayPosition,
	dxglDirectDrawSurface4_SetPalette,
	dxglDirectDrawSurface4_Unlock,
	dxglDirectDrawSurface4_UpdateOverlay,
	dxglDirectDrawSurface4_UpdateOverlayDisplay,
	dxglDirectDrawSurface4_UpdateOverlayZOrder,
	dxglDirectDrawSurface4_GetDDInterface,
	dxglDirectDrawSurface4_PageLock,
	dxglDirectDrawSurface4_PageUnlock,
	dxglDirectDrawSurface4_SetSurfaceDesc,
	dxglDirectDrawSurface4_SetPrivateData,
	dxglDirectDrawSurface4_GetPrivateData,
	dxglDirectDrawSurface4_FreePrivateData,
	dxglDirectDrawSurface4_GetUniquenessValue,
	dxglDirectDrawSurface4_ChangeUniquenessValue
};

HRESULT dxglDirectDrawSurface4_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface4 *glDDS4)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS4);
	glDDS4->lpVtbl = &glDirectDrawSurface4_impl;
	glDDS4->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI dxglDirectDrawSurface4_QueryInterface(dxglDirectDrawSurface4 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		dxglDirectDrawSurface4_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI dxglDirectDrawSurface4_AddRef(dxglDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_AddRef4(This->glDDS7));
}
ULONG WINAPI dxglDirectDrawSurface4_Release(dxglDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_Release4(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface4_AddAttachedSurface(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = dxglDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface4_AddOverlayDirtyRect(dxglDirectDrawSurface4 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI dxglDirectDrawSurface4_Blt(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,This,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			dxglDirectDrawSurface4 *pattern = (dxglDirectDrawSurface4*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI dxglDirectDrawSurface4_BltBatch(dxglDirectDrawSurface4 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_BltFast(dxglDirectDrawSurface4 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI dxglDirectDrawSurface4_DeleteAttachedSurface(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = dxglDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI dxglDirectDrawSurface4_EnumAttachedSurfaces(dxglDirectDrawSurface4 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback2));
}
HRESULT WINAPI dxglDirectDrawSurface4_EnumOverlayZOrders(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI dxglDirectDrawSurface4_Flip(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetAttachedSurface(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps2,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	dxglDirectDrawSurface7 *attachedsurface;
	dxglDirectDrawSurface4 *attached1;
	error = dxglDirectDrawSurface7_GetAttachedSurface(This->glDDS7,lpDDSCaps2,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		dxglDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface4,(void **)&attached1);
		dxglDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE4)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI dxglDirectDrawSurface4_GetBltStatus(dxglDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetCaps(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetCaps(This->glDDS7, lpDDSCaps));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetClipper(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetClipper(This->glDDS7, lplpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetColorKey(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetDC(dxglDirectDrawSurface4 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetDC(This->glDDS7, lphDC));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetFlipStatus(dxglDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetFlipStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetOverlayPosition(dxglDirectDrawSurface4 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetPalette(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetPixelFormat(dxglDirectDrawSurface4 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetSurfaceDesc(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, lpDDSurfaceDesc));
}
HRESULT WINAPI dxglDirectDrawSurface4_Initialize(dxglDirectDrawSurface4 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI dxglDirectDrawSurface4_IsLost(dxglDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface4_Lock(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Lock(This->glDDS7,lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent));
}
HRESULT WINAPI dxglDirectDrawSurface4_ReleaseDC(dxglDirectDrawSurface4 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI dxglDirectDrawSurface4_Restore(dxglDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetClipper(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetColorKey(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetOverlayPosition(dxglDirectDrawSurface4 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetPalette(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI dxglDirectDrawSurface4_Unlock(dxglDirectDrawSurface4 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_Unlock2(This->glDDS7, lpRect));
}
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlay(dxglDirectDrawSurface4 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((dxglDirectDrawSurface4*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlayDisplay(dxglDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlayZOrder(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetDDInterface(dxglDirectDrawSurface4 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetDDInterface(This->glDDS7,lplpDD));
}
HRESULT WINAPI dxglDirectDrawSurface4_PageLock(dxglDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_PageUnlock(dxglDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetSurfaceDesc(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetSurfaceDesc(This->glDDS7,lpddsd,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_SetPrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,This,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_SetPrivateData(This->glDDS7,guidTag,lpData,cbSize,dwFlags));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetPrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,This,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetPrivateData(This->glDDS7,guidTag,lpBuffer,lpcbBufferSize));
}
HRESULT WINAPI dxglDirectDrawSurface4_FreePrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag)
{
	TRACE_ENTER(2,14,This,24,&guidTag);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_FreePrivateData(This->glDDS7, guidTag));
}
HRESULT WINAPI dxglDirectDrawSurface4_GetUniquenessValue(dxglDirectDrawSurface4 *This, LPDWORD lpValue)
{
	TRACE_ENTER(2,14,This,14,lpValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_GetUniquenessValue(This->glDDS7, lpValue));
}
HRESULT WINAPI dxglDirectDrawSurface4_ChangeUniquenessValue(dxglDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, dxglDirectDrawSurface7_ChangeUniquenessValue(This->glDDS7));
}
