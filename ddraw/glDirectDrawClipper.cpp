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
#include "texture.h"
#include "glutil.h"
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
	if (cliplist) free(cliplist);
	if (vertices) free(vertices);
	if (indices) free(indices);
	if(glDD7) glDD7->DeleteClipper(this);
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI glDirectDrawClipper::QueryInterface(REFIID riid, LPVOID* obp)
{
	TRACE_ENTER(3,14,this,24,&riid,14,obp);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!obp) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
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
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glDirectDrawClipper::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
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
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDrawClipper::GetClipList: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::GetHWnd(HWND FAR *lphWnd)
{
	TRACE_ENTER(2,14,this,14,lphWnd);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!hWnd) TRACE_RET(HRESULT,23,DDERR_NOHWND);
	*lphWnd = hWnd;
	TRACE_VAR("*lphWnd",13,*lphWnd);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpDD,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(initialized) TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
	glDD7 = (glDirectDraw7*)lpDD;
	if(glDD7) hasparent = true;
	else hasparent = false;
	hWnd = NULL;
	cliplist = NULL;
	vertices = NULL;
	indices = NULL;
	hascliplist = false;
	maxsize = clipsize = 0;
	refcount = 1;
	initialized = true;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::IsClipListChanged(BOOL FAR *lpbChanged)
{
	TRACE_ENTER(2,14,this,14,lpbChanged);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("IDirectDrawClipper::IsClipListChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDrawClipper::SetClipList(LPRGNDATA lpClipList, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpClipList,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(hWnd) TRACE_RET(HRESULT,23,DDERR_CLIPPERISUSINGHWND);
	bool memfail;
	if(lpClipList)
	{
		if(lpClipList->rdh.dwSize != sizeof(RGNDATAHEADER)) TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(lpClipList->rdh.iType != RDH_RECTANGLES) TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(lpClipList->rdh.nRgnSize != lpClipList->rdh.nCount * sizeof(RECT))
			TRACE_RET(HRESULT,23,DDERR_INVALIDCLIPLIST);
		if(!cliplist)
		{
			memfail = false;
			maxsize = lpClipList->rdh.nCount;
			cliplist = (RECT*)malloc(maxsize*sizeof(RECT));
			if(!cliplist) memfail = true;
			if(!memfail) vertices = (BltVertex*)malloc(maxsize*4*sizeof(BltVertex));
			if(!vertices) memfail = true;
			if(!memfail) indices = (WORD*)malloc(maxsize*6*sizeof(WORD));
			if(!indices) memfail = true;
			if(memfail)
			{
				if(vertices)
				{
					free(vertices);
					vertices = NULL;
				}
				if(cliplist)
				{
					free(cliplist);
					cliplist = NULL;
				}
				maxsize = 0;
				TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
			}
		}
		if(lpClipList->rdh.nCount > maxsize)
		{
			memfail = false;
			RECT *newcliplist = NULL;
			BltVertex *newvertices = NULL;
			WORD *newindices = NULL;
			newcliplist = (RECT*)realloc(cliplist,lpClipList->rdh.nCount*sizeof(RECT));
			if(!newcliplist) memfail = true;
			else cliplist = newcliplist;
			if(!memfail) newvertices = (BltVertex*)realloc(vertices,lpClipList->rdh.nCount*4*sizeof(BltVertex));
			if(!newvertices) memfail = true;
			else vertices = newvertices;
			if(!memfail) newindices = (WORD*)realloc(indices,lpClipList->rdh.nCount*6*sizeof(WORD));
			if(!newindices) memfail = true;
			else indices = newindices;
			if(memfail) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
			maxsize = lpClipList->rdh.nCount;
		}
		clipsize = lpClipList->rdh.nCount;
		memcpy(cliplist,lpClipList->Buffer,lpClipList->rdh.nCount*sizeof(RECT));
		for(int i = 0; i < lpClipList->rdh.nCount; i++)
		{
			vertices[(i*4)+1].y = vertices[(i*4)+3].y = cliplist[i].left;
			vertices[i*4].x = vertices[(i*4)+2].x = cliplist[i].right;
			vertices[i*4].y = vertices[(i*4)+1].y = cliplist[i].top;
			vertices[(i*4)+2].y = vertices[(i*4)+3].y = cliplist[i].bottom;
			// 0 1 2 2 1 3
			indices[i*6] = i*4;
			indices[(i*6)+1] = indices[(i*6)+4] = (i*4)+1;
			indices[(i*6)+2] = indices[(i*6)+3] = (i*4)+2;
			indices[(i*6)+5] = (i*4)+3;
		}
		for(int i = 0; i < (4*lpClipList->rdh.nCount); i++)
		{
			vertices[i].r = 255;
			vertices[i].g = vertices[i].b = vertices[i].a = 0;
			vertices[i].s = vertices[i].t = 0.0f;
		}
	}
	else clipsize = 0;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
HRESULT WINAPI glDirectDrawClipper::SetHWnd(DWORD dwFlags, HWND hWnd)
{
	TRACE_ENTER(3,14,this,9,dwFlags,13,hWnd);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	this->hWnd = hWnd;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
