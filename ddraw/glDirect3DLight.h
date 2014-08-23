// DXGL
// Copyright (C) 2012-2014 William Feely

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
#ifndef __GLDIRECT3DLIGHT_H
#define __GLDIRECT3DLIGHT_H

struct glDirect3DViewport3;
class glDirect3DLight : public IDirect3DLight
{
public:
	glDirect3DLight();
	glDirect3DLight(D3DLIGHT7 *light_in);
	virtual ~glDirect3DLight();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetLight(LPD3DLIGHT lpLight);
	void GetLight7(LPD3DLIGHT7 lpLight7);
	HRESULT WINAPI Initialize(LPDIRECT3D lpDirect3D);
	HRESULT WINAPI SetLight(LPD3DLIGHT lpLight);
	void SetLight7(LPD3DLIGHT7 lpLight7);
	void SetDevice(glDirect3DDevice7 *device, int index);
	void Sync();
	D3DLIGHT7 light;
	glDirect3DViewport3 *viewport;
private:
	ULONG refcount;
	D3DLIGHT2 convert;
	glDirect3DDevice7 *device;
	int index;
};

#endif //__GLDIRECT3DLIGHT_H