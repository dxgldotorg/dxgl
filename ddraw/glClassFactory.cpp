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
#include "glDirectDrawClipper.h"

LONG locks;

ULONG WINAPI glClassFactory::AddRef()
{
	if(!this) return DDERR_INVALIDPARAMS;
	refcount++;
	return refcount;
}
ULONG WINAPI glClassFactory::Release()
{
	if(!this) return DDERR_INVALIDPARAMS;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if((riid == IID_IUnknown) || (riid == IID_IClassFactory))
	{
		*ppvObj = this;
		this->AddRef();
	}
	else
	{
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}
HRESULT WINAPI glClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	if(!this) return DDERR_INVALIDPARAMS;
	glDirectDraw7 *glDD7;
	if(pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;
	if(riid == IID_IDirectDraw)
	{
		glDD7 = new glDirectDraw7;
		*ppvObject = new glDirectDraw1(glDD7);
		return S_OK;
	}
	if(riid == IID_IDirectDraw2)
	{
		glDD7 = new glDirectDraw7;
		*ppvObject = new glDirectDraw2(glDD7);
		return S_OK;
	}
	if(riid == IID_IDirectDraw4)
	{
		glDD7 = new glDirectDraw7;
		*ppvObject = new glDirectDraw4(glDD7);
		return S_OK;
	}
	if(riid == IID_IDirectDraw7)
	{
		*ppvObject = new glDirectDraw7();
		return S_OK;
	}
	if(riid == IID_IDirectDrawClipper)
	{
		*ppvObject = new glDirectDrawClipper();
	}
	FIXME("glClassFactory::CreateInterface: stub");
	return E_NOINTERFACE;
}
HRESULT WINAPI glClassFactory::LockServer(BOOL fLock)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(fLock) InterlockedIncrement(&locks);
	else InterlockedDecrement(&locks);
	return S_OK;
}

