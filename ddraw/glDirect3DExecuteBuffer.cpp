// DXGL
// Copyright (C) 2013 William Feely

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

glDirect3DExecuteBuffer::glDirect3DExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc)
{
	TRACE_ENTER(2,14,this,14,lpDesc);
	locked = false;
	inuse = false;
	data = NULL;
	desc = *lpDesc;
	if(!(desc.dwFlags & D3DDEB_CAPS))
	{
		desc.dwFlags |= D3DDEB_CAPS;
		desc.dwCaps = D3DDEBCAPS_MEM;
	}
	ZeroMemory(&datadesc,sizeof(D3DEXECUTEDATA));
	datadesc.dwSize = sizeof(D3DEXECUTEDATA);
	TRACE_EXIT(-1,0);
}
glDirect3DExecuteBuffer::~glDirect3DExecuteBuffer()
{
	TRACE_ENTER(1,14,this);
	if(data) free(data);
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DExecuteBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DExecuteBuffer))
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DExecuteBuffer::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DExecuteBuffer::Release()
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

HRESULT WINAPI glDirect3DExecuteBuffer::GetExecuteData(LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,this,14,lpData);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(locked) TRACE_RET(23,D3DERR_EXECUTE_LOCKED);
	if(lpData->dwSize < sizeof(D3DEXECUTEDATA)) TRACE_RET(23,DDERR_INVALIDPARAMS);
	memcpy(lpData,&datadesc,sizeof(D3DEXECUTEDATA));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Initialize(LPDIRECT3DDEVICE lpDirect3DDevice,
	LPD3DEXECUTEBUFFERDESC lpDesc)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice,14,lpDesc);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Lock(LPD3DEXECUTEBUFFERDESC lpDesc)
{
	TRACE_ENTER(2,14,this,14,lpDesc);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!lpDesc) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(lpDesc->dwSize < sizeof(D3DEXECUTEBUFFERDESC)) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(inuse) TRACE_RET(23,DDERR_WASSTILLDRAWING);
	if(locked) TRACE_RET(23,D3DERR_EXECUTE_LOCKED);
	desc.dwCaps = lpDesc->dwCaps;
	desc.dwFlags |= D3DDEB_LPDATA;
	if(!data)
	{
		data = (unsigned char *)malloc(desc.dwBufferSize);
		if(!data) return DDERR_OUTOFMEMORY;
	}
	desc.lpData = data;
	memcpy(lpDesc,&desc,sizeof(D3DEXECUTEBUFFERDESC));
	locked = true;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Optimize(DWORD dwDummy)
{
	TRACE_ENTER(2,14,this,9,dwDummy);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI glDirect3DExecuteBuffer::SetExecuteData(LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,this,14,lpData);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(lpData->dwSize != sizeof(D3DEXECUTEDATA)) TRACE_RET(23,DDERR_INVALIDPARAMS);
	memcpy(&datadesc,lpData,sizeof(D3DEXECUTEDATA));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Unlock()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(inuse) TRACE_RET(23,D3DERR_EXECUTE_NOT_LOCKED);
	if(!locked) TRACE_RET(23,D3DERR_EXECUTE_NOT_LOCKED);
	locked = false;
	TRACE_RET(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Validate(LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved)
{
	TRACE_ENTER(5,14,this,14,lpdwOffset,14,lpFunc,14,lpUserArg,9,dwReserved);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT glDirect3DExecuteBuffer::ExecuteLock(LPD3DEXECUTEBUFFERDESC lpDesc,LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(3,14,this,14,lpDesc,14,lpData);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(inuse) TRACE_RET(23,DDERR_WASSTILLDRAWING);
	if(locked) TRACE_RET(23,D3DERR_EXECUTE_LOCKED);
	if(!lpDesc) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(!lpData) TRACE_RET(23,DDERR_INVALIDPARAMS);
	desc.dwFlags |= D3DDEB_LPDATA;
	if(!data)
	{
		data = (unsigned char *)malloc(desc.dwBufferSize);
		if(!data) TRACE_RET(23,DDERR_OUTOFMEMORY);
	}
	desc.lpData = data;
	memcpy(lpDesc,&desc,sizeof(D3DEXECUTEBUFFERDESC));
	memcpy(lpData,&datadesc,sizeof(D3DEXECUTEDATA));
	locked = true;
	inuse = true;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DExecuteBuffer::ExecuteUnlock(LPD3DEXECUTEDATA lpData)
{
	TRACE_ENTER(2,14,this,14,lpData);
	if(!this) TRACE_RET(23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(23,DDERR_INVALIDPARAMS);
	if(!inuse) TRACE_RET(23,D3DERR_EXECUTE_NOT_LOCKED);
	inuse = false;
	locked = false;
	memcpy(&datadesc,lpData,sizeof(D3DEXECUTEDATA));
	TRACE_RET(23,D3D_OK);
	return D3D_OK;
}