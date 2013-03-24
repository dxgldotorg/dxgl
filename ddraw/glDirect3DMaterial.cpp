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
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"

glDirect3DMaterial3::glDirect3DMaterial3()
{
	TRACE_ENTER(1,14,this);
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
	glD3DM1 = NULL;
	TRACE_EXIT(-1,0);
}

glDirect3DMaterial3::~glDirect3DMaterial3()
{
	TRACE_ENTER(1,14,this);
	if(device) device->Release();
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DMaterial3::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DMaterial3)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DMaterial2)
	{
		if(glD3DM2)
		{
			*ppvObj = glD3DM2;
			glD3DM2->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DMaterial2(this);
			glD3DM2 = (glDirect3DMaterial2*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DMaterial)
	{
		if(glD3DM1)
		{
			*ppvObj = glD3DM1;
			glD3DM1->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DMaterial1(this);
			glD3DM1 = (glDirect3DMaterial1*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DMaterial3::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DMaterial3::Release()
{
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial3::GetHandle(LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice3 *dev3 = (glDirect3DDevice3*)lpDirect3DDevice;
	if(handle)
	{
		if(device != dev3->GetGLD3DDev7()) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		*lpHandle = handle;
		TRACE_VAR("*lpHandle",9,*lpHandle);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	device = dev3->GetGLD3DDev7();
	handle = device->AddMaterial(this);
	if(handle == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	*lpHandle = handle;
	TRACE_VAR("*lpHandle",9,*lpHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3::GetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpMat->dwSize < sizeof(D3DMATERIAL)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpMat,&material,sizeof(D3DMATERIAL));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3::SetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpMat->dwSize != sizeof(D3DMATERIAL)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&material,lpMat,sizeof(D3DMATERIAL));
	if(device && current) Sync();
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

void glDirect3DMaterial3::unbind()
{
	TRACE_ENTER(1,14,this);
	device = NULL;
	current = false;
	handle = 0;
	TRACE_EXIT(0,0);
}

void glDirect3DMaterial3::SetCurrent(bool current)
{
	TRACE_ENTER(2,14,this,21,current);
	if(this->current == current)
	{
		TRACE_EXIT(0,0);
		return;
	}
	this->current = current;
	if(current) Sync();
	TRACE_EXIT(0,0);
}

void glDirect3DMaterial3::Sync()
{
	TRACE_ENTER(1,14,this);
	D3DMATERIAL7 mat7;
	mat7.diffuse = material.diffuse;
	mat7.ambient = material.ambient;
	mat7.specular = material.specular;
	mat7.emissive = material.emissive;
	mat7.power = material.power;
	device->SetMaterial(&mat7);
	TRACE_EXIT(0,0);
}

glDirect3DMaterial2::glDirect3DMaterial2(glDirect3DMaterial3 *glD3DM3)
{
	TRACE_ENTER(2,14,this,14,glD3DM3);
	refcount = 1;
	this->glD3DM3 = glD3DM3;
	TRACE_EXIT(-1,0);
}

glDirect3DMaterial2::~glDirect3DMaterial2()
{
	TRACE_ENTER(1,14,this);
	glD3DM3->glD3DM2 = NULL;
	glD3DM3->Release();
	TRACE_EXIT(-1,0);
}

ULONG WINAPI glDirect3DMaterial2::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DMaterial2::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial2::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glD3DM3->QueryInterface(riid,ppvObj));
}

HRESULT WINAPI glDirect3DMaterial2::GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice2 *glD3DDev2 = (glDirect3DDevice2*)lpDirect3DDevice;
	glDirect3DDevice3 *glD3DDev3;
	glD3DDev2->QueryInterface(IID_IDirect3DDevice3,(void**)&glD3DDev3);
	HRESULT ret = glD3DM3->GetHandle(glD3DDev3,lpHandle);
	glD3DDev3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial2::GetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DM3->GetMaterial(lpMat));
}
HRESULT WINAPI glDirect3DMaterial2::SetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DM3->SetMaterial(lpMat));
}

glDirect3DMaterial1::glDirect3DMaterial1(glDirect3DMaterial3 *glD3DM3)
{
	TRACE_ENTER(2,14,this,14,glD3DM3);
	refcount = 1;
	this->glD3DM3 = glD3DM3;
	TRACE_EXIT(-1,0);
}

glDirect3DMaterial1::~glDirect3DMaterial1()
{
	TRACE_ENTER(1,14,this);
	glD3DM3->glD3DM1 = NULL;
	glD3DM3->Release();
	TRACE_EXIT(-1,0);
}

ULONG WINAPI glDirect3DMaterial1::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DMaterial1::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial1::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glD3DM3->QueryInterface(riid,ppvObj));
}

HRESULT WINAPI glDirect3DMaterial1::GetHandle(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice1 *glD3DDev1 = (glDirect3DDevice1*)lpDirect3DDevice;
	glDirect3DDevice3 *glD3DDev3;
	glD3DDev1->QueryInterface(IID_IDirect3DDevice3,(void**)&glD3DDev3);
	HRESULT ret = glD3DM3->GetHandle(glD3DDev3,lpHandle);
	glD3DDev3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial1::GetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DM3->GetMaterial(lpMat));
}

HRESULT WINAPI glDirect3DMaterial1::Initialize(LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,this,14,lpDirect3D);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DMaterial1::Reserve()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI glDirect3DMaterial1::SetMaterial(LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,this,14,lpMat);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DM3->SetMaterial(lpMat));
}

HRESULT WINAPI glDirect3DMaterial1::Unreserve()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
