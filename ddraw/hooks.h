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
#pragma once
#ifndef __HOOKS_H
#define __HOOKS_H
#ifdef __cplusplus
extern "C" {
#endif

LRESULT CALLBACK DXGLWndHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InstallDXGLFullscreenHook(HWND hWnd);
void UninstallDXGLFullscreenHook(HWND hWnd);

#ifdef __cplusplus
}
#endif

#endif //__HOOKS_H