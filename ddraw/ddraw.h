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
#ifndef _DDRAW_PRIVATE_H
#define _DDRAW_PRIVATE_H

#ifdef DDRAW_EXPORTS
#define DDRAW_API __declspec(dllexport)
#else
#define DDRAW_API __declspec(dllimport)
#endif


DDRAW_API void WINAPI AcquireDDThreadLock(); // stub
DDRAW_API void WINAPI CompleteCreateSystemSurface(); // stub
DDRAW_API void WINAPI D3DParseUnknownCommand(); // stub
DDRAW_API void WINAPI DDGetAttachedSurfaceLcl(); // stub
DDRAW_API void WINAPI DDInternalLock(); // stub
DDRAW_API void WINAPI DDInternalUnlock(); // stub
DDRAW_API void WINAPI DSoundHelp(); // stub
#undef DirectDrawCreate
#undef DirectDrawCreateClipper
#undef DirectDrawCreateEx
#undef DirectDrawEnumerateA
#undef DirectDrawEnumerateW
#undef DirectDrawEnumerateExA
#undef DirectDrawEnumerateExW
HRESULT WINAPI DirectDrawCreate(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter);
HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter);
HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter);
HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext);
HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext);
HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);
HRESULT WINAPI DllCanUnloadNow();
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
DDRAW_API void WINAPI GetDDSurfaceLocal();
DDRAW_API HANDLE WINAPI GetOLEThunkData(int i1);
DDRAW_API HRESULT WINAPI GlobalGetSurfaceFromDC(LPDIRECTDRAW lpDD, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS);
DDRAW_API void WINAPI RegisterSpecialCase();
DDRAW_API void WINAPI ReleaseDDThreadLock();
DDRAW_API BOOL IsDXGLDDraw();

void DeleteDirectDraw();
#ifdef __cplusplus
extern "C" {
#endif
extern DXGLCFG dxglcfg;
extern DWORD gllock;
#ifdef __cplusplus
}
#endif
extern const GUID device_template;
struct glRenderer;
struct glDirectDraw7;
void InitGL(int width, int height, int bpp, BOOL fullscreen, unsigned int frequency, HWND hWnd, struct glDirectDraw7 *glDD7, BOOL devwnd);

#endif //_DDRAW_PRIVATE_H
