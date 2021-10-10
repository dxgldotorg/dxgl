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
#ifndef __GLDIRECT3D_H
#define __GLDIRECT3D_H

extern const D3DDEVICEDESC7 d3ddesc_default;
extern const D3DDEVICEDESC d3ddesc3_default;
struct glDirectDraw7;

struct glDirect3D3;
struct glDirect3D2;
struct glDirect3D1;

struct glDirect3D7Vtbl;
typedef struct glDirect3D7
{
	glDirect3D7Vtbl *lpVtbl;
	glDirectDraw7 *glDD7;
} glDirect3D7;

typedef struct glDirect3D7Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3D7 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3D7 *This);
	ULONG(WINAPI *Release)(glDirect3D7 *This);
	HRESULT(WINAPI *EnumDevices)(glDirect3D7 *This, LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT(WINAPI *CreateDevice)(glDirect3D7 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice);
	HRESULT(WINAPI *glDirect3D7_CreateVertexBuffer)(glDirect3D7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags);
	HRESULT(WINAPI *EnumZBufferFormats)(glDirect3D7 *This, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
	HRESULT(WINAPI *EvictManagedTextures)(glDirect3D7 *This);
} glDirect3D7Vtbl;

HRESULT glDirect3D7_Create(glDirectDraw7 *gl_DD7, glDirect3D7 **glD3D7);
HRESULT WINAPI glDirect3D7_QueryInterface(glDirect3D7 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3D7_AddRef(glDirect3D7 *This);
ULONG WINAPI glDirect3D7_Release(glDirect3D7 *This);
HRESULT WINAPI glDirect3D7_CreateDevice(glDirect3D7 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice);
HRESULT WINAPI glDirect3D7_CreateLight(glDirect3D7 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D7_CreateMaterial(glDirect3D7 *This, LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D7_CreateVertexBuffer(glDirect3D7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags);
HRESULT WINAPI glDirect3D7_CreateViewport(glDirect3D7 *This, LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D7_EnumDevices(glDirect3D7 *This, LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI glDirect3D7_EnumDevices3(glDirect3D7 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI glDirect3D7_EnumZBufferFormats(glDirect3D7 *This, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
HRESULT WINAPI glDirect3D7_EvictManagedTextures(glDirect3D7 *This);
HRESULT WINAPI glDirect3D7_FindDevice(glDirect3D7 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
HRESULT glDirect3D7_CreateDevice2(glDirect3D7 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice, int version);


struct glDirect3D3Vtbl;
typedef struct glDirect3D3
{
	glDirect3D3Vtbl *lpVtbl;
	glDirect3D7 *glD3D7;
} glDirect3D3;

typedef struct glDirect3D3Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3D3 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3D3 *This);
	ULONG(WINAPI *Release)(glDirect3D3 *This);
	HRESULT(WINAPI *EnumDevices)(glDirect3D3 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT(WINAPI *CreateLight)(glDirect3D3 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT(WINAPI *CreateMaterial)(glDirect3D3 *This, LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter);
	HRESULT(WINAPI *CreateViewport)(glDirect3D3 *This, LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter);
	HRESULT(WINAPI *FindDevice)(glDirect3D3 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
	HRESULT(WINAPI *CreateDevice)(glDirect3D3 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice, LPUNKNOWN pUnkOuter);
	HRESULT(WINAPI *CreateVertexBuffer)(glDirect3D3 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER* lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter);
	HRESULT(WINAPI *EnumZBufferFormats)(glDirect3D3 *This, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
	HRESULT(WINAPI *EvictManagedTextures)(glDirect3D3 *This);
} glDirect3D3Vtbl;

HRESULT glDirect3D3_Create(glDirect3D7 *glD3D7, glDirect3D3 **glD3D3);
HRESULT WINAPI glDirect3D3_QueryInterface(glDirect3D3 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3D3_AddRef(glDirect3D3 *This);
ULONG WINAPI glDirect3D3_Release(glDirect3D3 *This);
HRESULT WINAPI glDirect3D3_CreateDevice(glDirect3D3 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice, LPUNKNOWN pUnkOuter);
HRESULT WINAPI glDirect3D3_CreateLight(glDirect3D3 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D3_CreateMaterial(glDirect3D3 *This, LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D3_CreateVertexBuffer(glDirect3D3 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER* lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter);
HRESULT WINAPI glDirect3D3_CreateViewport(glDirect3D3 *This, LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D3_EnumDevices(glDirect3D3 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI glDirect3D3_EnumZBufferFormats(glDirect3D3 *This, REFCLSID riidDevice,  LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
HRESULT WINAPI glDirect3D3_EvictManagedTextures(glDirect3D3 *This);
HRESULT WINAPI glDirect3D3_FindDevice(glDirect3D3 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);

struct glDirect3D2Vtbl;
typedef struct glDirect3D2
{
	glDirect3D2Vtbl *lpVtbl;
	glDirect3D7 *glD3D7;
} glDirect3D2;

typedef struct glDirect3D2Vtbl
{
	HRESULT(WINAPI *glDirect3D2_QueryInterface)(glDirect3D2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *glDirect3D2_AddRef)(glDirect3D2 *This);
	ULONG(WINAPI *glDirect3D2_Release)(glDirect3D2 *This);
	HRESULT(WINAPI *glDirect3D2_EnumDevices)(glDirect3D2 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT(WINAPI *glDirect3D2_CreateLight)(glDirect3D2 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT(WINAPI *glDirect3D2_CreateMaterial)(glDirect3D2 *This, LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2, IUnknown* pUnkOuter);
	HRESULT(WINAPI *glDirect3D2_CreateViewport)(glDirect3D2 *This, LPDIRECT3DVIEWPORT2* lplpD3DViewport2, IUnknown* pUnkOuter);
	HRESULT(WINAPI *glDirect3D2_FindDevice)(glDirect3D2 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
	HRESULT(WINAPI *glDirect3D2_CreateDevice)(glDirect3D2 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice2);
} glDirect3D2Vtbl;

HRESULT glDirect3D2_Create(glDirect3D7 *glD3D7, glDirect3D2 **glD3D2);
HRESULT WINAPI glDirect3D2_QueryInterface(glDirect3D2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3D2_AddRef(glDirect3D2 *This);
ULONG WINAPI glDirect3D2_Release(glDirect3D2 *This);
HRESULT WINAPI glDirect3D2_CreateDevice(glDirect3D2 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice2);
HRESULT WINAPI glDirect3D2_CreateLight(glDirect3D2 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D2_CreateMaterial(glDirect3D2 *This, LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D2_CreateViewport(glDirect3D2 *This, LPDIRECT3DVIEWPORT2* lplpD3DViewport2, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D2_EnumDevices(glDirect3D2 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI glDirect3D2_FindDevice(glDirect3D2 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);

struct glDirect3D1Vtbl;
typedef struct glDirect3D1
{
	glDirect3D1Vtbl *lpVtbl;
	glDirect3D7 *glD3D7;
} glDirect3D1;

typedef struct glDirect3D1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3D1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3D1 *This);
	ULONG(WINAPI *Release)(glDirect3D1 *This);
	HRESULT(WINAPI *Initialize)(glDirect3D1 *This, REFIID lpREFIID);
	HRESULT(WINAPI *EnumDevices)(glDirect3D1 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT(WINAPI *CreateLight)(glDirect3D1 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT(WINAPI *CreateMaterial)(glDirect3D1 *This, LPDIRECT3DMATERIAL* lplpDirect3DMaterial, IUnknown* pUnkOuter);
	HRESULT(WINAPI *CreateViewport)(glDirect3D1 *This, LPDIRECT3DVIEWPORT* lplpD3DViewport, IUnknown* pUnkOuter);
	HRESULT(WINAPI *FindDevice)(glDirect3D1 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
} glDirect3D1Vtbl;

HRESULT glDirect3D1_Create(glDirect3D7 *glD3D7, glDirect3D1 **glD3D1);
HRESULT WINAPI glDirect3D1_QueryInterface(glDirect3D1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3D1_AddRef(glDirect3D1 *This);
ULONG WINAPI glDirect3D1_Release(glDirect3D1 *This);
HRESULT WINAPI glDirect3D1_CreateLight(glDirect3D1 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D1_CreateMaterial(glDirect3D1 *This, LPDIRECT3DMATERIAL* lplpDirect3DMaterial, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D1_CreateViewport(glDirect3D1 *This, LPDIRECT3DVIEWPORT* lplpD3DViewport, IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3D1_EnumDevices(glDirect3D1 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
HRESULT WINAPI glDirect3D1_FindDevice(glDirect3D1 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
HRESULT WINAPI glDirect3D1_Initialize(glDirect3D1 *This, REFIID lpREFIID);

#endif //__GLDIRECT3D_H
