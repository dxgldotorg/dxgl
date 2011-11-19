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

#include "common.h"
#include "ddraw.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"

DXGLCFG dxglcfg;
bool gllock = false;
HMODULE sysddraw = NULL;
HRESULT (WINAPI *sysddrawcreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter) = NULL;

DDRAW_API void WINAPI AcquireDDThreadLock()
{
	// FIXME:  Add thread lock
	FIXME("AcquireDDThreadLock: stub\n");
}
DDRAW_API void WINAPI CompleteCreateSystemSurface()
{
	FIXME("CompleteCreateSystemSurface: stub\n");
}
DDRAW_API void WINAPI D3DParseUnknownCommand()
{
	FIXME("D3DParseUnknownCommand: stub\n");
}
DDRAW_API void WINAPI DDGetAttachedSurfaceLcl()
{
	FIXME("DDGetAttachedSurfaceLcl: stub\n");
}
DDRAW_API void WINAPI DDInternalLock()
{
	//FIXME:  Add locking code
	FIXME("DDInternalLock: stub\n");
}
DDRAW_API void WINAPI DDInternalUnlock()
{
	//FIXME:  Add unlocking code
	FIXME("DDInternalUnlock: stub\n");
}
DDRAW_API void WINAPI DSoundHelp()
{
	FIXME("DSoundHelp: stub\n");
}
HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
	GetCurrentConfig(&dxglcfg);
	if(gllock)
	{
		if(!sysddraw)
		{
			char buffer[MAX_PATH];
			GetSystemDirectoryA(buffer,MAX_PATH);
			strcat(buffer,"\\ddraw.dll");
			sysddraw = LoadLibraryA(buffer);
			if(!sysddraw) return DDERR_GENERIC;
		}
		if(!sysddrawcreate)
		{
			sysddrawcreate = (HRESULT(WINAPI *)(GUID FAR*,LPDIRECTDRAW FAR*, IUnknown FAR*))GetProcAddress(sysddraw,"DirectDrawCreate");
			if(!sysddrawcreate) return DDERR_GENERIC;
		}
		return sysddrawcreate(lpGUID,lplpDD,pUnkOuter);
	}
	glDirectDraw7 *myddraw7;
	glDirectDraw1 *myddraw;
	HRESULT error;
	myddraw7 = new glDirectDraw7(lpGUID,lplpDD,pUnkOuter);
	error = myddraw7->err();
	if(error != DD_OK)
	{
		delete myddraw7;
		return error;
	}
	myddraw7->QueryInterface(IID_IDirectDraw,(VOID**)&myddraw);
	myddraw7->Release();
	*lplpDD = (LPDIRECTDRAW)myddraw;
	return error;
}
HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	*lplpDDClipper = new glDirectDrawClipper(dwFlags,lplpDDClipper,pUnkOuter,NULL);
	return DD_OK;
}
HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter)
{
	glDirectDraw7 *myddraw;
	HRESULT error;
	if(iid != IID_IDirectDraw7) return DDERR_UNSUPPORTED;
	myddraw = new glDirectDraw7(lpGUID,(LPDIRECTDRAW FAR *)lplpDD,pUnkOuter);
	error = myddraw->err();
	if(error != DD_OK)
	{
		delete myddraw;
		return error;
	}
	*lplpDD = (LPDIRECTDRAW7)myddraw;
	return error;
}
HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
	FIXME("DirectDrawEnumerateA: stub\n");
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext)
{
	FIXME("DirectDrawEnumerateW: stub\n");
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	FIXME("DirectDrawEnumerateExA: stub\n");
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	FIXME("DirectDrawEnumerateExW: stub\n");
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI DllCanUnloadNow()
{
	return S_FALSE;
}
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	FIXME("DllGetClassObject: stub\n");
	return CLASS_E_CLASSNOTAVAILABLE;
}
DDRAW_API void WINAPI GetDDSurfaceLocal()
{
	FIXME("GetDDSurfaceLocal: stub\n");
}
DDRAW_API void WINAPI GetOLEThunkData()
{
	FIXME("GetOleThunkData: stub\n");
}
DDRAW_API HRESULT WINAPI GlobalGetSurfaceFromDC(LPDIRECTDRAW7 lpDD, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	FIXME("GetSurfaceFromDC: Verify proper referencing for LPDIRECTDRAW7\n");
	return lpDD->GetSurfaceFromDC(hdc,lpDDS);
}
DDRAW_API void WINAPI RegisterSpecialCase()
{
	FIXME("RegisterSpecialCase: stub\n");
}
DDRAW_API void WINAPI ReleaseDDThreadLock()
{
	FIXME("ReleaseDDThreadLock: stub\n");
}
DDRAW_API void WINAPI SetAppCompatData()
{
	FIXME("SetAppCompatData: stub\n");
}
DDRAW_API BOOL IsDXGLDDraw()
{
	return TRUE;
}
