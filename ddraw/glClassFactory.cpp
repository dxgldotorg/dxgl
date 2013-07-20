// DXGL
// Copyright (C) 2011 William Feely

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
#include "glDirectDraw.h"
#include "texture.h"
#include "glutil.h"
#include "glDirectDrawClipper.h"

LONG locks;

ULONG WINAPI glClassFactory::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glClassFactory::Release()
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

HRESULT WINAPI glClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
		*ppvObj = this;
		this->AddRef();
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
HRESULT WINAPI glClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	TRACE_ENTER(4,14,this,14,pUnkOuter,24,&riid,14,ppvObject);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDraw7 *glDD7;
	if(pUnkOuter != NULL) TRACE_RET(HRESULT,23,CLASS_E_NOAGGREGATION);
	if(riid == IID_IDirectDraw)
	{
		glDD7 = new glDirectDraw7;
		glDD7->QueryInterface(IID_IDirectDraw,ppvObject);
		glDD7->Release();
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw2)
	{
		glDD7 = new glDirectDraw7;
		glDD7->QueryInterface(IID_IDirectDraw2,ppvObject);
		glDD7->Release();
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw4)
	{
		glDD7 = new glDirectDraw7;
		glDD7->QueryInterface(IID_IDirectDraw4,ppvObject);
		glDD7->Release();
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDraw7)
	{
		*ppvObject = new glDirectDraw7();
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	if(riid == IID_IDirectDrawClipper)
	{
		*ppvObject = new glDirectDrawClipper();
		TRACE_VAR("*ppvObject",14,*ppvObject);
		TRACE_EXIT(23,S_OK);
		return S_OK;
	}
	FIXME("glClassFactory::CreateInterface: stub");
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}
HRESULT WINAPI glClassFactory::LockServer(BOOL fLock)
{
	TRACE_ENTER(2,14,this,22,fLock);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(fLock) InterlockedIncrement(&locks);
	else InterlockedDecrement(&locks);
	TRACE_EXIT(23,S_OK);
	return S_OK;
}

