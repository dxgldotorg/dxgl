// DXGL
// Copyright (C) 2011-2015 William Feely

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
#include "util.h"
#include "ddraw.h"
#include "glTexture.h"
#include "glUtil.h"
#include "glClassFactory.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"
#include "timer.h"
#include "glRenderer.h"
#include "hooks.h"
#include <intrin.h>

DXGLCFG dxglcfg;
DWORD gllock = 0;
HMODULE sysddraw = NULL;
HRESULT (WINAPI *sysddrawcreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter) = NULL;

const GUID device_template = 
{ 0x9ff8900, 0x8c4a, 0x4ba4, { 0xbf, 0x29, 0x56, 0x50, 0x4a, 0xf, 0x3b, 0xb3 } };


void InitGL(int width, int height, int bpp, bool fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7, bool devwnd)
{
	TRACE_ENTER(6,11,width,11,height,11,bpp,21,fullscreen,13,hWnd,14,glDD7);
	if (!glDD7->renderer)
	{
		glDD7->renderer = (glRenderer*)malloc(sizeof(glRenderer));
		glRenderer_Init(glDD7->renderer,width,height,bpp,fullscreen,frequency,hWnd,glDD7,devwnd);
	}
	else glRenderer_SetWnd(glDD7->renderer,width,height,bpp,fullscreen,frequency,hWnd,devwnd);
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI AcquireDDThreadLock()
{
	TRACE_ENTER(0);
	// FIXME:  Add thread lock
	FIXME("AcquireDDThreadLock: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI CompleteCreateSystemSurface()
{
	TRACE_ENTER(0);
	FIXME("CompleteCreateSystemSurface: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI D3DParseUnknownCommand()
{
	TRACE_ENTER(0);
	FIXME("D3DParseUnknownCommand: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDGetAttachedSurfaceLcl()
{
	TRACE_ENTER(0);
	FIXME("DDGetAttachedSurfaceLcl: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDInternalLock()
{
	TRACE_ENTER(0);
	//FIXME:  Add locking code
	FIXME("DDInternalLock: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDInternalUnlock()
{
	TRACE_ENTER(0);
	//FIXME:  Add unlocking code
	FIXME("DDInternalUnlock: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DSoundHelp()
{
	TRACE_ENTER(0);
	FIXME("DSoundHelp: stub\n");
	TRACE_EXIT(0,0);
}

/**
  * Creates an IDirectDraw compatible interface to the DXGL graphics library.
  * @param lpGUID
  *  Address to the GUID of the device to be created, or NULL for the current
  *  display.  Currently, only NULL is supported.
  * @param lplpDD
  *  Pointer to an address to be filled with an IDirectDraw compatible interface.
  *  To retreive a pointer to a later DirectDraw or Direct3D interface, call the
  *  QueryInterface method in the returned object.
  * @param pUnkOuter
  *  Unused, should be NULL.
  * @return
  *  Returns DD_OK if lplpDD points to an IDirectDraw pointer, or an error code
  *  otherwise.
  */
HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(3,24,lpGUID,14,lplpDD,14,pUnkOuter);
	if(!lplpDD) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!dll_cs.LockCount && !dll_cs.OwningThread) InitializeCriticalSection(&dll_cs);
	EnterCriticalSection(&dll_cs);
	HRESULT ret;
	if(gllock || IsCallerOpenGL(_ReturnAddress()))
	{
		if(!sysddraw)
		{
			char buffer[MAX_PATH];
			GetSystemDirectoryA(buffer,MAX_PATH);
			strcat(buffer,"\\ddraw.dll");
			sysddraw = LoadLibraryA(buffer);
			if(!sysddraw)
			{
				LeaveCriticalSection(&dll_cs);
				TRACE_EXIT(23,DDERR_GENERIC);
				ERR(DDERR_GENERIC);
			}
		}
		if(!sysddrawcreate)
		{
			sysddrawcreate = (HRESULT(WINAPI *)(GUID FAR*,LPDIRECTDRAW FAR*, IUnknown FAR*))GetProcAddress(sysddraw,"DirectDrawCreate");
			if(!sysddrawcreate)
			{
				LeaveCriticalSection(&dll_cs);
				TRACE_EXIT(23, DDERR_GENERIC);
				ERR(DDERR_GENERIC);
			}
		}
		ret = sysddrawcreate(lpGUID,lplpDD,pUnkOuter);
		TRACE_VAR("*lplpDD",14,*lplpDD);
		LeaveCriticalSection(&dll_cs);
		TRACE_EXIT(23, ret);
		return ret;
	}
	GetCurrentConfig(&dxglcfg,false);
	glDirectDraw7 *myddraw7;
	glDirectDraw1 *myddraw;
	HRESULT error;
	myddraw7 = new glDirectDraw7(lpGUID,pUnkOuter);
	error = myddraw7->err();
	if(error != DD_OK)
	{
		delete myddraw7;
		LeaveCriticalSection(&dll_cs);
		TRACE_EXIT(23, error);
		return error;
	}
	myddraw7->QueryInterface(IID_IDirectDraw,(VOID**)&myddraw);
	myddraw7->Release();
	*lplpDD = (LPDIRECTDRAW)myddraw;
	TRACE_VAR("*lplpDD",14,*lplpDD);
	LeaveCriticalSection(&dll_cs);
	TRACE_EXIT(23, error);
	return error;
}

/**
  * Creates an IDirectDrawClipper compatible interface.
  * @param dwFlags
  *  Unused, set to 0.
  * @param lplpDD
  *  Pointer to an address to be filled with an IDirectDrawClipper compatible
  *  interface.
  * @param pUnkOuter
  *  Unused, should be NULL.
  * @return
  *  Returns DD_OK if lplpDD points to an IDirectDrawClipper pointer, or an error
  *  code otherwise.
  */
HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(3,9,dwFlags,14,lplpDDClipper,14,pUnkOuter);
	TRACE_RET(HRESULT, 23, glDirectDrawClipper_Create(dwFlags, NULL, lplpDDClipper));
}

/**
  * Creates an IDirectDraw7 compatible interface to the DXGL graphics library.
  * @param lpGUID
  *  Address to the GUID of the device to be created, or NULL for the current
  *  display.  Currently, only NULL is supported.
  * @param lplpDD
  *  Pointer to an address to be filled with an IDirectDraw7 compatible interface.
  *  To retreive a pointer to a later DirectDraw or Direct3D interface, call the
  *  QueryInterface method in the returned object.
  * @param iid
  *  Must be set to IID_IDirectDraw7
  * @param pUnkOuter
  *  Unused, should be NULL.
  * @return
  *  Returns DD_OK if lplpDD points to an IDirectDraw7 pointer, or an error code
  *  otherwise.
  */
HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter)
{
	TRACE_ENTER(4,24,lpGUID,14,lplpDD,24,&iid,14,pUnkOuter);
	if(!lplpDD) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	GetCurrentConfig(&dxglcfg,false);
	glDirectDraw7 *myddraw;
	HRESULT error;
	if(iid != IID_IDirectDraw7)
	{
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	myddraw = new glDirectDraw7(lpGUID,pUnkOuter);
	error = myddraw->err();
	if(error != DD_OK)
	{
		delete myddraw;
		TRACE_EXIT(23,error);
		return error;
	}
	*lplpDD = (LPDIRECTDRAW7)myddraw;
	TRACE_VAR("*lplpDD",14,*lplpDD);
	TRACE_EXIT(23,error);
	return error;
}

/// Callback wrapper for DirectDrawEnumerateA
BOOL WINAPI DDEnumA(GUID FAR *guid, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	TRACE_ENTER(5,24,guid,15,lpDriverDescription,15,lpDriverName,14,lpContext,13,hMonitor);
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKA callback = (LPDDENUMCALLBACKA)context[0];
	BOOL ret =  callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
	TRACE_EXIT(22,ret);
	return ret;
}

/**
  * Enumerates the available device GUIDs for DXGL, ANSI character format.
  * Legacy call, uses DirectDrawEnumerateExA
  * @param lpCallback
  *  Address of the function to call for each enumerated object.
  * @param lpContext
  *  Pointer to be passed to the callback function.
  * @return
  *  Returns DD_OK if the call succeeds, or DDERR_INVALIDPARAMS if lpCallback
  *  is invalid.
  */
HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
	TRACE_ENTER(2,14,lpCallback,14,lpContext);
	if(!IsReadablePointer(lpCallback))
	{
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	HRESULT ret = DirectDrawEnumerateExA(DDEnumA,&context,0);
	TRACE_EXIT(23,ret);
	return ret;
}

/// Callback wrapper for DirectDrawEnumerateW
BOOL WINAPI DDEnumW(GUID FAR *guid, LPWSTR lpDriverDescription, LPWSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	TRACE_ENTER(5,24,guid,16,lpDriverDescription,16,lpDriverName,14,lpContext,13,hMonitor);
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKW callback = (LPDDENUMCALLBACKW)context[0];
	BOOL ret = callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
	TRACE_EXIT(22,ret);
	return ret;
}

/**
  * Enumerates the available device GUIDs for DXGL, Unicode character format.
  * Legacy call, uses DirectDrawEnumerateExW
  * @param lpCallback
  *  Address of the function to call for each enumerated object.
  * @param lpContext
  *  Pointer to be passed to the callback function.
  * @return
  *  Returns DD_OK if the call succeeds, or DDERR_INVALIDPARAMS if lpCallback
  *  is invalid.
  */
HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext)
{
	TRACE_ENTER(2,14,lpCallback,14,lpContext);
	if(!IsReadablePointer(lpCallback))
	{
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	HRESULT ret = DirectDrawEnumerateExW(DDEnumW,&context,0);
	TRACE_EXIT(23,ret);
	return ret;
}

/// Callback wrapper for DirectDrawEnumerateExA
/// Converts Unicode strings to ANSI.
BOOL WINAPI DDEnumExA(GUID FAR *guid, LPWSTR lpDriverDescription, LPWSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	TRACE_ENTER(5,24,guid,16,lpDriverDescription,16,lpDriverName,14,lpContext,13,hMonitor);
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKEXA callback = (LPDDENUMCALLBACKEXA)context[0];
	CHAR desc[MAX_PATH];
	CHAR driver[MAX_PATH];
	WideCharToMultiByte(CP_ACP,0,lpDriverDescription,-1,desc,MAX_PATH,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,lpDriverName,-1,driver,MAX_PATH,NULL,NULL);
	BOOL ret = callback(guid,desc,driver,(LPVOID)context[1],hMonitor);
	TRACE_EXIT(22,ret);
	return ret;
}

/**
  * Enumerates the available device GUIDs for DXGL, ANSI character format.
  * Uses DirectDrawEnumerateExW
  * @param lpCallback
  *  Address of the function to call for each enumerated object.
  * @param lpContext
  *  Pointer to be passed to the callback function.
  * @param dwFlags
  *  Use DDENUM_ATTACHEDSECONDARYDEVICES to enumerate display devices attached
  *  to the system.
  * @return
  *  Returns DD_OK if the call succeeds, or DDERR_INVALIDPARAMS if lpCallback
  *  is invalid.
  */
HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	TRACE_ENTER(3,14,lpCallback,14,lpContext,9,dwFlags);
	if(!IsReadablePointer(lpCallback))
	{
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	HRESULT ret = DirectDrawEnumerateExW(DDEnumExA,&context,dwFlags);
	TRACE_EXIT(23,ret);
	return ret;
}

/**
  * Callback for EnumDisplayMonitors.
  * @param hMonitor
  *  Handle to display device found by EnumDisplayMonitors.
  * @param ptr
  *  Pointer to list of display devices.  First value is the number of devices
  *  found so far.  Subsequent values are stored display device handles.
  */
BOOL CALLBACK MonitorEnum(HMONITOR hMonitor, HDC unused, LPRECT unused2, LPARAM ptr)
{
	TRACE_ENTER(4,13,hMonitor,13,unused,26,unused2,13,ptr);
	int * monitors = *(int**)ptr;
	if(!monitors)
	{
		monitors = (int*)malloc(256*sizeof(int));
		if(!monitors)
		{
			TRACE_EXIT(22,FALSE);
			return FALSE;
		}
		monitors[0] = 1;
	}
	else monitors[0]++;
	monitors[monitors[0]] = (int)hMonitor;
	*(int**)ptr = monitors;
	BOOL ret;
	if(monitors[0] == 255) ret = FALSE;
	else ret = TRUE;
	TRACE_EXIT(22,ret);
	return ret;
}

/**
  * Enumerates the available device GUIDs for DXGL, Unicode character format.
  * @param lpCallback
  *  Address of the function to call for each enumerated object.
  * @param lpContext
  *  Pointer to be passed to the callback function.
  * @param dwFlags
  *  Use DDENUM_ATTACHEDSECONDARYDEVICES to enumerate display devices attached
  *  to the system.
  * @return
  *  Returns DD_OK if the call succeeds, or DDERR_INVALIDPARAMS if lpCallback
  *  is invalid.
  */
HRESULT WINAPI DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	TRACE_ENTER(3,14,lpCallback,14,lpContext,9,dwFlags);
	if(!IsReadablePointer(lpCallback))
	{
		TRACE_EXIT(23,DDERR_GENERIC);
		return DDERR_INVALIDPARAMS;
	}
	int *monitors = NULL;
	GUID guid;
	MONITORINFOEXW monitorinfo;
	monitorinfo.cbSize = sizeof(MONITORINFOEXW);
	DISPLAY_DEVICEW dev;
	dev.cb = sizeof(DISPLAY_DEVICE);
	if(!lpCallback(NULL,L"Primary Display Driver",L"display",lpContext,0)) return DD_OK;
	if(dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES)
	{
		EnumDisplayMonitors(NULL,NULL,MonitorEnum,(LPARAM)&monitors);
		if(!monitors)
		{
			TRACE_EXIT(23,DDERR_OUTOFMEMORY);
			return DDERR_OUTOFMEMORY;
		}
		for(int i = 1; i < monitors[0]; i++)
		{
			guid = device_template;
			guid.Data1 |= i;
			GetMonitorInfoW((HMONITOR)monitors[i],&monitorinfo);
			EnumDisplayDevicesW(NULL,(monitorinfo.szDevice[_tcslen(monitorinfo.szDevice)-1])-49,&dev,0);
			lpCallback(&guid,dev.DeviceString,monitorinfo.szDevice,lpContext,(HMONITOR)monitors[i]);
		}
		free(monitors);
	}
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

/**
  * Tells the operating system whether it can unload the DLL.
  * @return
  *  Currently returns S_FALSE, preventing the operating system from unloading
  *  the DLL until the application terminates.
  */
HRESULT WINAPI DllCanUnloadNow()
{
	TRACE_ENTER(0);
	TRACE_EXIT(23,S_FALSE);
	return S_FALSE;
}

/**
  * Creates an IClassFactory object to retrieve a DirectDraw object using COM.
  * @param rclsid
  *  Must be one of the following:  CLSID_DirectDraw, CLSID_DirectDraw7, or
  *  CLSID_DirectDrawClipper
  * @param riid
  *  Must be IID_IUnknown or IID_IClassFactory.
  * @param ppv
  *  Pointer to an address to the IClassFactory object created by this call.
  * @return
  *  Returns S_OK if the call succeeds, or an error otherwise.
  */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	TRACE_ENTER(3,24,&rclsid,24,&riid,14,ppv);
	if((rclsid != CLSID_DirectDraw) && (rclsid != CLSID_DirectDraw7) &&
		(rclsid != CLSID_DirectDrawClipper))
	{
		TRACE_EXIT(23,CLASS_E_CLASSNOTAVAILABLE);
		return CLASS_E_CLASSNOTAVAILABLE;
	}
	GetCurrentConfig(&dxglcfg,false);
	glClassFactory *factory = new glClassFactory;
	if(factory == NULL)
	{
		TRACE_EXIT(23,E_OUTOFMEMORY);
		return E_OUTOFMEMORY;
	}
	HRESULT result = factory->QueryInterface(riid,ppv);
	TRACE_VAR("*ppv",14,*ppv);
	factory->Release();
	TRACE_EXIT(23,result);
	return result;
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI GetDDSurfaceLocal()
{
	TRACE_ENTER(0);
	TRACE_EXIT(0,0);
	FIXME("GetDDSurfaceLocal: stub\n");
}

/// Stub for function found in system ddraw.dll
/// This function gets called by the fnddraw.exe test application.
DDRAW_API HANDLE WINAPI GetOLEThunkData(int i1)
{
	TRACE_ENTER(1,9,i1);
	TRACE_EXIT(14,NULL);
	DEBUG("GetOleThunkData: stub\n");
	return 0;
}

/// Stub for function found in system ddraw.dll
/// Function import is GetSurfaceFromDC
DDRAW_API HRESULT WINAPI GlobalGetSurfaceFromDC(LPDIRECTDRAW7 lpDD, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	TRACE_ENTER(3,14,lpDD,9,hdc,14,lpDDS);
	FIXME("GetSurfaceFromDC: Verify proper referencing for LPDIRECTDRAW7\n");
	HRESULT ret = lpDD->GetSurfaceFromDC(hdc,lpDDS);
	TRACE_VAR("*lpDDS",14,*lpDDS);
	TRACE_EXIT(23,ret);
	return ret;
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI RegisterSpecialCase()
{
	TRACE_ENTER(0);
	FIXME("RegisterSpecialCase: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI ReleaseDDThreadLock()
{
	TRACE_ENTER(0);
	FIXME("ReleaseDDThreadLock: stub\n");
	TRACE_EXIT(0,0);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI SetAppCompatData()
{
	TRACE_ENTER(0);
	FIXME("SetAppCompatData: stub\n");
	TRACE_EXIT(0,0);
}

/**
  * Test if the ddraw.dll file is DXGL.  Do not link to this entry point.
  * Use LoadLibrary and GetProcAddress instead.
  * @return
  *  Returns TRUE
  * @remark
  *  Test for DXGL by testing if this function exists.  Please do not use
  *  this function to test whether your program should run or not.  This
  *  function may be changed or removed in case of abuse.
  */

DDRAW_API BOOL IsDXGLDDraw()
{
	TRACE_ENTER(0);
	TRACE_EXIT(0,0);
	return TRUE;
}
