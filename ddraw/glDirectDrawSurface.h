// DXGL
// Copyright (C) 2011-2013 William Feely

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

#pragma once
#ifndef _GLDIRECTDRAWSURFACE_H
#define _GLDIRECTDRAWSURFACE_H

typedef struct
{
	bool enabled;
	bool colorspace;
	DDCOLORKEY key;
} CKEY;

class glDirectDrawClipper;
class glDirectDrawPalette;
class glDirectDrawSurface1;
class glDirectDrawSurface2;
class glDirectDrawSurface3;
class glDirectDrawSurface4;
class glDirect3DTexture2;
class glDirect3DTexture1;
class glDirect3DDevice7;
class glDirectDrawSurface7 : public IDirectDrawSurface7
{
public:
	glDirectDrawSurface7(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 *lplpDDSurface7, HRESULT *error, bool copysurface, glDirectDrawPalette *palettein);
	virtual ~glDirectDrawSurface7();
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
	HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT WINAPI Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT WINAPI BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
	HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback);
	HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT WINAPI GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface);
	HRESULT WINAPI GetBltStatus(DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPDDSCAPS2 lpDDSCaps);
	HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI GetDC(HDC FAR *lphDC);
	HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
	HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT WINAPI IsLost();
	HRESULT WINAPI Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT WINAPI ReleaseDC(HDC hDC);
	HRESULT WINAPI Restore();
	HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
	HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT WINAPI Unlock(LPRECT lpRect);
	HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference);
	// ddraw 2+ api
	HRESULT WINAPI GetDDInterface(LPVOID FAR *lplpDD);
	HRESULT WINAPI PageLock(DWORD dwFlags);
	HRESULT WINAPI PageUnlock(DWORD dwFlags);
	// ddraw 3+ api
	HRESULT WINAPI SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
	// ddraw 4+ api
	HRESULT WINAPI SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
	HRESULT WINAPI GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
	HRESULT WINAPI FreePrivateData(REFGUID guidTag);
	HRESULT WINAPI GetUniquenessValue(LPDWORD lpValue);
	HRESULT WINAPI ChangeUniquenessValue();
	// ddraw 7 api
	HRESULT WINAPI SetPriority(DWORD dwPriority);
	HRESULT WINAPI GetPriority(LPDWORD lpdwPriority);
	HRESULT WINAPI SetLOD(DWORD dwMaxLOD);
	HRESULT WINAPI GetLOD(LPDWORD lpdwMaxLOD);
	void SetFilter(int level, GLint mag, GLint min);
	// internal functions
	TEXTURE *GetTexture(){
		return texture;
	}
	void Restore2();
	HRESULT Flip2(LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags);
	void SetTexture(TEXTURE *newtexture){texture = newtexture;};
	glDirectDrawSurface7 *GetBackbuffer(){return backbuffer;};
	glDirectDrawSurface7 *GetZBuffer(){return zbuffer;};
	void RenderScreen(TEXTURE *texture, glDirectDrawSurface7 *surface);
	// Special ddraw2->ddraw7 api
	HRESULT WINAPI Unlock2(LPVOID lpSurfaceData);
	HRESULT GetHandle(glDirect3DDevice7 *glD3DDev7, LPD3DTEXTUREHANDLE lpHandle);
	HRESULT Load(glDirectDrawSurface7 *src);
	glDirectDrawSurface1 *dds1;
	glDirectDrawSurface2 *dds2;
	glDirectDrawSurface3 *dds3;
	glDirectDrawSurface4 *dds4;
	glDirect3DTexture2 *d3dt2;
	glDirect3DTexture1 *d3dt1;
	DWORD flipcount;
	DWORD fakex,fakey;
	DWORD dirty;
	// dirty bits:
	// 1 - Surface was locked
	// 2 - Texture was written to by ddraw
	CKEY colorkey[4];
	TEXTURE *texture;
	TEXTURE *paltex;
	bool hasstencil;
	char *buffer;
	char *bigbuffer;
	char *gdibuffer;
	DDSURFACEDESC2 ddsd;
	glDirectDrawPalette *palette;
	HGLRC hRC;
	glDirectDrawSurface7 *zbuffer;
	D3DMATERIALHANDLE handle;
private:
	ULONG refcount;
	int locked;
	HDC hdc;
	HBITMAP hbitmap;
	BITMAPINFO *bitmapinfo;
	glDirectDraw7 *ddInterface;
	int surfacetype;  // 0-generic memory, 1-GDI surface, 2-OpenGL Texture
	glDirectDrawSurface7 *backbuffer;
	glDirectDrawClipper *clipper;
	int pagelocked;
	GLint magfilter,minfilter;
	glDirect3DDevice7 *device;
};

// Legacy DDRAW Interfaces
class glDirectDrawSurface1 : public IDirectDrawSurface
{
public:
	glDirectDrawSurface1(glDirectDrawSurface7 *gl_DDS7);
	virtual ~glDirectDrawSurface1();
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
	HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT WINAPI Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT WINAPI BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
	HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT WINAPI GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface);
	HRESULT WINAPI GetBltStatus(DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPDDSCAPS lpDDSCaps);
	HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI GetDC(HDC FAR *lphDC);
	HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
	HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI IsLost();
	HRESULT WINAPI Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT WINAPI ReleaseDC(HDC hDC);
	HRESULT WINAPI Restore();
	HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
	HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT WINAPI Unlock(LPVOID lpSurfaceData);
	HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference);
	glDirectDrawSurface7 *GetDDS7() {return glDDS7;};
private:
	UINT refcount;
	glDirectDrawSurface7 *glDDS7;
};
class glDirectDrawSurface2 : public IDirectDrawSurface2
{
public:
	glDirectDrawSurface2(glDirectDrawSurface7 *gl_DDS7);
	virtual ~glDirectDrawSurface2();
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
	HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT WINAPI Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT WINAPI BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
	HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT WINAPI GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface);
	HRESULT WINAPI GetBltStatus(DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPDDSCAPS lpDDSCaps);
	HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI GetDC(HDC FAR *lphDC);
	HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
	HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI IsLost();
	HRESULT WINAPI Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT WINAPI ReleaseDC(HDC hDC);
	HRESULT WINAPI Restore();
	HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
	HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT WINAPI Unlock(LPVOID lpSurfaceData);
	HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference);
	// ddraw 2+ api
	HRESULT WINAPI GetDDInterface(LPVOID FAR *lplpDD);
	HRESULT WINAPI PageLock(DWORD dwFlags);
	HRESULT WINAPI PageUnlock(DWORD dwFlags);
	glDirectDrawSurface7 *GetDDS7() {return glDDS7;};
private:
	UINT refcount;
	glDirectDrawSurface7 *glDDS7;
};
class glDirectDrawSurface3 : public IDirectDrawSurface3
{
public:
	glDirectDrawSurface3(glDirectDrawSurface7 *gl_DDS7);
	virtual ~glDirectDrawSurface3();
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
	HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT WINAPI Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT WINAPI BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
	HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT WINAPI GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface);
	HRESULT WINAPI GetBltStatus(DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPDDSCAPS lpDDSCaps);
	HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI GetDC(HDC FAR *lphDC);
	HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
	HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI IsLost();
	HRESULT WINAPI Lock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT WINAPI ReleaseDC(HDC hDC);
	HRESULT WINAPI Restore();
	HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
	HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT WINAPI Unlock(LPVOID lpSurfaceData);
	HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference);
	// ddraw 2+ api
	HRESULT WINAPI GetDDInterface(LPVOID FAR *lplpDD);
	HRESULT WINAPI PageLock(DWORD dwFlags);
	HRESULT WINAPI PageUnlock(DWORD dwFlags);
	// ddraw 3+ api
	HRESULT WINAPI SetSurfaceDesc(LPDDSURFACEDESC lpddsd2, DWORD dwFlags);
	glDirectDrawSurface7 *GetDDS7() {return glDDS7;};
private:
	UINT refcount;
	glDirectDrawSurface7 *glDDS7;
};
class glDirectDrawSurface4 : public IDirectDrawSurface4
{
public:
	glDirectDrawSurface4(glDirectDrawSurface7 *gl_DDS7);
	virtual ~glDirectDrawSurface4();
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddAttachedSurface(LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
	HRESULT WINAPI AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT WINAPI Blt(LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT WINAPI BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT WINAPI BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT WINAPI DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
	HRESULT WINAPI EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
	HRESULT WINAPI EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback);
	HRESULT WINAPI Flip(LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT WINAPI GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface);
	HRESULT WINAPI GetBltStatus(DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPDDSCAPS2 lpDDSCaps);
	HRESULT WINAPI GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT WINAPI GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI GetDC(HDC FAR *lphDC);
	HRESULT WINAPI GetFlipStatus(DWORD dwFlags);
	HRESULT WINAPI GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT WINAPI GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT WINAPI GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT WINAPI GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT WINAPI IsLost();
	HRESULT WINAPI Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT WINAPI ReleaseDC(HDC hDC);
	HRESULT WINAPI Restore();
	HRESULT WINAPI SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT WINAPI SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT WINAPI SetOverlayPosition(LONG lX, LONG lY);
	HRESULT WINAPI SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT WINAPI Unlock(LPRECT lpRect);
	HRESULT WINAPI UpdateOverlay(LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT WINAPI UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT WINAPI UpdateOverlayZOrder(DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference);
	// ddraw 2+ api
	HRESULT WINAPI GetDDInterface(LPVOID FAR *lplpDD);
	HRESULT WINAPI PageLock(DWORD dwFlags);
	HRESULT WINAPI PageUnlock(DWORD dwFlags);
	// ddraw 3+ api
	HRESULT WINAPI SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
	// ddraw 4+ api
	HRESULT WINAPI SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
	HRESULT WINAPI GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
	HRESULT WINAPI FreePrivateData(REFGUID guidTag);
	HRESULT WINAPI GetUniquenessValue(LPDWORD lpValue);
	HRESULT WINAPI ChangeUniquenessValue();
	glDirectDrawSurface7 *GetDDS7() {return glDDS7;};
private:
	UINT refcount;
	glDirectDrawSurface7 *glDDS7;
};
#endif //_GLDIRECTDRAWSURFACE_H
