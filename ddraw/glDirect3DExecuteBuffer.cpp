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
}
glDirect3DExecuteBuffer::~glDirect3DExecuteBuffer()
{
	if(data) free(data);
}

HRESULT WINAPI glDirect3DExecuteBuffer::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DExecuteBuffer))
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DExecuteBuffer::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DExecuteBuffer::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DExecuteBuffer::GetExecuteData(LPD3DEXECUTEDATA lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpData) return DDERR_INVALIDPARAMS;
	if(locked) return D3DERR_EXECUTE_LOCKED;
	if(lpData->dwSize < sizeof(D3DEXECUTEDATA)) return DDERR_INVALIDPARAMS;
	memcpy(lpData,&datadesc,sizeof(D3DEXECUTEDATA));
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Initialize(LPDIRECT3DDEVICE lpDirect3DDevice,
	LPD3DEXECUTEBUFFERDESC lpDesc)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Lock(LPD3DEXECUTEBUFFERDESC lpDesc)
{
	if(!this) return  DDERR_INVALIDOBJECT;
	if(!lpDesc) return DDERR_INVALIDPARAMS;
	if(lpDesc->dwSize < sizeof(D3DEXECUTEBUFFERDESC)) return DDERR_INVALIDPARAMS;
	if(inuse) return DDERR_WASSTILLDRAWING;
	if(locked) return D3DERR_EXECUTE_LOCKED;
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
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Optimize(DWORD dwDummy)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI glDirect3DExecuteBuffer::SetExecuteData(LPD3DEXECUTEDATA lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpData) return DDERR_INVALIDPARAMS;
	if(lpData->dwSize != sizeof(D3DEXECUTEDATA)) return DDERR_INVALIDPARAMS;
	memcpy(&datadesc,lpData,sizeof(D3DEXECUTEDATA));
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Unlock()
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(inuse) return D3DERR_EXECUTE_NOT_LOCKED;
	if(!locked) return D3DERR_EXECUTE_NOT_LOCKED;
	locked = false;
	return D3D_OK;
}

HRESULT WINAPI glDirect3DExecuteBuffer::Validate(LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DDERR_UNSUPPORTED;
}

HRESULT glDirect3DExecuteBuffer::ExecuteLock(LPD3DEXECUTEBUFFERDESC lpDesc,LPD3DEXECUTEDATA lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(inuse) return DDERR_WASSTILLDRAWING;
	if(locked) return D3DERR_EXECUTE_LOCKED;
	if(!lpDesc) return DDERR_INVALIDPARAMS;
	if(!lpData) return DDERR_INVALIDPARAMS;
	desc.dwFlags |= D3DDEB_LPDATA;
	if(!data)
	{
		data = (unsigned char *)malloc(desc.dwBufferSize);
		if(!data) return DDERR_OUTOFMEMORY;
	}
	desc.lpData = data;
	memcpy(lpDesc,&desc,sizeof(D3DEXECUTEBUFFERDESC));
	memcpy(lpData,&datadesc,sizeof(D3DEXECUTEDATA));
	locked = true;
	inuse = true;
	return D3D_OK;
}

HRESULT glDirect3DExecuteBuffer::ExecuteUnlock()
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!inuse) return D3DERR_EXECUTE_NOT_LOCKED;
	inuse = false;
	locked = false;
	return D3D_OK;
}