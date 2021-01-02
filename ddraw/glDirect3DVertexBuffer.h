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

#pragma once
#ifndef __GLDIRECT3DVERTEXBUFFER_H
#define __GLDIRECT3DVERTEXBUFFER_H

struct glDirect3DVertexBuffer7Vtbl;

typedef struct glDirect3DVertexBuffer7
{
	glDirect3DVertexBuffer7Vtbl *lpVtbl;
	glDirect3D7 *glD3D7;
	int version;
	ULONG refcount;
	D3DVERTEXBUFFERDESC vbdesc;
	DWORD flags;
} glDirect3DVertexBuffer7;

typedef struct glDirect3DVertexBuffer7Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DVertexBuffer7 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DVertexBuffer7 *This);
	ULONG(WINAPI *Release)(glDirect3DVertexBuffer7 *This);
	HRESULT(WINAPI *Lock)(glDirect3DVertexBuffer7 *This, DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize);
	HRESULT(WINAPI *Unlock)(glDirect3DVertexBuffer7 *This);
	HRESULT(WINAPI *ProcessVertices)(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
	HRESULT(WINAPI *GetVertexBufferDesc)(glDirect3DVertexBuffer7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc);
	HRESULT(WINAPI *Optimize)(glDirect3DVertexBuffer7 *This, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
	HRESULT(WINAPI *ProcessVerticesStrided)(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
} glDirect3DVertexBuffer7Vtbl;

HRESULT glDirect3DVertexBuffer7_Create(glDirect3D7 *glD3D7, D3DVERTEXBUFFERDESC desc, DWORD flags, glDirect3DVertexBuffer7 **buffer);
HRESULT glDirect3DVertexBuffer1_Create(glDirect3D3 *glD3D3, D3DVERTEXBUFFERDESC desc, DWORD flags, glDirect3DVertexBuffer7 **buffer);
void glDirect3DVertexBuffer7_Destroy(glDirect3DVertexBuffer7 *This);
HRESULT WINAPI glDirect3DVertexBuffer7_QueryInterface(glDirect3DVertexBuffer7 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DVertexBuffer7_AddRef(glDirect3DVertexBuffer7 *This);
ULONG WINAPI glDirect3DVertexBuffer7_Release(glDirect3DVertexBuffer7 *This);
HRESULT WINAPI glDirect3DVertexBuffer7_GetVertexBufferDesc(glDirect3DVertexBuffer7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc);
HRESULT WINAPI glDirect3DVertexBuffer7_Lock(glDirect3DVertexBuffer7 *This, DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize);
HRESULT WINAPI glDirect3DVertexBuffer7_Optimize(glDirect3DVertexBuffer7 *This, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
HRESULT WINAPI glDirect3DVertexBuffer7_ProcessVertices(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
HRESULT WINAPI glDirect3DVertexBuffer7_ProcessVerticesStrided(glDirect3DVertexBuffer7 *This, DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
HRESULT WINAPI glDirect3DVertexBuffer7_Unlock(glDirect3DVertexBuffer7 *This);


class glDirect3DVertexBuffer1 : public IDirect3DVertexBuffer
{
public:
	glDirect3DVertexBuffer1(glDirect3DVertexBuffer7 *glD3DVB7);
	virtual ~glDirect3DVertexBuffer1();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpVBDesc); 
	HRESULT WINAPI Lock(DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize);
	HRESULT WINAPI Optimize(LPDIRECT3DDEVICE3 lpD3DDevice,DWORD dwFlags); 
	HRESULT WINAPI ProcessVertices(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPDIRECT3DVERTEXBUFFER lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE3 lpD3DDevice, DWORD dwFlags);
	HRESULT WINAPI Unlock();
	glDirect3DVertexBuffer7 *GetGLD3DVB7(){return glD3DVB7;}
private:
	glDirect3DVertexBuffer7 *glD3DVB7;
};

#endif //__GLDIRECT3DVERTEXBUFFER_H
