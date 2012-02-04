// DXGL
// Copyright (C) 2011-2012 William Feely

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
#include "shaders.h"
#include "shadergen.h"
#include "ddraw.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glDirectDrawClipper.h"
#include "glRenderer.h"
#include "glutil.h"


static inline int NextMultipleOf8(int number){return ((number+7) & (~7));}
static inline int NextMultipleOf4(int number){return ((number+3) & (~3));}
static inline int NextMultipleOf2(int number){return ((number+1) & (~1));}
#ifdef _M_X64
#define NextMultipleOfWord NextMultipleOf8
#else
#define NextMultipleOfWord NextMultipleOf4
#endif



// DDRAW7 routines
glDirectDrawSurface7::glDirectDrawSurface7(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 *lplpDDSurface7, HRESULT *error, bool copysurface, glDirectDrawPalette *palettein)
{
	hasstencil = false;
	dirty = 2;
	locked = 0;
	pagelocked = 0;
	flipcount = 0;
	ZeroMemory(colorkey,4*sizeof(CKEY));
	bitmapinfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO)+(255*sizeof(RGBQUAD)));
	ZeroMemory(bitmapinfo,sizeof(BITMAPINFO)+(255*sizeof(RGBQUAD)));
	palette = NULL;
	paltex = NULL;
	texture = NULL;
	clipper = NULL;
	hdc = NULL;
	dds1 = NULL;
	dds2 = NULL;
	dds3 = NULL;
	dds4 = NULL;
	buffer = gdibuffer = NULL;
	bigbuffer = NULL;
	zbuffer = NULL;
	DWORD colormasks[3];
	filter = GL_NEAREST;
	if(copysurface)
	{
		FIXME("glDirectDrawSurface7::glDirectDrawSurface7: copy surface stub\n");
		*error = DDERR_GENERIC;
		return;
	}
	else
	{
		ddInterface = (glDirectDraw7 *)lpDD7;
		renderer = ddInterface->renderer;
		hRC = ddInterface->renderer->hRC;
		ddsd = *lpDDSurfaceDesc2;
	}
	LONG sizes[6];
	ddInterface->GetSizes(sizes);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(((ddsd.dwFlags & DDSD_WIDTH) || (ddsd.dwFlags & DDSD_HEIGHT)
			|| (ddsd.dwFlags & DDSD_PIXELFORMAT)) && !(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER))
		{
			*error = DDERR_INVALIDPARAMS;
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
			if(!palettein) palette = new glDirectDrawPalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE,NULL,NULL);
			else
			{
				palette = palettein;
				palette->AddRef();
			}
			paltex = renderer->MakeTexture(GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP,256,1,GL_RGBA,GL_UNSIGNED_BYTE,GL_RGB);
		}
		else paltex = 0;
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
		bitmapinfo->bmiHeader.biBitCount = (WORD)ddInterface->GetBPPMultipleOf8();
	}
	surfacetype=2;
	bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapinfo->bmiHeader.biWidth = ddsd.dwWidth;
	bitmapinfo->bmiHeader.biHeight = -(signed)ddsd.dwHeight;
	bitmapinfo->bmiHeader.biPlanes = 1;
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
	maketex:
		buffer = NULL;
		if((dxglcfg.scalingfilter == 0) || (ddInterface->GetBPP() == 8)) filter = GL_NEAREST;
		else filter = GL_LINEAR;
		if(ddsd.dwFlags & DDSD_PIXELFORMAT)
		{
			if(ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
			{
				switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
				{
				case 8:
					if(ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
					{
						texformat = GL_LUMINANCE;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_LUMINANCE8;
						else texformat3 = GL_RGBA8;
						if(!palettein) palette = new glDirectDrawPalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE,NULL,NULL);
						bitmapinfo->bmiHeader.biBitCount = 8;
					}
					else
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_BYTE_3_3_2;
						if(dxglcfg.texformat) texformat3 = GL_R3_G3_B2;
						else texformat3 = GL_RGBA8;
					}
					ddsd.ddpfPixelFormat.dwRBitMask = 0;
					ddsd.ddpfPixelFormat.dwGBitMask = 0;
					ddsd.ddpfPixelFormat.dwBBitMask = 0;
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth);
					break;
				case 16:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0x7C00) && (ddsd.ddpfPixelFormat.dwGBitMask == 0x3E0)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0x1F))
					{
						texformat = GL_BGRA;
						texformat2 = GL_UNSIGNED_SHORT_1_5_5_5_REV;
						if(dxglcfg.texformat) texformat3 = GL_RGB5_A1;
						else texformat3 = GL_RGBA8;
					}
					else // fixme:  support more formats
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_SHORT_5_6_5;
						if(dxglcfg.texformat) texformat3 = GL_RGB;
						else texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
					break;
				case 24:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000) && (ddsd.ddpfPixelFormat.dwGBitMask == 0xFF00)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0xFF))
					{
						texformat = GL_BGR;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_RGB8;
						else texformat3 = GL_RGBA8;
					}
					else // fixme:  support more formats
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_RGB8;
						else texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*3);
					break;
				case 32:
				default:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000) && (ddsd.ddpfPixelFormat.dwGBitMask == 0xFF00)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0xFF))
					{
						texformat = GL_BGRA;
						texformat2 = GL_UNSIGNED_BYTE;
						texformat3 = GL_RGBA8;
					}
					else // fixme: support more formats
					{
						texformat = GL_RGBA;
						texformat2 = GL_UNSIGNED_BYTE;
						texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*4);
				}
			}
			else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)
			{
				switch(ddsd.ddpfPixelFormat.dwZBufferBitDepth)
				{
				case 16:
				default:
					texformat = GL_DEPTH_COMPONENT;
					texformat2 = GL_UNSIGNED_BYTE;
					texformat3 = GL_DEPTH_COMPONENT16;
					break;
				case 24:
					texformat = GL_DEPTH_COMPONENT;
					texformat2 = GL_UNSIGNED_BYTE;
					texformat3 = GL_DEPTH_COMPONENT24;
					break;
				case 32:
					if((ddsd.ddpfPixelFormat.dwRGBZBitMask == 0x00ffffff) &&
						!(ddsd.ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER))
					{
						texformat = GL_DEPTH_COMPONENT;
						texformat2 = GL_UNSIGNED_INT;
						texformat3 = GL_DEPTH_COMPONENT24;
						break;
					}
					else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER)
					{
						texformat = GL_DEPTH_STENCIL;
						texformat2 = GL_UNSIGNED_INT_24_8;
						texformat3 = GL_DEPTH24_STENCIL8;
						hasstencil = true;
						break;
					}
					else
					{
						texformat = GL_DEPTH_COMPONENT;
						texformat2 = GL_UNSIGNED_INT;
						texformat3 = GL_DEPTH_COMPONENT32;
						break;
					}
				}
			}
		}
		else
		{
			ddsd.ddpfPixelFormat.dwRGBBitCount = ddInterface->GetBPP();
			switch(ddInterface->GetBPP())
			{
			case 8:
				texformat = GL_LUMINANCE;
				texformat2 = GL_UNSIGNED_BYTE;
				if(dxglcfg.texformat) texformat3 = GL_LUMINANCE8;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
				ddsd.ddpfPixelFormat.dwRBitMask = 0;
				ddsd.ddpfPixelFormat.dwGBitMask = 0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth);
				break;
			case 15:
				texformat = GL_BGRA;
				texformat2 = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				if(dxglcfg.texformat) texformat3 = GL_RGB5_A1;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
				break;
			case 16:
				texformat = GL_RGB;
				texformat2 = GL_UNSIGNED_SHORT_5_6_5;
				if(dxglcfg.texformat) texformat3 = GL_RGB;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				break;
			case 24:
				texformat = GL_BGR;
				texformat2 = GL_UNSIGNED_BYTE;
				if(dxglcfg.texformat) texformat3 = GL_RGB8;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*3);
				break;
			case 32:
				texformat = GL_BGRA;
				texformat2 = GL_UNSIGNED_BYTE;
				texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*4);
				break;
			default:
				*error = DDERR_INVALIDPIXELFORMAT;
				return;
			}
			if(ddInterface->GetBPP() > 8)
			{
				colormasks[0] = ddsd.ddpfPixelFormat.dwRBitMask;
				colormasks[1] = ddsd.ddpfPixelFormat.dwGBitMask;
				colormasks[2] = ddsd.ddpfPixelFormat.dwBBitMask;
				memcpy(bitmapinfo->bmiColors,colormasks,3*sizeof(DWORD));
			}
		}
		texture = renderer->MakeTexture(filter,filter,GL_CLAMP,GL_CLAMP,fakex,fakey,texformat,texformat2,texformat3);
	}

	refcount = 1;
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
				glDirectDrawSurface7 *tmp;
				backbuffer = new glDirectDrawSurface7(ddInterface,&ddsdBack,(LPDIRECTDRAWSURFACE7 *)&tmp,error,false,palette);
			}
			else if (ddsd.dwFlags & DDSD_BACKBUFFERCOUNT){}
			else *error = DDERR_INVALIDPARAMS;
		}
	}
}
glDirectDrawSurface7::~glDirectDrawSurface7()
{
	AddRef();
	if(dds1) dds1->Release();
	if(dds2) dds2->Release();
	if(dds3) dds3->Release();
	if(dds4) dds4->Release();
	if(paltex) renderer->DeleteTexture(paltex);
	if(texture) renderer->DeleteTexture(texture);
	if(bitmapinfo) free(bitmapinfo);
	if(palette) palette->Release();
	if(backbuffer) backbuffer->Release();
	if(buffer) free(buffer);
	if(bigbuffer) free(bigbuffer);
	if(zbuffer) zbuffer->Release();
	ddInterface->DeleteSurface(this);
}
HRESULT WINAPI glDirectDrawSurface7::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IDirectDrawSurface7)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	if(riid == IID_IDirectDrawSurface4)
	{
		if(dds4)
		{
			*ppvObj = dds4;
			dds4->AddRef();
			return DD_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirectDrawSurface4(this);
			dds4 = (glDirectDrawSurface4*)*ppvObj;
			return DD_OK;
		}
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		if(dds3)
		{
			*ppvObj = dds3;
			dds3->AddRef();
			return DD_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirectDrawSurface3(this);
			dds3 = (glDirectDrawSurface3*)*ppvObj;
			return DD_OK;
		}
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		if(dds2)
		{
			*ppvObj = dds2;
			dds2->AddRef();
			return DD_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirectDrawSurface2(this);
			dds2 = (glDirectDrawSurface2*)*ppvObj;
			return DD_OK;
		}
	}
	if(riid == IID_IDirectDrawSurface)
	{
		if(dds1)
		{
			*ppvObj = dds1;
			dds1->AddRef();
			return DD_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirectDrawSurface1(this);
			dds1 = (glDirectDrawSurface1*)*ppvObj;
			return DD_OK;
		}
	}
	ERR(E_NOINTERFACE);
}
ULONG WINAPI glDirectDrawSurface7::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface7::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface7::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(zbuffer) ERR(DDERR_SURFACEALREADYATTACHED);
	glDirectDrawSurface7 *attached = (glDirectDrawSurface7 *)lpDDSAttachedSurface;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	attached->GetSurfaceDesc(&ddsd);
	if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)
	{
		attached->AddRef();
		zbuffer = attached;
		return DD_OK;
	}
	else return DDERR_CANNOTATTACHSURFACE;
}
HRESULT WINAPI glDirectDrawSurface7::AddOverlayDirtyRect(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::AddOverlayDirtyRect: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) return DDERR_INVALIDPARAMS;
	glDirectDrawSurface7 *src = (glDirectDrawSurface7 *)lpDDSrcSurface;
	if(dirty & 1)
	{
		renderer->UploadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
			fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
			ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2,texformat3);
		dirty &= ~1;
	}
	if(src && (src->dirty & 1))
	{
		renderer->UploadTexture(src->buffer,src->bigbuffer,src->texture,src->ddsd.dwWidth,src->ddsd.dwHeight,
			src->fakex,src->fakey,src->ddsd.lPitch,
			(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*src->fakex)),
			src->ddsd.ddpfPixelFormat.dwRGBBitCount,src->texformat,src->texformat2,src->texformat3);
		src->dirty &= ~1;
	}
	return renderer->Blt(lpDestRect,src,this,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface7::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::BltBatch: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	return this->Blt(&dest,lpDDSrcSurface,lpSrcRect,flags,NULL);
}
HRESULT WINAPI glDirectDrawSurface7::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSAttachedSurface == (LPDIRECTDRAWSURFACE7)zbuffer)
	{
		zbuffer->Release();
		zbuffer = NULL;
		return DD_OK;
	}
	else ERR(DDERR_SURFACENOTATTACHED);
}
HRESULT WINAPI glDirectDrawSurface7::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::EnumAttachedSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::EnumOverlayZOrders: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT ret = Flip2(lpDDSurfaceTargetOverride,dwFlags);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) RenderScreen(texture,this);
	return ret;
}
HRESULT glDirectDrawSurface7::Flip2(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
		if(lpDDSurfaceTargetOverride == this) return DD_OK;
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
		if(!success) return DDERR_INVALIDPARAMS;
		for(DWORD x = 0; x < i; x++)
		{
			if(x == i-1) return Flip2(NULL,dwFlags);
			else Flip2(NULL,0);
		}
	}
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) return DDERR_INVALIDOBJECT;
		GLuint *textures = new GLuint[ddsd.dwBackBufferCount+1];
		textures[0] = texture;
		tmp = this;
		if(dirty & 1)
		{
			renderer->UploadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
				fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
				ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2,texformat3);
			dirty &= ~1;
		}
		this->dirty |= 2;
		for(i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			if(tmp->dirty & 1)
			{
				renderer->UploadTexture(tmp->buffer,tmp->bigbuffer,tmp->texture,tmp->ddsd.dwWidth,tmp->ddsd.dwHeight,
					tmp->fakex,tmp->fakey,tmp->ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*tmp->fakex)),
					tmp->ddsd.ddpfPixelFormat.dwRGBBitCount,tmp->texformat,tmp->texformat2,tmp->texformat3);
				tmp->dirty &= ~1;
			}
			tmp->dirty |= 2;
			textures[i+1] = tmp->GetTexture();
		}
		GLuint tmptex = textures[0];
		memmove(textures,&textures[1],ddsd.dwBackBufferCount*sizeof(GLuint));
		textures[ddsd.dwBackBufferCount] = tmptex;
		tmp = this;
		this->SetTexture(textures[0]);
		for(DWORD i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			tmp->SetTexture(textures[i+1]);
		}
		delete textures;
	}
	else return DDERR_NOTFLIPPABLE;
	flipcount+=flips;
	if(flipcount > ddsd.dwBackBufferCount) flipcount -= (ddsd.dwBackBufferCount+1);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	DDSCAPS2 ddsComp;
	backbuffer->GetCaps(&ddsComp);
	unsigned __int64 comp1,comp2;
	memcpy(&comp1,lpDDSCaps,sizeof(unsigned __int64));
	memcpy(&comp2,&ddsComp,sizeof(unsigned __int64));
	if((comp1 & comp2) == comp1)
	{
		*lplpDDAttachedSurface = backbuffer;
		backbuffer->AddRef();
		return DD_OK;
	}
	else
	{
		zbuffer->GetCaps(&ddsComp);
		memcpy(&comp1,lpDDSCaps,sizeof(unsigned __int64));
		memcpy(&comp2,&ddsComp,sizeof(unsigned __int64));
		if((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = zbuffer;
			zbuffer->AddRef();
			return DD_OK;
		}
		ERR(DDERR_NOTFOUND);
	}
}
HRESULT WINAPI glDirectDrawSurface7::GetBltStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetBltStatus: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	if(!this) return DDERR_INVALIDPARAMS;
	memcpy(lpDDSCaps,&ddsd.ddsCaps,sizeof(DDSCAPS2));
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetClipper: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(dwFlags == DDCKEY_SRCBLT)
	{
		if(colorkey[0].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[0].key,sizeof(DDCOLORKEY));
			return DD_OK;
		}
		else return DDERR_NOCOLORKEY;
	}
	if(dwFlags == DDCKEY_DESTBLT)
	{
		if(colorkey[1].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[1].key,sizeof(DDCOLORKEY));
			return DD_OK;
		}
		else return DDERR_NOCOLORKEY;
	}
	if(dwFlags == DDCKEY_SRCOVERLAY)
	{
		if(colorkey[2].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[2].key,sizeof(DDCOLORKEY));
			return DD_OK;
		}
		else return DDERR_NOCOLORKEY;
	}
	if(dwFlags == DDCKEY_DESTOVERLAY)
	{
		if(colorkey[3].enabled)
		{
			memcpy(lpDDColorKey,&colorkey[3].key,sizeof(DDCOLORKEY));
			return DD_OK;
		}
		else return DDERR_NOCOLORKEY;
	}
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirectDrawSurface7::GetDC(HDC FAR *lphDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(hdc) ERR(DDERR_DCALREADYCREATED);
	DWORD colors[256];
	HRESULT error;
	LPVOID surface;
	error = this->Lock(NULL,&ddsd,0,NULL);
	if(error != DD_OK) return error;
	hdc = CreateCompatibleDC(NULL);
	bitmapinfo->bmiHeader.biWidth = ddsd.lPitch / (bitmapinfo->bmiHeader.biBitCount / 8);
	if(ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		memcpy(colors,palette->GetPalette(NULL),1024);
		for(int i = 0; i < 256; i++)
			colors[i] = ((colors[i]&0x0000FF)<<16) | (colors[i]&0x00FF00) | ((colors[i]&0xFF0000)>>16);
		memcpy(bitmapinfo->bmiColors,colors,1024);
	}
	else  if(ddsd.ddpfPixelFormat.dwRGBBitCount == 16)bitmapinfo->bmiHeader.biCompression = BI_BITFIELDS;
	hbitmap = CreateDIBSection(hdc,bitmapinfo,DIB_RGB_COLORS,&surface,NULL,0);
	memcpy(surface,ddsd.lpSurface,ddsd.lPitch*ddsd.dwHeight);
	HGDIOBJ temp = SelectObject(hdc,hbitmap);
	DeleteObject(temp);
	*lphDC = hdc;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetFlipStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DD_OK;
	FIXME("glDirectDrawSurface7::GetFlipStatus: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetOverlayPosition: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT err;
	if(palette)
	{
		palette->AddRef();
		*lplpDDPalette = palette;
		err = DD_OK;
	}
	else
	{
		err = DDERR_NOPALETTEATTACHED;
		*lplpDDPalette = NULL;
	}
	return err;
}
HRESULT WINAPI glDirectDrawSurface7::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if(!this) return DDERR_INVALIDPARAMS;
	*lpDDPixelFormat = ddsd.ddpfPixelFormat;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpDDSurfaceDesc) ERR(DDERR_INVALIDPARAMS);
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface7::IsLost()
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(hRC == ddInterface->renderer->hRC) return DD_OK;
	else return DDERR_SURFACELOST;
}

HRESULT WINAPI glDirectDrawSurface7::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(locked) ERR(DDERR_SURFACEBUSY);
	dirty |= 1;
	retry:
	switch(surfacetype)
	{
	default:
		ERR(DDERR_GENERIC);
		break;
	case 0:
		if(dirty & 2)
			renderer->DownloadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
				(ddInterface->GetBPPMultipleOf8()/8)*fakex,ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2);
		ddsd.lpSurface = buffer;
		break;
	case 1:
		FIXME("glDirectDrawSurface7::Lock: surface type 1 not supported yet");
		ERR(DDERR_UNSUPPORTED);
		break;
	case 2:
		buffer = (char *)malloc(ddsd.lPitch * ddsd.dwHeight);
		if((ddsd.dwWidth != fakex) || (ddsd.dwHeight != fakey))
			bigbuffer = (char *)malloc((ddsd.ddpfPixelFormat.dwRGBBitCount * NextMultipleOfWord(fakex) * fakey)/8);
		else bigbuffer = NULL;
		renderer->DownloadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
			(ddInterface->GetBPPMultipleOf8()/8)*fakex,ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2);
		dirty &= ~2;
		surfacetype = 0;
		goto retry;
	}
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	locked++;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::ReleaseDC(HDC hDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!hdc) return DDERR_INVALIDOBJECT;
	if(hDC != hdc) return DDERR_INVALIDOBJECT;
	GetDIBits(hDC,hbitmap,0,ddsd.dwHeight,ddsd.lpSurface,bitmapinfo,DIB_RGB_COLORS);
	Unlock(NULL);
	DeleteObject(hbitmap);
	hbitmap = NULL;
	DeleteDC(hdc);
	hdc = NULL;
	return DD_OK;
}
void glDirectDrawSurface7::Restore2()
{
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
		if(paltex) paltex = renderer->MakeTexture(GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP,256,1,GL_RGBA,GL_UNSIGNED_BYTE,GL_RGB);
		texture = renderer->MakeTexture(filter,filter,GL_CLAMP,GL_CLAMP,fakex,fakey,texformat,texformat2,texformat3);
	}
}
HRESULT WINAPI glDirectDrawSurface7::Restore()
{
	if(!this) return DDERR_INVALIDPARAMS;
	LONG sizes[6];
	if(!ddInterface->renderer) return DDERR_INVALIDOBJECT;
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
		if(paltex) paltex = renderer->MakeTexture(GL_NEAREST,GL_NEAREST,GL_CLAMP,GL_CLAMP,256,1,GL_RGBA,GL_UNSIGNED_BYTE,GL_RGB);
		texture = renderer->MakeTexture(filter,filter,GL_CLAMP,GL_CLAMP,fakex,fakey,texformat,texformat2,texformat3);
		return DD_OK;
	}
	else return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(clipper) clipper->Release();
	clipper = (glDirectDrawClipper *)lpDDClipper;
	if(clipper)clipper->AddRef();
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	CKEY key;
	key.enabled = true;
	if(dwFlags & DDCKEY_COLORSPACE) key.colorspace = true;
	else key.colorspace = false;
	key.key = *lpDDColorKey;
	if(dwFlags & DDCKEY_SRCBLT) colorkey[0] = key;
	if(dwFlags & DDCKEY_DESTBLT) colorkey[1] = key;
	if(dwFlags & DDCKEY_SRCOVERLAY) colorkey[2] = key;
	if(dwFlags & DDCKEY_DESTOVERLAY) colorkey[3] = key;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetOverlayPosition(LONG lX, LONG lY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::SetOverlayPosition: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(palette)
	{
		palette->Release();
		if(!lpDDPalette) palette = new glDirectDrawPalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE,NULL,NULL);
	}
	if(lpDDPalette)
	{
		palette = (glDirectDrawPalette *)lpDDPalette;
		palette->AddRef();
	}
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7::Unlock(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!locked) return DDERR_NOTLOCKED;
	locked--;
	ddsd.lpSurface = NULL;
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP))) RenderScreen(texture,this);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::UpdateOverlay: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayDisplay(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::UpdateOverlayDisplay: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	ERR(DDERR_GENERIC);
}

void glDirectDrawSurface7::RenderScreen(GLuint texture, glDirectDrawSurface7 *surface)
{
	renderer->DrawScreen(texture,paltex,this,surface);
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7::GetDDInterface(LPVOID FAR *lplpDD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	*lplpDD = ddInterface;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::PageLock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	pagelocked++;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::PageUnlock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!pagelocked) ERR(DDERR_NOTPAGELOCKED);
	pagelocked--;
	return DD_OK;
}
// ddraw 3+ api
HRESULT WINAPI glDirectDrawSurface7::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::SetSurfaceDesc: stub\n");
	ERR(DDERR_GENERIC);
}
// ddraw 4+ api
HRESULT WINAPI glDirectDrawSurface7::SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::SetPrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetPrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::FreePrivateData(REFGUID guidTag)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::FreePrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetUniquenessValue(LPDWORD lpValue)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetUniquenessValue: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::ChangeUniquenessValue()
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::ChangeUniquenessValue: stub\n");
	ERR(DDERR_GENERIC);
}
// ddraw 7 api
HRESULT WINAPI glDirectDrawSurface7::SetPriority(DWORD dwPriority)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::SetPriority: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPriority(LPDWORD lpdwPriority)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetPriority: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetLOD(DWORD dwMaxLOD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::SetLOD: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetLOD(LPDWORD lpdwMaxLOD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirectDrawSurface7::GetLOD: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Unlock2(LPVOID lpSurfaceData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return Unlock((LPRECT)lpSurfaceData);
}

// DDRAW1 wrapper
glDirectDrawSurface1::glDirectDrawSurface1(glDirectDrawSurface7 *gl_DDS7)
{
	glDDS7 = gl_DDS7;
	refcount = 1;
}
glDirectDrawSurface1::~glDirectDrawSurface1()
{
	glDDS7->dds1 = NULL;
	glDDS7->Release();
}
HRESULT WINAPI glDirectDrawSurface1::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface1::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface1::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface1::AddOverlayDirtyRect(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface1::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface1::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface1::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface1::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface1::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface1::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSurfaceTargetOverride)
		return glDDS7->Flip(((glDirectDrawSurface1*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags);
	else return glDDS7->Flip(NULL,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	}
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetBltStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetCaps(LPDDSCAPS lpDDSCaps)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface1::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface1::GetDC(HDC FAR *lphDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface1::GetFlipStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface1::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface1::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface1::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface1::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface1::IsLost()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface1::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface1::ReleaseDC(HDC hDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface1::Restore()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface1::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface1::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface1::SetOverlayPosition(LONG lX, LONG lY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface1::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface1::Unlock(LPVOID lpSurfaceData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayDisplay(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}

// DDRAW2 wrapper
glDirectDrawSurface2::glDirectDrawSurface2(glDirectDrawSurface7 *gl_DDS7)
{
	glDDS7 = gl_DDS7;
	refcount = 1;
}
glDirectDrawSurface2::~glDirectDrawSurface2()
{
	glDDS7->dds2 = NULL;
	glDDS7->Release();
}
HRESULT WINAPI glDirectDrawSurface2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::AddAttachedSurface(LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface2::AddOverlayDirtyRect(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface2::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface2::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface2::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface2::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface2::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface2::Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSurfaceTargetOverride)
		return glDDS7->Flip(((glDirectDrawSurface2*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags);
	else return glDDS7->Flip(NULL,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	}
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetBltStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetCaps(LPDDSCAPS lpDDSCaps)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface2::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface2::GetDC(HDC FAR *lphDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface2::GetFlipStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface2::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface2::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface2::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface2::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface2::IsLost()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface2::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface2::ReleaseDC(HDC hDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface2::Restore()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface2::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface2::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface2::SetOverlayPosition(LONG lX, LONG lY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface2::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface2::Unlock(LPVOID lpSurfaceData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayDisplay(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface2::GetDDInterface(LPVOID FAR *lplpDD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface2::PageLock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::PageUnlock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageUnlock(dwFlags);
}

// DDRAW3 wrapper
glDirectDrawSurface3::glDirectDrawSurface3(glDirectDrawSurface7 *gl_DDS7)
{
	glDDS7 = gl_DDS7;
	refcount = 1;
}
glDirectDrawSurface3::~glDirectDrawSurface3()
{
	glDDS7->dds3 = NULL;
	glDDS7->Release();
}
HRESULT WINAPI glDirectDrawSurface3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface3::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface3::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::AddAttachedSurface(LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface3::AddOverlayDirtyRect(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface3::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface3::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface3::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface3::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface3::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface3::Flip(LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSurfaceTargetOverride)
		return glDDS7->Flip(((glDirectDrawSurface3*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags);
	else return glDDS7->Flip(NULL,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	}
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetBltStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetCaps(LPDDSCAPS lpDDSCaps)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface3::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface3::GetDC(HDC FAR *lphDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface3::GetFlipStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface3::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface3::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface3::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface3::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface3::IsLost()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface3::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface3::ReleaseDC(HDC hDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface3::Restore()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface3::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface3::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface3::SetOverlayPosition(LONG lX, LONG lY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface3::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface3::Unlock(LPVOID lpSurfaceData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayDisplay(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface3::GetDDInterface(LPVOID FAR *lplpDD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface3::PageLock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::PageUnlock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageUnlock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::SetSurfaceDesc(LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetSurfaceDesc((LPDDSURFACEDESC2)lpddsd,dwFlags);
}

// DDRAW4 wrapper
glDirectDrawSurface4::glDirectDrawSurface4(glDirectDrawSurface7 *gl_DDS7)
{
	glDDS7 = gl_DDS7;
	refcount = 1;
}
glDirectDrawSurface4::~glDirectDrawSurface4()
{
	glDDS7->dds4 = NULL;
	glDDS7->Release();
}
HRESULT WINAPI glDirectDrawSurface4::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface4::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface4::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4::AddAttachedSurface(LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface4::AddOverlayDirtyRect(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface4::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface4::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface4::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface4::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface4::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface4::Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(lpDDSurfaceTargetOverride)
		return glDDS7->Flip(((glDirectDrawSurface4*)lpDDSurfaceTargetOverride)->GetDDS7(),dwFlags);
	else return glDDS7->Flip(NULL,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT error;
	glDirectDrawSurface7 *attachedsurface;
	glDirectDrawSurface4 *attached1;
	error = glDDS7->GetAttachedSurface(lpDDSCaps2,(LPDIRECTDRAWSURFACE7 FAR *)&attachedsurface);
	if(error == DD_OK)
	{
		attachedsurface->QueryInterface(IID_IDirectDrawSurface4,(void **)&attached1);
		attachedsurface->Release();
		*lplpDDAttachedSurface = attached1;
	}
	return error;
}
HRESULT WINAPI glDirectDrawSurface4::GetBltStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetCaps(lpDDSCaps);
}
HRESULT WINAPI glDirectDrawSurface4::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface4::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface4::GetDC(HDC FAR *lphDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface4::GetFlipStatus(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface4::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface4::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface4::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetSurfaceDesc(lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface4::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface4::IsLost()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface4::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface4::ReleaseDC(HDC hDC)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface4::Restore()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface4::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface4::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface4::SetOverlayPosition(LONG lX, LONG lY)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface4::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface4::Unlock(LPRECT lpRect)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->Unlock2(lpRect);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayDisplay(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface4::GetDDInterface(LPVOID FAR *lplpDD)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface4::PageLock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::PageUnlock(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->PageUnlock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetSurfaceDesc(lpddsd,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->SetPrivateData(guidTag,lpData,cbSize,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetPrivateData(guidTag,lpBuffer,lpcbBufferSize);
}
HRESULT WINAPI glDirectDrawSurface4::FreePrivateData(REFGUID guidTag)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->FreePrivateData(guidTag);
}
HRESULT WINAPI glDirectDrawSurface4::GetUniquenessValue(LPDWORD lpValue)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->GetUniquenessValue(lpValue);
}
HRESULT WINAPI glDirectDrawSurface4::ChangeUniquenessValue()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glDDS7->ChangeUniquenessValue();
}
