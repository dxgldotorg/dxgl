// DXGL
// Copyright (C) 2012-2026 William Feely

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
#include "dxglDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirect3DDevice.h"
#include "ddraw.h"


glDirect3DTexture2Vtbl glDirect3DTexture2_iface =
{
	glDirect3DTexture2_QueryInterface,
	glDirect3DTexture2_AddRef,
	glDirect3DTexture2_Release,
	glDirect3DTexture2_GetHandle,
	glDirect3DTexture2_PaletteChanged,
	glDirect3DTexture2_Load
};

HRESULT glDirect3DTexture2_Create(dxglDirectDrawSurface7 *glDDS7, glDirect3DTexture2 *texture)
{
	TRACE_ENTER(2, 14, glDDS7, 14, texture);
	texture->lpVtbl = &glDirect3DTexture2_iface;
	texture->glDDS7 = glDDS7;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DTexture2_QueryInterface(glDirect3DTexture2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if (!ppvObj) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*ppvObj = NULL;
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!&riid) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		glDirect3DTexture2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}

ULONG WINAPI glDirect3DTexture2_AddRef(glDirect3DTexture2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDDS7->textureparent->AddRef());
}

ULONG WINAPI glDirect3DTexture2_Release(glDirect3DTexture2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDDS7->textureparent->Release());
}

HRESULT WINAPI glDirect3DTexture2_GetHandle(glDirect3DTexture2 *This, LPDIRECT3DDEVICE2 lpDirect3DDevice2, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice2,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice7 *glD3DDev7 = ((glDirect3DDevice2*)lpDirect3DDevice2)->glD3DDev7;
	HRESULT ret = dxglDirectDrawSurface7_GetHandle(This->glDDS7,glD3DDev7,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DTexture2_Load(glDirect3DTexture2 *This, LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
	TRACE_ENTER(2,14,This,14,lpD3DTexture2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTexture2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Load(This->glDDS7,((glDirect3DTexture2*)lpD3DTexture2)->glDDS7));
}
HRESULT WINAPI glDirect3DTexture2_PaletteChanged(glDirect3DTexture2 *This, DWORD dwStart, DWORD dwCount)
{
	TRACE_ENTER(3,14,This,8,dwStart,8,dwCount);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DTexture2_PaletteChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

glDirect3DTexture1Vtbl glDirect3DTexture1_iface =
{
	glDirect3DTexture1_QueryInterface,
	glDirect3DTexture1_AddRef,
	glDirect3DTexture1_Release,
	glDirect3DTexture1_Initialize,
	glDirect3DTexture1_GetHandle,
	glDirect3DTexture1_PaletteChanged,
	glDirect3DTexture1_Load,
	glDirect3DTexture1_Unload
};

HRESULT glDirect3DTexture1_Create(dxglDirectDrawSurface7 *glDDS7, glDirect3DTexture1 *texture)
{
	TRACE_ENTER(2,14,glDDS7,14,texture);
	texture->lpVtbl = &glDirect3DTexture1_iface;
	texture->glDDS7 = glDDS7;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DTexture1_QueryInterface(glDirect3DTexture1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if (!ppvObj) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	*ppvObj = NULL;
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!&riid) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		glDirect3DTexture1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_QueryInterface(This->glDDS7,riid,ppvObj));
}
ULONG WINAPI glDirect3DTexture1_AddRef(glDirect3DTexture1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDDS7->textureparent->AddRef());
}
ULONG WINAPI glDirect3DTexture1_Release(glDirect3DTexture1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDDS7->textureparent->Release());
}

HRESULT WINAPI glDirect3DTexture1_GetHandle(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,This,14,lpDirect3DDevice,14,lpHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice7 *glD3DDev7 = ((glDirect3DDevice1*)lpDirect3DDevice)->glD3DDev7;
	HRESULT ret = dxglDirectDrawSurface7_GetHandle(This->glDDS7,glD3DDev7,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DTexture1_Initialize(glDirect3DTexture1 *This, LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface)
{
	TRACE_ENTER(3,14,This,14,lpD3DDevice,14,lpDDSurface);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirect3DTexture1_Load(glDirect3DTexture1 *This, LPDIRECT3DTEXTURE lpD3DTexture)
{
	TRACE_ENTER(2,14,This,14,lpD3DTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTexture) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,dxglDirectDrawSurface7_Load(This->glDDS7,((glDirect3DTexture1*)lpD3DTexture)->glDDS7));
}
HRESULT WINAPI glDirect3DTexture1_PaletteChanged(glDirect3DTexture1 *This, DWORD dwStart, DWORD dwCount)
{
	TRACE_ENTER(3,14,This,8,dwStart,8,dwCount);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DTexture1_PaletteChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DTexture1_Unload(glDirect3DTexture1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
