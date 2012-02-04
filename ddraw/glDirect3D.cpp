// DXGL
// Copyright (C) 2011 William Feely

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
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"

glDirect3D7::glDirect3D7(glDirectDraw7 *glDD7)
{
	refcount=1;
	this->glDD7 = glDD7;
	glDD7->AddRef();
}

glDirect3D7::~glDirect3D7()
{
	glDD7->Release();
}

ULONG WINAPI glDirect3D7::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3D7::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3D7::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::QueryInterface: stub");
	return E_NOINTERFACE;
}


HRESULT WINAPI glDirect3D7::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice)
{
	if(!this) return DDERR_INVALIDPARAMS;
	glDirect3DDevice7 *glD3DDev7 = new glDirect3DDevice7(this,(glDirectDrawSurface7*)lpDDS);
	*lplpD3DDevice = (LPDIRECT3DDEVICE7) glD3DDev7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::CreateVertexBuffer: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3D7::EnumDevices(LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::EnumDevices: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3D7::EnumZBufferFormats(REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	if(!this) return DDERR_INVALIDPARAMS;
	DDPIXELFORMAT ddpf;
	ZeroMemory(&ddpf,sizeof(DDPIXELFORMAT));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	ddpf.dwFlags = DDPF_ZBUFFER;
	ddpf.dwZBufferBitDepth = 16;
	ddpf.dwZBitMask = 0xffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBufferBitDepth = 24;
	ddpf.dwZBitMask = 0xffffff00;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBufferBitDepth = 32;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBitMask = 0xffffffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	if(GLEXT_EXT_packed_depth_stencil || GLEXT_NV_packed_depth_stencil)
	{
		ddpf.dwZBufferBitDepth = 32;
		ddpf.dwStencilBitDepth = 8;
		ddpf.dwZBitMask = 0xffffff00;
		ddpf.dwStencilBitMask = 0xff;
		if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
		ddpf.dwZBitMask = 0x00ffffff;
		ddpf.dwStencilBitMask = 0xff000000;
		if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	}
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::EvictManagedTextures()
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::EvictManagedTextures: stub");
	return DDERR_GENERIC;
}
