// DXGL
// Copyright (C) 2011 William Feely

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

#include "stdafx.h"
#include "shaders.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"

inline int NextMultipleOf4(int number){return ((number+3) & (~3));}
inline int NextMultipleOf2(int number){return ((number+1) & (~1));}

#pragma region DDRAW7 routines
glDirectDrawSurface7::glDirectDrawSurface7(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 *lplpDDSurface7, HRESULT *error, bool copysurface, glDirectDrawPalette *palettein)
{
	locked = 0;
	bitmapinfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO)+(255*sizeof(RGBQUAD)));
	palette = NULL;
	hdc = NULL;
	dds1 = NULL;
	if(copysurface)
	{
		FIXME("glDirectDrawSurface7::glDirectDrawSurface7: copy surface stub\n");
		*error = DDERR_GENERIC;
		return;
	}
	else
	{
		ddInterface = (glDirectDraw7 *)lpDD7;
		ddsd = *lpDDSurfaceDesc2;
	}
	DWORD sizes[6];
	ddInterface->GetSizes(sizes);
	if(!(ddsd.dwFlags & DDSD_CAPS))
	{
		*error = DDERR_INVALIDPARAMS;
		return;
	}
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
			fakex = sizes[0];
			fakey = sizes[1];
			ddsd.dwWidth = sizes[2];
			ddsd.dwHeight = sizes[3];
			ddsd.dwFlags |= (DDSD_WIDTH | DDSD_HEIGHT);
			*error = DD_OK;
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
			info.bmiHeader.biHeight = fakey;
			info.bmiHeader.biPlanes = 1;
			info.bmiHeader.biCompression = BI_RGB;
			info.bmiHeader.biSizeImage = 0;
			info.bmiHeader.biXPelsPerMeter = 0;
			info.bmiHeader.biYPelsPerMeter = 0;
			info.bmiHeader.biClrImportant = 0;
			info.bmiHeader.biClrUsed = 0;
			info.bmiHeader.biBitCount = (WORD)ddInterface->GetBPP();
			*bitmapinfo = info;
			surfacetype=1;
		}
		else
		{
			if(ddsd.dwFlags & DDSD_PIXELFORMAT) surfacetype=0;
			else
			{
				info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				info.bmiHeader.biWidth = fakex;
				info.bmiHeader.biHeight = fakey;
				info.bmiHeader.biPlanes = 1;
				info.bmiHeader.biCompression = BI_RGB;
				info.bmiHeader.biSizeImage = 0;
				info.bmiHeader.biXPelsPerMeter = 0;
				info.bmiHeader.biYPelsPerMeter = 0;
				info.bmiHeader.biClrImportant = 0;
				info.bmiHeader.biClrUsed = 0;
				info.bmiHeader.biBitCount = (WORD)ddInterface->GetBPP();
				*bitmapinfo = info;
				surfacetype=1;
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
		bitmapinfo->bmiHeader.biBitCount = (WORD)ddInterface->GetBPP();
		surfacetype=2;
	}
	bitmapinfo->bmiHeader.biWidth = ddsd.dwWidth;
	bitmapinfo->bmiHeader.biHeight = ddsd.dwHeight;
	switch(surfacetype)
	{
	case 0:
		buffer = (char *)malloc((ddsd.ddpfPixelFormat.dwRGBBitCount * fakex * fakey)/8);
		if(!buffer) *error = DDERR_OUTOFMEMORY;
		break;
	case 1:
		buffer = NULL;
		break;
	case 2:
		buffer = NULL;
		glGenTextures(1,&texture);
		glBindTexture(GL_TEXTURE_2D,texture);
		glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
		if(ddsd.dwFlags & DDSD_PIXELFORMAT)
		{
			switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
			{
			case 8:
			case 16:
			case 32:
				break;

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
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
				ddsd.ddpfPixelFormat.dwRBitMask = 0;
				ddsd.ddpfPixelFormat.dwGBitMask = 0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0;
				ddsd.lPitch = ddsd.dwWidth;
				break;
			case 15:
				texformat = GL_RGB;
				texformat2 = GL_UNSIGNED_SHORT_5_5_5_1;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = ddsd.dwWidth*2;
				break;
			case 16:
				texformat = GL_RGB;
				texformat2 = GL_UNSIGNED_SHORT_5_6_5;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = ddsd.dwWidth*2;
				break;
			case 32:
				texformat = GL_BGRA;
				texformat2 = GL_UNSIGNED_BYTE;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = ddsd.dwWidth*4;
				break;
			default:
				*error = DDERR_INVALIDPIXELFORMAT;
				return;
			}
		}
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fakex,fakey,0,texformat,texformat2,NULL);
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
	if(paltex)glDeleteTextures(1,&paltex);
	if(bitmapinfo) free(bitmapinfo);
	if(palette) palette->Release();
	if(backbuffer) backbuffer->Release();
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
		FIXME("glDirectDrawSurface4 unimplimented");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirectDrawSurface3)
	{
		FIXME("glDirectDrawSurface3 unimplimented");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirectDrawSurface2)
	{
		FIXME("glDirectDrawSurface2 unimplimented");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirectDrawSurface)
	{
		this->AddRef();
		*ppvObj = new glDirectDrawSurface1(this);
		dds1 = (glDirectDrawSurface1*)*ppvObj;
		return DD_OK;
	}
	return E_NOINTERFACE;
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
	FIXME("glDirectDrawSurface7::AddAttachedSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::AddOverlayDirtyRect(LPRECT lpRect)
{
	FIXME("glDirectDrawSurface7::AddOverlayDirtyRect: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	FIXME("glDirectDrawSurface7::Blt: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::BltBatch: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	FIXME("glDirectDrawSurface7::BltFast: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface)
{
	FIXME("glDirectDrawSurface7::DeleteAttachedSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	FIXME("glDirectDrawSurface7::EnumAttachedSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback)
{
	FIXME("glDirectDrawSurface7::EnumOverlayZOrders: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags)
{
	DWORD sizes[6];
	if(lpDDSurfaceTargetOverride) return DDERR_GENERIC;
	if(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)
	{
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) return DDERR_INVALIDOBJECT;
		GLuint *textures = new GLuint[ddsd.dwBackBufferCount+1];
		textures[0] = texture;
		glDirectDrawSurface7 *tmp = this;
		for(DWORD i = 0; i < ddsd.dwBackBufferCount; i++)
		{
			tmp = tmp->GetBackbuffer();
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
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			ddInterface->GetSizes(sizes);
			glOrtho((signed)-(sizes[4]-fakex)/2,(sizes[4]-fakex)/2+fakex,
				(signed)-(sizes[5]-fakey)/2,(sizes[5]-fakey)/2+fakey,0,1);
		}
		else glOrtho(0,fakex,fakey,0,0,1);
		glMatrixMode(GL_MODELVIEW);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		if(ddInterface->GetBPP() == 8)
		{
			GLint palprog = ddInterface->PalProg();
			glUseProgram(palprog);
			glBindTexture(GL_TEXTURE_2D,paltex);
			glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,256,1,0,GL_BGRA,GL_UNSIGNED_BYTE,palette->GetPalette(NULL));
			GLint palloc = glGetUniformLocation(palprog,"ColorTable");
			GLint texloc = glGetUniformLocation(palprog,"IndexTexture");
			glUniform1i(texloc,0);
			glUniform1i(palloc,1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D,textures[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D,paltex);
			glActiveTexture(GL_TEXTURE0);
		}
		else
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D,textures[0]);
		}
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
		glDisable(GL_TEXTURE_2D);
		glFlush();
		SwapBuffers(ddInterface->hDC);
		delete textures;
	}
	else return DDERR_NOTFLIPPABLE;
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
		return DD_OK;
	}
	else
	{
		FIXME("glDirectDrawSurface7::GetAttachedSurface: stub\n");
		return DDERR_GENERIC;
	}
}
HRESULT WINAPI glDirectDrawSurface7::GetBltStatus(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::GetBltStatus: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetCaps(LPDDSCAPS2 lpDDSCaps)
{
	memcpy(lpDDSCaps,&ddsd.ddsCaps,sizeof(DDSCAPS2));
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	FIXME("glDirectDrawSurface7::GetClipper: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	FIXME("glDirectDrawSurface7::GetColorKey: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetDC(HDC FAR *lphDC)
{
	if(hdc) return DDERR_DCALREADYCREATED;
	HRESULT error;
	unsigned char *bitmap;
	error = this->Lock(NULL,&ddsd,0,NULL);
	if(error != DD_OK) return error;
	hdc = CreateCompatibleDC(NULL);
	bitmapinfo->bmiHeader.biWidth = ddsd.lPitch / (bitmapinfo->bmiHeader.biBitCount / 8);
	if(ddInterface->GetBPP() == 8)
		memcpy(bitmapinfo->bmiColors,palette->GetPalette(NULL),1024);
	hbitmap = CreateDIBitmap(hdc,&bitmapinfo->bmiHeader,CBM_INIT,ddsd.lpSurface,bitmapinfo,DIB_RGB_COLORS);
	HGDIOBJ temp = SelectObject(hdc,hbitmap);
	DeleteObject(temp);
	*lphDC = hdc;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::GetFlipStatus(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::GetFlipStatus: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetOverlayPosition(LPLONG lplX, LPLONG lplY)
{
	FIXME("glDirectDrawSurface7::GetOverlayPosition: stub\n");
	return DDERR_GENERIC;
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
	else err = DDERR_NOPALETTEATTACHED;
	return err;
}
HRESULT WINAPI glDirectDrawSurface7::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	FIXME("glDirectDrawSurface7::GetPixelFormat: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	if(!lpDDSurfaceDesc) return DDERR_INVALIDPARAMS;
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	FIXME("glDirectDrawSurface7::Initialize: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::IsLost()
{
	FIXME("glDirectDrawSurface7::IsLost: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{	
	if(locked) return DDERR_SURFACEBUSY;
	DWORD x,y;
	unsigned char *bitmap = (unsigned char *)malloc((ddInterface->GetBPP()/8) * ddsd.dwWidth * ddsd.dwHeight);
	unsigned short *&bmp16 = (unsigned short *&)bitmap;
	unsigned long *&bmp32 = (unsigned long *&)bitmap;
	unsigned char *temptex = (unsigned char *)malloc(NextMultipleOf4((ddInterface->GetBPP()/8)*fakex)*fakey);
	unsigned short *&tmptex16 = (unsigned short *&)temptex;
	unsigned long *&tmptex32 = (unsigned long *&)temptex;
	ddsd.lPitch = NextMultipleOf4(ddsd.dwWidth * (ddInterface->GetBPP()/8));
	DWORD pixsize = ddInterface->GetBPP()/8;
	float mulx, muly;
	if(!bitmap) return DDERR_OUTOFMEMORY;
	if(!temptex) return DDERR_OUTOFMEMORY;
	mulx = (float)fakex / (float)ddsd.dwWidth;
	muly = (float)fakey / (float)ddsd.dwHeight;
	switch(surfacetype)
	{
	case 0:
		FIXME("glDirectDrawSurface7::Lock: surface type 0 not supported yet");
		return DDERR_UNSUPPORTED;
		break;
	case 1:
		FIXME("glDirectDrawSurface7::Lock: surface type 1 not supported yet");
		return DDERR_UNSUPPORTED;
		break;
	case 2:
		glBindTexture(GL_TEXTURE_2D,this->texture);  // Select surface's texture
		glGetTexImage(GL_TEXTURE_2D,0,texformat,texformat2,temptex);
		switch(ddInterface->GetBPP())
		{
		case 8:
			for(y = 0; y < ddsd.dwHeight; y++)
				for(x = 0; x < ddsd.dwWidth; x++)
					bitmap[x + (ddsd.dwWidth*y)] = temptex[(int)(x*mulx) + (fakex*(int)(y*muly))];
			break;
		case 16:
			for(y = 0; y < ddsd.dwHeight; y++)
				for(x = 0; x < ddsd.dwWidth; x++)
					bmp16[x + (ddsd.dwWidth*y)] = tmptex16[(int)(x*mulx) + (fakex*(int)(y*muly))];
			break;
		case 32:
			for(y = 0; y < ddsd.dwHeight; y++)
				for(x = 0; x < ddsd.dwWidth; x++)
					bmp32[x + (ddsd.dwWidth*y)] = tmptex32[(int)(x*mulx) + (fakex*(int)(y*muly))];
			break;
		break;
		}
	}
	ddsd.lpSurface = bitmap;
	memcpy(lpDDSurfaceDesc,&ddsd,lpDDSurfaceDesc->dwSize);
	free(temptex);
	locked++;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::ReleaseDC(HDC hDC)
{
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
HRESULT WINAPI glDirectDrawSurface7::Restore()
{
	FIXME("glDirectDrawSurface7::Restore: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	FIXME("glDirectDrawSurface7::SetClipper: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	FIXME("glDirectDrawSurface7::GetColorKey: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::SetOverlayPosition(LONG lX, LONG lY)
{
	FIXME("glDirectDrawSurface7::SetOverlayPosition: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	FIXME("glDirectDrawSurface7::SetPalette: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::Unlock(LPRECT lpRect)
{
	if(!locked) return DDERR_NOTLOCKED;
	locked--;
	unsigned char *bitmap = (unsigned char *)ddsd.lpSurface;
	unsigned short *&bmp16 = (unsigned short *&)bitmap;
	unsigned long *&bmp32 = (unsigned long *&)bitmap;
	unsigned char *temptex = (unsigned char *)malloc(NextMultipleOf4((ddInterface->GetBPP()/8)*fakex)*fakey);
	unsigned short *&tmptex16 = (unsigned short *&)temptex;
	unsigned long *&tmptex32 = (unsigned long *&)temptex;
	if(!temptex) return DDERR_OUTOFMEMORY;
	float mulx, muly;
	DWORD x,y;
	mulx = (float)ddsd.dwWidth / (float)fakex;
	muly = (float)ddsd.dwHeight / (float)fakey;
	switch(ddInterface->GetBPP())
	{
	case 8:
		for(y = 0; y < fakey; y++)
			for(x = 0; x < fakex; x++)
				temptex[x + (NextMultipleOf4(fakex)*y)] = bitmap[(int)(x*mulx) + (ddsd.dwWidth*(int)(y*muly))];
		break;
	case 16:
		for(y = 0; y < fakey; y++)
			for(x = 0; x < fakex; x++)
				tmptex16[x + (NextMultipleOf2(fakex)*y)] = bmp16[(int)(x*mulx) + (ddsd.dwWidth*(int)(y*muly))];
		break;
	case 32:
		for(y = 0; y < fakey; y++)
			for(x = 0; x < fakex; x++)
				tmptex32[x + (fakex*y)] = bmp32[(int)(x*mulx) + (ddsd.dwWidth*(int)(y*muly))];
		break;
	break;
	}
	glBindTexture(GL_TEXTURE_2D,this->texture);  // Select surface's texture
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,fakex,fakey,0,texformat,texformat2,temptex);
	if(ddsd.lpSurface) free(ddsd.lpSurface);
	ddsd.lpSurface = NULL;
	free(temptex);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	FIXME("glDirectDrawSurface7::UpdateOverlay: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayDisplay(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::UpdateOverlayDisplay: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference)
{
	FIXME("glDirectDrawSurface7::UpdateOverlayZOrder: stub\n");
	return DDERR_GENERIC;
}
// ddraw 2+ api
HRESULT WINAPI glDirectDrawSurface7::GetDDInterface(LPVOID FAR *lplpDD)
{
	*lplpDD = ddInterface;
	FIXME("glDirectDrawSurface7::GetDDInterface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::PageLock(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::PageLock: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::PageUnlock(DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::PageUnlock: stub\n");
	return DDERR_GENERIC;
}
// ddraw 3+ api
HRESULT WINAPI glDirectDrawSurface7::SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags)
{
	FIXME("glDirectDrawSurface7::SetSurfaceDesc: stub\n");
	return DDERR_GENERIC;
}
// ddraw 4+ api
HRESULT WINAPI glDirectDrawSurface7::SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags)
{
	FIXME("glDirectDrawSurface7::SetPrivateData: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize)
{
	FIXME("glDirectDrawSurface7::GetPrivateData: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::FreePrivateData(REFGUID guidTag)
{
	FIXME("glDirectDrawSurface7::FreePrivateData: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetUniquenessValue(LPDWORD lpValue)
{
	FIXME("glDirectDrawSurface7::GetUniquenessValue: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::ChangeUniquenessValue()
{
	FIXME("glDirectDrawSurface7::ChangeUniquenessValue: stub\n");
	return DDERR_GENERIC;
}
// ddraw 7 api
HRESULT WINAPI glDirectDrawSurface7::SetPriority(DWORD dwPriority)
{
	FIXME("glDirectDrawSurface7::SetPriority: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetPriority(LPDWORD lpdwPriority)
{
	FIXME("glDirectDrawSurface7::GetPriority: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::SetLOD(DWORD dwMaxLOD)
{
	FIXME("glDirectDrawSurface7::SetLOD: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::GetLOD(LPDWORD lpdwMaxLOD)
{
	FIXME("glDirectDrawSurface7::GetLOD: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawSurface7::Unlock2(LPVOID lpSurfaceData)
{
	return Unlock((LPRECT)lpSurfaceData);
}

#pragma endregion

#pragma region DDRAW1 wrapper
glDirectDrawSurface1::glDirectDrawSurface1(glDirectDrawSurface7 *gl_DDS7)
{
	glDDS7 = gl_DDS7;
	refcount = 1;
}
glDirectDrawSurface1::~glDirectDrawSurface1()
{
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
	return glDDS7->Blt(lpDestRect,(LPDIRECTDRAWSURFACE7)lpDDSrcSurface,lpSrcRect,dwFlags,lpDDBltFx);
}
HRESULT WINAPI glDirectDrawSurface1::BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags)
{
	return glDDS7->BltBatch(lpDDBltBatch,dwCount,dwFlags);
}
HRESULT WINAPI glDirectDrawSurface1::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	return glDDS7->BltFast(dwX,dwY,(LPDIRECTDRAWSURFACE7)lpDDSrcSurface,lpSrcRect,dwTrans);
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
	FIXME("glDirectDrawSurface1::Initialize: stub\n");
	return DDERR_GENERIC;
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
#pragma endregion