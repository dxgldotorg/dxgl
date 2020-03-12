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

#include "common.h"
#include "hooks.h"
#include <tlhelp32.h>
#include "../minhook/include/MinHook.h"

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

// temporary references to C++ C-linked stuff
void glDirectDraw7_UnrestoreDisplayMode(LPDIRECTDRAW7 lpDD7);
void glDirectDraw7_SetWindowSize(LPDIRECTDRAW7 lpDD7, DWORD dwWidth, DWORD dwHeight);
void glDirectDraw7_GetSizes(LPDIRECTDRAW7 lpDD7, LONG *sizes);
BOOL glDirectDraw7_GetFullscreen(LPDIRECTDRAW7 lpDD7);
extern DXGLCFG dxglcfg;

const TCHAR *wndprop = _T("DXGLWndProc");
const TCHAR *wndpropdd7 = _T("DXGLWndDD7");
static HWND_HOOK *hwndhooks = NULL;
static int hwndhook_count = 0;
static int hwndhook_max = 0;
CRITICAL_SECTION hook_cs = { NULL, 0, 0, NULL, NULL, 0 };
static BOOL hooks_init = FALSE;

// Window management
LONG(WINAPI *_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwNewLong) = NULL;
LONG(WINAPI *_SetWindowLongW)(HWND hWnd, int nIndex, LONG dwNewLong) = NULL;
LONG(WINAPI *_GetWindowLongA)(HWND hWnd, int nIndex) = NULL;
LONG(WINAPI *_GetWindowLongW)(HWND hWnd, int nIndex) = NULL;
#ifdef _M_X64 // 64-bit only, 32-bit uses macros
LONG_PTR(WINAPI *_SetWindowLongPtrA)(HWND hWnd, int nIndex, LONG_PTR dwNewLong) = NULL;
LONG_PTR(WINAPI *_SetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong) = NULL;
LONG_PTR(WINAPI *_GetWindowLongPtrA)(HWND hWnd, int nIndex) = NULL;
LONG_PTR(WINAPI *_GetWindowLongPtrW)(HWND hWnd, int nIndex) = NULL;
#else
#define _SetWindowLongPtrA   _SetWindowLongA
#define _SetWindowLongPtrW   _SetWindowLongW
#define _GetWindowLongPtrA   _GetWindowLongA
#define _GetWindowLongPtrW   _GetWindowLongW
#endif

// Mouse pointer
BOOL(WINAPI *_GetCursorPos)(LPPOINT point) = NULL;
BOOL(WINAPI *_SetCursorPos)(int X, int Y) = NULL;
HCURSOR(WINAPI *_SetCursor)(HCURSOR hCursor) = NULL;

// Display geometry
BOOL(WINAPI *_EnumDisplaySettingsA)(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode);
BOOL(WINAPI *_EnumDisplaySettingsW)(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode);
BOOL(WINAPI *_EnumDisplaySettingsExA)(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode, DWORD dwFlags);
BOOL(WINAPI *_EnumDisplaySettingsExW)(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode, DWORD dwFlags);
int(WINAPI *_GetSystemMetrics)(int nIndex);

UINT wndhook_count = 0;
static BOOL keyctrlalt = FALSE;
static BOOL cursorclipped = FALSE;
static RECT rcOldClip;
static RECT rcClip;
static RECT rcWindow;
static BOOL windowscalehook = FALSE;

int hwndhookcmp(const HWND_HOOK *key, const HWND_HOOK *cmp)
{
	if (!cmp->hwnd) return 1; // Put blanks at end for cleanup
	if (key->hwnd < cmp->hwnd) return -1;
	else if (key->hwnd == cmp->hwnd) return 0;
	else return 1;
}

void SetHookWndProc(HWND hWnd, WNDPROC wndproc, LPDIRECTDRAW7 lpDD7, BOOL proconly, BOOL delete)
{
	HWND_HOOK cmphook;
	HWND_HOOK *hook;
	cmphook.hwnd = hWnd;
	if (!hwndhooks)
	{
		hwndhooks = (HWND_HOOK*)malloc(16 * sizeof(HWND_HOOK));
		hwndhook_count = 0;
		hwndhook_max = 16;
	}
	hook = bsearch(&cmphook, hwndhooks, hwndhook_count, sizeof(HWND_HOOK), hwndhookcmp);
	if (delete)
	{
		if (!hook) return;
		hook->hwnd = NULL;
		hook->wndproc = NULL;
		hook->lpDD7 = NULL;
		qsort(hwndhooks, hwndhook_count, sizeof(HWND_HOOK), hwndhookcmp);
		hwndhook_count--;
		return;
	}
	if (!hook)
	{
		hwndhook_count++;
		if (hwndhook_count >= hwndhook_max)
		{
			hwndhook_max += 16;
			hwndhooks = (HWND_HOOK*)realloc(hwndhooks, hwndhook_max*sizeof(HWND_HOOK));
		}
		hook = &hwndhooks[hwndhook_count - 1];
	}
	hook->hwnd = hWnd;
	hook->wndproc = wndproc;
	if (!proconly) hook->lpDD7 = lpDD7;
	qsort(hwndhooks, hwndhook_count, sizeof(HWND_HOOK), hwndhookcmp);
}

HWND_HOOK *GetWndHook(HWND hWnd)
{
	HWND_HOOK cmphook;
	cmphook.hwnd = hWnd;
	if (!hwndhooks) return NULL;
	return bsearch(&cmphook, hwndhooks, hwndhook_count, sizeof(HWND_HOOK), hwndhookcmp);
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
BOOL IsCallerOpenGL(BYTE *returnaddress)
{
	HANDLE hSnapshot;
	int isgl = 0;
	MODULEENTRY32 modentry = { 0 };
	TRACE_ENTER(1, 14, returnaddress);
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	modentry.dwSize = sizeof(MODULEENTRY32);
	Module32First(hSnapshot, &modentry);
	do
	{
		if ((modentry.modBaseAddr <= returnaddress) &&
			(modentry.modBaseAddr + modentry.modBaseSize > returnaddress))
		{
			if (!_tcsicmp(modentry.szModule, _T("opengl32.dll"))) isgl = 1;
			break;
		}
	} while (Module32Next(hSnapshot, &modentry));
	CloseHandle(hSnapshot);
	TRACE_EXIT(22, isgl);
	return isgl;
}


LRESULT CALLBACK nullwndproc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return 0;
}

void InitHooks()
{
	if (hooks_init) return;
	EnterCriticalSection(&hook_cs);
	wndhook_count = 0;
	MH_Initialize();
	MH_CreateHook(&SetWindowLongA, HookSetWindowLongA, (LPVOID*)&_SetWindowLongA);
	MH_CreateHook(&SetWindowLongW, HookSetWindowLongW, (LPVOID*)&_SetWindowLongW);
	MH_CreateHook(&GetWindowLongA, HookGetWindowLongA, (LPVOID*)&_GetWindowLongA);
	MH_CreateHook(&GetWindowLongW, HookGetWindowLongW, (LPVOID*)&_GetWindowLongW);
#ifdef _M_X64
	MH_CreateHook(&SetWindowLongPtrA, HookSetWindowLongPtrA, (LPVOID*)&_SetWindowLongPtrA);
	MH_CreateHook(&SetWindowLongPtrW, HookSetWindowLongPtrW, (LPVOID*)&_SetWindowLongPtrW);
	MH_CreateHook(&GetWindowLongPtrA, HookGetWindowLongPtrA, (LPVOID*)&_GetWindowLongPtrA);
	MH_CreateHook(&GetWindowLongPtrW, HookGetWindowLongPtrW, (LPVOID*)&_GetWindowLongPtrW);
#endif

	MH_CreateHook(&GetCursorPos, HookGetCursorPos, (LPVOID*)&_GetCursorPos);
	MH_CreateHook(&SetCursorPos, HookSetCursorPos, (LPVOID*)&_SetCursorPos);
	MH_CreateHook(&SetCursor, HookSetCursor, (LPVOID*)&_SetCursor);

	MH_CreateHook(&EnumDisplaySettingsA, HookEnumDisplaySettingsA, (LPVOID*)&_EnumDisplaySettingsA);
	MH_CreateHook(&EnumDisplaySettingsW, HookEnumDisplaySettingsW, (LPVOID*)&_EnumDisplaySettingsW);
	MH_CreateHook(&EnumDisplaySettingsExA, HookEnumDisplaySettingsExA, (LPVOID*)&_EnumDisplaySettingsExA);
	MH_CreateHook(&EnumDisplaySettingsExW, HookEnumDisplaySettingsExW, (LPVOID*)&_EnumDisplaySettingsExW);
	MH_CreateHook(&GetSystemMetrics, HookGetSystemMetrics, (LPVOID*)&_GetSystemMetrics);

	hooks_init = TRUE;
	LeaveCriticalSection(&hook_cs);
}

void ShutdownHooks()
{
	if (!hooks_init) return;
	EnterCriticalSection(&hook_cs);
	MH_RemoveHook(&SetWindowLongA);
	MH_RemoveHook(&SetWindowLongW);
	MH_RemoveHook(&GetWindowLongA);
	MH_RemoveHook(&GetWindowLongW);
#ifdef _M_X64
	MH_RemoveHook(&SetWindowLongPtrA);
	MH_RemoveHook(&SetWindowLongPtrW);
	MH_RemoveHook(&GetWindowLongPtrA);
	MH_RemoveHook(&GetWindowLongPtrW);
#endif

	MH_RemoveHook(&GetCursorPos);
	MH_RemoveHook(&SetCursorPos);
	MH_RemoveHook(&SetCursor);

	MH_RemoveHook(&EnumDisplaySettingsA);
	MH_RemoveHook(&EnumDisplaySettingsW);
	MH_RemoveHook(&EnumDisplaySettingsExA);
	MH_RemoveHook(&EnumDisplaySettingsExW);
	MH_RemoveHook(&GetSystemMetrics);

	MH_Uninitialize();
	wndhook_count = 0;
	hooks_init = FALSE;
	LeaveCriticalSection(&hook_cs);
}

void EnableDXGLHooks()
{
	EnterCriticalSection(&hook_cs);
	wndhook_count++;
	if (wndhook_count == 1)
	{
		MH_EnableHook(&SetWindowLongA);
		MH_EnableHook(&SetWindowLongW);
		MH_EnableHook(&GetWindowLongA);
		MH_EnableHook(&GetWindowLongW);
#ifdef _M_X64
		MH_EnableHook(&SetWindowLongPtrA);
		MH_EnableHook(&SetWindowLongPtrW);
		MH_EnableHook(&GetWindowLongPtrA);
		MH_EnableHook(&GetWindowLongPtrW);
#endif

		MH_EnableHook(&GetCursorPos);
		MH_EnableHook(&SetCursorPos);
		MH_EnableHook(&SetCursor);

		MH_EnableHook(&EnumDisplaySettingsA);
		MH_EnableHook(&EnumDisplaySettingsW);
		MH_EnableHook(&EnumDisplaySettingsExA);
		MH_EnableHook(&EnumDisplaySettingsExW);
		MH_EnableHook(&GetSystemMetrics);
	}
	LeaveCriticalSection(&hook_cs);
}

void DisableDXGLHooks(BOOL force)
{
	if (!wndhook_count) return;
	EnterCriticalSection(&hook_cs);
	wndhook_count--;
	if (force) wndhook_count = 0;
	if (!wndhook_count)
	{
		MH_DisableHook(&SetWindowLongA);
		MH_DisableHook(&SetWindowLongW);
		MH_DisableHook(&GetWindowLongA);
		MH_DisableHook(&GetWindowLongW);
#ifdef _M_X64
		MH_DisableHook(&SetWindowLongPtrA);
		MH_DisableHook(&SetWindowLongPtrW);
		MH_DisableHook(&GetWindowLongPtrA);
		MH_DisableHook(&GetWindowLongPtrW);
#endif

		MH_DisableHook(&GetCursorPos);
		MH_DisableHook(&SetCursorPos);
		MH_DisableHook(&SetCursor);

		MH_DisableHook(&EnumDisplaySettingsA);
		MH_DisableHook(&EnumDisplaySettingsW);
		MH_DisableHook(&EnumDisplaySettingsExA);
		MH_DisableHook(&EnumDisplaySettingsExW);
		MH_DisableHook(&GetSystemMetrics);
	}
	LeaveCriticalSection(&hook_cs);
}

void EnableWindowScaleHook(BOOL enable)
{
	windowscalehook = enable;
}

void InstallDXGLHook(HWND hWnd, LPDIRECTDRAW7 lpDD7)
{
	WNDPROC wndproc;
	HWND_HOOK *wndhook = GetWndHook(hWnd);
	if (wndhook)
	{
		if (lpDD7) wndhook->lpDD7 = lpDD7;
		return;
	}
	wndproc = (WNDPROC)_GetWindowLongPtrA(hWnd, GWLP_WNDPROC);
	SetHookWndProc(hWnd, wndproc, lpDD7, FALSE, FALSE);
	_SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)DXGLWndHookProc);
	EnableDXGLHooks();
}
void UninstallDXGLHook(HWND hWnd)
{
	HWND_HOOK *wndhook = GetWndHook(hWnd);
	if (!wndhook) return;
	_SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)wndhook->wndproc);
	SetHookWndProc(hWnd, NULL, NULL, FALSE, TRUE);
	DisableDXGLHooks(FALSE);
}
LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC parentproc;
	HWND_HOOK *wndhook;
	STYLESTRUCT *style;
	RECT r1, r2;
	POINT pt;
	LONG sizes[6];
	BOOL fixstyle = FALSE;
	LONG winstyle, exstyle;
	LPDIRECTDRAW7 lpDD7;
	int oldx, oldy;
	float mulx, muly;
	int translatex, translatey;
	LPARAM newpos;
	wndhook = GetWndHook(hWnd);
	if (!wndhook)
	{
		parentproc = nullwndproc;
		lpDD7 = NULL;
	}
	parentproc = wndhook->wndproc;
	lpDD7 = wndhook->lpDD7;
	switch (uMsg)
	{
	case WM_DESTROY:
		if (cursorclipped)
		{
			ClipCursor(NULL);
			cursorclipped = FALSE;
		}
		UninstallDXGLHook(hWnd);
		break;
	case WM_MOVE:
	case WM_KILLFOCUS:
		if (cursorclipped)
		{
			ClipCursor(NULL);
			cursorclipped = FALSE;
		}
		break;
	case WM_ACTIVATEAPP:
		if (!wParam)
		{
			if (dxglcfg.fullmode < 2 && glDirectDraw7_GetFullscreen(lpDD7))
			{
				ShowWindow(hWnd, SW_MINIMIZE);
				if (lpDD7) IDirectDraw7_SetDisplayMode(lpDD7, -1, -1, -1, -1, 0);
			}
		}
		break;
	case WM_STYLECHANGED:
		style = (STYLESTRUCT*)lParam;
		if (style->styleNew == style->styleOld) break;
		if (glDirectDraw7_GetFullscreen(lpDD7))
		{
			if (wParam == GWL_STYLE)
			{
				switch (dxglcfg.fullmode)
				{
				case 0:
					// Fix exclusive fullscreen mode
					if (lpDD7)
					{
						glDirectDraw7_GetSizes(lpDD7, sizes);
						GetWindowRect(hWnd, &r1);
						GetClientRect(hWnd, &r2);
						winstyle = GetWindowLong(hWnd, GWL_STYLE);
						exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
						if (!(winstyle & WS_POPUP)) fixstyle = TRUE;
						if (winstyle & (WS_CAPTION | WS_THICKFRAME | WS_BORDER)) fixstyle = TRUE;
						if (winstyle & (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)) fixstyle = TRUE;
						if (!((r1.left == 0) && (r1.top == 0) && (r2.right == sizes[4]) && (r2.bottom == sizes[5]))) fixstyle = TRUE;
						if (fixstyle)
						{
							SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exstyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
							SetWindowLongPtrA(hWnd, GWL_STYLE, (winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
							SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
						}
					}
					break;
				case 1:
					// Fix non-exclusive fullscreen mode
				case 4:
					// Fix borderless window mode
				case 5:
					// Fix scaled borderless window mode
					if (lpDD7)
					{
						glDirectDraw7_GetSizes(lpDD7, sizes);
						GetWindowRect(hWnd, &r1);
						GetClientRect(hWnd, &r2);
						winstyle = GetWindowLong(hWnd, GWL_STYLE);
						exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
						if (winstyle & (WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP)) fixstyle = TRUE;
						if (exstyle & (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)) fixstyle = TRUE;
						if (!((r1.left == 0) && (r1.top == 0) && (r2.right == sizes[4]) && (r2.bottom == sizes[5]))) fixstyle = TRUE;
						if (fixstyle)
						{
							SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exstyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
							SetWindowLongPtrA(hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
							SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
						}
					}
					break;
				case 2:
					// Fix non-resizable window mode
					if (lpDD7)
					{
						glDirectDraw7_GetSizes(lpDD7, sizes);
						GetClientRect(hWnd, &r2);
						winstyle = GetWindowLong(hWnd, GWL_STYLE);
						exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
						if (winstyle & (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP)) fixstyle = TRUE;
						if (!(winstyle & (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP)))) fixstyle = TRUE;
						if (!(exstyle & WS_EX_APPWINDOW)) fixstyle = TRUE;
						if (!((r2.right == sizes[4]) && (r2.bottom == sizes[5]))) fixstyle = TRUE;
						if (fixstyle)
						{
							SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exstyle | WS_EX_APPWINDOW);
							SetWindowLongPtrA(hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
							SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
						}
					}
					break;
				case 3:
					// Fix resizable window mode
					if (lpDD7)
					{
						glDirectDraw7_GetSizes(lpDD7, sizes);
						GetClientRect(hWnd, &r2);
						winstyle = GetWindowLong(hWnd, GWL_STYLE);
						exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
						if (winstyle & (WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP)) fixstyle = TRUE;
						if (!(winstyle & WS_OVERLAPPEDWINDOW)) fixstyle = TRUE;
						if (!(exstyle & WS_EX_APPWINDOW)) fixstyle = TRUE;
						if (fixstyle)
						{
							SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exstyle | WS_EX_APPWINDOW);
							SetWindowLongPtrA(hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
						}
					}
					break;
				}
			}
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam == SC_MOVE) || (wParam == SC_SIZE))
		{
			if (cursorclipped)
			{
				ClipCursor(NULL);
				cursorclipped = FALSE;
			}
		}
		if (wParam == SC_RESTORE)
		{
			if (lpDD7 && (dxglcfg.fullmode < 2)) glDirectDraw7_UnrestoreDisplayMode(lpDD7);
		}
		break;
	case WM_LBUTTONDOWN:
		if (dxglcfg.CaptureMouse)
		{
			if (!cursorclipped)
			{
				GetClipCursor(&rcOldClip);
				ClipCursor(NULL);
				GetClipCursor(&rcClip);
				if (!memcmp(&rcOldClip, &rcClip, sizeof(RECT)))
				{
					// Cursor is not clipped
					pt.x = 0;
					pt.y = 0;
					GetClientRect(hWnd, &rcWindow);
					ClientToScreen(hWnd, &pt);
					OffsetRect(&rcWindow, pt.x, pt.y);
					ClipCursor(&rcWindow);
					cursorclipped = TRUE;
				}
				else ClipCursor(&rcOldClip);
			}
		}
		if (lpDD7)
		{
			if (((dxglcfg.scaler != 0) || ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4)))
				&& glDirectDraw7_GetFullscreen(lpDD7))
			{
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				glDirectDraw7_GetSizes(lpDD7, sizes);
				mulx = (float)sizes[2] / (float)sizes[0];
				muly = (float)sizes[3] / (float)sizes[1];
				translatex = (sizes[4] - sizes[0]) / 2;
				translatey = (sizes[5] - sizes[1]) / 2;
				oldx -= translatex;
				oldy -= translatey;
				oldx = (int)((float)oldx * mulx);
				oldy = (int)((float)oldy * muly);
				if (oldx < 0) oldx = 0;
				if (oldy < 0) oldy = 0;
				if (oldx >= sizes[2]) oldx = sizes[2] - 1;
				if (oldy >= sizes[3]) oldy = sizes[3] - 1;
				newpos = oldx + (oldy << 16);
				return CallWindowProc(parentproc, hWnd, uMsg, wParam, newpos);
			}
			else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
		}
		else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (dxglcfg.CaptureMouse)
		{
			if (wParam == VK_CONTROL)
			{
				if (GetKeyState(VK_MENU)) keyctrlalt = TRUE;
			}
			if (wParam == VK_MENU)
			{
				if (GetKeyState(VK_CONTROL)) keyctrlalt = TRUE;
			}
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (dxglcfg.CaptureMouse)
		{
			if ((wParam == VK_CONTROL) || (wParam == VK_MENU))
			{
				if (keyctrlalt)
				{
					keyctrlalt = FALSE;
					if (cursorclipped)
					{
						ClipCursor(NULL);
						cursorclipped = FALSE;
					}
				}
			}
		}
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		if (lpDD7)
		{
			if (((dxglcfg.scaler != 0) || ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4)))
				&& glDirectDraw7_GetFullscreen(lpDD7))
			{
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				glDirectDraw7_GetSizes(lpDD7, sizes);
				mulx = (float)sizes[2] / (float)sizes[0];
				muly = (float)sizes[3] / (float)sizes[1];
				translatex = (sizes[4] - sizes[0]) / 2;
				translatey = (sizes[5] - sizes[1]) / 2;
				oldx -= translatex;
				oldy -= translatey;
				oldx = (int)((float)oldx * mulx);
				oldy = (int)((float)oldy * muly);
				if (oldx < 0) oldx = 0;
				if (oldy < 0) oldy = 0;
				if (oldx >= sizes[2]) oldx = sizes[2] - 1;
				if (oldy >= sizes[3]) oldy = sizes[3] - 1;
				newpos = oldx + (oldy << 16);
				return CallWindowProc(parentproc, hWnd, uMsg, wParam, newpos);
			}
			else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
		}
		else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (lpDD7)
		{
			if (((dxglcfg.scaler != 0) || ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4)))
				&& glDirectDraw7_GetFullscreen(lpDD7))
			{
				oldx = LOWORD(lParam);
				oldy = HIWORD(lParam);
				glDirectDraw7_GetSizes(lpDD7, sizes);
				mulx = (float)sizes[2] / (float)sizes[0];
				muly = (float)sizes[3] / (float)sizes[1];
				translatex = (sizes[4] - sizes[0]) / 2;
				translatey = (sizes[5] - sizes[1]) / 2;
				oldx -= translatex;
				oldy -= translatey;
				pt.x = 0;
				pt.y = 0;
				ClientToScreen(hWnd, &pt);
				oldx -= pt.x;
				oldy -= pt.y;
				oldx = (int)((float)oldx * mulx);
				oldy = (int)((float)oldy * muly);
				if (oldx < 0) oldx = 0;
				if (oldy < 0) oldy = 0;
				if (oldx >= sizes[2]) oldx = sizes[2] - 1;
				if (oldy >= sizes[3]) oldy = sizes[3] - 1;
				newpos = oldx + (oldy << 16);
				return CallWindowProc(parentproc, hWnd, uMsg, wParam, newpos);
			}
			else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
		}
		else return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
	case WM_SIZE:
		if (cursorclipped)
		{
			ClipCursor(NULL);
			cursorclipped = FALSE;
		}
		if (wParam != SIZE_MINIMIZED)
		{
			if (glDirectDraw7_GetFullscreen(lpDD7))
			{
				if (((dxglcfg.fullmode == 0) || (dxglcfg.fullmode == 1)) && lpDD7 && (dxglcfg.scaler != 0))
				{
					glDirectDraw7_GetSizes(lpDD7, sizes);
					if ((LOWORD(lParam) == sizes[4]) && (HIWORD(lParam) == sizes[5])) break;
					SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
					glDirectDraw7_SetWindowSize(lpDD7, sizes[4], sizes[5]);
				}
				if (dxglcfg.fullmode == 3)
				{
					if (lpDD7) glDirectDraw7_SetWindowSize(lpDD7, LOWORD(lParam), HIWORD(lParam));
				}
			}
		}
		break;
	}
	return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
}


LONG WINAPI HookSetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
{
	LONG oldproc;
	HWND_HOOK *wndhook;
	if (nIndex != GWL_WNDPROC) return _SetWindowLongA(hWnd, nIndex, dwNewLong);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _SetWindowLongA(hWnd, nIndex, dwNewLong);
	oldproc = (LONG)wndhook->wndproc;
	wndhook->wndproc = (WNDPROC)dwNewLong;
	return oldproc;
}
LONG WINAPI HookSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong)
{
	LONG oldproc;
	HWND_HOOK *wndhook;
	if (nIndex != GWL_WNDPROC) return _SetWindowLongW(hWnd, nIndex, dwNewLong);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _SetWindowLongW(hWnd, nIndex, dwNewLong);
	oldproc = (LONG)wndhook->wndproc;
	wndhook->wndproc = (WNDPROC)dwNewLong;
	return oldproc;
}
LONG WINAPI HookGetWindowLongA(HWND hWnd, int nIndex)
{
	HWND_HOOK *wndhook;
	if (nIndex != GWL_WNDPROC) return _GetWindowLongA(hWnd, nIndex);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _GetWindowLongA(hWnd, nIndex);
	return (LONG)wndhook->wndproc;
}
LONG WINAPI HookGetWindowLongW(HWND hWnd, int nIndex)
{
	HWND_HOOK *wndhook;
	if (nIndex != GWL_WNDPROC) return _GetWindowLongW(hWnd, nIndex);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _GetWindowLongW(hWnd, nIndex);
	return (LONG)wndhook->wndproc;
}
#ifdef _M_X64
LONG_PTR WINAPI HookSetWindowLongPtrA(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	LONG_PTR oldproc;
	HWND_HOOK *wndhook;
	if (nIndex != GWLP_WNDPROC) return _SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
	oldproc = (LONG_PTR)wndhook->wndproc;
	wndhook->wndproc = (WNDPROC)dwNewLong;
	return oldproc;
}
LONG_PTR WINAPI HookSetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong)
{
	LONG_PTR oldproc;
	HWND_HOOK *wndhook;
	if (nIndex != GWLP_WNDPROC) return _SetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _SetWindowLongPtrW(hWnd, nIndex, dwNewLong);
	oldproc = (LONG_PTR)wndhook->wndproc;
	wndhook->wndproc = (WNDPROC)dwNewLong;
	return oldproc;
}
LONG_PTR WINAPI HookGetWindowLongPtrA(HWND hWnd, int nIndex)
{
	HWND_HOOK *wndhook;
	if (nIndex != GWLP_WNDPROC) return _GetWindowLongPtrA(hWnd, nIndex);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _GetWindowLongPtrA(hWnd, nIndex);
	return (LONG_PTR)wndhook->wndproc;
}
LONG_PTR WINAPI HookGetWindowLongPtrW(HWND hWnd, int nIndex)
{
	HWND_HOOK *wndhook;
	if (nIndex != GWLP_WNDPROC) return _GetWindowLongPtrW(hWnd, nIndex);
	wndhook = GetWndHook(hWnd);
	if (!wndhook) return _GetWindowLongPtrW(hWnd, nIndex);
	return (LONG_PTR)wndhook->wndproc;
}
#endif
BOOL WINAPI HookGetCursorPos(LPPOINT point)
{
	HWND_HOOK *wndhook;
	LONG sizes[6];
	POINT pt;
	BOOL error;
	int oldx, oldy;
	float mulx, muly;
	int translatex, translatey;

	if (dxglcfg.DebugNoMouseHooks)
		return _GetCursorPos(point);

	wndhook = GetWndHook(GetActiveWindow());
	if (!wndhook) {
		return _GetCursorPos(point);
	}

	if (wndhook->lpDD7)
	{
		if (glDirectDraw7_GetFullscreen(wndhook->lpDD7))
		{
			error = _GetCursorPos(&pt);
			if (!error) return error;
			if (((dxglcfg.scaler != 0) || ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4)))
				&& glDirectDraw7_GetFullscreen(wndhook->lpDD7))
			{
				oldx = pt.x;
				oldy = pt.y;
				if ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4))
				{
					pt.x = 0;
					pt.y = 0;
					ClientToScreen(wndhook->hwnd, &pt);
					oldx -= pt.x;
					oldy -= pt.y;
				}
				glDirectDraw7_GetSizes(wndhook->lpDD7, sizes);
				mulx = (float)sizes[2] / (float)sizes[0];
				muly = (float)sizes[3] / (float)sizes[1];
				translatex = (sizes[4] - sizes[0]) / 2;
				translatey = (sizes[5] - sizes[1]) / 2;
				oldx -= translatex;
				oldy -= translatey;
				oldx = (int)((float)oldx * mulx);
				oldy = (int)((float)oldy * muly);
				if (oldx < 0) oldx = 0;
				if (oldy < 0) oldy = 0;
				if (oldx >= sizes[2]) oldx = sizes[2] - 1;
				if (oldy >= sizes[3]) oldy = sizes[3] - 1;
				point->x = oldx;
				point->y = oldy;
				return error;
			}
			else
			{
				point->x = pt.x;
				point->y = pt.y;
				return error;
			}
		}
		else
		{
			return _GetCursorPos(point);
		}
	}
}

BOOL WINAPI HookSetCursorPos(int x, int y)
{
	HWND_HOOK *wndhook;
	LONG sizes[6];
	POINT pt;
	int oldx, oldy;
	float mulx, muly;
	int translatex, translatey;

	if (dxglcfg.DebugNoMouseHooks) {
		return _SetCursorPos(x, y);
	}

	wndhook = GetWndHook(GetActiveWindow()); 
	if (!wndhook) {
		return _SetCursorPos(x, y);
	}
	if (((dxglcfg.scaler != 0) || ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4)))
		&& glDirectDraw7_GetFullscreen(wndhook->lpDD7))
	{
		oldx = x;
		oldy = y;
		glDirectDraw7_GetSizes(wndhook->lpDD7, sizes);
		mulx = (float)sizes[0] / (float)sizes[2];
		muly = (float)sizes[1] / (float)sizes[3];
		oldx = (int)((float)oldx * mulx);
		oldy = (int)((float)oldy * muly);
		translatex = (sizes[4] - sizes[0]) / 2;
		translatey = (sizes[5] - sizes[1]) / 2;
		oldx += translatex;
		oldy += translatey;
		if ((dxglcfg.fullmode >= 2) && (dxglcfg.fullmode <= 4))
		{
			pt.x = 0;
			pt.y = 0;
			ClientToScreen(wndhook->hwnd, &pt);
			oldx += pt.x;
			oldy += pt.y;
		}
		return _SetCursorPos(oldx, oldy);
	}
	else return _SetCursorPos(x, y);
}
HCURSOR WINAPI HookSetCursor(HCURSOR hCursor)
{
	static HCURSOR prevCursor = NULL;
	static BOOLEAN prevCursorSet = FALSE;

	if (!dxglcfg.HackSetCursor) {
		return _SetCursor(hCursor);
	}

	if (!prevCursorSet) {
		prevCursorSet = TRUE;
		prevCursor = GetCursor();
	}

	if (hCursor == NULL) {
		prevCursor = _SetCursor(hCursor);
	}

	return prevCursor;
}

BOOL WINAPI HookEnumDisplaySettingsA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode)
{
	BOOL ret = _EnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode);
	if (!ret) return ret;
	if (windowscalehook && (iModeNum == ENUM_CURRENT_SETTINGS))
	{
		lpDevMode->dmPelsWidth = (DWORD)((float)lpDevMode->dmPelsWidth / dxglcfg.WindowScaleX);
		lpDevMode->dmPelsHeight = (DWORD)((float)lpDevMode->dmPelsHeight / dxglcfg.WindowScaleY);
	}
	return ret;
}
BOOL WINAPI HookEnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode)
{
	BOOL ret = _EnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
	if (!ret) return ret;
	if (windowscalehook && (iModeNum == ENUM_CURRENT_SETTINGS))
	{
		lpDevMode->dmPelsWidth = (DWORD)((float)lpDevMode->dmPelsWidth / dxglcfg.WindowScaleX);
		lpDevMode->dmPelsHeight = (DWORD)((float)lpDevMode->dmPelsHeight / dxglcfg.WindowScaleY);
	}
	return ret;
}
BOOL WINAPI HookEnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode, DWORD dwFlags)
{
	BOOL ret = _EnumDisplaySettingsExA(lpszDeviceName, iModeNum, lpDevMode, dwFlags);
	if (!ret) return ret;
	if (windowscalehook && (iModeNum == ENUM_CURRENT_SETTINGS))
	{
		lpDevMode->dmPelsWidth = (DWORD)((float)lpDevMode->dmPelsWidth / dxglcfg.WindowScaleX);
		lpDevMode->dmPelsHeight = (DWORD)((float)lpDevMode->dmPelsHeight / dxglcfg.WindowScaleY);
	}
	return ret;
}
BOOL WINAPI HookEnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEW lpDevMode, DWORD dwFlags)
{
	BOOL ret = _EnumDisplaySettingsExW(lpszDeviceName, iModeNum, lpDevMode, dwFlags);
	if (!ret) return ret;
	if (windowscalehook && (iModeNum == ENUM_CURRENT_SETTINGS))
	{
		lpDevMode->dmPelsWidth = (DWORD)((float)lpDevMode->dmPelsWidth / dxglcfg.WindowScaleX);
		lpDevMode->dmPelsHeight = (DWORD)((float)lpDevMode->dmPelsHeight / dxglcfg.WindowScaleY);
	}
	return ret;
}
int WINAPI HookGetSystemMetrics(int nIndex)
{
	int ret = _GetSystemMetrics(nIndex);
	if (windowscalehook)
	{
		switch (nIndex)
		{
		default:
			return ret;
		case SM_CXSCREEN:
		case SM_CXVIRTUALSCREEN:
			return (int)((float)ret / dxglcfg.WindowScaleX);
		case SM_CYSCREEN:
		case SM_CYVIRTUALSCREEN:
			return (int)((float)ret / dxglcfg.WindowScaleY);
		}
	}
	else return ret;
}
