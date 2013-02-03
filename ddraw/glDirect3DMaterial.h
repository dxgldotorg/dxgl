// DXGL
// Copyright (C) 2012-2013 William Feely

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

class glDirect3DMaterial1;
class glDirect3DMaterial2;
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
	D3DMATERIALHANDLE handle;
	glDirect3DMaterial2 *glD3DM2;
	glDirect3DMaterial1 *glD3DM1;
private:
	ULONG refcount;
	bool current;
	glDirect3DDevice7 *device;
};

class glDirect3DMaterial2 : public IDirect3DMaterial2
{
public:
	glDirect3DMaterial2(glDirect3DMaterial3 *glD3DM3);
	virtual ~glDirect3DMaterial2();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
	HRESULT WINAPI GetMaterial(LPD3DMATERIAL lpMat);
	HRESULT WINAPI SetMaterial(LPD3DMATERIAL lpMat);
private:
	ULONG refcount;
	glDirect3DMaterial3 *glD3DM3;
};

class glDirect3DMaterial1 : public IDirect3DMaterial
{
public:
	glDirect3DMaterial1(glDirect3DMaterial3 *glD3DM3);
	virtual ~glDirect3DMaterial1();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetHandle(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle);
	HRESULT WINAPI GetMaterial(LPD3DMATERIAL lpMat);
	HRESULT WINAPI Initialize(LPDIRECT3D lpDirect3D);
	HRESULT WINAPI Reserve();
	HRESULT WINAPI SetMaterial(LPD3DMATERIAL lpMat);
	HRESULT WINAPI Unreserve();
private:
	ULONG refcount;
	glDirect3DMaterial3 *glD3DM3;
};

#endif //__GLDIRECT3DMATERIAL_H