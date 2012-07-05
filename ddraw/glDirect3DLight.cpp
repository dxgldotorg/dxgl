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
#include "glDirect3DViewport.h"
#include "glDirect3DLight.h"
#define _USE_MATH_DEFINES
#include <math.h>



glDirect3DLight::glDirect3DLight()
{
	refcount=1;
	viewport = NULL;
	device = NULL;
	ZeroMemory(&light,sizeof(D3DLIGHT7));
	light.dltType = D3DLIGHT_DIRECTIONAL;
	light.dcvAmbient.r = light.dcvAmbient.g = light.dcvAmbient.b = 1.0f;
	light.dvDirection = D3DVECTOR(0,0,1.0);
}
glDirect3DLight::glDirect3DLight(D3DLIGHT7 *light_in)
{
	refcount=1;
	memcpy(&light,light_in,sizeof(D3DLIGHT7));
}

glDirect3DLight::~glDirect3DLight()
{
}

ULONG WINAPI glDirect3DLight::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DLight::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DLight::Initialize(LPDIRECT3D lpDirect3D)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI glDirect3DLight::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return E_NOINTERFACE;
}

void glDirect3DLight::GetLight7(LPD3DLIGHT7 lpLight7)
{
	memcpy(lpLight7,&light,sizeof(D3DLIGHT7));
}
void glDirect3DLight::SetLight7(LPD3DLIGHT7 lpLight7)
{
	memcpy(&light,lpLight7,sizeof(D3DLIGHT7));
}

HRESULT WINAPI glDirect3DLight::GetLight(LPD3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpLight) return DDERR_INVALIDPARAMS;
	if(lpLight->dwSize < sizeof(D3DLIGHT)) return DDERR_INVALIDPARAMS;
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
	return D3D_OK;
}
HRESULT WINAPI glDirect3DLight::SetLight(LPD3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpLight) return DDERR_INVALIDPARAMS;
	if(lpLight->dwSize < sizeof(D3DLIGHT)) return DDERR_INVALIDPARAMS;
	light.dltType = lpLight->dltType;
	light.dcvDiffuse = lpLight->dcvColor;
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
		if(((LPD3DLIGHT2)lpLight)->dwFlags & D3DLIGHT_ACTIVE)
		{
			device->SetLight(index,&light);
			device->LightEnable(index,TRUE);
		}
		else device->LightEnable(index,FALSE);
	}
	return D3D_OK;
}

void glDirect3DLight::SetDevice(glDirect3DDevice7 *device, int index)
{
	this->device = device;
	this->index = index;
}