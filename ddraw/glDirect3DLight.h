// DXGL
// Copyright (C) 2012-2021 William Feely

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

struct glDirect3DLightVtbl;
typedef struct glDirect3DLight
{
	glDirect3DLightVtbl *lpVtbl;
	D3DLIGHT7 light;
	glDirect3DViewport3 *viewport;
	ULONG refcount;
	D3DLIGHT2 convert;
	glDirect3DDevice7 *device;
	int index;
} glDirect3DLight;
typedef struct glDirect3DLightVtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DLight *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DLight *This);
	ULONG(WINAPI *Release)(glDirect3DLight *This);
	HRESULT(WINAPI *Initialize)(glDirect3DLight *This, LPDIRECT3D lpDirect3D);
	HRESULT(WINAPI *SetLight)(glDirect3DLight *This, LPD3DLIGHT lpLight);
	HRESULT(WINAPI *GetLight)(glDirect3DLight *This, LPD3DLIGHT lpLight);
} glDirect3DLightVtbl;

HRESULT glDirect3DLight_Create(D3DLIGHT7 *light_in, glDirect3DLight **light);
HRESULT glDirect3DLight_CreateNoLight(glDirect3DLight **light);
HRESULT WINAPI glDirect3DLight_QueryInterface(glDirect3DLight *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DLight_AddRef(glDirect3DLight *This);
ULONG WINAPI glDirect3DLight_Release(glDirect3DLight *This);
HRESULT WINAPI glDirect3DLight_GetLight(glDirect3DLight *This, LPD3DLIGHT lpLight);
void glDirect3DLight_GetLight7(glDirect3DLight *This, LPD3DLIGHT7 lpLight7);
HRESULT WINAPI glDirect3DLight_Initialize(glDirect3DLight *This, LPDIRECT3D lpDirect3D);
HRESULT WINAPI glDirect3DLight_SetLight(glDirect3DLight *This, LPD3DLIGHT lpLight);
void glDirect3DLight_SetLight7(glDirect3DLight *This, LPD3DLIGHT7 lpLight7);
void glDirect3DLight_SetDevice(glDirect3DLight *This, glDirect3DDevice7 *device, int index);
void glDirect3DLight_Sync(glDirect3DLight *This);

#endif //__GLDIRECT3DLIGHT_H
