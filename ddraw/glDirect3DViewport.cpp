// DXGL
// Copyright (C) 2011-2012 William Feely

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
}

glDirect3DViewport3::~glDirect3DViewport3()
{
}

HRESULT WINAPI glDirect3DViewport3::QueryInterface(REFIID riid, void** ppvObj)
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
			lights[i] = (glDirect3DLight*)lpDirect3DLight;
			lights[i]->AddRef();
			lights[i]->viewport = this;
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
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::SetViewport2(LPD3DVIEWPORT2 lpData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!device) return D3DERR_VIEWPORTHASNODEVICE;
	if(!lpData) return DDERR_INVALIDPARAMS;
	viewport = *lpData;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3::TransformVertices(DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DViewport3::TransformVertices: stub");
	return DDERR_GENERIC;
}