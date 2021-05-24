// DXGL
// Copyright (C) 2011-2021 William Feely

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
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirectDrawPalette.h"
#include "glDirectDrawClipper.h"
#include "glDirectDrawGammaControl.h"
#include "glRenderer.h"
#include <string>
using namespace std;
#include "ShaderGen3D.h"
#include <math.h>

glDirectDrawSurface7Vtbl glDirectDrawSurface7_impl =
{
	glDirectDrawSurface7_QueryInterface,
	glDirectDrawSurface7_AddRef,
	glDirectDrawSurface7_Release,
	glDirectDrawSurface7_AddAttachedSurface,
	glDirectDrawSurface7_AddOverlayDirtyRect,
	glDirectDrawSurface7_Blt,
	glDirectDrawSurface7_BltBatch,
	glDirectDrawSurface7_BltFast,
	glDirectDrawSurface7_DeleteAttachedSurface,
	glDirectDrawSurface7_EnumAttachedSurfaces,
	glDirectDrawSurface7_EnumOverlayZOrders,
	glDirectDrawSurface7_Flip,
	glDirectDrawSurface7_GetAttachedSurface,
	glDirectDrawSurface7_GetBltStatus,
	glDirectDrawSurface7_GetCaps,
	glDirectDrawSurface7_GetClipper,
	glDirectDrawSurface7_GetColorKey,
	glDirectDrawSurface7_GetDC,
	glDirectDrawSurface7_GetFlipStatus,
	glDirectDrawSurface7_GetOverlayPosition,
	glDirectDrawSurface7_GetPalette,
	glDirectDrawSurface7_GetPixelFormat,
	glDirectDrawSurface7_GetSurfaceDesc,
	glDirectDrawSurface7_Initialize,
	glDirectDrawSurface7_IsLost,
	glDirectDrawSurface7_Lock,
	glDirectDrawSurface7_ReleaseDC,
	glDirectDrawSurface7_Restore,
	glDirectDrawSurface7_SetClipper,
	glDirectDrawSurface7_SetColorKey,
	glDirectDrawSurface7_SetOverlayPosition,
	glDirectDrawSurface7_SetPalette,
	glDirectDrawSurface7_Unlock,
	glDirectDrawSurface7_UpdateOverlay,
	glDirectDrawSurface7_UpdateOverlayDisplay,
	glDirectDrawSurface7_UpdateOverlayZOrder,
	glDirectDrawSurface7_GetDDInterface,
	glDirectDrawSurface7_PageLock,
	glDirectDrawSurface7_PageUnlock,
	glDirectDrawSurface7_SetSurfaceDesc,
	glDirectDrawSurface7_SetPrivateData,
	glDirectDrawSurface7_GetPrivateData,
	glDirectDrawSurface7_FreePrivateData,
	glDirectDrawSurface7_GetUniquenessValue,
	glDirectDrawSurface7_ChangeUniquenessValue,
	glDirectDrawSurface7_SetPriority,
	glDirectDrawSurface7_GetPriority,
	glDirectDrawSurface7_SetLOD,
	glDirectDrawSurface7_GetLOD
};

// DDRAW7 routines
HRESULT glDirectDrawSurface7_Create(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, HRESULT *error, glDirectDrawPalette *palettein,
	glTexture *parenttex, DWORD miplevel, int version, glDirectDrawSurface7 *front, glDirectDrawSurface7 **glDDS7)
{
	TRACE_ENTER(9,14,lpDD7,14,lpDDSurfaceDesc2,14,error,14,palettein,14,parenttex,8,miplevel,11,version,14,front,14,glDDS7);
	glDirectDrawSurface7 *This = (glDirectDrawSurface7*)malloc(sizeof(glDirectDrawSurface7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	ZeroMemory(This, sizeof(glDirectDrawSurface7));
	*glDDS7 = This;
	This->lpVtbl = &glDirectDrawSurface7_impl;
	This->version = version;
	glDirectDrawSurface1_Create(This, &This->dds1);
	glDirectDrawSurface2_Create(This, &This->dds2);
	glDirectDrawSurface3_Create(This, &This->dds3);
	glDirectDrawSurface4_Create(This, &This->dds4);
	glDirect3DTexture2_Create(This, &This->d3dt2);
	glDirect3DTexture1_Create(This, &This->d3dt1);
	glDirectDrawGammaControl_Create(This, &This->gammacontrol);
	This->miplevel = miplevel;
	This->ddInterface = (glDirectDraw7 *)lpDD7;
	This->hRC = This->ddInterface->renderer->hRC;
	This->ddsd = *lpDDSurfaceDesc2;
	LONG sizes[6];
	int i;
	float xscale, yscale;
	DWORD winver, winvermajor, winverminor;
	glDirectDraw7_GetSizes(This->ddInterface, sizes);
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(((This->ddsd.dwFlags & DDSD_WIDTH) || (This->ddsd.dwFlags & DDSD_HEIGHT)
			|| (This->ddsd.dwFlags & DDSD_PIXELFORMAT)) && !(This->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
		{
			*error = DDERR_INVALIDPARAMS;
			TRACE_VAR("*error",23,DDERR_INVALIDPARAMS);
			TRACE_EXIT(23,DDERR_INVALIDPARAMS);
			return DDERR_INVALIDPARAMS;
		}
		else
		{
			if(glDirectDraw7_GetFullscreen(This->ddInterface))
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
					switch (dxglcfg.primaryscale)
					{
					case 1: // Scale to window size
					default:
						This->fakex = (DWORD)((float)sizes[0] / xscale);
						This->fakey = (DWORD)((float)sizes[1] / yscale);
						break;
					case 2: // Scale to integer auto
						for (i = 1; i < 100; i++)
						{
							if ((This->ddsd.dwWidth * i) >(DWORD)((float)sizes[0] / xscale))
							{
								This->fakex = This->ddsd.dwWidth * i;
								break;
							}
						}
						for (i = 1; i < 100; i++)
						{
							if ((This->ddsd.dwHeight * i) >(DWORD)((float)sizes[1] / yscale))
							{
								This->fakey = This->ddsd.dwHeight * i;
								break;
							}
						}
						break;
					case 3: // 1.5x scale
						This->fakex = (DWORD)((float)This->ddsd.dwWidth * 1.5f);
						This->fakey = (DWORD)((float)This->ddsd.dwHeight * 1.5f);
						break;
					case 4: // 2x scale
						This->fakex = This->ddsd.dwWidth * 2;
						This->fakey = This->ddsd.dwHeight * 2;
						break;
					case 5: // 2.5x scale
						This->fakex = (DWORD)((float)This->ddsd.dwWidth * 2.5f);
						This->fakey = (DWORD)((float)This->ddsd.dwHeight * 2.5f);
						break;
					case 6: // 3x scale
						This->fakex = This->ddsd.dwWidth * 3;
						This->fakey = This->ddsd.dwHeight * 3;
						break;
					case 7: // 4x scale
						This->fakex = This->ddsd.dwWidth * 4;
						This->fakey = This->ddsd.dwHeight * 4;
						break;
					case 8: // 5x scale
						This->fakex = This->ddsd.dwWidth * 5;
						This->fakey = This->ddsd.dwHeight * 5;
						break;
					case 9: // 6x scale
						This->fakex = This->ddsd.dwWidth * 6;
						This->fakey = This->ddsd.dwHeight * 6;
						break;
					case 10: // 7x scale
						This->fakex = This->ddsd.dwWidth * 7;
						This->fakey = This->ddsd.dwHeight * 7;
						break;
					case 11: // 8x scale
						This->fakex = This->ddsd.dwWidth * 8;
						This->fakey = This->ddsd.dwHeight * 8;
						break;
					case 12: // Custom scale
						This->fakex = (DWORD)((float)This->ddsd.dwWidth * dxglcfg.primaryscalex);
						This->fakey = (DWORD)((float)This->ddsd.dwHeight * dxglcfg.primaryscaley);
						break;
					}
				}
				else
				{
					This->fakex = This->ddsd.dwWidth;
					This->fakey = This->ddsd.dwHeight;
				}
				This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
				*error = DD_OK;
			}
			else
			{
				winver = GetVersion();
				winvermajor = (DWORD)(LOBYTE(LOWORD(winver)));
				winverminor = (DWORD)(HIBYTE(LOWORD(winver)));
				if ((winvermajor > 4) || ((winvermajor == 4) && (winverminor >= 1)))
				{
					This->fakex = This->ddsd.dwWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
					This->fakey = This->ddsd.dwHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
					This->ddInterface->renderer->xoffset = GetSystemMetrics(SM_XVIRTUALSCREEN);
					This->ddInterface->renderer->yoffset = GetSystemMetrics(SM_YVIRTUALSCREEN);
				}
				else
				{
					This->fakex = This->ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
					This->fakey = This->ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				}
				This->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
				*error = DD_OK;
			}
		}
	}
	else
	{
		if((This->ddsd.dwFlags & DDSD_WIDTH) && (This->ddsd.dwFlags & DDSD_HEIGHT))
		{
			This->fakex = This->ddsd.dwWidth;
			This->fakey = This->ddsd.dwHeight;
		}
		else
		{
			*error = DDERR_INVALIDPARAMS;
			TRACE_VAR("*error",23,DDERR_INVALIDPARAMS);
			TRACE_EXIT(23,DDERR_INVALIDPARAMS);
			return DDERR_INVALIDPARAMS;
		}
	}
/*	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	{
		BITMAPINFO info;
		ZeroMemory(&info,sizeof(BITMAPINFO));
		if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			info.bmiHeader.biWidth = This->fakex;
			info.bmiHeader.biHeight = -(signed)This->fakey;
			info.bmiHeader.biCompression = BI_RGB;
			info.bmiHeader.biSizeImage = 0;
			info.bmiHeader.biXPelsPerMeter = 0;
			info.bmiHeader.biYPelsPerMeter = 0;
			info.bmiHeader.biClrImportant = 0;
			info.bmiHeader.biClrUsed = 0;
			info.bmiHeader.biBitCount = (WORD)glDirectDraw7_GetBPPMultipleOf8(This->ddInterface);
			*This->bitmapinfo = info;
		}
		else
		{
			if(This->ddsd.dwFlags & DDSD_PIXELFORMAT) This->surfacetype=2;
			else
			{
				info.bmiHeader.biWidth = This->fakex;
				info.bmiHeader.biHeight = -(signed)This->fakey;
				info.bmiHeader.biCompression = BI_RGB;
				info.bmiHeader.biSizeImage = 0;
				info.bmiHeader.biXPelsPerMeter = 0;
				info.bmiHeader.biYPelsPerMeter = 0;
				info.bmiHeader.biClrImportant = 0;
				info.bmiHeader.biClrUsed = 0;
				info.bmiHeader.biBitCount = (WORD)glDirectDraw7_GetBPPMultipleOf8(This->ddInterface);
				*This->bitmapinfo = info;
			}
		}
	}
	else
	{
		This->bitmapinfo->bmiHeader.biSizeImage = 0;
		This->bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
		This->bitmapinfo->bmiHeader.biYPelsPerMeter = 0;
		This->bitmapinfo->bmiHeader.biClrImportant = 0;
		This->bitmapinfo->bmiHeader.biClrUsed = 0;
		This->bitmapinfo->bmiHeader.biCompression = BI_RGB;
		This->bitmapinfo->bmiHeader.biBitCount = 0;
	}
	This->surfacetype=2;
	This->bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	This->bitmapinfo->bmiHeader.biWidth = This->ddsd.dwWidth;
	This->bitmapinfo->bmiHeader.biHeight = -(signed)This->ddsd.dwHeight;
	This->bitmapinfo->bmiHeader.biPlanes = 1; */
	This->backbuffer = NULL;
	This->backbufferwraparound = NULL;
	if ((This->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && !(This->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if (!(This->ddsd.dwFlags & DDSD_MIPMAPCOUNT))
		{
			This->ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
			This->ddsd.dwMipMapCount = CalculateMipLevels(This->ddsd.dwWidth, This->ddsd.dwHeight);
		}
	}
	/*

	}*/
	if (This->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
	{
		This->texture = parenttex;
		glTexture_AddRef(This->texture);
	}
	else
	{
		*error = glTexture_Create(&This->ddsd, &This->texture, This->ddInterface->renderer, This->fakex, This->fakey, This->hasstencil, FALSE, 0);
		if (*error != DD_OK)
		{
			TRACE_VAR("*error",23,*error);
			TRACE_EXIT(23,*error);
			return *error;
		}
	}
	if (!(This->ddsd.dwFlags & DDSD_PITCH))
	{
		This->ddsd.dwFlags |= DDSD_PITCH;
		This->ddsd.lPitch = This->texture->levels[This->miplevel].ddsd.lPitch;
	}
	if (!(This->ddsd.dwFlags & DDSD_PIXELFORMAT))
	{
		This->ddsd.dwFlags |= DDSD_PIXELFORMAT;
		memcpy(&This->ddsd.ddpfPixelFormat, &This->texture->levels[This->miplevel].ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
	}
	if (!glTexture_ValidatePixelFormat(&This->ddsd.ddpfPixelFormat))
	{
		*error = DDERR_INVALIDPIXELFORMAT;
		TRACE_VAR("*error", 23, DDERR_INVALIDPIXELFORMAT);
		TRACE_EXIT(23, DDERR_INVALIDPIXELFORMAT);
		return DDERR_INVALIDPIXELFORMAT;
	}
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (This->ddInterface->primarybpp == 8)
		{
			if (!palettein)
			{
				glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&This->palette);
				glDirectDrawSurface7_SetPaletteNoDraw(This, (LPDIRECTDRAWPALETTE)This->palette);
			}
			else
			{
				glDirectDrawSurface7_SetPaletteNoDraw(This, (LPDIRECTDRAWPALETTE)palettein);
			}
		}
	}
	if ((This->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && This->ddsd.dwMipMapCount)
	{
		DDSURFACEDESC2 newdesc = This->ddsd;
		newdesc.dwWidth = max(1, (DWORD)floorf((float)This->ddsd.dwWidth / 2.0f));
		newdesc.dwHeight = max(1, (DWORD)floorf((float)This->ddsd.dwHeight / 2.0f));
		newdesc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
		newdesc.dwMipMapCount = This->ddsd.dwMipMapCount - 1;
		HRESULT miperror;
		if(newdesc.dwMipMapCount)
			glDirectDrawSurface7_Create(lpDD7, &newdesc, &miperror, This->palette,
				This->texture, miplevel + 1, version, NULL,&This->miptexture);
	}

/*	if(This->ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
	{
		This->colormasks[0] = This->ddsd.ddpfPixelFormat.dwRBitMask;
		This->colormasks[1] = This->ddsd.ddpfPixelFormat.dwGBitMask;
		This->colormasks[2] = This->ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(This->bitmapinfo->bmiColors,This->colormasks,3*sizeof(DWORD));
	}
	if(!This->bitmapinfo->bmiHeader.biBitCount)
		This->bitmapinfo->bmiHeader.biBitCount = (WORD)This->ddsd.ddpfPixelFormat.dwRGBBitCount;*/
	This->refcount7 = 1;
	This->refcount4 = 0;
	This->refcount3 = 0;
	This->refcount2 = 0;
	This->refcount1 = 0;
	This->refcountgamma = 0;
	This->refcountcolor = 0;
	This->mulx = (float)This->fakex / (float)This->ddsd.dwWidth;
	This->muly = (float)This->fakey / (float)This->ddsd.dwHeight;
	*error = DD_OK;
	This->backbuffer = NULL;
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
	{
		if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
		{
			if((This->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && (This->ddsd.dwBackBufferCount > 0))
			{
				if(!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
					This->ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
				DDSURFACEDESC2 ddsdBack;
				memcpy(&ddsdBack,&This->ddsd, This->ddsd.dwSize);
				ddsdBack.dwBackBufferCount--;
				ddsdBack.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
				ddsdBack.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				glDirectDrawSurface7_Create((LPDIRECTDRAW7)This->ddInterface, &ddsdBack, error,
					This->palette, parenttex, miplevel, version, front ? front : This, &This->backbuffer);
			}
			else if (This->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
			{
				This->backbufferwraparound = front;
			}
			else *error = DDERR_INVALIDPARAMS;
		}
	}
	switch (version)
	{
	case 1:
	case 2:
	case 3:
	default:
		This->textureparent = (IUnknown*)&This->dds1;
		break;
	case 4:
		This->textureparent = (IUnknown*)&This->dds4;
		break;
	case 7:
		This->textureparent = (IUnknown*)This;
		break;
	}
	TRACE_VAR("*error",23,*error);
	TRACE_EXIT(23,*error);
}

void glDirectDrawSurface7_Delete(glDirectDrawSurface7 *This)
{
	int i;
	TRACE_ENTER(1,14,This);
	glDirectDrawSurface7_AddRef(This);
	if (This->overlaydest) glDirectDrawSurface7_DeleteOverlay(This->overlaydest, This);
	if (This->overlays)
	{
		for (i = 0; i < This->overlaycount; i++)
			glTexture_Release(This->overlays[i].texture, FALSE);
		free(This->overlays);
		glDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, FALSE, NULL, -1);
	}
	if (This->texture) glTexture_Release(This->texture, FALSE);
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
	if(This->backbuffer) glDirectDrawSurface7_Release(This->backbuffer);
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
	if(This->miptexture) glDirectDrawSurface7_Release(This->miptexture);
	if (This->device) glDirect3DDevice7_Release(This->device); 
	if (This->device1) glDirect3DDevice7_Destroy(This->device1);
	glDirectDraw7_DeleteSurface(This->ddInterface, This);
	if (This->creator) This->creator->Release();
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI glDirectDrawSurface7_QueryInterface(glDirectDrawSurface7 *This, REFIID riid, void** ppvObj)
{
	HRESULT ret;
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDrawSurface7))
	{
		glDirectDrawSurface7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface4)
	{
		glDirectDrawSurface7_AddRef4(This);
		*ppvObj = &This->dds4;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		glDirectDrawSurface7_AddRef3(This);
		*ppvObj = &This->dds3;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		glDirectDrawSurface7_AddRef2(This);
		*ppvObj = &This->dds2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface)
	{
		glDirectDrawSurface7_AddRef1(This);
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
		glDirectDrawSurface7_AddRefGamma(This);
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
ULONG WINAPI glDirectDrawSurface7_AddRef(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount7);
	TRACE_EXIT(8,This->refcount7);
	return This->refcount7;
}
ULONG WINAPI glDirectDrawSurface7_Release(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	if (This->refcount7 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount7);
	ret = This->refcount7;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8,ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef4(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount4);
	TRACE_EXIT(8, This->refcount4);
	return This->refcount4;
}
ULONG WINAPI glDirectDrawSurface7_Release4(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount4 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount4);
	ret = This->refcount4;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef3(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount3);
	TRACE_EXIT(8, This->refcount3);
	return This->refcount3;
}
ULONG WINAPI glDirectDrawSurface7_Release3(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount3 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount3);
	ret = This->refcount3;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef2(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount2);
	TRACE_EXIT(8, This->refcount2);
	return This->refcount2;
}
ULONG WINAPI glDirectDrawSurface7_Release2(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount2 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount2);
	ret = This->refcount2;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef1(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcount1);
	TRACE_EXIT(8, This->refcount1);
	return This->refcount1;
}
ULONG WINAPI glDirectDrawSurface7_Release1(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount1 == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcount1);
	ret = This->refcount1;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRefGamma(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcountgamma);
	TRACE_EXIT(8, This->refcountgamma);
	return This->refcountgamma;
}
ULONG WINAPI glDirectDrawSurface7_ReleaseGamma(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountgamma == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcountgamma);
	ret = This->refcountgamma;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRefColor(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	InterlockedIncrement((LONG*)&This->refcountcolor);
	TRACE_EXIT(8, This->refcountcolor);
	return This->refcountcolor;
}
ULONG WINAPI glDirectDrawSurface7_ReleaseColor(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountcolor == 0) TRACE_RET(ULONG, 8, 0);
	InterlockedDecrement((LONG*)&This->refcountcolor);
	ret = This->refcountcolor;
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Delete(This);
	TRACE_EXIT(8, ret);
	return ret;
}

HRESULT WINAPI glDirectDrawSurface7_AddAttachedSurface(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(This, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}

HRESULT glDirectDrawSurface7_AddAttachedSurface2(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface, IUnknown *iface)
{
	TRACE_ENTER(3,14,This,14,lpDDSAttachedSurface,iface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSAttachedSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->zbuffer) TRACE_RET(HRESULT,23,DDERR_SURFACEALREADYATTACHED);
	glDirectDrawSurface7 *attached = (glDirectDrawSurface7 *)lpDDSAttachedSurface;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	glDirectDrawSurface7_GetSurfaceDesc(attached, &ddsd);
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
			glRenderer_MakeTexturePrimary(This->ddInterface->renderer, This->zbuffer->texture, This->texture, TRUE);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_CANNOTATTACHSURFACE);
}
HRESULT WINAPI glDirectDrawSurface7_AddOverlayDirtyRect(glDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI glDirectDrawSurface7_Blt(glDirectDrawSurface7 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	HRESULT error;
	RECT tmprect;
	glDirectDrawSurface7* pattern;
	BltCommand cmd;
	TRACE_ENTER(6, 14, This, 26, lpDestRect, 14, lpDDSrcSurface, 26, lpSrcRect, 9, dwFlags, 14, lpDDBltFx);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if ((dwFlags & DDBLT_DEPTHFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 32, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_DDFX) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ZeroMemory(&cmd, sizeof(BltCommand));
	cmd.dest = This->texture;
	cmd.destlevel = This->miplevel;
	if (lpDestRect)
	{
		cmd.destrect = *lpDestRect;
		if(!glDirectDraw7_GetFullscreen(This->ddInterface) && (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			OffsetRect(&cmd.destrect, 0 - This->ddInterface->renderer->xoffset, 0 - This->ddInterface->renderer->yoffset);
	}
	else cmd.destrect = nullrect;
	if (lpSrcRect) cmd.srcrect = *lpSrcRect;
	else cmd.srcrect = nullrect;
	if (lpDDSrcSurface)
	{
		cmd.src = ((glDirectDrawSurface7*)lpDDSrcSurface)->texture;
		cmd.srclevel = ((glDirectDrawSurface7*)lpDDSrcSurface)->miplevel;
	}
	cmd.flags = dwFlags;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			pattern = (glDirectDrawSurface7*)lpDDBltFx->lpDDSPattern;
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
		if (!(((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwFlags & DDSD_CKSRCBLT))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue !=
			((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
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
	glDirectDrawSurface7 *src = (glDirectDrawSurface7 *)lpDDSrcSurface;
	if (This->clipper)
	{
		if (!This->clipper->hWnd)
		{
			if (!This->clipper->clipsize) TRACE_RET(HRESULT, 23, DDERR_NOCLIPLIST);
			if (This->clipper->dirty)
			{
				glRenderer_UpdateClipper(This->ddInterface->renderer, This->clipper->texture, This->clipper->indices,
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
		if (!(This->ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (This->attachparent)
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, This->attachparent->texture, This->attachparent->miplevel));
		}
		else
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, NULL, 0));
		}
	}
	if (This == src)
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
		error = glDirectDrawSurface7_Blt(This->ddInterface->tmpsurface, &tmprect, lpDDSrcSurface, lpSrcRect, 0, NULL);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		if (dwFlags & DDBLT_KEYSRC)
		{
			if (This->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue !=
				This->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
				glDirectDrawSurface7_SetColorKey(This->ddInterface->tmpsurface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE,
					&((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
			else glDirectDrawSurface7_SetColorKey(This->ddInterface->tmpsurface, DDCKEY_SRCBLT,
				&((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
		}
		TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This, lpDestRect, (LPDIRECTDRAWSURFACE7)This->ddInterface->tmpsurface,
			&tmprect, dwFlags, lpDDBltFx));
	}
	else TRACE_RET(HRESULT, 23, glRenderer_Blt(This->ddInterface->renderer, &cmd));
}
HRESULT WINAPI glDirectDrawSurface7_BltBatch(glDirectDrawSurface7 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_BltBatch: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_BltFast(glDirectDrawSurface7 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
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
		dest.right = dwX + ((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwWidth;
		dest.bottom = dwY + ((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwHeight;
	}
	DWORD flags = 0;
	if(dwTrans & DDBLTFAST_WAIT) flags |= DDBLT_WAIT;
	if(dwTrans & DDBLTFAST_DESTCOLORKEY) flags |= DDBLT_KEYDEST;
	if(dwTrans & DDBLTFAST_SRCCOLORKEY) flags |= DDBLT_KEYSRC;
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_Blt(This,&dest,lpDDSrcSurface,lpSrcRect,flags,NULL));
}
HRESULT WINAPI glDirectDrawSurface7_DeleteAttachedSurface(glDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
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
HRESULT WINAPI glDirectDrawSurface7_EnumAttachedSurfaces(glDirectDrawSurface7 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT enumret = DDENUMRET_OK;
	if (This->backbuffer)
	{
		glDirectDrawSurface7_AddRef(This->backbuffer);
		enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)This->backbuffer, &This->backbuffer->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (This->zbuffer)
	{
		glDirectDrawSurface7_AddRef(This->zbuffer);
		enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)This->zbuffer, &This->zbuffer->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (This->miptexture)
	{
		glDirectDrawSurface7_AddRef(This->miptexture);
		enumret = lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE7)This->miptexture, &This->miptexture->ddsd, lpContext);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_EnumOverlayZOrders(glDirectDrawSurface7 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
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
				&((glDirectDrawSurface7*)This->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	else
	{
		for (i = This->overlaycount; i > 0; i--)
		{
			if (lpfnCallback((LPDIRECTDRAWSURFACE7)This->overlays[i].surface,
				&((glDirectDrawSurface7*)This->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	TRACE_RET(HRESULT, 23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_Flip(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glTexture *previous;
	HRESULT ret = glDirectDrawSurface7_Flip2(This,lpDDSurfaceTargetOverride,dwFlags,&previous);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(This->ddInterface->lastsync)
		{
			This->swapinterval++;
			This->ddInterface->lastsync = false;
		}
		glDirectDrawSurface7_RenderScreen(This,This->texture,This->swapinterval,previous,TRUE,NULL,0);
	}
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)
	{
		if (This->overlaydest) glDirectDrawSurface7_UpdateOverlayTexture(This->overlaydest, This, This->texture);
	}
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT glDirectDrawSurface7_Flip2(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags, glTexture **previous)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & 0xF8FFFFC0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	if(!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && ((dwFlags & DDFLIP_ODD) || (dwFlags & DDFLIP_EVEN))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD i;
	glDirectDrawSurface7 *tmp;
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
			tmp = tmp->backbuffer;
			if(lpDDSurfaceTargetOverride == (LPDIRECTDRAWSURFACE7)tmp)
			{
				success = TRUE;
				i++;
				break;
			}
		}
		if(!success) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		for(DWORD x = 0; x < i; x++)
		{
			if(x == i-1) {TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip2(This,NULL,dwFlags,NULL));}
			else glDirectDrawSurface7_Flip2(This,NULL,0,NULL);
		}
	}
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		glTexture **textures = new glTexture*[This->ddsd.dwBackBufferCount+1];
		textures[0] = This->texture;
		tmp = This;
		for(i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->backbuffer;
			textures[i+1] = tmp->texture;
		}
		glTexture *tmptex = textures[0];
		memmove(textures,&textures[1],This->ddsd.dwBackBufferCount*sizeof(glTexture*));
		textures[This->ddsd.dwBackBufferCount] = tmptex;
		tmp = This;
		glDirectDrawSurface7_SetTexture(This, textures[0]);
		if(This->palette && (This->texture->palette != This->palette->texture))
			glTexture_SetPalette(This->texture, This->palette->texture, FALSE);
		if(This->clipper && (This->texture->stencil != This->clipper->texture))
			glTexture_SetStencil(This->texture, This->clipper->texture, FALSE);
		for(DWORD i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->backbuffer;
			glDirectDrawSurface7_SetTexture(tmp, textures[i+1]);
			if (tmp->palette && (tmp->texture->palette != tmp->palette->texture))
				glTexture_SetPalette(tmp->texture, tmp->palette->texture, FALSE);
			if (tmp->clipper && (tmp->texture->stencil != tmp->clipper->texture))
				glTexture_SetStencil(tmp->texture, tmp->clipper->texture, FALSE);
		}
		delete[] textures;
	}
	else TRACE_RET(HRESULT,23,DDERR_NOTFLIPPABLE);
	This->flipcount+=flips;
	if(This->flipcount > This->ddsd.dwBackBufferCount)
		This->flipcount -= (This->ddsd.dwBackBufferCount+1);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_GetAttachedSurface(glDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSCAPS2 ddsComp;
	ZeroMemory(&ddsComp, sizeof(DDSCAPS2));
	unsigned __int64 comp1,comp2;
	if (This->backbuffer)
	{ 
		glDirectDrawSurface7_GetCaps(This->backbuffer, &ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)This->backbuffer;
			glDirectDrawSurface7_AddRef(This->backbuffer);
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	if (This->backbufferwraparound)
	{ 
		glDirectDrawSurface7_GetCaps(This->backbufferwraparound, &ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)This->backbufferwraparound;
			glDirectDrawSurface7_AddRef(This->backbufferwraparound);
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	if(This->zbuffer)
	{
		glDirectDrawSurface7_GetCaps(This->zbuffer, &ddsComp);
		memcpy(&comp1,lpDDSCaps,sizeof(unsigned __int64));
		memcpy(&comp2,&ddsComp,sizeof(unsigned __int64));
		if((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)This->zbuffer;
			glDirectDrawSurface7_AddRef(This->zbuffer);
			TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
	}
	if (This->miptexture)
	{
		glDirectDrawSurface7_GetCaps(This->miptexture, &ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE7)This->miptexture;
			glDirectDrawSurface7_AddRef(This->miptexture);
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}

	TRACE_EXIT(23,DDERR_NOTFOUND);
	return DDERR_NOTFOUND;
}
HRESULT WINAPI glDirectDrawSurface7_GetBltStatus(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT islost = glDirectDrawSurface7_IsLost(This);
	if (islost == DDERR_SURFACELOST) return DDERR_SURFACELOST;
	// Async rendering not yet implemented
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_GetCaps(glDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpDDSCaps,&This->ddsd.ddsCaps,sizeof(DDSCAPS2));
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_GetClipper(glDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetClipper: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetColorKey(glDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDColorKey) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags == DDCKEY_SRCBLT)
	{
		if(This->colorkey[0].enabled)
		{
			memcpy(lpDDColorKey,&This->colorkey[0].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTBLT)
	{
		if(This->colorkey[1].enabled)
		{
			memcpy(lpDDColorKey,&This->colorkey[1].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_SRCOVERLAY)
	{
		if(This->colorkey[2].enabled)
		{
			memcpy(lpDDColorKey,&This->colorkey[2].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTOVERLAY)
	{
		if(This->colorkey[3].enabled)
		{
			memcpy(lpDDColorKey,&This->colorkey[3].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirectDrawSurface7_GetDC(glDirectDrawSurface7 *This, HDC FAR *lphDC)
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
HRESULT WINAPI glDirectDrawSurface7_GetFlipStatus(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
	FIXME("glDirectDrawSurface7_GetFlipStatus: stub\n");
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetOverlayPosition(glDirectDrawSurface7 *This, LPLONG lplX, LPLONG lplY)
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
HRESULT WINAPI glDirectDrawSurface7_GetPalette(glDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
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
HRESULT WINAPI glDirectDrawSurface7_GetPixelFormat(glDirectDrawSurface7 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDPixelFormat) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lpDDPixelFormat = This->ddsd.ddpfPixelFormat;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_GetSurfaceDesc(glDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&lpDDSurfaceDesc->dwSize+1,&This->ddsd.dwSize+1,lpDDSurfaceDesc->dwSize-sizeof(DWORD)); // copy skipping first DWORD dwSize
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_Initialize(glDirectDrawSurface7 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface7_IsLost(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->hRC == This->ddInterface->renderer->hRC) {TRACE_RET(HRESULT,23,DD_OK);}
	else TRACE_RET(HRESULT,23,DDERR_SURFACELOST);
}

HRESULT WINAPI glDirectDrawSurface7_Lock(glDirectDrawSurface7 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
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
HRESULT WINAPI glDirectDrawSurface7_ReleaseDC(glDirectDrawSurface7 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!hDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error = glTexture_ReleaseDC(This->texture, This->miplevel, hDC);
	if (((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		if (This->ddInterface->lastsync)
		{
			glDirectDrawSurface7_RenderScreen(This, This->texture, 1, NULL, TRUE, NULL, 0);
			This->ddInterface->lastsync = false;
		}
		else glDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23,error);
	return error;
}
void glDirectDrawSurface7_Restore2(glDirectDrawSurface7 *This)
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
		if(This->backbuffer) glDirectDrawSurface7_Restore2(This->backbuffer);
		if(This->zbuffer) glDirectDrawSurface7_Restore2(This->zbuffer);
		if(This->paltex) glRenderer_MakeTexture(This->ddInterface->renderer,This->paltex,256,1);
		glRenderer_MakeTexture(This->ddInterface->renderer,This->texture,This->fakex,This->fakey);
	}*/
	TRACE_EXIT(0,0);
}
HRESULT WINAPI glDirectDrawSurface7_Restore(glDirectDrawSurface7 *This)
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
			if(This->backbuffer) glDirectDrawSurface7_Restore2(This->backbuffer);
			if(This->zbuffer) glDirectDrawSurface7_Restore2(This->zbuffer);
		}
		else
		{
			if(This->backbuffer) glDirectDrawSurface7_Restore(This->backbuffer);
			if(This->zbuffer) glDirectDrawSurface7_Restore(This->zbuffer);
		}
		if(This->paltex) glRenderer_MakeTexture(This->ddInterface->renderer,This->paltex,256,1);
		glRenderer_MakeTexture(This->ddInterface->renderer,This->texture,This->fakex,This->fakey);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else */TRACE_RET(HRESULT,23,DD_OK);
}
HRESULT WINAPI glDirectDrawSurface7_SetClipper(glDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
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
		if (!This->clipper->texture) glDirectDrawClipper_CreateTexture(This->clipper, This->texture, This->ddInterface->renderer);
		glTexture_SetStencil(This->texture, This->clipper->texture, FALSE);
	}
	else
	{
		glTexture_SetStencil(This->texture, NULL, FALSE);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_SetColorKey(glDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	CKEY key;
	if (lpDDColorKey) key.enabled = true;
	else key.enabled = false;
	if(dwFlags & DDCKEY_COLORSPACE) key.colorspace = true;
	else key.colorspace = false;
	if(lpDDColorKey) key.key = *lpDDColorKey;
	if(dwFlags & DDCKEY_SRCBLT)
	{
		This->colorkey[0] = key;
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKSRCBLT;
			This->ddsd.ddckCKSrcBlt = *lpDDColorKey;
			if (!key.colorspace) This->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else This->ddsd.dwFlags &= ~DDSD_CKSRCBLT;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_DESTBLT)
	{
		This->colorkey[1] = key;
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKDESTBLT;
			This->ddsd.ddckCKDestBlt = *lpDDColorKey;
			if (!key.colorspace) This->ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else This->ddsd.dwFlags &= ~DDSD_CKDESTBLT;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_SRCOVERLAY)
	{
		This->colorkey[2] = key;
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
			This->ddsd.ddckCKSrcOverlay = *lpDDColorKey;
			if (!key.colorspace) This->ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else This->ddsd.dwFlags &= ~DDSD_CKSRCOVERLAY;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
	}
	if(dwFlags & DDCKEY_DESTOVERLAY)
	{
		This->colorkey[3] = key;
		if (lpDDColorKey)
		{
			This->ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
			This->ddsd.ddckCKDestOverlay = *lpDDColorKey;
			if (!key.colorspace) This->ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else This->ddsd.dwFlags &= ~DDSD_CKDESTOVERLAY;
		glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey, This->miplevel);
		if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			glDirectDrawSurface7_RenderScreen(This, This->texture, 0, NULL, FALSE, 0, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_SetOverlayPosition(glDirectDrawSurface7 *This, LONG lX, LONG lY)
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
HRESULT WINAPI glDirectDrawSurface7_SetPalette(glDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette)
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
		if (!This->palette->texture) glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
		glTexture_SetPalette(This->texture, This->palette->texture, FALSE);
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
				glDirectDrawSurface7_RenderScreen(This, This->texture, dxglcfg.HackPaletteVsync, NULL, FALSE, NULL, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7_SetPaletteNoDraw(glDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette)
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
		if (!This->palette->texture) glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
		glTexture_SetPalette(This->texture, This->palette->texture, FALSE);
	}
	else
	{
		glTexture_SetPalette(This->texture, NULL, FALSE);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7_Unlock(glDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!This->locked)
	{
		This->texture->levels[This->miplevel].dirty |= 1;
		TRACE_RET(HRESULT, 23, DDERR_NOTLOCKED);
	}
	This->locked--;
	glTexture_Unlock(This->texture, This->miplevel, lpRect, FALSE);
	This->ddsd.lpSurface = NULL;
	if(((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		{
			if(This->ddInterface->lastsync)
			{
				glDirectDrawSurface7_RenderScreen(This,This->texture,1,NULL,TRUE,NULL,0);
				This->ddInterface->lastsync = false;
			}
			else glDirectDrawSurface7_RenderScreen(This,This->texture,0,NULL,TRUE,NULL,0);
		}
	if ((This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && This->overlayenabled)
	{
		if (This->ddInterface->lastsync)
		{
			glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture, 1, NULL, TRUE, NULL, 0);
			This->ddInterface->lastsync = false;
		}
		else glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlay(glDirectDrawSurface7 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) 
		TRACE_RET(HRESULT,23,DDERR_NOTAOVERLAYSURFACE);
	if (!(((glDirectDrawSurface7 *)lpDDDestSurface)->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
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
	This->overlaydest = (glDirectDrawSurface7 *)lpDDDestSurface;
	glDirectDrawSurface7_AddOverlay((glDirectDrawSurface7 *)lpDDDestSurface, &newoverlay);
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlayDisplay(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlayZOrder(glDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

extern "C" void glDirectDrawSurface7_RenderScreen2(LPDIRECTDRAWSURFACE7 surface, int vsync, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	glDirectDrawSurface7_RenderScreen((glDirectDrawSurface7*)surface, ((glDirectDrawSurface7*)surface)->texture, vsync, NULL, settime, overlays, overlaycount);
}

void glDirectDrawSurface7_RenderScreen(glDirectDrawSurface7 *This, glTexture *texture, int vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	TRACE_ENTER(3,14,This,14,texture,14,vsync);
	glRenderer_DrawScreen(This->ddInterface->renderer,texture, texture->palette, vsync, previous, settime, overlays, overlaycount);
	TRACE_EXIT(0,0);
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7_GetDDInterface(glDirectDrawSurface7 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7_AddRef(This->ddInterface);
	*lplpDD = This->ddInterface;
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_PageLock(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	This->pagelocked++;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_PageUnlock(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->pagelocked) TRACE_RET(HRESULT,23,DDERR_NOTPAGELOCKED);
	This->pagelocked--;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
// ddraw 3+ api
HRESULT WINAPI glDirectDrawSurface7_SetSurfaceDesc(glDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd2,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_SURFACEBUSY);
	FIXME("glDirectDrawSurface7_SetSurfaceDesc: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
// ddraw 4+ api
HRESULT WINAPI glDirectDrawSurface7_SetPrivateData(glDirectDrawSurface7 *This, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,This,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_SetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetPrivateData(glDirectDrawSurface7 *This, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,This,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_FreePrivateData(glDirectDrawSurface7 *This, REFGUID guidTag)
{
	TRACE_ENTER(2,14,This,24,&guidTag);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_FreePrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetUniquenessValue(glDirectDrawSurface7 *This, LPDWORD lpValue)
{
	TRACE_ENTER(2,14,This,14,lpValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_ChangeUniquenessValue(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_ChangeUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
// ddraw 7 api
HRESULT WINAPI glDirectDrawSurface7_SetPriority(glDirectDrawSurface7 *This, DWORD dwPriority)
{
	TRACE_ENTER(2,14,This,8,dwPriority);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_SetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetPriority(glDirectDrawSurface7 *This, LPDWORD lpdwPriority)
{
	TRACE_ENTER(2,14,This,14,lpdwPriority);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_SetLOD(glDirectDrawSurface7 *This, DWORD dwMaxLOD)
{
	TRACE_ENTER(2,14,This,8,dwMaxLOD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_SetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetLOD(glDirectDrawSurface7 *This, LPDWORD lpdwMaxLOD)
{
	TRACE_ENTER(2,14,This,14,lpdwMaxLOD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_Unlock2(glDirectDrawSurface7 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	// FIXME:  Get rect from pointer
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_Unlock(This, (LPRECT)lpSurfaceData));
}

HRESULT glDirectDrawSurface7_GetHandle(glDirectDrawSurface7 *This, glDirect3DDevice7 *glD3DDev7, LPD3DTEXTUREHANDLE lpHandle)
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

HRESULT glDirectDrawSurface7_Load(glDirectDrawSurface7 *This, glDirectDrawSurface7 *src)
{
	TRACE_ENTER(2,14,This,14,src);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!src) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (src == This) TRACE_RET(HRESULT, 23, DD_OK);
	glDirectDrawSurface7_Blt(This,NULL,(LPDIRECTDRAWSURFACE7)src,NULL,DDBLT_WAIT,NULL);
	if (src->ddsd.dwFlags & DDSD_CKSRCBLT)
		glDirectDrawSurface7_SetColorKey(This, DDCKEY_SRCBLT, &src->colorkey[0].key);
	This->ddsd.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirectDrawSurface7_GetGammaRamp(glDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetGammaRamp: stub\n");
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT glDirectDrawSurface7_SetGammaRamp(glDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_SetGammaRamp: stub\n");
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT glDirectDrawSurface7_AddOverlay(glDirectDrawSurface7 *This, OVERLAY *overlay)
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
				glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					1, NULL, TRUE, This->overlays, This->overlaycount);
				This->ddInterface->lastsync = false;
			}
			else glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
				0, NULL, TRUE, This->overlays, This->overlaycount);
			return DD_OK;
		}
	}
	This->overlays[This->overlaycount] = *overlay;
	glTexture_AddRef(This->overlays[This->overlaycount].texture);
	This->overlaycount++;
	if (This->ddInterface->lastsync)
	{
		glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
			1, NULL, TRUE, This->overlays, This->overlaycount);
		This->ddInterface->lastsync = false;
	}
	else glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
		0, NULL, TRUE, This->overlays, This->overlaycount);
	return DD_OK;
}

HRESULT glDirectDrawSurface7_DeleteOverlay(glDirectDrawSurface7 *This, glDirectDrawSurface7 *surface)
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
					glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
						1, NULL, TRUE, This->overlays, This->overlaycount);
					This->ddInterface->lastsync = false;
				}
				else glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					0, NULL, TRUE, This->overlays, This->overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
}

HRESULT glDirectDrawSurface7_UpdateOverlayTexture(glDirectDrawSurface7 *This, glDirectDrawSurface7 *surface, glTexture *texture)
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
					glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
						1, NULL, TRUE, This->overlays, This->overlaycount);
					This->ddInterface->lastsync = false;
				}
				else glDirectDrawSurface7_RenderScreen(This, This->ddInterface->primary->texture,
					0, NULL, TRUE, This->overlays, This->overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
}

void glDirectDrawSurface7_SetTexture(glDirectDrawSurface7 *This, glTexture *newtexture)
{
	This->texture = newtexture;
}

// DDRAW1 wrapper
glDirectDrawSurface1Vtbl glDirectDrawSurface1_impl =
{
	glDirectDrawSurface1_QueryInterface,
	glDirectDrawSurface1_AddRef,
	glDirectDrawSurface1_Release,
	glDirectDrawSurface1_AddAttachedSurface,
	glDirectDrawSurface1_AddOverlayDirtyRect,
	glDirectDrawSurface1_Blt,
	glDirectDrawSurface1_BltBatch,
	glDirectDrawSurface1_BltFast,
	glDirectDrawSurface1_DeleteAttachedSurface,
	glDirectDrawSurface1_EnumAttachedSurfaces,
	glDirectDrawSurface1_EnumOverlayZOrders,
	glDirectDrawSurface1_Flip,
	glDirectDrawSurface1_GetAttachedSurface,
	glDirectDrawSurface1_GetBltStatus,
	glDirectDrawSurface1_GetCaps,
	glDirectDrawSurface1_GetClipper,
	glDirectDrawSurface1_GetColorKey,
	glDirectDrawSurface1_GetDC,
	glDirectDrawSurface1_GetFlipStatus,
	glDirectDrawSurface1_GetOverlayPosition,
	glDirectDrawSurface1_GetPalette,
	glDirectDrawSurface1_GetPixelFormat,
	glDirectDrawSurface1_GetSurfaceDesc,
	glDirectDrawSurface1_Initialize,
	glDirectDrawSurface1_IsLost,
	glDirectDrawSurface1_Lock,
	glDirectDrawSurface1_ReleaseDC,
	glDirectDrawSurface1_Restore,
	glDirectDrawSurface1_SetClipper,
	glDirectDrawSurface1_SetColorKey,
	glDirectDrawSurface1_SetOverlayPosition,
	glDirectDrawSurface1_SetPalette,
	glDirectDrawSurface1_Unlock,
	glDirectDrawSurface1_UpdateOverlay,
	glDirectDrawSurface1_UpdateOverlayDisplay,
	glDirectDrawSurface1_UpdateOverlayZOrder
};
HRESULT glDirectDrawSurface1_Create(glDirectDrawSurface7 *gl_DDS7, glDirectDrawSurface1 *glDDS1)
{
	TRACE_ENTER(2,14,gl_DDS7, 14, glDDS1);
	glDDS1->lpVtbl = &glDirectDrawSurface1_impl;
	glDDS1->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface1_QueryInterface(glDirectDrawSurface1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirectDrawSurface1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface1_AddRef(glDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef1(This->glDDS7));
}
ULONG WINAPI glDirectDrawSurface1_Release(glDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release1(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1_AddAttachedSurface(glDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1_AddOverlayDirtyRect(glDirectDrawSurface1 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface1_Blt(glDirectDrawSurface1 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
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
			glDirectDrawSurface1 *pattern = (glDirectDrawSurface1*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, 
				(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface1_BltBatch(glDirectDrawSurface1 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1_BltFast(glDirectDrawSurface1 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_BltFast(This->glDDS7, dwX, dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwTrans));
}
HRESULT WINAPI glDirectDrawSurface1_DeleteAttachedSurface(glDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1_EnumAttachedSurfaces(glDirectDrawSurface1 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7,context,EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface1_EnumOverlayZOrders(glDirectDrawSurface1 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface1_Flip(glDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1_GetAttachedSurface(glDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface1 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface,(void **)&attached1);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface1_GetBltStatus(glDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1_GetCaps(glDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	ZeroMemory(&ddsCaps1, sizeof(DDSCAPS2));
	error = glDirectDrawSurface7_GetCaps(This->glDDS7,&ddsCaps1);
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface1_GetClipper(glDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetClipper(This->glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1_GetColorKey(glDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1_GetDC(glDirectDrawSurface1 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(This->glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface1_GetFlipStatus(glDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1_GetOverlayPosition(glDirectDrawSurface1 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface1_GetPalette(glDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(This->glDDS7,lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1_GetPixelFormat(glDirectDrawSurface1 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(This->glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface1_GetSurfaceDesc(glDirectDrawSurface1 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(This->glDDS7,&ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1_Initialize(glDirectDrawSurface1 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface1_IsLost(glDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1_Lock(glDirectDrawSurface1 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1_ReleaseDC(glDirectDrawSurface1 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI glDirectDrawSurface1_Restore(glDirectDrawSurface1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1_SetClipper(glDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1_SetColorKey(glDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1_SetOverlayPosition(glDirectDrawSurface1 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface1_SetPalette(glDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(This->glDDS7,lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1_Unlock(glDirectDrawSurface1 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(This->glDDS7,lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface1_UpdateOverlay(glDirectDrawSurface1 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface1_UpdateOverlayDisplay(glDirectDrawSurface1 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1_UpdateOverlayZOrder(glDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7, dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}

// DDRAW2 wrapper
glDirectDrawSurface2Vtbl glDirectDrawSurface2_impl =
{
	glDirectDrawSurface2_QueryInterface,
	glDirectDrawSurface2_AddRef,
	glDirectDrawSurface2_Release,
	glDirectDrawSurface2_AddAttachedSurface,
	glDirectDrawSurface2_AddOverlayDirtyRect,
	glDirectDrawSurface2_Blt,
	glDirectDrawSurface2_BltBatch,
	glDirectDrawSurface2_BltFast,
	glDirectDrawSurface2_DeleteAttachedSurface,
	glDirectDrawSurface2_EnumAttachedSurfaces,
	glDirectDrawSurface2_EnumOverlayZOrders,
	glDirectDrawSurface2_Flip,
	glDirectDrawSurface2_GetAttachedSurface,
	glDirectDrawSurface2_GetBltStatus,
	glDirectDrawSurface2_GetCaps,
	glDirectDrawSurface2_GetClipper,
	glDirectDrawSurface2_GetColorKey,
	glDirectDrawSurface2_GetDC,
	glDirectDrawSurface2_GetFlipStatus,
	glDirectDrawSurface2_GetOverlayPosition,
	glDirectDrawSurface2_GetPalette,
	glDirectDrawSurface2_GetPixelFormat,
	glDirectDrawSurface2_GetSurfaceDesc,
	glDirectDrawSurface2_Initialize,
	glDirectDrawSurface2_IsLost,
	glDirectDrawSurface2_Lock,
	glDirectDrawSurface2_ReleaseDC,
	glDirectDrawSurface2_Restore,
	glDirectDrawSurface2_SetClipper,
	glDirectDrawSurface2_SetColorKey,
	glDirectDrawSurface2_SetOverlayPosition,
	glDirectDrawSurface2_SetPalette,
	glDirectDrawSurface2_Unlock,
	glDirectDrawSurface2_UpdateOverlay,
	glDirectDrawSurface2_UpdateOverlayDisplay,
	glDirectDrawSurface2_UpdateOverlayZOrder,
	glDirectDrawSurface2_GetDDInterface,
	glDirectDrawSurface2_PageLock,
	glDirectDrawSurface2_PageUnlock
};

HRESULT glDirectDrawSurface2_Create(glDirectDrawSurface7 *gl_DDS7, glDirectDrawSurface2 *glDDS2)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS2);
	glDDS2->lpVtbl = &glDirectDrawSurface2_impl;
	glDDS2->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface2_QueryInterface(glDirectDrawSurface2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirectDrawSurface2_AddRef(This);
		*ppvObj = This;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface2_AddRef(glDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef2(This->glDDS7));
}
ULONG WINAPI glDirectDrawSurface2_Release(glDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release2(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2_AddAttachedSurface(glDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2_AddOverlayDirtyRect(glDirectDrawSurface2 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface2_Blt(glDirectDrawSurface2 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
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
			glDirectDrawSurface2 *pattern = (glDirectDrawSurface2*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface2_BltBatch(glDirectDrawSurface2 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_BltFast(glDirectDrawSurface2 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,4,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface2_DeleteAttachedSurface(glDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2_EnumAttachedSurfaces(glDirectDrawSurface2 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface2_EnumOverlayZOrders(glDirectDrawSurface2 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface2_Flip(glDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_GetAttachedSurface(glDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface2 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface2,(void **)&attached1);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE2)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2_GetBltStatus(glDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_GetCaps(glDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = glDirectDrawSurface7_GetCaps(This->glDDS7, &ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2_GetClipper(glDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(This->glDDS7, lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2_GetColorKey(glDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(This->glDDS7, dwFlags, lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2_GetDC(glDirectDrawSurface2 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(This->glDDS7, lphDC));
}
HRESULT WINAPI glDirectDrawSurface2_GetFlipStatus(glDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_GetOverlayPosition(glDirectDrawSurface2 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface2_GetPalette(glDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2_GetPixelFormat(glDirectDrawSurface2 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface2_GetSurfaceDesc(glDirectDrawSurface2 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, &ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2_Initialize(glDirectDrawSurface2 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface2_IsLost(glDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2_Lock(glDirectDrawSurface2 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2_ReleaseDC(glDirectDrawSurface2 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI glDirectDrawSurface2_Restore(glDirectDrawSurface2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2_SetClipper(glDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2_SetColorKey(glDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2_SetOverlayPosition(glDirectDrawSurface2 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface2_SetPalette(glDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2_Unlock(glDirectDrawSurface2 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(This->glDDS7, lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface2_UpdateOverlay(glDirectDrawSurface2 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(This->glDDS7, lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface2_UpdateOverlayDisplay(glDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_UpdateOverlayZOrder(glDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface2_GetDDInterface(glDirectDrawSurface2 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDirectDrawSurface7_GetDDInterface(This->glDDS7, (void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw,lplpDD);
	glDirectDraw7_Release(glDD7);
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_RET(HRESULT,23,ret);
}
HRESULT WINAPI glDirectDrawSurface2_PageLock(glDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2_PageUnlock(glDirectDrawSurface2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}

// DDRAW3 wrapper
glDirectDrawSurface3Vtbl glDirectDrawSurface3_impl =
{
	glDirectDrawSurface3_QueryInterface,
	glDirectDrawSurface3_AddRef,
	glDirectDrawSurface3_Release,
	glDirectDrawSurface3_AddAttachedSurface,
	glDirectDrawSurface3_AddOverlayDirtyRect,
	glDirectDrawSurface3_Blt,
	glDirectDrawSurface3_BltBatch,
	glDirectDrawSurface3_BltFast,
	glDirectDrawSurface3_DeleteAttachedSurface,
	glDirectDrawSurface3_EnumAttachedSurfaces,
	glDirectDrawSurface3_EnumOverlayZOrders,
	glDirectDrawSurface3_Flip,
	glDirectDrawSurface3_GetAttachedSurface,
	glDirectDrawSurface3_GetBltStatus,
	glDirectDrawSurface3_GetCaps,
	glDirectDrawSurface3_GetClipper,
	glDirectDrawSurface3_GetColorKey,
	glDirectDrawSurface3_GetDC,
	glDirectDrawSurface3_GetFlipStatus,
	glDirectDrawSurface3_GetOverlayPosition,
	glDirectDrawSurface3_GetPalette,
	glDirectDrawSurface3_GetPixelFormat,
	glDirectDrawSurface3_GetSurfaceDesc,
	glDirectDrawSurface3_Initialize,
	glDirectDrawSurface3_IsLost,
	glDirectDrawSurface3_Lock,
	glDirectDrawSurface3_ReleaseDC,
	glDirectDrawSurface3_Restore,
	glDirectDrawSurface3_SetClipper,
	glDirectDrawSurface3_SetColorKey,
	glDirectDrawSurface3_SetOverlayPosition,
	glDirectDrawSurface3_SetPalette,
	glDirectDrawSurface3_Unlock,
	glDirectDrawSurface3_UpdateOverlay,
	glDirectDrawSurface3_UpdateOverlayDisplay,
	glDirectDrawSurface3_UpdateOverlayZOrder,
	glDirectDrawSurface3_GetDDInterface,
	glDirectDrawSurface3_PageLock,
	glDirectDrawSurface3_PageUnlock,
	glDirectDrawSurface3_SetSurfaceDesc
};

HRESULT glDirectDrawSurface3_Create(glDirectDrawSurface7 *gl_DDS7, glDirectDrawSurface3 *glDDS3)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS3);
	glDDS3->lpVtbl = &glDirectDrawSurface3_impl;
	glDDS3->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface3_QueryInterface(glDirectDrawSurface3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirectDrawSurface3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface3_AddRef(glDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if (!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef3(This->glDDS7));
}
ULONG WINAPI glDirectDrawSurface3_Release(glDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release3(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3_AddAttachedSurface(glDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3_AddOverlayDirtyRect(glDirectDrawSurface3 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface3_Blt(glDirectDrawSurface3 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
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
			glDirectDrawSurface3 *pattern = (glDirectDrawSurface3*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(This->glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface3_BltBatch(glDirectDrawSurface3 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_BltFast(glDirectDrawSurface3 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface3_DeleteAttachedSurface(glDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3_EnumAttachedSurfaces(glDirectDrawSurface3 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface3_EnumOverlayZOrders(glDirectDrawSurface3 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface3_Flip(glDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_GetAttachedSurface(glDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface3 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(This->glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface3,(void **)&attached1);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE3)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3_GetBltStatus(glDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_GetCaps(glDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = glDirectDrawSurface7_GetCaps(This->glDDS7, &ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3_GetClipper(glDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(This->glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3_GetColorKey(glDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3_GetDC(glDirectDrawSurface3 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(This->glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface3_GetFlipStatus(glDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(This->glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_GetOverlayPosition(glDirectDrawSurface3 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface3_GetPalette(glDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3_GetPixelFormat(glDirectDrawSurface3 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface3_GetSurfaceDesc(glDirectDrawSurface3 *This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, &ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3_Initialize(glDirectDrawSurface3 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface3_IsLost(glDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3_Lock(glDirectDrawSurface3 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_Lock(This->glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3_ReleaseDC(glDirectDrawSurface3 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI glDirectDrawSurface3_Restore(glDirectDrawSurface3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3_SetClipper(glDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3_SetColorKey(glDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3_SetOverlayPosition(glDirectDrawSurface3 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface3_SetPalette(glDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3_Unlock(glDirectDrawSurface3 *This, LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,This,14,lpSurfaceData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(This->glDDS7, lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface3_UpdateOverlay(glDirectDrawSurface3 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface3_UpdateOverlayDisplay(glDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_UpdateOverlayZOrder(glDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface3_GetDDInterface(glDirectDrawSurface3 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDirectDrawSurface7_GetDDInterface(This->glDDS7, (void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw,lplpDD);
	glDirectDraw7_Release(glDD7);
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3_PageLock(glDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,14,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_PageUnlock(glDirectDrawSurface3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3_SetSurfaceDesc(glDirectDrawSurface3 *This, LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetSurfaceDesc(This->glDDS7,(LPDDSURFACEDESC2)lpddsd,dwFlags));
}

// DDRAW4 wrapper
glDirectDrawSurface4Vtbl glDirectDrawSurface4_impl =
{
	glDirectDrawSurface4_QueryInterface,
	glDirectDrawSurface4_AddRef,
	glDirectDrawSurface4_Release,
	glDirectDrawSurface4_AddAttachedSurface,
	glDirectDrawSurface4_AddOverlayDirtyRect,
	glDirectDrawSurface4_Blt,
	glDirectDrawSurface4_BltBatch,
	glDirectDrawSurface4_BltFast,
	glDirectDrawSurface4_DeleteAttachedSurface,
	glDirectDrawSurface4_EnumAttachedSurfaces,
	glDirectDrawSurface4_EnumOverlayZOrders,
	glDirectDrawSurface4_Flip,
	glDirectDrawSurface4_GetAttachedSurface,
	glDirectDrawSurface4_GetBltStatus,
	glDirectDrawSurface4_GetCaps,
	glDirectDrawSurface4_GetClipper,
	glDirectDrawSurface4_GetColorKey,
	glDirectDrawSurface4_GetDC,
	glDirectDrawSurface4_GetFlipStatus,
	glDirectDrawSurface4_GetOverlayPosition,
	glDirectDrawSurface4_GetPalette,
	glDirectDrawSurface4_GetPixelFormat,
	glDirectDrawSurface4_GetSurfaceDesc,
	glDirectDrawSurface4_Initialize,
	glDirectDrawSurface4_IsLost,
	glDirectDrawSurface4_Lock,
	glDirectDrawSurface4_ReleaseDC,
	glDirectDrawSurface4_Restore,
	glDirectDrawSurface4_SetClipper,
	glDirectDrawSurface4_SetColorKey,
	glDirectDrawSurface4_SetOverlayPosition,
	glDirectDrawSurface4_SetPalette,
	glDirectDrawSurface4_Unlock,
	glDirectDrawSurface4_UpdateOverlay,
	glDirectDrawSurface4_UpdateOverlayDisplay,
	glDirectDrawSurface4_UpdateOverlayZOrder,
	glDirectDrawSurface4_GetDDInterface,
	glDirectDrawSurface4_PageLock,
	glDirectDrawSurface4_PageUnlock,
	glDirectDrawSurface4_SetSurfaceDesc,
	glDirectDrawSurface4_SetPrivateData,
	glDirectDrawSurface4_GetPrivateData,
	glDirectDrawSurface4_FreePrivateData,
	glDirectDrawSurface4_GetUniquenessValue,
	glDirectDrawSurface4_ChangeUniquenessValue
};

HRESULT glDirectDrawSurface4_Create(glDirectDrawSurface7 *gl_DDS7, glDirectDrawSurface4 *glDDS4)
{
	TRACE_ENTER(2, 14, gl_DDS7, 14, glDDS4);
	glDDS4->lpVtbl = &glDirectDrawSurface4_impl;
	glDDS4->glDDS7 = gl_DDS7;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface4_QueryInterface(glDirectDrawSurface4 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirectDrawSurface4_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface4_AddRef(glDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef4(This->glDDS7));
}
ULONG WINAPI glDirectDrawSurface4_Release(glDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release4(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4_AddAttachedSurface(glDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, This, 14, lpDDSAttachedSurface);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(This->glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4_AddOverlayDirtyRect(glDirectDrawSurface4 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(This->glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface4_Blt(glDirectDrawSurface4 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
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
			glDirectDrawSurface4 *pattern = (glDirectDrawSurface4*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->glDDS7;
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
				(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->glDDS7, lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(This->glDDS7, lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface4_BltBatch(glDirectDrawSurface4 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltBatch(This->glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_BltFast(glDirectDrawSurface4 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,This,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(This->glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->glDDS7,lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface4_DeleteAttachedSurface(glDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(This->glDDS7, dwFlags,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSAttachedSurface)->glDDS7);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4_EnumAttachedSurfaces(glDirectDrawSurface4 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,This,14,lpContext,14,lpEnumSurfacesCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(This->glDDS7, context, EnumSurfacesCallback2));
}
HRESULT WINAPI glDirectDrawSurface4_EnumOverlayZOrders(glDirectDrawSurface4 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(This->glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface4_Flip(glDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSurfaceTargetOverride)->glDDS7,dwFlags));}
	else TRACE_RET(HRESULT,23,glDirectDrawSurface7_Flip(This->glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_GetAttachedSurface(glDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,This,14,lpDDSCaps2,14,lplpDDAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface4 *attached1;
	error = glDirectDrawSurface7_GetAttachedSurface(This->glDDS7,lpDDSCaps2,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface4,(void **)&attached1);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = (LPDIRECTDRAWSURFACE4)attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface4_GetBltStatus(glDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_GetCaps(glDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,This,14,lpDDSCaps);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetCaps(This->glDDS7, lpDDSCaps));
}
HRESULT WINAPI glDirectDrawSurface4_GetClipper(glDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lplpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(This->glDDS7, lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4_GetColorKey(glDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4_GetDC(glDirectDrawSurface4 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(This->glDDS7, lphDC));
}
HRESULT WINAPI glDirectDrawSurface4_GetFlipStatus(glDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_GetOverlayPosition(glDirectDrawSurface4 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(This->glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface4_GetPalette(glDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(This->glDDS7, lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4_GetPixelFormat(glDirectDrawSurface4 *This, LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,This,14,lpDDPixelFormat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(This->glDDS7, lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface4_GetSurfaceDesc(glDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,This,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetSurfaceDesc(This->glDDS7, lpDDSurfaceDesc));
}
HRESULT WINAPI glDirectDrawSurface4_Initialize(glDirectDrawSurface4 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,This,14,lpDD,14,lpDDSurfaceDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface4_IsLost(glDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4_Lock(glDirectDrawSurface4 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Lock(This->glDDS7,lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent));
}
HRESULT WINAPI glDirectDrawSurface4_ReleaseDC(glDirectDrawSurface4 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(This->glDDS7, hDC));
}
HRESULT WINAPI glDirectDrawSurface4_Restore(glDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(This->glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4_SetClipper(glDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetClipper(This->glDDS7, lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4_SetColorKey(glDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(This->glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4_SetOverlayPosition(glDirectDrawSurface4 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(This->glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface4_SetPalette(glDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(This->glDDS7, lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4_Unlock(glDirectDrawSurface4 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(This->glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface4_UpdateOverlay(glDirectDrawSurface4 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(This->glDDS7,lpSrcRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface4_UpdateOverlayDisplay(glDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_UpdateOverlayZOrder(glDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(This->glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface4_GetDDInterface(glDirectDrawSurface4 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDDInterface(This->glDDS7,lplpDD));
}
HRESULT WINAPI glDirectDrawSurface4_PageLock(glDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_PageLock(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_PageUnlock(glDirectDrawSurface4 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageUnlock(This->glDDS7, dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_SetSurfaceDesc(glDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpddsd,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetSurfaceDesc(This->glDDS7,lpddsd,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_SetPrivateData(glDirectDrawSurface4 *This, REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,This,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPrivateData(This->glDDS7,guidTag,lpData,cbSize,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4_GetPrivateData(glDirectDrawSurface4 *This, REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,This,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPrivateData(This->glDDS7,guidTag,lpBuffer,lpcbBufferSize));
}
HRESULT WINAPI glDirectDrawSurface4_FreePrivateData(glDirectDrawSurface4 *This, REFGUID guidTag)
{
	TRACE_ENTER(2,14,This,24,&guidTag);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_FreePrivateData(This->glDDS7, guidTag));
}
HRESULT WINAPI glDirectDrawSurface4_GetUniquenessValue(glDirectDrawSurface4 *This, LPDWORD lpValue)
{
	TRACE_ENTER(2,14,This,14,lpValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetUniquenessValue(This->glDDS7, lpValue));
}
HRESULT WINAPI glDirectDrawSurface4_ChangeUniquenessValue(glDirectDrawSurface4 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ChangeUniquenessValue(This->glDDS7));
}
