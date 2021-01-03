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
#ifndef __GLDIRECT3DTEXTURE_H
#define __GLDIRECT3DTEXTURE_H

struct glDirect3DTexture2Vtbl;
typedef struct glDirect3DTexture2
{
	glDirect3DTexture2Vtbl *lpVtbl;
	glDirectDrawSurface7 *glDDS7;
} glDirect3DTexture2;

typedef struct glDirect3DTexture2Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DTexture2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DTexture2 *This);
	ULONG(WINAPI *Release)(glDirect3DTexture2 *This);
	HRESULT(WINAPI *GetHandle)(glDirect3DTexture2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle);
	HRESULT(WINAPI *PaletteChanged)(glDirect3DTexture2 *This, DWORD dwStart, DWORD dwCount);
	HRESULT(WINAPI *Load)(glDirect3DTexture2 *This, LPDIRECT3DTEXTURE2 lpD3DTexture);
} glDirect3DTexture2Vtbl;

HRESULT glDirect3DTexture2_Create(glDirectDrawSurface7 *glDDS7, glDirect3DTexture2 **texture);
HRESULT WINAPI glDirect3DTexture2_QueryInterface(glDirect3DTexture2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DTexture2_AddRef(glDirect3DTexture2 *This);
ULONG WINAPI glDirect3DTexture2_Release(glDirect3DTexture2 *This);
HRESULT WINAPI glDirect3DTexture2_GetHandle(glDirect3DTexture2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice2, LPD3DTEXTUREHANDLE lpHandle);
HRESULT WINAPI glDirect3DTexture2_Load(glDirect3DTexture2 *This, LPDIRECT3DTEXTURE2 lpD3DTexture2);
HRESULT WINAPI glDirect3DTexture2_PaletteChanged(glDirect3DTexture2 *This, DWORD dwStart, DWORD dwCount);

struct glDirect3DTexture1Vtbl;
typedef struct glDirect3DTexture1
{
	glDirect3DTexture1Vtbl *lpVtbl;
	glDirectDrawSurface7 *glDDS7;
} glDirect3DTexture1;

typedef struct glDirect3DTexture1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DTexture1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DTexture1 *This);
	ULONG(WINAPI *Release)(glDirect3DTexture1 *This);
	HRESULT(WINAPI *Initialize)(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface);
	HRESULT(WINAPI *GetHandle)(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle);
	HRESULT(WINAPI *PaletteChanged)(glDirect3DTexture1 *This, DWORD dwStart, DWORD dwCount);
	HRESULT(WINAPI *Load)(glDirect3DTexture1 *This, LPDIRECT3DTEXTURE lpD3DTexture);
	HRESULT(WINAPI *Unload)(glDirect3DTexture1 *This);
} glDirect3DTexture1Vtbl;

HRESULT glDirect3DTexture1_Create(glDirectDrawSurface7 *glDDS7, glDirect3DTexture1 **texture);
HRESULT WINAPI glDirect3DTexture1_QueryInterface(glDirect3DTexture1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DTexture1_AddRef(glDirect3DTexture1 *This);
ULONG WINAPI glDirect3DTexture1_Release(glDirect3DTexture1 *This);
HRESULT WINAPI glDirect3DTexture1_GetHandle(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle);
HRESULT WINAPI glDirect3DTexture1_Initialize(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface);
HRESULT WINAPI glDirect3DTexture1_Load(glDirect3DTexture1 *This, LPDIRECT3DTEXTURE lpD3DTexture);
HRESULT WINAPI glDirect3DTexture1_PaletteChanged(glDirect3DTexture1 *This, DWORD dwStart, DWORD dwCount);
HRESULT WINAPI glDirect3DTexture1_Unload(glDirect3DTexture1 *This);

#endif //__GLDIRECT3DTEXTURE_H
