// DXGL
// Copyright (C) 2011-2013 William Feely

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

extern D3DDEVICEDESC7 d3ddesc;
extern D3DDEVICEDESC d3ddesc3;
class glDirectDraw7;

class glDirect3D3;
class glDirect3D2;
class glDirect3D1;
class glDirect3D7 : public IDirect3D7
{
public:
	glDirect3D7();
	virtual ~glDirect3D7();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice);
	HRESULT WINAPI CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateMaterial(LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags);
	HRESULT WINAPI CreateViewport(LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter);
	HRESULT WINAPI EnumDevices(LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT WINAPI EnumDevices3(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT WINAPI EnumZBufferFormats(REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
	HRESULT WINAPI EvictManagedTextures();
	HRESULT WINAPI FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
	glDirect3D3 *glD3D3;
	glDirect3D2 *glD3D2;
	glDirect3D1 *glD3D1;
private:
	ULONG refcount;
};

class glDirect3D3 : public IDirect3D3
{
public:
	glDirect3D3(glDirect3D7 *glD3D7);
	virtual ~glDirect3D3();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice, LPUNKNOWN pUnkOuter);
	HRESULT WINAPI CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateMaterial(LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER* lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter);
	HRESULT WINAPI CreateViewport(LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter);
	HRESULT WINAPI EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT WINAPI EnumZBufferFormats(REFCLSID riidDevice,  LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext);
	HRESULT WINAPI EvictManagedTextures();
	HRESULT WINAPI FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
private:
	ULONG refcount;
	glDirect3D7 *glD3D7;
};

class glDirect3D2 : public IDirect3D2
{
public:
	glDirect3D2(glDirect3D7 *glD3D7);
	virtual ~glDirect3D2();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice2);
	HRESULT WINAPI CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateMaterial(LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateViewport(LPDIRECT3DVIEWPORT2* lplpD3DViewport2, IUnknown* pUnkOuter);
	HRESULT WINAPI EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT WINAPI FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
private:
	ULONG refcount;
	glDirect3D7 *glD3D7;
};

class glDirect3D1 : public IDirect3D
{
public:
	glDirect3D1(glDirect3D7 *glD3D7);
	virtual ~glDirect3D1();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateMaterial(LPDIRECT3DMATERIAL* lplpDirect3DMaterial, IUnknown* pUnkOuter);
	HRESULT WINAPI CreateViewport(LPDIRECT3DVIEWPORT* lplpD3DViewport, IUnknown* pUnkOuter);
	HRESULT WINAPI EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg);
	HRESULT WINAPI FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR);
	HRESULT WINAPI Initialize(REFIID lpREFIID);
private:
	ULONG refcount;
	glDirect3D7 *glD3D7;
};

#endif //__GLDIRECT3D_H