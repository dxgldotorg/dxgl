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

#pragma once
#ifndef _GLDIRECTDRAWSURFACE_H
#define _GLDIRECTDRAWSURFACE_H

typedef struct
{
	bool enabled;
	bool colorspace;
	DDCOLORKEY key;
} CKEY;

struct glDirectDraw7;
struct glDirectDrawClipper;
struct glDirectDrawPalette;
struct dxglDirectDrawSurface1;
struct dxglDirectDrawSurface2;
struct dxglDirectDrawSurface3;
struct dxglDirectDrawSurface4;
struct glDirect3DTexture2;
struct glDirect3DTexture1;
struct glDirect3DDevice7;
struct glDirectDrawGammaControl;

struct dxglDirectDrawSurface1Vtbl;
typedef struct dxglDirectDrawSurface1
{
	dxglDirectDrawSurface1Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} dxglDirectDrawSurface1;

struct dxglDirectDrawSurface2Vtbl;
typedef struct dxglDirectDrawSurface2
{
	dxglDirectDrawSurface2Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} dxglDirectDrawSurface2;

struct dxglDirectDrawSurface3Vtbl;
typedef struct dxglDirectDrawSurface3
{
	dxglDirectDrawSurface3Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} dxglDirectDrawSurface3;

struct dxglDirectDrawSurface4Vtbl;
typedef struct dxglDirectDrawSurface4
{
	dxglDirectDrawSurface4Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} dxglDirectDrawSurface4;

struct glDirectDrawGammaControlVtbl;
typedef struct glDirectDrawGammaControl
{
	glDirectDrawGammaControlVtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} glDirectDrawGammaControl;

struct glDirect3DTexture1Vtbl;
typedef struct glDirect3DTexture1
{
	glDirect3DTexture1Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} glDirect3DTexture1;

struct glDirect3DTexture2Vtbl;
typedef struct glDirect3DTexture2
{
	glDirect3DTexture2Vtbl *lpVtbl;
	dxglDirectDrawSurface7 *glDDS7;
} glDirect3DTexture2;

struct dxglDirectDrawSurface7Vtbl;

// dxglDirectDrawSurface7 - DDraw surface
typedef struct dxglDirectDrawSurface7
{
	// COM function pointer table
	dxglDirectDrawSurface7Vtbl *lpVtbl;

	// Previous version interfaces
	dxglDirectDrawSurface1 dds1;
	dxglDirectDrawSurface2 dds2;
	dxglDirectDrawSurface3 dds3;
	dxglDirectDrawSurface4 dds4;
	glDirect3DTexture2 d3dt2;
	glDirect3DTexture1 d3dt1;
	// Gamma control interface
	glDirectDrawGammaControl gammacontrol;

	// Reference counts
	ULONG refcount7, refcount4, refcount3, refcount2, refcount1;
	ULONG refcountgamma, refcountcolor;

	// DDraw version of interface
	int version;

	// Surface description
	DDSURFACEDESC2 ddsd;

	// Number of times surface has flipped - resets to 0 when it reaches ddsd.dwBackBufferCount
	DWORD flipcount;

	// Big surface for scaled modes
	dxglDirectDrawSurface7 *bigsurface;

	// Parent of big surface
	dxglDirectDrawSurface7 *bigparent;

	// The texture represented by the surface
	glTexture *texture;

	// The parent surface, null if it's the top of a complex structure 
	dxglDirectDrawSurface7 *parent;

	// Z-buffer surface
	dxglDirectDrawSurface7 *zbuffer;
	IUnknown *zbuffer_iface;

	// Clipper object attached to surface
	glDirectDrawClipper *clipper;

	// Palette object attached to surface
	glDirectDrawPalette *palette;

	// Interface that created the surface
	glDirectDraw7 *ddInterface;
	IUnknown *creator;

	// Complex surface storage
	dxglDirectDrawSurface7 *attached;

	// Parent interface of IDirectDrawTexture interface
	IUnknown *textureparent;

	// Mipmap level of surface, used to reference texture miplevel
	DWORD miplevel;

	// Flip index of surface
	DWORD flipindex;

	// Parent surface for manually attached surfaces
	dxglDirectDrawSurface7 *attachparent;
	// Attachment count for manually attached surfaces
	DWORD attachcount;

	// D3D device attached to surface
	glDirect3DDevice7 *device1;
	glDirect3DDevice7 *device;

	// Tracks vertical sync interval for the primary surface.
	int swapinterval;

	// Non-zero if the surface has been locked by the DDraw API.
	int locked;

	// Tracks status of PageLock API.
	int pagelocked;

	// GL context that created the surface
	HGLRC hRC;

	// Texture handle for D3D
	D3DTEXTUREHANDLE handle;

	// Overlay information, to be removed/optimized in a later build
	BOOL overlayenabled;
	BOOL overlayset;
	int overlaycount;
	int maxoverlays;
	OVERLAY *overlays;
	dxglDirectDrawSurface7 *overlaydest;
	POINT overlaypos;

	// Padding on x86
#ifdef _M_IX86
	//DWORD padding[2];
#endif

	// Used by older versions of DXGL, may or may not be removed
	/*DWORD fakex, fakey;
	float mulx, muly;
	CKEY colorkey[4];
	glTexture *texture;
	bool hasstencil;
	dxglDirectDrawSurface7 *miptexture;
	dxglDirectDrawSurface7 *backbuffer;
	dxglDirectDrawSurface7 *backbufferwraparound;
	//int surfacetype;  // 0-generic memory, 1-GDI surface, 2-OpenGL Texture
	unsigned char *clientbuffer;*/
} dxglDirectDrawSurface7;

typedef struct dxglDirectDrawSurface7Vtbl
{
	HRESULT(WINAPI *QueryInterface)(dxglDirectDrawSurface7 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(dxglDirectDrawSurface7 *This);
	ULONG(WINAPI *Release)(dxglDirectDrawSurface7 *This);
	HRESULT(WINAPI *AddAttachedSurface)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
	HRESULT(WINAPI *AddOverlayDirtyRect)(dxglDirectDrawSurface7 *This, LPRECT lpRect);
	HRESULT(WINAPI *Blt)(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT(WINAPI *BltBatch)(dxglDirectDrawSurface7 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT(WINAPI *BltFast)(dxglDirectDrawSurface7 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT(WINAPI *DeleteAttachedSurface)(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
	HRESULT(WINAPI *EnumAttachedSurfaces)(dxglDirectDrawSurface7 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT(WINAPI *EnumOverlayZOrders)(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback);
	HRESULT(WINAPI *Flip)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT(WINAPI *GetAttachedSurface)(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface);
	HRESULT(WINAPI *GetBltStatus)(dxglDirectDrawSurface7 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetCaps)(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps);
	HRESULT(WINAPI *GetClipper)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT(WINAPI *GetColorKey)(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *GetDC)(dxglDirectDrawSurface7 *This, HDC FAR *lphDC);
	HRESULT(WINAPI *GetFlipStatus)(dxglDirectDrawSurface7 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetOverlayPosition)(dxglDirectDrawSurface7 *This, LPLONG lplX, LPLONG lplY);
	HRESULT(WINAPI *GetPalette)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT(WINAPI *GetPixelFormat)(dxglDirectDrawSurface7 *This, LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT(WINAPI *GetSurfaceDesc)(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT(WINAPI *Initialize)(dxglDirectDrawSurface7 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT(WINAPI *IsLost)(dxglDirectDrawSurface7 *This);
	HRESULT(WINAPI *Lock)(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *ReleaseDC)(dxglDirectDrawSurface7 *This, HDC hDC);
	HRESULT(WINAPI *Restore)(dxglDirectDrawSurface7 *This);
	HRESULT(WINAPI *SetClipper)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT(WINAPI *SetColorKey)(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *SetOverlayPosition)(dxglDirectDrawSurface7 *This, LONG lX, LONG lY);
	HRESULT(WINAPI *SetPalette)(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT(WINAPI *Unlock)(dxglDirectDrawSurface7 *This, LPRECT lpRect);
	HRESULT(WINAPI *UpdateOverlay)(dxglDirectDrawSurface7 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT(WINAPI *UpdateOverlayDisplay)(dxglDirectDrawSurface7 *This, DWORD dwFlags);
	HRESULT(WINAPI *UpdateOverlayZOrder)(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference);
	HRESULT(WINAPI *GetDDInterface)(dxglDirectDrawSurface7 *This, LPVOID FAR *lplpDD);
	HRESULT(WINAPI *PageLock)(dxglDirectDrawSurface7 *This, DWORD dwFlags);
	HRESULT(WINAPI *PageUnlock)(dxglDirectDrawSurface7 *This, DWORD dwFlags);
	HRESULT(WINAPI *SetSurfaceDesc)(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
	HRESULT(WINAPI *SetPrivateData)(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
	HRESULT(WINAPI *GetPrivateData)(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
	HRESULT(WINAPI *FreePrivateData)(dxglDirectDrawSurface7 *This, REFGUID guidTag);
	HRESULT(WINAPI *GetUniquenessValue)(dxglDirectDrawSurface7 *This, LPDWORD lpValue);
	HRESULT(WINAPI *ChangeUniquenessValue)(dxglDirectDrawSurface7 *This);
	HRESULT(WINAPI *SetPriority)(dxglDirectDrawSurface7 *This, DWORD dwPriority);
	HRESULT(WINAPI *GetPriority)(dxglDirectDrawSurface7 *This, LPDWORD lpdwPriority);
	HRESULT(WINAPI *SetLOD)(dxglDirectDrawSurface7 *This, DWORD dwMaxLOD);
	HRESULT(WINAPI *GetLOD)(dxglDirectDrawSurface7 *This, LPDWORD lpdwMaxLOD);
} glDirectDrawSurface7Vtbl;

HRESULT dxglDirectDrawSurface7_Create(LPDIRECTDRAW7 lpDD7, LPDDSURFACEDESC2 lpDDSurfaceDesc2, glDirectDrawPalette *palettein,
	glTexture *parenttex, int version, dxglDirectDrawSurface7 *glDDS7);
void dxglDirectDrawSurface7_Delete(dxglDirectDrawSurface7 *This);
// ddraw 1+ api
HRESULT WINAPI dxglDirectDrawSurface7_QueryInterface(dxglDirectDrawSurface7 *This, REFIID riid, void** ppvObj);
ULONG WINAPI dxglDirectDrawSurface7_AddRef(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_Release(dxglDirectDrawSurface7 *This);
HRESULT WINAPI dxglDirectDrawSurface7_AddAttachedSurface(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface7_AddOverlayDirtyRect(dxglDirectDrawSurface7 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface7_Blt(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI dxglDirectDrawSurface7_BltBatch(dxglDirectDrawSurface7 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_BltFast(dxglDirectDrawSurface7 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE7 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI dxglDirectDrawSurface7_DeleteAttachedSurface(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface7_EnumAttachedSurfaces(dxglDirectDrawSurface7 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
HRESULT WINAPI dxglDirectDrawSurface7_EnumOverlayZOrders(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback);
HRESULT WINAPI dxglDirectDrawSurface7_Flip(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_GetAttachedSurface(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE7 FAR *lplpDDAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface7_GetBltStatus(dxglDirectDrawSurface7 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_GetCaps(dxglDirectDrawSurface7 *This, LPDDSCAPS2 lpDDSCaps);
HRESULT WINAPI dxglDirectDrawSurface7_GetClipper(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface7_GetColorKey(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface7_GetDC(dxglDirectDrawSurface7 *This, HDC FAR *lphDC);
HRESULT WINAPI dxglDirectDrawSurface7_GetFlipStatus(dxglDirectDrawSurface7 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_GetOverlayPosition(dxglDirectDrawSurface7 *This, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI dxglDirectDrawSurface7_GetPalette(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface7_GetPixelFormat(dxglDirectDrawSurface7 *This, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI dxglDirectDrawSurface7_GetSurfaceDesc(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface7_Initialize(dxglDirectDrawSurface7 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface7_IsLost(dxglDirectDrawSurface7 *This);
HRESULT WINAPI dxglDirectDrawSurface7_Lock(dxglDirectDrawSurface7 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI dxglDirectDrawSurface7_ReleaseDC(dxglDirectDrawSurface7 *This, HDC hDC);
HRESULT WINAPI dxglDirectDrawSurface7_Restore(dxglDirectDrawSurface7 *This);
HRESULT WINAPI dxglDirectDrawSurface7_SetClipper(dxglDirectDrawSurface7 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface7_SetColorKey(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface7_SetOverlayPosition(dxglDirectDrawSurface7 *This, LONG lX, LONG lY);
HRESULT WINAPI dxglDirectDrawSurface7_SetPalette(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface7_Unlock(dxglDirectDrawSurface7 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlay(dxglDirectDrawSurface7 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE7 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlayDisplay(dxglDirectDrawSurface7 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_UpdateOverlayZOrder(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE7 lpDDSReference);
// ddraw 2+ api
HRESULT WINAPI dxglDirectDrawSurface7_GetDDInterface(dxglDirectDrawSurface7 *This, LPVOID FAR *lplpDD);
HRESULT WINAPI dxglDirectDrawSurface7_PageLock(dxglDirectDrawSurface7 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_PageUnlock(dxglDirectDrawSurface7 *This, DWORD dwFlags);
// ddraw 3+ api
HRESULT WINAPI dxglDirectDrawSurface7_SetSurfaceDesc(dxglDirectDrawSurface7 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
// ddraw 4+ api
HRESULT WINAPI dxglDirectDrawSurface7_SetPrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
HRESULT WINAPI dxglDirectDrawSurface7_GetPrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
HRESULT WINAPI dxglDirectDrawSurface7_FreePrivateData(dxglDirectDrawSurface7 *This, REFGUID guidTag);
HRESULT WINAPI dxglDirectDrawSurface7_GetUniquenessValue(dxglDirectDrawSurface7 *This, LPDWORD lpValue);
HRESULT WINAPI dxglDirectDrawSurface7_ChangeUniquenessValue(dxglDirectDrawSurface7 *This);
// ddraw 7 api
HRESULT WINAPI dxglDirectDrawSurface7_SetPriority(dxglDirectDrawSurface7 *This, DWORD dwPriority);
HRESULT WINAPI dxglDirectDrawSurface7_GetPriority(dxglDirectDrawSurface7 *This, LPDWORD lpdwPriority);
HRESULT WINAPI dxglDirectDrawSurface7_SetLOD(dxglDirectDrawSurface7 *This, DWORD dwMaxLOD);
HRESULT WINAPI dxglDirectDrawSurface7_GetLOD(dxglDirectDrawSurface7 *This, LPDWORD lpdwMaxLOD);
// internal functions
ULONG WINAPI dxglDirectDrawSurface7_AddRef4(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_Release4(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_AddRef3(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_Release3(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_AddRef2(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_Release2(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_AddRef1(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_Release1(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_AddRefGamma(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_ReleaseGamma(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_AddRefColor(dxglDirectDrawSurface7 *This);
ULONG WINAPI dxglDirectDrawSurface7_ReleaseColor(dxglDirectDrawSurface7 *This);
HRESULT WINAPI dxglDirectDrawSurface7_SetPaletteNoDraw(dxglDirectDrawSurface7 *This, LPDIRECTDRAWPALETTE lpDDPalette);
void dxglDirectDrawSurface7_Restore2(dxglDirectDrawSurface7 *This);
HRESULT dxglDirectDrawSurface7_Flip2(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSurfaceTargetOverride, DWORD dwFlags, glTexture **previous);
HRESULT dxglDirectDrawSurface7_AddAttachedSurface2(dxglDirectDrawSurface7 *This, LPDIRECTDRAWSURFACE7 lpDDSAttachedSurface, IUnknown *iface);
void dxglDirectDrawSurface7_SetTexture(dxglDirectDrawSurface7 *This, glTexture *newtexture);
void dxglDirectDrawSurface7_RenderScreen(dxglDirectDrawSurface7 *This, glTexture *texture, int vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount);
// Special ddraw2->ddraw7 api
HRESULT WINAPI dxglDirectDrawSurface7_Unlock2(dxglDirectDrawSurface7 *This, LPVOID lpSurfaceData);
HRESULT dxglDirectDrawSurface7_GetHandle(dxglDirectDrawSurface7 *This, glDirect3DDevice7 *glD3DDev7, LPD3DTEXTUREHANDLE lpHandle);
HRESULT dxglDirectDrawSurface7_Load(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *src);
HRESULT dxglDirectDrawSurface7_GetGammaRamp(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);
HRESULT dxglDirectDrawSurface7_SetGammaRamp(dxglDirectDrawSurface7 *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);
HRESULT dxglDirectDrawSurface7_AddOverlay(dxglDirectDrawSurface7 *This, OVERLAY *overlay);
HRESULT dxglDirectDrawSurface7_DeleteOverlay(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *surface);
HRESULT dxglDirectDrawSurface7_UpdateOverlayTexture(dxglDirectDrawSurface7 *This, dxglDirectDrawSurface7 *surface, glTexture *texture);

// Legacy DDRAW Interfaces
typedef struct dxglDirectDrawSurface1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(dxglDirectDrawSurface1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *Release)(dxglDirectDrawSurface1 *This);
	ULONG(WINAPI *AddRef)(dxglDirectDrawSurface1 *This);
	HRESULT(WINAPI *AddAttachedSurface)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
	HRESULT(WINAPI *AddOverlayDirtyRect)(dxglDirectDrawSurface1 *This, LPRECT lpRect);
	HRESULT(WINAPI *Blt)(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT(WINAPI *BltBatch)(dxglDirectDrawSurface1 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT(WINAPI *BltFast)(dxglDirectDrawSurface1 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT(WINAPI *DeleteAttachedSurface)(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
	HRESULT(WINAPI *EnumAttachedSurfaces)(dxglDirectDrawSurface1 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT(WINAPI *EnumOverlayZOrders)(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT(WINAPI *Flip)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT(WINAPI *GetAttachedSurface)(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface);
	HRESULT(WINAPI *GetBltStatus)(dxglDirectDrawSurface1 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetCaps)(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps);
	HRESULT(WINAPI *GetClipper)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT(WINAPI *GetColorKey)(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *GetDC)(dxglDirectDrawSurface1 *This, HDC FAR *lphDC);
	HRESULT(WINAPI *GetFlipStatus)(dxglDirectDrawSurface1 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetOverlayPosition)(dxglDirectDrawSurface1 *This, LPLONG lplX, LPLONG lplY);
	HRESULT(WINAPI *GetPalette)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT(WINAPI *GetPixelFormat)(dxglDirectDrawSurface1 *This, LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT(WINAPI *GetSurfaceDesc)(dxglDirectDrawSurface1 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *Initialize)(dxglDirectDrawSurface1 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *IsLost)(dxglDirectDrawSurface1 *This);
	HRESULT(WINAPI *Lock)(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *ReleaseDC)(dxglDirectDrawSurface1 *This, HDC hDC);
	HRESULT(WINAPI *Restore)(dxglDirectDrawSurface1 *This);
	HRESULT(WINAPI *SetClipper)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT(WINAPI *SetColorKey)(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *SetOverlayPosition)(dxglDirectDrawSurface1 *This, LONG lX, LONG lY);
	HRESULT(WINAPI *SetPalette)(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT(WINAPI *Unlock)(dxglDirectDrawSurface1 *This, LPVOID lpSurfaceData);
	HRESULT(WINAPI *UpdateOverlay)(dxglDirectDrawSurface1 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT(WINAPI *UpdateOverlayDisplay)(dxglDirectDrawSurface1 *This, DWORD dwFlags);
	HRESULT(WINAPI *UpdateOverlayZOrder)(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference);
} dxglDirectDrawSurface1Vtbl;


HRESULT dxglDirectDrawSurface1_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface1 *glDDS1);
// ddraw 1+ api
HRESULT WINAPI dxglDirectDrawSurface1_QueryInterface(dxglDirectDrawSurface1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI dxglDirectDrawSurface1_AddRef(dxglDirectDrawSurface1 *This);
ULONG WINAPI dxglDirectDrawSurface1_Release(dxglDirectDrawSurface1 *This);
HRESULT WINAPI dxglDirectDrawSurface1_AddAttachedSurface(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface1_AddOverlayDirtyRect(dxglDirectDrawSurface1 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface1_Blt(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI dxglDirectDrawSurface1_BltBatch(dxglDirectDrawSurface1 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface1_BltFast(dxglDirectDrawSurface1 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI dxglDirectDrawSurface1_DeleteAttachedSurface(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface1_EnumAttachedSurfaces(dxglDirectDrawSurface1 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI dxglDirectDrawSurface1_EnumOverlayZOrders(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
HRESULT WINAPI dxglDirectDrawSurface1_Flip(dxglDirectDrawSurface1 *This, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface1_GetAttachedSurface(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR *lplpDDAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface1_GetBltStatus(dxglDirectDrawSurface1 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface1_GetCaps(dxglDirectDrawSurface1 *This, LPDDSCAPS lpDDSCaps);
HRESULT WINAPI dxglDirectDrawSurface1_GetClipper(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface1_GetColorKey(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface1_GetDC(dxglDirectDrawSurface1 *This, HDC FAR *lphDC);
HRESULT WINAPI dxglDirectDrawSurface1_GetFlipStatus(dxglDirectDrawSurface1 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface1_GetOverlayPosition(dxglDirectDrawSurface1 *This, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI dxglDirectDrawSurface1_GetPalette(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface1_GetPixelFormat(dxglDirectDrawSurface1 *This, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI dxglDirectDrawSurface1_GetSurfaceDesc(dxglDirectDrawSurface1 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface1_Initialize(dxglDirectDrawSurface1 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface1_IsLost(dxglDirectDrawSurface1 *This);
HRESULT WINAPI dxglDirectDrawSurface1_Lock(dxglDirectDrawSurface1 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI dxglDirectDrawSurface1_ReleaseDC(dxglDirectDrawSurface1 *This, HDC hDC);
HRESULT WINAPI dxglDirectDrawSurface1_Restore(dxglDirectDrawSurface1 *This);
HRESULT WINAPI dxglDirectDrawSurface1_SetClipper(dxglDirectDrawSurface1 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface1_SetColorKey(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface1_SetOverlayPosition(dxglDirectDrawSurface1 *This, LONG lX, LONG lY);
HRESULT WINAPI dxglDirectDrawSurface1_SetPalette(dxglDirectDrawSurface1 *This, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface1_Unlock(dxglDirectDrawSurface1 *This, LPVOID lpSurfaceData);
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlay(dxglDirectDrawSurface1 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlayDisplay(dxglDirectDrawSurface1 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface1_UpdateOverlayZOrder(dxglDirectDrawSurface1 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference);

typedef struct dxglDirectDrawSurface2Vtbl
{
	HRESULT(WINAPI *QueryInterface)(dxglDirectDrawSurface2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(dxglDirectDrawSurface2 *This);
	ULONG(WINAPI *Release)(dxglDirectDrawSurface2 *This);
	HRESULT(WINAPI *AddAttachedSurface)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
	HRESULT(WINAPI *AddOverlayDirtyRect)(dxglDirectDrawSurface2 *This, LPRECT lpRect);
	HRESULT(WINAPI *Blt)(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT(WINAPI *BltBatch)(dxglDirectDrawSurface2 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT(WINAPI *BltFast)(dxglDirectDrawSurface2 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT(WINAPI *DeleteAttachedSurface)(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
	HRESULT(WINAPI *EnumAttachedSurfaces)(dxglDirectDrawSurface2 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT(WINAPI *EnumOverlayZOrders)(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT(WINAPI *Flip)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT(WINAPI *GetAttachedSurface)(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface);
	HRESULT(WINAPI *GetBltStatus)(dxglDirectDrawSurface2 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetCaps)(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps);
	HRESULT(WINAPI *GetClipper)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT(WINAPI *GetColorKey)(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *GetDC)(dxglDirectDrawSurface2 *This, HDC FAR *lphDC);
	HRESULT(WINAPI *GetFlipStatus)(dxglDirectDrawSurface2 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetOverlayPosition)(dxglDirectDrawSurface2 *This, LPLONG lplX, LPLONG lplY);
	HRESULT(WINAPI *GetPalette)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT(WINAPI *GetPixelFormat)(dxglDirectDrawSurface2 *This, LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT(WINAPI *GetSurfaceDesc)(dxglDirectDrawSurface2 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *Initialize)(dxglDirectDrawSurface2 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *IsLost)(dxglDirectDrawSurface2 *This);
	HRESULT(WINAPI *Lock)(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *ReleaseDC)(dxglDirectDrawSurface2 *This, HDC hDC);
	HRESULT(WINAPI *Restore)(dxglDirectDrawSurface2 *This);
	HRESULT(WINAPI *SetClipper)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT(WINAPI *SetColorKey)(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *SetOverlayPosition)(dxglDirectDrawSurface2 *This, LONG lX, LONG lY);
	HRESULT(WINAPI *SetPalette)(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT(WINAPI *Unlock)(dxglDirectDrawSurface2 *This, LPVOID lpSurfaceData);
	HRESULT(WINAPI *UpdateOverlay)(dxglDirectDrawSurface2 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT(WINAPI *UpdateOverlayDisplay)(dxglDirectDrawSurface2 *This, DWORD dwFlags);
	HRESULT(WINAPI *UpdateOverlayZOrder)(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference);
	HRESULT(WINAPI *GetDDInterface)(dxglDirectDrawSurface2 *This, LPVOID FAR *lplpDD);
	HRESULT(WINAPI *PageLock)(dxglDirectDrawSurface2 *This, DWORD dwFlags);
	HRESULT(WINAPI *PageUnlock)(dxglDirectDrawSurface2 *This, DWORD dwFlags);
} dxglDirectDrawSurface2Vtbl;

HRESULT dxglDirectDrawSurface2_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface2 *glDDS2);
// ddraw 1+ api
HRESULT WINAPI dxglDirectDrawSurface2_QueryInterface(dxglDirectDrawSurface2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI dxglDirectDrawSurface2_AddRef(dxglDirectDrawSurface2 *This);
ULONG WINAPI dxglDirectDrawSurface2_Release(dxglDirectDrawSurface2 *This);
HRESULT WINAPI dxglDirectDrawSurface2_AddAttachedSurface(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface2_AddOverlayDirtyRect(dxglDirectDrawSurface2 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface2_Blt(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI dxglDirectDrawSurface2_BltBatch(dxglDirectDrawSurface2 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_BltFast(dxglDirectDrawSurface2 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE2 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI dxglDirectDrawSurface2_DeleteAttachedSurface(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface2_EnumAttachedSurfaces(dxglDirectDrawSurface2 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI dxglDirectDrawSurface2_EnumOverlayZOrders(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
HRESULT WINAPI dxglDirectDrawSurface2_Flip(dxglDirectDrawSurface2 *This, LPDIRECTDRAWSURFACE2 lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_GetAttachedSurface(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR *lplpDDAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface2_GetBltStatus(dxglDirectDrawSurface2 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_GetCaps(dxglDirectDrawSurface2 *This, LPDDSCAPS lpDDSCaps);
HRESULT WINAPI dxglDirectDrawSurface2_GetClipper(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface2_GetColorKey(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface2_GetDC(dxglDirectDrawSurface2 *This, HDC FAR *lphDC);
HRESULT WINAPI dxglDirectDrawSurface2_GetFlipStatus(dxglDirectDrawSurface2 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_GetOverlayPosition(dxglDirectDrawSurface2 *This, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI dxglDirectDrawSurface2_GetPalette(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface2_GetPixelFormat(dxglDirectDrawSurface2 *This, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI dxglDirectDrawSurface2_GetSurfaceDesc(dxglDirectDrawSurface2 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface2_Initialize(dxglDirectDrawSurface2 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface2_IsLost(dxglDirectDrawSurface2 *This);
HRESULT WINAPI dxglDirectDrawSurface2_Lock(dxglDirectDrawSurface2 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI dxglDirectDrawSurface2_ReleaseDC(dxglDirectDrawSurface2 *This, HDC hDC);
HRESULT WINAPI dxglDirectDrawSurface2_Restore(dxglDirectDrawSurface2 *This);
HRESULT WINAPI dxglDirectDrawSurface2_SetClipper(dxglDirectDrawSurface2 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface2_SetColorKey(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface2_SetOverlayPosition(dxglDirectDrawSurface2 *This, LONG lX, LONG lY);
HRESULT WINAPI dxglDirectDrawSurface2_SetPalette(dxglDirectDrawSurface2 *This, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface2_Unlock(dxglDirectDrawSurface2 *This, LPVOID lpSurfaceData);
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlay(dxglDirectDrawSurface2 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE2 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlayDisplay(dxglDirectDrawSurface2 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_UpdateOverlayZOrder(dxglDirectDrawSurface2 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDSReference);
// ddraw 2+ api
HRESULT WINAPI dxglDirectDrawSurface2_GetDDInterface(dxglDirectDrawSurface2 *This, LPVOID FAR *lplpDD);
HRESULT WINAPI dxglDirectDrawSurface2_PageLock(dxglDirectDrawSurface2 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface2_PageUnlock(dxglDirectDrawSurface2 *This, DWORD dwFlags);

typedef struct dxglDirectDrawSurface3Vtbl
{
	HRESULT(WINAPI *QueryInterface)(dxglDirectDrawSurface3 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(dxglDirectDrawSurface3 *This);
	ULONG(WINAPI *Release)(dxglDirectDrawSurface3 *This);
	HRESULT(WINAPI *AddAttachedSurface)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
	HRESULT(WINAPI *AddOverlayDirtyRect)(dxglDirectDrawSurface3 *This, LPRECT lpRect);
	HRESULT(WINAPI *Blt)(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT(WINAPI *BltBatch)(dxglDirectDrawSurface3 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT(WINAPI *BltFast)(dxglDirectDrawSurface3 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT(WINAPI *DeleteAttachedSurface)(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
	HRESULT(WINAPI *EnumAttachedSurfaces)(dxglDirectDrawSurface3 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT(WINAPI *EnumOverlayZOrders)(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
	HRESULT(WINAPI *Flip)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT(WINAPI *GetAttachedSurface)(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface);
	HRESULT(WINAPI *GetBltStatus)(dxglDirectDrawSurface3 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetCaps)(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps);
	HRESULT(WINAPI *GetClipper)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT(WINAPI *GetColorKey)(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *GetDC)(dxglDirectDrawSurface3 *This, HDC FAR *lphDC);
	HRESULT(WINAPI *GetFlipStatus)(dxglDirectDrawSurface3 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetOverlayPosition)(dxglDirectDrawSurface3 *This, LPLONG lplX, LPLONG lplY);
	HRESULT(WINAPI *GetPalette)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT(WINAPI *GetPixelFormat)(dxglDirectDrawSurface3 *This, LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT(WINAPI *GetSurfaceDesc)(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *Initialize)(dxglDirectDrawSurface3 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *IsLost)(dxglDirectDrawSurface3 *This);
	HRESULT(WINAPI *Lock)(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *ReleaseDC)(dxglDirectDrawSurface3 *This, HDC hDC);
	HRESULT(WINAPI *Restore)(dxglDirectDrawSurface3 *This);
	HRESULT(WINAPI *SetClipper)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT(WINAPI *SetColorKey)(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *SetOverlayPosition)(dxglDirectDrawSurface3 *This, LONG lX, LONG lY);
	HRESULT(WINAPI *SetPalette)(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT(WINAPI *Unlock)(dxglDirectDrawSurface3 *This, LPVOID lpSurfaceData);
	HRESULT(WINAPI *UpdateOverlay)(dxglDirectDrawSurface3 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT(WINAPI *UpdateOverlayDisplay)(dxglDirectDrawSurface3 *This, DWORD dwFlags);
	HRESULT(WINAPI *UpdateOverlayZOrder)(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference);
	HRESULT(WINAPI *GetDDInterface)(dxglDirectDrawSurface3 *This, LPVOID FAR *lplpDD);
	HRESULT(WINAPI *PageLock)(dxglDirectDrawSurface3 *This, DWORD dwFlags);
	HRESULT(WINAPI *PageUnlock)(dxglDirectDrawSurface3 *This, DWORD dwFlags);
	HRESULT(WINAPI *SetSurfaceDesc)(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpddsd2, DWORD dwFlags);
} dxglDirectDrawSurface3Vtbl;

HRESULT dxglDirectDrawSurface3_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface3 *glDDS3);
// ddraw 1+ api
HRESULT WINAPI dxglDirectDrawSurface3_QueryInterface(dxglDirectDrawSurface3 *This, REFIID riid, void** ppvObj);
ULONG WINAPI dxglDirectDrawSurface3_AddRef(dxglDirectDrawSurface3 *This);
ULONG WINAPI dxglDirectDrawSurface3_Release(dxglDirectDrawSurface3 *This);
HRESULT WINAPI dxglDirectDrawSurface3_AddAttachedSurface(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface3_AddOverlayDirtyRect(dxglDirectDrawSurface3 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface3_Blt(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI dxglDirectDrawSurface3_BltBatch(dxglDirectDrawSurface3 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_BltFast(dxglDirectDrawSurface3 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE3 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI dxglDirectDrawSurface3_DeleteAttachedSurface(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface3_EnumAttachedSurfaces(dxglDirectDrawSurface3 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI dxglDirectDrawSurface3_EnumOverlayZOrders(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
HRESULT WINAPI dxglDirectDrawSurface3_Flip(dxglDirectDrawSurface3 *This, LPDIRECTDRAWSURFACE3 lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_GetAttachedSurface(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR *lplpDDAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface3_GetBltStatus(dxglDirectDrawSurface3 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_GetCaps(dxglDirectDrawSurface3 *This, LPDDSCAPS lpDDSCaps);
HRESULT WINAPI dxglDirectDrawSurface3_GetClipper(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface3_GetColorKey(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface3_GetDC(dxglDirectDrawSurface3 *This, HDC FAR *lphDC);
HRESULT WINAPI dxglDirectDrawSurface3_GetFlipStatus(dxglDirectDrawSurface3 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_GetOverlayPosition(dxglDirectDrawSurface3 *This, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI dxglDirectDrawSurface3_GetPalette(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface3_GetPixelFormat(dxglDirectDrawSurface3 *This, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI dxglDirectDrawSurface3_GetSurfaceDesc(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface3_Initialize(dxglDirectDrawSurface3 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface3_IsLost(dxglDirectDrawSurface3 *This);
HRESULT WINAPI dxglDirectDrawSurface3_Lock(dxglDirectDrawSurface3 *This, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI dxglDirectDrawSurface3_ReleaseDC(dxglDirectDrawSurface3 *This, HDC hDC);
HRESULT WINAPI dxglDirectDrawSurface3_Restore(dxglDirectDrawSurface3 *This);
HRESULT WINAPI dxglDirectDrawSurface3_SetClipper(dxglDirectDrawSurface3 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface3_SetColorKey(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface3_SetOverlayPosition(dxglDirectDrawSurface3 *This, LONG lX, LONG lY);
HRESULT WINAPI dxglDirectDrawSurface3_SetPalette(dxglDirectDrawSurface3 *This, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface3_Unlock(dxglDirectDrawSurface3 *This, LPVOID lpSurfaceData);
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlay(dxglDirectDrawSurface3 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE3 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlayDisplay(dxglDirectDrawSurface3 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_UpdateOverlayZOrder(dxglDirectDrawSurface3 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDSReference);
// ddraw 2+ api
HRESULT WINAPI dxglDirectDrawSurface3_GetDDInterface(dxglDirectDrawSurface3 *This, LPVOID FAR *lplpDD);
HRESULT WINAPI dxglDirectDrawSurface3_PageLock(dxglDirectDrawSurface3 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface3_PageUnlock(dxglDirectDrawSurface3 *This, DWORD dwFlags);
// ddraw 3+ api
HRESULT WINAPI dxglDirectDrawSurface3_SetSurfaceDesc(dxglDirectDrawSurface3 *This, LPDDSURFACEDESC lpddsd2, DWORD dwFlags);

typedef struct dxglDirectDrawSurface4Vtbl
{
	HRESULT(WINAPI *QueryInterface)(dxglDirectDrawSurface4 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(dxglDirectDrawSurface4 *This);
	ULONG(WINAPI *Release)(dxglDirectDrawSurface4 *This);
	HRESULT(WINAPI *AddAttachedSurface)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
	HRESULT(WINAPI *AddOverlayDirtyRect)(dxglDirectDrawSurface4 *This, LPRECT lpRect);
	HRESULT(WINAPI *Blt)(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT(WINAPI *BltBatch)(dxglDirectDrawSurface4 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT(WINAPI *BltFast)(dxglDirectDrawSurface4 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT(WINAPI *DeleteAttachedSurface)(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
	HRESULT(WINAPI *EnumAttachedSurfaces)(dxglDirectDrawSurface4 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
	HRESULT(WINAPI *EnumOverlayZOrders)(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback);
	HRESULT(WINAPI *Flip)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT(WINAPI *GetAttachedSurface)(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface);
	HRESULT(WINAPI *GetBltStatus)(dxglDirectDrawSurface4 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetCaps)(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps);
	HRESULT(WINAPI *GetClipper)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT(WINAPI *GetColorKey)(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *GetDC)(dxglDirectDrawSurface4 *This, HDC FAR *lphDC);
	HRESULT(WINAPI *GetFlipStatus)(dxglDirectDrawSurface4 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetOverlayPosition)(dxglDirectDrawSurface4 *This, LPLONG lplX, LPLONG lplY);
	HRESULT(WINAPI *GetPalette)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT(WINAPI *GetPixelFormat)(dxglDirectDrawSurface4 *This, LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT(WINAPI *GetSurfaceDesc)(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT(WINAPI *Initialize)(dxglDirectDrawSurface4 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT(WINAPI *IsLost)(dxglDirectDrawSurface4 *This);
	HRESULT(WINAPI *Lock)(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *ReleaseDC)(dxglDirectDrawSurface4 *This, HDC hDC);
	HRESULT(WINAPI *Restore)(dxglDirectDrawSurface4 *This);
	HRESULT(WINAPI *SetClipper)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT(WINAPI *SetColorKey)(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT(WINAPI *SetOverlayPosition)(dxglDirectDrawSurface4 *This, LONG lX, LONG lY);
	HRESULT(WINAPI *SetPalette)(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT(WINAPI *Unlock)(dxglDirectDrawSurface4 *This, LPRECT lpRect);
	HRESULT(WINAPI *UpdateOverlay)(dxglDirectDrawSurface4 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT(WINAPI *UpdateOverlayDisplay)(dxglDirectDrawSurface4 *This, DWORD dwFlags);
	HRESULT(WINAPI *UpdateOverlayZOrder)(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference);
	HRESULT(WINAPI *GetDDInterface)(dxglDirectDrawSurface4 *This, LPVOID FAR *lplpDD);
	HRESULT(WINAPI *PageLock)(dxglDirectDrawSurface4 *This, DWORD dwFlags);
	HRESULT(WINAPI *PageUnlock)(dxglDirectDrawSurface4 *This, DWORD dwFlags);
	HRESULT(WINAPI *SetSurfaceDesc)(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
	HRESULT(WINAPI *SetPrivateData)(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
	HRESULT(WINAPI *GetPrivateData)(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
	HRESULT(WINAPI *FreePrivateData)(dxglDirectDrawSurface4 *This, REFGUID guidTag);
	HRESULT(WINAPI *GetUniquenessValue)(dxglDirectDrawSurface4 *This, LPDWORD lpValue);
	HRESULT(WINAPI *ChangeUniquenessValue)(dxglDirectDrawSurface4 *This);
} dxglDirectDrawSurface4Vtbl;

HRESULT dxglDirectDrawSurface4_Create(dxglDirectDrawSurface7 *gl_DDS7, dxglDirectDrawSurface4 *glDDS4);
// ddraw 1+ api
HRESULT WINAPI dxglDirectDrawSurface4_QueryInterface(dxglDirectDrawSurface4 *This, REFIID riid, void** ppvObj);
ULONG WINAPI dxglDirectDrawSurface4_AddRef(dxglDirectDrawSurface4 *This);
ULONG WINAPI dxglDirectDrawSurface4_Release(dxglDirectDrawSurface4 *This);
HRESULT WINAPI dxglDirectDrawSurface4_AddAttachedSurface(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface4_AddOverlayDirtyRect(dxglDirectDrawSurface4 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface4_Blt(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI dxglDirectDrawSurface4_BltBatch(dxglDirectDrawSurface4 *This, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_BltFast(dxglDirectDrawSurface4 *This, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE4 lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI dxglDirectDrawSurface4_DeleteAttachedSurface(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface4_EnumAttachedSurfaces(dxglDirectDrawSurface4 *This, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
HRESULT WINAPI dxglDirectDrawSurface4_EnumOverlayZOrders(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpfnCallback);
HRESULT WINAPI dxglDirectDrawSurface4_Flip(dxglDirectDrawSurface4 *This, LPDIRECTDRAWSURFACE4 lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_GetAttachedSurface(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4 FAR *lplpDDAttachedSurface);
HRESULT WINAPI dxglDirectDrawSurface4_GetBltStatus(dxglDirectDrawSurface4 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_GetCaps(dxglDirectDrawSurface4 *This, LPDDSCAPS2 lpDDSCaps);
HRESULT WINAPI dxglDirectDrawSurface4_GetClipper(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface4_GetColorKey(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface4_GetDC(dxglDirectDrawSurface4 *This, HDC FAR *lphDC);
HRESULT WINAPI dxglDirectDrawSurface4_GetFlipStatus(dxglDirectDrawSurface4 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_GetOverlayPosition(dxglDirectDrawSurface4 *This, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI dxglDirectDrawSurface4_GetPalette(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface4_GetPixelFormat(dxglDirectDrawSurface4 *This, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI dxglDirectDrawSurface4_GetSurfaceDesc(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface4_Initialize(dxglDirectDrawSurface4 *This, LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
HRESULT WINAPI dxglDirectDrawSurface4_IsLost(dxglDirectDrawSurface4 *This);
HRESULT WINAPI dxglDirectDrawSurface4_Lock(dxglDirectDrawSurface4 *This, LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI dxglDirectDrawSurface4_ReleaseDC(dxglDirectDrawSurface4 *This, HDC hDC);
HRESULT WINAPI dxglDirectDrawSurface4_Restore(dxglDirectDrawSurface4 *This);
HRESULT WINAPI dxglDirectDrawSurface4_SetClipper(dxglDirectDrawSurface4 *This, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI dxglDirectDrawSurface4_SetColorKey(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI dxglDirectDrawSurface4_SetOverlayPosition(dxglDirectDrawSurface4 *This, LONG lX, LONG lY);
HRESULT WINAPI dxglDirectDrawSurface4_SetPalette(dxglDirectDrawSurface4 *This, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI dxglDirectDrawSurface4_Unlock(dxglDirectDrawSurface4 *This, LPRECT lpRect);
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlay(dxglDirectDrawSurface4 *This, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE4 lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlayDisplay(dxglDirectDrawSurface4 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_UpdateOverlayZOrder(dxglDirectDrawSurface4 *This, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDSReference);
// ddraw 2+ api
HRESULT WINAPI dxglDirectDrawSurface4_GetDDInterface(dxglDirectDrawSurface4 *This, LPVOID FAR *lplpDD);
HRESULT WINAPI dxglDirectDrawSurface4_PageLock(dxglDirectDrawSurface4 *This, DWORD dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_PageUnlock(dxglDirectDrawSurface4 *This, DWORD dwFlags);
// ddraw 3+ api
HRESULT WINAPI dxglDirectDrawSurface4_SetSurfaceDesc(dxglDirectDrawSurface4 *This, LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
// ddraw 4+ api
HRESULT WINAPI dxglDirectDrawSurface4_SetPrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
HRESULT WINAPI dxglDirectDrawSurface4_GetPrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
HRESULT WINAPI dxglDirectDrawSurface4_FreePrivateData(dxglDirectDrawSurface4 *This, REFGUID guidTag);
HRESULT WINAPI dxglDirectDrawSurface4_GetUniquenessValue(dxglDirectDrawSurface4 *This, LPDWORD lpValue);
HRESULT WINAPI dxglDirectDrawSurface4_ChangeUniquenessValue(dxglDirectDrawSurface4 *This);

#endif //_GLDIRECTDRAWSURFACE_H
