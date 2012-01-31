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
HRESULT WINAPI DirectDrawCreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	*lplpDDClipper = new glDirectDrawClipper(dwFlags,NULL);
	return DD_OK;
}
HRESULT WINAPI DirectDrawCreateEx(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter)
{
	if(ddenabled) return DDERR_DIRECTDRAWALREADYCREATED;
	GetCurrentConfig(&dxglcfg);
	glDirectDraw7 *myddraw;
	HRESULT error;
	if(iid != IID_IDirectDraw7) ERR(DDERR_UNSUPPORTED);
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

BOOL WINAPI DDEnumA(GUID FAR *guid, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKA callback = (LPDDENUMCALLBACKA)context[0];
	return callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
}

HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA lpCallback, LPVOID lpContext)
{
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExA(DDEnumA,&context,0);
}

BOOL WINAPI DDEnumW(GUID FAR *guid, LPWSTR lpDriverDescription, LPWSTR lpDriverName, LPVOID lpContext, HMONITOR hMonitor)
{
	int *context = (int *)lpContext;
	LPDDENUMCALLBACKW callback = (LPDDENUMCALLBACKW)context[0];
	return callback(guid,lpDriverDescription,lpDriverName,(LPVOID)context[1]);
}

HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, LPVOID lpContext)
{
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExW(DDEnumW,&context,0);
}

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

HRESULT WINAPI DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
	if(IsBadReadPointer(lpCallback)) return DDERR_INVALIDPARAMS;
	LPVOID context[2];
	context[0] = (LPVOID) lpCallback;
	context[1] = lpContext;
	return DirectDrawEnumerateExW(DDEnumExA,&context,dwFlags);
}

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
HRESULT WINAPI DllCanUnloadNow()
{
	return S_FALSE;
}
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
DDRAW_API void WINAPI GetDDSurfaceLocal()
{
	FIXME("GetDDSurfaceLocal: stub\n");
}
DDRAW_API HANDLE WINAPI GetOLEThunkData(int i1)
{
	DEBUG("GetOleThunkData: stub\n");
	return 0;
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
