// DXGL
// Copyright (C) 2014-2015 William Feely
// Portions copyright (C) 2018 Syahmi Azhar

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
#ifndef __DXGLHOOKS_H
#define __DXGLHOOKS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	HWND hwnd;
	WNDPROC wndproc;
	LPDIRECTDRAW7 lpDD7;
} HWND_HOOK;

extern CRITICAL_SECTION hook_cs;

BOOL IsCallerOpenGL(void *returnaddress);

void InitHooks();
void ShutdownHooks();
LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InstallDXGLFullscreenHook(HWND hWnd, LPDIRECTDRAW7 lpDD7);
void UninstallDXGLFullscreenHook(HWND hWnd);

LONG WINAPI HookSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI HookSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI HookGetWindowLongA(HWND hWnd, int nIndex);
LONG WINAPI HookGetWindowLongW(HWND hWnd, int nIndex);
#ifdef _M_X64
LONG_PTR WINAPI HookSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI HookSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR WINAPI HookGetWindowLongPtrA(HWND hWnd, int nIndex);
LONG_PTR WINAPI HookGetWindowLongPtrW(HWND hWnd, int nIndex);
#endif
BOOL WINAPI HookGetCursorPos(LPPOINT point);
BOOL WINAPI HookSetCursorPos(int x, int y);
HCURSOR WINAPI HookSetCursor(HCURSOR hCursor);

#ifdef __cplusplus
}
#endif

#endif //__DXGLHOOKS_H
