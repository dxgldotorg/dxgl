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

#include "common.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"

glDirect3DMaterial3::glDirect3DMaterial3()
{
	device = NULL;
	refcount = 1;	
	material.dwSize = sizeof(D3DMATERIAL);
	const D3DCOLORVALUE defaultcolor = {1.0f,1.0f,1.0f,1.0f};
	material.diffuse = material.ambient = material.specular =
		material.emissive = defaultcolor;
	material.power = 0.0f;
	material.hTexture = 0;
	material.dwRampSize = 0;
	handle = 0;
}

glDirect3DMaterial3::~glDirect3DMaterial3()
{
	if(device) device->Release();
}

HRESULT WINAPI glDirect3DMaterial3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DMaterial3::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DMaterial3::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DMaterial3::GetHandle(LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DDevice) return DDERR_INVALIDOBJECT;
	glDirect3DDevice3 *dev3 = (glDirect3DDevice3*)lpDirect3DDevice;
	if(handle)
	{
		if(device != dev3->GetGLD3DDev7()) return DDERR_INVALIDOBJECT;
		*lpHandle = handle;
		return D3D_OK;
	}
	device = dev3->GetGLD3DDev7();
	handle = device->AddMaterial(this);
	*lpHandle = handle;
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3::GetMaterial(LPD3DMATERIAL lpMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(lpMat->dwSize < sizeof(D3DMATERIAL)) return DDERR_INVALIDPARAMS;
	memcpy(lpMat,&material,sizeof(D3DMATERIAL));
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3::SetMaterial(LPD3DMATERIAL lpMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(lpMat->dwSize != sizeof(D3DMATERIAL)) return DDERR_INVALIDPARAMS;
	memcpy(&material,lpMat,sizeof(D3DMATERIAL));
	return D3D_OK;
}

void glDirect3DMaterial3::unbind()
{
	device = NULL;
	handle = 0;
}