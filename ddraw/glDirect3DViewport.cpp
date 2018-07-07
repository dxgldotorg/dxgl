// DXGL
// Copyright (C) 2011-2016 William Feely

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
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DLight.h"
#include "glDirect3DDevice.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DViewport.h"

extern "C" {

inline D3DCOLOR d3dcvtod3dcolor(D3DCOLORVALUE value)
{
	int r = (int)(value.r * 255);
	int g = (int)(value.g * 255);
	int b = (int)(value.b * 255);
	int a = (int)(value.a * 255);
	return b|(g<<8)|(r<<16)|(a<<24);
}

glDirect3DViewport3Vtbl glDirect3DViewport3_iface =
{
	glDirect3DViewport3_QueryInterface,
	glDirect3DViewport3_AddRef,
	glDirect3DViewport3_Release,
	glDirect3DViewport3_Initialize,
	glDirect3DViewport3_GetViewport,
	glDirect3DViewport3_SetViewport,
	glDirect3DViewport3_TransformVertices,
	glDirect3DViewport3_LightElements,
	glDirect3DViewport3_SetBackground,
	glDirect3DViewport3_GetBackground,
	glDirect3DViewport3_SetBackgroundDepth,
	glDirect3DViewport3_GetBackgroundDepth,
	glDirect3DViewport3_Clear,
	glDirect3DViewport3_AddLight,
	glDirect3DViewport3_DeleteLight,
	glDirect3DViewport3_NextLight,
	glDirect3DViewport3_GetViewport2,
	glDirect3DViewport3_SetViewport2,
	glDirect3DViewport3_SetBackgroundDepth2,
	glDirect3DViewport3_GetBackgroundDepth2,
	glDirect3DViewport3_Clear2
};

HRESULT glDirect3DViewport3_Create(LPDIRECT3DVIEWPORT3 *viewport)
{
	glDirect3DViewport3 *newvp;
	TRACE_ENTER(1, 14, viewport);
	if (!viewport) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newvp = (glDirect3DViewport3*)malloc(sizeof(glDirect3DViewport3));
	if (!newvp) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*viewport = (LPDIRECT3DVIEWPORT3)newvp;
	newvp->lpVtbl = &glDirect3DViewport3_iface;
	newvp->background = 0;
	ZeroMemory(&newvp->viewport,sizeof(D3DVIEWPORT2));
	newvp->viewport.dwSize = sizeof(D3DVIEWPORT2);
	ZeroMemory(&newvp->viewport, sizeof(D3DVIEWPORT));
	newvp->viewport.dwSize = sizeof(D3DVIEWPORT);
	newvp->device = NULL;
	newvp->backZ = NULL;
	for(int i = 0; i < 8; i++)
		newvp->lights[i] = NULL;
	newvp->refcount = 1;
	newvp->current = false;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirect3DViewport3_QueryInterface(glDirect3DViewport3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if ((riid == IID_IDirect3DViewport3) || (riid == IID_IDirect3DViewport2) ||
		(riid == IID_IDirect3DViewport) || (riid == IID_IUnknown))
	{
		glDirect3DViewport3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	*ppvObj = NULL;
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DViewport3_AddRef(glDirect3DViewport3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	This->refcount++;
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}

ULONG WINAPI glDirect3DViewport3_Release(glDirect3DViewport3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	This->refcount--;
	ret = This->refcount;
	if (This->refcount == 0)
	{
		if (This->device) This->device->Release();
		if (This->backZ) This->backZ->Release();
		for (int i = 0; i < 8; i++)
		{
			if (This->lights[i]) This->lights[i]->Release();
		}
		free(This);
	}
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DViewport3_AddLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(((glDirect3DLight*)lpDirect3DLight)->viewport) TRACE_RET(HRESULT,23,D3DERR_LIGHTHASVIEWPORT);
	for(int i = 0; i < 8; i++)
	{
		if(!This->lights[i])
		{
			if(This->lights[i] == lpDirect3DLight) return D3D_OK;
			This->lights[i] = (glDirect3DLight*)lpDirect3DLight;
			This->lights[i]->AddRef();
			This->lights[i]->viewport = This;
			if(This->device) This->lights[i]->SetDevice(This->device,i);
			This->lights[i]->Sync();
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,D3DERR_LIGHT_SET_FAILED);
	return D3DERR_LIGHT_SET_FAILED;
}

HRESULT WINAPI glDirect3DViewport3_Clear(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,8,dwCount,14,lpRects,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->device) TRACE_RET(HRESULT,23,D3DERR_VIEWPORTHASNODEVICE);
	D3DCOLORVALUE bgcolor = {0.0,0.0,0.0,0.0};
	if(This->device->materials[This->background]) bgcolor = This->device->materials[This->background]->material.diffuse;
	TRACE_RET(HRESULT,23,This->device->Clear(dwCount,lpRects,dwFlags,d3dcvtod3dcolor(bgcolor),0.0,0));
}

HRESULT WINAPI glDirect3DViewport3_Clear2(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	TRACE_ENTER(7,14,This,8,dwCount,14,lpRects,9,dwFlags,9,dwColor,19,&dvZ,9,dwStencil);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->device) TRACE_RET(HRESULT,23,D3DERR_VIEWPORTHASNODEVICE);
	TRACE_RET(HRESULT,23,This->device->Clear(dwCount,lpRects,dwFlags,dwColor,dvZ,dwStencil));
}

HRESULT WINAPI glDirect3DViewport3_DeleteLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(int i = 0; i < 8; i++)
	{
		if(This->lights[i] == lpDirect3DLight)
		{
			This->lights[i]->Release();
			This->lights[i]->SetDevice(NULL,0);
			This->lights[i] = NULL;
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,D3DERR_LIGHTHASVIEWPORT);
	return D3DERR_LIGHTNOTINTHISVIEWPORT;
}

HRESULT WINAPI glDirect3DViewport3_GetBackground(glDirect3DViewport3 *This, LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lphMat,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lphMat) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpValid)
	{
		if(This->background) *lpValid = TRUE;
		else *lpValid = FALSE;
		TRACE_VAR("*lpValid",22,*lpValid);
	}
	*lphMat = This->background;
	TRACE_VAR("*lphMat",9,*lphMat);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DViewport3_GetBackgroundDepth(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lplpDDSurface,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDSurface) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpValid)
	{
		if(This->backZ) *lpValid = TRUE;
		else *lpValid = FALSE;
		TRACE_VAR("*lpValid",22,*lpValid);
	}
	if(This->backZ) This->backZ->QueryInterface(IID_IDirectDrawSurface,(void**)lplpDDSurface);
	else *lplpDDSurface = NULL;
	TRACE_VAR("*lplpDDSurface",14,*lplpDDSurface);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_GetBackgroundDepth2(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4* lplpDDS, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lplpDDS,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDDS) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpValid)
	{
		if(This->backZ) *lpValid = TRUE;
		else *lpValid = FALSE;
		TRACE_VAR("*lpValid",22,*lpValid);
	}
	if(This->backZ) This->backZ->QueryInterface(IID_IDirectDrawSurface4,(void**)lplpDDS);
	else *lplpDDS = NULL;
	TRACE_VAR("*lplpDDS",14,*lplpDDS);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_GetViewport(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpData,&This->viewport1,sizeof(D3DVIEWPORT2));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_GetViewport2(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpData,&This->viewport,sizeof(D3DVIEWPORT2));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_Initialize(glDirect3DViewport3 *This, LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,This,14,lpDirect3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirect3DViewport3_LightElements(glDirect3DViewport3 *This, DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	TRACE_ENTER(3,14,This,8,dwElementCount,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_UNSUPPORTED);
	return DDERR_UNSUPPORTED;
}
HRESULT WINAPI glDirect3DViewport3_NextLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DLight,14,lplpDirect3DLight,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDirect3DLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DViewport3::NextLight: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DViewport3_SetBackground(glDirect3DViewport3 *This, D3DMATERIALHANDLE hMat)
{
	TRACE_ENTER(2,14,This,9,hMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->device) TRACE_RET(HRESULT,23,D3DERR_VIEWPORTHASNODEVICE);
	if(hMat > This->device->materialcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->background = hMat;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_SetBackgroundDepth(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE lpDDSurface)
{
	HRESULT ret = D3D_OK;
	TRACE_ENTER(2, 14, This, 14, lpDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->backZ && !lpDDSurface) TRACE_RET(HRESULT,23,D3D_OK);
	if(((glDirectDrawSurface1*)lpDDSurface)->GetDDS7() == This->backZ) TRACE_RET(HRESULT,23,D3D_OK);
	if(This->backZ)This->backZ->Release();
	if(lpDDSurface) ret = lpDDSurface->QueryInterface(IID_IDirectDrawSurface7,(void**)&This->backZ);
	else This->backZ = NULL;
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DViewport3_SetBackgroundDepth2(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4 lpDDS)
{
	HRESULT ret = D3D_OK;
	TRACE_ENTER(2,14,This,14,lpDDS);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->backZ && !lpDDS) TRACE_RET(HRESULT,23,D3D_OK);
	if(((glDirectDrawSurface4*)lpDDS)->GetDDS7() == This->backZ) TRACE_RET(HRESULT,23,D3D_OK);
	if(This->backZ)This->backZ->Release();
	if(lpDDS) ret = lpDDS->QueryInterface(IID_IDirectDrawSurface7,(void**)&This->backZ);
	else This->backZ = NULL;
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DViewport3_SetViewport(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->device) TRACE_RET(HRESULT,23,D3DERR_VIEWPORTHASNODEVICE);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	D3DVIEWPORT2 vp;
	memcpy(&vp,lpData,sizeof(D3DVIEWPORT));
	vp.dvClipHeight = This->viewport.dvClipHeight;
	vp.dvClipWidth = This->viewport.dvClipWidth;
	vp.dvClipX = This->viewport.dvClipX;
	vp.dvClipY = This->viewport.dvClipY;
	if((vp.dvMinZ == 0) && (vp.dvMaxZ == 0)) vp.dvMaxZ = 1.0f;
	This->viewport = vp;
	This->viewport1 = *lpData;
	if ((This->viewport1.dvMinZ == 0) && (This->viewport1.dvMaxZ == 0))
		This->viewport1.dvMaxZ = 1.0f;
	This->viewportver = 1;
	if(This->current && This->device) glDirect3DViewport3_Sync(This);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_SetViewport2(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->device) TRACE_RET(HRESULT,23,D3DERR_VIEWPORTHASNODEVICE);
	if(!lpData) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	D3DVIEWPORT vp;
	memcpy(&vp, lpData, sizeof(D3DVIEWPORT2));
	vp.dvMaxX = This->viewport1.dvMaxX;
	vp.dvMaxY = This->viewport1.dvMaxY;
	vp.dvScaleX = This->viewport1.dvScaleX;
	vp.dvScaleY = This->viewport1.dvScaleY;
	if ((vp.dvMinZ == 0) && (vp.dvMaxZ == 0)) vp.dvMaxZ = 1.0f;
	This->viewport = *lpData;
	This->viewport1 = vp;
	if ((This->viewport.dvMinZ == 0) && (This->viewport.dvMaxZ == 0))
		This->viewport.dvMaxZ = 1.0f;
	if (This->current && This->device) glDirect3DViewport3_Sync(This);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DViewport3_TransformVertices(glDirect3DViewport3 *This, DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	TRACE_ENTER(5,14,This,8,dwVertexCount,14,lpData,9,dwFlags,14,lpOffscreen);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DViewport3::TransformVertices: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

void glDirect3DViewport3_SetCurrent(glDirect3DViewport3 *This, bool current)
{
	TRACE_ENTER(2,14,This,21,current);
	if(This->current && current)
	{
		TRACE_EXIT(0,0);
		return;
	}
	This->current = current;
	if(current && This->device)
	{
		glDirect3DViewport3_Sync(This);
		glDirect3DViewport3_SyncLights(This);
	}
	TRACE_EXIT(0,0);
}

void glDirect3DViewport3_SetDevice(glDirect3DViewport3 *This, glDirect3DDevice7 *device)
{
	This->device = device; 
}

void glDirect3DViewport3_Sync(glDirect3DViewport3 *This)
{
	TRACE_ENTER(1,14,This);
	D3DVIEWPORT7 vp7;
	vp7.dwX = This->viewport.dwX;
	vp7.dwY = This->viewport.dwY;
	vp7.dwHeight = This->viewport.dwHeight;
	vp7.dwWidth = This->viewport.dwWidth;
	vp7.dvMinZ = This->viewport.dvMinZ;
	vp7.dvMaxZ = This->viewport.dvMaxZ;
	This->device->SetViewport(&vp7);
	This->device->SetScale(This->viewport1.dvScaleX,This->viewport1.dvScaleY);
	TRACE_EXIT(0,0);
}

void glDirect3DViewport3_SyncLights(glDirect3DViewport3 *This)
{
	TRACE_ENTER(1,14,This);
	D3DLIGHT7 light;
	for(int i = 0; i < 8; i++)
	{
		if(This->lights[i])
		{
			This->lights[i]->SetDevice(This->device,i);
			This->lights[i]->GetLight7(&light);
			This->device->SetLight(i,&light);
			This->device->LightEnable(i,TRUE);
		}
		else This->device->LightEnable(i,FALSE);
	}
	TRACE_EXIT(0,0);
}


}