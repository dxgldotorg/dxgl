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
#ifndef __GLDIRECT3DTEXTURE_H
#define __GLDIRECT3DTEXTURE_H

class glDirect3DTexture2 : public IDirect3DTexture2
{
public:
	glDirect3DTexture2(glDirectDrawSurface7 *glDDS7);
	virtual ~glDirect3DTexture2();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice2, LPD3DTEXTUREHANDLE lpHandle);
	HRESULT WINAPI Load(LPDIRECT3DTEXTURE2 lpD3DTexture2);
	HRESULT WINAPI PaletteChanged(DWORD dwStart, DWORD dwCount);
	glDirectDrawSurface7 *GetDDS7(){return glDDS7;}
private:
	glDirectDrawSurface7 *glDDS7;
	ULONG refcount;
};

class glDirect3DTexture1 : public IDirect3DTexture
{
public:
	glDirect3DTexture1(glDirectDrawSurface7 *glDDS7);
	virtual ~glDirect3DTexture1();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetHandle(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle);
	HRESULT WINAPI Initialize(LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface);
	HRESULT WINAPI Load(LPDIRECT3DTEXTURE lpD3DTexture);
	HRESULT WINAPI PaletteChanged(DWORD dwStart, DWORD dwCount);
	HRESULT WINAPI Unload();
	glDirectDrawSurface7 *GetDDS7(){return glDDS7;}
private:
	glDirectDrawSurface7 *glDDS7;
	ULONG refcount;
};

#endif //__GLDIRECT3DTEXTURE_H