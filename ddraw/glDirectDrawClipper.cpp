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

#include "common.h"
#include "glDirectDrawClipper.h"

glDirectDrawClipper::glDirectDrawClipper()
{
	initialized = false;
	refcount = 1;
}

glDirectDrawClipper::glDirectDrawClipper(DWORD dwFlags, glDirectDraw7 *parent)
{
	initialized = false;
	refcount = 1;
	Initialize((LPDIRECTDRAW)parent,dwFlags);
}
glDirectDrawClipper::~glDirectDrawClipper()
{
}
HRESULT WINAPI glDirectDrawClipper::QueryInterface(REFIID riid, LPVOID* obp)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IDirectDrawClipper)
	{
		*obp = this;
		this->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDrawClipper::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDrawClipper::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDrawClipper::GetClipList(LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("IDirectDrawClipper::GetClipList: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::GetHWnd(HWND FAR *lphWnd)
{
	if(!this) return DDERR_INVALIDPARAMS;
	*lphWnd = hWnd;
	if(!hWnd) return DDERR_INVALIDOBJECT;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(initialized) return DDERR_ALREADYINITIALIZED;
	glDD7 = (glDirectDraw7*)lpDD;
	if(glDD7) hasparent = true;
	else hasparent = false;
	hWnd = NULL;
	refcount = 1;
	initialized = true;
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::IsClipListChanged(BOOL FAR *lpbChanged)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("IDirectDrawClipper::IsClipListChanged: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("IDirectDrawClipper::SetClipList: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
	if(!this) return DDERR_INVALIDPARAMS;
	this->hWnd = hWnd;
	return DD_OK;
}
