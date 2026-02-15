// DXGL
// Copyright (C) 2013-2026 William Feely

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
#include "glDirect3DExecuteBuffer.h"
#include "ddraw.h"

glDirect3DExecuteBufferVtbl glDirect3DExecuteBuffer_iface =
{
	glDirect3DExecuteBuffer_QueryInterface,
	glDirect3DExecuteBuffer_AddRef,
	glDirect3DExecuteBuffer_Release,
	glDirect3DExecuteBuffer_Initialize,
	glDirect3DExecuteBuffer_Lock,
	glDirect3DExecuteBuffer_Unlock,
	glDirect3DExecuteBuffer_SetExecuteData,
	glDirect3DExecuteBuffer_GetExecuteData,
	glDirect3DExecuteBuffer_Validate,
	glDirect3DExecuteBuffer_Optimize
};

HRESULT glDirect3DExecuteBuffer_Create(LPD3DEXECUTEBUFFERDESC lpDesc, glDirect3DExecuteBuffer **buffer)
{
	TRACE_ENTER(2, 14, lpDesc, 14, buffer);
	glDirect3DExecuteBuffer *This = (glDirect3DExecuteBuffer*)malloc(sizeof(glDirect3DExecuteBuffer));
	if (!This) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	*buffer = This;
	This->lpVtbl = &glDirect3DExecuteBuffer_iface;
	This->refcount = 1;
	This->locked = FALSE;
	This->inuse = FALSE;
	This->data = NULL;
	This->desc = *lpDesc;
	if(!(This->desc.dwFlags & D3DDEB_CAPS))
	{
		This->desc.dwFlags |= D3DDEB_CAPS;
		This->desc.dwCaps = D3DDEBCAPS_MEM;
	}
	ZeroMemory(&This->datadesc,sizeof(D3DEXECUTEDATA));
	This->datadesc.dwSize = sizeof(D3DEXECUTEDATA);
	TRACE_EXIT(23, D3D_OK);
	return DD_OK;
}
void glDirect3DExecuteBuffer_Destroy(glDirect3DExecuteBuffer *This)
{
	TRACE_ENTER(1,14,This);
	if(This->data) free(This->data);
	free(This);
	TRACE_EXIT(0,0);
}

HRESULT WINAPI glDirect3DExecuteBuffer_QueryInterface(glDirect3DExecuteBuffer *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if (!ppvObj) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*ppvObj = NULL;
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!&riid) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DExecuteBuffer))
	{
		glDirect3DExecuteBuffer_AddRef(This);
		*ppvObj = This;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DExecuteBuffer_AddRef(glDirect3DExecuteBuffer *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}

ULONG WINAPI glDirect3DExecuteBuffer_Release(glDirect3DExecuteBuffer *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement((LONG*)&This->refcount);
	ret = This->refcount;
	if(This->refcount == 0) glDirect3DExecuteBuffer_Destroy(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DExecuteBuffer_GetExecuteData(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->locked) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_LOCKED);
	if(lpData->dwSize < sizeof(D3DEXECUTEDATA)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpData,&This->datadesc,sizeof(D3DEXECUTEDATA));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer_Initialize(glDirect3DExecuteBuffer *This, LPDIRECT3DDEVICE lpDirect3DDevice,
	LPD3DEXECUTEBUFFERDESC lpDesc)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice,14,lpDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DExecuteBuffer_Lock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEBUFFERDESC lpDesc)
{
	TRACE_ENTER(2,14,This,14,lpDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDesc->dwSize < sizeof(D3DEXECUTEBUFFERDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->inuse) TRACE_RET(HRESULT,23,DDERR_WASSTILLDRAWING);
	if(This->locked) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_LOCKED);
	This->desc.dwCaps = lpDesc->dwCaps;
	This->desc.dwFlags |= D3DDEB_LPDATA;
	if(!This->data)
	{
		This->data = (unsigned char *)malloc(This->desc.dwBufferSize);
		if(!This->data) return DDERR_OUTOFMEMORY;
	}
	This->desc.lpData = This->data;
	memcpy(lpDesc,&This->desc,sizeof(D3DEXECUTEBUFFERDESC));
	This->locked = TRUE;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer_Optimize(glDirect3DExecuteBuffer *This, DWORD dwDummy)
{
	TRACE_ENTER(2,14,This,9,dwDummy);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI glDirect3DExecuteBuffer_SetExecuteData(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpData->dwSize != sizeof(D3DEXECUTEDATA)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&This->datadesc,lpData,sizeof(D3DEXECUTEDATA));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer_Unlock(glDirect3DExecuteBuffer *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->inuse) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_NOT_LOCKED);
	if(!This->locked) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_NOT_LOCKED);
	This->locked = FALSE;
	TRACE_RET(HRESULT,23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer_Validate(glDirect3DExecuteBuffer *This, LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved)
{
	TRACE_ENTER(5,14,This,14,lpdwOffset,14,lpFunc,14,lpUserArg,9,dwReserved);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT glDirect3DExecuteBuffer_ExecuteLock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEBUFFERDESC lpDesc,LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(3,14,This,14,lpDesc,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->inuse) TRACE_RET(HRESULT,23,DDERR_WASSTILLDRAWING);
	if(This->locked) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_LOCKED);
	if(!lpDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->desc.dwFlags |= D3DDEB_LPDATA;
	if(!This->data)
	{
		This->data = (unsigned char *)malloc(This->desc.dwBufferSize);
		if(!This->data) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	}
	This->desc.lpData = This->data;
	memcpy(lpDesc,&This->desc,sizeof(D3DEXECUTEBUFFERDESC));
	memcpy(lpData,&This->datadesc,sizeof(D3DEXECUTEDATA));
	This->locked = TRUE;
	This->inuse = TRUE;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DExecuteBuffer_ExecuteUnlock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->inuse) TRACE_RET(HRESULT,23,D3DERR_EXECUTE_NOT_LOCKED);
	This->inuse = FALSE;
	This->locked = FALSE;
	memcpy(&This->datadesc,lpData,sizeof(D3DEXECUTEDATA));
	TRACE_RET(HRESULT,23,D3D_OK);
	return D3D_OK;
}