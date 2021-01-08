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

#pragma once
#ifndef _GLDIRECTDRAW_H
#define _GLDIRECTDRAW_H

#define DXGLBLT_NOPALSHADER 0x80000000

class glDirectDrawSurface7;
struct glDirectDrawClipper;
struct glDirect3D7;
struct glDirect3D3;
struct glDirect3D2;
struct glDirect3D1;
struct glRenderer;

struct D3DDevice
{
	char name[64];
	char devname[64];
};

struct glDirectDraw1;
class glDirectDraw2;
class glDirectDraw4;

struct glDirectDraw7Vtbl;
typedef struct glDirectDraw7
{
	glDirectDraw7Vtbl *lpVtbl;
	DWORD screenx, screeny, screenrefresh, screenbpp;
	DWORD internalx, internaly, internalrefresh, internalbpp;
	DWORD primaryx, primaryy, primaryrefresh, primarybpp;
	glDirectDrawSurface7 *primary;
	bool primarylost;
	bool lastsync;
	glDirectDraw1 *glDD1;
	glDirectDraw2 *glDD2;
	glDirectDraw4 *glDD4;
	DDDEVICEIDENTIFIER2 devid;
	glRenderer *renderer;
	glDirectDrawSurface7 *tmpsurface;
	D3DDevice stored_devices[3];
	D3DDEVICEDESC7 d3ddesc;
	D3DDEVICEDESC d3ddesc3;
	DEVMODE currmode;
	HRESULT error;
	ULONG refcount7, refcount4, refcount2, refcount1;
	HWND hWnd;
	bool fullscreen;
	bool fpupreserve;
	bool fpusetup;
	bool threadsafe;
	bool nowindowchanges;
	LONG_PTR winstyle, winstyleex;
	glDirectDrawSurface7 **surfaces;
	int surfacecount, surfacecountmax;
	glDirectDrawClipper **clippers;
	int clippercount, clippercountmax;
	DEVMODE oldmode;
	bool initialized;
	glDirect3D7 *glD3D7;
	glDirect3D3 *glD3D3;
	glDirect3D2 *glD3D2;
	glDirect3D1 *glD3D1;
	DWORD timer;
	bool devwnd;
	DWORD cooplevel;
} glDirectDraw7;

typedef struct glDirectDraw7Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDraw7 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDraw7 *This);
	ULONG(WINAPI *Release)(glDirectDraw7 *This);
	HRESULT(WINAPI *Compact)(glDirectDraw7 *This);
	HRESULT(WINAPI *CreateClipper)(glDirectDraw7 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreatePalette)(glDirectDraw7 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreateSurface)(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *DuplicateSurface)(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR *lplpDupDDSurface);
	HRESULT(WINAPI *EnumDisplayModes)(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
	HRESULT(WINAPI *EnumSurfaces)(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT(WINAPI *FlipToGDISurface)(glDirectDraw7 *This);
	HRESULT(WINAPI *GetCaps)(glDirectDraw7 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT(WINAPI *GetDisplayMode)(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2);
	HRESULT(WINAPI *GetFourCCCodes)(glDirectDraw7 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT(WINAPI *GetGDISurface)(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface);
	HRESULT(WINAPI *GetMonitorFrequency)(glDirectDraw7 *This, LPDWORD lpdwFrequency);
	HRESULT(WINAPI *GetScanLine)(glDirectDraw7 *This, LPDWORD lpdwScanLine);
	HRESULT(WINAPI *GetVerticalBlankStatus)(glDirectDraw7 *This, LPBOOL lpbIsInVB);
	HRESULT(WINAPI *Initialize)(glDirectDraw7 *This, GUID FAR *lpGUID);
	HRESULT(WINAPI *RestoreDisplayMode)(glDirectDraw7 *This);
	HRESULT(WINAPI *SetCooperativeLevel)(glDirectDraw7 *This, HWND hWnd, DWORD dwFlags);
	HRESULT(WINAPI *SetDisplayMode)(glDirectDraw7 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT(WINAPI *WaitForVerticalBlank)(glDirectDraw7 *This, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *GetAvailableVidMem)(glDirectDraw7 *This, LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
	HRESULT(WINAPI *GetSurfaceFromDC)(glDirectDraw7 *This, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS);
	HRESULT(WINAPI *RestoreAllSurfaces)(glDirectDraw7 *This);
	HRESULT(WINAPI *TestCooperativeLevel)(glDirectDraw7 *This);
	HRESULT(WINAPI *GetDeviceIdentifier)(glDirectDraw7 *This, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags);
	HRESULT(WINAPI *StartModeTest)(glDirectDraw7 *This, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags);
	HRESULT(WINAPI *EvaluateMode)(glDirectDraw7 *This, DWORD dwFlags, DWORD *pSecondsUntilTimeout);
} glDirectDraw7Vtbl;


HRESULT glDirectDraw7_Create(glDirectDraw7 **glDD7);
HRESULT glDirectDraw7_CreateAndInitialize(GUID FAR* lpGUID, IUnknown FAR* pUnkOuter, glDirectDraw7 **glDD7);
void glDirectDraw7_Delete(glDirectDraw7 *This);

// ddraw 1+ api
HRESULT WINAPI glDirectDraw7_QueryInterface(glDirectDraw7 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDraw7_AddRef(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_Release(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_Compact(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_CreateClipper(glDirectDraw7 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw7_CreatePalette(glDirectDraw7 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw7_CreateSurface(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw7_DuplicateSurface(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR *lplpDupDDSurface);
HRESULT WINAPI glDirectDraw7_EnumDisplayModes(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
HRESULT WINAPI glDirectDraw7_EnumSurfaces(glDirectDraw7 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
HRESULT WINAPI glDirectDraw7_FlipToGDISurface(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_GetCaps(glDirectDraw7 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
HRESULT WINAPI glDirectDraw7_GetDisplayMode(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2);
HRESULT WINAPI glDirectDraw7_GetFourCCCodes(glDirectDraw7 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
HRESULT WINAPI glDirectDraw7_GetGDISurface(glDirectDraw7 *This, LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface);
HRESULT WINAPI glDirectDraw7_GetMonitorFrequency(glDirectDraw7 *This, LPDWORD lpdwFrequency);
HRESULT WINAPI glDirectDraw7_GetScanLine(glDirectDraw7 *This, LPDWORD lpdwScanLine);
HRESULT WINAPI glDirectDraw7_GetVerticalBlankStatus(glDirectDraw7 *This, LPBOOL lpbIsInVB);
HRESULT WINAPI glDirectDraw7_Initialize(glDirectDraw7 *This, GUID FAR *lpGUID);
HRESULT WINAPI glDirectDraw7_RestoreDisplayMode(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_SetCooperativeLevel(glDirectDraw7 *This, HWND hWnd, DWORD dwFlags);
HRESULT WINAPI glDirectDraw7_SetDisplayMode(glDirectDraw7 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI glDirectDraw7_WaitForVerticalBlank(glDirectDraw7 *This, DWORD dwFlags, HANDLE hEvent);
// ddraw 2+ api
HRESULT WINAPI glDirectDraw7_GetAvailableVidMem(glDirectDraw7 *This, LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
// ddraw 4+ api
HRESULT WINAPI glDirectDraw7_GetSurfaceFromDC(glDirectDraw7 *This, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS);
HRESULT WINAPI glDirectDraw7_RestoreAllSurfaces(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_TestCooperativeLevel(glDirectDraw7 *This);
HRESULT WINAPI glDirectDraw7_GetDeviceIdentifier(glDirectDraw7 *This, LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags);
// ddraw 7 api
HRESULT WINAPI glDirectDraw7_StartModeTest(glDirectDraw7 *This, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags);
HRESULT WINAPI glDirectDraw7_EvaluateMode(glDirectDraw7 *This, DWORD dwFlags, DWORD *pSecondsUntilTimeout);

// internal functions
ULONG WINAPI glDirectDraw7_AddRef4(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_Release4(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_AddRef2(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_Release2(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_AddRef1(glDirectDraw7 *This);
ULONG WINAPI glDirectDraw7_Release1(glDirectDraw7 *This);
HRESULT glDirectDraw7_CreateSurface2(glDirectDraw7 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter, BOOL RecordSurface, int version);
HRESULT glDirectDraw7_CreateClipper2(glDirectDraw7 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT glDirectDraw7_CreatePalette2(glDirectDraw7 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
void glDirectDraw7_RemoveSurface(glDirectDraw7 *This, glDirectDrawSurface7 *surface);
extern "C" void glDirectDraw7_GetSizes(glDirectDraw7 *This, LONG *sizes);
DWORD glDirectDraw7_GetBPPMultipleOf8(glDirectDraw7 *This);
void glDirectDraw7_DeleteSurface(glDirectDraw7 *This, glDirectDrawSurface7 *surface);
void glDirectDraw7_DeleteClipper(glDirectDraw7 *This, glDirectDrawClipper *clipper);
HRESULT glDirectDraw7_SetupTempSurface(glDirectDraw7 *This, DWORD width, DWORD height);
void glDirectDraw7_DeleteTempSurface(glDirectDraw7 *This);

extern "C" void glDirectDraw7_UnrestoreDisplayMode(glDirectDraw7 *This);
extern "C" void glDirectDraw7_SetWindowSize(glDirectDraw7 *glDD7, DWORD dwWidth, DWORD dwHeight);
extern "C" BOOL glDirectDraw7_GetFullscreen(glDirectDraw7 *glDD7);
LONG Try640400Mode(LPCTSTR devname, DEVMODE *mode, DWORD flags, BOOL *crop400);

struct glDirectDraw1Vtbl;
typedef struct glDirectDraw1
{
	glDirectDraw1Vtbl *lpVtbl;
	glDirectDraw7 *glDD7;
} glDirectDraw1;
typedef struct glDirectDraw1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDraw1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDraw1 *This);
	ULONG(WINAPI *Release)(glDirectDraw1 *This);
	HRESULT(WINAPI *Compact)(glDirectDraw1 *This);
	HRESULT(WINAPI *CreateClipper)(glDirectDraw1 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreatePalette)(glDirectDraw1 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreateSurface)(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *DuplicateSurface)(glDirectDraw1 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface);
	HRESULT(WINAPI *EnumDisplayModes)(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
	HRESULT(WINAPI *EnumSurfaces)(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT(WINAPI *FlipToGDISurface)(glDirectDraw1 *This);
	HRESULT(WINAPI *GetCaps)(glDirectDraw1 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT(WINAPI *GetDisplayMode)(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *GetFourCCCodes)(glDirectDraw1 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT(WINAPI *GetGDISurface)(glDirectDraw1 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface);
	HRESULT(WINAPI *GetMonitorFrequency)(glDirectDraw1 *This, LPDWORD lpdwFrequency);
	HRESULT(WINAPI *GetScanLine)(glDirectDraw1 *This, LPDWORD lpdwScanLine);
	HRESULT(WINAPI *GetVerticalBlankStatus)(glDirectDraw1 *This, LPBOOL lpbIsInVB);
	HRESULT(WINAPI *Initialize)(glDirectDraw1 *This, GUID FAR *lpGUID);
	HRESULT(WINAPI *RestoreDisplayMode)(glDirectDraw1 *This);
	HRESULT(WINAPI *SetCooperativeLevel)(glDirectDraw1 *This, HWND hWnd, DWORD dwFlags);
	HRESULT(WINAPI *SetDisplayMode)(glDirectDraw1 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
	HRESULT(WINAPI *WaitForVerticalBlank)(glDirectDraw1 *This, DWORD dwFlags, HANDLE hEvent);
} glDirectDraw1Vtbl;

HRESULT glDirectDraw1_Create(glDirectDraw7 *gl_DD7, glDirectDraw1 **glDD1);
// ddraw 1+ api
HRESULT WINAPI glDirectDraw1_QueryInterface(glDirectDraw1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDraw1_AddRef(glDirectDraw1 *This);
ULONG WINAPI glDirectDraw1_Release(glDirectDraw1 *This);
HRESULT WINAPI glDirectDraw1_Compact(glDirectDraw1 *This);
HRESULT WINAPI glDirectDraw1_CreateClipper(glDirectDraw1 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw1_CreatePalette(glDirectDraw1 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw1_CreateSurface(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw1_DuplicateSurface(glDirectDraw1 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface);
HRESULT WINAPI glDirectDraw1_EnumDisplayModes(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
HRESULT WINAPI glDirectDraw1_EnumSurfaces(glDirectDraw1 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI glDirectDraw1_FlipToGDISurface(glDirectDraw1 *This);
HRESULT WINAPI glDirectDraw1_GetCaps(glDirectDraw1 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
HRESULT WINAPI glDirectDraw1_GetDisplayMode(glDirectDraw1 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI glDirectDraw1_GetFourCCCodes(glDirectDraw1 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
HRESULT WINAPI glDirectDraw1_GetGDISurface(glDirectDraw1 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface);
HRESULT WINAPI glDirectDraw1_GetMonitorFrequency(glDirectDraw1 *This, LPDWORD lpdwFrequency);
HRESULT WINAPI glDirectDraw1_GetScanLine(glDirectDraw1 *This, LPDWORD lpdwScanLine);
HRESULT WINAPI glDirectDraw1_GetVerticalBlankStatus(glDirectDraw1 *This, LPBOOL lpbIsInVB);
HRESULT WINAPI glDirectDraw1_Initialize(glDirectDraw1 *This, GUID FAR *lpGUID);
HRESULT WINAPI glDirectDraw1_RestoreDisplayMode(glDirectDraw1 *This);
HRESULT WINAPI glDirectDraw1_SetCooperativeLevel(glDirectDraw1 *This, HWND hWnd, DWORD dwFlags);
HRESULT WINAPI glDirectDraw1_SetDisplayMode(glDirectDraw1 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
HRESULT WINAPI glDirectDraw1_WaitForVerticalBlank(glDirectDraw1 *This, DWORD dwFlags, HANDLE hEvent);

class glDirectDraw2 : public IDirectDraw2
{
public:
	glDirectDraw2(glDirectDraw7 *gl_DD7);
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI Compact();
	HRESULT WINAPI CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface);
	HRESULT WINAPI EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
	HRESULT WINAPI EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT WINAPI FlipToGDISurface();
	HRESULT WINAPI GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT WINAPI GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT WINAPI GetGDISurface(LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface);
	HRESULT WINAPI GetMonitorFrequency(LPDWORD lpdwFrequency);
	HRESULT WINAPI GetScanLine(LPDWORD lpdwScanLine);
	HRESULT WINAPI GetVerticalBlankStatus(LPBOOL lpbIsInVB);
	HRESULT WINAPI Initialize(GUID FAR *lpGUID);
	HRESULT WINAPI RestoreDisplayMode();
	HRESULT WINAPI SetCooperativeLevel(HWND hWnd, DWORD dwFlags);
	HRESULT WINAPI SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT WINAPI WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent);
	// ddraw 2+ api
	HRESULT WINAPI GetAvailableVidMem(LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree);

	HRESULT err() {return glDD7->error;}
private:
	glDirectDraw7 *glDD7;
};
class glDirectDraw4 : public IDirectDraw4
{
public:
	glDirectDraw4(glDirectDraw7 *gl_DD7);
	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI Compact();
	HRESULT WINAPI CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE4 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI DuplicateSurface(LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface);
	HRESULT WINAPI EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
	HRESULT WINAPI EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
	HRESULT WINAPI FlipToGDISurface();
	HRESULT WINAPI GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2);
	HRESULT WINAPI GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT WINAPI GetGDISurface(LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface);
	HRESULT WINAPI GetMonitorFrequency(LPDWORD lpdwFrequency);
	HRESULT WINAPI GetScanLine(LPDWORD lpdwScanLine);
	HRESULT WINAPI GetVerticalBlankStatus(LPBOOL lpbIsInVB);
	HRESULT WINAPI Initialize(GUID FAR *lpGUID);
	HRESULT WINAPI RestoreDisplayMode();
	HRESULT WINAPI SetCooperativeLevel(HWND hWnd, DWORD dwFlags);
	HRESULT WINAPI SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT WINAPI WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent);
	// ddraw 2+ api
	HRESULT WINAPI GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
	// ddraw 4+ api
	HRESULT WINAPI GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS);
	HRESULT WINAPI RestoreAllSurfaces();
	HRESULT WINAPI TestCooperativeLevel();
	HRESULT WINAPI GetDeviceIdentifier(LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags);

	HRESULT err() {return glDD7->error;}
private:
	glDirectDraw7 *glDD7;
};

HRESULT WINAPI EnumSurfacesCallback1(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext);
HRESULT WINAPI EnumSurfacesCallback2(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext);

#endif //_GLDIRECTDRAW_H
