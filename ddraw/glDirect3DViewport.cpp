// DXGL
// Copyright (C) 2011-2014 William Feely

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
#include "TextureManager.h"
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
	newvp->maxX = newvp->maxY = newvp->scaleX = newvp->scaleY = 0;
	newvp->device = NULL;
	newvp->backZ = NULL;
	for(int i = 0; i < 8; i++)
		newvp->lights[i] = NULL;
	newvp->refcount = 1;
	newvp->current = false;
	newvp->glD3DV2 = NULL;
	newvp->glD3DV1 = NULL;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirect3DViewport3_QueryInterface(glDirect3DViewport3 *This, REFIID riid, void** ppvObj)
{
	HRESULT ret;
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DViewport3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DViewport3)
	{
		glDirect3DViewport3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DViewport2)
	{
		if(This->glD3DV2)
		{
			*ppvObj = This->glD3DV2;
			glDirect3DViewport2_AddRef(This->glD3DV2);
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			ret = glDirect3DViewport2_Create(This, (LPDIRECT3DVIEWPORT2*)ppvObj);
			if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
			glDirect3DViewport3_AddRef(This);
			This->glD3DV2 = (glDirect3DViewport2*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DViewport)
	{
		if(This->glD3DV1)
		{
			*ppvObj = This->glD3DV1;
			glDirect3DViewport1_AddRef(This->glD3DV1);
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			ret = glDirect3DViewport1_Create(This, (LPDIRECT3DVIEWPORT*)ppvObj);
			if (ret != DD_OK) TRACE_RET(HRESULT, 23, ret);
			glDirect3DViewport3_AddRef(This);
			This->glD3DV1 = (glDirect3DViewport1*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
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
	memcpy(lpData,&This->viewport,sizeof(D3DVIEWPORT2));
	lpData->dvScaleX = This->scaleX;
	lpData->dvScaleY = This->scaleY;
	lpData->dvMaxX = This->maxX;
	lpData->dvMaxY = This->maxY;
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
	This->maxX = lpData->dvMaxX;
	This->maxY = lpData->dvMaxY;
	This->scaleX = lpData->dvScaleX;
	This->scaleY = lpData->dvScaleY;
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
	This->viewport = *lpData;
	if(This->current && This->device) glDirect3DViewport3_Sync(This);
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
	This->device->SetScale(This->scaleX,This->scaleY);
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

glDirect3DViewport2Vtbl glDirect3DViewport2_iface =
{
	glDirect3DViewport2_QueryInterface,
	glDirect3DViewport2_AddRef,
	glDirect3DViewport2_Release,
	glDirect3DViewport2_Initialize,
	glDirect3DViewport2_GetViewport,
	glDirect3DViewport2_SetViewport,
	glDirect3DViewport2_TransformVertices,
	glDirect3DViewport2_LightElements,
	glDirect3DViewport2_SetBackground,
	glDirect3DViewport2_GetBackground,
	glDirect3DViewport2_SetBackgroundDepth,
	glDirect3DViewport2_GetBackgroundDepth,
	glDirect3DViewport2_Clear,
	glDirect3DViewport2_AddLight,
	glDirect3DViewport2_DeleteLight,
	glDirect3DViewport2_NextLight,
	glDirect3DViewport2_GetViewport2,
	glDirect3DViewport2_SetViewport2
};

HRESULT glDirect3DViewport2_Create(glDirect3DViewport3 *glD3DV3, LPDIRECT3DVIEWPORT2 *viewport)
{
	glDirect3DViewport2 *newvp;
	TRACE_ENTER(2, 14, glD3DV3, 14, viewport);
	if (!viewport) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newvp = (glDirect3DViewport2*)malloc(sizeof(glDirect3DViewport2));
	if (!newvp) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*viewport = (LPDIRECT3DVIEWPORT2)newvp;
	newvp->lpVtbl = &glDirect3DViewport2_iface;
	newvp->glD3DV3 = glD3DV3;
	newvp->refcount = 1;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

ULONG WINAPI glDirect3DViewport2_AddRef(glDirect3DViewport2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	This->refcount++;
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}

ULONG WINAPI glDirect3DViewport2_Release(glDirect3DViewport2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	This->refcount--;
	ret = This->refcount;
	if (This->refcount == 0)
	{
		This->glD3DV3->glD3DV2 = NULL;
		glDirect3DViewport3_Release(This->glD3DV3);
		free(This);
	}
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DViewport2_QueryInterface(glDirect3DViewport2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DViewport2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DViewport3_QueryInterface(This->glD3DV3,riid,ppvObj));
}

HRESULT WINAPI glDirect3DViewport2_AddLight(glDirect3DViewport2 *This, LPDIRECT3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,This,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_AddLight(This->glD3DV3,lpLight));
}
HRESULT WINAPI glDirect3DViewport2_Clear(glDirect3DViewport2 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,9,dwCount,14,lpRects,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_Clear(This->glD3DV3,dwCount,lpRects,dwFlags));
}
HRESULT WINAPI glDirect3DViewport2_DeleteLight(glDirect3DViewport2 *This, LPDIRECT3DLIGHT lpDirect3DLight)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_DeleteLight(This->glD3DV3,lpDirect3DLight));
}
HRESULT WINAPI glDirect3DViewport2_GetBackground(glDirect3DViewport2 *This, LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lphMat,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetBackground(This->glD3DV3,lphMat,lpValid));
}
HRESULT WINAPI glDirect3DViewport2_GetBackgroundDepth(glDirect3DViewport2 *This, LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lplpDDSurface,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetBackgroundDepth(This->glD3DV3,lplpDDSurface,lpValid));
}
HRESULT WINAPI glDirect3DViewport2_GetViewport(glDirect3DViewport2 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetViewport(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport2_GetViewport2(glDirect3DViewport2 *This, LPD3DVIEWPORT2 lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetViewport2(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport2_Initialize(glDirect3DViewport2 *This, LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,This,14,lpDirect3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_Initialize(This->glD3DV3,lpDirect3D));
}
HRESULT WINAPI glDirect3DViewport2_LightElements(glDirect3DViewport2 *This, DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	TRACE_ENTER(3,14,This,8,dwElementCount,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_LightElements(This->glD3DV3,dwElementCount,lpData));
}
HRESULT WINAPI glDirect3DViewport2_NextLight(glDirect3DViewport2 *This, LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DLight,14,lplpDirect3DLight,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_NextLight(This->glD3DV3,lpDirect3DLight,lplpDirect3DLight,dwFlags));
}
HRESULT WINAPI glDirect3DViewport2_SetBackground(glDirect3DViewport2 *This, D3DMATERIALHANDLE hMat)
{
	TRACE_ENTER(2,14,This,9,hMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetBackground(This->glD3DV3,hMat));
}
HRESULT WINAPI glDirect3DViewport2_SetBackgroundDepth(glDirect3DViewport2 *This, LPDIRECTDRAWSURFACE lpDDSurface)
{
	TRACE_ENTER(2,14,This,14,lpDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetBackgroundDepth(This->glD3DV3,lpDDSurface));
}
HRESULT WINAPI glDirect3DViewport2_SetViewport(glDirect3DViewport2 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetViewport(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport2_SetViewport2(glDirect3DViewport2 *This, LPD3DVIEWPORT2 lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetViewport2(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport2_TransformVertices(glDirect3DViewport2 *This, DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	TRACE_ENTER(5,14,This,8,dwVertexCount,14,lpData,9,dwFlags,14,lpOffscreen);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_TransformVertices(This->glD3DV3,dwVertexCount,lpData,dwFlags,lpOffscreen));
}


glDirect3DViewport1Vtbl glDirect3DViewport1_iface =
{
	glDirect3DViewport1_QueryInterface,
	glDirect3DViewport1_AddRef,
	glDirect3DViewport1_Release,
	glDirect3DViewport1_Initialize,
	glDirect3DViewport1_GetViewport,
	glDirect3DViewport1_SetViewport,
	glDirect3DViewport1_TransformVertices,
	glDirect3DViewport1_LightElements,
	glDirect3DViewport1_SetBackground,
	glDirect3DViewport1_GetBackground,
	glDirect3DViewport1_SetBackgroundDepth,
	glDirect3DViewport1_GetBackgroundDepth,
	glDirect3DViewport1_Clear,
	glDirect3DViewport1_AddLight,
	glDirect3DViewport1_DeleteLight,
	glDirect3DViewport1_NextLight
};

HRESULT glDirect3DViewport1_Create(glDirect3DViewport3 *glD3DV3, LPDIRECT3DVIEWPORT *viewport)
{
	glDirect3DViewport1 *newvp;
	TRACE_ENTER(2, 14, glD3DV3, 14, viewport);
	if (!viewport) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	newvp = (glDirect3DViewport1*)malloc(sizeof(glDirect3DViewport1));
	if (!newvp) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*viewport = (LPDIRECT3DVIEWPORT)newvp;
	newvp->lpVtbl = &glDirect3DViewport1_iface;
	newvp->glD3DV3 = glD3DV3;
	newvp->refcount = 1;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

ULONG WINAPI glDirect3DViewport1_AddRef(glDirect3DViewport1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	This->refcount++;
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}

ULONG WINAPI glDirect3DViewport1_Release(glDirect3DViewport1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	This->refcount--;
	ret = This->refcount;
	if (This->refcount == 0)
	{
		This->glD3DV3->glD3DV1 = NULL;
		glDirect3DViewport3_Release(This->glD3DV3);
		free(This);
	}
	TRACE_EXIT(8, ret);
	return ret;
}

HRESULT WINAPI glDirect3DViewport1_QueryInterface(glDirect3DViewport1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(riid == IID_IUnknown)
	{
		glDirect3DViewport1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DViewport3_QueryInterface(This->glD3DV3,riid,ppvObj));
}

HRESULT WINAPI glDirect3DViewport1_AddLight(glDirect3DViewport1 *This, LPDIRECT3DLIGHT lpLight)
{
	TRACE_ENTER(2,14,This,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_AddLight(This->glD3DV3,lpLight));
}
HRESULT WINAPI glDirect3DViewport1_Clear(glDirect3DViewport1 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,8,dwCount,14,lpRects,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_Clear(This->glD3DV3,dwCount,lpRects,dwFlags));
}
HRESULT WINAPI glDirect3DViewport1_DeleteLight(glDirect3DViewport1 *This, LPDIRECT3DLIGHT lpDirect3DLight)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_DeleteLight(This->glD3DV3,lpDirect3DLight));
}
HRESULT WINAPI glDirect3DViewport1_GetBackground(glDirect3DViewport1 *This, LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lphMat,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetBackground(This->glD3DV3,lphMat,lpValid));
}
HRESULT WINAPI glDirect3DViewport1_GetBackgroundDepth(glDirect3DViewport1 *This, LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid)
{
	TRACE_ENTER(3,14,This,14,lplpDDSurface,14,lpValid);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetBackgroundDepth(This->glD3DV3,lplpDDSurface,lpValid));
}
HRESULT WINAPI glDirect3DViewport1_GetViewport(glDirect3DViewport1 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_GetViewport(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport1_Initialize(glDirect3DViewport1 *This, LPDIRECT3D lpDirect3D)
{
	TRACE_ENTER(2,14,This,14,lpDirect3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_Initialize(This->glD3DV3,lpDirect3D));
}
HRESULT WINAPI glDirect3DViewport1_LightElements(glDirect3DViewport1 *This, DWORD dwElementCount, LPD3DLIGHTDATA lpData)
{
	TRACE_ENTER(3,14,This,8,dwElementCount,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	return glDirect3DViewport3_LightElements(This->glD3DV3,dwElementCount,lpData);
}
HRESULT WINAPI glDirect3DViewport1_NextLight(glDirect3DViewport1 *This, LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DLight,14,lplpDirect3DLight,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_NextLight(This->glD3DV3,lpDirect3DLight,lplpDirect3DLight,dwFlags));
}
HRESULT WINAPI glDirect3DViewport1_SetBackground(glDirect3DViewport1 *This, D3DMATERIALHANDLE hMat)
{
	TRACE_ENTER(2,14,This,9,hMat);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetBackground(This->glD3DV3,hMat));
}
HRESULT WINAPI glDirect3DViewport1_SetBackgroundDepth(glDirect3DViewport1 *This, LPDIRECTDRAWSURFACE lpDDSurface)
{
	TRACE_ENTER(2,14,This,14,lpDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetBackgroundDepth(This->glD3DV3,lpDDSurface));
}
HRESULT WINAPI glDirect3DViewport1_SetViewport(glDirect3DViewport1 *This, LPD3DVIEWPORT lpData)
{
	TRACE_ENTER(2,14,This,14,lpData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_SetViewport(This->glD3DV3,lpData));
}
HRESULT WINAPI glDirect3DViewport1_TransformVertices(glDirect3DViewport1 *This, DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen)
{
	TRACE_ENTER(5,14,This,8,dwVertexCount,14,lpData,9,dwFlags,14,lpOffscreen);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DViewport3_TransformVertices(This->glD3DV3,dwVertexCount,lpData,dwFlags,lpOffscreen));
}

}