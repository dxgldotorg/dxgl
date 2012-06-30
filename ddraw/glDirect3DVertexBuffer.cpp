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
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirect3DVertexBuffer.h"

glDirect3DVertexBuffer7::glDirect3DVertexBuffer7(glDirect3D7 *glD3D7, D3DVERTEXBUFFERDESC desc, DWORD flags)
{
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	refcount = 1;
	vbdesc = desc;
	this->flags = flags;
}

glDirect3DVertexBuffer7::~glDirect3DVertexBuffer7()
{
	glD3D7->Release();
}

ULONG WINAPI glDirect3DVertexBuffer7::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DVertexBuffer7::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DVertexBuffer7::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return E_NOINTERFACE;
}

HRESULT WINAPI glDirect3DVertexBuffer7::GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpVBDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpVBDesc) return DDERR_INVALIDPARAMS;
	*lpVBDesc = vbdesc;
	return D3D_OK;
}

HRESULT WINAPI glDirect3DVertexBuffer7::Lock(DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DVertexBuffer7::Lock: stub");
	return DDERR_GENERIC;
}

HRESULT WINAPI glDirect3DVertexBuffer7::Optimize(LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DVertexBuffer7::Optimize: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::ProcessVertices(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount, 
	LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DVertexBuffer7::ProcessVertices: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::ProcessVerticesStrided(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DVertexBuffer7::ProcessVerticesStrided: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7::Unlock()
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DVertexBuffer7::Unlock: stub");
	return DDERR_GENERIC;
}