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

#pragma once
#ifndef _DXGLCLASSFACTORY_H
#define _DXGLCLASSFACTORY_H

class glClassFactory : public IClassFactory
{
public:
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	HRESULT WINAPI CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	HRESULT WINAPI LockServer(BOOL fLock);
	glClassFactory() {refcount = 1; lockcount = 0;}
	~glClassFactory() {}
private:
	ULONG refcount;
	ULONG lockcount;
};
#endif //_DXGLCLASSFACTORY_H
