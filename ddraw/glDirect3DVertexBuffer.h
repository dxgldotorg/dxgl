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

#pragma once
#ifndef __GLDIRECT3DVERTEXBUFFER_H
#define __GLDIRECT3DVERTEXBUFFER_H

class glDirect3DVertexBuffer7 : public IDirect3DVertexBuffer7
{
public:
	glDirect3DVertexBuffer7(glDirect3D7 *glD3DD7, D3DVERTEXBUFFERDESC desc, DWORD flags);
	~glDirect3DVertexBuffer7();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetVertexBufferDesc(LPD3DVERTEXBUFFERDESC lpVBDesc);
	HRESULT WINAPI Lock(DWORD dwFlags, LPVOID* lplpData, LPDWORD lpdwSize); 
	HRESULT WINAPI Optimize(LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
	HRESULT WINAPI ProcessVertices(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount, 
		LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
	HRESULT WINAPI ProcessVerticesStrided(DWORD dwVertexOp, DWORD dwDestIndex, DWORD dwCount,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwSrcIndex, LPDIRECT3DDEVICE7 lpD3DDevice, DWORD dwFlags);
	HRESULT WINAPI Unlock();
private:
	glDirect3D7 *glD3D7;
	ULONG refcount;
	D3DVERTEXBUFFERDESC vbdesc;
	DWORD flags;
};

#endif //__GLDIRECT3DVERTEXBUFFER_H