// DXGL
// Copyright (C) 2011-2018 William Feely

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
ATOM WindowClass = NULL;
CRITICAL_SECTION dll_cs = {NULL,0,0,NULL,NULL,0};
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		if(!dll_cs.LockCount && !dll_cs.OwningThread) InitializeCriticalSection(&dll_cs);
		if (!hook_cs.LockCount && !hook_cs.OwningThread) InitializeCriticalSection(&hook_cs);
		GetCurrentConfig(&dxglcfg, TRUE);
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

