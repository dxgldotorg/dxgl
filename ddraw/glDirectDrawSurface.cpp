// DXGL
// Copyright (C) 2011-2015 William Feely

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

glDirectDrawSurface7Vtbl glDirectDrawSurface7_iface =
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

// Create DDraw7 surface
HRESULT glDirectDrawSurface7_Create(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 *lplpDDSurface, glDirectDrawPalette *palettein,
	glTexture *parenttex, DWORD miplevel, int version, glDirectDrawSurface7 *front)
{
	TRACE_ENTER(8, 14, lpDD7, 14, lpDDSurfaceDesc2, 14, lplpDDSurface, 14, palettein, 14, parenttex, 8, miplevel, 11, version, 14, front);
	if (!lpDDSurfaceDesc2 || (lpDDSurfaceDesc2->dwSize != sizeof(DDSURFACEDESC2))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (!lplpDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirectDrawSurface7 *surface = (glDirectDrawSurface7*)malloc(sizeof(glDirectDrawSurface7));
	ZeroMemory(surface, sizeof(glDirectDrawSurface7));
	surface->version = version;
	surface->dds1 = new glDirectDrawSurface1(surface);
	surface->dds2 = new glDirectDrawSurface2(surface);
	surface->dds3 = new glDirectDrawSurface3(surface);
	surface->dds4 = new glDirectDrawSurface4(surface);
	surface->d3dt2 = new glDirect3DTexture2(surface);
	surface->d3dt1 = new glDirect3DTexture1(surface);
	glDirectDrawGammaControl_Create(surface, (LPDIRECTDRAWGAMMACONTROL*)&surface->gammacontrol);
	surface->miplevel = miplevel;
	surface->hdc = NULL;
	DWORD colormasks[3];
	surface->ddInterface = (glDirectDraw7 *)lpDD7;
	surface->ddsd = *lpDDSurfaceDesc2;
	surface->ddsd.dwFlags |= DDSD_CAPS;
	LONG sizes[6];
	surface->ddInterface->GetSizes(sizes);
	if(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(((surface->ddsd.dwFlags & DDSD_WIDTH) || (surface->ddsd.dwFlags & DDSD_HEIGHT)
			|| (surface->ddsd.dwFlags & DDSD_PIXELFORMAT)) && !(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
		{
			free(surface);
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		}
		else
		{
			if(surface->ddInterface->GetFullscreen())
			{
				surface->ddsd.dwWidth = sizes[2];
				surface->ddsd.dwHeight = sizes[3];
				if(dxglcfg.primaryscale)  // FIXME:  Support new scaling modes
				{
					surface->fakex = sizes[0];
					surface->fakey = sizes[1];
				}
				else
				{
					surface->fakex = surface->ddsd.dwWidth;
					surface->fakey = surface->ddsd.dwHeight;
				}
				surface->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
			else
			{
				surface->fakex = surface->ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
				surface->fakey = surface->ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				surface->ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
		}
	}
	else
	{
		if((surface->ddsd.dwFlags & DDSD_WIDTH) && (surface->ddsd.dwFlags & DDSD_HEIGHT))
		{
			surface->fakex = surface->ddsd.dwWidth;
			surface->fakey = surface->ddsd.dwHeight;
		}
		else
		{
			free(surface);
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		}
	}
/*	if(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	{
		BITMAPINFO info;
		ZeroMemory(&info,sizeof(BITMAPINFO));
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			info.bmiHeader.biWidth = fakex;
			info.bmiHeader.biHeight = -(signed)fakey;
			info.bmiHeader.biCompression = BI_RGB;
			info.bmiHeader.biSizeImage = 0;
			info.bmiHeader.biXPelsPerMeter = 0;
			info.bmiHeader.biYPelsPerMeter = 0;
			info.bmiHeader.biClrImportant = 0;
			info.bmiHeader.biClrUsed = 0;
			info.bmiHeader.biBitCount = (WORD)ddInterface->GetBPPMultipleOf8();
			*bitmapinfo = info;
		}
		else
		{
			if(ddsd.dwFlags & DDSD_PIXELFORMAT) surfacetype=2;
			else
			{
				info.bmiHeader.biWidth = fakex;
				info.bmiHeader.biHeight = -(signed)fakey;
				info.bmiHeader.biCompression = BI_RGB;
				info.bmiHeader.biSizeImage = 0;
				info.bmiHeader.biXPelsPerMeter = 0;
				info.bmiHeader.biYPelsPerMeter = 0;
				info.bmiHeader.biClrImportant = 0;
				info.bmiHeader.biClrUsed = 0;
				info.bmiHeader.biBitCount = (WORD)ddInterface->GetBPPMultipleOf8();
				*bitmapinfo = info;
			}
		}
	}
	else
	{
		bitmapinfo->bmiHeader.biSizeImage = 0;
		bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
		bitmapinfo->bmiHeader.biYPelsPerMeter = 0;
		bitmapinfo->bmiHeader.biClrImportant = 0;
		bitmapinfo->bmiHeader.biClrUsed = 0;
		bitmapinfo->bmiHeader.biCompression = BI_RGB;
		bitmapinfo->bmiHeader.biBitCount = 0;
	}
	surfacetype=2;
	bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapinfo->bmiHeader.biWidth = ddsd.dwWidth;
	bitmapinfo->bmiHeader.biHeight = -(signed)ddsd.dwHeight;
	bitmapinfo->bmiHeader.biPlanes = 1;*/
	surface->backbuffer = NULL;
	surface->backbufferwraparound = NULL;
	if ((surface->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && !(surface->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if (!(surface->ddsd.dwFlags & DDSD_MIPMAPCOUNT))
		{
			surface->ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
			surface->ddsd.dwMipMapCount = CalculateMipLevels(surface->ddsd.dwWidth, surface->ddsd.dwHeight);
		}
	}
	if (surface->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
	{
		surface->texture = parenttex;
		glTexture_AddRef(surface->texture);
	}
	else glRenderer_MakeTexture(surface->ddInterface->renderer, &surface->texture, &surface->ddsd, surface->fakex, surface->fakey);
	if (surface->ddInterface->GetBPP() == 8)
	{
		if (!palettein)
		{
			glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&surface->palette);
			glDirectDrawSurface7_SetPalette(surface, (LPDIRECTDRAWPALETTE)surface->palette);
		}
		else
		{
			glDirectDrawSurface7_SetPalette(surface, (LPDIRECTDRAWPALETTE)palettein);
		}
	}
	if ((dxglcfg.scalingfilter == 0) || (surface->ddInterface->GetBPP() == 8))
		glRenderer_SetTextureFilter(surface->ddInterface->renderer, surface->texture, GL_NEAREST, GL_NEAREST);
	else glRenderer_SetTextureFilter(surface->ddInterface->renderer, surface->texture, GL_LINEAR, GL_LINEAR);
	if(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
	{
		surface->ddsd.dwFlags |= DDSD_PIXELFORMAT;
		surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
		if(!surface->ddsd.ddpfPixelFormat.dwZBufferBitDepth)
			surface->ddsd.ddpfPixelFormat.dwZBufferBitDepth = surface->ddsd.dwRefreshRate;
		switch(surface->ddsd.ddpfPixelFormat.dwZBufferBitDepth)
		{
		case 8:
			surface->ddsd.ddpfPixelFormat.dwZBitMask = 0xFF;
			break;
		case 16:
			surface->ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFF;
			break;
		case 24:
			surface->ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFF;
			break;
		case 32:
		default:
			surface->ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFFFF;
			break;
		}
	}
	if(!(surface->ddsd.dwFlags & DDSD_PIXELFORMAT))
	{
		ZeroMemory(&surface->ddsd.ddpfPixelFormat,sizeof(DDPIXELFORMAT));
		surface->ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		surface->ddsd.ddpfPixelFormat.dwRGBBitCount = surface->ddInterface->GetBPP();
		switch(surface->ddInterface->GetBPP())
		{
		case 8:
			surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			surface->ddsd.ddpfPixelFormat.dwRBitMask = 0;
			surface->ddsd.ddpfPixelFormat.dwGBitMask = 0;
			surface->ddsd.ddpfPixelFormat.dwBBitMask = 0;
			surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth);
			break;
		case 15:
			surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			surface->ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
			surface->ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
			surface->ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth*2);
			surface->ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
			break;
		case 16:
			surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			surface->ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
			surface->ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
			surface->ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
			surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth*2);
			break;
		case 24:
			surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			surface->ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			surface->ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
			surface->ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
			surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth*3);
			break;
		case 32:
			surface->ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			surface->ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			surface->ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
			surface->ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
			surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth*4);
			break;
		default:
			free(surface);
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPIXELFORMAT);
		}
	}
	else surface->ddsd.lPitch = NextMultipleOf4(surface->ddsd.dwWidth*(surface->ddsd.ddpfPixelFormat.dwRGBBitCount / 8));
	if (!(surface->ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if ((surface->ddsd.dwFlags & DDSD_CAPS) && (surface->ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
			glRenderer_SetTextureFilter(surface->ddInterface->renderer, surface->texture, GL_NEAREST, GL_NEAREST);
		else
		{
			if (dxglcfg.scalingfilter && (surface->ddInterface->GetBPP() > 8))
				glRenderer_SetTextureFilter(surface->ddInterface->renderer, surface->texture, GL_LINEAR, GL_LINEAR);
			else glRenderer_SetTextureFilter(surface->ddInterface->renderer, surface->texture, GL_NEAREST, GL_NEAREST);
		}
		glRenderer_SetTextureWrap(surface->ddInterface->renderer, surface->texture, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		/*if (surface->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) texture->miplevel = ddsd.dwMipMapCount;
		else texture->miplevel = 1;*/
	}
	if ((surface->ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && surface->ddsd.dwMipMapCount)
	{
		DDSURFACEDESC2 newdesc = surface->ddsd;
		newdesc.dwWidth = max(1, (DWORD)floorf((float)surface->ddsd.dwWidth / 2.0f));
		newdesc.dwHeight = max(1, (DWORD)floorf((float)surface->ddsd.dwHeight / 2.0f));
		newdesc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
		newdesc.dwMipMapCount = surface->ddsd.dwMipMapCount - 1;
		HRESULT miperror;
		if(newdesc.dwMipMapCount)
			miperror = glDirectDrawSurface7_Create(lpDD7, &newdesc, (LPDIRECTDRAWSURFACE7*)&surface->miptexture,
				palettein, surface->texture, miplevel + 1, version, NULL);
	}

	/*if(ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
	{
		colormasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
		colormasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
		colormasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(bitmapinfo->bmiColors,colormasks,3*sizeof(DWORD));
	}
	if(!bitmapinfo->bmiHeader.biBitCount)
		bitmapinfo->bmiHeader.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;*/
	surface->refcount7 = 1;
	surface->refcount4 = 0;
	surface->refcount3 = 0;
	surface->refcount2 = 0;
	surface->refcount1 = 0;
	surface->refcountgamma = 0;
	surface->refcountcolor = 0;
	surface->mulx = (float)surface->fakex / (float)surface->ddsd.dwWidth;
	surface->muly = (float)surface->fakey / (float)surface->ddsd.dwHeight;
	if(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
	{
		if(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
		{
			if((surface->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && (surface->ddsd.dwBackBufferCount > 0))
			{
				if(!(surface->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
					surface->ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
				DDSURFACEDESC2 ddsdBack;
				memcpy(&ddsdBack,&surface->ddsd,surface->ddsd.dwSize);
				ddsdBack.dwBackBufferCount--;
				ddsdBack.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
				ddsdBack.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				glDirectDrawSurface7_Create(surface->ddInterface, &ddsdBack, (LPDIRECTDRAWSURFACE7*)&surface->backbuffer,
					surface->palette, parenttex, miplevel, version, front ? front : surface);
			}
			else if (surface->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
			{
				surface->backbufferwraparound = front;
			}
			else
			{
				glDirectDrawSurface7_Release(surface);
				TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			}
		}
	}
	switch (version)
	{
	case 1:
	case 2:
	case 3:
	default:
		surface->textureparent = surface->dds1;
		break;
	case 4:
		surface->textureparent = surface->dds4;
		break;
	case 7:
		surface->textureparent = (IUnknown*)surface;
		break;
	}
	surface->lpVtbl = &glDirectDrawSurface7_iface;
	*lplpDDSurface = (LPDIRECTDRAWSURFACE7)surface;
	if ((surface->ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && (surface->ddsd.dwBackBufferCount > 0))
	{
		int backcount = surface->ddsd.dwBackBufferCount;
		glDirectDrawSurface7 *front = surface;
		front->fliplist = (glTexture**)malloc((surface->ddsd.dwBackBufferCount + 1)*sizeof(glTexture*));
		for (int i = 0; i < backcount + 1; i++)
		{
			front->fliplist[i] = surface->texture;
			if (i < backcount) surface = surface->backbuffer;
		}
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
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
		*ppvObj = This->dds4;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		glDirectDrawSurface7_AddRef3(This);
		*ppvObj = This->dds3;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		glDirectDrawSurface7_AddRef2(This);
		*ppvObj = This->dds2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface)
	{
		glDirectDrawSurface7_AddRef1(This);
		*ppvObj = This->dds1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture2)
	{
		This->d3dt2->AddRef();
		*ppvObj = This->d3dt2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture)
	{
		This->d3dt1->AddRef();
		*ppvObj = This->d3dt1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if (riid == IID_IDirectDrawGammaControl)
	{
		glDirectDrawSurface7_AddRefGamma(This);
		*ppvObj = This->gammacontrol;
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
			This->ddInterface->QueryInterface(IID_IDirect3D7,(void**)&tmpd3d);
			if(!tmpd3d) TRACE_RET(HRESULT,23,E_NOINTERFACE);
			This->device1 = new glDirect3DDevice7(riid, tmpd3d, This, This->dds1, 1);
			if (FAILED(This->device1->err()))
			{
				ret = This->device1->err();
				delete This->device1;
				tmpd3d->Release();
				TRACE_EXIT(23, ret);
				return ret;
			}
			*ppvObj = This->device1->glD3DDev1;
			This->device1->glD3DDev1->AddRef();
			This->device1->InitDX5();
			This->device1->InitDX2();
			tmpd3d->Release();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else
		{
			*ppvObj = This->device1->glD3DDev1;
			This->device1->glD3DDev1->AddRef();
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
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcount7);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_Release(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount7 == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcount7);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
void glDirectDrawSurface7_Destroy(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) {
		TRACE_EXIT(0, 0);
		return;
	}
	glDirectDrawSurface7_AddRef(This);
	if (This->palette) glDirectDrawPalette_Release(This->palette);
	if (This->clipper) glDirectDrawClipper_Release(This->clipper);
	if (This->dds1) delete This->dds1;
	if (This->dds2) delete This->dds2;
	if (This->dds3) delete This->dds3;
	if (This->dds4) delete This->dds4;
	if (This->d3dt1) delete This->d3dt1;
	if (This->d3dt2) delete This->d3dt2;
	if (This->gammacontrol) free(This->gammacontrol);
	if (This->texture) glTexture_Release(This->texture, FALSE, This->ddInterface->renderer);
	//if (bitmapinfo) free(bitmapinfo);
	if (This->backbuffer) glDirectDrawSurface7_Release(This->backbuffer);
	if (This->zbuffer)
	{
		if (This->zbuffer->attachparent == This) This->zbuffer->attachparent = NULL;
		if (This->zbuffer->attachcount) This->zbuffer->attachcount--;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = NULL;
		This->zbuffer_iface->Release();
	}
	if (This->miptexture) glDirectDrawSurface7_Release(This->miptexture);
	if (This->device) This->device->Release();
	if (This->device1) delete This->device1;
	This->ddInterface->DeleteSurface(This);
	if (This->creator) This->creator->Release();
	if (This->fliplist) free(This->fliplist);
	free(This);
	TRACE_EXIT(-1, 0);
}

ULONG WINAPI glDirectDrawSurface7_AddRef4(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcount4);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_Release4(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount4 == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcount4);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef3(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcount3);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_Release3(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount3 == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcount3);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef2(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcount2);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_Release2(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount2 == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcount2);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRef1(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcount1);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_Release1(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcount1 == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcount1);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRefGamma(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcountgamma);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_ReleaseGamma(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountgamma == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcountgamma);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_AddRefColor(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret = InterlockedIncrement(&This->refcountcolor);
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7_ReleaseColor(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (This->refcountcolor == 0) TRACE_RET(ULONG, 8, 0);
	ret = InterlockedDecrement(&This->refcountcolor);
	if ((This->refcount7 == 0) && (This->refcount4 == 0) && (This->refcount3 == 0) && (This->refcount2 == 0) &&
		(This->refcount1 == 0) && (This->refcountgamma == 0) && (This->refcountcolor == 0))
		glDirectDrawSurface7_Destroy(This);
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
			This->zbuffer->attachcount--;
			This->zbuffer_iface->Release();
		}
		This->zbuffer = attached;
		This->zbuffer_iface = iface;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = This;
		This->zbuffer->attachcount++;
		glRenderer_AttachZ(This->ddInterface->renderer, This->texture, attached->texture);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_CANNOTATTACHSURFACE);
}
HRESULT WINAPI glDirectDrawSurface7_AddOverlayDirtyRect(glDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_AddOverlayDirtyRect: stub\n");
	TRACE_EXIT(23,DDERR_CURRENTLYNOTAVAIL);
	ERR(DDERR_CURRENTLYNOTAVAIL);
}
HRESULT WINAPI glDirectDrawSurface7_Blt(glDirectDrawSurface7 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	DDSURFACEDESC2 ddsdnewstencil;
	HRESULT error;
	RECT tmprect;
	glDirectDrawSurface7 *pattern;
	BltCommand cmd;
	TRACE_ENTER(6,14,This,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	ZeroMemory(&cmd, sizeof(BltCommand));
	cmd.dest = This->texture;
	cmd.destlevel = This->miplevel;
	if (lpDestRect) cmd.destrect = *lpDestRect;
	else cmd.destrect = nullrect;
	if (lpSrcRect) cmd.srcrect = *lpSrcRect;
	else cmd.srcrect = nullrect;
	if (lpDDSrcSurface)
	{
		cmd.src = ((glDirectDrawSurface7*)lpDDSrcSurface)->texture;
		cmd.srclevel = ((glDirectDrawSurface7*)lpDDSrcSurface)->miplevel;
	}
	cmd.flags = dwFlags;
	if (lpDDBltFx) cmd.bltfx = *lpDDBltFx;
	if ((dwFlags & DDBLT_DEPTHFILL) && !lpDDBltFx) TRACE_RET(HRESULT,32,DDERR_INVALIDPARAMS);
	if((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((dwFlags & DDBLT_DDFX) && !lpDDBltFx) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
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
	if (dwFlags & DDBLT_KEYSRC)
	{
		if (!lpDDSrcSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (!(((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.dwFlags & DDSD_CKSRCBLT))
			TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue != 
			((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
			cmd.flags |= 0x20000000;
	}
	if (dwFlags & DDBLT_KEYDEST)
	{
		if (!(This->ddsd.dwFlags & DDSD_CKDESTBLT)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if(This->ddsd.ddckCKDestBlt.dwColorSpaceHighValue != This->ddsd.ddckCKDestBlt.dwColorSpaceLowValue)
			cmd.flags |= 0x40000000;
	}
	glDirectDrawSurface7 *src = (glDirectDrawSurface7 *)lpDDSrcSurface;
	/*if (This->clipper)
	{
		if (!This->clipper->hWnd)
		{
			if (!stencil)
			{
				ddsdnewstencil = ddsdstencil;
				ddsdnewstencil.dwWidth = ddsd.dwWidth;
				ddsdnewstencil.dwHeight = ddsd.dwHeight;
				ddsdnewstencil.lPitch = ddsd.dwWidth * 2;
				glRenderer_MakeTexture(ddInterface->renderer, &stencil, &ddsdnewstencil, ddsd.dwWidth, ddsd.dwHeight);
			}
			if (clipper->dirty)
			{
				glRenderer_UpdateClipper(ddInterface->renderer, stencil, clipper);
				clipper->dirty = false;
			}
			dwFlags |= 0x10000000;
		}
	}*/
	if (This->clipper && !(This->clipper->hWnd)) cmd.flags |= 0x10000000;
	if (dwFlags & DDBLT_DEPTHFILL)
	{
		if (!(This->ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (This->attachparent)
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, This->attachparent->texture));
		}
		else
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(This->ddInterface->renderer, &cmd, NULL));
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
		error = This->ddInterface->SetupTempSurface(tmprect.right, tmprect.bottom);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		error = glDirectDrawSurface7_Blt(This->ddInterface->tmpsurface, &tmprect, lpDDSrcSurface, lpSrcRect, 0, NULL);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		TRACE_RET(HRESULT,23,glDirectDrawSurface7_Blt(This, lpDestRect, (LPDIRECTDRAWSURFACE7)This->ddInterface->tmpsurface,
			&tmprect, dwFlags, lpDDBltFx));
	}
	else TRACE_RET(HRESULT,23,glRenderer_Blt(This->ddInterface->renderer,&cmd));
}
HRESULT WINAPI glDirectDrawSurface7_BltBatch(glDirectDrawSurface7 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_BltBatch: stub\n");
	TRACE_EXIT(23, DDERR_CURRENTLYNOTAVAIL);
	ERR(DDERR_CURRENTLYNOTAVAIL);
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
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(This, &dest, lpDDSrcSurface, lpSrcRect, flags, NULL));
}
HRESULT WINAPI glDirectDrawSurface7_DeleteAttachedSurface(glDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSAttachedSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSAttachedSurface == (LPDIRECTDRAWSURFACE7)This->zbuffer)
	{
		if (This->zbuffer->attachparent == This) This->zbuffer->attachparent = NULL;
		if (This->zbuffer->attachcount) This->zbuffer->attachcount--;
		if (!This->zbuffer->attachcount) This->zbuffer->attachparent = NULL;
		This->zbuffer_iface->Release();
		glRenderer_DetachZ(This->ddInterface->renderer, This->texture);
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
	TRACE_ENTER(4,14,This,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_EnumOverlayZOrders: stub\n");
	TRACE_EXIT(23,DDERR_CURRENTLYNOTAVAIL);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_Flip(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_Flip2(This, lpDDSurfaceTargetOverride, dwFlags);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (This->ddInterface->lastsync)
		{
			This->swapinterval++;
			This->ddInterface->lastsync = false;
		}
		glDirectDrawSurface7_RenderScreen(This, This, This->swapinterval);
	}	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT glDirectDrawSurface7_Flip2(glDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & 0xF8FFFFC0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	if(!This->overlay && ((dwFlags & DDFLIP_ODD) || (dwFlags & DDFLIP_EVEN))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD i;
	glDirectDrawSurface7 *tmp;
	if (dwFlags & DDFLIP_NOVSYNC) This->swapinterval = 0;
	else
	{
		if (dwFlags & DDFLIP_INTERVAL3) This->swapinterval = 3;
		else if (dwFlags & DDFLIP_INTERVAL2) This->swapinterval = 2;
		else if (dwFlags & DDFLIP_INTERVAL4) This->swapinterval = 4;
		else This->swapinterval = 1;
	}
	DWORD flips = 1;
	if(lpDDSurfaceTargetOverride)
	{
		bool success = false;
		if (lpDDSurfaceTargetOverride == (LPDIRECTDRAWSURFACE7)This) TRACE_RET(HRESULT, 23, DD_OK);
		tmp = This;
		for (i = 0; i < This->ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->backbuffer;
			if (lpDDSurfaceTargetOverride == (LPDIRECTDRAWSURFACE7)tmp)
			{
				success = true;
				i++;
				break;
			}
		}
		if (!success) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		for (DWORD x = 0; x < i; x++)
		{
			if (x == i - 1) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Flip2(This, NULL, dwFlags)); }
			else glDirectDrawSurface7_Flip2(This, NULL, 0);
		}
	}
	if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if (This->ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
		{
			TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
		}
		glRenderer_FlipTexture(This->ddInterface->renderer, This->fliplist, This->ddsd.dwBackBufferCount+1, FALSE, dwFlags, 1);
	}
	else TRACE_RET(HRESULT,23,DDERR_NOTFLIPPABLE);
	This->flipcount+=flips;
	if (This->flipcount > This->ddsd.dwBackBufferCount)
		This->flipcount -= (This->ddsd.dwBackBufferCount + 1);
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
		comp2 |= DDSCAPS_BACKBUFFER;
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
	if (FAILED(islost)) return islost;
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
			memcpy(lpDDColorKey,&This->ddsd.ddckCKDestOverlay,sizeof(DDCOLORKEY));
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
HRESULT WINAPI glDirectDrawSurface7_GetDC(glDirectDrawSurface7 *This, HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,This,14,lphDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lphDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->hdc) TRACE_RET(HRESULT,23,DDERR_DCALREADYCREATED);
	This->hdc = glRenderer_GetTextureDC(This->ddInterface->renderer, This->texture, This->miplevel);
	*lphDC = This->hdc;
	if (!This->hdc)	TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
	/*  Move this to glTexture
	glDirectDrawPalette *pal;
	DWORD colors[256];
	HRESULT error;
	LPVOID surface;
	error = this->Lock(NULL,&ddsd,0,NULL);
	if(error != DD_OK) TRACE_RET(HRESULT,23,error);
	hdc = CreateCompatibleDC(NULL);
	bitmapinfo->bmiHeader.biWidth = ddsd.lPitch / (bitmapinfo->bmiHeader.biBitCount / 8);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		if(palette) pal = palette;
		else pal = ddInterface->primary->palette;
		memcpy(colors, glDirectDrawPalette_GetPalette(pal,NULL), 1024);
		for(int i = 0; i < 256; i++)
			colors[i] = ((colors[i]&0x0000FF)<<16) | (colors[i]&0x00FF00) | ((colors[i]&0xFF0000)>>16);
		memcpy(bitmapinfo->bmiColors,colors,1024);
	}
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16) bitmapinfo->bmiHeader.biCompression = BI_BITFIELDS;
	else bitmapinfo->bmiHeader.biCompression = BI_RGB;
	hbitmap = CreateDIBSection(hdc,bitmapinfo,DIB_RGB_COLORS,&surface,NULL,0);
	memcpy(surface,ddsd.lpSurface,ddsd.lPitch*ddsd.dwHeight);
	HGDIOBJ temp = SelectObject(hdc,hbitmap);
	DeleteObject(temp);
	*lphDC = hdc;
	TRACE_VAR("*lphDC",14,*lphDC);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;*/
}
HRESULT WINAPI glDirectDrawSurface7_GetFlipStatus(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
	/*FIXME("glDirectDrawSurface7_GetFlipStatus: stub\n");
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);*/
}
HRESULT WINAPI glDirectDrawSurface7_GetOverlayPosition(glDirectDrawSurface7 *This, LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,This,14,lplX,14,lplY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_GetOverlayPosition: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_GetPalette(glDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lplpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT err;
	*lplpDDPalette = (LPDIRECTDRAWPALETTE)This->palette;
	if(This->palette)
	{
		glDirectDrawPalette_AddRef(This->palette);
		err = DD_OK;
	}
	else err = DDERR_NOPALETTEATTACHED;
	TRACE_VAR("*lplpDDPalette", 14, *lplpDDPalette);
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
	if (lpDDSurfaceDesc->dwSize > sizeof(DDSURFACEDESC2)) { TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS); }
	memcpy(lpDDSurfaceDesc,&This->ddsd,lpDDSurfaceDesc->dwSize);
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
	if (This->hRC == This->ddInterface->renderer->hRC) { TRACE_RET(HRESULT, 23, DD_OK); }
	else { TRACE_RET(HRESULT, 23, DDERR_SURFACELOST); }
}

HRESULT WINAPI glDirectDrawSurface7_Lock(glDirectDrawSurface7 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,This,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->hdc) TRACE_RET(HRESULT, 23, DDERR_SURFACEBUSY);
	if(This->locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	DDSURFACEDESC2 ddsd = This->ddsd;
	glRenderer_LockTexture(This->ddInterface->renderer, This->texture, lpDestRect, &ddsd, dwFlags, This->miplevel);
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	This->locked++;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_ReleaseDC(glDirectDrawSurface7 *This, HDC hDC)
{
	TRACE_ENTER(2,14,This,14,hDC);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->hdc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(hDC != This->hdc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	BOOL primary = FALSE;
	int vsync = 0;
	if (((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		primary = TRUE;
		if (This->ddInterface->lastsync)
		{
			vsync = 1;
			This->ddInterface->lastsync = false;
		}
	}
	glRenderer_ReleaseTextureDC(This->ddInterface->renderer, This->texture, This->miplevel, primary, vsync);
	This->hdc = NULL;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
	/* Move this to glTexture
	GetDIBits(hDC,hbitmap,0,ddsd.dwHeight,ddsd.lpSurface,bitmapinfo,DIB_RGB_COLORS);
	Unlock(NULL);
	DeleteObject(hbitmap);
	hbitmap = NULL;
	DeleteDC(hdc);
	hdc = NULL;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
	*/
}
void glDirectDrawSurface7_Restore2(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	LONG sizes[6];
	if(This->hRC != This->ddInterface->renderer->hRC)
	{
		This->ddInterface->GetSizes(sizes);
		if(This->ddInterface->GetFullscreen())
		{
			This->ddsd.dwWidth = sizes[2];
			This->ddsd.dwHeight = sizes[3];
			if(dxglcfg.primaryscale)  // FIXME:  Support new scaling modes
			{
				This->fakex = sizes[0];
				This->fakey = sizes[1];
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
		if (This->palette) glRenderer_RestoreTexture(This->ddInterface->renderer, This->palette->texture);
		glRenderer_RestoreTexture(This->ddInterface->renderer, This->texture);
	}
	TRACE_EXIT(0,0);
}
HRESULT WINAPI glDirectDrawSurface7_Restore(glDirectDrawSurface7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LONG sizes[6];
	if(!This->ddInterface->renderer) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->hRC != This->ddInterface->renderer->hRC)
	{
		if(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			This->ddInterface->GetSizes(sizes);
			if(This->ddInterface->GetFullscreen())
			{
				This->ddsd.dwWidth = sizes[2];
				This->ddsd.dwHeight = sizes[3];
				if(dxglcfg.primaryscale)  // FIXME:  Support new scaling modes
				{
					This->fakex = sizes[0];
					This->fakey = sizes[1];
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
			if (This->backbuffer) glDirectDrawSurface7_Restore2(This->backbuffer);
			if (This->zbuffer) glDirectDrawSurface7_Restore2(This->zbuffer);
		}
		else
		{
			if (This->backbuffer) glDirectDrawSurface7_Restore2(This->backbuffer);
			if (This->zbuffer) glDirectDrawSurface7_Restore2(This->zbuffer);
		}
		if (This->palette) glRenderer_RestoreTexture(This->ddInterface->renderer, This->palette->texture);
		glRenderer_RestoreTexture(This->ddInterface->renderer, This->texture);
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DD_OK);
}
HRESULT WINAPI glDirectDrawSurface7_SetClipper(glDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,This,14,lpDDClipper);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->clipper) glDirectDrawClipper_Release(This->clipper);
	This->clipper = (glDirectDrawClipper *)lpDDClipper;
	if (This->clipper)
	{
		glDirectDrawClipper_AddRef(This->clipper);
		if (!This->clipper->texture) glDirectDrawClipper_CreateTexture(This->clipper, This->texture, This->ddInterface->renderer);
		glRenderer_SetTextureStencil(This->ddInterface->renderer, This->texture, This->clipper->texture);
	}
	else glRenderer_SetTextureStencil(This->ddInterface->renderer, This->texture, NULL);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_SetColorKey(glDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDColorKey);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDColorKey) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags & DDCKEY_SRCBLT)
	{
		This->ddsd.ddckCKSrcBlt = *lpDDColorKey;
		if (!(dwFlags & DDCKEY_COLORSPACE))
			This->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = This->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue;
		This->ddsd.dwFlags |= DDSD_CKSRCBLT;
	}
	if(dwFlags & DDCKEY_DESTBLT)
	{
		This->ddsd.ddckCKDestBlt = *lpDDColorKey;
		if (!(dwFlags & DDCKEY_COLORSPACE))
			This->ddsd.ddckCKDestBlt.dwColorSpaceHighValue = This->ddsd.ddckCKDestBlt.dwColorSpaceLowValue;
		This->ddsd.dwFlags |= DDSD_CKDESTBLT;
	}
	if(dwFlags & DDCKEY_SRCOVERLAY)
	{
		This->ddsd.ddckCKSrcOverlay = *lpDDColorKey;
		if (!(dwFlags & DDCKEY_COLORSPACE))
			This->ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = This->ddsd.ddckCKSrcOverlay.dwColorSpaceLowValue;
		This->ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
	}
	if(dwFlags & DDCKEY_DESTOVERLAY)
	{
		This->ddsd.ddckCKDestOverlay = *lpDDColorKey;
		if (!(dwFlags & DDCKEY_COLORSPACE))
			This->ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = This->ddsd.ddckCKDestOverlay.dwColorSpaceLowValue;
		This->ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
	}
	glRenderer_SetTextureColorKey(This->ddInterface->renderer, This->texture, dwFlags, lpDDColorKey);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_SetOverlayPosition(glDirectDrawSurface7 *This, LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,This,7,lX,7,lY);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_SetOverlayPosition: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_SetPalette(glDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,This,14,lpDDPalette);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if ((glDirectDrawPalette*)lpDDPalette != This->palette)
	{
		if (This->palette)
		{
			glDirectDrawPalette_Release(This->palette);
			if (!lpDDPalette)
			{
				glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&This->palette);
				glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
			}
		}
		if (lpDDPalette)
		{
			This->palette = (glDirectDrawPalette *)lpDDPalette;
			glDirectDrawPalette_AddRef(This->palette);
		}
	}
	if (lpDDPalette)
	{
		if (!This->palette->texture) glDirectDrawPalette_CreateTexture(This->palette, This->ddInterface->renderer);
	}
	glRenderer_SetTexturePalette(This->ddInterface->renderer, This->texture, This->palette->texture);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7_Unlock(glDirectDrawSurface7 *This, LPRECT lpRect)
{
	TRACE_ENTER(2,14,This,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!This->locked) TRACE_RET(HRESULT, 23, DDERR_NOTLOCKED);
	This->locked--;
	BOOL primary = FALSE;
	int vsync = 0;
	if (((This->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((This->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(This->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		primary = TRUE;
		if (This->ddInterface->lastsync)
		{
			vsync = 1;
			This->ddInterface->lastsync = false;
		}
	}
	HRESULT error = glRenderer_UnlockTexture(This->ddInterface->renderer, This->texture, lpRect, This->miplevel, primary, vsync);
	if (FAILED(error))
	{
		TRACE_EXIT(23, error);
		return error;
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlay(glDirectDrawSurface7 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,This,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_UpdateOverlay: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlayDisplay(glDirectDrawSurface7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_UpdateOverlayDisplay: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7_UpdateOverlayZOrder(glDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	TRACE_ENTER(3,14,This,9,dwFlags,14,lpDDSReference);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7_UpdateOverlayZOrder: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

void glDirectDrawSurface7_RenderScreen(glDirectDrawSurface7 *This, glDirectDrawSurface7 *surface, int vsync)
{
	TRACE_ENTER(3,14,This,14,surface,11,vsync);
	glRenderer_DrawScreen(This->ddInterface->renderer, This->texture, surface->texture, vsync);
	TRACE_EXIT(0,0);
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7_GetDDInterface(glDirectDrawSurface7 *This, LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,This,14,lplpDD);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	This->ddInterface->AddRef();
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
	if (!lpddsd2) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
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
		TRACE_VAR("*lpHandle", 9, *lpHandle);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	This->device = glD3DDev7;
	This->handle = This->device->AddTexture(This);
	if(This->handle == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	This->device->AddRef();
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
	DWORD flags = DDCKEY_SRCBLT;
	glDirectDrawSurface7_Blt(This,NULL,(LPDIRECTDRAWSURFACE7)src,NULL,DDBLT_WAIT,NULL);
	if (src->ddsd.dwFlags & DDSD_CKSRCBLT)
	{
		if (src->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue != src->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
			flags |= DDCKEY_COLORSPACE;
		glDirectDrawSurface7_SetColorKey(This, flags, &src->ddsd.ddckCKSrcBlt);
	}
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

// DDRAW1 wrapper
glDirectDrawSurface1::glDirectDrawSurface1(glDirectDrawSurface7 *gl_DDS7)
{
	TRACE_ENTER(2,14,this,14,gl_DDS7);
	glDDS7 = gl_DDS7;
	TRACE_EXIT(-1, 0);
}
HRESULT WINAPI glDirectDrawSurface1::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_QueryInterface(glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface1::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef1(glDDS7));
}
ULONG WINAPI glDirectDrawSurface1::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release1(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, this, 14, lpDDSAttachedSurface);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(glDDS7, lpRect));
}
HRESULT WINAPI glDirectDrawSurface1::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,this,26,lpDestRect,14,lpDDSrcSurface,14,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			glDirectDrawSurface1 *pattern = (glDirectDrawSurface1*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->GetDDS7();
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, 
				lpDestRect, (LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface1::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltBatch(glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface1::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSAttachedSurface)->GetDDS7());
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,this,14,lpContext,14,lpEnumSurfacesCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumAttachedSurfaces(glDDS7,context,EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface1::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface1::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface1*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface1 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface,(void **)&attached1);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = glDirectDrawSurface7_GetCaps(glDDS7,&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface1::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface1::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(glDDS7,lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface1::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(glDDS7,&ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,this,14,lpDD,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface1::IsLost()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDirectDrawSurface7_Lock(glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(glDDS7,hDC));
}
HRESULT WINAPI glDirectDrawSurface1::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface1::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(glDDS7,lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface1::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(glDDS7,lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(glDDS7,lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(glDDS7,lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}

// DDRAW2 wrapper
glDirectDrawSurface2::glDirectDrawSurface2(glDirectDrawSurface7 *gl_DDS7)
{
	TRACE_ENTER(2,14,this,14,gl_DDS7);
	glDDS7 = gl_DDS7;
	TRACE_EXIT(-1, 0);
}
HRESULT WINAPI glDirectDrawSurface2::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_QueryInterface(glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface2::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef2(glDDS7));
}
ULONG WINAPI glDirectDrawSurface2::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release2(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2::AddAttachedSurface(LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, this, 14, lpDDSAttachedSurface);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(glDDS7,lpRect));
}
HRESULT WINAPI glDirectDrawSurface2::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,this,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			glDirectDrawSurface2 *pattern = (glDirectDrawSurface2*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->GetDDS7();
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, 
				lpDestRect, (LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface2::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_BltBatch(glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,4,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface2::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSAttachedSurface)->GetDDS7());
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,this,14,lpContext,14,lpEnumSurfacesCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface2::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_EnumOverlayZOrders(glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface2::Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface2*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface2 *attached2;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface2,(void **)&attached2);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = attached2;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetBltStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error = glDirectDrawSurface7_GetCaps(glDDS7,&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface2::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface2::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(glDDS7,lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface2::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(glDDS7,&ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,this,14,lpDD,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface2::IsLost()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDirectDrawSurface7_Lock(glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(glDDS7,hDC));
}
HRESULT WINAPI glDirectDrawSurface2::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface2::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(glDDS7,lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface2::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(glDDS7,lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(glDDS7,lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(glDDS7,lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface2::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDirectDrawSurface7_GetDDInterface(glDDS7,(void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDD7->QueryInterface(IID_IDirectDraw,lplpDD);
	glDD7->Release();
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_RET(HRESULT,23,ret);
}
HRESULT WINAPI glDirectDrawSurface2::PageLock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageLock(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageUnlock(glDDS7,dwFlags));
}

// DDRAW3 wrapper
glDirectDrawSurface3::glDirectDrawSurface3(glDirectDrawSurface7 *gl_DDS7)
{
	TRACE_ENTER(2,14,this,14,gl_DDS7);
	glDDS7 = gl_DDS7;
	TRACE_EXIT(-1, 0);
}
HRESULT WINAPI glDirectDrawSurface3::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_QueryInterface(glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface3::AddRef()
{
	TRACE_ENTER(1,14,this);
	if (!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef3(glDDS7));
}
ULONG WINAPI glDirectDrawSurface3::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release3(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3::AddAttachedSurface(LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, this, 14, lpDDSAttachedSurface);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_AddOverlayDirtyRect(glDDS7,lpRect));
}
HRESULT WINAPI glDirectDrawSurface3::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,this,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			glDirectDrawSurface3 *pattern = (glDirectDrawSurface3*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->GetDDS7();
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, 
				lpDestRect, (LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface3::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_BltBatch(glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface3::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSAttachedSurface)->GetDDS7());
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,this,14,lpContext,14,lpEnumSurfacesCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(glDDS7, context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface3::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_EnumOverlayZOrders(glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface3::Flip(LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface3*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface3 *attached3;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDirectDrawSurface7_GetAttachedSurface(glDDS7,&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface3,(void **)&attached3);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = attached3;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetBltStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDirectDrawSurface7_GetCaps(glDDS7,&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetClipper(glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetDC(glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface3::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetFlipStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetOverlayPosition(glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface3::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetPalette(glDDS7,lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_GetPixelFormat(glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface3::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDirectDrawSurface7_GetSurfaceDesc(glDDS7,&ddsd);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,this,14,lpDD,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface3::IsLost()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_IsLost(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDirectDrawSurface7_Lock(glDDS7, lpDestRect, &ddsd, dwFlags, hEvent);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	memcpy(lpDDSurfaceDesc, &ddsd, sizeof(DDSURFACEDESC));
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_ReleaseDC(glDDS7,hDC));
}
HRESULT WINAPI glDirectDrawSurface3::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_Restore(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface3::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetClipper(glDDS7,lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetOverlayPosition(glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface3::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetPalette(glDDS7,lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_Unlock2(glDDS7,lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_UpdateOverlay(glDDS7,lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_UpdateOverlayDisplay(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_UpdateOverlayZOrder(glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface3::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDirectDrawSurface7_GetDDInterface(glDDS7,(void**)&glDD7);
	if(ret != DD_OK) TRACE_RET(HRESULT,23,ret);
	glDD7->QueryInterface(IID_IDirectDraw,lplpDD);
	glDD7->Release();
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::PageLock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,14,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_PageLock(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_PageUnlock(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::SetSurfaceDesc(LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpddsd,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_SetSurfaceDesc(glDDS7,(LPDDSURFACEDESC2)lpddsd,dwFlags));
}

// DDRAW4 wrapper
glDirectDrawSurface4::glDirectDrawSurface4(glDirectDrawSurface7 *gl_DDS7)
{
	TRACE_ENTER(2,14,this,14,gl_DDS7);
	glDDS7 = gl_DDS7;
	TRACE_EXIT(-1, 0);
}
HRESULT WINAPI glDirectDrawSurface4::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_QueryInterface(glDDS7, riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface4::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_AddRef4(glDDS7));
}
ULONG WINAPI glDirectDrawSurface4::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirectDrawSurface7_Release4(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4::AddAttachedSurface(LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, this, 14, lpDDSAttachedSurface);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirectDrawSurface7_AddAttachedSurface2(glDDS7, dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_AddOverlayDirtyRect(glDDS7,lpRect));
}
HRESULT WINAPI glDirectDrawSurface4::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	TRACE_ENTER(6,14,this,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDBLTFX bltfx;
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		bltfx = *lpDDBltFx;
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			glDirectDrawSurface4 *pattern = (glDirectDrawSurface4*)lpDDBltFx->lpDDSPattern;
			bltfx.lpDDSPattern = (LPDIRECTDRAWSURFACE)pattern->GetDDS7();
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, 
				lpDestRect, (LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDirectDrawSurface7_Blt(glDDS7, lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Blt(glDDS7,lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface4::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_BltBatch(glDDS7,lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_BltFast(glDDS7,dwX,dwY,
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface4::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDirectDrawSurface7_DeleteAttachedSurface(glDDS7, dwFlags, 
		(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSAttachedSurface)->GetDDS7());
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,this,14,lpContext,14,lpEnumSurfacesCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpEnumSurfacesCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPVOID context[2];
	context[0] = lpEnumSurfacesCallback;
	context[1] = lpContext;
	TRACE_RET(HRESULT, 23, glDirectDrawSurface7_EnumAttachedSurfaces(glDDS7, context, EnumSurfacesCallback2));
}
HRESULT WINAPI glDirectDrawSurface4::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirectDrawSurface7_EnumOverlayZOrders(glDDS7,dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface4::Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,
			(LPDIRECTDRAWSURFACE7)((glDirectDrawSurface4*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23, glDirectDrawSurface7_Flip(glDDS7,NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps2,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface4 *attached4;
	error = glDirectDrawSurface7_GetAttachedSurface(glDDS7,lpDDSCaps2,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		glDirectDrawSurface7_QueryInterface(attachedsurface,IID_IDirectDrawSurface4,(void **)&attached4);
		glDirectDrawSurface7_Release(attachedsurface);
		*lplpDDAttachedSurface = attached4;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface4::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetBltStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetCaps(glDDS7,lpDDSCaps));
}
HRESULT WINAPI glDirectDrawSurface4::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetClipper(glDDS7,lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDC(glDDS7,lphDC));
}
HRESULT WINAPI glDirectDrawSurface4::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetFlipStatus(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetOverlayPosition(glDDS7,lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface4::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPalette(glDDS7,lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPixelFormat(glDDS7,lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface4::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetSurfaceDesc(glDDS7,lpDDSurfaceDesc));
}
HRESULT WINAPI glDirectDrawSurface4::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,this,14,lpDD,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface4::IsLost()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_IsLost(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Lock(glDDS7,lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent));
}
HRESULT WINAPI glDirectDrawSurface4::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ReleaseDC(glDDS7,hDC));
}
HRESULT WINAPI glDirectDrawSurface4::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Restore(glDDS7));
}
HRESULT WINAPI glDirectDrawSurface4::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetClipper(glDDS7,lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetColorKey(glDDS7,dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetOverlayPosition(glDDS7,lX,lY));
}
HRESULT WINAPI glDirectDrawSurface4::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPalette(glDDS7,lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4::Unlock(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_Unlock2(glDDS7,lpRect));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlay(glDDS7,lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayDisplay(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_UpdateOverlayZOrder(glDDS7,dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface4::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetDDInterface(glDDS7,lplpDD));
}
HRESULT WINAPI glDirectDrawSurface4::PageLock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageLock(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_PageUnlock(glDDS7,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpddsd,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetSurfaceDesc(glDDS7,lpddsd,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,this,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_SetPrivateData(glDDS7,guidTag,lpData,cbSize,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,this,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetPrivateData(glDDS7,guidTag,lpBuffer,lpcbBufferSize));
}
HRESULT WINAPI glDirectDrawSurface4::FreePrivateData(REFGUID guidTag)
{
	TRACE_ENTER(2,14,this,24,&guidTag);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_FreePrivateData(glDDS7,guidTag));
}
HRESULT WINAPI glDirectDrawSurface4::GetUniquenessValue(LPDWORD lpValue)
{
	TRACE_ENTER(2,14,this,14,lpValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_GetUniquenessValue(glDDS7,lpValue));
}
HRESULT WINAPI glDirectDrawSurface4::ChangeUniquenessValue()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirectDrawSurface7_ChangeUniquenessValue(glDDS7));
}
