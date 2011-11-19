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

#include "stdafx.h"
#include "glDirectDrawClipper.h"

glDirectDrawClipper::glDirectDrawClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter, glDirectDraw7 *parent)
{
	glDD7 = parent;
	if(glDD7) hasparent = true;
	else hasparent = false;
	hWnd = NULL;
	refcount = 1;
}
glDirectDrawClipper::~glDirectDrawClipper()
{
}
HRESULT WINAPI glDirectDrawClipper::QueryInterface(REFIID riid, LPVOID* obp)
{
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDrawClipper::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawClipper::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawClipper::GetClipList(LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
	FIXME("IDirectDrawClipper::GetClipList: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawClipper::GetHWnd(HWND FAR *lphWnd)
{
	*lphWnd = hWnd;
	if(!hWnd) return DDERR_INVALIDOBJECT;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
	FIXME("IDirectDrawClipper::Initialize: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawClipper::IsClipListChanged(BOOL FAR *lpbChanged)
{
	FIXME("IDirectDrawClipper::IsClipListChanged: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
	FIXME("IDirectDrawClipper::SetClipList: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
	this->hWnd = hWnd;
	return DD_OK;
}