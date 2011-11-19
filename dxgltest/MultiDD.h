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

#pragma once
#ifndef _MULTIDD_H
#define _MULTIDD_H

class MultiDirectDrawSurface;

class MultiDirectDraw
{
public:
	MultiDirectDraw(int version, HRESULT *error, GUID *lpGUID);
	virtual ~MultiDirectDraw();
	HRESULT QueryInterface(REFIID riid, void** ppvObj);
	ULONG AddRef();
	ULONG Release();
	HRESULT Compact();
	HRESULT CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, MultiDirectDrawSurface FAR **lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT DuplicateSurface(MultiDirectDrawSurface *lpDDSurface, MultiDirectDrawSurface FAR **lplpDupDDSurface);
	HRESULT EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
	HRESULT EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT FlipToGDISurface();
	HRESULT GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2);
	HRESULT GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT GetGDISurface(MultiDirectDrawSurface FAR **lplpGDIDDSurface);
	HRESULT GetMonitorFrequency(LPDWORD lpdwFrequency);
	HRESULT GetScanLine(LPDWORD lpdwScanLine);
	HRESULT GetVerticalBlankStatus(LPBOOL lpbIsInVB);
	HRESULT Initialize(GUID FAR *lpGUID);
	HRESULT RestoreDisplayMode();
	HRESULT SetCooperativeLevel(HWND hWnd, DWORD dwFlags);
	HRESULT SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent);
	// ddraw 2+ api
	HRESULT WINAPI GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
	// ddraw 4+ api
	HRESULT WINAPI GetSurfaceFromDC(HDC hdc, MultiDirectDrawSurface **lpDDS);
	HRESULT WINAPI RestoreAllSurfaces();
	HRESULT WINAPI TestCooperativeLevel();
	HRESULT WINAPI GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags);
	// ddraw 7 api
	HRESULT WINAPI StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags);
    HRESULT WINAPI EvaluateMode(DWORD dwFlags, DWORD *pSecondsUntilTimeout);
private:
	ULONG refcount;
	int version;
	LPDIRECTDRAW dd1;
	LPDIRECTDRAW2 dd2;
	LPDIRECTDRAW4 dd4;
	LPDIRECTDRAW7 dd7;
	// temporary storage
	LPDIRECTDRAWSURFACE dds1;
	LPDIRECTDRAWSURFACE2 dds2;
	LPDIRECTDRAWSURFACE3 dds3;
	LPDIRECTDRAWSURFACE4 dds4;
	LPDIRECTDRAWSURFACE7 dds7;
	MultiDirectDrawSurface *ddsm;
};

class MultiDirectDrawSurface
{
public:
	MultiDirectDrawSurface(int version, LPVOID surface);
	virtual ~MultiDirectDrawSurface();
	// ddraw 1+ api
	HRESULT QueryInterface(REFIID riid, void** ppvObj);
	ULONG AddRef();
	ULONG Release();
	HRESULT AddAttachedSurface(MultiDirectDrawSurface *lpDDSAttachedSurface);
	HRESULT AddOverlayDirtyRect(LPRECT lpRect);
	HRESULT Blt(LPRECT lpDestRect, MultiDirectDrawSurface *lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	HRESULT BltBatch(LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
	HRESULT BltFast(DWORD dwX, DWORD dwY, MultiDirectDrawSurface *lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
	HRESULT DeleteAttachedSurface(DWORD dwFlags, MultiDirectDrawSurface *lpDDSAttachedSurface);
	HRESULT EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpfnCallback);
	HRESULT Flip(MultiDirectDrawSurface *lpDDSurfaceTargetOverride, DWORD dwFlags);
	HRESULT GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, MultiDirectDrawSurface FAR **lplpDDAttachedSurface);
	HRESULT GetBltStatus(DWORD dwFlags);
	HRESULT GetCaps(LPDDSCAPS2 lpDDSCaps);
	HRESULT GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper);
	HRESULT GetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT GetDC(HDC FAR *lphDC);
	HRESULT GetFlipStatus(DWORD dwFlags);
	HRESULT GetOverlayPosition(LPLONG lplX, LPLONG lplY);
	HRESULT GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	HRESULT GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat);
	HRESULT GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT Initialize(MultiDirectDraw *lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc);
	HRESULT IsLost();
	HRESULT Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
	HRESULT ReleaseDC(HDC hDC);
	HRESULT Restore();
	HRESULT SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper);
	HRESULT SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
	HRESULT SetOverlayPosition(LONG lX, LONG lY);
	HRESULT SetPalette(LPDIRECTDRAWPALETTE lpDDPalette);
	HRESULT Unlock(LPRECT lpRect);
	HRESULT UpdateOverlay(LPRECT lpSrcRect, MultiDirectDrawSurface *lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
	HRESULT UpdateOverlayDisplay(DWORD dwFlags);
	HRESULT UpdateOverlayZOrder(DWORD dwFlags, MultiDirectDrawSurface *lpDDSReference);
	// ddraw 2+ api
	HRESULT GetDDInterface(LPVOID FAR *lplpDD);
	HRESULT PageLock(DWORD dwFlags);
	HRESULT PageUnlock(DWORD dwFlags);
	// ddraw 3+ api
	HRESULT SetSurfaceDesc(LPDDSURFACEDESC2 lpddsd2, DWORD dwFlags);
	// ddraw 4+ api
	HRESULT SetPrivateData(REFGUID guidTag, LPVOID  lpData, DWORD   cbSize, DWORD   dwFlags);
	HRESULT GetPrivateData(REFGUID guidTag, LPVOID  lpBuffer, LPDWORD lpcbBufferSize);
	HRESULT FreePrivateData(REFGUID guidTag);
	HRESULT GetUniquenessValue(LPDWORD lpValue);
	HRESULT ChangeUniquenessValue();
	// ddraw 7 api
	HRESULT SetPriority(DWORD dwPriority);
	HRESULT GetPriority(LPDWORD lpdwPriority);
	HRESULT SetLOD(DWORD dwMaxLOD);
	HRESULT GetLOD(LPDWORD lpdwMaxLOD);
	// Internal functions
	LPVOID GetSurface();
private:
	int version;
	ULONG refcount;
	LPDIRECTDRAWSURFACE dds1;
	LPDIRECTDRAWSURFACE2 dds2;
	LPDIRECTDRAWSURFACE3 dds3;
	LPDIRECTDRAWSURFACE4 dds4;
	LPDIRECTDRAWSURFACE7 dds7;
	// temporary surface
	MultiDirectDrawSurface *ddsm;
};

#endif //_MULTIDD_H