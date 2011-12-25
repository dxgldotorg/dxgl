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

class glDirectDrawClipper : public IDirectDrawClipper
{
public:
	glDirectDrawClipper();
	glDirectDrawClipper(DWORD dwFlags, glDirectDraw7 *parent);
	~glDirectDrawClipper();
	// ddraw api
	HRESULT WINAPI QueryInterface(REFIID riid, LPVOID* obp);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetClipList(LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize);
	HRESULT WINAPI GetHWnd(HWND FAR *lphWnd);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags);
	HRESULT WINAPI IsClipListChanged(BOOL FAR *lpbChanged);
	HRESULT WINAPI SetClipList(LPRGNDATA lpClipList, DWORD dwFlags);
	HRESULT WINAPI SetHWnd(DWORD dwFlags, HWND hWnd);
private:
	ULONG refcount;
	glDirectDraw7 *glDD7;
	bool hasparent;
	bool initialized;
	HWND hWnd;
};
#endif //_GLDIRECTDRAWCLIPPER_H