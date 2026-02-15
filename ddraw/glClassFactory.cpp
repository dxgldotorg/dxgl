// DXGL
// Copyright (C) 2011-2026 William Feely

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
#include "glClassFactory.h"
#include "DXGLTexture.h"
#include "DXGLRenderer.h"
#include "glDirectDraw.h"
#include "glTexture.h"
#include "glUtil.h"
#include "glDirectDrawClipper.h"
#include "ddraw.h"
#include "hooks.h"

glClassFactoryVtbl glClassFactory_impl =
{
	glClassFactory_QueryInterface,
	glClassFactory_AddRef,
	glClassFactory_Release,
	glClassFactory_CreateInstance,
	glClassFactory_LockServer
};

HRESULT glClassFactory_Create(glClassFactory **factory)
{
	glClassFactory *This;
	TRACE_ENTER(1, 14, factory);
	This = (glClassFactory*)malloc(sizeof(glClassFactory));
	if (!This) TRACE_RET(HRESULT, 23, E_OUTOFMEMORY);
	This->lpVtbl = &glClassFactory_impl;
	This->refcount = 1;
	This->lockcount = 0;
	*factory = This;
	TRACE_EXIT(23, S_OK);
	return S_OK;
}

ULONG WINAPI glClassFactory_AddRef(glClassFactory *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement((LONG*)&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}
ULONG WINAPI glClassFactory_Release(glClassFactory *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement((LONG*)&This->refcount);
	ret = This->refcount;
	if(This->refcount == 0) free(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glClassFactory_QueryInterface(glClassFactory *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if (!ppvObj) TRACE_RET(HRESULT, 23, E_INVALIDARG);
	*ppvObj = NULL;
	if (!This) TRACE_RET(HRESULT, 23, E_POINTER);
	if (!&riid) TRACE_RET(HRESULT, 23, E_POINTER);
	if((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
		*ppvObj = This;
		glClassFactory_AddRef(This);
	}
	else
	{
		*ppvObj = NULL;
		TRACE_RET(HRESULT,23,E_NOINTERFACE);
	}
	TRACE_VAR("*ppvObj",14,*ppvObj);
	TRACE_EXIT(23,S_OK);
	return S_OK;
}
HRESULT WINAPI glClassFactory_CreateInstance(glClassFactory *This, IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	HRESULT error;
	TRACE_ENTER(4,14,This,14,pUnkOuter,24,&riid,14,ppvObject);
	if (!This) TRACE_RET(HRESULT, 23, E_POINTER);
	glDirectDraw7 *glDD7;
	if(pUnkOuter != NULL) TRACE_RET(HRESULT,23,CLASS_E_NOAGGREGATION);
	if(riid == IID_IDirectDraw)
	{
		InitHooks();
		error = glDirectDraw7_Create(&glDD7);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		glDD7 = new glDirectDraw7;
		glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw,ppvObject);
		glDirectDraw7_Release(glDD7);
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw2)
	{
		InitHooks();
		error = glDirectDraw7_Create(&glDD7);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw2,ppvObject);
		glDirectDraw7_Release(glDD7);
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw4)
	{
		InitHooks();
		error = glDirectDraw7_Create(&glDD7);
		if (FAILED(error)) TRACE_RET(HRESULT, 23, error);
		glDirectDraw7_QueryInterface(glDD7,IID_IDirectDraw4,ppvObject);
		glDirectDraw7_Release(glDD7);
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw7)
	{
		InitHooks();
		error = glDirectDraw7_Create((glDirectDraw7**)ppvObject);
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,error);
		return error;
	}
	if(riid == IID_IDirectDrawClipper)
	{
		TRACE_RET(HRESULT, 23, glDirectDrawClipper_CreateNoInit((LPDIRECTDRAWCLIPPER*)ppvObject));
	}
	FIXME("glClassFactory_CreateInterface: stub");
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
HRESULT WINAPI glClassFactory_LockServer(glClassFactory *This, BOOL fLock)
{
	TRACE_ENTER(2,14,This,22,fLock);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(fLock) InterlockedIncrement((LONG*)&This->lockcount);
	else InterlockedDecrement((LONG*)&This->lockcount);
	TRACE_EXIT(23,S_OK);
	return S_OK;
}

