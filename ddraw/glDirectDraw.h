// DXGL
// Copyright (C) 2011-2023 William Feely

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

#ifdef __cplusplus
extern "C" {
#endif


#define DXGLBLT_NOPALSHADER 0x80000000

struct dxglDirectDrawSurface7;
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
struct glDirectDraw2;
struct glDirectDraw4;

struct glDirectDraw7Vtbl;
typedef struct glDirectDraw7
{
	glDirectDraw7Vtbl *lpVtbl;
	DWORD screenx, screeny, screenrefresh, screenbpp;
	DWORD internalx, internaly, internalrefresh, internalbpp;
	DWORD primaryx, primaryy, primaryrefresh, primarybpp;
	dxglDirectDrawSurface7 *primary;
	bool primarylost;
	bool lastsync;
	glDirectDraw1 *glDD1;
	glDirectDraw2 *glDD2;
	glDirectDraw4 *glDD4;
	DDDEVICEIDENTIFIER2 devid;
	LPDXGLRENDERER renderer;
	dxglDirectDrawSurface7 *tmpsurface;
	D3DDevice stored_devices[3];
	DEVMODE currmode;
	HRESULT error;
	ULONG refcount7, refcount4, refcount2, refcount1;
	HWND hWnd;
	bool fullscreen;
	bool fpupreserve;
	bool fpusetup;
	bool threadsafe;
	bool nowindowchanges;
	dxglDirectDrawSurface7 **surfaces;
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
void glDirectDraw7_RemoveSurface(glDirectDraw7 *This, dxglDirectDrawSurface7 *surface);
extern "C" void glDirectDraw7_GetSizes(glDirectDraw7 *This, LONG *sizes);
DWORD glDirectDraw7_GetBPPMultipleOf8(glDirectDraw7 *This);
void glDirectDraw7_DeleteSurface(glDirectDraw7 *This, dxglDirectDrawSurface7 *surface);
void glDirectDraw7_DeleteClipper(glDirectDraw7 *This, glDirectDrawClipper *clipper);
HRESULT glDirectDraw7_SetupTempSurface(glDirectDraw7 *This, DWORD width, DWORD height);
void glDirectDraw7_DeleteTempSurface(glDirectDraw7 *This);

void glDirectDraw7_UnrestoreDisplayMode(glDirectDraw7 *This);
void glDirectDraw7_SetWindowSize(glDirectDraw7 *glDD7, DWORD dwWidth, DWORD dwHeight);
BOOL glDirectDraw7_GetFullscreen(glDirectDraw7 *glDD7);
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

struct glDirectDraw2Vtbl;
typedef struct glDirectDraw2
{
	glDirectDraw2Vtbl *lpVtbl;
	glDirectDraw7 *glDD7;
} glDirectDraw2;
typedef struct glDirectDraw2Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDraw2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDraw2 *This);
	ULONG(WINAPI *Release)(glDirectDraw2 *This);
	HRESULT(WINAPI *Compact)(glDirectDraw2 *This);
	HRESULT(WINAPI *CreateClipper)(glDirectDraw2 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreatePalette)(glDirectDraw2 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreateSurface)(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *DuplicateSurface)(glDirectDraw2 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface);
	HRESULT(WINAPI *EnumDisplayModes)(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
	HRESULT(WINAPI *EnumSurfaces)(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
	HRESULT(WINAPI *FlipToGDISurface)(glDirectDraw2 *This);
	HRESULT(WINAPI *GetCaps)(glDirectDraw2 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT(WINAPI *GetDisplayMode)(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
	HRESULT(WINAPI *GetFourCCCodes)(glDirectDraw2 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT(WINAPI *GetGDISurface)(glDirectDraw2 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface);
	HRESULT(WINAPI *GetMonitorFrequency)(glDirectDraw2 *This, LPDWORD lpdwFrequency);
	HRESULT(WINAPI *GetScanLine)(glDirectDraw2 *This, LPDWORD lpdwScanLine);
	HRESULT(WINAPI *GetVerticalBlankStatus)(glDirectDraw2 *This, LPBOOL lpbIsInVB);
	HRESULT(WINAPI *Initialize)(glDirectDraw2 *This, GUID FAR *lpGUID);
	HRESULT(WINAPI *RestoreDisplayMode)(glDirectDraw2 *This);
	HRESULT(WINAPI *SetCooperativeLevel)(glDirectDraw2 *This, HWND hWnd, DWORD dwFlags);
	HRESULT(WINAPI *SetDisplayMode)(glDirectDraw2 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT(WINAPI *WaitForVerticalBlank)(glDirectDraw2 *This, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *GetAvailableVidMem)(glDirectDraw2 *This, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree);
} glDirectDraw2Vtbl;


HRESULT glDirectDraw2_Create(glDirectDraw7 *gl_DD7, glDirectDraw2 **glDD2);
// ddraw 1+ api
HRESULT WINAPI glDirectDraw2_QueryInterface(glDirectDraw2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDraw2_AddRef(glDirectDraw2 *This);
ULONG WINAPI glDirectDraw2_Release(glDirectDraw2 *This);
HRESULT WINAPI glDirectDraw2_Compact(glDirectDraw2 *This);
HRESULT WINAPI glDirectDraw2_CreateClipper(glDirectDraw2 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw2_CreatePalette(glDirectDraw2 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw2_CreateSurface(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw2_DuplicateSurface(glDirectDraw2 *This, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface);
HRESULT WINAPI glDirectDraw2_EnumDisplayModes(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
HRESULT WINAPI glDirectDraw2_EnumSurfaces(glDirectDraw2 *This, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI glDirectDraw2_FlipToGDISurface(glDirectDraw2 *This);
HRESULT WINAPI glDirectDraw2_GetCaps(glDirectDraw2 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
HRESULT WINAPI glDirectDraw2_GetDisplayMode(glDirectDraw2 *This, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI glDirectDraw2_GetFourCCCodes(glDirectDraw2 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
HRESULT WINAPI glDirectDraw2_GetGDISurface(glDirectDraw2 *This, LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface);
HRESULT WINAPI glDirectDraw2_GetMonitorFrequency(glDirectDraw2 *This, LPDWORD lpdwFrequency);
HRESULT WINAPI glDirectDraw2_GetScanLine(glDirectDraw2 *This, LPDWORD lpdwScanLine);
HRESULT WINAPI glDirectDraw2_GetVerticalBlankStatus(glDirectDraw2 *This, LPBOOL lpbIsInVB);
HRESULT WINAPI glDirectDraw2_Initialize(glDirectDraw2 *This, GUID FAR *lpGUID);
HRESULT WINAPI glDirectDraw2_RestoreDisplayMode(glDirectDraw2 *This);
HRESULT WINAPI glDirectDraw2_SetCooperativeLevel(glDirectDraw2 *This, HWND hWnd, DWORD dwFlags);
HRESULT WINAPI glDirectDraw2_SetDisplayMode(glDirectDraw2 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI glDirectDraw2_WaitForVerticalBlank(glDirectDraw2 *This, DWORD dwFlags, HANDLE hEvent);
// ddraw 2+ api
HRESULT WINAPI glDirectDraw2_GetAvailableVidMem(glDirectDraw2 *This, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree);

struct glDirectDraw4Vtbl;
typedef struct glDirectDraw4
{
	glDirectDraw4Vtbl *lpVtbl;
	glDirectDraw7 *glDD7;
} glDirectDraw4;
typedef struct glDirectDraw4Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDraw4 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDraw4 *This);
	ULONG(WINAPI *Release)(glDirectDraw4 *This);
	HRESULT(WINAPI *Compact)(glDirectDraw4 *This);
	HRESULT(WINAPI *CreateClipper)(glDirectDraw4 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreatePalette)(glDirectDraw4 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *CreateSurface)(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE4 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT(WINAPI *DuplicateSurface)(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface);
	HRESULT(WINAPI *EnumDisplayModes)(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
	HRESULT(WINAPI *EnumSurfaces)(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
	HRESULT(WINAPI *FlipToGDISurface)(glDirectDraw4 *This);
	HRESULT(WINAPI *GetCaps)(glDirectDraw4 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT(WINAPI *GetDisplayMode)(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2);
	HRESULT(WINAPI *GetFourCCCodes)(glDirectDraw4 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT(WINAPI *GetGDISurface)(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface);
	HRESULT(WINAPI *GetMonitorFrequency)(glDirectDraw4 *This, LPDWORD lpdwFrequency);
	HRESULT(WINAPI *GetScanLine)(glDirectDraw4 *This, LPDWORD lpdwScanLine);
	HRESULT(WINAPI *GetVerticalBlankStatus)(glDirectDraw4 *This, LPBOOL lpbIsInVB);
	HRESULT(WINAPI *Initialize)(glDirectDraw4 *This, GUID FAR *lpGUID);
	HRESULT(WINAPI *RestoreDisplayMode)(glDirectDraw4 *This);
	HRESULT(WINAPI *SetCooperativeLevel)(glDirectDraw4 *This, HWND hWnd, DWORD dwFlags);
	HRESULT(WINAPI *SetDisplayMode)(glDirectDraw4 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
	HRESULT(WINAPI *WaitForVerticalBlank)(glDirectDraw4 *This, DWORD dwFlags, HANDLE hEvent);
	HRESULT(WINAPI *GetAvailableVidMem)(glDirectDraw4 *This, LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
	HRESULT(WINAPI *GetSurfaceFromDC)(glDirectDraw4 *This, HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS);
	HRESULT(WINAPI *RestoreAllSurfaces)(glDirectDraw4 *This);
	HRESULT(WINAPI *TestCooperativeLevel)(glDirectDraw4 *This);
	HRESULT(WINAPI *GetDeviceIdentifier)(glDirectDraw4 *This, LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags);
} glDirectDraw4Vtbl;

HRESULT glDirectDraw4_Create(glDirectDraw7 *gl_DD7, glDirectDraw4 **glDD4);
// ddraw 1+ api
HRESULT WINAPI glDirectDraw4_QueryInterface(glDirectDraw4 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDraw4_AddRef(glDirectDraw4 *This);
ULONG WINAPI glDirectDraw4_Release(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_Compact(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_CreateClipper(glDirectDraw4 *This, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw4_CreatePalette(glDirectDraw4 *This, DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw4_CreateSurface(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE4 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
HRESULT WINAPI glDirectDraw4_DuplicateSurface(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface);
HRESULT WINAPI glDirectDraw4_EnumDisplayModes(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
HRESULT WINAPI glDirectDraw4_EnumSurfaces(glDirectDraw4 *This, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback);
HRESULT WINAPI glDirectDraw4_FlipToGDISurface(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_GetCaps(glDirectDraw4 *This, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
HRESULT WINAPI glDirectDraw4_GetDisplayMode(glDirectDraw4 *This, LPDDSURFACEDESC2 lpDDSurfaceDesc2);
HRESULT WINAPI glDirectDraw4_GetFourCCCodes(glDirectDraw4 *This, LPDWORD lpNumCodes, LPDWORD lpCodes);
HRESULT WINAPI glDirectDraw4_GetGDISurface(glDirectDraw4 *This, LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface);
HRESULT WINAPI glDirectDraw4_GetMonitorFrequency(glDirectDraw4 *This, LPDWORD lpdwFrequency);
HRESULT WINAPI glDirectDraw4_GetScanLine(glDirectDraw4 *This, LPDWORD lpdwScanLine);
HRESULT WINAPI glDirectDraw4_GetVerticalBlankStatus(glDirectDraw4 *This, LPBOOL lpbIsInVB);
HRESULT WINAPI glDirectDraw4_Initialize(glDirectDraw4 *This, GUID FAR *lpGUID);
HRESULT WINAPI glDirectDraw4_RestoreDisplayMode(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_SetCooperativeLevel(glDirectDraw4 *This, HWND hWnd, DWORD dwFlags);
HRESULT WINAPI glDirectDraw4_SetDisplayMode(glDirectDraw4 *This, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags);
HRESULT WINAPI glDirectDraw4_WaitForVerticalBlank(glDirectDraw4 *This, DWORD dwFlags, HANDLE hEvent);
// ddraw 2+ api
HRESULT WINAPI glDirectDraw4_GetAvailableVidMem(glDirectDraw4 *This, LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree);
// ddraw 4+ api
HRESULT WINAPI glDirectDraw4_GetSurfaceFromDC(glDirectDraw4 *This, HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS);
HRESULT WINAPI glDirectDraw4_RestoreAllSurfaces(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_TestCooperativeLevel(glDirectDraw4 *This);
HRESULT WINAPI glDirectDraw4_GetDeviceIdentifier(glDirectDraw4 *This, LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags);


HRESULT WINAPI EnumSurfacesCallback1(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext);
HRESULT WINAPI EnumSurfacesCallback2(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext);

#ifdef __cplusplus
}
#endif

#endif //_GLDIRECTDRAW_H
