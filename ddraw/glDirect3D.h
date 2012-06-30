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
#ifndef __GLDIRECT3D_H
#define __GLDIRECT3D_H

extern D3DDEVICEDESC7 d3ddesc;
class glDirectDraw7;

class glDirect3D7 : public IDirect3D7
{
public:
	glDirect3D7(glDirectDraw7 *glDD7);
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
	glDirectDraw7 *glDD7;
private:
	ULONG refcount;
};

class glDirect3D3 : public IDirect3D3
{
public:
	glDirect3D3(glDirect3D7 *glDD7);
	virtual ~glDirect3D3();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *  lplpD3DDevice, LPUNKNOWN pUnkOuter);
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


#endif //__GLDIRECT3D_H