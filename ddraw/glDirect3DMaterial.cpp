// DXGL
// Copyright (C) 2012-2021 William Feely

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
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"
#include "ddraw.h"

glDirect3DMaterial3Vtbl glDirect3DMaterial3_impl =
{
	glDirect3DMaterial3_QueryInterface,
	glDirect3DMaterial3_AddRef,
	glDirect3DMaterial3_Release,
	glDirect3DMaterial3_SetMaterial,
	glDirect3DMaterial3_GetMaterial,
	glDirect3DMaterial3_GetHandle
};

HRESULT glDirect3DMaterial3_Create(glDirect3DMaterial3 **material)
{
	HRESULT error;
	TRACE_ENTER(1,14,material);
	glDirect3DMaterial3 *This = (glDirect3DMaterial3*)malloc(sizeof(glDirect3DMaterial3));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DMaterial3_impl;
	This->device = NULL;
	This->current = FALSE;
	This->refcount = 1;	
	This->material.dwSize = sizeof(D3DMATERIAL);
	const D3DCOLORVALUE defaultcolor = {1.0f,1.0f,1.0f,1.0f};
	This->material.diffuse = This->material.ambient = This->material.specular =
		This->material.emissive = defaultcolor;
	This->material.power = 0.0f;
	This->material.hTexture = 0;
	This->material.dwRampSize = 0;
	This->handle = 0;
	error = glDirect3DMaterial2_Create(This, &This->glD3DM2);
	if (FAILED(error))
	{
		free(This);
		return error;
	}
	error = glDirect3DMaterial1_Create(This, &This->glD3DM1);
	if (FAILED(error))
	{
		free(This->glD3DM2);
		free(This);
		return error;
	}
	*material = This;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

void glDirect3DMaterial3_Destroy(glDirect3DMaterial3 *This)
{
	TRACE_ENTER(1,14,This);
	if(This->device) glDirect3DDevice7_Release(This->device);
	free(This->glD3DM2);
	free(This->glD3DM1);
	free(This);
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DMaterial3_QueryInterface(glDirect3DMaterial3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DMaterial3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DMaterial3)
	{
		glDirect3DMaterial3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DMaterial2)
	{
		glDirect3DMaterial3_AddRef(This);
		*ppvObj = This->glD3DM2;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DMaterial)
	{
		glDirect3DMaterial3_AddRef(This);
		*ppvObj = This->glD3DM1;
		TRACE_VAR("*ppvObj", 14, *ppvObj);
		TRACE_EXIT(23, D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23, E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DMaterial3_AddRef(glDirect3DMaterial3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}

ULONG WINAPI glDirect3DMaterial3_Release(glDirect3DMaterial3 *This)
{
	TRACE_ENTER(1, 14, This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement((LONG*)&This->refcount);
	ret = This->refcount;
	if (This->refcount == 0) glDirect3DMaterial3_Destroy(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial3_GetHandle(glDirect3DMaterial3 *This, LPDIRECT3DDEVICE3 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice3 *dev3 = (glDirect3DDevice3*)lpDirect3DDevice;
	if(This->handle)
	{
		if(This->device != dev3->glD3DDev7) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
		*lpHandle = This->handle;
		TRACE_VAR("*lpHandle",9,*lpHandle);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	This->device = dev3->glD3DDev7;
	This->handle = glDirect3DDevice7_AddMaterial(This->device, This);
	if(This->handle == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	*lpHandle = This->handle;
	TRACE_VAR("*lpHandle",9,*lpHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3_GetMaterial(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpMat->dwSize < sizeof(D3DMATERIAL)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpMat,&This->material,sizeof(D3DMATERIAL));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DMaterial3_SetMaterial(glDirect3DMaterial3 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpMat->dwSize != sizeof(D3DMATERIAL)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&This->material,lpMat,sizeof(D3DMATERIAL));
	if(This->device && This->current) glDirect3DMaterial3_Sync(This);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

void glDirect3DMaterial3_unbind(glDirect3DMaterial3 *This)
{
	TRACE_ENTER(1,14,This);
	This->device = NULL;
	This->current = false;
	This->handle = 0;
	TRACE_EXIT(0,0);
}

void glDirect3DMaterial3_SetCurrent(glDirect3DMaterial3 *This, BOOL current)
{
	TRACE_ENTER(2,14,This,21,current);
	if(This->current == current)
	{
		TRACE_EXIT(0,0);
		return;
	}
	This->current = current;
	if(current) glDirect3DMaterial3_Sync(This);
	TRACE_EXIT(0,0);
}

void glDirect3DMaterial3_Sync(glDirect3DMaterial3 *This)
{
	TRACE_ENTER(1,14,This);
	D3DMATERIAL7 mat7;
	mat7.diffuse = This->material.diffuse;
	mat7.ambient = This->material.ambient;
	mat7.specular = This->material.specular;
	mat7.emissive = This->material.emissive;
	mat7.power = This->material.power;
	glDirect3DDevice7_SetMaterial(This->device, &mat7);
	TRACE_EXIT(0,0);
}

glDirect3DMaterial2Vtbl glDirect3DMaterial2_impl =
{
	glDirect3DMaterial2_QueryInterface,
	glDirect3DMaterial2_AddRef,
	glDirect3DMaterial2_Release,
	glDirect3DMaterial2_SetMaterial,
	glDirect3DMaterial2_GetMaterial,
	glDirect3DMaterial2_GetHandle
};


HRESULT glDirect3DMaterial2_Create(glDirect3DMaterial3 *glD3DM3, glDirect3DMaterial2 **material)
{
	TRACE_ENTER(2, 14, glD3DM3, 14, material);
	glDirect3DMaterial2 *This;
	This = (glDirect3DMaterial2*)malloc(sizeof(glDirect3DMaterial2));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DMaterial2_impl;
	This->glD3DM3 = glD3DM3;
	*material = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

ULONG WINAPI glDirect3DMaterial2_AddRef(glDirect3DMaterial2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DMaterial3_AddRef(This->glD3DM3));
}

ULONG WINAPI glDirect3DMaterial2_Release(glDirect3DMaterial2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DMaterial3_Release(This->glD3DM3));
}

HRESULT WINAPI glDirect3DMaterial2_QueryInterface(glDirect3DMaterial2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DMaterial2))
	{
		glDirect3DMaterial2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_QueryInterface(This->glD3DM3,riid,ppvObj));
}

HRESULT WINAPI glDirect3DMaterial2_GetHandle(glDirect3DMaterial2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice2 *glD3DDev2 = (glDirect3DDevice2*)lpDirect3DDevice;
	glDirect3DDevice3 *glD3DDev3 = glD3DDev2->glD3DDev7->glD3DDev3;
	HRESULT ret = glDirect3DMaterial3_GetHandle(This->glD3DM3,(LPDIRECT3DDEVICE3)glD3DDev3,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial2_GetMaterial(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_GetMaterial(This->glD3DM3,lpMat));
}
HRESULT WINAPI glDirect3DMaterial2_SetMaterial(glDirect3DMaterial2 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_SetMaterial(This->glD3DM3,lpMat));
}

glDirect3DMaterial1Vtbl glDirect3DMaterial1_impl =
{
	glDirect3DMaterial1_QueryInterface,
	glDirect3DMaterial1_AddRef,
	glDirect3DMaterial1_Release,
	glDirect3DMaterial1_Initialize,
	glDirect3DMaterial1_SetMaterial,
	glDirect3DMaterial1_GetMaterial,
	glDirect3DMaterial1_GetHandle,
	glDirect3DMaterial1_Reserve,
	glDirect3DMaterial1_Unreserve,
};

HRESULT glDirect3DMaterial1_Create(glDirect3DMaterial3 *glD3DM3, glDirect3DMaterial1 **material)
{
	TRACE_ENTER(2,14,glD3DM3,14,material);
	glDirect3DMaterial1 *This;
	This = (glDirect3DMaterial1*)malloc(sizeof(glDirect3DMaterial1));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DMaterial1_impl;
	This->glD3DM3 = glD3DM3;
	*material = This;
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}

ULONG WINAPI glDirect3DMaterial1_AddRef(glDirect3DMaterial1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DMaterial3_AddRef(This->glD3DM3));
}

ULONG WINAPI glDirect3DMaterial1_Release(glDirect3DMaterial1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DMaterial3_Release(This->glD3DM3));
}

HRESULT WINAPI glDirect3DMaterial1_QueryInterface(glDirect3DMaterial1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DMaterial))
	{
		glDirect3DMaterial1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_QueryInterface(This->glD3DM3,riid,ppvObj));
}

HRESULT WINAPI glDirect3DMaterial1_GetHandle(glDirect3DMaterial1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DMATERIALHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice1 *glD3DDev1 = (glDirect3DDevice1*)lpDirect3DDevice;
	glDirect3DDevice3 *glD3DDev3 = glD3DDev1->glD3DDev7->glD3DDev3;
	HRESULT ret = glDirect3DMaterial3_GetHandle(This->glD3DM3,(LPDIRECT3DDEVICE3)glD3DDev3,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DMaterial1_GetMaterial(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_GetMaterial(This->glD3DM3,lpMat));
}

HRESULT WINAPI glDirect3DMaterial1_Initialize(glDirect3DMaterial1 *This, LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,This,14,lpDirect3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DMaterial1_Reserve(glDirect3DMaterial1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}

HRESULT WINAPI glDirect3DMaterial1_SetMaterial(glDirect3DMaterial1 *This, LPD3DMATERIAL lpMat)
{
	TRACE_ENTER(2,14,This,14,lpMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DMaterial3_SetMaterial(This->glD3DM3,lpMat));
}

HRESULT WINAPI glDirect3DMaterial1_Unreserve(glDirect3DMaterial1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
