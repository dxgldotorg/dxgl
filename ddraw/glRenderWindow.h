// DXGL
// Copyright (C) 2012-2021 William Feely

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
#ifndef _GLRENDERWINDOW_H
#define _GLRENDERWINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct glDirectDraw7;

void WaitForObjectAndMessages(HANDLE object);

typedef struct glRenderWindow
{
	HWND hWnd;
	HWND hParentWnd;
	HANDLE hThread;
	HANDLE ReadyEvent;
	int width;
	int height;
	BOOL fullscreen;
	BOOL dead;
	BOOL device;
	glDirectDraw7 *ddInterface;
} glRenderWindow;

HRESULT glRenderWindow_Create(int width, int height, BOOL fullscreen,
	HWND parent, glDirectDraw7 *glDD7, BOOL devwnd, glRenderWindow **renderwnd);
void glRenderWindow_Delete(glRenderWindow *This);
//void glRenderWindow_resize(glRenderWindow *This, int width, int height);
LRESULT glRenderWindow_WndProc(glRenderWindow *This, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI glRenderWindow_ThreadEntry(void *entry);
DWORD glRenderWindow__Entry(glRenderWindow *This);

#ifdef __cplusplus
}
#endif

#endif //_GLRENDERWINDOW_H
