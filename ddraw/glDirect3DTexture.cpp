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
#include "TextureManager.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirect3DDevice.h"

glDirect3DTexture2::glDirect3DTexture2(glDirectDrawSurface7 *glDDS7)
{
	TRACE_ENTER(2,14,this,14,glDDS7);
	this->glDDS7 = glDDS7;
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DTexture2::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}

ULONG WINAPI glDirect3DTexture2::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->textureparent->AddRef());
}

ULONG WINAPI glDirect3DTexture2::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->textureparent->Release());
}

HRESULT WINAPI glDirect3DTexture2::GetHandle(LPDIRECT3DDEVICE2 lpDirect3DDevice2, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice2,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice7 *glD3DDev7 = ((glDirect3DDevice2*)lpDirect3DDevice2)->GetGLD3DDev7();
	HRESULT ret = glDDS7->GetHandle(glD3DDev7,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DTexture2::Load(LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
	TRACE_ENTER(2,14,this,14,lpD3DTexture2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTexture2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->Load(((glDirect3DTexture2*)lpD3DTexture2)->GetDDS7()));
}
HRESULT WINAPI glDirect3DTexture2::PaletteChanged(DWORD dwStart, DWORD dwCount)
{
	TRACE_ENTER(3,14,this,8,dwStart,8,dwCount);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DTexture2::PaletteChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

glDirect3DTexture1::glDirect3DTexture1(glDirectDrawSurface7 *glDDS7)
{
	TRACE_ENTER(2,14,this,14,glDDS7);
	this->glDDS7 = glDDS7;
	TRACE_EXIT(-1,0);
}
HRESULT WINAPI glDirect3DTexture1::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDDS7->QueryInterface(riid,ppvObj));
}
ULONG WINAPI glDirect3DTexture1::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->textureparent->AddRef());
}
ULONG WINAPI glDirect3DTexture1::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDDS7->textureparent->Release());
}

HRESULT WINAPI glDirect3DTexture1::GetHandle(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DTEXTUREHANDLE lpHandle)
{
	TRACE_ENTER(3,14,this,14,lpDirect3DDevice,14,lpHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DDevice7 *glD3DDev7 = ((glDirect3DDevice1*)lpDirect3DDevice)->GetGLD3DDev7();
	HRESULT ret = glDDS7->GetHandle(glD3DDev7,lpHandle);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DTexture1::Initialize(LPDIRECT3DDEVICE lpD3DDevice, LPDIRECTDRAWSURFACE lpDDSurface)
{
	TRACE_ENTER(3,14,this,14,lpD3DDevice,14,lpDDSurface);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DDERR_ALREADYINITIALIZED);
	return DDERR_ALREADYINITIALIZED;
}
HRESULT WINAPI glDirect3DTexture1::Load(LPDIRECT3DTEXTURE lpD3DTexture)
{
	TRACE_ENTER(2,14,this,14,lpD3DTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTexture) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDDS7->Load(((glDirect3DTexture1*)lpD3DTexture)->GetDDS7()));
}
HRESULT WINAPI glDirect3DTexture1::PaletteChanged(DWORD dwStart, DWORD dwCount)
{
	TRACE_ENTER(3,14,this,8,dwStart,8,dwCount);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DTexture1::PaletteChanged: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DTexture1::Unload()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_EXIT(23,DD_OK);
	return DD_OK;
}
