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
#include "glDirect3DViewport.h"
#include "glDirect3DLight.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "ddraw.h"

glDirect3DLightVtbl glDirect3DLight_impl =
{
	glDirect3DLight_QueryInterface,
	glDirect3DLight_AddRef,
	glDirect3DLight_Release,
	glDirect3DLight_Initialize,
	glDirect3DLight_SetLight,
	glDirect3DLight_GetLight
};

HRESULT glDirect3DLight_Create(D3DLIGHT7 *light_in, glDirect3DLight **light)
{
	TRACE_ENTER(2, 14, light_in, 14, light);
	glDirect3DLight *This = (glDirect3DLight*)malloc(sizeof(glDirect3DLight));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DLight_impl;
	This->refcount = 1;
	memcpy(&This->light, light_in, sizeof(D3DLIGHT7));
	*light = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}
HRESULT glDirect3DLight_CreateNoLight(glDirect3DLight **light)
{
	D3DVECTOR dir = { 0,0,1.0 };
	TRACE_ENTER(1, 14, light);
	glDirect3DLight *This = (glDirect3DLight*)malloc(sizeof(glDirect3DLight));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DLight_impl;
	This->refcount = 1;
	This->viewport = NULL;
	This->device = NULL;
	ZeroMemory(&This->light, sizeof(D3DLIGHT7));
	This->light.dltType = D3DLIGHT_DIRECTIONAL;
	This->light.dcvDiffuse.r = This->light.dcvDiffuse.g = This->light.dcvDiffuse.b = 1.0f;
	This->light.dvDirection = dir;
	*light = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

ULONG WINAPI glDirect3DLight_AddRef(glDirect3DLight *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement(&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}
ULONG WINAPI glDirect3DLight_Release(glDirect3DLight *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement(&This->refcount);
	ret = This->refcount;
	if(This->refcount == 0) free(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DLight_Initialize(glDirect3DLight *This, LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,This,14,lpDirect3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DLight_QueryInterface(glDirect3DLight *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3DLight))
	{
		glDirect3DLight_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

void glDirect3DLight_GetLight7(glDirect3DLight *This, LPD3DLIGHT7 lpLight7)
{
	TRACE_ENTER(2,14,This,14,lpLight7);
	memcpy(lpLight7,&This->light,sizeof(D3DLIGHT7));
	TRACE_EXIT(0,0);
}
void glDirect3DLight_SetLight7(glDirect3DLight *This, LPD3DLIGHT7 lpLight7)
{
	TRACE_ENTER(2,14,This,14,lpLight7);
	memcpy(&This->light,lpLight7,sizeof(D3DLIGHT7));
	TRACE_EXIT(0,0);
}

HRESULT WINAPI glDirect3DLight_GetLight(glDirect3DLight *This, LPD3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,This,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpLight->dwSize < sizeof(D3DLIGHT)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	lpLight->dltType = This->light.dltType;
	lpLight->dcvColor = This->light.dcvDiffuse;
	lpLight->dvPosition = This->light.dvPosition;
	lpLight->dvDirection = This->light.dvDirection;
	lpLight->dvRange = This->light.dvRange;
	lpLight->dvFalloff = This->light.dvFalloff;
	lpLight->dvAttenuation0 = This->light.dvAttenuation0;
	lpLight->dvAttenuation1 = This->light.dvAttenuation1;
	lpLight->dvAttenuation2 = This->light.dvAttenuation2;
	lpLight->dvTheta = This->light.dvTheta;
	lpLight->dvPhi = This->light.dvPhi;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DLight_SetLight(glDirect3DLight *This, LPD3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,This,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpLight->dwSize < sizeof(D3DLIGHT)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->light.dltType = lpLight->dltType;
	This->light.dcvDiffuse = lpLight->dcvColor;
	This->light.dcvAmbient.r = This->light.dcvAmbient.g = This->light.dcvAmbient.b = This->light.dcvAmbient.a = 0;
	This->light.dvPosition = lpLight->dvPosition;
	This->light.dvDirection = lpLight->dvDirection;
	This->light.dvRange = lpLight->dvRange;
	This->light.dvFalloff = lpLight->dvFalloff;
	This->light.dvAttenuation0 = lpLight->dvAttenuation0;
	This->light.dvAttenuation1 = lpLight->dvAttenuation1;
	This->light.dvAttenuation2 = lpLight->dvAttenuation2;
	This->light.dvTheta = lpLight->dvTheta;
	This->light.dvPhi = lpLight->dvPhi;
	BOOL enablelight = FALSE;
	if(This->device && (lpLight->dwSize >= sizeof(D3DLIGHT2))) enablelight = TRUE;
	else if(This->device) enablelight = TRUE;
	if(enablelight)
	{
		if((((LPD3DLIGHT2)lpLight)->dwFlags & D3DLIGHT_ACTIVE) || lpLight->dwSize == sizeof(D3DLIGHT))
		{
			glDirect3DDevice7_SetLight(This->device, This->index, &This->light);
			glDirect3DDevice7_LightEnable(This->device, This->index, TRUE);
		}
		else glDirect3DDevice7_LightEnable(This->device, This->index, FALSE);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

void glDirect3DLight_SetDevice(glDirect3DLight *This, glDirect3DDevice7 *device, int index)
{
	TRACE_ENTER(3,14,This,14,device,11,index);
	This->device = device;
	This->index = index;
	TRACE_EXIT(0,0);
}

void glDirect3DLight_Sync(glDirect3DLight *This)
{
	TRACE_ENTER(1,14,This);
	if(!This)
	{
		TRACE_EXIT(0,0);
		return;
	}
	glDirect3DDevice7_SetLight(This->device, This->index, &This->light);
	glDirect3DDevice7_LightEnable(This->device, This->index, TRUE);
	TRACE_EXIT(0,0);
}