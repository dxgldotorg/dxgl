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
#ifndef _GLDIRECTDRAW_H
#define _GLDIRECTDRAW_H

class glDirectDrawSurface7;
class glDirectDrawClipper;

LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef struct
{
	float Version;
	float ShaderVer;
	int TextureMaxX;
	int TextureMaxY;
	bool NonPowerOfTwo;
	int RenderToTexture; // 0 - no support, 1 -
	int MultiTextureExt;
	int PalettedTextures;
} GLCAPS;

class glDirectDraw7 : public IDirectDraw7
{
public:
	glDirectDraw7(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter);
	virtual ~glDirectDraw7();

	// ddraw 1+ api
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI Compact();
	HRESULT WINAPI CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter);
	HRESULT WINAPI DuplicateSurface(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR *lplpDupDDSurface);
	HRESULT WINAPI EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback);
	HRESULT WINAPI EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback);
	HRESULT WINAPI FlipToGDISurface();
	HRESULT WINAPI GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
	HRESULT WINAPI GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2);
	HRESULT WINAPI GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes);
	HRESULT WINAPI GetGDISurface(LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface);
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
	HRESULT WINAPI GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS);
	HRESULT WINAPI RestoreAllSurfaces();
	HRESULT WINAPI TestCooperativeLevel();
	HRESULT WINAPI GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags);
	// ddraw 7 api
	HRESULT WINAPI StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags);
    HRESULT WINAPI EvaluateMode(DWORD dwFlags, DWORD *pSecondsUntilTimeout);

    // internal functions
	HRESULT err() {return error;}
	void RemoveSurface(glDirectDrawSurface7 *surface);
	void GetSizes(LONG *sizes);
	GLuint PalProg(){return palprog;}
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	DWORD GetBPP(){return primarybpp;}
	DWORD GetBPPMultipleOf8(){if(primarybpp == 15) return 16; else return primarybpp;}
	DWORD GetBPPPowerOf2(){if(primarybpp == 15) return 16; if(primarybpp == 24) return 32; else return primarybpp;}
	HGLRC hRC;
	HDC  hDC;
	DWORD screenx,screeny,screenrefresh,screenbpp;
	DWORD internalx,internaly,internalrefresh,internalbpp;
	DWORD primaryx,primaryy,primaryrefresh,primarybpp;
	bool GetFullscreen(){return fullscreen;};
	GLuint fbo;
	GLuint pbo;
	GLuint depthbuffer;
	void GetHandles(HWND *hwnd, HWND *hrender);
private:
	void DeleteGL();
	BOOL InitGL(int width, int height, int bpp, bool fullscreen, HWND hWnd);
	HRESULT error;
	ULONG refcount;
	HWND hWnd;
	HWND hRenderWnd;
	bool fullscreen;
	bool fpupreserve;
	bool fpusetup;
	bool threadsafe;
	bool nowindowchanges;
	LONG_PTR winstyle,winstyleex;
	glDirectDrawSurface7 **surfaces;
	glDirectDrawSurface7 *primary;
	int surfacecount, surfacecountmax;
	glDirectDrawClipper **clippers;
	int clippercount, clippercountmax;
	GLuint palprog;
	GLCAPS gl_caps;
};

class glDirectDraw1 : public IDirectDraw
{
public:
	glDirectDraw1(glDirectDraw7 *gl_DD7);
	virtual ~glDirectDraw1();
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
	HRESULT WINAPI SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
	HRESULT WINAPI WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent);
	HRESULT err() {return glDD7->err();}
private:
	ULONG refcount;
	glDirectDraw7 *glDD7;
};

class glDirectDraw2 : public IDirectDraw2
{
public:
	glDirectDraw2(glDirectDraw7 *gl_DD7);
	virtual ~glDirectDraw2();
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

	HRESULT err() {return glDD7->err();}
private:
	ULONG refcount;
	glDirectDraw7 *glDD7;
};
class glDirectDraw4 : public IDirectDraw4
{
public:
	glDirectDraw4(glDirectDraw7 *gl_DD7);
	virtual ~glDirectDraw4();
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

	HRESULT err() {return glDD7->err();}
private:
	ULONG refcount;
	glDirectDraw7 *glDD7;
};
#endif //_GLDIRECTDRAW_H
