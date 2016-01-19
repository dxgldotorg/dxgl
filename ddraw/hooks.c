// DXGL
// Copyright (C) 2014-2015 William Feely

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
#include "../minhook-1.3/include/MinHook.h"

// temporary references to C++ C-linked stuff
void glDirectDraw7_UnrestoreDisplayMode(LPDIRECTDRAW7 lpDD7);
void glDirectDraw7_SetWindowSize(LPDIRECTDRAW7 lpDD7, DWORD dwWidth, DWORD dwHeight);
void glDirectDraw7_GetSizes(LPDIRECTDRAW7 lpDD7, LONG *sizes);
extern DXGLCFG dxglcfg;

const TCHAR *wndprop = _T("DXGLWndProc");
const TCHAR *wndpropdd7 = _T("DXGLWndDD7");
static HWND_HOOK *hwndhooks = NULL;
static int hwndhook_count = 0;
static int hwndhook_max = 0;
CRITICAL_SECTION hook_cs = { NULL, 0, 0, NULL, NULL, 0 };
static BOOL hooks_init = FALSE;

LONG(WINAPI *_SetWindowLongA)(HWND hWnd, int nIndex, LONG dwNewLong) = NULL;
LONG(WINAPI *_SetWindowLongW)(HWND hWnd, int nIndex, LONG dwNewLong) = NULL;
LONG(WINAPI *_GetWindowLongA)(HWND hWnd, int nIndex) = NULL;
LONG(WINAPI *_GetWindowLongW)(HWND hWnd, int nIndex) = NULL;
#ifdef _M_X64
LONG_PTR(WINAPI *_SetWindowLongPtrA)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR(WINAPI *_SetWindowLongPtrW)(HWND hWnd, int nIndex, LONG_PTR dwNewLong);
LONG_PTR(WINAPI *_GetWindowLongPtrA)(HWND hWnd, int nIndex) = NULL;
LONG_PTR(WINAPI *_GetWindowLongPtrW)(HWND hWnd, int nIndex) = NULL;
#else
#define _SetWindowLongPtrA   _SetWindowLongA
#define _SetWindowLongPtrW   _SetWindowLongW
#define _GetWindowLongPtrA   _GetWindowLongA
#define _GetWindowLongPtrW   _GetWindowLongW
#endif
UINT wndhook_count = 0;

int hwndhookcmp(const HWND_HOOK *key, const HWND_HOOK *cmp)
{
	if (!cmp->hwnd) return 1; // Put blanks at end for cleanup
	if (key->hwnd < cmp->hwnd) return -1;
	if (key->hwnd == cmp->hwnd) return 0;
	if (key->hwnd > cmp->hwnd) return 1;
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
BOOL IsCallerOpenGL(void *returnaddress)
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
	MH_CreateHook(&SetWindowLongA, HookSetWindowLongA, &_SetWindowLongA);
	MH_CreateHook(&SetWindowLongW, HookSetWindowLongW, &_SetWindowLongW);
	MH_CreateHook(&GetWindowLongA, HookGetWindowLongA, &_GetWindowLongA);
	MH_CreateHook(&GetWindowLongW, HookGetWindowLongW, &_GetWindowLongW);
#ifdef _M_X64
	MH_CreateHook(&SetWindowLongPtrA, HookSetWindowLongPtrA, &_SetWindowLongPtrA);
	MH_CreateHook(&SetWindowLongPtrW, HookSetWindowLongPtrW, &_SetWindowLongPtrW);
	MH_CreateHook(&GetWindowLongPtrA, HookGetWindowLongPtrA, &_GetWindowLongPtrA);
	MH_CreateHook(&GetWindowLongPtrW, HookGetWindowLongPtrW, &_GetWindowLongPtrW);
#endif
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
	MH_Uninitialize();
	wndhook_count = 0;
	hooks_init = FALSE;
	LeaveCriticalSection(&hook_cs);
}

void EnableWindowLongHooks()
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
	}
	LeaveCriticalSection(&hook_cs);
}

void DisableWindowLongHooks(BOOL force)
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
	}
	LeaveCriticalSection(&hook_cs);
}

void InstallDXGLFullscreenHook(HWND hWnd, LPDIRECTDRAW7 lpDD7)
{
	WNDPROC wndproc;
	HWND_HOOK *wndhook = GetWndHook(hWnd);
	if (wndhook)
	{
		if (lpDD7) wndhook->lpDD7 = lpDD7;
		return;
	}
	wndproc = _GetWindowLongPtrA(hWnd, GWLP_WNDPROC);
	SetHookWndProc(hWnd, wndproc, lpDD7, FALSE, FALSE);
	_SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)DXGLWndHookProc);
	EnableWindowLongHooks();
}
void UninstallDXGLFullscreenHook(HWND hWnd)
{
	HWND_HOOK *wndhook = GetWndHook(hWnd);
	if (!wndhook) return;
	_SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR)wndhook->wndproc);
	SetHookWndProc(hWnd, NULL, NULL, FALSE, TRUE);
	DisableWindowLongHooks(FALSE);
}
LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC parentproc;
	HWND_HOOK *wndhook;
	STYLESTRUCT *style;
	RECT r1, r2;
	LONG sizes[6];
	BOOL fixstyle = FALSE;
	LONG winstyle, exstyle;
	LPDIRECTDRAW7 lpDD7;
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
		UninstallDXGLFullscreenHook(hWnd);
		break;
	case WM_ACTIVATEAPP:
		if (!wParam)
		{
			if (dxglcfg.fullmode < 2)
			{
				ShowWindow(hWnd, SW_MINIMIZE);
				if (lpDD7) IDirectDraw7_SetDisplayMode(lpDD7, -1, -1, -1, -1, 0);
			}
		}
		break;
	case WM_STYLECHANGED:
		style = lParam;
		if (style->styleNew == style->styleOld) break;
		if (wParam == GWL_STYLE)
		{
			switch (dxglcfg.fullmode)
			{
			case 0:
				// Fix fullscreen mode
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
						SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
					}
				}
				break;
			case 1:
				// Fix borderless mode
				if (lpDD7)
				{
					glDirectDraw7_GetSizes(lpDD7, sizes);
					GetWindowRect(hWnd, &r1);
					GetClientRect(hWnd, &r2);
					winstyle = GetWindowLong(hWnd, GWL_STYLE);
					exstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
					if (winstyle & (WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP)) fixstyle = TRUE;
					if (winstyle & (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)) fixstyle = TRUE;
					if (!((r1.left == 0) && (r1.top == 0) && (r2.right == sizes[4]) && (r2.bottom == sizes[5]))) fixstyle = TRUE;
					if (fixstyle)
					{
						SetWindowLongPtrA(hWnd, GWL_EXSTYLE, exstyle & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
						SetWindowLongPtrA(hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
						SetWindowPos(hWnd, NULL, 0, 0, sizes[4], sizes[5], SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
					}
				}
				break;
				break;
			case 2:
				// Fix non-resizable window mode
				break;
			case 3:
				// Fix resizable window mode
				break;
			}
		}
		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_RESTORE)
		{
			if (lpDD7 && (dxglcfg.fullmode < 2)) glDirectDraw7_UnrestoreDisplayMode(lpDD7);
		}
		break;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			if (dxglcfg.fullmode == 3)
			{
				if (lpDD7) glDirectDraw7_SetWindowSize(lpDD7, LOWORD(lParam), HIWORD(lParam));
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
