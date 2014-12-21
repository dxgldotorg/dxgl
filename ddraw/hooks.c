// DXGL
// Copyright (C) 2014 William Feely

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

const TCHAR *wndprop = _T("DXGLWndProc");

void InstallDXGLFullscreenHook(HWND hWnd)
{
	HANDLE wndproc = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
	if (GetProp(hWnd, wndprop)) return;
	SetProp(hWnd, wndprop, wndproc);
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)DXGLWndHookProc);
}
void UninstallDXGLFullscreenHook(HWND hWnd)
{
	if (!GetProp(hWnd, wndprop)) return;
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)GetProp(hWnd, wndprop));
	RemoveProp(hWnd, wndprop);
}
LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC parentproc = (WNDPROC)GetProp(hWnd, wndprop);
	switch (uMsg)
	{
	case WM_DESTROY:
		UninstallDXGLFullscreenHook(hWnd);
		break;
	case WM_ACTIVATEAPP:
		if (!wParam) ShowWindow(hWnd, SW_MINIMIZE);
		break;
	}
	return CallWindowProc(parentproc, hWnd, uMsg, wParam, lParam);
}
