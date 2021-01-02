// DXGL
// Copyright (C) 2011-2021 William Feely

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

#pragma once
#ifndef __GLDIRECT3DVIEWPORT_H
#define __GLDIRECT3DVIEWPORT_H

#ifdef __cplusplus
class glDirect3DLight;
struct glDirect3DDevice7;
class glDirectDrawSurface7;
extern "C" {
#else
typedef int glDirect3DLight;
typedef int glDirect3DDevice7;
typedef int glDirectDrawSurface7;
#endif

struct glDirect3DViewport3Vtbl;

typedef struct glDirect3DViewport3
{
	glDirect3DViewport3Vtbl *lpVtbl;
	ULONG refcount;
	glDirect3DLight *lights[8];
	glDirect3DDevice7 *device;
	glDirectDrawSurface7 *backZ;
	D3DMATERIALHANDLE background;
	D3DVIEWPORT viewport1;
	D3DVIEWPORT2 viewport;
	int viewportver;
	bool current;
} glDirect3DViewport3;

typedef struct glDirect3DViewport3Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DViewport3 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirect3DViewport3 *This);
	ULONG(WINAPI *Release)(glDirect3DViewport3 *This);
	HRESULT(WINAPI *Initialize)(glDirect3DViewport3 *This, LPDIRECT3D lpDirect3D);
	HRESULT(WINAPI *GetViewport)(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData);
	HRESULT(WINAPI *SetViewport)(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData);
	HRESULT(WINAPI *TransformVertices)(glDirect3DViewport3 *This, DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen);
	HRESULT(WINAPI *LightElements)(glDirect3DViewport3 *This, DWORD dwElementCount, LPD3DLIGHTDATA lpData);
	HRESULT(WINAPI *SetBackground)(glDirect3DViewport3 *This, D3DMATERIALHANDLE hMat);
	HRESULT(WINAPI *GetBackground)(glDirect3DViewport3 *This, LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid);
	HRESULT(WINAPI *SetBackgroundDepth)(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE lpDDSurface);
	HRESULT(WINAPI *GetBackgroundDepth)(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid);
	HRESULT(WINAPI *Clear)(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags);
	HRESULT(WINAPI *AddLight)(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight);
	HRESULT(WINAPI *DeleteLight)(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight);
	HRESULT(WINAPI *NextLight)(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags);
	HRESULT(WINAPI *GetViewport2)(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData);
	HRESULT(WINAPI *SetViewport2)(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData);
	HRESULT(WINAPI *SetBackgroundDepth2)(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4 lpDDS);
	HRESULT(WINAPI *GetBackgroundDepth2)(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4* lplpDDS, LPBOOL lpValid);
	HRESULT(WINAPI *Clear2)(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
} glDirect3DViewport3Vtbl;

HRESULT glDirect3DViewport3_Create(LPDIRECT3DVIEWPORT3 *viewport);
void glDirect3DViewport3_SetCurrent(glDirect3DViewport3 *This, bool current);
void glDirect3DViewport3_SetDevice(glDirect3DViewport3 *This, glDirect3DDevice7 *device);
void glDirect3DViewport3_Sync(glDirect3DViewport3 *This);
void glDirect3DViewport3_SyncLights(glDirect3DViewport3 *This);

HRESULT WINAPI glDirect3DViewport3_QueryInterface(glDirect3DViewport3 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DViewport3_AddRef(glDirect3DViewport3 *This);
ULONG WINAPI glDirect3DViewport3_Release(glDirect3DViewport3 *This);
HRESULT WINAPI glDirect3DViewport3_Initialize(glDirect3DViewport3 *This, LPDIRECT3D lpDirect3D);
HRESULT WINAPI glDirect3DViewport3_GetViewport(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData);
HRESULT WINAPI glDirect3DViewport3_SetViewport(glDirect3DViewport3 *This, LPD3DVIEWPORT lpData);
HRESULT WINAPI glDirect3DViewport3_TransformVertices(glDirect3DViewport3 *This, DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen);
HRESULT WINAPI glDirect3DViewport3_LightElements(glDirect3DViewport3 *This, DWORD dwElementCount, LPD3DLIGHTDATA lpData);
HRESULT WINAPI glDirect3DViewport3_SetBackground(glDirect3DViewport3 *This, D3DMATERIALHANDLE hMat);
HRESULT WINAPI glDirect3DViewport3_GetBackground(glDirect3DViewport3 *This, LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid);
HRESULT WINAPI glDirect3DViewport3_SetBackgroundDepth(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE lpDDSurface);
HRESULT WINAPI glDirect3DViewport3_GetBackgroundDepth(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid);
HRESULT WINAPI glDirect3DViewport3_Clear(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags);
HRESULT WINAPI glDirect3DViewport3_AddLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight);
HRESULT WINAPI glDirect3DViewport3_DeleteLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight);
HRESULT WINAPI glDirect3DViewport3_NextLight(glDirect3DViewport3 *This, LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags);
HRESULT WINAPI glDirect3DViewport3_GetViewport2(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData);
HRESULT WINAPI glDirect3DViewport3_SetViewport2(glDirect3DViewport3 *This, LPD3DVIEWPORT2 lpData);
HRESULT WINAPI glDirect3DViewport3_SetBackgroundDepth2(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4 lpDDS);
HRESULT WINAPI glDirect3DViewport3_GetBackgroundDepth2(glDirect3DViewport3 *This, LPDIRECTDRAWSURFACE4* lplpDDS, LPBOOL lpValid);
HRESULT WINAPI glDirect3DViewport3_Clear2(glDirect3DViewport3 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);



#ifdef __cplusplus
}
#endif

#endif //__GLDIRECT3DVIEWPORT_H
