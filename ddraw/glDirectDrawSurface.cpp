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
#include "ddraw.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glDirectDrawClipper.h"
#include "glutil.h"

int swapinterval = 0;
inline void SetSwap(int swap)
{
	if(swap != swapinterval)
	{
		wglSwapIntervalEXT(swap);
		swapinterval = wglGetSwapIntervalEXT();
		swapinterval = swap;
	}
}

inline int NextMultipleOf8(int number){return ((number+7) & (~7));}
inline int NextMultipleOf4(int number){return ((number+3) & (~3));}
inline int NextMultipleOf2(int number){return ((number+1) & (~1));}
#ifdef _M_X64
#define NextMultipleOfWord NextMultipleOf8
#else
#define NextMultipleOfWord NextMultipleOf4
#endif

inline int _5to8bit(int number)
{
	return (number << 3)+(number>>2);
}
inline int _6to8bit(int number)
{
	return (number<<2)+(number>>4);
}


int UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3)
{
	if(bpp == 15) bpp = 16;
	glBindTexture(GL_TEXTURE_2D,texture);  // Select surface's texture
	if((x == bigx && y == bigy) || !bigbuffer)
	{
		glTexImage2D(GL_TEXTURE_2D,0,texformat3,x,y,0,texformat,texformat2,buffer);
	}
	else
	{
		switch(bpp)
		{
		case 8:
			ScaleNearest8(bigbuffer,buffer,bigx,bigy,x,y,pitch,bigpitch);
			break;
		case 16:
			ScaleNearest16(bigbuffer,buffer,bigx,bigy,x,y,pitch/2,bigpitch/2);
			break;
		case 24:
			ScaleNearest24(bigbuffer,buffer,bigx,bigy,x,y,pitch,bigpitch);
			break;
		case 32:
			ScaleNearest32(bigbuffer,buffer,bigx,bigy,x,y,pitch/4,bigpitch/4);
			break;
		break;
		}
		glTexImage2D(GL_TEXTURE_2D,0,texformat3,bigx,bigy,0,texformat,texformat2,bigbuffer);
	}
	return 0;
}
int DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2)
{
	glBindTexture(GL_TEXTURE_2D,texture);  // Select surface's texture
	if((bigx == x && bigy == y) || !bigbuffer)
	{
		glGetTexImage(GL_TEXTURE_2D,0,texformat,texformat2,buffer); // Shortcut for no scaling
	}
	else
	{
		glGetTexImage(GL_TEXTURE_2D,0,texformat,texformat2,bigbuffer);
		switch(bpp)
		{
		case 8:
			ScaleNearest8(buffer,bigbuffer,x,y,bigx,bigy,bigpitch,pitch);
			break;
		case 15:
		case 16:
			ScaleNearest16(buffer,bigbuffer,x,y,bigx,bigy,bigpitch/2,pitch/2);
			break;
		case 24:
			ScaleNearest24(buffer,bigbuffer,x,y,bigx,bigy,bigpitch,pitch);
			break;
		case 32:
			ScaleNearest32(buffer,bigbuffer,x,y,bigx,bigy,bigpitch/4,pitch/4);
			break;
		break;
		}
	}
	return 0;
}

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
	palette = NULL;
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
	if(copysurface)
	{
		FIXME("glDirectDrawSurface7::glDirectDrawSurface7: copy surface stub\n");
		*error = DDERR_GENERIC;
		return;
	}
	else
	{
		ddInterface = (glDirectDraw7 *)lpDD7;
		hrc = ddInterface->hRC;
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
			glGenTextures(1,&paltex);
			glBindTexture(GL_TEXTURE_2D,paltex);
			glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
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
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			info.bmiHeader.biWidth = fakex;
			info.bmiHeader.biHeight = -(signed)fakey;
			info.bmiHeader.biPlanes = 1;
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
				info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				info.bmiHeader.biWidth = fakex;
				info.bmiHeader.biHeight = -(signed)fakey;
				info.bmiHeader.biPlanes = 1;
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
		bitmapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapinfo->bmiHeader.biPlanes = 1;
		bitmapinfo->bmiHeader.biSizeImage = 0;
		bitmapinfo->bmiHeader.biXPelsPerMeter = 0;
		bitmapinfo->bmiHeader.biYPelsPerMeter = 0;
		bitmapinfo->bmiHeader.biClrImportant = 0;
		bitmapinfo->bmiHeader.biClrUsed = 0;
		bitmapinfo->bmiHeader.biCompression = BI_RGB;
		bitmapinfo->bmiHeader.biBitCount = (WORD)ddInterface->GetBPPMultipleOf8();
	}
	surfacetype=2;
	bitmapinfo->bmiHeader.biWidth = ddsd.dwWidth;
	bitmapinfo->bmiHeader.biHeight = -(signed)ddsd.dwHeight;
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
		glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D,texture);
		glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		if((dxglcfg.scalingfilter == 0) || (ddInterface->GetBPP() == 8))
		{
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		}
		else
		{
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
		if(ddsd.dwFlags & DDSD_PIXELFORMAT)
		{
			if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)
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
		glTexImage2D(GL_TEXTURE_2D,0,texformat3,fakex,fakey,0,texformat,texformat2,NULL);
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
	ddInterface->AddRef();
}
glDirectDrawSurface7::~glDirectDrawSurface7()
{
	if(dds1) dds1->Release();
	if(dds2) dds2->Release();
	if(dds3) dds3->Release();
	if(dds4) dds4->Release();
	if(paltex)glDeleteTextures(1,&paltex);
	if(bitmapinfo) free(bitmapinfo);
	if(palette) palette->Release();
	if(backbuffer) backbuffer->Release();
	if(buffer) free(buffer);
	if(bigbuffer) free(bigbuffer);
	if(zbuffer) zbuffer->Release();
	ddInterface->Release();
}
HRESULT WINAPI glDirectDrawSurface7::QueryInterface(REFIID riid, void** ppvObj)
{
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
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface7::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface7::AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
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
	FIXME("glDirectDrawSurface7::AddOverlayDirtyRect: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if((dwFlags & DDBLT_COLORFILL) && !lpDDBltFx) return DDERR_INVALIDPARAMS;
	glDirectDrawSurface7 *src = (glDirectDrawSurface7 *)lpDDSrcSurface;
	if(dirty & 1)
	{
		UploadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
			fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
			ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2,texformat3);
		dirty &= ~1;
	}
	if(src && (src->dirty & 1))
	{
		UploadTexture(src->buffer,src->bigbuffer,src->texture,src->ddsd.dwWidth,src->ddsd.dwHeight,
			src->fakex,src->fakey,src->ddsd.lPitch,
			(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*src->fakex)),
			src->ddsd.ddpfPixelFormat.dwRGBBitCount,src->texformat,src->texformat2,src->texformat3);
		src->dirty &= ~1;
	}
	LONG sizes[6];
	ddInterface->GetSizes(sizes);
	int error;
	error = SetFBO(texture,0,false);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0,0,fakex,fakey);
	RECT destrect;
	if(!lpDestRect)
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else destrect = *lpDestRect;
	RECT srcrect;
	DDSURFACEDESC2 ddsdSrc;
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	if(lpDDSrcSurface) lpDDSrcSurface->GetSurfaceDesc(&ddsdSrc);
	if(!lpSrcRect)
	{
		srcrect.left = 0;
		srcrect.top = 0;
		srcrect.right = ddsdSrc.dwWidth;
		srcrect.bottom = ddsdSrc.dwHeight;
	}
	else srcrect = *lpSrcRect;
	GLfloat coords[8];
	coords[0] = (GLfloat)destrect.left * ((GLfloat)fakex/(GLfloat)ddsd.dwWidth);
	coords[1] = (GLfloat)destrect.right * ((GLfloat)fakex/(GLfloat)ddsd.dwWidth);
	coords[2] = (GLfloat)fakey-((GLfloat)destrect.top * ((GLfloat)fakey/(GLfloat)ddsd.dwHeight));
	coords[3] = (GLfloat)fakey-((GLfloat)destrect.bottom * ((GLfloat)fakey/(GLfloat)ddsd.dwHeight));
	coords[4] = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
	coords[5] = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
	coords[6] = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
	coords[7] = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
	glMatrixMode(GL_PROJECTION);
	glClear(GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glOrtho(0,fakex,fakey,0,0,1);
	glMatrixMode(GL_MODELVIEW);
	if(dwFlags & DDBLT_COLORFILL)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_ALPHA_TEST);
		switch(ddInterface->GetBPP())
		{
		case 8:
			glColor3ub((GLubyte)lpDDBltFx->dwFillColor,(GLubyte)lpDDBltFx->dwFillColor,
				(GLubyte)lpDDBltFx->dwFillColor);
			break;
		case 15:
			glColor3ub(_5to8bit((lpDDBltFx->dwFillColor>>10) & 31),
				_5to8bit((lpDDBltFx->dwFillColor>>5) & 31),
				_5to8bit(lpDDBltFx->dwFillColor & 31));
			break;
		case 16:
			glColor3ub(_5to8bit((lpDDBltFx->dwFillColor>>11) & 31),
				_6to8bit((lpDDBltFx->dwFillColor>>5) & 63),
				_5to8bit(lpDDBltFx->dwFillColor & 31));
			break;
		case 24:
		case 32:
			glColor3ub(((lpDDBltFx->dwFillColor>>16) & 255),
				((lpDDBltFx->dwFillColor>>8) & 255),(lpDDBltFx->dwFillColor & 255));
		default:
			break;
		}
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glColor3f(1.0,1.0,1.0);
	}
	if(lpDDSrcSurface) glBindTexture(GL_TEXTURE_2D,((glDirectDrawSurface7*)lpDDSrcSurface)->GetTexture());
	if((dwFlags & DDBLT_KEYSRC) && (src && src->colorkey[0].enabled) && !(dwFlags & DDBLT_COLORFILL))
	{
		glUseProgram(shaders[PROG_CKEY].prog);
		GLint keyloc = glGetUniformLocation(shaders[PROG_CKEY].prog,"keyIn");
		switch(ddInterface->GetBPP())
		{
		case 8:
			glUniform3i(keyloc,src->colorkey[0].key.dwColorSpaceHighValue,src->colorkey[0].key.dwColorSpaceHighValue,
				src->colorkey[0].key.dwColorSpaceHighValue);
			break;
		case 15:
			glUniform3i(keyloc,_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>10 & 31),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>5 & 31),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue & 31));
			break;
		case 16:
			glUniform3i(keyloc,_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>11 & 31),
				_6to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>5 & 63),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue & 31));
			break;
		case 24:
		case 32:
		default:
			glUniform3i(keyloc,(src->colorkey[0].key.dwColorSpaceHighValue>>16 & 255),
				(src->colorkey[0].key.dwColorSpaceHighValue>>8 & 255),
				(src->colorkey[0].key.dwColorSpaceHighValue & 255));
			break;
		}
		GLint texloc = glGetUniformLocation(shaders[PROG_CKEY].prog,"myTexture");
		glUniform1i(texloc,0);
	}
	else glUseProgram(0);
	this->dirty |= 2;
	glBegin(GL_QUADS);
	glTexCoord2f(coords[4], coords[6]);
	glVertex2f(coords[0], coords[2]);
	glTexCoord2f(coords[5], coords[6]);
	glVertex2f( coords[1], coords[2]);
	glTexCoord2f(coords[5], coords[7]);
	glVertex2f( coords[1], coords[3]);
	glTexCoord2f(coords[4], coords[7]);
	glVertex2f(coords[0], coords[3]);
	glEnd();
	glColor3f(1.0,1.0,1.0);
	glUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	SetFBO(0,0,false);
	glPopAttrib();
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP))) RenderScreen(texture,this);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::BltBatch: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
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
	FIXME("glDirectDrawSurface7::EnumAttachedSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
{
	FIXME("glDirectDrawSurface7::EnumOverlayZOrders: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	if(dwFlags & DDFLIP_NOVSYNC) SetSwap(0);
	else
	{
		if(dwFlags & DDFLIP_INTERVAL3) SetSwap(3);
		else if(dwFlags & DDFLIP_INTERVAL2) SetSwap(2);
		else if(dwFlags & DDFLIP_INTERVAL4) SetSwap(4);
		else SetSwap(1);
	}
	int flips = 1;
	if(lpDDSurfaceTargetOverride) ERR(DDERR_GENERIC);
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) return DDERR_INVALIDOBJECT;
		GLuint *textures = new GLuint[ddsd.dwBackBufferCount+1];
		textures[0] = texture;
		glDirectDrawSurface7 *tmp = this;
		if(dirty & 1)
		{
			UploadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,
				fakex,fakey,ddsd.lPitch,(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*fakex)),
				ddsd.ddpfPixelFormat.dwRGBBitCount,texformat,texformat2,texformat3);
			dirty &= ~1;
		}
		this->dirty |= 2;
		for(DWORD i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
			if(tmp->dirty & 1)
			{
				UploadTexture(tmp->buffer,tmp->bigbuffer,tmp->texture,tmp->ddsd.dwWidth,tmp->ddsd.dwHeight,
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
		RenderScreen(textures[0],this);
		delete textures;
	}
	else return DDERR_NOTFLIPPABLE;
	flipcount+=flips;
	if(flipcount > ddsd.dwBackBufferCount) flipcount -= (ddsd.dwBackBufferCount+1);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface)
{
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
	FIXME("glDirectDrawSurface7::GetBltStatus: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	memcpy(lpDDSCaps,&ddsd.ddsCaps,sizeof(DDSCAPS2));
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	FIXME("glDirectDrawSurface7::GetClipper: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
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
	if(hdc) ERR(DDERR_DCALREADYCREATED);
	DWORD colors[256];
	HRESULT error;
	LPVOID surface;
	error = this->Lock(NULL,&ddsd,0,NULL);
	if(error != DD_OK) return error;
	hdc = CreateCompatibleDC(NULL);
	bitmapinfo->bmiHeader.biWidth = ddsd.lPitch / (bitmapinfo->bmiHeader.biBitCount / 8);
	if(ddInterface->GetBPP() == 8)
	{
		memcpy(colors,ddInterface->primary->palette->GetPalette(NULL),1024);
		for(int i = 0; i < 256; i++)
			colors[i] = ((colors[i]&0x0000FF)<<16) | (colors[i]&0x00FF00) | ((colors[i]&0xFF0000)>>16);
		memcpy(bitmapinfo->bmiColors,colors,1024);
	}
	else  if(ddInterface->GetBPPMultipleOf8() == 16)bitmapinfo->bmiHeader.biCompression = BI_BITFIELDS;
	hbitmap = CreateDIBSection(hdc,bitmapinfo,DIB_RGB_COLORS,&surface,NULL,0);
	memcpy(surface,ddsd.lpSurface,ddsd.lPitch*ddsd.dwHeight);
	HGDIOBJ temp = SelectObject(hdc,hbitmap);
	DeleteObject(temp);
	*lphDC = hdc;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetFlipStatus(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::GetFlipStatus: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	FIXME("glDirectDrawSurface7::GetOverlayPosition: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
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
	*lpDDPixelFormat = ddsd.ddpfPixelFormat;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!lpDDSurfaceDesc) ERR(DDERR_INVALIDPARAMS);
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface7::IsLost()
{
	FIXME("glDirectDrawSurface7::IsLost: stub\n");
	ERR(DDERR_GENERIC);
}

HRESULT WINAPI glDirectDrawSurface7::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
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
			DownloadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
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
		DownloadTexture(buffer,bigbuffer,texture,ddsd.dwWidth,ddsd.dwHeight,fakex,fakey,ddsd.lPitch,
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
	if(!hdc) ERR(DDERR_INVALIDOBJECT);
	if(hDC != hdc) ERR(DDERR_INVALIDOBJECT);
	GetDIBits(hDC,hbitmap,0,ddsd.dwHeight,ddsd.lpSurface,bitmapinfo,DIB_RGB_COLORS);
	Unlock(NULL);
	DeleteObject(hbitmap);
	hbitmap = NULL;
	DeleteDC(hdc);
	hdc = NULL;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Restore()
{
	if(hrc != ddInterface->hRC)
	{
		FIXME("glDirectDrawSurface7::Restore: stub\n");
		ERR(DDERR_GENERIC);
	}
	else return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	if(clipper) clipper->Release();
	clipper = (glDirectDrawClipper *)lpDDClipper;
	if(clipper)clipper->AddRef();
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
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
	FIXME("glDirectDrawSurface7::SetOverlayPosition: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
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
	FIXME("glDirectDrawSurface7::UpdateOverlay: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayDisplay(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::UpdateOverlayDisplay: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	ERR(DDERR_GENERIC);
}

void glDirectDrawSurface7::RenderScreen(GLuint texture, glDirectDrawSurface7 *surface)
{
	LONG sizes[6];
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	RECT r,r2;
	if(surface->dirty & 1)
	{
		UploadTexture(buffer,surface->bigbuffer,texture,surface->ddsd.dwWidth,surface->ddsd.dwHeight,
			surface->fakex,surface->fakey,surface->ddsd.lPitch,
			(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*surface->fakex)),
			surface->ddsd.ddpfPixelFormat.dwRGBBitCount,surface->texformat,surface->texformat2,surface->texformat3);
		surface->dirty &= ~1;
	}
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(ddInterface->GetFullscreen())
		{
			ddInterface->GetSizes(sizes);
			glOrtho((signed)-(sizes[4]-sizes[0])/2,(sizes[4]-sizes[0])/2+sizes[0],
				(signed)-(sizes[5]-sizes[1])/2,(sizes[5]-sizes[1])/2+sizes[1],0,1);
		}
		else
		{
			HWND hwnd,hrender;
			ddInterface->GetHandles(&hwnd,&hrender);
			GetClientRect(hwnd,&r);
			GetClientRect(hrender,&r2);
			if(memcmp(&r2,&r,sizeof(RECT)))
				SetWindowPos(hrender,NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
			r2 = r;
			ClientToScreen(hwnd,(LPPOINT)&r2.left);
			ClientToScreen(hwnd,(LPPOINT)&r2.right);
			glViewport(0,0,r.right,r.bottom);
			glOrtho((signed)r2.left,(signed)r2.right,(signed)(fakey-r2.bottom),(signed)(fakey-r2.top),0,1);
		}
	}
	else glOrtho(0,fakex,fakey,0,0,1);
	glMatrixMode(GL_MODELVIEW);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(ddInterface->GetBPP() == 8)
	{
		glUseProgram(shaders[PROG_PAL256].prog);
		glBindTexture(GL_TEXTURE_2D,paltex);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,256,1,0,GL_RGBA,GL_UNSIGNED_BYTE,palette->GetPalette(NULL));
		GLint palloc = glGetUniformLocation(shaders[PROG_PAL256].prog,"ColorTable");
		GLint texloc = glGetUniformLocation(shaders[PROG_PAL256].prog,"IndexTexture");
		glUniform1i(texloc,0);
		glUniform1i(palloc,1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,paltex);
		glActiveTexture(GL_TEXTURE0);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,texture);
	}
	if(ddInterface->GetFullscreen())
	{
		glBegin(GL_QUADS);
		glTexCoord2f(0., 1.);
		glVertex2f(0., 0.);
		glTexCoord2f(1., 1.);
		glVertex2f( (float)sizes[0], 0.);
		glTexCoord2f(1., 0.);
		glVertex2f( (float)sizes[0], (float)sizes[1]);
		glTexCoord2f(0., 0.);
		glVertex2f(0., (float)sizes[1]);
		glEnd();
	}
	else
	{
		glBegin(GL_QUADS);
		glTexCoord2f(0., 1.);
		glVertex2f(0., 0.);
		glTexCoord2f(1., 1.);
		glVertex2f( (float)fakex, 0.);
		glTexCoord2f(1., 0.);
		glVertex2f( (float)fakex, (float)fakey);
		glTexCoord2f(0., 0.);
		glVertex2f(0., (float)fakey);
		glEnd();
	}
	glDisable(GL_TEXTURE_2D);
	glFlush();
	SwapBuffers(ddInterface->hDC);
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7::GetDDInterface(LPVOID FAR *lplpDD)
{
	*lplpDD = ddInterface;
	FIXME("glDirectDrawSurface7::GetDDInterface: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::PageLock(DWORD dwFlags)
{
	pagelocked++;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::PageUnlock(DWORD dwFlags)
{
	if(!pagelocked) ERR(DDERR_NOTPAGELOCKED);
	pagelocked--;
	return DD_OK;
}
// ddraw 3+ api
HRESULT WINAPI glDirectDrawSurface7::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::SetSurfaceDesc: stub\n");
	ERR(DDERR_GENERIC);
}
// ddraw 4+ api
HRESULT WINAPI glDirectDrawSurface7::SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags)
{
	FIXME("glDirectDrawSurface7::SetPrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize)
{
	FIXME("glDirectDrawSurface7::GetPrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::FreePrivateData(REFGUID guidTag)
{
	FIXME("glDirectDrawSurface7::FreePrivateData: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetUniquenessValue(LPDWORD lpValue)
{
	FIXME("glDirectDrawSurface7::GetUniquenessValue: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::ChangeUniquenessValue()
{
	FIXME("glDirectDrawSurface7::ChangeUniquenessValue: stub\n");
	ERR(DDERR_GENERIC);
}
// ddraw 7 api
HRESULT WINAPI glDirectDrawSurface7::SetPriority(DWORD dwPriority)
{
	FIXME("glDirectDrawSurface7::SetPriority: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetPriority(LPDWORD lpdwPriority)
{
	FIXME("glDirectDrawSurface7::GetPriority: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::SetLOD(DWORD dwMaxLOD)
{
	FIXME("glDirectDrawSurface7::SetLOD: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::GetLOD(LPDWORD lpdwMaxLOD)
{
	FIXME("glDirectDrawSurface7::GetLOD: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawSurface7::Unlock2(LPVOID lpSurfaceData)
{
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
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface1::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface1::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface1::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface1::AddOverlayDirtyRect(LPRECT lpRect)
{
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface1::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface1::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface1*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface1::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface1::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface1::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface1::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	return glDDS7->Flip((LPDIRECTDRAWSURFACE7)lpDDSurfaceTargetOverride,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface)
{
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
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetCaps(LPDDSCAPS lpDDSCaps)
{
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface1::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface1::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface1::GetDC(HDC FAR *lphDC)
{
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface1::GetFlipStatus(DWORD dwFlags)
{
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface1::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface1::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface1::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface1::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface1::IsLost()
{
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface1::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface1::ReleaseDC(HDC hDC)
{
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface1::Restore()
{
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface1::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface1::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface1::SetOverlayPosition(LONG lX, LONG lY)
{
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface1::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface1::Unlock(LPVOID lpSurfaceData)
{
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayDisplay(DWORD dwFlags)
{
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference)
{
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
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface2::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface2::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface2::AddAttachedSurface(LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface2::AddOverlayDirtyRect(LPRECT lpRect)
{
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface2::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface2::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface2*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface2::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface)
{
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface2::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface2::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface2::Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	return glDDS7->Flip((LPDIRECTDRAWSURFACE7)lpDDSurfaceTargetOverride,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface)
{
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
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetCaps(LPDDSCAPS lpDDSCaps)
{
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface2::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface2::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface2::GetDC(HDC FAR *lphDC)
{
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface2::GetFlipStatus(DWORD dwFlags)
{
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface2::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface2::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface2::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface2::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface2::IsLost()
{
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface2::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface2::ReleaseDC(HDC hDC)
{
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface2::Restore()
{
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface2::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface2::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface2::SetOverlayPosition(LONG lX, LONG lY)
{
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface2::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface2::Unlock(LPVOID lpSurfaceData)
{
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayDisplay(DWORD dwFlags)
{
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference)
{
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface2::GetDDInterface(LPVOID FAR *lplpDD)
{
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface2::PageLock(DWORD dwFlags)
{
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface2::PageUnlock(DWORD dwFlags)
{
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
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface3::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface3::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface3::AddAttachedSurface(LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface3::AddOverlayDirtyRect(LPRECT lpRect)
{
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface3::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface3::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface3*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface3::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface)
{
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface3::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface3::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback)
{
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface3::Flip(LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	return glDDS7->Flip((LPDIRECTDRAWSURFACE7)lpDDSurfaceTargetOverride,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface)
{
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
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetCaps(LPDDSCAPS lpDDSCaps)
{
	HRESULT error;
	DDSCAPS2 ddsCaps1;
	error =  glDDS7->GetCaps(&ddsCaps1);
	ZeroMemory(&ddsCaps1,sizeof(DDSCAPS2));
	memcpy(lpDDSCaps,&ddsCaps1,sizeof(DDSCAPS));
	return error;
}
HRESULT WINAPI glDirectDrawSurface3::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface3::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface3::GetDC(HDC FAR *lphDC)
{
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface3::GetFlipStatus(DWORD dwFlags)
{
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface3::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface3::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface3::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return glDDS7->GetSurfaceDesc((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface3::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface3::IsLost()
{
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface3::Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	return glDDS7->Lock(lpDestRect,(LPDDSURFACEDESC2)lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface3::ReleaseDC(HDC hDC)
{
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface3::Restore()
{
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface3::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface3::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface3::SetOverlayPosition(LONG lX, LONG lY)
{
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface3::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface3::Unlock(LPVOID lpSurfaceData)
{
	return glDDS7->Unlock2(lpSurfaceData);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayDisplay(DWORD dwFlags)
{
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference)
{
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface3::GetDDInterface(LPVOID FAR *lplpDD)
{
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface3::PageLock(DWORD dwFlags)
{
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::PageUnlock(DWORD dwFlags)
{
	return glDDS7->PageUnlock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface3::SetSurfaceDesc(LPDDSURFACEDESC lpddsd, DWORD dwFlags)
{
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
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDrawSurface4::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawSurface4::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawSurface4::AddAttachedSurface(LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	return glDDS7->AddAttachedSurface((LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface4::AddOverlayDirtyRect(LPRECT lpRect)
{
	return glDDS7->AddOverlayDirtyRect(lpRect);
}
HRESULT WINAPI glDirectDrawSurface4::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(lpDDSrcSurface) return glDDS7->Blt(lpDestRect,((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwFlags,lpDDBltFx);
	else return glDDS7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface4::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	return glDDS7->BltFast(dwX,dwY,((glDirectDrawSurface4*)lpDDSrcSurface)->GetDDS7(),lpSrcRect,dwTrans);
}
HRESULT WINAPI glDirectDrawSurface4::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface)
{
	return glDDS7->DeleteAttachedSurface(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSAttachedSurface);
}
HRESULT WINAPI glDirectDrawSurface4::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	return glDDS7->EnumAttachedSurfaces(lpContext,(LPDDENUMSURFACESCALLBACK7)lpEnumSurfacesCallback);
}
HRESULT WINAPI glDirectDrawSurface4::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback)
{
	return glDDS7->EnumOverlayZOrders(dwFlags,lpContext,(LPDDENUMSURFACESCALLBACK7)lpfnCallback);
}
HRESULT WINAPI glDirectDrawSurface4::Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	return glDDS7->Flip((LPDIRECTDRAWSURFACE7)lpDDSurfaceTargetOverride,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface)
{
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
	return glDDS7->GetBltStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	return glDDS7->GetCaps(lpDDSCaps);
}
HRESULT WINAPI glDirectDrawSurface4::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	return glDDS7->GetClipper(lplpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface4::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->GetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface4::GetDC(HDC FAR *lphDC)
{
	return glDDS7->GetDC(lphDC);
}
HRESULT WINAPI glDirectDrawSurface4::GetFlipStatus(DWORD dwFlags)
{
	return glDDS7->GetFlipStatus(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	return glDDS7->GetOverlayPosition(lplX,lplY);
}
HRESULT WINAPI glDirectDrawSurface4::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	return glDDS7->GetPalette(lplpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface4::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	return glDDS7->GetPixelFormat(lpDDPixelFormat);
}
HRESULT WINAPI glDirectDrawSurface4::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	return glDDS7->GetSurfaceDesc(lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDrawSurface4::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirectDrawSurface4::IsLost()
{
	return glDDS7->IsLost();
}
HRESULT WINAPI glDirectDrawSurface4::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	return glDDS7->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent);
}
HRESULT WINAPI glDirectDrawSurface4::ReleaseDC(HDC hDC)
{
	return glDDS7->ReleaseDC(hDC);
}
HRESULT WINAPI glDirectDrawSurface4::Restore()
{
	return glDDS7->Restore();
}
HRESULT WINAPI glDirectDrawSurface4::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	return glDDS7->SetClipper(lpDDClipper);
}
HRESULT WINAPI glDirectDrawSurface4::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	return glDDS7->SetColorKey(dwFlags,lpDDColorKey);
}
HRESULT WINAPI glDirectDrawSurface4::SetOverlayPosition(LONG lX, LONG lY)
{
	return glDDS7->SetOverlayPosition(lX,lY);
}
HRESULT WINAPI glDirectDrawSurface4::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	return glDDS7->SetPalette(lpDDPalette);
}
HRESULT WINAPI glDirectDrawSurface4::Unlock(LPRECT lpRect)
{
	return glDDS7->Unlock2(lpRect);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	return glDDS7->UpdateOverlay(lpSrcRect,(LPDIRECTDRAWSURFACE7)lpDDDestSurface,lpDestRect,dwFlags,lpDDOverlayFx);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayDisplay(DWORD dwFlags)
{
	return glDDS7->UpdateOverlayDisplay(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference)
{
	return glDDS7->UpdateOverlayZOrder(dwFlags,(LPDIRECTDRAWSURFACE7)lpDDSReference);
}
HRESULT WINAPI glDirectDrawSurface4::GetDDInterface(LPVOID FAR *lplpDD)
{
	return glDDS7->GetDDInterface(lplpDD);
}
HRESULT WINAPI glDirectDrawSurface4::PageLock(DWORD dwFlags)
{
	return glDDS7->PageLock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::PageUnlock(DWORD dwFlags)
{
	return glDDS7->PageUnlock(dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd, DWORD dwFlags)
{
	return glDDS7->SetSurfaceDesc(lpddsd,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags)
{
	return glDDS7->SetPrivateData(guidTag,lpData,cbSize,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface4::GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize)
{
	return glDDS7->GetPrivateData(guidTag,lpBuffer,lpcbBufferSize);
}
HRESULT WINAPI glDirectDrawSurface4::FreePrivateData(REFGUID guidTag)
{
	return glDDS7->FreePrivateData(guidTag);
}
HRESULT WINAPI glDirectDrawSurface4::GetUniquenessValue(LPDWORD lpValue)
{
	return glDDS7->GetUniquenessValue(lpValue);
}
HRESULT WINAPI glDirectDrawSurface4::ChangeUniquenessValue()
{
	return glDDS7->ChangeUniquenessValue();
}
