// DXGL
// Copyright (C) 2011-2013 William Feely

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
#include "MultiDD.h"


MultiDirectDraw::MultiDirectDraw(int version, HRESULT *error, GUID *lpGUID)
{
	dd1 = NULL;
	dd2 = NULL;
	dd4 = NULL;
	dd7 = NULL;
	this->version = version;
	*error = DD_OK;
	if(version < 7) *error = DirectDrawCreate(lpGUID,&dd1,NULL);
	if(FAILED(*error)) return;
	switch(version)
	{
	case 1:
		break;
	case 2:
	case 3:
		*error = dd1->QueryInterface(IID_IDirectDraw2,(LPVOID*)&dd2);
		dd1->Release();
		dd1 = NULL;
		break;
	case 4:
		*error = dd1->QueryInterface(IID_IDirectDraw4,(LPVOID*)&dd4);
		dd1->Release();
		dd1 = NULL;
		break;
	case 7:
		*error = DirectDrawCreateEx(lpGUID,(LPVOID*)&dd7,IID_IDirectDraw7,NULL);
		break;
	default:
		*error = DDERR_INVALIDPARAMS;
		break;
	}
	refcount = 1;
}

MultiDirectDraw::~MultiDirectDraw()
{
	if(dd1) dd1->Release();
	if(dd2) dd2->Release();
	if(dd4) dd4->Release();
	if(dd7) dd7->Release();
}

ULONG MultiDirectDraw::AddRef()
{
	refcount++;
	return refcount;
}

ULONG MultiDirectDraw::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(!refcount) delete this;
	return ret;
}

HRESULT MultiDirectDraw::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	switch(version)
	{
	case 1:
		return dd1->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
	case 2:
	case 3:
		return dd2->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
	case 4:
		return dd4->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
	case 7:
		return dd7->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDraw::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	switch(version)
	{
	case 1:
		return dd1->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
	case 2:
	case 3:
		return dd2->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
	case 4:
		return dd4->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
	case 7:
		return dd7->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDraw::CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, MultiDirectDrawSurface FAR **lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	ddsm = NULL;
	HRESULT error;
	switch(version)
	{
	case 1:
		error = dd1->CreateSurface((LPDDSURFACEDESC)lpDDSurfaceDesc2,&dds1,pUnkOuter);
		if(error != DD_OK) break;
		ddsm = new MultiDirectDrawSurface(1,(LPVOID)dds1);
		break;
	case 2:
		error = dd2->CreateSurface((LPDDSURFACEDESC)lpDDSurfaceDesc2,&dds1,pUnkOuter);
		if(error != DD_OK) break;
		dds1->QueryInterface(IID_IDirectDrawSurface2,(LPVOID*)&dds2);
		dds1->Release();
		ddsm = new MultiDirectDrawSurface(2,(LPVOID)dds2);
		break;
	case 3:
		error = dd2->CreateSurface((LPDDSURFACEDESC)lpDDSurfaceDesc2,&dds1,pUnkOuter);
		if(error != DD_OK) break;
		dds1->QueryInterface(IID_IDirectDrawSurface3,(LPVOID*)&dds3);
		dds1->Release();
		ddsm = new MultiDirectDrawSurface(3,(LPVOID)dds3);
		break;
	case 4:
		error = dd4->CreateSurface(lpDDSurfaceDesc2,&dds4,pUnkOuter);
		if(error != DD_OK) break;
		ddsm = new MultiDirectDrawSurface(4,(LPVOID)dds4);
		break;
	case 7:
		error = dd7->CreateSurface(lpDDSurfaceDesc2,&dds7,pUnkOuter);
		if(error != DD_OK) break;
		ddsm = new MultiDirectDrawSurface(7,(LPVOID)dds7);
		break;
	default:
		return DDERR_GENERIC;
	}
	if(ddsm) *lplpDDSurface = ddsm;
	return error;
}

HRESULT MultiDirectDraw::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	switch (version)
	{
	case 1:
		return dd1->GetCaps(lpDDDriverCaps, lpDDHELCaps);
	case 2:
	case 3:
		return dd2->GetCaps(lpDDDriverCaps, lpDDHELCaps);
	case 4:
		return dd4->GetCaps(lpDDDriverCaps, lpDDHELCaps);
	case 7:
		return dd7->GetCaps(lpDDDriverCaps, lpDDHELCaps);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDraw::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	switch(version)
	{
	case 1:
		return dd1->SetCooperativeLevel(hWnd,dwFlags);
	case 2:
	case 3:
		return dd2->SetCooperativeLevel(hWnd,dwFlags);
	case 4:
		return dd4->SetCooperativeLevel(hWnd,dwFlags);
	case 7:
		return dd7->SetCooperativeLevel(hWnd,dwFlags);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDraw::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	switch(version)
	{
	case 1:
		return dd1->SetDisplayMode(dwWidth,dwHeight,dwBPP);
	case 2:
	case 3:
		return dd2->SetDisplayMode(dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
	case 4:
		return dd4->SetDisplayMode(dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
	case 7:
		return dd7->SetDisplayMode(dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
	default:
		return  DDERR_GENERIC;
	}
}
HRESULT MultiDirectDraw::QueryInterface(REFIID riid, void** ppvObj)
{
	switch(version)
	{
	case 1:
		return dd1->QueryInterface(riid,ppvObj);
	case 2:
	case 3:
		return dd2->QueryInterface(riid,ppvObj);
	case 4:
		return dd4->QueryInterface(riid,ppvObj);
	case 7:
		return dd7->QueryInterface(riid,ppvObj);
	default:
		return NULL;
	}
}



MultiDirectDrawSurface::MultiDirectDrawSurface(int version, LPVOID surface)
{
	dds1 = NULL;
	dds2 = NULL;
	dds3 = NULL;
	dds4 = NULL;
	dds7 = NULL;
	this->version = version;
	switch(version)
	{
	case 1:
		dds1 = (LPDIRECTDRAWSURFACE)surface;
		break;
	case 2:
		dds2 = (LPDIRECTDRAWSURFACE2)surface;
		break;
	case 3:
		dds3 = (LPDIRECTDRAWSURFACE3)surface;
		break;
	case 4:
		dds4 = (LPDIRECTDRAWSURFACE4)surface;
		break;
	case 7:
		dds7 = (LPDIRECTDRAWSURFACE7)surface;
		break;
	default:
		break;
	}
	refcount = 1;
}

MultiDirectDrawSurface::~MultiDirectDrawSurface()
{
	if(dds1) dds1->Release();
	if(dds2) dds2->Release();
	if(dds3) dds3->Release();
	if(dds4) dds4->Release();
	if(dds7) dds7->Release();
}


ULONG MultiDirectDrawSurface::AddRef()
{
	refcount++;
	return refcount;
}

ULONG MultiDirectDrawSurface::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(!refcount) delete this;
	return ret;
}

HRESULT MultiDirectDrawSurface::AddAttachedSurface(MultiDirectDrawSurface *lpDDSAttachedSurface)
{
	switch(version)
	{
	case 1:
		return dds1->AddAttachedSurface(lpDDSAttachedSurface->dds1);
	case 2:
		return dds2->AddAttachedSurface(lpDDSAttachedSurface->dds2);
	case 3:
		return dds3->AddAttachedSurface(lpDDSAttachedSurface->dds3);
	case 4:
		return dds4->AddAttachedSurface(lpDDSAttachedSurface->dds4);
	case 7:
		return dds7->AddAttachedSurface(lpDDSAttachedSurface->dds7);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::Lock(LPRECT lpDestRect, LPDDSURFACEDESC2 lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
	switch(version)
	{
	case 1:
		return dds1->Lock(lpDestRect,(LPDDSURFACEDESC)lpDDSurfaceDesc,dwFlags,hEvent);
	case 2:
		return dds2->Lock(lpDestRect,(LPDDSURFACEDESC)lpDDSurfaceDesc,dwFlags,hEvent);
	case 3:
		return dds3->Lock(lpDestRect,(LPDDSURFACEDESC)lpDDSurfaceDesc,dwFlags,hEvent);
	case 4:
		return dds4->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent);
	case 7:
		return dds7->Lock(lpDestRect,lpDDSurfaceDesc,dwFlags,hEvent);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::Unlock(LPRECT lpRect)
{
	switch(version)
	{
	case 1:
		return dds1->Unlock(lpRect);
	case 2:
		return dds2->Unlock(lpRect);
	case 3:
		return dds3->Unlock(lpRect);
	case 4:
		return dds4->Unlock(lpRect);
	case 7:
		return dds7->Unlock(lpRect);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::GetPalette(LPDIRECTDRAWPALETTE FAR *lplpDDPalette)
{
	switch(version)
	{
	case 1:
		return dds1->GetPalette(lplpDDPalette);
	case 2:
		return dds2->GetPalette(lplpDDPalette);
	case 3:
		return dds3->GetPalette(lplpDDPalette);
	case 4:
		return dds4->GetPalette(lplpDDPalette);
	case 7:
		return dds7->GetPalette(lplpDDPalette);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDrawSurface::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	switch(version)
	{
	case 1:
		return dds1->SetPalette(lpDDPalette);
	case 2:
		return dds2->SetPalette(lpDDPalette);
	case 3:
		return dds3->SetPalette(lpDDPalette);
	case 4:
		return dds4->SetPalette(lpDDPalette);
	case 7:
		return dds7->SetPalette(lpDDPalette);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::GetClipper(LPDIRECTDRAWCLIPPER FAR *lplpDDClipper)
{
	switch(version)
	{
	case 1:
		return dds1->GetClipper(lplpDDClipper);
	case 2:
		return dds2->GetClipper(lplpDDClipper);
	case 3:
		return dds3->GetClipper(lplpDDClipper);
	case 4:
		return dds4->GetClipper(lplpDDClipper);
	case 7:
		return dds7->GetClipper(lplpDDClipper);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDrawSurface::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	switch(version)
	{
	case 1:
		return dds1->SetClipper(lpDDClipper);
	case 2:
		return dds2->SetClipper(lpDDClipper);
	case 3:
		return dds3->SetClipper(lpDDClipper);
	case 4:
		return dds4->SetClipper(lpDDClipper);
	case 7:
		return dds7->SetClipper(lpDDClipper);
	default:
		return DDERR_GENERIC;
	}
}
HRESULT MultiDirectDrawSurface::GetSurfaceDesc(LPDDSURFACEDESC2 lpDDSurfaceDesc)
{
	switch(version)
	{
	case 1:
		return dds1->GetSurfaceDesc((LPDDSURFACEDESC)lpDDSurfaceDesc);
	case 2:
		return dds2->GetSurfaceDesc((LPDDSURFACEDESC)lpDDSurfaceDesc);
	case 3:
		return dds3->GetSurfaceDesc((LPDDSURFACEDESC)lpDDSurfaceDesc);
	case 4:
		return dds4->GetSurfaceDesc(lpDDSurfaceDesc);
	case 7:
		return dds7->GetSurfaceDesc(lpDDSurfaceDesc);
	default:
		return DDERR_GENERIC;
	}
}


HRESULT MultiDirectDrawSurface::GetAttachedSurface(LPDDSCAPS2 lpDDSCaps, MultiDirectDrawSurface FAR **lplpDDAttachedSurface)
{
	VOID *tmpsurface = NULL;
	HRESULT error;
	switch(version)
	{
	case 1:
		error = dds1->GetAttachedSurface((LPDDSCAPS)lpDDSCaps,(LPDIRECTDRAWSURFACE*)&tmpsurface);
		if(error != DD_OK) return error;
		break;
	case 2:
		error = dds2->GetAttachedSurface((LPDDSCAPS)lpDDSCaps,(LPDIRECTDRAWSURFACE2*)&tmpsurface);
		if(error != DD_OK) return error;
		break;
	case 3:
		error = dds3->GetAttachedSurface((LPDDSCAPS)lpDDSCaps,(LPDIRECTDRAWSURFACE3*)&tmpsurface);
		if(error != DD_OK) return error;
		break;
	case 4:
		error = dds4->GetAttachedSurface(lpDDSCaps,(LPDIRECTDRAWSURFACE4*)&tmpsurface);
		if(error != DD_OK) return error;
		break;
	case 7:
		error = dds7->GetAttachedSurface(lpDDSCaps,(LPDIRECTDRAWSURFACE7*)&tmpsurface);
		if(error != DD_OK) return error;
		break;
	default:
		return DDERR_GENERIC;
	}
	if(tmpsurface)
	{
		switch(version)
		{
		case 1:
			ddsm = new MultiDirectDrawSurface(1,tmpsurface);
			*lplpDDAttachedSurface = ddsm;
			return DD_OK;
		case 2:
			ddsm = new MultiDirectDrawSurface(2,tmpsurface);
			*lplpDDAttachedSurface = ddsm;
			return DD_OK;
		case 3:
			ddsm = new MultiDirectDrawSurface(3,tmpsurface);
			*lplpDDAttachedSurface = ddsm;
			return DD_OK;
		case 4:
			ddsm = new MultiDirectDrawSurface(4,tmpsurface);
			*lplpDDAttachedSurface = ddsm;
			return DD_OK;
		case 7:
			ddsm = new MultiDirectDrawSurface(7,tmpsurface);
			*lplpDDAttachedSurface = ddsm;
			return DD_OK;
		default:
			return DDERR_GENERIC;
		}
	}
	return DDERR_GENERIC;
}

HRESULT MultiDirectDrawSurface::Flip(MultiDirectDrawSurface *lpDDSurfaceTargetOverride,DWORD dwFlags)
{
	if(lpDDSurfaceTargetOverride)
	{
		switch(version)
		{
		case 1:
			return dds1->Flip(lpDDSurfaceTargetOverride->dds1,dwFlags);
		case 2:
			return dds2->Flip(lpDDSurfaceTargetOverride->dds2,dwFlags);
		case 3:
			return dds3->Flip(lpDDSurfaceTargetOverride->dds3,dwFlags);
		case 4:
			return dds4->Flip(lpDDSurfaceTargetOverride->dds4,dwFlags);
		case 7:
			return dds7->Flip(lpDDSurfaceTargetOverride->dds7,dwFlags);
		default:
			return DDERR_GENERIC;
		}
	}
	else
	{
		switch(version)
		{
		case 1:
			return dds1->Flip(NULL,dwFlags);
		case 2:
			return dds2->Flip(NULL,dwFlags);
		case 3:
			return dds3->Flip(NULL,dwFlags);
		case 4:
			return dds4->Flip(NULL,dwFlags);
		case 7:
			return dds7->Flip(NULL,dwFlags);
		default:
			return DDERR_GENERIC;
		}
	}
	return DDERR_GENERIC;
}

HRESULT MultiDirectDrawSurface::Blt(LPRECT lpDestRect, MultiDirectDrawSurface *lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	if(lpDDSrcSurface)
	{
		switch(version)
		{
		case 1:
			return dds1->Blt(lpDestRect,lpDDSrcSurface->dds1,lpSrcRect,dwFlags,lpDDBltFx);
		case 2:
			return dds2->Blt(lpDestRect,lpDDSrcSurface->dds2,lpSrcRect,dwFlags,lpDDBltFx);
		case 3:
			return dds3->Blt(lpDestRect,lpDDSrcSurface->dds3,lpSrcRect,dwFlags,lpDDBltFx);
		case 4:
			return dds4->Blt(lpDestRect,lpDDSrcSurface->dds4,lpSrcRect,dwFlags,lpDDBltFx);
		case 7:
			return dds7->Blt(lpDestRect,lpDDSrcSurface->dds7,lpSrcRect,dwFlags,lpDDBltFx);
		}
	}
	else
	{
		switch(version)
		{
		case 1:
			return dds1->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
		case 2:
			return dds2->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
		case 3:
			return dds3->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
		case 4:
			return dds4->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
		case 7:
			return dds7->Blt(lpDestRect,NULL,lpSrcRect,dwFlags,lpDDBltFx);
		}
	}
	return DDERR_GENERIC;
}

HRESULT MultiDirectDrawSurface::BltFast(DWORD dwX, DWORD dwY, MultiDirectDrawSurface *lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans)
{
	if(lpDDSrcSurface)
	{
		switch(version)
		{
		case 1:
			return dds1->BltFast(dwX,dwY,lpDDSrcSurface->dds1,lpSrcRect,dwTrans);
		case 2:
			return dds2->BltFast(dwX,dwY,lpDDSrcSurface->dds2,lpSrcRect,dwTrans);
		case 3:
			return dds3->BltFast(dwX,dwY,lpDDSrcSurface->dds3,lpSrcRect,dwTrans);
		case 4:
			return dds4->BltFast(dwX,dwY,lpDDSrcSurface->dds4,lpSrcRect,dwTrans);
		case 7:
			return dds7->BltFast(dwX,dwY,lpDDSrcSurface->dds7,lpSrcRect,dwTrans);
		}
	}
	else
	{
		switch(version)
		{
		case 1:
			return dds1->BltFast(dwX,dwY,NULL,lpSrcRect,dwTrans);
		case 2:
			return dds2->BltFast(dwX,dwY,NULL,lpSrcRect,dwTrans);
		case 3:
			return dds3->BltFast(dwX,dwY,NULL,lpSrcRect,dwTrans);
		case 4:
			return dds4->BltFast(dwX,dwY,NULL,lpSrcRect,dwTrans);
		case 7:
			return dds7->BltFast(dwX,dwY,NULL,lpSrcRect,dwTrans);
		}
	}
	return DDERR_GENERIC;
}

HRESULT MultiDirectDrawSurface::GetDC(HDC FAR *lphDC)
{
	switch(version)
	{
	case 1:
		return dds1->GetDC(lphDC);
	case 2:
		return dds2->GetDC(lphDC);
	case 3:
		return dds3->GetDC(lphDC);
	case 4:
		return dds4->GetDC(lphDC);
	case 7:
		return dds7->GetDC(lphDC);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::ReleaseDC(HDC hDC)
{
	switch(version)
	{
	case 1:
		return dds1->ReleaseDC(hDC);
	case 2:
		return dds2->ReleaseDC(hDC);
	case 3:
		return dds3->ReleaseDC(hDC);
	case 4:
		return dds4->ReleaseDC(hDC);
	case 7:
		return dds7->ReleaseDC(hDC);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::SetColorKey(DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	switch(version)
	{
	case 1:
		return dds1->SetColorKey(dwFlags,lpDDColorKey);
	case 2:
		return dds2->SetColorKey(dwFlags,lpDDColorKey);
	case 3:
		return dds3->SetColorKey(dwFlags,lpDDColorKey);
	case 4:
		return dds4->SetColorKey(dwFlags,lpDDColorKey);
	case 7:
		return dds7->SetColorKey(dwFlags,lpDDColorKey);
	default:
		return DDERR_GENERIC;
	}
}

LPVOID MultiDirectDrawSurface::GetSurface()
{
	switch(version)
	{
	case 1:
		return dds1;
	case 2:
		return dds2;
	case 3:
		return dds3;
	case 4:
		return dds4;
	case 7:
		return dds7;
	default:
		return NULL;
	}
}

HRESULT MultiDirectDrawSurface::QueryInterface(REFIID riid, void** ppvObj)
{
	switch(version)
	{
	case 1:
		return dds1->QueryInterface(riid,ppvObj);
	case 2:
		return dds2->QueryInterface(riid,ppvObj);
	case 3:
		return dds3->QueryInterface(riid,ppvObj);
	case 4:
		return dds4->QueryInterface(riid,ppvObj);
	case 7:
		return dds7->QueryInterface(riid,ppvObj);
	default:
		return DDERR_GENERIC;
	}
}

HRESULT MultiDirectDrawSurface::UpdateOverlay(LPRECT lpSrcRect, MultiDirectDrawSurface *lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx)
{
	switch (version)
	{
	case 1:
		return dds1->UpdateOverlay(lpSrcRect, lpDDDestSurface->dds1, lpDestRect, dwFlags, lpDDOverlayFx);
	case 2:
		return dds2->UpdateOverlay(lpSrcRect, lpDDDestSurface->dds2, lpDestRect, dwFlags, lpDDOverlayFx);
	case 3:
		return dds3->UpdateOverlay(lpSrcRect, lpDDDestSurface->dds3, lpDestRect, dwFlags, lpDDOverlayFx);
	case 4:
		return dds4->UpdateOverlay(lpSrcRect, lpDDDestSurface->dds4, lpDestRect, dwFlags, lpDDOverlayFx);
	case 7:
		return dds7->UpdateOverlay(lpSrcRect, lpDDDestSurface->dds7, lpDestRect, dwFlags, lpDDOverlayFx);
	default:
		return DDERR_GENERIC;
	}
}