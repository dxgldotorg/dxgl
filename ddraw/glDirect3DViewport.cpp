// DXGL
// Copyright (C) 2011-2013 William Feely

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
#include "glDirect3DLight.h"
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DViewport.h"

inline D3DCOLOR d3dcvtod3dcolor(D3DCOLORVALUE value)
{
	int r = (int)(value.r * 255);
	int g = (int)(value.g * 255);
	int b = (int)(value.b * 255);
	int a = (int)(value.a * 255);
	return b|(g<<8)|(r<<16)|(a<<24);
}

glDirect3DViewport3::glDirect3DViewport3()
{
	background = 0;
	ZeroMemory(&viewport,sizeof(D3DVIEWPORT2));
	viewport.dwSize = sizeof(D3DVIEWPORT2);
	maxX = maxY = scaleX = scaleY = 0;
	device = NULL;
	backZ = NULL;
	for(int i = 0; i < 8; i++)
		lights[i] = NULL;
	refcount = 1;
	current = false;
	glD3DV2 = NULL;
	glD3DV1 = NULL;
}

glDirect3DViewport3::~glDirect3DViewport3()
{
	if(device) device->Release();
	if(backZ) backZ->Release();
	for(int i = 0; i < 8; i++)
	{
		if(lights[i]) lights[i]->Release();
	}
}

HRESULT WINAPI glDirect3DViewport3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	if(riid == IID_IDirect3DViewport3)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	if(riid == IID_IDirect3DViewport2)
	{
		if(glD3DV2)
		{
			*ppvObj = glD3DV2;
			glD3DV2->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DViewport2(this);
			glD3DV2 = (glDirect3DViewport2*)*ppvObj;
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DViewport)
	{
		if(glD3DV1)
		{
			*ppvObj = glD3DV1;
			glD3DV1->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DViewport1(this);
			glD3DV1 = (glDirect3DViewport1*)*ppvObj;
			return D3D_OK;
		}
	}
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DViewport3::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DViewport3::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DViewport3::AddLight(LPDIRECT3DLIGHT lpDirect3DLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(((glDirect3DLight*)lpDirect3DLight)->viewport) return D3DERR_LIGHTHASVIEWPORT;
	for(int i = 0; i < 8; i++)
	{
		if(!lights[i])
		{
			if(lights[i] == lpDirect3DLight) return D3D_OK;
			lights[i] = (glDirect3DLight*)lpDirect3DLight;
			lights[i]->AddRef();
			lights[i]->viewport = this;
			if(device) lights[i]->SetDevice(device,i);
			lights[i]->Sync();
			return D3D_OK;
		}
	}
	return D3DERR_LIGHT_SET_FAILED;
}

HRESULT WINAPI glDirect3DViewport3::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	D3DCOLORVALUE bgcolor = {0.0,0.0,0.0,0.0};
	if(device->materials[background]) bgcolor = device->materials[background]->material.diffuse;
	return device->Clear(dwCount,lpRects,dwFlags,d3dcvtod3dcolor(bgcolor),0.0,0);
}

HRESULT WINAPI glDirect3DViewport3::Clear2(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	return device->Clear(dwCount,lpRects,dwFlags,dwColor,dvZ,dwStencil);
}

HRESULT WINAPI glDirect3DViewport3::DeleteLight(LPDIRECT3DLIGHT lpDirect3DLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DLight) return DDERR_INVALIDPARAMS;
	for(int i = 0; i < 8; i++)
	{
		if(lights[i] == lpDirect3DLight)
		{
			lights[i]->Release();
			lights[i]->SetDevice(NULL,0);
			lights[i] = NULL;
			return D3D_OK;
		}
	}
	return D3DERR_LIGHTNOTINTHISVIEWPORT;
}

HRESULT WINAPI glDirect3DViewport3::GetBackground(LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lphMat) return DDERR_INVALIDPARAMS;
	if(lpValid)
	{
		if(background) *lpValid = TRUE;
		else *lpValid = FALSE;
	}
	*lphMat = background;
	return D3D_OK;
}

HRESULT WINAPI glDirect3DViewport3::GetBackgroundDepth(LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lplpDDSurface) return DDERR_INVALIDPARAMS;
	if(lpValid)
	{
		if(backZ) *lpValid = TRUE;
		else *lpValid = FALSE;
	}
	if(backZ) backZ->QueryInterface(IID_IDirectDrawSurface,(void**)lplpDDSurface);
	else *lplpDDSurface = NULL;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::GetBackgroundDepth2(LPDIRECTDRAWSURFACE4* lplpDDS, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lplpDDS) return DDERR_INVALIDPARAMS;
	if(lpValid)
	{
		if(backZ) *lpValid = TRUE;
		else *lpValid = FALSE;
	}
	if(backZ) backZ->QueryInterface(IID_IDirectDrawSurface4,(void**)lplpDDS);
	else *lplpDDS = NULL;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::GetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpData) return DDERR_INVALIDPARAMS;
	memcpy(lpData,&viewport,sizeof(D3DVIEWPORT2));
	lpData->dvScaleX = scaleX;
	lpData->dvScaleY = scaleY;
	lpData->dvMaxX = maxX;
	lpData->dvMaxY = maxY;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::GetViewport2(LPD3DVIEWPORT2 lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpData) return DDERR_INVALIDPARAMS;
	memcpy(lpData,&viewport,sizeof(D3DVIEWPORT2));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::Initialize(LPDIRECT3D lpDirect3D)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirect3DViewport3::LightElements(DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	return E_NOTIMPL;
}
HRESULT WINAPI glDirect3DViewport3::NextLight(LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lplpDirect3DLight) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DViewport3::NextLight: stub");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DViewport3::SetBackground(D3DMATERIALHANDLE hMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	if(hMat > device->materialcount) return DDERR_INVALIDPARAMS;
	background = hMat;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::SetBackgroundDepth(LPDIRECTDRAWSURFACE lpDDSurface)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!backZ && !lpDDSurface) return D3D_OK;
	if(((glDirectDrawSurface1*)lpDDSurface)->GetDDS7() == backZ) return D3D_OK;
	if(backZ)backZ->Release();
	if(lpDDSurface) lpDDSurface->QueryInterface(IID_IDirectDrawSurface7,(void**)&backZ);
	else backZ = NULL;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::SetBackgroundDepth2(LPDIRECTDRAWSURFACE4 lpDDS)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!backZ && !lpDDS) return D3D_OK;
	if(((glDirectDrawSurface4*)lpDDS)->GetDDS7() == backZ) return D3D_OK;
	if(backZ)backZ->Release();
	if(lpDDS) lpDDS->QueryInterface(IID_IDirectDrawSurface7,(void**)&backZ);
	else backZ = NULL;
	return D3D_OK;
}

HRESULT WINAPI glDirect3DViewport3::SetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	if(!lpData) return DDERR_INVALIDPARAMS;
	D3DVIEWPORT2 vp;
	memcpy(&vp,lpData,sizeof(D3DVIEWPORT));
	vp.dvClipHeight = viewport.dvClipHeight;
	vp.dvClipWidth = viewport.dvClipWidth;
	vp.dvClipX = viewport.dvClipX;
	vp.dvClipY = viewport.dvClipY;
	viewport = vp;
	if(current && device) Sync();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::SetViewport2(LPD3DVIEWPORT2 lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	if(!lpData) return DDERR_INVALIDPARAMS;
	viewport = *lpData;
	if(current && device) Sync();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::TransformVertices(DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DViewport3::TransformVertices: stub");
	return DDERR_GENERIC;
}

void glDirect3DViewport3::SetCurrent(bool current)
{
	if(this->current && current) return;
	this->current = current;
	if(current && device)
	{
		Sync();
		SyncLights();
	}
}

void glDirect3DViewport3::Sync()
{
	D3DVIEWPORT7 vp7;
	vp7.dwX = viewport.dwX;
	vp7.dwY = viewport.dwY;
	vp7.dwHeight = viewport.dwHeight;
	vp7.dwWidth = viewport.dwWidth;
	vp7.dvMinZ = viewport.dvMinZ;
	vp7.dvMaxZ = viewport.dvMaxZ;
	device->SetViewport(&vp7);
}

void glDirect3DViewport3::SyncLights()
{
	D3DLIGHT7 light;
	for(int i = 0; i < 8; i++)
	{
		if(lights[i])
		{
			lights[i]->SetDevice(device,i);
			lights[i]->GetLight7(&light);
			device->SetLight(i,&light);
			device->LightEnable(i,TRUE);
		}
		else device->LightEnable(i,FALSE);
	}
}


glDirect3DViewport2::glDirect3DViewport2(glDirect3DViewport3 *glD3DV3)
{
	this->glD3DV3 = glD3DV3;
	refcount = 1;
}

glDirect3DViewport2::~glDirect3DViewport2()
{
	glD3DV3->glD3DV2 = NULL;
	glD3DV3->Release();
}

ULONG WINAPI glDirect3DViewport2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DViewport2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DViewport2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return glD3DV3->QueryInterface(riid,ppvObj);
}

HRESULT WINAPI glDirect3DViewport2::AddLight(LPDIRECT3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->AddLight(lpLight);
}
HRESULT WINAPI glDirect3DViewport2::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->Clear(dwCount,lpRects,dwFlags);
}
HRESULT WINAPI glDirect3DViewport2::DeleteLight(LPDIRECT3DLIGHT lpDirect3DLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->DeleteLight(lpDirect3DLight);
}
HRESULT WINAPI glDirect3DViewport2::GetBackground(LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetBackground(lphMat,lpValid);
}
HRESULT WINAPI glDirect3DViewport2::GetBackgroundDepth(LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetBackgroundDepth(lplpDDSurface,lpValid);
}
HRESULT WINAPI glDirect3DViewport2::GetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetViewport(lpData);
}
HRESULT WINAPI glDirect3DViewport2::GetViewport2(LPD3DVIEWPORT2 lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetViewport2(lpData);
}
HRESULT WINAPI glDirect3DViewport2::Initialize(LPDIRECT3D lpDirect3D)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->Initialize(lpDirect3D);
}
HRESULT WINAPI glDirect3DViewport2::LightElements(DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->LightElements(dwElementCount,lpData);
}
HRESULT WINAPI glDirect3DViewport2::NextLight(LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->NextLight(lpDirect3DLight,lplpDirect3DLight,dwFlags);
}
HRESULT WINAPI glDirect3DViewport2::SetBackground(D3DMATERIALHANDLE hMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetBackground(hMat);
}
HRESULT WINAPI glDirect3DViewport2::SetBackgroundDepth(LPDIRECTDRAWSURFACE lpDDSurface)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetBackgroundDepth(lpDDSurface);
}
HRESULT WINAPI glDirect3DViewport2::SetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetViewport(lpData);
}
HRESULT WINAPI glDirect3DViewport2::SetViewport2(LPD3DVIEWPORT2 lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetViewport2(lpData);
}
HRESULT WINAPI glDirect3DViewport2::TransformVertices(DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->TransformVertices(dwVertexCount,lpData,dwFlags,lpOffscreen);
}


glDirect3DViewport1::glDirect3DViewport1(glDirect3DViewport3 *glD3DV3)
{
	this->glD3DV3 = glD3DV3;
	refcount = 1;
}

glDirect3DViewport1::~glDirect3DViewport1()
{
	glD3DV3->glD3DV1 = NULL;
	glD3DV3->Release();
}

ULONG WINAPI glDirect3DViewport1::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DViewport1::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DViewport1::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	return glD3DV3->QueryInterface(riid,ppvObj);
}

HRESULT WINAPI glDirect3DViewport1::AddLight(LPDIRECT3DLIGHT lpLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->AddLight(lpLight);
}
HRESULT WINAPI glDirect3DViewport1::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->Clear(dwCount,lpRects,dwFlags);
}
HRESULT WINAPI glDirect3DViewport1::DeleteLight(LPDIRECT3DLIGHT lpDirect3DLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->DeleteLight(lpDirect3DLight);
}
HRESULT WINAPI glDirect3DViewport1::GetBackground(LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetBackground(lphMat,lpValid);
}
HRESULT WINAPI glDirect3DViewport1::GetBackgroundDepth(LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetBackgroundDepth(lplpDDSurface,lpValid);
}
HRESULT WINAPI glDirect3DViewport1::GetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->GetViewport(lpData);
}
HRESULT WINAPI glDirect3DViewport1::Initialize(LPDIRECT3D lpDirect3D)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->Initialize(lpDirect3D);
}
HRESULT WINAPI glDirect3DViewport1::LightElements(DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->LightElements(dwElementCount,lpData);
}
HRESULT WINAPI glDirect3DViewport1::NextLight(LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->NextLight(lpDirect3DLight,lplpDirect3DLight,dwFlags);
}
HRESULT WINAPI glDirect3DViewport1::SetBackground(D3DMATERIALHANDLE hMat)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetBackground(hMat);
}
HRESULT WINAPI glDirect3DViewport1::SetBackgroundDepth(LPDIRECTDRAWSURFACE lpDDSurface)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetBackgroundDepth(lpDDSurface);
}
HRESULT WINAPI glDirect3DViewport1::SetViewport(LPD3DVIEWPORT lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->SetViewport(lpData);
}
HRESULT WINAPI glDirect3DViewport1::TransformVertices(DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DV3->TransformVertices(dwVertexCount,lpData,dwFlags,lpOffscreen);
}
