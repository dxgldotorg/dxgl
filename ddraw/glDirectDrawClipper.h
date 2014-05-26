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

#pragma once
#ifndef _GLDIRECTDRAWCLIPPER_H
#define _GLDIRECTDRAWCLIPPER_H

class glDirectDraw7;

struct glDirectDrawClipperVtbl;

typedef struct glDirectDrawClipper
{
	struct glDirectDrawClipperVtbl *lpVtbl;
	ULONG refcount;
	glDirectDraw7 *glDD7;
	bool hasparent;
	bool initialized;
	HWND hWnd;
	RECT *cliplist;
	BltVertex *vertices;
	WORD *indices;
	int clipsize;
	int maxsize;
	bool hascliplist;
	bool dirty;
} glDirectDrawClipper;

typedef struct glDirectDrawClipperVtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDrawClipper *This, REFIID riid, LPVOID* obp);
	ULONG(WINAPI *AddRef)(glDirectDrawClipper *This);
	ULONG(WINAPI *Release)(glDirectDrawClipper *This);
	// ddraw api
	HRESULT(WINAPI *GetClipList)(glDirectDrawClipper *This, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize);
	HRESULT(WINAPI *GetHWnd)(glDirectDrawClipper *This, HWND FAR *lphWnd);
	HRESULT(WINAPI *Initialize)(glDirectDrawClipper *This, LPDIRECTDRAW lpDD, DWORD dwFlags);
	HRESULT(WINAPI *IsClipListChanged)(glDirectDrawClipper *This, BOOL FAR *lpbChanged);
	HRESULT(WINAPI *SetClipList)(glDirectDrawClipper *This, LPRGNDATA lpClipList, DWORD dwFlags);
	HRESULT(WINAPI *SetHWnd)(glDirectDrawClipper *This, DWORD dwFlags, HWND hWnd);
} glDirectDrawClipperVtbl;

HRESULT glDirectDrawClipper_CreateNoInit(LPDIRECTDRAWCLIPPER *lplpDDClipper);
HRESULT glDirectDrawClipper_Create(DWORD dwFlags, glDirectDraw7 *parent, LPDIRECTDRAWCLIPPER *lplpDDClipper);
// ddraw api
HRESULT WINAPI glDirectDrawClipper_QueryInterface(glDirectDrawClipper *This, REFIID riid, LPVOID* obp);
ULONG WINAPI glDirectDrawClipper_AddRef(glDirectDrawClipper *This);
ULONG WINAPI glDirectDrawClipper_Release(glDirectDrawClipper *This);
HRESULT WINAPI glDirectDrawClipper_GetClipList(glDirectDrawClipper *This, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize);
HRESULT WINAPI glDirectDrawClipper_GetHWnd(glDirectDrawClipper *This, HWND FAR *lphWnd);
HRESULT WINAPI glDirectDrawClipper_Initialize(glDirectDrawClipper *This, LPDIRECTDRAW lpDD, DWORD dwFlags);
HRESULT WINAPI glDirectDrawClipper_IsClipListChanged(glDirectDrawClipper *This, BOOL FAR *lpbChanged);
HRESULT WINAPI glDirectDrawClipper_SetClipList(glDirectDrawClipper *This, LPRGNDATA lpClipList, DWORD dwFlags);
HRESULT WINAPI glDirectDrawClipper_SetHWnd(glDirectDrawClipper *This, DWORD dwFlags, HWND hWnd);
#endif //_GLDIRECTDRAWCLIPPER_H