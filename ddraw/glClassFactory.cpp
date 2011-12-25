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

ULONG WINAPI glClassFactory::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glClassFactory::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glClassFactory::QueryInterface(REFIID riid, void** ppvObj)
{
	FIXME("glClassFactory::QueryInterface: stub");
	return E_FAIL;
}
HRESULT WINAPI glClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	FIXME("glClassFactory::CreateInterface: stub");
	return E_FAIL;
}
HRESULT WINAPI glClassFactory::LockServer(BOOL fLock)
{
	FIXME("glClassFactory::LockServer: stub");
	return E_FAIL;
}

