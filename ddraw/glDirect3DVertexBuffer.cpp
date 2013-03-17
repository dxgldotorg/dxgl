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
#include "include/d3d.h"
#include "include/d3dtypes.h"
#include "texture.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirect3DVertexBuffer.h"

glDirect3DVertexBuffer7::glDirect3DVertexBuffer7(glDirect3D7 *glD3D7, D3DVERTEXBUFFERDESC desc, DWORD flags)
{
	TRACE_ENTER(4,14,this,14,glD3D7,14,&desc,9,flags);
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	refcount = 1;
	vbdesc = desc;
	this->flags = flags;
	TRACE_EXIT(-1,0);
}

glDirect3DVertexBuffer7::~glDirect3DVertexBuffer7()
{
	TRACE_ENTER(1,14,this);
	glD3D7->Release();
	TRACE_EXIT(-1,0);
}

ULONG WINAPI glDirect3DVertexBuffer7::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glDirect3DVertexBuffer7::Release()
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

HRESULT WINAPI glDirect3DVertexBuffer7::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

HRESULT WINAPI glDirect3DVertexBuffer7::GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpVBDesc)
{
	TRACE_ENTER(2,14,this,14,lpVBDesc);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!lpVBDesc) TRACE_RET(23,DDERR_INVALIDPARAMS);
	*lpVBDesc = vbdesc;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DVertexBuffer7::Lock(DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize)
{
	TRACE_ENTER(4,14,this,9,dwFlags,14,lplpData,14,lpdwSize);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7::Lock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT WINAPI glDirect3DVertexBuffer7::Optimize(LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpD3DDevice,9,dwFlags);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7::Optimize: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::ProcessVertices(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount, 
	LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,9,dwVertexOp,8,dwDestIndex,8,dwCount,14,lpSrcBuffer,8,dwSrcIndex,14,lpD3DDevice,9,dwFlags);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7::ProcessVertices: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::ProcessVerticesStrided(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,9,dwVertexOp,8,dwDestIndex,8,dwCount,14,lpVertexArray,8,dwSrcIndex,14,lpD3DDevice,9,dwFlags);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7::ProcessVerticesStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::Unlock()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7::Unlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}