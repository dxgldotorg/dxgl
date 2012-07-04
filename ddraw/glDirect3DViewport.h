// DXGL
// Copyright (C) 2011-2012 William Feely

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

class glDirect3DLight;
class glDirect3DDevice7;
class glDirectDrawSurface7;
class glDirect3DViewport3 : public IDirect3DViewport3
{
public:
	glDirect3DViewport3();
	virtual ~glDirect3DViewport3();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddLight(LPDIRECT3DLIGHT lpDirect3DLight);
	HRESULT WINAPI Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags);
	HRESULT WINAPI Clear2(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	HRESULT WINAPI DeleteLight(LPDIRECT3DLIGHT lpDirect3DLight);
	HRESULT WINAPI GetBackground(LPD3DMATERIALHANDLE lphMat, LPBOOL lpValid);
	HRESULT WINAPI GetBackgroundDepth(LPDIRECTDRAWSURFACE* lplpDDSurface, LPBOOL lpValid);
	HRESULT WINAPI GetBackgroundDepth2(LPDIRECTDRAWSURFACE4* lplpDDS, LPBOOL lpValid); 
	HRESULT WINAPI GetViewport(LPD3DVIEWPORT lpData);
	HRESULT WINAPI GetViewport2(LPD3DVIEWPORT2 lpData);
	HRESULT WINAPI Initialize(LPDIRECT3D lpDirect3D);
	HRESULT WINAPI LightElements(DWORD dwElementCount, LPD3DLIGHTDATA lpData);
	HRESULT WINAPI NextLight(LPDIRECT3DLIGHT lpDirect3DLight, LPDIRECT3DLIGHT* lplpDirect3DLight, DWORD dwFlags);
	HRESULT WINAPI SetBackground(D3DMATERIALHANDLE hMat);
	HRESULT WINAPI SetBackgroundDepth(LPDIRECTDRAWSURFACE lpDDSurface);
	HRESULT WINAPI SetBackgroundDepth2(LPDIRECTDRAWSURFACE4 lpDDS);
	HRESULT WINAPI SetViewport(LPD3DVIEWPORT lpData);
	HRESULT WINAPI SetViewport2(LPD3DVIEWPORT2 lpData);
	HRESULT WINAPI TransformVertices(DWORD dwVertexCount, LPD3DTRANSFORMDATA lpData, DWORD dwFlags, LPDWORD lpOffscreen);
	void SetCurrent(bool current);
	void SetDevice(glDirect3DDevice7 *device){this->device = device;};
	void Sync();
private:
	ULONG refcount;
	glDirect3DLight *lights[8];
	glDirect3DDevice7 *device;
	glDirectDrawSurface7 *backZ;
	D3DMATERIALHANDLE background;
	D3DVIEWPORT2 viewport;
	D3DVALUE maxX;
	D3DVALUE maxY;
	D3DVALUE scaleX;
	D3DVALUE scaleY;
	bool current;
};

#endif //__GLDIRECT3DVIEWPORT_H