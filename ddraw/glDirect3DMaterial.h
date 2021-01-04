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
#ifndef __GLDIRECT3DMATERIAL_H
#define __GLDIRECT3DMATERIAL_H

struct glDirect3DMaterial1;
struct glDirect3DMaterial2;

struct glDirect3DMaterial3Vtbl;
typedef struct glDirect3DMaterial3
{
	glDirect3DMaterial3Vtbl *lpVtbl;
	D3DMATERIAL material;
	D3DMATERIALHANDLE handle;
	glDirect3DMaterial2 *glD3DM2;
	glDirect3DMaterial1 *glD3DM1;
	ULONG refcount;
	BOOL current;
	glDirect3DDevice7 *device;
} glDirect3DMaterial3;

typedef struct glDirect3DMaterial3Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DMaterial3 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DMaterial3 *This);
	ULONG(WINAPI *Release)(glDirect3DMaterial3 *This);
	HRESULT(WINAPI *SetMaterial)(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetMaterial)(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetHandle)(glDirect3DMaterial3 *This, LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
} glDirect3DMaterial3Vtbl;

HRESULT glDirect3DMaterial3_Create(glDirect3DMaterial3 **material);
void glDirect3DMaterial3_Destroy(glDirect3DMaterial3 *This);
HRESULT WINAPI glDirect3DMaterial3_QueryInterface(glDirect3DMaterial3 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DMaterial3_AddRef(glDirect3DMaterial3 *This);
ULONG WINAPI glDirect3DMaterial3_Release(glDirect3DMaterial3 *This);
HRESULT WINAPI glDirect3DMaterial3_GetHandle(glDirect3DMaterial3 *This, LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
HRESULT WINAPI glDirect3DMaterial3_GetMaterial(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat);
HRESULT WINAPI glDirect3DMaterial3_SetMaterial(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat);
void glDirect3DMaterial3_Sync(glDirect3DMaterial3 *This);
void glDirect3DMaterial3_SetCurrent(glDirect3DMaterial3 *This, BOOL current);
void glDirect3DMaterial3_unbind(glDirect3DMaterial3 *This);


struct glDirect3DMaterial2Vtbl;
typedef struct glDirect3DMaterial2
{
	glDirect3DMaterial2Vtbl *lpVtbl;
	glDirect3DMaterial3 *glD3DM3;
} glDirect3DMaterial2;

typedef struct glDirect3DMaterial2Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DMaterial2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DMaterial2 *This);
	ULONG(WINAPI *Release)(glDirect3DMaterial2 *This);
	HRESULT(WINAPI *SetMaterial)(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetMaterial)(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetHandle)(glDirect3DMaterial2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
} glDirect3DMaterial2Vtbl;

HRESULT glDirect3DMaterial2_Create(glDirect3DMaterial3 *glD3DM3, glDirect3DMaterial2 **material);
HRESULT WINAPI glDirect3DMaterial2_QueryInterface(glDirect3DMaterial2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DMaterial2_AddRef(glDirect3DMaterial2 *This);
ULONG WINAPI glDirect3DMaterial2_Release(glDirect3DMaterial2 *This);
HRESULT WINAPI glDirect3DMaterial2_GetHandle(glDirect3DMaterial2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
HRESULT WINAPI glDirect3DMaterial2_GetMaterial(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat);
HRESULT WINAPI glDirect3DMaterial2_SetMaterial(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat);


struct glDirect3DMaterial1Vtbl;
typedef struct glDirect3DMaterial1
{
	glDirect3DMaterial1Vtbl *lpVtbl;
	glDirect3DMaterial3 *glD3DM3;
} glDirect3DMaterial1;

typedef struct glDirect3DMaterial1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DMaterial1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DMaterial1 *This);
	ULONG(WINAPI *Release)(glDirect3DMaterial1 *This);
	HRESULT(WINAPI *Initialize)(glDirect3DMaterial1 *This, LPDIRECT3D lpDirect3D);
	HRESULT(WINAPI *SetMaterial)(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetMaterial)(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat);
	HRESULT(WINAPI *GetHandle)(glDirect3DMaterial1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
	HRESULT(WINAPI *Reserve)(glDirect3DMaterial1 *This);
	HRESULT(WINAPI *Unreserve)(glDirect3DMaterial1 *This);
} glDirect3DMaterial1Vtbl;

HRESULT glDirect3DMaterial1_Create(glDirect3DMaterial3 *glD3DM3, glDirect3DMaterial1 **material);
HRESULT WINAPI glDirect3DMaterial1_QueryInterface(glDirect3DMaterial1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DMaterial1_AddRef(glDirect3DMaterial1 *This);
ULONG WINAPI glDirect3DMaterial1_Release(glDirect3DMaterial1 *This);
HRESULT WINAPI glDirect3DMaterial1_GetHandle(glDirect3DMaterial1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
HRESULT WINAPI glDirect3DMaterial1_GetMaterial(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat);
HRESULT WINAPI glDirect3DMaterial1_Initialize(glDirect3DMaterial1 *This, LPDIRECT3D lpDirect3D);
HRESULT WINAPI glDirect3DMaterial1_Reserve(glDirect3DMaterial1 *This);
HRESULT WINAPI glDirect3DMaterial1_SetMaterial(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat);
HRESULT WINAPI glDirect3DMaterial1_Unreserve(glDirect3DMaterial1 *This);

#endif //__GLDIRECT3DMATERIAL_H
