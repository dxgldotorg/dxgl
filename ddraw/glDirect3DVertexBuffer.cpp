// DXGL
// Copyright (C) 2011-2021 William Feely

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
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirect3DVertexBuffer.h"
#include "ddraw.h"

glDirect3DVertexBuffer7Vtbl glDirect3DVertexBuffer7_iface =
{
	glDirect3DVertexBuffer7_QueryInterface,
	glDirect3DVertexBuffer7_AddRef,
	glDirect3DVertexBuffer7_Release,
	glDirect3DVertexBuffer7_Lock,
	glDirect3DVertexBuffer7_Unlock,
	glDirect3DVertexBuffer7_ProcessVertices,
	glDirect3DVertexBuffer7_GetVertexBufferDesc,
	glDirect3DVertexBuffer7_Optimize,
	glDirect3DVertexBuffer7_ProcessVerticesStrided
};

HRESULT glDirect3DVertexBuffer7_Create(glDirect3D7 *glD3D7, D3DVERTEXBUFFERDESC desc, DWORD flags, glDirect3DVertexBuffer7 **buffer)
{
	TRACE_ENTER(4, 14, glD3D7, 14, &desc, 9, flags, 14, buffer);
	glDirect3DVertexBuffer7 *This = (glDirect3DVertexBuffer7*)malloc(sizeof(glDirect3DVertexBuffer7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DVertexBuffer7_iface;
	This->glD3D7 = glD3D7;
	This->glD3D7->AddRef();
	This->refcount = 1;
	This->vbdesc = desc;
	This->flags = flags;
	This->version = 7;
	*buffer = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DVertexBuffer1_Create(glDirect3D3 *glD3D3, D3DVERTEXBUFFERDESC desc, DWORD flags, glDirect3DVertexBuffer7 **buffer)
{
	TRACE_ENTER(4, 14, glD3D3, 14, &desc, 9, flags, 14, buffer);
	glDirect3DVertexBuffer7 *This = (glDirect3DVertexBuffer7*)malloc(sizeof(glDirect3DVertexBuffer7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DVertexBuffer7_iface;
	This->glD3D7 = glD3D3->glD3D7;
	This->glD3D7->AddRef();
	This->refcount = 1;
	This->vbdesc = desc;
	This->flags = flags;
	This->version = 1;
	*buffer = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

void glDirect3DVertexBuffer7_Destroy(glDirect3DVertexBuffer7 *This)
{
	TRACE_ENTER(1,14,This);
	This->glD3D7->Release();
	free(This);
	TRACE_EXIT(0,0);
}

ULONG WINAPI glDirect3DVertexBuffer7_AddRef(glDirect3DVertexBuffer7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement(&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}
ULONG WINAPI glDirect3DVertexBuffer7_Release(glDirect3DVertexBuffer7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement(&This->refcount);
	ret = This->refcount;
	if(This->refcount == 0) glDirect3DVertexBuffer7_Destroy(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DVertexBuffer7_QueryInterface(glDirect3DVertexBuffer7 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		glDirect3DVertexBuffer7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if ((riid == IID_IDirect3DVertexBuffer7) && (This->version == 7))
	{
		glDirect3DVertexBuffer7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj", 14, *ppvObj);
		TRACE_EXIT(23, D3D_OK);
		return D3D_OK;
	}
	if ((riid == IID_IDirect3DVertexBuffer) && (This->version == 1))
	{
		glDirect3DVertexBuffer7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj", 14, *ppvObj);
		TRACE_EXIT(23, D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

HRESULT WINAPI glDirect3DVertexBuffer7_GetVertexBufferDesc(glDirect3DVertexBuffer7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc)
{
	TRACE_ENTER(2,14,This,14,lpVBDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpVBDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lpVBDesc = This->vbdesc;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DVertexBuffer7_Lock(glDirect3DVertexBuffer7 *This, DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize)
{
	TRACE_ENTER(4,14,This,9,dwFlags,14,lplpData,14,lpdwSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7_Lock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT WINAPI glDirect3DVertexBuffer7_Optimize(glDirect3DVertexBuffer7 *This, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	glDirect3DDevice7 *dev7;
	TRACE_ENTER(3,14,This,14,lpD3DDevice,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->version == 7) dev7 = (glDirect3DDevice7*)lpD3DDevice;
	else dev7 = ((glDirect3DDevice3*)lpD3DDevice)->glD3DDev7;
	FIXME("glDirect3DVertexBuffer7_Optimize: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7_ProcessVertices(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount, 
	LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	glDirect3DDevice7 *dev7;
	TRACE_ENTER(8,14,This,9,dwVertexOp,8,dwDestIndex,8,dwCount,14,lpSrcBuffer,8,dwSrcIndex,14,lpD3DDevice,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (This->version == 7) dev7 = (glDirect3DDevice7*)lpD3DDevice;
	else dev7 = ((glDirect3DDevice3*)lpD3DDevice)->glD3DDev7;
	FIXME("glDirect3DVertexBuffer7_ProcessVertices: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7_ProcessVerticesStrided(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,9,dwVertexOp,8,dwDestIndex,8,dwCount,14,lpVertexArray,8,dwSrcIndex,14,lpD3DDevice,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7_ProcessVerticesStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DVertexBuffer7_Unlock(glDirect3DVertexBuffer7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DVertexBuffer7_Unlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}