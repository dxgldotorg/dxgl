// DXGL
// Copyright (C) 2011-2012 William Feely

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
#include "glClassFactory.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"
#include <intrin.h>
#include <tlhelp32.h>

DXGLCFG dxglcfg;
bool gllock = false;
HMODULE sysddraw = NULL;
HRESULT (WINAPI *sysddrawcreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter) = NULL;

const GUID device_template = 
{ 0x9ff8900, 0x8c4a, 0x4ba4, { 0xbf, 0x29, 0x56, 0x50, 0x4a, 0xf, 0x3b, 0xb3 } };

DWORD timer;
int vsyncstatus;
bool ddenabled = false;

/**
  * Tests if a pointer is valid for reading from.  Compile ion Visual C++ with /EHa
  * enabled Structed Exception Handling in C++ code, to prevent crashes on invalid
  * pointers.
  * @param ptr
  *  Pointer to test for validity.
  * @return
  *  Returns false if the pointer is valid, or true if an error occurs.
  */
bool IsBadReadPointer(void *ptr)
{
	char a;
	try
	{
		a = *(char*)ptr;
		if(a == *(char*)ptr) return false;
		else return true;
	}
	catch(...)
	{
		return true;
	}
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI AcquireDDThreadLock()
{
	// FIXME:  Add thread lock
	FIXME("AcquireDDThreadLock: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI CompleteCreateSystemSurface()
{
	FIXME("CompleteCreateSystemSurface: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI D3DParseUnknownCommand()
{
	FIXME("D3DParseUnknownCommand: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDGetAttachedSurfaceLcl()
{
	FIXME("DDGetAttachedSurfaceLcl: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDInternalLock()
{
	//FIXME:  Add locking code
	FIXME("DDInternalLock: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DDInternalUnlock()
{
	//FIXME:  Add unlocking code
	FIXME("DDInternalUnlock: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI DSoundHelp()
{
	FIXME("DSoundHelp: stub\n");
}

/**
  * This function is used by DirectDrawCreate to test if opengl32.dll is calling
  * these functions.  If so, DirectDrawCreate will load the system ddraw.dll and
  * call its DirectDrawCreate function.
  * @param returnaddress
  *  The address to evaluate whether it is from opengl32.dll or not.
  *  The return address of the calling function may be obtained with the
  *  _ReturnAddress() function.
  * @return
  *  Returns nonzero if the address points to opengl32.dll, otherwise returns zero.
  */
int IsCallerOpenGL(void *returnaddress)
{
	int isgl = 0;
	MODULEENTRY32 modentry = {0};
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,0);
	modentry.dwSize = sizeof(MODULEENTRY32);
	Module32First(hSnapshot,&modentry);
	do
	{
		if((modentry.modBaseAddr <= returnaddress) &&
			(modentry.modBaseAddr+modentry.modBaseSize > returnaddress))
		{
			if(!_tcsicmp(modentry.szModule,_T("opengl32.dll"))) isgl=1;
			break;
		}
	} while(Module32Next(hSnapshot,&modentry));
	CloseHandle(hSnapshot);
	return isgl;
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
	if(gllock || IsCallerOpenGL(_ReturnAddress()))
	{
		if(!sysddraw)
		{
			char buffer[MAX_PATH];
			GetSystemDirectoryA(buffer,MAX_PATH);
			strcat(buffer,"\\ddraw.dll");
			sysddraw = LoadLibraryA(buffer);
			if(!sysddraw) ERR(DDERR_GENERIC);
		}
		if(!sysddrawcreate)
		{
			sysddrawcreate = (HRESULT(WINAPI *)(GUID FAR*,LPDIRECTDRAW FAR*, IUnknown FAR*))GetProcAddress(sysddraw,"DirectDrawCreate");
			if(!sysddrawcreate) ERR(DDERR_GENERIC);
		}
		return sysddrawcreate(lpGUID,lplpDD,pUnkOuter);
	}
	if(ddenabled) return DDERR_DIRECTDRAWALREADYCREATED;
	GetCurrentConfig(&dxglcfg);
	glDirectDraw7 *myddraw7;
	glDirectDraw1 *myddraw;
	HRESULT error;
	myddraw7 = new glDirectDraw7(lpGUID,pUnkOuter);
	error = myddraw7->err();
	if(error != DD_OK)
	{
		delete myddraw7;
		return error;
	}
	ddenabled = true;
	myddraw7->QueryInterface(IID_IDirectDraw,(VOID**)&myddraw);
	myddraw7->Release();
	*lplpDD = (LPDIRECTDRAW)myddraw;
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
	*lplpDDClipper = new glDirectDrawClipper(dwFlags,NULL);
	return DD_OK;
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
	if(ddenabled) return DDERR_DIRECTDRAWALREADYCREATED;
	GetCurrentConfig(&dxglcfg);
	glDirectDraw7 *myddraw;
	HRESULT error;
	if(iid != IID_IDirectDraw7) ERR(DDERR_INVALIDPARAMS);
	myddraw = new glDirectDraw7(lpGUID,pUnkOuter);
	error = myddraw->err();
	if(error != DD_OK)
	{
		delete myddraw;
		return error;
	}
	ddenabled = true;
	*lplpDD = (LPDIRECTDRAW7)myddraw;
	return error;
}

/// Callback wrapper for DirectDrawEnumerateA
BOOL WINAPI DDEnumA(GUID FAR *guid, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKA callback = (LPDDENUMCALLBACKA)context[0];
	return callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
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
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExA(DDEnumA,&context,0);
}

/// Callback wrapper for DirectDrawEnumerateW
BOOL WINAPI DDEnumW(GUID FAR *guid, LPWSTR lpDriverDescription, LPWSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKW callback = (LPDDENUMCALLBACKW)context[0];
	return callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
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
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExW(DDEnumW,&context,0);
}

/// Callback wrapper for DirectDrawEnumerateExA
/// Converts Unicode strings to ANSI.
BOOL WINAPI DDEnumExA(GUID FAR *guid, LPWSTR lpDriverDescription, LPWSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKEXA callback = (LPDDENUMCALLBACKEXA)context[0];
	CHAR desc[MAX_PATH];
	CHAR driver[MAX_PATH];
	WideCharToMultiByte(CP_ACP,0,lpDriverDescription,-1,desc,MAX_PATH,NULL,NULL);
	WideCharToMultiByte(CP_ACP,0,lpDriverName,-1,driver,MAX_PATH,NULL,NULL);
	return callback(guid,desc,driver,(LPVOID)context[1],hMonitor);
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
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExW(DDEnumExA,&context,dwFlags);
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
	int * monitors = *(int**)ptr;
	if(!monitors)
	{
		monitors = (int*)malloc(256*sizeof(int));
		monitors[0] = 1;
	}
	else monitors[0]++;
	monitors[monitors[0]] = (int)hMonitor;
	*(int**)ptr = monitors;
	if(monitors[0] == 255) return FALSE;
	return TRUE;
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
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
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
	if((rclsid != CLSID_DirectDraw) && (rclsid != CLSID_DirectDraw7) &&
		(rclsid != CLSID_DirectDrawClipper)) return CLASS_E_CLASSNOTAVAILABLE;
	GetCurrentConfig(&dxglcfg);
	glClassFactory *factory = new glClassFactory;
	if(factory == NULL) return E_OUTOFMEMORY;
	HRESULT result = factory->QueryInterface(riid,ppv);
	factory->Release();
	return result;
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI GetDDSurfaceLocal()
{
	FIXME("GetDDSurfaceLocal: stub\n");
}

/// Stub for function found in system ddraw.dll
/// This function gets called by the fnddraw.exe test application.
DDRAW_API HANDLE WINAPI GetOLEThunkData(int i1)
{
	DEBUG("GetOleThunkData: stub\n");
	return 0;
}

/// Stub for function found in system ddraw.dll
/// Function import is GetSurfaceFromDC
DDRAW_API HRESULT WINAPI GlobalGetSurfaceFromDC(LPDIRECTDRAW7 lpDD, HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	FIXME("GetSurfaceFromDC: Verify proper referencing for LPDIRECTDRAW7\n");
	return lpDD->GetSurfaceFromDC(hdc,lpDDS);
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI RegisterSpecialCase()
{
	FIXME("RegisterSpecialCase: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI ReleaseDDThreadLock()
{
	FIXME("ReleaseDDThreadLock: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API void WINAPI SetAppCompatData()
{
	FIXME("SetAppCompatData: stub\n");
}

/// Stub for function found in system ddraw.dll
DDRAW_API BOOL IsDXGLDDraw()
{
	return TRUE;
}
