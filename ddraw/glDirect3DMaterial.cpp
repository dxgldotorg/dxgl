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
#include "texture.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"

glDirect3DMaterial3::glDirect3DMaterial3()
{
	device = NULL;
	current = FALSE;
	refcount = 1;	
	material.dwSize = sizeof(D3DMATERIAL);
	const D3DCOLORVALUE defaultcolor = {1.0f,1.0f,1.0f,1.0f};
	material.diffuse = material.ambient = material.specular =
		material.emissive = defaultcolor;
	material.power = 0.0f;
	material.hTexture = 0;
	material.dwRampSize = 0;
	handle = 0;
	glD3DM2 = NULL;
}

glDirect3DMaterial3::~glDirect3DMaterial3()
{
	if(device) device->Release();
}

HRESULT WINAPI glDirect3DMaterial3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	if(riid == IID_IDirect3DMaterial3)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	if(riid == IID_IDirect3DMaterial2)
	{
		if(glD3DM2)
		{
			*ppvObj = glD3DM2;
			glD3DM2->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DMaterial2(this);
			glD3DM2 = (glDirect3DMaterial2*)*ppvObj;
			return D3D_OK;
		}
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
	if(!lpDirect3DDevice) return DDERR_INVALIDPARAMS;
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
	if(device && current) Sync();
	return D3D_OK;
}

void glDirect3DMaterial3::unbind()
{
	device = NULL;
	current = false;
	handle = 0;
}

void glDirect3DMaterial3::SetCurrent(bool current)
{
	if(this->current == current) return;
	this->current = current;
	if(current) Sync();
}

void glDirect3DMaterial3::Sync()
{
	D3DMATERIAL7 mat7;
	mat7.diffuse = material.diffuse;
	mat7.ambient = material.ambient;
	mat7.specular = material.specular;
	mat7.emissive = material.emissive;
	mat7.power = material.power;
	device->SetMaterial(&mat7);
}

glDirect3DMaterial2::glDirect3DMaterial2(glDirect3DMaterial3 *glD3DM3)
{
	refcount = 1;
	this->glD3DM3 = glD3DM3;
}

glDirect3DMaterial2::~glDirect3DMaterial2()
{
	glD3DM3->glD3DM2 = NULL;
	glD3DM3->Release();
}

ULONG WINAPI glDirect3DMaterial2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DMaterial2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DMaterial2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return glD3DM3->QueryInterface(riid,ppvObj);
}

HRESULT WINAPI glDirect3DMaterial2::GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DDevice) return DDERR_INVALIDPARAMS;
	glDirect3DDevice2 *glD3DDev2 = (glDirect3DDevice2*)lpDirect3DDevice;
	glDirect3DDevice3 *glD3DDev3;
	glD3DDev2->QueryInterface(IID_IDirect3DDevice3,(void**)&glD3DDev3);
	HRESULT ret = glD3DM3->GetHandle(glD3DDev3,lpHandle);
	glD3DDev3->Release();
	return ret;
}

HRESULT WINAPI glDirect3DMaterial2::GetMaterial(LPD3DMATERIAL lpMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DM3->GetMaterial(lpMat);
}
HRESULT WINAPI glDirect3DMaterial2::SetMaterial(LPD3DMATERIAL lpMat)
{
	if(!this) return  DDERR_INVALIDOBJECT;
	return glD3DM3->SetMaterial(lpMat);
}
