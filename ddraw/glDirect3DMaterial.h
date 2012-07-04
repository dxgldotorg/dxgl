// DXGL
// Copyright (C) 2012 William Feely

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
#ifndef __GLDIRECT3DMATERIAL_H
#define __GLDIRECT3DMATERIAL_H

class glDirect3DMaterial3 : public IDirect3DMaterial3
{
public:
	glDirect3DMaterial3();
	virtual ~glDirect3DMaterial3();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetHandle(LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
	HRESULT WINAPI GetMaterial(LPD3DMATERIAL lpMat);
	HRESULT WINAPI SetMaterial(LPD3DMATERIAL lpMat);
	void Sync();
	void SetCurrent(bool current);
	void unbind();
	D3DMATERIAL material;
private:
	ULONG refcount;
	bool current;
	glDirect3DDevice7 *device;
	D3DMATERIALHANDLE handle;
};

#endif //__GLDIRECT3DMATERIAL_H