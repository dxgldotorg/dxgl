// DXGL
// Copyright (C) 2011-2024 William Feely

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
#include "hooks.h"
#include "util.h"


MEMORYSTATUSEX memstatusex;
MEMORYSTATUS memstatus;

BOOL(WINAPI *_GlobalMemoryStatusEx)(LPMEMORYSTATUSEX lpBuffer) = NULL;
HRESULT(WINAPI *_DirectDrawCreate)(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter);

ATOM WindowClass = NULL;
extern "C" CRITICAL_SECTION dll_cs = {NULL,0,0,NULL,NULL,0};
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	HMODULE hKernel32;
	HMODULE hSystemDDraw;
	TCHAR path[MAX_PATH];
	IDirectDraw *dd;
	IDirectDraw2 *dd2;
	DDSCAPS caps;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if(!dll_cs.LockCount && !dll_cs.OwningThread) InitializeCriticalSection(&dll_cs);
		if (!hook_cs.LockCount && !hook_cs.OwningThread) InitializeCriticalSection(&hook_cs);
		GetCurrentConfig(&dxglcfg, TRUE);
		dxglcfg.SystemRAM = 0;
		dxglcfg.VideoRAM = 0;
		hKernel32 = GetModuleHandle(_T("kernel32.dll"));
		if (hKernel32)
		{
			_GlobalMemoryStatusEx = 
				(BOOL(WINAPI*)(LPMEMORYSTATUSEX))GetProcAddress(hKernel32, "GlobalMemoryStatusEx");
			if (_GlobalMemoryStatusEx)
			{
				memstatusex.dwLength = sizeof(MEMORYSTATUSEX);
				_GlobalMemoryStatusEx(&memstatusex);
				dxglcfg.SystemRAM = memstatusex.ullTotalPhys;
			}
		}
		if (!dxglcfg.SystemRAM)
		{
			memstatus.dwLength = sizeof(MEMORYSTATUS);
			GlobalMemoryStatus(&memstatus);
			dxglcfg.SystemRAM = memstatus.dwTotalPhys;
		}
		/*GetSystemDirectory(path, MAX_PATH);
		_tcscat(path, _T("\\ddraw.dll"));
		hSystemDDraw = LoadLibrary(path);
		if (!hSystemDDraw)
		{
			MessageBox(NULL, _T("Cannot find ddraw.dll; DirectX must be installed."), _T("Fatal error"), MB_OK | MB_ICONHAND);
			ExitProcess(-1);
		}
		if (GetProcAddress(hSystemDDraw, "IsDXGLDDraw"))
		{
			MessageBox(NULL, _T("DXGL detected in system folder.  Your operating system is damaged, and this program cannot continue.  Please restore your operating system or DirectX files to continue."), _T("Fatal error"), MB_OK | MB_ICONHAND);
			ExitProcess(-1);
		}
		_DirectDrawCreate = 
			(HRESULT(WINAPI*)(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown *))GetProcAddress(hSystemDDraw, "DirectDrawCreate");
		if(!_DirectDrawCreate)
		{
			MessageBox(NULL, _T("Failed to load sytem DirectDraw."), _T("Fatal error"), MB_OK | MB_ICONHAND);
			ExitProcess(-1);
		}
		_DirectDrawCreate(NULL, &dd, NULL);
		dd->QueryInterface(IID_IDirectDraw2, (void**)&dd2);
		caps.dwCaps = 0;
		dd2->GetAvailableVidMem(&caps, (DWORD*)&dxglcfg.VideoRAM, NULL);
		dd2->Release();
		dd->Release();
		FreeLibrary(hSystemDDraw);*/
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		ShutdownHooks();
		DeleteCriticalSection(&hook_cs);
		ZeroMemory(&hook_cs, sizeof(CRITICAL_SECTION));
		DeleteCriticalSection(&dll_cs);
		ZeroMemory(&dll_cs, sizeof(CRITICAL_SECTION));
		if (wndclassdxgltempatom) UnregisterDXGLTempWindowClass();
		break;
	}
	return TRUE;
}

