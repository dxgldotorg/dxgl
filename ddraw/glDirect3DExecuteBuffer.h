// DXGL
// Copyright (C) 2013 William Feely

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

class glDirect3DExecuteBuffer : public IDirect3DExecuteBuffer

{
public:
	glDirect3DExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc);
	virtual ~glDirect3DExecuteBuffer();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI GetExecuteData(LPD3DEXECUTEDATA lpData);
	HRESULT WINAPI Initialize(LPDIRECT3DDEVICE lpDirect3DDevice, LPD3DEXECUTEBUFFERDESC lpDesc);
	HRESULT WINAPI Lock(LPD3DEXECUTEBUFFERDESC lpDesc);
	HRESULT WINAPI Optimize(DWORD dwDummy);
	HRESULT WINAPI SetExecuteData(LPD3DEXECUTEDATA lpData);
	HRESULT WINAPI Unlock();
	HRESULT WINAPI Validate(LPDWORD lpdwOffset, LPD3DVALIDATECALLBACK lpFunc, LPVOID lpUserArg, DWORD dwReserved);
	HRESULT ExecuteLock(LPD3DEXECUTEBUFFERDESC lpDesc,LPD3DEXECUTEDATA lpData);
	HRESULT ExecuteUnlock();
private:
	ULONG refcount;
	D3DEXECUTEBUFFERDESC desc;
	D3DEXECUTEDATA datadesc;
	unsigned char *data;
	bool locked;
	bool inuse;
};