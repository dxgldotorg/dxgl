// DXGL
// Copyright (C) 2012-2014 William Feely

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



glDirect3DLight::glDirect3DLight()
{
	D3DVECTOR dir = { 0,0,1.0 };
	TRACE_ENTER(1,14,this);
	refcount=1;
	viewport = NULL;
	device = NULL;
	ZeroMemory(&light,sizeof(D3DLIGHT7));
	light.dltType = D3DLIGHT_DIRECTIONAL;
	light.dcvDiffuse.r = light.dcvDiffuse.g = light.dcvDiffuse.b = 1.0f;
	light.dvDirection = dir;
	TRACE_EXIT(-1,0);
}
glDirect3DLight::glDirect3DLight(D3DLIGHT7 *light_in)
{
	TRACE_ENTER(2,14,this,14,light_in);
	refcount=1;
	memcpy(&light,light_in,sizeof(D3DLIGHT7));
	TRACE_EXIT(-1,0);
}

glDirect3DLight::~glDirect3DLight()
{
	TRACE_ENTER(1,14,this);
	TRACE_EXIT(-1,0);
}

ULONG WINAPI glDirect3DLight::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glDirect3DLight::Release()
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

HRESULT WINAPI glDirect3DLight::Initialize(LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,this,14,lpDirect3D);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DLight::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

void glDirect3DLight::GetLight7(LPD3DLIGHT7 lpLight7)
{
	TRACE_ENTER(2,14,this,14,lpLight7);
	memcpy(lpLight7,&light,sizeof(D3DLIGHT7));
	TRACE_EXIT(0,0);
}
void glDirect3DLight::SetLight7(LPD3DLIGHT7 lpLight7)
{
	TRACE_ENTER(2,14,this,14,lpLight7);
	memcpy(&light,lpLight7,sizeof(D3DLIGHT7));
	TRACE_EXIT(0,0);
}

HRESULT WINAPI glDirect3DLight::GetLight(LPD3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,this,14,lpLight);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpLight->dwSize < sizeof(D3DLIGHT)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	lpLight->dltType = light.dltType;
	lpLight->dcvColor = light.dcvDiffuse;
	lpLight->dvPosition = light.dvPosition;
	lpLight->dvDirection = light.dvDirection;
	lpLight->dvRange = light.dvRange;
	lpLight->dvFalloff = light.dvFalloff;
	lpLight->dvAttenuation0 = light.dvAttenuation0;
	lpLight->dvAttenuation1 = light.dvAttenuation1;
	lpLight->dvAttenuation2 = light.dvAttenuation2;
	lpLight->dvTheta = light.dvTheta;
	lpLight->dvPhi = light.dvPhi;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DLight::SetLight(LPD3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,this,14,lpLight);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpLight->dwSize < sizeof(D3DLIGHT)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	light.dltType = lpLight->dltType;
	light.dcvDiffuse = lpLight->dcvColor;
	light.dcvAmbient.r = light.dcvAmbient.g = light.dcvAmbient.b = light.dcvAmbient.a = 0;
	light.dvPosition = lpLight->dvPosition;
	light.dvDirection = lpLight->dvDirection;
	light.dvRange = lpLight->dvRange;
	light.dvFalloff = lpLight->dvFalloff;
	light.dvAttenuation0 = lpLight->dvAttenuation0;
	light.dvAttenuation1 = lpLight->dvAttenuation1;
	light.dvAttenuation2 = lpLight->dvAttenuation2;
	light.dvTheta = lpLight->dvTheta;
	light.dvPhi = lpLight->dvPhi;
	bool enablelight = false;
	if(device && (lpLight->dwSize >= sizeof(D3DLIGHT2))) enablelight = true;
	else if(device) enablelight = true;
	if(enablelight)
	{
		if((((LPD3DLIGHT2)lpLight)->dwFlags & D3DLIGHT_ACTIVE) || lpLight->dwSize == sizeof(D3DLIGHT))
		{
			device->SetLight(index,&light);
			device->LightEnable(index,TRUE);
		}
		else device->LightEnable(index,FALSE);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

void glDirect3DLight::SetDevice(glDirect3DDevice7 *device, int index)
{
	TRACE_ENTER(3,14,this,14,device,11,index);
	this->device = device;
	this->index = index;
	TRACE_EXIT(0,0);
}

void glDirect3DLight::Sync()
{
	TRACE_ENTER(1,14,this);
	if(!this)
	{
		TRACE_EXIT(0,0);
		return;
	}
	device->SetLight(index,&light);
	device->LightEnable(index,TRUE);
	TRACE_EXIT(0,0);
}