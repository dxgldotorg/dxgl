// DXGL
// Copyright (C) 2013-2021 William Feely

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

#pragma once
#ifndef __GLDIRECT3DEXECUTEBUFFER_H
#define __GLDIRECT3DEXECUTEBUFFER_H

struct glDirect3DExecuteBufferVtbl;

typedef struct glDirect3DExecuteBuffer
{
	glDirect3DExecuteBufferVtbl *lpVtbl;
	ULONG refcount;
	D3DEXECUTEBUFFERDESC desc;
	D3DEXECUTEDATA datadesc;
	unsigned char *data;
	bool locked;
	bool inuse;
} glDirect3DExecuteBuffer;

typedef struct glDirect3DExecuteBufferVtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DExecuteBuffer *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DExecuteBuffer *This);
	ULONG(WINAPI *Release)(glDirect3DExecuteBuffer *This);
	HRESULT(WINAPI *Initialize)(glDirect3DExecuteBuffer *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DEXECUTEBUFFERDESC lpDesc);
	HRESULT(WINAPI *Lock)(glDirect3DExecuteBuffer *This, LPD3DEXECUTEBUFFERDESC lpDesc);
	HRESULT(WINAPI *Unlock)(glDirect3DExecuteBuffer *This);
	HRESULT(WINAPI *GetExecuteData)(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData);
	HRESULT(WINAPI *SetExecuteData)(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData);
	HRESULT(WINAPI *Validate)(glDirect3DExecuteBuffer *This, LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved);
	HRESULT(WINAPI *Optimize)(glDirect3DExecuteBuffer *This, DWORD dwDummy);
} glDirect3DExecuteBufferVtbl;

HRESULT glDirect3DExecuteBuffer_Create(LPD3DEXECUTEBUFFERDESC lpDesc, glDirect3DExecuteBuffer **buffer);
void glDirect3DExecuteBuffer_Destroy(glDirect3DExecuteBuffer *This);
HRESULT WINAPI glDirect3DExecuteBuffer_QueryInterface(glDirect3DExecuteBuffer *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DExecuteBuffer_AddRef(glDirect3DExecuteBuffer *This);
ULONG WINAPI glDirect3DExecuteBuffer_Release(glDirect3DExecuteBuffer *This);
HRESULT WINAPI glDirect3DExecuteBuffer_GetExecuteData(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData);
HRESULT WINAPI glDirect3DExecuteBuffer_Initialize(glDirect3DExecuteBuffer *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DEXECUTEBUFFERDESC lpDesc);
HRESULT WINAPI glDirect3DExecuteBuffer_Lock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEBUFFERDESC lpDesc);
HRESULT WINAPI glDirect3DExecuteBuffer_Optimize(glDirect3DExecuteBuffer *This, DWORD dwDummy);
HRESULT WINAPI glDirect3DExecuteBuffer_SetExecuteData(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData);
HRESULT WINAPI glDirect3DExecuteBuffer_Unlock(glDirect3DExecuteBuffer *This);
HRESULT WINAPI glDirect3DExecuteBuffer_Validate(glDirect3DExecuteBuffer *This, LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved);
HRESULT glDirect3DExecuteBuffer_ExecuteLock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEBUFFERDESC lpDesc,LPD3DEXECUTEDATA lpData);
HRESULT glDirect3DExecuteBuffer_ExecuteUnlock(glDirect3DExecuteBuffer *This, LPD3DEXECUTEDATA lpData);

#endif //__GLDIRECT3DEXECUTEBUFFER_H
