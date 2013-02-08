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

#include "common.h"
#include "texture.h"
#include "glRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirect3DDevice.h"

glDirect3DTexture2::glDirect3DTexture2(glDirectDrawSurface7 *glDDS7)
{
	this->glDDS7 = glDDS7;
	refcount = 1;
}

glDirect3DTexture2::~glDirect3DTexture2()
{
	glDDS7->Release();
}

HRESULT WINAPI glDirect3DTexture2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glDDS7->QueryInterface(riid,ppvObj);
}

ULONG WINAPI glDirect3DTexture2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DTexture2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DTexture2::GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice2, LPD3DTEXTUREHANDLE lpHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DDevice2) return DDERR_INVALIDPARAMS;
	glDirect3DDevice7 *glD3DDev7;
	lpDirect3DDevice2->QueryInterface(IID_IDirect3DDevice7,(void**)&glD3DDev7);
	HRESULT ret = glDDS7->GetHandle(glD3DDev7,lpHandle);
	glD3DDev7->Release();
	return ret;
}
HRESULT WINAPI glDirect3DTexture2::Load(LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DTexture2::Load: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DTexture2::PaletteChanged(DWORD dwStart, DWORD dwCount)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DTexture2::PaletteChanged: stub");
	return DDERR_GENERIC;
}

glDirect3DTexture1::glDirect3DTexture1(glDirectDrawSurface7 *glDDS7)
{
	this->glDDS7 = glDDS7;
	refcount = 1;
}
glDirect3DTexture1::~glDirect3DTexture1()
{
	glDDS7->Release();
}
HRESULT WINAPI glDirect3DTexture1::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glDDS7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirect3DTexture1::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DTexture1::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DTexture1::GetHandle(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DDevice) return DDERR_INVALIDPARAMS;
	glDirect3DDevice7 *glD3DDev7;
	lpDirect3DDevice->QueryInterface(IID_IDirect3DDevice7,(void**)&glD3DDev7);
	HRESULT ret = glDDS7->GetHandle(glD3DDev7,lpHandle);
	glD3DDev7->Release();
	return ret;
}
HRESULT WINAPI glDirect3DTexture1::Initialize(LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirect3DTexture1::Load(LPDIRECT3DTEXTURE lpD3DTexture)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DTexture1::Load: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DTexture1::PaletteChanged(DWORD dwStart, DWORD dwCount)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DTexture1::PaletteChanged: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DTexture1::Unload()
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DTexture1::Unload: stub");
	return DDERR_GENERIC;
}
