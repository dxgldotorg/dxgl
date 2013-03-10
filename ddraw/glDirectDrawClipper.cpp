// DXGL
// Copyright (C) 2011-2012 William Feely

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
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"

glDirectDrawClipper::glDirectDrawClipper()
{
	TRACE_ENTER(1,14,this);
	initialized = false;
	refcount = 1;
	TRACE_EXIT(-1,0);
}

glDirectDrawClipper::glDirectDrawClipper(DWORD dwFlags, glDirectDraw7 *parent)
{
	TRACE_ENTER(3,14,this,9,dwFlags,14,parent);
	initialized = false;
	refcount = 1;
	Initialize((LPDIRECTDRAW)parent,dwFlags);
	TRACE_EXIT(-1,0);
}
glDirectDrawClipper::~glDirectDrawClipper()
{
	TRACE_ENTER(1,14,this);
	if(glDD7) glDD7->DeleteClipper(this);
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI glDirectDrawClipper::QueryInterface(REFIID riid, LPVOID* obp)
{
	TRACE_ENTER(3,14,this,24,&riid,14,obp);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!obp) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*obp = this;
		TRACE_VAR("*obp",14,*obp);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	if(riid == IID_IDirectDrawClipper)
	{
		*obp = this;
		this->AddRef();
		TRACE_VAR("*obp",14,*obp);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDrawClipper::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glDirectDrawClipper::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}
HRESULT WINAPI glDirectDrawClipper::GetClipList(LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize)
{
	TRACE_ENTER(4,14,this,26,lpRect,14,lpClipList,14,lpdwSize);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDrawClipper::GetClipList: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::GetHWnd(HWND FAR *lphWnd)
{
	TRACE_ENTER(2,14,this,14,lphWnd);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!hWnd) TRACE_RET(23,DDERR_NOHWND);
	*lphWnd = hWnd;
	TRACE_VAR("*lphWnd",13,*lphWnd);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDD,9,dwFlags);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(initialized) TRACE_RET(23,DDERR_ALREADYINITIALIZED);
	glDD7 = (glDirectDraw7*)lpDD;
	if(glDD7) hasparent = true;
	else hasparent = false;
	hWnd = NULL;
	refcount = 1;
	initialized = true;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::IsClipListChanged(BOOL FAR *lpbChanged)
{
	TRACE_ENTER(2,14,this,14,lpbChanged);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDrawClipper::IsClipListChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpClipList,9,dwFlags);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDrawClipper::SetClipList: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
	TRACE_ENTER(3,14,this,9,dwFlags,13,hWnd);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	this->hWnd = hWnd;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
