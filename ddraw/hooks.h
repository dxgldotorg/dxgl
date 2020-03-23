// DXGL
// Copyright (C) 2014-2020 William Feely
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

BOOL IsCallerOpenGL(BYTE *returnaddress);

void InitHooks();
void ShutdownHooks();
LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InstallDXGLHook(HWND hWnd, LPDIRECTDRAW7 lpDD7);
void UninstallDXGLHook(HWND hWnd);
void EnableWindowScaleHook(BOOL enable);

// Window management
extern LONG(WINAPI *_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwNewLong);
extern LONG(WINAPI *_SetWindowLongW)(HWND hWnd, int nIndex, LONG dwNewLong);
extern LONG(WINAPI *_GetWindowLongA)(HWND hWnd, int nIndex);
extern LONG(WINAPI *_GetWindowLongW)(HWND hWnd, int nIndex);
#ifdef _M_X64 // 64-bit only, 32-bit uses macros
extern LONG_PTR(WINAPI *_SetWindowLongPtrA)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
extern LONG_PTR(WINAPI *_SetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
extern LONG_PTR(WINAPI *_GetWindowLongPtrA)(HWND hWnd, int nIndex);
extern LONG_PTR(WINAPI *_GetWindowLongPtrW)(HWND hWnd, int nIndex);
#else
#define _SetWindowLongPtrA   _SetWindowLongA
#define _SetWindowLongPtrW   _SetWindowLongW
#define _GetWindowLongPtrA   _GetWindowLongA
#define _GetWindowLongPtrW   _GetWindowLongW
#endif

// Mouse pointer
extern BOOL(WINAPI *_GetCursorPos)(LPPOINT point);
extern BOOL(WINAPI *_SetCursorPos)(int X, int Y);
extern HCURSOR(WINAPI *_SetCursor)(HCURSOR hCursor);

// Display geometry
extern BOOL(WINAPI *_EnumDisplaySettingsA)(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode);
extern BOOL(WINAPI *_EnumDisplaySettingsW)(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode);
extern BOOL(WINAPI *_EnumDisplaySettingsExA)(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode, DWORD dwFlags);
extern BOOL(WINAPI *_EnumDisplaySettingsExW)(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode, DWORD dwFlags);
extern int(WINAPI *_GetSystemMetrics)(int nIndex);

// Window geometry
extern BOOL(WINAPI *_GetClientRect)(HWND hWnd, LPRECT lpRect);
extern BOOL(WINAPI *_GetWindowRect)(HWND hWnd, LPRECT lpRect);
extern BOOL(WINAPI *_ClientToScreen)(HWND hWnd, LPPOINT lpPoint);
extern BOOL(WINAPI *_MoveWindow)(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
extern BOOL(WINAPI* _SetWindowPos)(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);


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

BOOL WINAPI HookEnumDisplaySettingsA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode);
BOOL WINAPI HookEnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode);
BOOL WINAPI HookEnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode, DWORD dwFlags);
BOOL WINAPI HookEnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode, DWORD dwFlags);
int WINAPI HookGetSystemMetrics(int nIndex);

BOOL WINAPI HookGetClientRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI HookGetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI HookClientToScreen(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI HookMoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI HookSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);


#ifdef __cplusplus
}
#endif

#endif //__DXGLHOOKS_H
