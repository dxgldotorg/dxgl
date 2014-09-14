// DXGL
// Copyright (C) 2011-2014 William Feely

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
#include "TextureManager.h"
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


// DDRAW7 routines
glDirectDrawSurface7::glDirectDrawSurface7(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, HRESULT *error,
	glDirectDrawPalette *palettein, TEXTURE *parenttex, DWORD miplevel, int version)
{
	TRACE_ENTER(5,14,this,14,lpDD7,14,lpDDSurfaceDesc2,14,error,14,palettein);
	creator = NULL;
	overlay = false;
	hasstencil = false;
	dirty = 2;
	handle = 0;
	device = NULL;
	device1 = NULL;
	locked = 0;
	pagelocked = 0;
	flipcount = 0;
	ZeroMemory(colorkey,4*sizeof(CKEY));
	bitmapinfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO)+(255*sizeof(RGBQUAD)));
	ZeroMemory(bitmapinfo,sizeof(BITMAPINFO)+(255*sizeof(RGBQUAD)));
	ZeroMemory(&fbo,sizeof(FBO));
	ZeroMemory(&stencilfbo,sizeof(FBO));
	palette = NULL;
	stencil = NULL;
	paltex = NULL;
	texture = NULL;
	clipper = NULL;
	hdc = NULL;
	dds1 = new glDirectDrawSurface1(this);
	dds2 = new glDirectDrawSurface2(this);
	dds3 = new glDirectDrawSurface3(this);
	dds4 = new glDirectDrawSurface4(this);
	d3dt2 = new glDirect3DTexture2(this);
	d3dt1 = new glDirect3DTexture1(this);
	glDirectDrawGammaControl_Create(this, (LPDIRECTDRAWGAMMACONTROL*)&gammacontrol);
	buffer = gdibuffer = NULL;
	bigbuffer = NULL;
	zbuffer = NULL;
	this->miplevel = miplevel;
	DWORD colormasks[3];
	magfilter = minfilter = GL_NEAREST;
	ddInterface = (glDirectDraw7 *)lpDD7;
	hRC = ddInterface->renderer->hRC;
	ddsd = *lpDDSurfaceDesc2;
	miptexture = NULL;
	LONG sizes[6];
	ddInterface->GetSizes(sizes);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(((ddsd.dwFlags & DDSD_WIDTH) || (ddsd.dwFlags & DDSD_HEIGHT)
			|| (ddsd.dwFlags & DDSD_PIXELFORMAT)) && !(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
		{
			*error = DDERR_INVALIDPARAMS;
			TRACE_VAR("*error",23,DDERR_INVALIDPARAMS);
			TRACE_EXIT(-1,0);
			return;
		}
		else
		{
			if(ddInterface->GetFullscreen())
			{
				ddsd.dwWidth = sizes[2];
				ddsd.dwHeight = sizes[3];
				if(dxglcfg.highres)
				{
					fakex = sizes[0];
					fakey = sizes[1];
				}
				else
				{
					fakex = ddsd.dwWidth;
					fakey = ddsd.dwHeight;
				}
				ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
				*error = DD_OK;
			}
			else
			{
				fakex = ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
				fakey = ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
				*error = DD_OK;
			}
		}
		if(ddInterface->GetBPP() == 8)
		{
			if (!palettein) glDirectDrawPalette_Create(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE|0x800,NULL,(LPDIRECTDRAWPALETTE*)&palette);
			else
			{
				palette = palettein;
				glDirectDrawPalette_AddRef(palette);
			}
			paltex = new TEXTURE;
			ZeroMemory(paltex,sizeof(TEXTURE));
			paltex->minfilter = paltex->magfilter = GL_NEAREST;
			paltex->wraps = paltex->wrapt = GL_CLAMP_TO_EDGE;
			paltex->pixelformat.dwFlags = DDPF_RGB;
			paltex->pixelformat.dwBBitMask = 0xFF0000;
			paltex->pixelformat.dwGBitMask = 0xFF00;
			paltex->pixelformat.dwRBitMask = 0xFF;
			paltex->pixelformat.dwRGBBitCount = 32;
			glRenderer_MakeTexture(ddInterface->renderer,paltex,256,1);
		}
		else paltex = NULL;
	}
	else
	{
		if((ddsd.dwFlags & DDSD_WIDTH) && (ddsd.dwFlags & DDSD_HEIGHT))
		{
			fakex = ddsd.dwWidth;
			fakey = ddsd.dwHeight;
		}
		else
		{
			*error = DDERR_INVALIDPARAMS;
			TRACE_VAR("*error",23,DDERR_INVALIDPARAMS);
			TRACE_EXIT(-1,0);
			return;
		}
	}
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
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
	bitmapinfo->bmiHeader.biPlanes = 1;
	backbuffer = NULL;
	if ((ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && !(ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if (!(ddsd.dwFlags & DDSD_MIPMAPCOUNT))
		{
			ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
			ddsd.dwMipMapCount = CalculateMipLevels(ddsd.dwWidth, ddsd.dwHeight);
		}
	}
	if (ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) texture = parenttex;
	else texture = new TEXTURE;
	switch(surfacetype)
	{
	case 0:
		buffer = (char *)malloc(NextMultipleOfWord((ddsd.ddpfPixelFormat.dwRGBBitCount * ddsd.dwWidth)/8) * ddsd.dwHeight);
		if((ddsd.dwWidth != fakex) || (ddsd.dwHeight != fakey))
			bigbuffer = (char *)malloc(NextMultipleOfWord((ddsd.ddpfPixelFormat.dwRGBBitCount * fakex)/8) * fakey);
		if(!buffer) *error = DDERR_OUTOFMEMORY;
		goto maketex;
		break;
	case 1:
		buffer = NULL;
		break;
	case 2:
		buffer = NULL;
	maketex:
		if((dxglcfg.scalingfilter == 0) || (ddInterface->GetBPP() == 8)) magfilter = minfilter = GL_NEAREST;
		else magfilter = minfilter = GL_LINEAR;
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
		{
			ddsd.dwFlags |= DDSD_PIXELFORMAT;
			ddsd.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
			if(!ddsd.ddpfPixelFormat.dwZBufferBitDepth)
				ddsd.ddpfPixelFormat.dwZBufferBitDepth = ddsd.dwRefreshRate;
			switch(ddsd.ddpfPixelFormat.dwZBufferBitDepth)
			{
			case 8:
				ddsd.ddpfPixelFormat.dwZBitMask = 0xFF;
				break;
			case 16:
				ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFF;
				break;
			case 24:
				ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFF;
				break;
			case 32:
			default:
				ddsd.ddpfPixelFormat.dwZBitMask = 0xFFFFFFFF;
				break;
			}
		}
		if(!(ddsd.dwFlags & DDSD_PIXELFORMAT))
		{
			ZeroMemory(&ddsd.ddpfPixelFormat,sizeof(DDPIXELFORMAT));
			ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
			ddsd.ddpfPixelFormat.dwRGBBitCount = ddInterface->GetBPP();
			switch(ddInterface->GetBPP())
			{
			case 8:
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
				ddsd.ddpfPixelFormat.dwRBitMask = 0;
				ddsd.ddpfPixelFormat.dwGBitMask = 0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth);
				break;
			case 15:
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
				break;
			case 16:
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				break;
			case 24:
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*3);
				break;
			case 32:
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*4);
				break;
			default:
				*error = DDERR_INVALIDPIXELFORMAT;
				TRACE_VAR("*error",23,DDERR_INVALIDPIXELFORMAT);
				TRACE_EXIT(-1,0);
				return;
			}
		}
		else ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*(ddsd.ddpfPixelFormat.dwRGBBitCount / 8));
		if (!(ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
		{
			texture->pixelformat = ddsd.ddpfPixelFormat;
			if ((ddsd.dwFlags & DDSD_CAPS) && (ddsd.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
				texture->minfilter = texture->magfilter = GL_NEAREST;
			else
			{
				if (dxglcfg.scalingfilter && (ddInterface->GetBPP() > 8)) texture->minfilter = texture->magfilter = GL_LINEAR;
				else texture->minfilter = texture->magfilter = GL_NEAREST;
			}
			texture->wraps = texture->wrapt = GL_CLAMP_TO_EDGE;
			if (ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) texture->miplevel = ddsd.dwMipMapCount;
			else texture->miplevel = 1;
			glRenderer_MakeTexture(ddInterface->renderer, texture, fakex, fakey);
		}
	}
	if ((ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && ddsd.dwMipMapCount)
	{
		DDSURFACEDESC2 newdesc = ddsd;
		newdesc.dwWidth = max(1, (DWORD)floorf((float)ddsd.dwWidth / 2.0f));
		newdesc.dwHeight = max(1, (DWORD)floorf((float)ddsd.dwHeight / 2.0f));
		newdesc.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
		newdesc.dwMipMapCount = ddsd.dwMipMapCount - 1;
		HRESULT miperror;
		if(newdesc.dwMipMapCount) miptexture = new glDirectDrawSurface7(lpDD7, &newdesc, &miperror, palette, texture, miplevel + 1, version);
	}

	if(ddsd.ddpfPixelFormat.dwRGBBitCount > 8)
	{
		colormasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
		colormasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
		colormasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
		memcpy(bitmapinfo->bmiColors,colormasks,3*sizeof(DWORD));
	}
	if(!bitmapinfo->bmiHeader.biBitCount)
		bitmapinfo->bmiHeader.biBitCount = (WORD)ddsd.ddpfPixelFormat.dwRGBBitCount;
	refcount7 = 1;
	refcount4 = 0;
	refcount3 = 0;
	refcount2 = 0;
	refcount1 = 0;
	refcountgamma = 0;
	refcountcolor = 0;
	mulx = (float)fakex / (float)ddsd.dwWidth;
	muly = (float)fakey / (float)ddsd.dwHeight;
	*error = DD_OK;
	backbuffer = NULL;
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
		{
			if((ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) && (ddsd.dwBackBufferCount > 0))
			{
				if(!(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))	ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
				DDSURFACEDESC2 ddsdBack;
				memcpy(&ddsdBack,&ddsd,ddsd.dwSize);
				ddsdBack.dwBackBufferCount--;
				ddsdBack.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
				ddsdBack.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
				backbuffer = new glDirectDrawSurface7(ddInterface,&ddsdBack,error,palette,parenttex,miplevel,version);
			}
			else if (ddsd.dwFlags & DDSD_BACKBUFFERCOUNT){}
			else *error = DDERR_INVALIDPARAMS;
		}
	}
	switch (version)
	{
	case 1:
	case 2:
	case 3:
	default:
		textureparent = this->dds1;
		break;
	case 4:
		textureparent = this->dds4;
		break;
	case 7:
		textureparent = this;
		break;
	}
	TRACE_VAR("*error",23,*error);
	TRACE_EXIT(-1,0);
}
glDirectDrawSurface7::~glDirectDrawSurface7()
{
	TRACE_ENTER(1,14,this);
	AddRef();
	if (dds1) delete dds1;
	if (dds2) delete dds2;
	if (dds3) delete dds3;
	if (dds4) delete dds4;
	if (d3dt1) delete d3dt1;
	if (d3dt2) delete d3dt2;
	if (gammacontrol) free(gammacontrol);
	if(paltex)
	{
		glRenderer_DeleteTexture(ddInterface->renderer, paltex);
		delete paltex;
	}
	if(texture && !(ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		glRenderer_DeleteTexture(ddInterface->renderer, texture);
		delete texture;
	}
	if(stencil)
	{
		glRenderer_DeleteTexture(ddInterface->renderer, stencil);
		delete stencil;
	}
	if(fbo.fbo) glRenderer_DeleteFBO(ddInterface->renderer, &fbo);
	if(stencilfbo.fbo) glRenderer_DeleteFBO(ddInterface->renderer, &stencilfbo);
	if(bitmapinfo) free(bitmapinfo);
	if(palette) glDirectDrawPalette_Release(palette);
	if(backbuffer) backbuffer->Release();
	if(clipper) glDirectDrawClipper_Release(clipper);
	if(buffer) free(buffer);
	if(bigbuffer) free(bigbuffer);
	if(zbuffer) zbuffer_iface->Release();
	if(miptexture) miptexture->Release();
	if (device) device->Release(); 
	if (device1) delete device1;
	ddInterface->DeleteSurface(this);
	if (creator) creator->Release();
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI glDirectDrawSurface7::QueryInterface(REFIID riid, void** ppvObj)
{
	HRESULT ret;
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirectDrawSurface7))
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface4)
	{
		this->AddRef4();
		*ppvObj = dds4;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		this->AddRef3();
		*ppvObj = dds3;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		this->AddRef2();
		*ppvObj = dds2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface)
	{
		this->AddRef1();
		*ppvObj = dds1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture2)
	{
		d3dt2->AddRef();
		*ppvObj = d3dt2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirect3DTexture)
	{
		d3dt1->AddRef();
		*ppvObj = d3dt1;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if (riid == IID_IDirectDrawGammaControl)
	{
		this->AddRefGamma();
		*ppvObj = gammacontrol;
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

		if(!device1)
		{
			glDirect3D7 *tmpd3d;
			ddInterface->QueryInterface(IID_IDirect3D7,(void**)&tmpd3d);
			if(!tmpd3d) TRACE_RET(HRESULT,23,E_NOINTERFACE);
			device1 = new glDirect3DDevice7(riid, tmpd3d, this, dds1);
			if (FAILED(device1->err()))
			{
				ret = device1->err();
				delete device1;
				tmpd3d->Release();
				TRACE_EXIT(23, ret);
				return ret;
			}
			*ppvObj = device1->glD3DDev1;
			device1->glD3DDev1->AddRef();
			tmpd3d->Release();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else
		{
			HRESULT ret = device->QueryInterface(IID_IDirect3DDevice,ppvObj);
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,ret);
			return ret;
		}
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDrawSurface7::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount7++;
	TRACE_EXIT(8,refcount7);
	return refcount7;
}
ULONG WINAPI glDirectDrawSurface7::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	if (refcount7 == 0) TRACE_RET(ULONG, 8, 0);
	refcount7--;
	ret = refcount7;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8,ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRef4()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcount4++;
	TRACE_EXIT(8, refcount4);
	return refcount4;
}
ULONG WINAPI glDirectDrawSurface7::Release4()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcount4 == 0) TRACE_RET(ULONG, 8, 0);
	refcount4--;
	ret = refcount4;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRef3()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcount3++;
	TRACE_EXIT(8, refcount3);
	return refcount3;
}
ULONG WINAPI glDirectDrawSurface7::Release3()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcount3 == 0) TRACE_RET(ULONG, 8, 0);
	refcount3--;
	ret = refcount3;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRef2()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcount2++;
	TRACE_EXIT(8, refcount2);
	return refcount2;
}
ULONG WINAPI glDirectDrawSurface7::Release2()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcount2 == 0) TRACE_RET(ULONG, 8, 0);
	refcount2--;
	ret = refcount2;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRef1()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcount1++;
	TRACE_EXIT(8, refcount1);
	return refcount1;
}
ULONG WINAPI glDirectDrawSurface7::Release1()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcount1 == 0) TRACE_RET(ULONG, 8, 0);
	refcount1--;
	ret = refcount1;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRefGamma()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcountgamma++;
	TRACE_EXIT(8, refcountgamma);
	return refcountgamma;
}
ULONG WINAPI glDirectDrawSurface7::ReleaseGamma()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcountgamma == 0) TRACE_RET(ULONG, 8, 0);
	refcountgamma--;
	ret = refcountgamma;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}
ULONG WINAPI glDirectDrawSurface7::AddRefColor()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	refcountcolor++;
	TRACE_EXIT(8, refcountcolor);
	return refcountcolor;
}
ULONG WINAPI glDirectDrawSurface7::ReleaseColor()
{
	TRACE_ENTER(1, 14, this);
	if (!this) TRACE_RET(ULONG, 8, 0);
	ULONG ret;
	if (refcountcolor == 0) TRACE_RET(ULONG, 8, 0);
	refcountcolor--;
	ret = refcountcolor;
	if ((refcount7 == 0) && (refcount4 == 0) && (refcount3 == 0) && (refcount2 == 0) &&
		(refcount1 == 0) && (refcountgamma == 0) && (refcountcolor == 0))delete this;
	TRACE_EXIT(8, ret);
	return ret;
}

HRESULT WINAPI glDirectDrawSurface7::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(2, 14, this, 14, lpDDSAttachedSurface);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT ret;
	ret = ((IUnknown*)lpDDSAttachedSurface)->QueryInterface(IID_IDirectDrawSurface7, (void**)&dds7);
	if (ret != S_OK) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = AddAttachedSurface2(dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}

HRESULT glDirectDrawSurface7::AddAttachedSurface2(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface, IUnknown *iface)
{
	TRACE_ENTER(3,14,this,14,lpDDSAttachedSurface,iface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSAttachedSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(zbuffer) TRACE_RET(HRESULT,23,DDERR_SURFACEALREADYATTACHED);
	glDirectDrawSurface7 *attached = (glDirectDrawSurface7 *)lpDDSAttachedSurface;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	attached->GetSurfaceDesc(&ddsd);
	if((ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER) || (ddsd.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
	{
		zbuffer = attached;
		zbuffer_iface = iface;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_CANNOTATTACHSURFACE);
}
HRESULT WINAPI glDirectDrawSurface7::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::AddOverlayDirtyRect: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	HRESULT error;
	RECT tmprect;
	glDirectDrawSurface7 *pattern;
	TRACE_ENTER(6,14,this,26,lpDestRect,14,lpDDSrcSurface,26,lpSrcRect,9,dwFlags,14,lpDDBltFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((dwFlags & DDBLT_DDFX) && !lpDDBltFx) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (dwFlags & DDBLT_ROP)
	{
		if (!lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if ((rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4))
		{
			if (!lpDDBltFx->lpDDSPattern) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
			pattern = (glDirectDrawSurface7*)lpDDBltFx->lpDDSPattern;
			if (pattern->dirty & 1)
				glRenderer_UploadTexture(ddInterface->renderer, pattern->buffer, pattern->bigbuffer, pattern->texture,
					pattern->ddsd.dwWidth, pattern->ddsd.dwHeight, pattern->fakex, pattern->fakey, pattern->ddsd.lPitch,
					(NextMultipleOf4((ddInterface->GetBPPMultipleOf8() / 8)*pattern->fakex)),
					pattern->ddsd.ddpfPixelFormat.dwRGBBitCount,pattern->miplevel);
		}
	}
	if (dwFlags & DDBLT_KEYSRC)
	{
		if (!lpDDSrcSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (((glDirectDrawSurface7*)lpDDSrcSurface)->colorkey[0].colorspace) dwFlags |= 0x20000000;
	}
	if ((dwFlags & DDBLT_KEYDEST) && colorkey[1].colorspace) dwFlags |= 0x40000000;
	glDirectDrawSurface7 *src = (glDirectDrawSurface7 *)lpDDSrcSurface;
	if(dirty & 1)
	{
		glRenderer_UploadTexture(ddInterface->renderer,buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
			fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
			ddsd.ddpfPixelFormat.dwRGBBitCount,miplevel);
		dirty &= ~1;
	}
	if(src && (src->dirty & 1))
	{
		glRenderer_UploadTexture(ddInterface->renderer,src->buffer,src->bigbuffer,src->texture,src->ddsd.dwWidth,src->ddsd.dwHeight,
			src->fakex,src->fakey,src->ddsd.lPitch,
			(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*src->fakex)),
			src->ddsd.ddpfPixelFormat.dwRGBBitCount,src->miplevel);
		src->dirty &= ~1;
	}
	if (clipper)
	{
		if (!clipper->hWnd)
		{
			if (!stencil)
			{
				stencil = (TEXTURE*)malloc(sizeof(TEXTURE));
				ZeroMemory(stencil, sizeof(TEXTURE));
				stencil->minfilter = stencil->magfilter = GL_NEAREST;
				stencil->wraps = stencil->wrapt = GL_CLAMP_TO_EDGE;
				stencil->pixelformat.dwSize = sizeof(DDPIXELFORMAT);
				stencil->pixelformat.dwFlags = DDPF_RGB|DDPF_ALPHAPIXELS;
				stencil->pixelformat.dwBBitMask = 0xF;
				stencil->pixelformat.dwGBitMask = 0xF0;
				stencil->pixelformat.dwRBitMask = 0xF00;
				stencil->pixelformat.dwRGBAlphaBitMask = 0xF000;
				stencil->pixelformat.dwRGBBitCount = 16;
				glRenderer_MakeTexture(ddInterface->renderer, stencil, ddsd.dwWidth, ddsd.dwHeight);
			}
			if (clipper->dirty)
			{
				glRenderer_UpdateClipper(ddInterface->renderer, this);
				clipper->dirty = false;
			}
			dwFlags |= 0x10000000;
		}
	}
	if (this == src)
	{
		tmprect.left = tmprect.top = 0;
		tmprect.right = lpSrcRect->right - lpSrcRect->left;
		tmprect.bottom = lpSrcRect->bottom - lpSrcRect->top;
		error = ddInterface->SetupTempSurface(tmprect.right, tmprect.bottom);
		if (error) TRACE_RET(HRESULT, 23, error);
		error = ddInterface->tmpsurface->Blt(&tmprect, lpDDSrcSurface, lpSrcRect, 0, NULL);
		if (error) TRACE_RET(HRESULT, 23, error);
		TRACE_RET(HRESULT,23,this->Blt(lpDestRect, ddInterface->tmpsurface, &tmprect, dwFlags, lpDDBltFx));
	}
	else TRACE_RET(HRESULT,23,glRenderer_Blt(ddInterface->renderer,lpDestRect,src,this,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface7::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::BltBatch: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (clipper) TRACE_RET(HRESULT, 23, DDERR_BLTFASTCANTCLIP);
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
	TRACE_RET(HRESULT,23,Blt(&dest,lpDDSrcSurface,lpSrcRect,flags,NULL));
}
HRESULT WINAPI glDirectDrawSurface7::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSAttachedSurface == (LPDIRECTDRAWSURFACE7)zbuffer)
	{
		zbuffer_iface->Release();
		zbuffer = NULL;
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_SURFACENOTATTACHED);
}
HRESULT WINAPI glDirectDrawSurface7::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	TRACE_ENTER(3,14,this,14,lpContext,14,lpEnumSurfacesCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT enumret = DDENUMRET_OK;
	if (backbuffer)
	{
		backbuffer->AddRef();
		enumret = lpEnumSurfacesCallback(backbuffer, &backbuffer->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (zbuffer)
	{
		zbuffer->AddRef();
		enumret = lpEnumSurfacesCallback(zbuffer, &zbuffer->ddsd, lpContext);
	}
	if (enumret == DDENUMRET_CANCEL) TRACE_RET(HRESULT, 23, DD_OK);
	if (miptexture)
	{
		miptexture->AddRef();
		enumret = lpEnumSurfacesCallback(miptexture, &miptexture->ddsd, lpContext);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::EnumOverlayZOrders: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = Flip2(lpDDSurfaceTargetOverride,dwFlags);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(ddInterface->lastsync)
		{
			swapinterval++;
			ddInterface->lastsync = false;
		}
		RenderScreen(texture,this,swapinterval);
	}
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT glDirectDrawSurface7::Flip2(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & 0xF8FFFFC0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	if(!overlay && ((dwFlags & DDFLIP_ODD) || (dwFlags & DDFLIP_EVEN))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD i;
	glDirectDrawSurface7 *tmp;
	if(dwFlags & DDFLIP_NOVSYNC) swapinterval=0;
	else
	{
		if(dwFlags & DDFLIP_INTERVAL3) swapinterval=3;
		else if(dwFlags & DDFLIP_INTERVAL2) swapinterval=2;
		else if(dwFlags & DDFLIP_INTERVAL4) swapinterval=4;
		else swapinterval=1;
	}
	int flips = 1;
	if(lpDDSurfaceTargetOverride)
	{
		bool success = false;
		if(lpDDSurfaceTargetOverride == this) TRACE_RET(HRESULT,23,DD_OK);
		tmp = this;
		for(i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			if(lpDDSurfaceTargetOverride == tmp)
			{
				success = true;
				i++;
				break;
			}
		}
		if(!success) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		for(DWORD x = 0; x < i; x++)
		{
			if(x == i-1) {TRACE_RET(HRESULT,23,Flip2(NULL,dwFlags));}
			else Flip2(NULL,0);
		}
	}
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		TEXTURE **textures = new TEXTURE*[ddsd.dwBackBufferCount+1];
		textures[0] = texture;
		tmp = this;
		if(dirty & 1)
		{
			glRenderer_UploadTexture(ddInterface->renderer,buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
				fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
				ddsd.ddpfPixelFormat.dwRGBBitCount,miplevel);
			dirty &= ~1;
		}
		this->dirty |= 2;
		for(i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			if(tmp->dirty & 1)
			{
				glRenderer_UploadTexture(ddInterface->renderer,tmp->buffer,tmp->bigbuffer,tmp->texture,tmp->ddsd.dwWidth,tmp->ddsd.dwHeight,
					tmp->fakex,tmp->fakey,tmp->ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*tmp->fakex)),
					tmp->ddsd.ddpfPixelFormat.dwRGBBitCount,miplevel);
				tmp->dirty &= ~1;
			}
			tmp->dirty |= 2;
			textures[i+1] = tmp->GetTexture();
		}
		TEXTURE *tmptex = textures[0];
		memmove(textures,&textures[1],ddsd.dwBackBufferCount*sizeof(GLuint));
		textures[ddsd.dwBackBufferCount] = tmptex;
		tmp = this;
		this->SetTexture(textures[0]);
		for(DWORD i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			tmp->SetTexture(textures[i+1]);
		}
		delete[] textures;
	}
	else TRACE_RET(HRESULT,23,DDERR_NOTFLIPPABLE);
	flipcount+=flips;
	if(flipcount > ddsd.dwBackBufferCount) flipcount -= (ddsd.dwBackBufferCount+1);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSCAPS2 ddsComp;
	ZeroMemory(&ddsComp, sizeof(DDSCAPS2));
	unsigned __int64 comp1,comp2;
	if (backbuffer)
	{ 
		backbuffer->GetCaps(&ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = backbuffer;
			backbuffer->AddRef();
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}
	if(zbuffer)
	{
		zbuffer->GetCaps(&ddsComp);
		memcpy(&comp1,lpDDSCaps,sizeof(unsigned __int64));
		memcpy(&comp2,&ddsComp,sizeof(unsigned __int64));
		if((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = zbuffer;
			zbuffer->AddRef();
			TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
	}
	if (miptexture)
	{
		miptexture->GetCaps(&ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = miptexture;
			miptexture->AddRef();
			TRACE_VAR("*lplpDDAttachedSurface", 14, *lplpDDAttachedSurface);
			TRACE_EXIT(23, DD_OK);
			return DD_OK;
		}
	}

	TRACE_EXIT(23,DDERR_NOTFOUND);
	return DDERR_NOTFOUND;
}
HRESULT WINAPI glDirectDrawSurface7::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT islost = this->IsLost();
	if (islost == DDERR_SURFACELOST) return DDERR_SURFACELOST;
	// Async rendering not yet implemented
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpDDSCaps,&ddsd.ddsCaps,sizeof(DDSCAPS2));
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetClipper: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDColorKey) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags == DDCKEY_SRCBLT)
	{
		if(colorkey[0].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[0].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTBLT)
	{
		if(colorkey[1].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[1].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_SRCOVERLAY)
	{
		if(colorkey[2].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[2].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	if(dwFlags == DDCKEY_DESTOVERLAY)
	{
		if(colorkey[3].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[3].key,sizeof(DDCOLORKEY));
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else TRACE_RET(HRESULT,23,DDERR_NOCOLORKEY);
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirectDrawSurface7::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lphDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(hdc) TRACE_RET(HRESULT,23,DDERR_DCALREADYCREATED);
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
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
	FIXME("glDirectDrawSurface7::GetFlipStatus: stub\n");
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetOverlayPosition: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT err;
	if(palette)
	{
		glDirectDrawPalette_AddRef(palette);
		*lplpDDPalette = (LPDIRECTDRAWPALETTE)palette;
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
HRESULT WINAPI glDirectDrawSurface7::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDPixelFormat) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lpDDPixelFormat = ddsd.ddpfPixelFormat;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSurfaceDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(3,14,this,14,lpDD,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface7::IsLost()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(hRC == ddInterface->renderer->hRC) {TRACE_RET(HRESULT,23,DD_OK);}
	else TRACE_RET(HRESULT,23,DDERR_SURFACELOST);
}

HRESULT WINAPI glDirectDrawSurface7::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	dirty |= 1;
	retry:
	switch(surfacetype)
	{
	default:
		TRACE_EXIT(23,DDERR_GENERIC);
		ERR(DDERR_GENERIC);
		break;
	case 0:
		if(dirty & 2)
			glRenderer_DownloadTexture(ddInterface->renderer,buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
				(ddInterface->GetBPPMultipleOf8()/8)*fakex,ddsd.ddpfPixelFormat.dwRGBBitCount,miplevel);
		ddsd.lpSurface = buffer;
		dirty &= ~2;
		break;
	case 1:
		FIXME("glDirectDrawSurface7::Lock: surface type 1 not supported yet");
		TRACE_EXIT(23,DDERR_UNSUPPORTED);
		ERR(DDERR_UNSUPPORTED);
		break;
	case 2:
		buffer = (char *)malloc(ddsd.lPitch * ddsd.dwHeight);
		if((ddsd.dwWidth != fakex) || (ddsd.dwHeight != fakey))
			bigbuffer = (char *)malloc((ddsd.ddpfPixelFormat.dwRGBBitCount * NextMultipleOfWord(fakex) * fakey)/8);
		else bigbuffer = NULL;
		glRenderer_DownloadTexture(ddInterface->renderer,buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
			(ddInterface->GetBPPMultipleOf8()/8)*fakex,ddsd.ddpfPixelFormat.dwRGBBitCount,miplevel);
		dirty &= ~2;
		surfacetype = 0;
		goto retry;
	}
	if(lpDestRect)
	{
		ULONG_PTR ptr = (ULONG_PTR)ddsd.lpSurface;
		ptr += (lpDestRect->left * (ddsd.ddpfPixelFormat.dwRGBBitCount/8));
		ptr += (lpDestRect->top * (ddsd.lPitch));
		ddsd.lpSurface = (LPVOID)ptr;
	}
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	locked++;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!hdc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(hDC != hdc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	GetDIBits(hDC,hbitmap,0,ddsd.dwHeight,ddsd.lpSurface,bitmapinfo,DIB_RGB_COLORS);
	Unlock(NULL);
	DeleteObject(hbitmap);
	hbitmap = NULL;
	DeleteDC(hdc);
	hdc = NULL;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
void glDirectDrawSurface7::Restore2()
{
	TRACE_ENTER(1,14,this);
	LONG sizes[6];
	if(hRC != ddInterface->renderer->hRC)
	{
		ddInterface->GetSizes(sizes);
		if(ddInterface->GetFullscreen())
		{
			ddsd.dwWidth = sizes[2];
			ddsd.dwHeight = sizes[3];
			if(dxglcfg.highres)
			{
				fakex = sizes[0];
				fakey = sizes[1];
			}
			else
			{
				fakex = ddsd.dwWidth;
				fakey = ddsd.dwHeight;
			}
			ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
		else
		{
			fakex = ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
			fakey = ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
			ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
		}
		if(backbuffer) backbuffer->Restore2();
		if(zbuffer) zbuffer->Restore2();
		if(paltex) glRenderer_MakeTexture(ddInterface->renderer,paltex,256,1);
		glRenderer_MakeTexture(ddInterface->renderer,texture,fakex,fakey);
	}
	TRACE_EXIT(0,0);
}
HRESULT WINAPI glDirectDrawSurface7::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LONG sizes[6];
	if(!ddInterface->renderer) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(hRC != ddInterface->renderer->hRC)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			ddInterface->GetSizes(sizes);
			if(ddInterface->GetFullscreen())
			{
				ddsd.dwWidth = sizes[2];
				ddsd.dwHeight = sizes[3];
				if(dxglcfg.highres)
				{
					fakex = sizes[0];
					fakey = sizes[1];
				}
				else
				{
					fakex = ddsd.dwWidth;
					fakey = ddsd.dwHeight;
				}
				ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
			else
			{
				fakex = ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
				fakey = ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			}
			if(backbuffer) backbuffer->Restore2();
			if(zbuffer) zbuffer->Restore2();
		}
		else
		{
			if(backbuffer) backbuffer->Restore();
			if(zbuffer) zbuffer->Restore();
		}
		if(paltex) glRenderer_MakeTexture(ddInterface->renderer,paltex,256,1);
		glRenderer_MakeTexture(ddInterface->renderer,texture,fakex,fakey);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DD_OK);
}
HRESULT WINAPI glDirectDrawSurface7::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (clipper)
	{
		clipper->dirty = true;
		glDirectDrawClipper_Release(clipper);
	}
	clipper = (glDirectDrawClipper *)lpDDClipper;
	if (clipper)
	{
		glDirectDrawClipper_AddRef(clipper);
		clipper->dirty = true;
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDColorKey) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	CKEY key;
	key.enabled = true;
	if(dwFlags & DDCKEY_COLORSPACE) key.colorspace = true;
	else key.colorspace = false;
	key.key = *lpDDColorKey;
	if(dwFlags & DDCKEY_SRCBLT)
	{
		ddsd.dwFlags |= DDSD_CKSRCBLT;
		colorkey[0] = key;
	}
	if(dwFlags & DDCKEY_DESTBLT)
	{
		ddsd.dwFlags |= DDSD_CKDESTBLT;
		colorkey[1] = key;
	}
	if(dwFlags & DDCKEY_SRCOVERLAY)
	{
		ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
		colorkey[2] = key;
	}
	if(dwFlags & DDCKEY_DESTOVERLAY)
	{
		ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
		colorkey[3] = key;
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetOverlayPosition: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(palette)
	{
		glDirectDrawPalette_Release(palette);
		if(!lpDDPalette) glDirectDrawPalette_Create(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE|0x800,NULL,(LPDIRECTDRAWPALETTE*)&palette);
	}
	if(lpDDPalette)
	{
		palette = (glDirectDrawPalette *)lpDDPalette;
		glDirectDrawPalette_AddRef(palette);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7::Unlock(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!locked) TRACE_RET(HRESULT,23,DDERR_NOTLOCKED);
	locked--;
	ddsd.lpSurface = NULL;
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		{
			if(ddInterface->lastsync)
			{
				RenderScreen(texture,this,1);
				ddInterface->lastsync = false;
			}
			else RenderScreen(texture,this,0);
		}
	if (ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP)
	{
		glRenderer_UploadTexture(ddInterface->renderer, buffer, bigbuffer, texture, ddsd.dwWidth, ddsd.dwHeight,
			fakex, fakey, ddsd.lPitch, (NextMultipleOf4((ddInterface->GetBPPMultipleOf8() / 8)*fakex)),
			ddsd.ddpfPixelFormat.dwRGBBitCount, miplevel);
		dirty &= ~1;
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::UpdateOverlay: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::UpdateOverlayDisplay: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

void glDirectDrawSurface7::RenderScreen(TEXTURE *texture, glDirectDrawSurface7 *surface, int vsync)
{
	TRACE_ENTER(3,14,this,14,texture,14,surface);
	glRenderer_DrawScreen(ddInterface->renderer,texture, paltex, this, surface, vsync);
	TRACE_EXIT(0,0);
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	ddInterface->AddRef();
	*lplpDD = ddInterface;
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::PageLock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	pagelocked++;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!pagelocked) TRACE_RET(HRESULT,23,DDERR_NOTPAGELOCKED);
	pagelocked--;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
// ddraw 3+ api
HRESULT WINAPI glDirectDrawSurface7::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpddsd2,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetSurfaceDesc: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
// ddraw 4+ api
HRESULT WINAPI glDirectDrawSurface7::SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,this,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,this,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetPrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::FreePrivateData(REFGUID guidTag)
{
	TRACE_ENTER(2,14,this,24,&guidTag);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::FreePrivateData: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetUniquenessValue(LPDWORD lpValue)
{
	TRACE_ENTER(2,14,this,14,lpValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::ChangeUniquenessValue()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::ChangeUniquenessValue: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
// ddraw 7 api
HRESULT WINAPI glDirectDrawSurface7::SetPriority(DWORD dwPriority)
{
	TRACE_ENTER(2,14,this,8,dwPriority);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPriority(LPDWORD lpdwPriority)
{
	TRACE_ENTER(2,14,this,14,lpdwPriority);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetPriority: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetLOD(DWORD dwMaxLOD)
{
	TRACE_ENTER(2,14,this,8,dwMaxLOD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetLOD(LPDWORD lpdwMaxLOD)
{
	TRACE_ENTER(2,14,this,14,lpdwMaxLOD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetLOD: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Unlock2(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,Unlock((LPRECT)lpSurfaceData));
}
void glDirectDrawSurface7::SetFilter(int level, GLint mag, GLint min, glExtensions *ext, TextureManager *texman)
{
	TRACE_ENTER(4,14,this,11,level,11,mag,11,min);
	switch(dxglcfg.texfilter)
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
	if(ext->GLEXT_ARB_sampler_objects)
	{
		ext->glSamplerParameteri(texman->samplers[level].id,GL_TEXTURE_MAG_FILTER,mag);
		ext->glSamplerParameteri(texman->samplers[level].id,GL_TEXTURE_MIN_FILTER,min);
	}
	else
	{
		if(ext->GLEXT_ARB_direct_state_access)
		{
			ext->glTextureParameteri(texture->id,GL_TEXTURE_MAG_FILTER,mag);
			ext->glTextureParameteri(texture->id,GL_TEXTURE_MIN_FILTER,min);
		}
		else if (ext->GLEXT_EXT_direct_state_access)
		{
			ext->glTextureParameteriEXT(texture->id, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
			ext->glTextureParameteriEXT(texture->id, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		}
		else
		{
			TextureManager_SetTexture(texman,level,texture);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,mag);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,min);
		}
	}
	if(this)
	{
		magfilter = mag;
		minfilter = min;
	}
	TRACE_EXIT(0,0);
}

HRESULT glDirectDrawSurface7::GetHandle(glDirect3DDevice7 *glD3DDev7, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,glD3DDev7,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!glD3DDev7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(handle)
	{
		if(device != glD3DDev7) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		*lpHandle = handle;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	device = glD3DDev7;
	handle = device->AddTexture(this);
	if(handle == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	device->AddRef();
	*lpHandle = handle;
	TRACE_VAR("*lpHandle",9,*lpHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirectDrawSurface7::Load(glDirectDrawSurface7 *src)
{
	TRACE_ENTER(2,14,this,14,src);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!src) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (src == this) TRACE_RET(HRESULT, 23, DD_OK);
	this->Blt(NULL,src,NULL,DDBLT_WAIT,NULL);
	if (src->ddsd.dwFlags & DDSD_CKSRCBLT)
		this->SetColorKey(DDCKEY_SRCBLT, &src->colorkey[0].key);
	ddsd.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirectDrawSurface7::GetGammaRamp(DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, this, 9, dwFlags, 14, lpRampData);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::GetGammaRamp: stub\n");
	TRACE_EXIT(23, DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT glDirectDrawSurface7::SetGammaRamp(DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, this, 9, dwFlags, 14, lpRampData);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::SetGammaRamp: stub\n");
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
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface1::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->AddRef1());
}
ULONG WINAPI glDirectDrawSurface1::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->Release1());
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
	ret = glDDS7->AddAttachedSurface2(dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->AddOverlayDirtyRect(lpRect));
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
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if(lpDDSrcSurface) {TRACE_RET(HRESULT,23,glDDS7->Blt(lpDestRect,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx))}
	else TRACE_RET(HRESULT,23,glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface1::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface1::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDDS7->DeleteAttachedSurface(dwFlags, ((glDirectDrawSurface1*)lpDDSAttachedSurface)->GetDDS7());
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
	TRACE_RET(HRESULT,23,glDDS7->EnumAttachedSurfaces(context,EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface1::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface1::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDDS7->Flip(((glDirectDrawSurface1*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23,glDDS7->Flip(NULL,dwFlags));
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
	error = glDDS7->GetAttachedSurface(&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		attachedsurface->QueryInterface(IID_IDirectDrawSurface,(void **)&attached1);
		attachedsurface->Release();
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
	TRACE_RET(HRESULT,23,glDDS7->GetBltStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetClipper(lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetDC(lphDC));
}
HRESULT WINAPI glDirectDrawSurface1::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetFlipStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetOverlayPosition(lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface1::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPalette(lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPixelFormat(lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface1::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDDS7->GetSurfaceDesc(&ddsd);
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
	TRACE_RET(HRESULT,23,glDDS7->IsLost());
}
HRESULT WINAPI glDirectDrawSurface1::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDDS7->Lock(lpDestRect, &ddsd, dwFlags, hEvent);
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
	TRACE_RET(HRESULT,23,glDDS7->ReleaseDC(hDC));
}
HRESULT WINAPI glDirectDrawSurface1::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Restore());
}
HRESULT WINAPI glDirectDrawSurface1::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetClipper(lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface1::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface1::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetOverlayPosition(lX,lY));
}
HRESULT WINAPI glDirectDrawSurface1::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetPalette(lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface1::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Unlock2(lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayDisplay(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
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
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface2::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->AddRef2());
}
ULONG WINAPI glDirectDrawSurface2::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->Release2());
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
	ret = glDDS7->AddAttachedSurface2(dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->AddOverlayDirtyRect(lpRect));
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
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23,glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface2::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,4,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface2::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDDS7->DeleteAttachedSurface(dwFlags, ((glDirectDrawSurface2*)lpDDSAttachedSurface)->GetDDS7());
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
	TRACE_RET(HRESULT, 23, glDDS7->EnumAttachedSurfaces(context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface2::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface2::Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDDS7->Flip(((glDirectDrawSurface2*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23,glDDS7->Flip(NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface2 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDDS7->GetAttachedSurface(&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		attachedsurface->QueryInterface(IID_IDirectDrawSurface2,(void **)&attached1);
		attachedsurface->Release();
		*lplpDDAttachedSurface = attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetBltStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetClipper(lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetDC(lphDC));
}
HRESULT WINAPI glDirectDrawSurface2::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetFlipStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetOverlayPosition(lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface2::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPalette(lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPixelFormat(lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface2::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDDS7->GetSurfaceDesc(&ddsd);
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
	TRACE_RET(HRESULT,23,glDDS7->IsLost());
}
HRESULT WINAPI glDirectDrawSurface2::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDDS7->Lock(lpDestRect, &ddsd, dwFlags, hEvent);
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
	TRACE_RET(HRESULT,23,glDDS7->ReleaseDC(hDC));
}
HRESULT WINAPI glDirectDrawSurface2::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Restore());
}
HRESULT WINAPI glDirectDrawSurface2::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetClipper(lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface2::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface2::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetOverlayPosition(lX,lY));
}
HRESULT WINAPI glDirectDrawSurface2::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetPalette(lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface2::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Unlock2(lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayDisplay(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface2::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDDS7->GetDDInterface((void**)&glDD7);
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
	TRACE_RET(HRESULT,23,glDDS7->PageLock(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface2::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->PageUnlock(dwFlags));
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
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface3::AddRef()
{
	TRACE_ENTER(1,14,this);
	if (!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->AddRef3());
}
ULONG WINAPI glDirectDrawSurface3::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->Release3());
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
	ret = glDDS7->AddAttachedSurface2(dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->AddOverlayDirtyRect(lpRect));
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
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23,glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface3::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface3::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDDS7->DeleteAttachedSurface(dwFlags, ((glDirectDrawSurface3*)lpDDSAttachedSurface)->GetDDS7());
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
	TRACE_RET(HRESULT, 23, glDDS7->EnumAttachedSurfaces(context, EnumSurfacesCallback1));
}
HRESULT WINAPI glDirectDrawSurface3::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface3::Flip(LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDDS7->Flip(((glDirectDrawSurface3*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23,glDDS7->Flip(NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface3 *attached1;
	DDSCAPS2 ddscaps1;
	ddscaps1.dwCaps = lpDDSCaps->dwCaps;
	ddscaps1.dwCaps2 = ddscaps1.dwCaps3 = ddscaps1.dwCaps4 = 0;
	error = glDDS7->GetAttachedSurface(&ddscaps1,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		attachedsurface->QueryInterface(IID_IDirectDrawSurface3,(void **)&attached1);
		attachedsurface->Release();
		*lplpDDAttachedSurface = attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetBltStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetCaps(LPDDSCAPS lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDDSCaps) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetClipper(lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetDC(lphDC));
}
HRESULT WINAPI glDirectDrawSurface3::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetFlipStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetOverlayPosition(lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface3::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPalette(lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPixelFormat(lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface3::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDSurfaceDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT ret = glDDS7->GetSurfaceDesc(&ddsd);
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
	TRACE_RET(HRESULT,23,glDDS7->IsLost());
}
HRESULT WINAPI glDirectDrawSurface3::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDSURFACEDESC2 ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	memcpy(&ddsd, lpDDSurfaceDesc, sizeof(DDSURFACEDESC));
	HRESULT ret = glDDS7->Lock(lpDestRect, &ddsd, dwFlags, hEvent);
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
	TRACE_RET(HRESULT,23,glDDS7->ReleaseDC(hDC));
}
HRESULT WINAPI glDirectDrawSurface3::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Restore());
}
HRESULT WINAPI glDirectDrawSurface3::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetClipper(lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface3::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface3::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetOverlayPosition(lX,lY));
}
HRESULT WINAPI glDirectDrawSurface3::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetPalette(lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface3::Unlock(LPVOID lpSurfaceData)
{
	TRACE_ENTER(2,14,this,14,lpSurfaceData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Unlock2(lpSurfaceData));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayDisplay(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface3::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	HRESULT ret = glDDS7->GetDDInterface((void**)&glDD7);
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
	TRACE_RET(HRESULT,23,glDDS7->PageLock(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->PageUnlock(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface3::SetSurfaceDesc(LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpddsd,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetSurfaceDesc((LPDDSURFACEDESC2)lpddsd,dwFlags));
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
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}
ULONG WINAPI glDirectDrawSurface4::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->AddRef4());
}
ULONG WINAPI glDirectDrawSurface4::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->Release4());
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
	ret = glDDS7->AddAttachedSurface2(dds7, lpDDSAttachedSurface);
	if (ret == DD_OK) ((IUnknown*)lpDDSAttachedSurface)->AddRef();
	dds7->Release();
	TRACE_RET(HRESULT, 23, ret);
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->AddOverlayDirtyRect(lpRect));
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
			if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, &bltfx)) }
			else TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, NULL, lpSrcRect, dwFlags, &bltfx));
		}
	}
	if (lpDDSrcSurface) { TRACE_RET(HRESULT, 23, glDDS7->Blt(lpDestRect, ((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(), lpSrcRect, dwFlags, lpDDBltFx)); }
	else TRACE_RET(HRESULT,23,glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx));
}
HRESULT WINAPI glDirectDrawSurface4::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDDBltBatch,8,dwCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	TRACE_ENTER(6,14,this,8,dwX,8,dwY,14,lpDDSrcSurface,26,lpSrcRect,9,dwTrans);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans));
}
HRESULT WINAPI glDirectDrawSurface4::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT ret = glDDS7->DeleteAttachedSurface(dwFlags, ((glDirectDrawSurface4*)lpDDSAttachedSurface)->GetDDS7());
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
	TRACE_RET(HRESULT, 23, glDDS7->EnumAttachedSurfaces(context, EnumSurfacesCallback2));
}
HRESULT WINAPI glDirectDrawSurface4::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback));
}
HRESULT WINAPI glDirectDrawSurface4::Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDDSurfaceTargetOverride)
		{TRACE_RET(HRESULT,23,glDDS7->Flip(((glDirectDrawSurface4*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags));}
	else TRACE_RET(HRESULT,23,glDDS7->Flip(NULL,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
	TRACE_ENTER(3,14,this,14,lpDDSCaps2,14,lplpDDAttachedSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface4 *attached1;
	error = glDDS7->GetAttachedSurface(lpDDSCaps2,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		attachedsurface->QueryInterface(IID_IDirectDrawSurface4,(void **)&attached1);
		attachedsurface->Release();
		*lplpDDAttachedSurface = attached1;
		TRACE_VAR("*lplpDDAttachedSurface",14,*lplpDDAttachedSurface);
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface4::GetBltStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetBltStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	TRACE_ENTER(2,14,this,14,lpDDSCaps);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetCaps(lpDDSCaps));
}
HRESULT WINAPI glDirectDrawSurface4::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lplpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetClipper(lplpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4::GetDC(HDC FAR *lphDC)
{
	TRACE_ENTER(2,14,this,14,lphDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetDC(lphDC));
}
HRESULT WINAPI glDirectDrawSurface4::GetFlipStatus(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetFlipStatus(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	TRACE_ENTER(3,14,this,14,lplX,14,lplY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetOverlayPosition(lplX,lplY));
}
HRESULT WINAPI glDirectDrawSurface4::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lplpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPalette(lplpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	TRACE_ENTER(2,14,this,14,lpDDPixelFormat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPixelFormat(lpDDPixelFormat));
}
HRESULT WINAPI glDirectDrawSurface4::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	TRACE_ENTER(2,14,this,14,lpDDSurfaceDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetSurfaceDesc(lpDDSurfaceDesc));
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
	TRACE_RET(HRESULT,23,glDDS7->IsLost());
}
HRESULT WINAPI glDirectDrawSurface4::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	TRACE_ENTER(5,14,this,26,lpDestRect,14,lpDDSurfaceDesc,9,dwFlags,14,hEvent);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent));
}
HRESULT WINAPI glDirectDrawSurface4::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->ReleaseDC(hDC));
}
HRESULT WINAPI glDirectDrawSurface4::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Restore());
}
HRESULT WINAPI glDirectDrawSurface4::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	TRACE_ENTER(2,14,this,14,lpDDClipper);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetClipper(lpDDClipper));
}
HRESULT WINAPI glDirectDrawSurface4::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetColorKey(dwFlags,lpDDColorKey));
}
HRESULT WINAPI glDirectDrawSurface4::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetOverlayPosition(lX,lY));
}
HRESULT WINAPI glDirectDrawSurface4::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetPalette(lpDDPalette));
}
HRESULT WINAPI glDirectDrawSurface4::Unlock(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->Unlock2(lpRect));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayDisplay(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference));
}
HRESULT WINAPI glDirectDrawSurface4::GetDDInterface(LPVOID FAR *lplpDD)
{
	TRACE_ENTER(2,14,this,14,lplpDD);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetDDInterface(lplpDD));
}
HRESULT WINAPI glDirectDrawSurface4::PageLock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->PageLock(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::PageUnlock(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->PageUnlock(dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpddsd,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetSurfaceDesc(lpddsd,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::SetPrivateData(REFGUID guidTag, LPVOID lpData, DWORD cbSize, DWORD dwFlags)
{
	TRACE_ENTER(5,14,this,24,&guidTag,14,lpData,8,cbSize,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->SetPrivateData(guidTag,lpData,cbSize,dwFlags));
}
HRESULT WINAPI glDirectDrawSurface4::GetPrivateData(REFGUID guidTag, LPVOID lpBuffer, LPDWORD lpcbBufferSize)
{
	TRACE_ENTER(4,14,this,24,&guidTag,14,lpBuffer,14,lpcbBufferSize);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetPrivateData(guidTag,lpBuffer,lpcbBufferSize));
}
HRESULT WINAPI glDirectDrawSurface4::FreePrivateData(REFGUID guidTag)
{
	TRACE_ENTER(2,14,this,24,&guidTag);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->FreePrivateData(guidTag));
}
HRESULT WINAPI glDirectDrawSurface4::GetUniquenessValue(LPDWORD lpValue)
{
	TRACE_ENTER(2,14,this,14,lpValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->GetUniquenessValue(lpValue));
}
HRESULT WINAPI glDirectDrawSurface4::ChangeUniquenessValue()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDDS7->ChangeUniquenessValue());
}
