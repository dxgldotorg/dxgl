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
#ifndef _DXGLCLASSFACTORY_H
#define _DXGLCLASSFACTORY_H

struct glClassFactoryVtbl;
typedef struct glClassFactory
{
	glClassFactoryVtbl *lpVtbl;
	ULONG refcount;
	ULONG lockcount;
} glClassFactory;

typedef struct glClassFactoryVtbl
{
	HRESULT(WINAPI *QueryInterface)(glClassFactory *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glClassFactory *This);
	ULONG(WINAPI *Release)(glClassFactory *This);
	HRESULT(WINAPI *CreateInstance)(glClassFactory *This, IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	HRESULT(WINAPI *LockServer)(glClassFactory *This, BOOL fLock);
} glClassFactoryVtbl;

HRESULT glClassFactory_Create(glClassFactory **factory);
ULONG WINAPI glClassFactory_AddRef(glClassFactory *This);
ULONG WINAPI glClassFactory_Release(glClassFactory *This);
HRESULT WINAPI glClassFactory_QueryInterface(glClassFactory *This, REFIID riid, void** ppvObj);
HRESULT WINAPI glClassFactory_CreateInstance(glClassFactory *This, IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
HRESULT WINAPI glClassFactory_LockServer(glClassFactory *This, BOOL fLock);

#endif //_DXGLCLASSFACTORY_H
