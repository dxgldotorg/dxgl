// DXGL
// Copyright (C) 2011-2019 William Feely

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


// DDRAW7 routines
glDirectDrawSurface7::glDirectDrawSurface7(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, HRESULT *error,
	glDirectDrawPalette *palettein, glTexture *parenttex, DWORD miplevel, int version, glDirectDrawSurface7 *front)
{
	TRACE_ENTER(5,14,this,14,lpDD7,14,lpDDSurfaceDesc2,14,error,14,palettein);
	this->version = version;
	creator = NULL;
	hasstencil = false;
	handle = 0;
	device = NULL;
	device1 = NULL;
	locked = 0;
	pagelocked = 0;
	flipcount = 0;
	ZeroMemory(colorkey,4*sizeof(CKEY));
	palette = NULL;
	texture = NULL;
	clipper = NULL;
	dds1 = new glDirectDrawSurface1(this);
	dds2 = new glDirectDrawSurface2(this);
	dds3 = new glDirectDrawSurface3(this);
	dds4 = new glDirectDrawSurface4(this);
	d3dt2 = new glDirect3DTexture2(this);
	d3dt1 = new glDirect3DTexture1(this);
	glDirectDrawGammaControl_Create(this, (LPDIRECTDRAWGAMMACONTROL*)&gammacontrol);
	clientbuffer = NULL;
	zbuffer = NULL;
	attachcount = 0;
	attachparent = NULL;
	overlayenabled = FALSE;
	overlayset = FALSE;
	overlaycount = 0;
	maxoverlays = 0;
	overlays = NULL;
	overlaydest = NULL;
	this->miplevel = miplevel;
	ddInterface = (glDirectDraw7 *)lpDD7;
	hRC = ddInterface->renderer->hRC;
	ddsd = *lpDDSurfaceDesc2;
	miptexture = NULL;
	LONG sizes[6];
	int i;
	float xscale, yscale;
	DWORD winver, winvermajor, winverminor;
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
				if(dxglcfg.primaryscale)
				{
					if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
						(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
					{
						if (ddsd.dwWidth <= 400) xscale = 2.0f;
						else xscale = 1.0f;
						if (ddsd.dwHeight <= 300) yscale = 2.0f;
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
						fakex = (DWORD)((float)sizes[0] / xscale);
						fakey = (DWORD)((float)sizes[1] / yscale);
						break;
					case 2: // Scale to integer auto
						for (i = 1; i < 100; i++)
						{
							if ((ddsd.dwWidth * i) >(DWORD)((float)sizes[0] / xscale))
							{
								fakex = ddsd.dwWidth * i;
								break;
							}
						}
						for (i = 1; i < 100; i++)
						{
							if ((ddsd.dwHeight * i) >(DWORD)((float)sizes[1] / yscale))
							{
								fakey = ddsd.dwHeight * i;
								break;
							}
						}
						break;
					case 3: // 1.5x scale
						fakex = (DWORD)((float)ddsd.dwWidth * 1.5f);
						fakey = (DWORD)((float)ddsd.dwHeight * 1.5f);
						break;
					case 4: // 2x scale
						fakex = ddsd.dwWidth * 2;
						fakey = ddsd.dwHeight * 2;
						break;
					case 5: // 2.5x scale
						fakex = (DWORD)((float)ddsd.dwWidth * 2.5f);
						fakey = (DWORD)((float)ddsd.dwHeight * 2.5f);
						break;
					case 6: // 3x scale
						fakex = ddsd.dwWidth * 3;
						fakey = ddsd.dwHeight * 3;
						break;
					case 7: // 4x scale
						fakex = ddsd.dwWidth * 4;
						fakey = ddsd.dwHeight * 4;
						break;
					case 8: // 5x scale
						fakex = ddsd.dwWidth * 5;
						fakey = ddsd.dwHeight * 5;
						break;
					case 9: // 6x scale
						fakex = ddsd.dwWidth * 6;
						fakey = ddsd.dwHeight * 6;
						break;
					case 10: // 7x scale
						fakex = ddsd.dwWidth * 7;
						fakey = ddsd.dwHeight * 7;
						break;
					case 11: // 8x scale
						fakex = ddsd.dwWidth * 8;
						fakey = ddsd.dwHeight * 8;
						break;
					case 12: // Custom scale
						fakex = (DWORD)((float)ddsd.dwWidth * dxglcfg.primaryscalex);
						fakey = (DWORD)((float)ddsd.dwHeight * dxglcfg.primaryscaley);
						break;
					}
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
				winver = GetVersion();
				winvermajor = (DWORD)(LOBYTE(LOWORD(winver)));
				winverminor = (DWORD)(HIBYTE(LOWORD(winver)));
				if ((winvermajor > 4) || ((winvermajor == 4) && (winverminor >= 1)))
				{
					fakex = ddsd.dwWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
					fakey = ddsd.dwHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
					ddInterface->renderer->xoffset = GetSystemMetrics(SM_XVIRTUALSCREEN);
					ddInterface->renderer->yoffset = GetSystemMetrics(SM_YVIRTUALSCREEN);
				}
				else
				{
					fakex = ddsd.dwWidth = GetSystemMetrics(SM_CXSCREEN);
					fakey = ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
				}
				ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
				*error = DD_OK;
			}
		}
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
/*	if(ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
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
	backbuffer = NULL;
	backbufferwraparound = NULL;
	if ((ddsd.ddsCaps.dwCaps & DDSCAPS_MIPMAP) && !(ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL))
	{
		if (!(ddsd.dwFlags & DDSD_MIPMAPCOUNT))
		{
			ddsd.dwFlags |= DDSD_MIPMAPCOUNT;
			ddsd.dwMipMapCount = CalculateMipLevels(ddsd.dwWidth, ddsd.dwHeight);
		}
	}
	/*

	}*/
	if (ddsd.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
	{
		texture = parenttex;
		glTexture_AddRef(texture);
	}
	else glTexture_Create(&ddsd, &texture, ddInterface->renderer, fakex, fakey, hasstencil, FALSE, 0);
	if (!(ddsd.dwFlags & DDSD_PITCH))
	{
		ddsd.dwFlags |= DDSD_PITCH;
		ddsd.lPitch = texture->levels[this->miplevel].ddsd.lPitch;
	}
	if (!(ddsd.dwFlags & DDSD_PIXELFORMAT))
	{
		ddsd.dwFlags |= DDSD_PIXELFORMAT;
		memcpy(&ddsd.ddpfPixelFormat, &texture->levels[this->miplevel].ddsd.ddpfPixelFormat, sizeof(DDPIXELFORMAT));
	}
	if (!glTexture_ValidatePixelFormat(&ddsd.ddpfPixelFormat))
	{
		*error = DDERR_INVALIDPIXELFORMAT;
		TRACE_VAR("*error", 23, DDERR_INVALIDPIXELFORMAT);
		TRACE_EXIT(-1, 0);
		return;
	}
	if (ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (ddInterface->GetBPP() == 8)
		{
			if (!palettein)
			{
				glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&palette);
				this->SetPaletteNoDraw((LPDIRECTDRAWPALETTE)palette);
			}
			else
			{
				this->SetPaletteNoDraw((LPDIRECTDRAWPALETTE)palettein);
			}
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
		if(newdesc.dwMipMapCount) miptexture = new glDirectDrawSurface7(lpDD7, &newdesc, &miperror, palette, texture, miplevel + 1, version, NULL);
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
				backbuffer = new glDirectDrawSurface7(ddInterface,&ddsdBack,error,palette,parenttex,miplevel,version,front?front:this);
			}
			else if (ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
			{
				backbufferwraparound = front;
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
	int i;
	TRACE_ENTER(1,14,this);
	AddRef();
	if (dds1) delete dds1;
	if (dds2) delete dds2;
	if (dds3) delete dds3;
	if (dds4) delete dds4;
	if (d3dt1) delete d3dt1;
	if (d3dt2) delete d3dt2;
	if (gammacontrol) free(gammacontrol);
	if (overlaydest) overlaydest->DeleteOverlay(this);
	if (overlays)
	{
		for (i = 0; i < overlaycount; i++)
			glTexture_Release(overlays[i].texture, FALSE);
		free(overlays);
		RenderScreen(this->texture, 0, NULL, FALSE, NULL, -1);
	}
	if (texture) glTexture_Release(texture, FALSE);
	//if(bitmapinfo) free(bitmapinfo);
	if (palette)
	{
		if (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			palette->surface = NULL;
			palette->timer = NULL;
		}
		glDirectDrawPalette_Release(palette);
	}
	if(backbuffer) backbuffer->Release();
	if(clipper) glDirectDrawClipper_Release(clipper);
	//if(buffer) free(buffer);
	//if(bigbuffer) free(bigbuffer);
	if (zbuffer)
	{
		if (zbuffer->attachparent == this) zbuffer->attachparent = NULL;
		if (zbuffer->attachcount) zbuffer->attachcount--;
		if (!zbuffer->attachcount) zbuffer->attachparent = NULL;
		zbuffer_iface->Release();
	}
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
			device1 = new glDirect3DDevice7(riid, tmpd3d, this, dds1, 1);
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
			device1->InitDX5();
			device1->InitDX2();
			tmpd3d->Release();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
		else
		{
			*ppvObj = device1->glD3DDev1;
			device1->glD3DDev1->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,DD_OK);
			return DD_OK;
		}
	}
	if (version == 7)
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
		if (zbuffer)
		{
			if (zbuffer->texture->dummycolor)
			{
				glTexture_DeleteDummyColor(zbuffer->texture, FALSE);
			}
		}
		zbuffer = attached;
		zbuffer_iface = iface;
		if (!zbuffer->attachcount) zbuffer->attachparent = this;
		zbuffer->attachcount++;
		if (zbuffer && dxglcfg.primaryscale && (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			glRenderer_MakeTexturePrimary(ddInterface->renderer, zbuffer->texture, this->texture, TRUE);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	else TRACE_RET(HRESULT,23,DDERR_CANNOTATTACHSURFACE);
}
HRESULT WINAPI glDirectDrawSurface7::AddOverlayDirtyRect(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI glDirectDrawSurface7::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	HRESULT error;
	RECT tmprect;
	glDirectDrawSurface7* pattern;
	BltCommand cmd;
	TRACE_ENTER(6, 14, this, 26, lpDestRect, 14, lpDDSrcSurface, 26, lpSrcRect, 9, dwFlags, 14, lpDDBltFx);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if ((dwFlags & DDBLT_DEPTHFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 32, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((dwFlags & DDBLT_DDFX) && !lpDDBltFx) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ZeroMemory(&cmd, sizeof(BltCommand));
	cmd.dest = this->texture;
	cmd.destlevel = this->miplevel;
	if (lpDestRect)
	{
		cmd.destrect = *lpDestRect;
		if(!ddInterface->GetFullscreen() && (ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			OffsetRect(&cmd.destrect, 0 - ddInterface->renderer->xoffset, 0 - ddInterface->renderer->yoffset);
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
		if (!(this->ddsd.dwFlags & DDSD_CKDESTBLT)) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
		if (this->ddsd.ddckCKDestBlt.dwColorSpaceHighValue != this->ddsd.ddckCKDestBlt.dwColorSpaceLowValue)
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
	if (clipper)
	{
		if (!clipper->hWnd)
		{
			if (!clipper->clipsize) TRACE_RET(HRESULT, 23, DDERR_NOCLIPLIST);
			if (clipper->dirty)
			{
				glRenderer_UpdateClipper(ddInterface->renderer, this->clipper->texture, this->clipper->indices,
					this->clipper->vertices,this->clipper->clipsize, this->ddsd.dwWidth, this->ddsd.dwHeight);
				clipper->dirty = false;
			}
			dwFlags |= 0x10000000;
		}
	}
	if (this->clipper && !(this->clipper->hWnd)) cmd.flags |= 0x10000000;
	if (lpDDBltFx) cmd.bltfx = *lpDDBltFx;
	if (dwFlags & DDBLT_DEPTHFILL)
	{
		if (!(this->ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)) TRACE_RET(HRESULT, 23, DDERR_UNSUPPORTED);
		if (this->attachparent)
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(this->ddInterface->renderer, &cmd, this->attachparent->texture, this->attachparent->miplevel));
		}
		else
		{
			TRACE_RET(HRESULT, 23, glRenderer_DepthFill(this->ddInterface->renderer, &cmd, NULL, 0));
		}
	}
	if (this == src)
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
		error = this->ddInterface->SetupTempSurface(tmprect.right, tmprect.bottom);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		error = this->ddInterface->tmpsurface->Blt(&tmprect, lpDDSrcSurface, lpSrcRect, 0, NULL);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		if (dwFlags & DDBLT_KEYSRC)
		{
			if (this->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue !=
				this->ddInterface->tmpsurface->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue)
				this->ddInterface->tmpsurface->SetColorKey(DDCKEY_SRCBLT | DDCKEY_COLORSPACE,
					&((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
			else this->ddInterface->tmpsurface->SetColorKey(DDCKEY_SRCBLT,
				&((glDirectDrawSurface7*)lpDDSrcSurface)->ddsd.ddckCKSrcBlt);
		}
		TRACE_RET(HRESULT, 23, this->Blt(lpDestRect, (LPDIRECTDRAWSURFACE7)this->ddInterface->tmpsurface,
			&tmprect, dwFlags, lpDDBltFx));
	}
	else TRACE_RET(HRESULT, 23, glRenderer_Blt(this->ddInterface->renderer, &cmd));
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
	if (!lpDDSAttachedSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(lpDDSAttachedSurface == (LPDIRECTDRAWSURFACE7)zbuffer)
	{
		if (zbuffer && dxglcfg.primaryscale && (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
			glRenderer_MakeTexturePrimary(ddInterface->renderer, zbuffer->texture, NULL, FALSE);
		if (zbuffer->attachparent == this) zbuffer->attachparent = NULL;
		if (zbuffer->attachcount) zbuffer->attachcount--;
		if (!zbuffer->attachcount) zbuffer->attachparent = NULL;
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
	int i;
	TRACE_ENTER(4,14,this,9,dwFlags,14,lpContext,14,lpfnCallback);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpfnCallback) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (dwFlags > 1) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (this->overlaycount == 0) TRACE_RET(HRESULT, 23, DD_OK);
	if (dwFlags == 1)
	{
		for (i = this->overlaycount; i > 0; i--)
		{
			if (lpfnCallback((LPDIRECTDRAWSURFACE7)this->overlays[i].surface,
				&((glDirectDrawSurface7*)this->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	else
	{
		for (i = this->overlaycount; i > 0; i--)
		{
			if (lpfnCallback((LPDIRECTDRAWSURFACE7)this->overlays[i].surface,
				&((glDirectDrawSurface7*)this->overlays[i].surface)->ddsd, lpContext) == DDENUMRET_CANCEL) break;
		}
	}
	TRACE_RET(HRESULT, 23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glTexture *previous;
	HRESULT ret = Flip2(lpDDSurfaceTargetOverride,dwFlags,&previous);
	if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(ddInterface->lastsync)
		{
			swapinterval++;
			ddInterface->lastsync = false;
		}
		RenderScreen(texture,swapinterval,previous,TRUE,NULL,0);
	}
	if (ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)
	{
		if (overlaydest) overlaydest->UpdateOverlayTexture(this, this->texture);
	}
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT glDirectDrawSurface7::Flip2(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags, glTexture **previous)
{
	TRACE_ENTER(3,14,this,14,lpDDSurfaceTargetOverride,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwFlags & 0xF8FFFFC0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(locked) TRACE_RET(HRESULT,23,DDERR_SURFACEBUSY);
	if(!(this->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && ((dwFlags & DDFLIP_ODD) || (dwFlags & DDFLIP_EVEN))) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DWORD i;
	glDirectDrawSurface7 *tmp;
	if(previous) *previous = this->texture;
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
			if(x == i-1) {TRACE_RET(HRESULT,23,Flip2(NULL,dwFlags,NULL));}
			else Flip2(NULL,0,NULL);
		}
	}
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		glTexture **textures = new glTexture*[ddsd.dwBackBufferCount+1];
		textures[0] = texture;
		tmp = this;
		for(i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			textures[i+1] = tmp->texture;
		}
		glTexture *tmptex = textures[0];
		memmove(textures,&textures[1],ddsd.dwBackBufferCount*sizeof(GLuint));
		textures[ddsd.dwBackBufferCount] = tmptex;
		tmp = this;
		this->SetTexture(textures[0]);
		if(this->palette && (this->texture->palette != this->palette->texture))
			glTexture_SetPalette(this->texture, this->palette->texture, FALSE);
		if(this->clipper && (this->texture->stencil != this->clipper->texture))
			glTexture_SetStencil(this->texture, this->clipper->texture, FALSE);
		for(DWORD i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			tmp->SetTexture(textures[i+1]);
			if (tmp->palette && (tmp->texture->palette != tmp->palette->texture))
				glTexture_SetPalette(tmp->texture, tmp->palette->texture, FALSE);
			if (tmp->clipper && (tmp->texture->stencil != tmp->clipper->texture))
				glTexture_SetStencil(tmp->texture, tmp->clipper->texture, FALSE);
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
	if (backbufferwraparound)
	{ 
		backbufferwraparound->GetCaps(&ddsComp);
		memcpy(&comp1, lpDDSCaps, sizeof(unsigned __int64));
		memcpy(&comp2, &ddsComp, sizeof(unsigned __int64));
		if ((comp1 & comp2) == comp1)
		{
			*lplpDDAttachedSurface = backbufferwraparound;
			backbufferwraparound->AddRef();
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
	glDirectDrawPalette *pal = NULL;
	HRESULT error;
	if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
	{
		if (palette) pal = palette;
		else pal = ddInterface->primary->palette;
	}
	error = glTexture_GetDC(this->texture, this->miplevel, lphDC, pal);
	if (SUCCEEDED(error)) { TRACE_VAR("*lphDC", 14, *lphDC); }
	TRACE_EXIT(23,error);
	return error;
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
	if (!(this->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
		TRACE_RET(HRESULT, 23, DDERR_NOTAOVERLAYSURFACE);
	if (!this->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_OVERLAYNOTVISIBLE);
	if (!this->overlayset) TRACE_RET(HRESULT, 23, DDERR_NOOVERLAYDEST);
	if (!lplX && !lplY) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (lplX) *lplX = this->overlaypos.x;
	if (lplY) *lplY = this->overlaypos.y;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
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
	HRESULT error = glTexture_Lock(this->texture, this->miplevel, lpDestRect, lpDDSurfaceDesc, dwFlags, FALSE);
	if (SUCCEEDED(error))
	{
		locked++;
		ddsd.lpSurface = lpDDSurfaceDesc->lpSurface;
	}
	TRACE_EXIT(23,error);
	return error;
}
HRESULT WINAPI glDirectDrawSurface7::ReleaseDC(HDC hDC)
{
	TRACE_ENTER(2,14,this,14,hDC);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!hDC) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT error = glTexture_ReleaseDC(this->texture, this->miplevel, hDC);
	if (((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		if (ddInterface->lastsync)
		{
			RenderScreen(texture, 1, NULL, TRUE, NULL, 0);
			ddInterface->lastsync = false;
		}
		else RenderScreen(texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23,error);
	return error;
}
void glDirectDrawSurface7::Restore2()
{
	TRACE_ENTER(1,14,this);
#	/*LONG sizes[6];
	float xscale, yscale;
	if(hRC != ddInterface->renderer->hRC)
	{
		ddInterface->GetSizes(sizes);
		if(ddInterface->GetFullscreen())
		{
			ddsd.dwWidth = sizes[2];
			ddsd.dwHeight = sizes[3];
			if(dxglcfg.primaryscale)
			{
				if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
					(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
				{
					if (ddsd.dwWidth <= 400) xscale = 2.0f;
					else xscale = 1.0f;
					if (ddsd.dwHeight <= 300) yscale = 2.0f;
					else yscale = 1.0f;
				}
				else
				{
					xscale = dxglcfg.postsizex;
					yscale = dxglcfg.postsizey;
				}
				fakex = (DWORD)((float)sizes[0] / xscale);
				fakey = (DWORD)((float)sizes[1] / yscale);
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
	}*/
	TRACE_EXIT(0,0);
}
HRESULT WINAPI glDirectDrawSurface7::Restore()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	//LONG sizes[6];
	//float xscale, yscale;
	if(!ddInterface->renderer) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	/*if(hRC != ddInterface->renderer->hRC)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			ddInterface->GetSizes(sizes);
			if(ddInterface->GetFullscreen())
			{
				ddsd.dwWidth = sizes[2];
				ddsd.dwHeight = sizes[3];
				if(dxglcfg.primaryscale)
				{
					if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
						(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
					{
						if (ddsd.dwWidth <= 400) xscale = 2.0f;
						else xscale = 1.0f;
						if (ddsd.dwHeight <= 300) yscale = 2.0f;
						else yscale = 1.0f;
					}
					else
					{
						xscale = dxglcfg.postsizex;
						yscale = dxglcfg.postsizey;
					}
					fakex = (DWORD)((float)sizes[0] / xscale);
					fakey = (DWORD)((float)sizes[1] / yscale);
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
	else */TRACE_RET(HRESULT,23,DD_OK);
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
		if (!clipper->texture) glDirectDrawClipper_CreateTexture(clipper, texture, ddInterface->renderer);
		glTexture_SetStencil(texture, clipper->texture, FALSE);
	}
	else
	{
		glTexture_SetStencil(texture, NULL, FALSE);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDColorKey);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	CKEY key;
	if (lpDDColorKey) key.enabled = true;
	else key.enabled = false;
	if(dwFlags & DDCKEY_COLORSPACE) key.colorspace = true;
	else key.colorspace = false;
	if(lpDDColorKey) key.key = *lpDDColorKey;
	if(dwFlags & DDCKEY_SRCBLT)
	{
		colorkey[0] = key;
		if (lpDDColorKey)
		{
			ddsd.dwFlags |= DDSD_CKSRCBLT;
			ddsd.ddckCKSrcBlt = *lpDDColorKey;
			if (!key.colorspace) ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else ddsd.dwFlags &= ~DDSD_CKSRCBLT;
		glRenderer_SetTextureColorKey(this->ddInterface->renderer, this->texture, dwFlags, lpDDColorKey, this->miplevel);
	}
	if(dwFlags & DDCKEY_DESTBLT)
	{
		colorkey[1] = key;
		if (lpDDColorKey)
		{
			ddsd.dwFlags |= DDSD_CKDESTBLT;
			ddsd.ddckCKDestBlt = *lpDDColorKey;
			if (!key.colorspace) ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else ddsd.dwFlags &= ~DDSD_CKDESTBLT;
		glRenderer_SetTextureColorKey(this->ddInterface->renderer, this->texture, dwFlags, lpDDColorKey, this->miplevel);
	}
	if(dwFlags & DDCKEY_SRCOVERLAY)
	{
		colorkey[2] = key;
		if (lpDDColorKey)
		{
			ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
			ddsd.ddckCKSrcOverlay = *lpDDColorKey;
			if (!key.colorspace) ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else ddsd.dwFlags &= ~DDSD_CKSRCOVERLAY;
		glRenderer_SetTextureColorKey(this->ddInterface->renderer, this->texture, dwFlags, lpDDColorKey, this->miplevel);
	}
	if(dwFlags & DDCKEY_DESTOVERLAY)
	{
		colorkey[3] = key;
		if (lpDDColorKey)
		{
			ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
			ddsd.ddckCKDestOverlay = *lpDDColorKey;
			if (!key.colorspace) ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else ddsd.dwFlags &= ~DDSD_CKDESTOVERLAY;
		glRenderer_SetTextureColorKey(this->ddInterface->renderer, this->texture, dwFlags, lpDDColorKey, this->miplevel);
		if (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			RenderScreen(texture, 0, NULL, FALSE, 0, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetOverlayPosition(LONG lX, LONG lY)
{
	TRACE_ENTER(3,14,this,7,lX,7,lY);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!(this->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY))
		TRACE_RET(HRESULT, 23, DDERR_NOTAOVERLAYSURFACE);
	if (!this->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_OVERLAYNOTVISIBLE);
	if (!this->overlayset) TRACE_RET(HRESULT, 23, DDERR_NOOVERLAYDEST);
	this->overlaypos.x = lX;
	this->overlaypos.y = lY;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2,14,this,14,lpDDPalette);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if ((LPDIRECTDRAWPALETTE)palette != lpDDPalette)
	{
		if (palette)
		{
			palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
			palette->surface = NULL;
			palette->timer = NULL;
			glDirectDrawPalette_Release(palette);
			if (!lpDDPalette) glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&palette);
		}
		if (lpDDPalette)
		{
			palette = (glDirectDrawPalette *)lpDDPalette;
			glDirectDrawPalette_AddRef(palette);
			if (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
			{
				palette->flags |= DDPCAPS_PRIMARYSURFACE;
				palette->surface = this;
				palette->timer = &ddInterface->renderer->timer;
			}
			else
			{
				palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
				palette->surface = NULL;
				palette->timer = NULL;
			}
		}
	}
	if (palette)
	{
		if (!palette->texture) glDirectDrawPalette_CreateTexture(palette, ddInterface->renderer);
		glTexture_SetPalette(texture, palette->texture, FALSE);
	}
	else
	{
		glTexture_SetPalette(texture, NULL, FALSE);
	}
	if (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if (!dxglcfg.DebugNoPaletteRedraw)
		{
			if(DXGLTimer_CheckLastDraw(&ddInterface->renderer->timer,dxglcfg.HackPaletteDelay))
				RenderScreen(texture, dxglcfg.HackPaletteVsync, NULL, FALSE, NULL, 0);
		}
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7::SetPaletteNoDraw(LPDIRECTDRAWPALETTE lpDDPalette)
{
	TRACE_ENTER(2, 14, this, 14, lpDDPalette);
	if (!this) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if ((LPDIRECTDRAWPALETTE)palette != lpDDPalette)
	{
		if (palette)
		{
			palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
			palette->surface = NULL;
			palette->timer = NULL;
			glDirectDrawPalette_Release(palette);
			if (!lpDDPalette) glDirectDrawPalette_Create(DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_PRIMARYSURFACE | 0x800, NULL, (LPDIRECTDRAWPALETTE*)&palette);
		}
		if (lpDDPalette)
		{
			palette = (glDirectDrawPalette *)lpDDPalette;
			glDirectDrawPalette_AddRef(palette);
			if (this->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
			{
				palette->flags |= DDPCAPS_PRIMARYSURFACE;
				palette->surface = this;
				palette->timer = &ddInterface->renderer->timer;
			}
			else
			{
				palette->flags &= ~DDPCAPS_PRIMARYSURFACE;
				palette->surface = NULL;
				palette->timer = NULL;
			}
		}
	}
	if (palette)
	{
		if (!palette->texture) glDirectDrawPalette_CreateTexture(palette, ddInterface->renderer);
		glTexture_SetPalette(texture, palette->texture, FALSE);
	}
	else
	{
		glTexture_SetPalette(texture, NULL, FALSE);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawSurface7::Unlock(LPRECT lpRect)
{
	TRACE_ENTER(2,14,this,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!locked)
	{
		this->texture->levels[this->miplevel].dirty |= 1;
		TRACE_RET(HRESULT, 23, DDERR_NOTLOCKED);
	}
	locked--;
	glTexture_Unlock(this->texture, this->miplevel, lpRect, FALSE);
	ddsd.lpSurface = NULL;
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		{
			if(ddInterface->lastsync)
			{
				RenderScreen(texture,1,NULL,TRUE,NULL,0);
				ddInterface->lastsync = false;
			}
			else RenderScreen(texture,0,NULL,TRUE,NULL,0);
		}
	if ((ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY) && overlayenabled)
	{
		if (ddInterface->lastsync)
		{
			RenderScreen(ddInterface->primary->texture, 1, NULL, TRUE, NULL, 0);
			ddInterface->lastsync = false;
		}
		else RenderScreen(ddInterface->primary->texture, 0, NULL, TRUE, NULL, 0);
	}
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	TRACE_ENTER(6,14,this,26,lpSrcRect,14,lpDDDestSurface,26,lpDestRect,9,dwFlags,14,lpDDOverlayFx);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!(this->ddsd.ddsCaps.dwCaps & DDSCAPS_OVERLAY)) 
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
		if((lpSrcRect->left < 0) ||	(lpSrcRect->right > this->ddsd.dwWidth) ||
			(lpSrcRect->top < 0) ||	(lpSrcRect->bottom > this->ddsd.dwHeight) ||
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
	newoverlay.surface = this;
	newoverlay.texture = this->texture;
	if (lpDDOverlayFx) newoverlay.fx = *lpDDOverlayFx;
	newoverlay.flags = dwFlags;
	if (dwFlags & DDOVER_SHOW)
	{
		this->overlayenabled = TRUE;
		newoverlay.enabled = TRUE;
	}
	if (dwFlags & DDOVER_HIDE)
	{
		this->overlayenabled = FALSE;
		newoverlay.enabled = FALSE;
	}
	this->overlaydest = (glDirectDrawSurface7 *)lpDDDestSurface;
	((glDirectDrawSurface7 *)lpDDDestSurface)->AddOverlay(&newoverlay);
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayDisplay(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,lpDDSReference);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

extern "C" void glDirectDrawSurface7_RenderScreen(LPDIRECTDRAWSURFACE7 surface, int vsync, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	((glDirectDrawSurface7*)surface)->RenderScreen(((glDirectDrawSurface7*)surface)->texture, vsync, NULL, settime, overlays, overlaycount);
}

void glDirectDrawSurface7::RenderScreen(glTexture *texture, int vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	TRACE_ENTER(3,14,this,14,texture,14,vsync);
	glRenderer_DrawScreen(ddInterface->renderer,texture, texture->palette, vsync, previous, settime, overlays, overlaycount);
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
	if (this->overlayenabled) TRACE_RET(HRESULT, 23, DDERR_SURFACEBUSY);
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

HRESULT glDirectDrawSurface7::AddOverlay(OVERLAY *overlay)
{
	OVERLAY *tmpptr;
	int i;
	if (overlaycount + 1 > maxoverlays)
	{
		if (!overlays)
		{
			overlays = (OVERLAY*)malloc(16 * sizeof(OVERLAY));
			if (!overlays) return DDERR_OUTOFMEMORY;
			maxoverlays = 16;
		}
		else
		{
			if (maxoverlays == 256) return DDERR_OUTOFCAPS;
			maxoverlays += 16;
			tmpptr = (OVERLAY*)realloc(overlays, maxoverlays * sizeof(OVERLAY));
			if (!tmpptr) return DDERR_OUTOFMEMORY;
			overlays = tmpptr;
		}
	}
	for (i = 0; i < overlaycount; i++)
	{
		if (overlays[i].surface == overlay->surface)
		{
			glTexture_Release(overlays[i].texture, FALSE);
			overlays[i].destrect = overlay->destrect;
			overlays[i].srcrect = overlay->srcrect;
			overlays[i].flags = overlay->flags;
			overlays[i].fx = overlay->fx;
			overlays[i].surface = overlay->surface;
			overlays[i].texture = overlay->texture;
			if (overlay->flags & DDOVER_SHOW) overlays[i].enabled = TRUE;
			if (overlay->flags & DDOVER_HIDE) overlays[i].enabled = FALSE;
			memcpy(&overlays[i], overlay, sizeof(OVERLAY));
			glTexture_AddRef(overlays[i].texture);
			if (ddInterface->lastsync)
			{
				RenderScreen(ddInterface->primary->texture, 1, NULL, TRUE, overlays, overlaycount);
				ddInterface->lastsync = false;
			}
			else RenderScreen(ddInterface->primary->texture, 0, NULL, TRUE, overlays, overlaycount);
			return DD_OK;
		}
	}
	overlays[overlaycount] = *overlay;
	glTexture_AddRef(overlays[overlaycount].texture);
	overlaycount++;
	if (ddInterface->lastsync)
	{
		RenderScreen(ddInterface->primary->texture, 1, NULL, TRUE, overlays, overlaycount);
		ddInterface->lastsync = false;
	}
	else RenderScreen(ddInterface->primary->texture, 0, NULL, TRUE, overlays, overlaycount);
	return DD_OK;
}

HRESULT glDirectDrawSurface7::DeleteOverlay(glDirectDrawSurface7 *surface)
{
	int i;
	for (i = 0; i < overlaycount; i++)
	{
		if (overlays[i].surface == surface)
		{
			glTexture_Release(overlays[i].texture, FALSE);
			overlaycount--;
			memmove(&overlays[i], &overlays[i + 1], (overlaycount - i) * sizeof(OVERLAY));
			if (surface->overlayenabled)
			{
				if (ddInterface->lastsync)
				{
					RenderScreen(ddInterface->primary->texture, 1, NULL, TRUE, overlays, overlaycount);
					ddInterface->lastsync = false;
				}
				else RenderScreen(ddInterface->primary->texture, 0, NULL, TRUE, overlays, overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
}

HRESULT glDirectDrawSurface7::UpdateOverlayTexture(glDirectDrawSurface7 *surface, glTexture *texture)
{
	int i;
	for (i = 0; i < overlaycount; i++)
	{
		if (overlays[i].surface == surface)
		{
			glTexture_Release(overlays[i].texture, FALSE);
			overlays[i].texture = texture;
			glTexture_AddRef(overlays[i].texture);
			if (surface->overlayenabled)
			{
				if (ddInterface->lastsync)
				{
					RenderScreen(ddInterface->primary->texture, 1, NULL, TRUE, overlays, overlaycount);
					ddInterface->lastsync = false;
				}
				else RenderScreen(ddInterface->primary->texture, 0, NULL, TRUE, overlays, overlaycount);
			}
			return DD_OK;
		}
	}
	return DDERR_NOTFOUND;
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
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,((glDirectDrawSurface1*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
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
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect, ((glDirectDrawSurface2*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
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
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,((glDirectDrawSurface3*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
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
	if (!lpDDDestSurface) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->UpdateOverlay(lpSrcRect,((glDirectDrawSurface4*)lpDDDestSurface)->glDDS7,lpDestRect,dwFlags,lpDDOverlayFx));
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
