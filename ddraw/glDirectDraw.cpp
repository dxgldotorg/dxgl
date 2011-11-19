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

#include "stdafx.h"
#include "shaders.h"
#include "ddraw.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"

bool directdraw_created = false; // emulate only one ddraw device
bool wndclasscreated = false;

HRESULT EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	bool match;
	DWORD modenum = 0;
	DWORD modemax = 128;
	DEVMODE mode;
	ZeroMemory(&mode,sizeof(DEVMODE));
	mode.dmSize = sizeof(DEVMODE);
	DEVMODE *modes = (DEVMODE*)malloc(128*sizeof(DEVMODE));
	DEVMODE *tmp;
	if(!modes) return DDERR_OUTOFMEMORY;
	DDSURFACEDESC ddmode;
	ZeroMemory(&ddmode,sizeof(DDSURFACEDESC));
	ddmode.dwSize = sizeof(DDSURFACEDESC);
	ddmode.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
	while(EnumDisplaySettings(NULL,modenum++,&mode))
	{
		modes[modenum-1] = mode;
		if(modenum >= modemax)
		{
			modemax += 128;
			tmp = (DEVMODE*)realloc(modes,modemax*sizeof(DEVMODE));
			if(tmp == NULL)
			{
				free(modes);
				return DDERR_OUTOFMEMORY;
			}
			modes = tmp;
		}
	}
	modenum--;
	for(DWORD i = 0; i < modenum; i++)
	{
		match = true;
		if(dwFlags & DDEDM_REFRESHRATES) ddmode.dwRefreshRate = modes[i].dmDisplayFrequency;
		else
		{
			ddmode.dwRefreshRate = 0;
			for(DWORD x = 0; x < i; x++)
				if((modes[x].dmBitsPerPel == modes[i].dmBitsPerPel) &&
					(modes[x].dmPelsWidth == modes[i].dmPelsWidth) &&
					(modes[x].dmPelsHeight == modes[i].dmPelsHeight)) match = false;
		}
		if(lpDDSurfaceDesc)
		{
			if(lpDDSurfaceDesc->dwFlags & DDSD_WIDTH)
				if(lpDDSurfaceDesc->dwWidth != modes[i].dmPelsWidth) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT)
				if(lpDDSurfaceDesc->dwHeight != modes[i].dmPelsHeight) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
				if(lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != modes[i].dmBitsPerPel) match = false;
			if(lpDDSurfaceDesc->dwFlags & DDSD_REFRESHRATE)
				if(lpDDSurfaceDesc->dwRefreshRate != modes[i].dmDisplayFrequency) match = false;
		}
		if(match)
		{
			if(modes[i].dmBitsPerPel == 8) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
			else if(modes[i].dmBitsPerPel == 4) ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
			else ddmode.ddpfPixelFormat.dwFlags = DDPF_RGB;
			if(modes[i].dmBitsPerPel == 8)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0;
				ddmode.ddpfPixelFormat.dwGBitMask = 0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0;
			}
			else if(modes[i].dmBitsPerPel == 15)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddmode.ddpfPixelFormat.dwRBitMask = 0x1F;
			}
			else if(modes[i].dmBitsPerPel == 16)
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddmode.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddmode.ddpfPixelFormat.dwBBitMask = 0x1F;
			}
			else
			{
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF00;
				ddmode.ddpfPixelFormat.dwRBitMask = 0xFF;
			}
			ddmode.ddpfPixelFormat.dwRGBBitCount = modes[i].dmBitsPerPel;
			ddmode.dwWidth = modes[i].dmPelsWidth;
			ddmode.dwHeight = modes[i].dmPelsHeight;
			if(modes[i].dmBitsPerPel == 15) ddmode.lPitch = modes[i].dmPelsWidth * 2;
			else if(modes[i].dmBitsPerPel == 4) ddmode.lPitch = modes[i].dmPelsWidth / 2;
			else ddmode.lPitch = modes[i].dmPelsWidth * (modes[i].dmBitsPerPel / 8);
			if(lpEnumModesCallback(&ddmode,lpContext) == DDENUMRET_CANCEL) return DD_OK;
		}
	}
	free(modes);
	return DD_OK;
}

#pragma region DDRAW7/common routines
glDirectDraw7::glDirectDraw7(GUID FAR* lpGUID, LPDIRECTDRAW FAR* lplpDD, IUnknown FAR* pUnkOuter)
{
	hDC = NULL;
	hRC = NULL;
	hRenderWnd = NULL;
	primary = NULL;
	fullscreen = false;
	fpupreserve = false;
	fpusetup = false;
	threadsafe = false;
	nowindowchanges = false;
	surfaces = (glDirectDrawSurface7 **)malloc(1024*sizeof(glDirectDrawSurface7 *));
	if(!surfaces)
	{
		error = DDERR_OUTOFMEMORY;
		return;
	}
	ZeroMemory(surfaces,1024*sizeof(glDirectDrawSurface7 *));
	surfacecount = 0;
	surfacecountmax = 1024;
	clippers = (glDirectDrawClipper **)malloc(1024*sizeof(glDirectDrawClipper *));
	if(!clippers)
	{
		error = DDERR_OUTOFMEMORY;
		return;
	}
	ZeroMemory(clippers,1024*sizeof(glDirectDrawClipper *));
	clippercount = 0;
	clippercountmax = 1024;
	if(directdraw_created) error = DDERR_DIRECTDRAWALREADYCREATED;
	bool useguid = false;
	if(pUnkOuter) error = DDERR_INVALIDPARAMS ;
	switch((INT_PTR)lpGUID)
	{
	case NULL:
		break;
	case DDCREATE_EMULATIONONLY:
		DEBUG("DDRAW software emulation not supported, using OpenGL.\n");
		break;
	case DDCREATE_HARDWAREONLY:
		DEBUG("DDCREATE_HARDWAREONLY unnecessarily called.\n");
		break;
	default:
		useguid = true;
		FIXME("Display GUIDs not yet supported, using primary.\n");
	}
	refcount = 1;
	error = 0;
}

glDirectDraw7::~glDirectDraw7()
{
	//FIXME("glDirectDraw7::~glDirectDraw7:  Destructor.\n");
	if(clippers) free(clippers);
	if(surfaces)
	{

		free(surfaces);
	}
	DeleteGL();
}

HRESULT WINAPI glDirectDraw7::QueryInterface(REFIID riid, void** ppvObj)
{
	if(riid == IID_IDirectDraw)
	{
		// Create an IDirectDraw1 interface
		this->AddRef();
		*ppvObj = new glDirectDraw1(this);
		this->Release();
		return DD_OK;
	}
	if(riid == IID_IDirectDraw2)
	{
		// Create an IDirectDraw2 interface
		this->AddRef();
		*ppvObj = new glDirectDraw2(this);
		this->Release();
		return DD_OK;
	}
	if(riid == IID_IDirectDraw4)
	{
		// Create an IDirectDraw4 interface
		this->AddRef();
		*ppvObj = new glDirectDraw4(this);
		return DD_OK;
	}
	if(riid == IID_IDirectDraw7)
	{
		// Probably non-DX compliant, but give a copy of the IDirectDraw7 interface
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	if(riid == IID_IDirect3D)
	{
		FIXME("Add IDirect3D Interfaces\n");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirect3D2)
	{
		FIXME("Add IDirect3D Interfaces\n");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirect3D3)
	{
		FIXME("Add IDirect3D Interfaces\n");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirect3D7)
	{
		FIXME("Add IDirect3D Interfaces\n");
		return DDERR_GENERIC;
	}
	if(riid == IID_IDirectDrawGammaControl)
	{
		FIXME("Add IDirect3D Interfaces\n");
		return DDERR_GENERIC;
	}
	/*if(riid == IID_IDDVideoPortContainer)
	{
		return DDERR_GENERIC;
	}*/
	return E_NOINTERFACE;
}
ULONG WINAPI glDirectDraw7::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDraw7::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) 
		delete this;
	return ret;
}
HRESULT WINAPI glDirectDraw7::Compact()
{
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	clippercount++;
	if(clippercount > clippercountmax)
	{
		glDirectDrawClipper **clippers2;
		clippers2 = (glDirectDrawClipper **)realloc(clippers,(clippercountmax+1024)*sizeof(glDirectDrawClipper *));
		if(!clippers2)
			return DDERR_OUTOFMEMORY;
		clippers = clippers2;
		ZeroMemory(&clippers[clippercountmax],1024*sizeof(glDirectDrawClipper *));
		clippercountmax += 1024;
	}
	clippers[clippercount-1] = new glDirectDrawClipper(dwFlags,lplpDDClipper,pUnkOuter,this);
	*lplpDDClipper = clippers[clippercount-1];
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	glDirectDrawPalette *pal = new glDirectDrawPalette(dwFlags,lpDDColorArray,lplpDDPalette);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPDIRECTDRAWSURFACE7 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	if(primary && (lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
		return DDERR_PRIMARYSURFACEALREADYEXISTS;
	surfacecount++;
	if(surfacecount > surfacecountmax)
	{
		glDirectDrawSurface7 **surfaces2;
		surfaces2 = (glDirectDrawSurface7 **)realloc(surfaces,(surfacecountmax+1024)*sizeof(glDirectDrawSurface7 *));
		if(!surfaces2)
			return DDERR_OUTOFMEMORY;
		surfaces = surfaces2;
		ZeroMemory(&surfaces[surfacecountmax],1024*sizeof(glDirectDrawSurface7 *));
		surfacecountmax += 1024;
	}
	HRESULT error;
	surfaces[surfacecount-1] = new glDirectDrawSurface7(this,lpDDSurfaceDesc2,lplpDDSurface,&error,false,NULL);
	if(lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		primary = surfaces[surfacecount-1];
	*lplpDDSurface = surfaces[surfacecount-1];
	return error;
}
HRESULT WINAPI glDirectDraw7::DuplicateSurface(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDIRECTDRAWSURFACE7 FAR *lplpDupDDSurface)
{
	FIXME("IDirectDraw::DuplicateSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	FIXME("IDirectDraw::EnumDisplayModes: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	FIXME("IDirectDraw::EnumSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::FlipToGDISurface()
{
	FIXME("IDirectDraw::FlipToGDISurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	FIXME("IDirectDraw::GetCaps: Fill in as implemented.\n");
	DDCAPS_DX7 ddCaps;
	ZeroMemory(&ddCaps,sizeof(DDCAPS_DX7));
	ddCaps.dwSize = sizeof(DDCAPS_DX7);
	ddCaps.dwCaps = 0; // FIXME:  Fill in capabilities
	ddCaps.dwCaps2 = 0;
	memcpy(lpDDDriverCaps,&ddCaps,sizeof(DDCAPS_DX7));
	memcpy(lpDDHELCaps,&ddCaps,sizeof(DDCAPS_DX7));
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	FIXME("IDirectDraw::GetDisplayMode: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	FIXME("IDirectDraw::GetFourCCCodes: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetGDISurface(LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface)
{
	FIXME("IDirectDraw::GetGDISurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	FIXME("IDirectDraw::GetMonitorFrequency: support multi-monitor\n");
	DEVMODEA devmode;
	devmode.dmSize = sizeof(DEVMODEA);
	EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	*lpdwFrequency = devmode.dmDisplayFrequency;
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetScanLine(LPDWORD lpdwScanLine)
{
	FIXME("IDirectDraw::GetScanLine: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	FIXME("IDirectDraw::GetVerticalBlankStatis: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::Initialize(GUID FAR *lpGUID)
{
	FIXME("IDirectDraw::Initialize: stub\n");
	return DDERR_DIRECTDRAWALREADYCREATED;
}
HRESULT WINAPI glDirectDraw7::RestoreDisplayMode()
{
	FIXME("IDirectDraw::RestoreDisplayMode: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	this->hWnd = hWnd;
	winstyle = GetWindowLongPtrA(hWnd,GWL_STYLE);
	winstyleex = GetWindowLongPtrA(hWnd,GWL_EXSTYLE);
	bool exclusive = false;
	if(dwFlags & DDSCL_ALLOWMODEX)
	{
		// Validate flags
		if(dwFlags & (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE))
		{
			DEBUG("IDirectDraw::SetCooperativeLevel: Mode X not supported, ignoring\n");
		}
		else DEBUG("IDirectDraw::SetCooperativeLevel: DDSCL_ALLOWMODEX without DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE\n");
	}
	if(dwFlags & DDSCL_ALLOWREBOOT)
		DEBUG("IDirectDraw::SetCooperativeLevel: DDSCL_ALLOWREBOOT unnecessary\n");
	if(dwFlags & DDSCL_CREATEDEVICEWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_CREATEDEVICEWINDOW unsupported\n");
	if(dwFlags & DDSCL_EXCLUSIVE)
		exclusive = true;
	else exclusive = false;
	if(dwFlags & DDSCL_FULLSCREEN)
		fullscreen = true;
	else fullscreen = false;
	if(exclusive)
		if(!fullscreen) return DDERR_INVALIDPARAMS;
	if(dwFlags & DDSCL_FPUPRESERVE)
		fpupreserve = true;
	else fpupreserve = false;
	if(dwFlags & DDSCL_FPUSETUP)
		fpusetup = true;
	else fpusetup = false;
	if(dwFlags & DDSCL_MULTITHREADED)
		threadsafe = true;
	else threadsafe = false;
	if(dwFlags & DDSCL_NORMAL)
		if(exclusive) return DDERR_INVALIDPARAMS;
	if(dwFlags & DDSCL_NOWINDOWCHANGES)
		nowindowchanges = true;
	else nowindowchanges = false;
	if(dwFlags & DDSCL_SETDEVICEWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETDEVICEWINDOW unsupported\n");
	if(dwFlags & DDSCL_SETFOCUSWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETFOCUSWINDOW unsupported\n");
	DEVMODEA devmode;
	EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	int x,y,bpp;
	if(fullscreen)
	{
		x = devmode.dmPelsWidth;
		y = devmode.dmPelsHeight;
	}
	else
	{
		RECT rect;
		GetClientRect(hWnd,&rect);
		x = rect.right - rect.left;
		y = rect.bottom - rect.top;
		internalx = screenx = primaryx = devmode.dmPelsWidth;
		internaly = screeny = primaryy = devmode.dmPelsHeight;
	}
	bpp = devmode.dmBitsPerPel;
	primarybpp = bpp;
	if(InitGL(x,y,bpp,fullscreen,hWnd)) return DD_OK;
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	DEBUG("IDirectDraw::SetDisplayMode: implement multiple monitors\n");
	DEVMODEA newmode;
	DEVMODEA currmode;
	float aspect,xmul,ymul;
	LONG error;
	DWORD flags;
	currmode.dmSize = sizeof(DEVMODEA);
	EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&currmode);
	switch(dxglcfg.scaler)
	{
	case 0: // No scaling, switch mode
		newmode.dmSize = sizeof(DEVMODEA);
		newmode.dmDriverExtra = 0;
		newmode.dmPelsWidth = dwWidth;
		newmode.dmPelsHeight = dwHeight;
		if(dxglcfg.colormode)
			newmode.dmBitsPerPel = dwBPP;
		else newmode.dmBitsPerPel = currmode.dmBitsPerPel;
		newmode.dmDisplayFrequency = dwRefreshRate;
		if(dwRefreshRate) newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		else newmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		flags = 0;
		if(fullscreen) flags |= CDS_FULLSCREEN;
		error = ChangeDisplaySettingsExA(NULL,&newmode,NULL,flags,NULL);
		switch(error)
		{
		case DISP_CHANGE_SUCCESSFUL:
			internalx = primaryx = screenx = newmode.dmPelsWidth;
			internaly = primaryy = screeny = newmode.dmPelsHeight;
			internalbpp = screenbpp = newmode.dmBitsPerPel;
			primarybpp = dwBPP;
			if(dwRefreshRate) internalrefresh = primaryrefresh = screenrefresh = dwRefreshRate;
			else internalrefresh = primaryrefresh = screenrefresh = currmode.dmDisplayFrequency;
			DeleteGL();
			InitGL(screenx,screeny,screenbpp,true,hWnd);
			return DD_OK;
		case DISP_CHANGE_BADMODE:
			return DDERR_INVALIDMODE;
		case DISP_CHANGE_BADFLAGS:
			return DDERR_INVALIDPARAMS;
		case DISP_CHANGE_BADDUALVIEW:
			return DDERR_GENERIC;
		case DISP_CHANGE_BADPARAM:
			return DDERR_INVALIDPARAMS;
		case DISP_CHANGE_FAILED:
			return DDERR_GENERIC;
		case DISP_CHANGE_NOTUPDATED:
			return DDERR_GENERIC;
		case DISP_CHANGE_RESTART:
			return DDERR_UNSUPPORTEDMODE;
		default:
			return DDERR_GENERIC;
		}
		return DDERR_GENERIC;
		break;
	case 1: // Stretch to screen
		primaryx = dwWidth;
		internalx = screenx = currmode.dmPelsWidth;
		primaryy = dwHeight;
		internaly = screeny = currmode.dmPelsHeight;
		if(dxglcfg.colormode) internalbpp = screenbpp = dwBPP;
		else internalbpp = screenbpp = currmode.dmBitsPerPel;
		primarybpp = dwBPP;
		DeleteGL();
		InitGL(screenx,screeny,screenbpp,true,hWnd);
		return DD_OK;
		break;
	case 2: // Scale to screen
		primaryx = dwWidth;
		screenx = currmode.dmPelsWidth;
		primaryy = dwHeight;
		screeny = currmode.dmPelsHeight;
		aspect = (float)dwWidth / (float)dwHeight;
		xmul = (float)screenx / (float)dwWidth;
		ymul = (float)screeny / (float)dwHeight;
		if((float)dwWidth*(float)ymul > (float)screenx)
		{
			internalx = (DWORD)((float)dwWidth * (float)xmul);
			internaly = (DWORD)((float)dwHeight * (float)xmul);
		}
		else
		{
			internalx = (DWORD)((float)dwWidth * (float)ymul);
			internaly = (DWORD)((float)dwHeight * (float)ymul);
		}
		if(dxglcfg.colormode) internalbpp = screenbpp = dwBPP;
		else internalbpp = screenbpp = currmode.dmBitsPerPel;
		primarybpp = dwBPP;
		DeleteGL();
		InitGL(screenx,screeny,screenbpp,true,hWnd);
		return DD_OK;
		break;
	}
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	FIXME("IDirectDraw::WaitForVerticalBlank: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	FIXME("IDirectDraw::GetAvailableVidMem: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	FIXME("IDirectDraw::GetSurfaceFromDC: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::RestoreAllSurfaces()
{
	FIXME("IDirectDraw::RestoreAllSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::TestCooperativeLevel()
{
	FIXME("IDirectDraw::TestCooperativeLevel: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags)
{
	FIXME("IDirectDraw::GetDeviceIdentifier: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags)
{
	FIXME("IDirectDraw::StartModeTest: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw7::EvaluateMode(DWORD dwFlags, DWORD *pSecondsUntilTimeout)
{
	FIXME("IDirectDraw::EvaluateMode: stub\n");
	return DDERR_GENERIC;
}

void glDirectDraw7::DeleteGL()
{
	if(hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(hRC);
	};
	if(hDC) ReleaseDC(hRenderWnd,hDC);
	if(hRenderWnd) DestroyWindow(hRenderWnd);
	hRC = NULL;
	hDC = NULL;
	hRenderWnd = NULL;
}

BOOL glDirectDraw7::InitGL(int width, int height, int bpp, bool fullscreen, HWND hWnd)
{
	if(hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(hRC);
	};
	if(hDC) ReleaseDC(hRenderWnd,hDC);
	if(hRenderWnd) DestroyWindow(hRenderWnd);
	WNDCLASSEXA wndclass;
	if(!wndclasscreated)
	{
		wndclass.cbSize = sizeof(WNDCLASSEXA);
		wndclass.style = 0;
		wndclass.lpfnWndProc = RenderWndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE);
		wndclass.hIcon = NULL;
		wndclass.hCursor = NULL;
		wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "DXGLRenderWindow";
		wndclass.hIconSm = NULL;
		if(!RegisterClassExA(&wndclass)) return DDERR_GENERIC;
		wndclasscreated = true;
	}
	GLuint pf;
	RECT rect;
	rect.left = 0;
	rect.right = width;
	rect.top = 0;
	rect.bottom = height;
	if(fullscreen)
	{
		SetWindowLongPtrA(hWnd,GWL_EXSTYLE,WS_EX_APPWINDOW);
		SetWindowLongPtrA(hWnd,GWL_STYLE,WS_POPUP);
		ShowWindow(hWnd,SW_MAXIMIZE);
	}
	if(width)
	{
		// TODO:  Adjust window rect
	}
	SetWindowPos(hWnd,NULL,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	RECT rectRender;
	GetClientRect(hWnd,&rectRender);
	hRenderWnd = CreateWindowA("DXGLRenderWindow","Renderer",WS_CHILD|WS_VISIBLE,0,0,rectRender.right - rectRender.left,
		rectRender.bottom - rectRender.top,hWnd,NULL,NULL,this);
	SetWindowPos(hRenderWnd,HWND_TOP,0,0,rectRender.right,rectRender.bottom,SWP_SHOWWINDOW);
	PIXELFORMATDESCRIPTOR pfd = 
	{sizeof(PIXELFORMATDESCRIPTOR),1,PFD_SUPPORT_OPENGL,bpp,
		0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,PFD_MAIN_PLANE,0,0,0,0};
	hDC = GetDC(hRenderWnd);
	if(!hDC)
	{
		DEBUG("glDirectDraw7::InitGL: Can not create hDC\n");
		return FALSE;
	}
	pf = ChoosePixelFormat(hDC,&pfd);
	if(!pf)
	{
		DEBUG("glDirectDraw7::InitGL: Can not get pixelformat\n");
		return FALSE;
	}
	if(!SetPixelFormat(hDC,pf,&pfd))
		DEBUG("glDirectDraw7::InitGL: Can not set pixelformat\n");
	gllock = true;
	hRC = wglCreateContext(hDC);
	if(!hRC)
	{
		DEBUG("glDirectDraw7::InitGL: Can not create GL context\n");
		gllock = false;
		return FALSE;
	}
	if(!wglMakeCurrent(hDC,hRC))
	{
		DEBUG("glDirectDraw7::InitGL: Can not activate GL context\n");
		wglDeleteContext(hRC);
		hRC = NULL;
		ReleaseDC(hRenderWnd,hDC);
		hDC = NULL;
		gllock = false;
		return FALSE;
	}
	gllock = false;
	glewInit();
	glViewport(0,0,width,height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,width,height,0,0,1);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	const GLubyte *glver = glGetString(GL_VERSION);
	gl_caps.Version = atof((char*)glver);
	if(gl_caps.Version >= 2)
	{
		glver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		gl_caps.ShaderVer = atof((char*)glver);
	}
	else gl_caps.ShaderVer = 0;
	if(GetBPP() <= 8)
	{
		GLuint palshader = glCreateShader(GL_FRAGMENT_SHADER);
		const GLchar *palsrc = frag_IndexedTexture;
		GLint palsrclen = strlen(frag_IndexedTexture);
		glShaderSource(palshader,1,&palsrc,&palsrclen);
		glCompileShader(palshader);
		palprog = glCreateProgram();
		glAttachShader(palprog,palshader);
		glLinkProgram(palprog);
	}
	if(GLEW_ARB_framebuffer_object)
	{
		glGenFramebuffers(1,&fbo);
		glGenRenderbuffers(1,&depthbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER,depthbuffer);
		//glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT,width,height);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER,0);
	}
	else if(GLEW_EXT_framebuffer_object)
	{
		glGenFramebuffersEXT(1,&fbo);
		glGenRenderbuffersEXT(1,&depthbuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,depthbuffer);
		//glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH_COMPONENT,width,height);
		//glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_RENDERBUFFER_EXT,depthbuffer);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT,0);
	}
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	SwapBuffers(hDC);
	return TRUE;
}
void glDirectDraw7::GetSizes(LONG *sizes) // allocate 6 dwords
{
	sizes[0] = internalx;
	sizes[1] = internaly;
	sizes[2] = primaryx;
	sizes[3] = primaryy;
	sizes[4] = screenx;
	sizes[5] = screeny;
}
LRESULT glDirectDraw7::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProcA(hWnd,msg,wParam,lParam);
}
void glDirectDraw7::GetHandles(HWND *hwnd, HWND *hrender)
{
	if(hwnd) *hwnd = hWnd;
	if(hrender) *hrender = hRenderWnd;
}
#pragma endregion
#pragma region DDRAW1 wrapper
glDirectDraw1::glDirectDraw1(glDirectDraw7 *gl_DD7)
{
	glDD7 = gl_DD7;
	glDD7->AddRef();
	refcount = 1;
}
glDirectDraw1::~glDirectDraw1()
{
	glDD7->Release();
}
HRESULT WINAPI glDirectDraw1::QueryInterface(REFIID riid, void** ppvObj)
{
	return glDD7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDraw1::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDraw1::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDraw1::Compact()
{
	return glDD7->Compact();
}
HRESULT WINAPI glDirectDraw1::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
}
HRESULT WINAPI glDirectDraw1::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
}
HRESULT WINAPI glDirectDraw1::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	LPDIRECTDRAWSURFACE7 lpDDS7;
	HRESULT err = glDD7->CreateSurface((LPDDSURFACEDESC2)lpDDSurfaceDesc,&lpDDS7,pUnkOuter);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		return DD_OK;
	}
	return err;
}
HRESULT WINAPI glDirectDraw1::DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface)
{
	FIXME("glDirectDraw1::DuplicateSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw1::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw1::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw1::EnumSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw1::FlipToGDISurface()
{
	return glDD7->FlipToGDISurface();
}
HRESULT WINAPI glDirectDraw1::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	return glDD7->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}
HRESULT WINAPI glDirectDraw1::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc2)
{
	FIXME("glDirectDraw1::GetDisplayMode: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw1::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw1::GetGDISurface(LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw1::GetGDISurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw1::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	return glDD7->GetMonitorFrequency(lpdwFrequency);
}
HRESULT WINAPI glDirectDraw1::GetScanLine(LPDWORD lpdwScanLine)
{
	return glDD7->GetScanLine(lpdwScanLine);
}
HRESULT WINAPI glDirectDraw1::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	return glDD7->GetVerticalBlankStatus(lpbIsInVB);
}
HRESULT WINAPI glDirectDraw1::Initialize(GUID FAR *lpGUID)
{
	FIXME("IDirectDraw1::Initialize: stub\n");
	return DDERR_DIRECTDRAWALREADYCREATED;
}
HRESULT WINAPI glDirectDraw1::RestoreDisplayMode()
{
	return glDD7->RestoreDisplayMode();
}
HRESULT WINAPI glDirectDraw1::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	return glDD7->SetCooperativeLevel(hWnd,dwFlags);
}
HRESULT WINAPI glDirectDraw1::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	return glDD7->SetDisplayMode(dwWidth,dwHeight,dwBPP,0,0);
}
HRESULT WINAPI glDirectDraw1::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	return glDD7->WaitForVerticalBlank(dwFlags,hEvent);
}
#pragma endregion
#pragma region DDRAW2 wrapper
glDirectDraw2::glDirectDraw2(glDirectDraw7 *gl_DD7)
{
	glDD7 = gl_DD7;
	glDD7->AddRef();
	refcount = 1;
}
glDirectDraw2::~glDirectDraw2()
{
	glDD7->Release();
}
HRESULT WINAPI glDirectDraw2::QueryInterface(REFIID riid, void** ppvObj)
{
	return glDD7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDraw2::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDraw2::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDraw2::Compact()
{
	return glDD7->Compact();
}
HRESULT WINAPI glDirectDraw2::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
}
HRESULT WINAPI glDirectDraw2::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
}
HRESULT WINAPI glDirectDraw2::CreateSurface(LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	LPDIRECTDRAWSURFACE7 lpDDS7;
	HRESULT err = glDD7->CreateSurface((LPDDSURFACEDESC2)lpDDSurfaceDesc,&lpDDS7,pUnkOuter);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		return DD_OK;
	}
	return err;
}
HRESULT WINAPI glDirectDraw2::DuplicateSurface(LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface)
{
	FIXME("glDirectDraw2::DuplicateSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw2::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw2::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw2::EnumSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw2::FlipToGDISurface()
{
	return glDD7->FlipToGDISurface();
}
HRESULT WINAPI glDirectDraw2::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	return glDD7->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}
HRESULT WINAPI glDirectDraw2::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc2)
{
	FIXME("glDirectDraw2::GetDisplayMode: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw2::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw2::GetGDISurface(LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw2::GetGDISurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw2::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	return glDD7->GetMonitorFrequency(lpdwFrequency);
}
HRESULT WINAPI glDirectDraw2::GetScanLine(LPDWORD lpdwScanLine)
{
	return glDD7->GetScanLine(lpdwScanLine);
}
HRESULT WINAPI glDirectDraw2::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	return glDD7->GetVerticalBlankStatus(lpbIsInVB);
}
HRESULT WINAPI glDirectDraw2::Initialize(GUID FAR *lpGUID)
{
	FIXME("IDirectDraw2::Initialize: stub\n");
	return DDERR_DIRECTDRAWALREADYCREATED;
}
HRESULT WINAPI glDirectDraw2::RestoreDisplayMode()
{
	return glDD7->RestoreDisplayMode();
}
HRESULT WINAPI glDirectDraw2::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	return glDD7->SetCooperativeLevel(hWnd,dwFlags);
}
HRESULT WINAPI glDirectDraw2::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	return glDD7->SetDisplayMode(dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
}
HRESULT WINAPI glDirectDraw2::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	return glDD7->WaitForVerticalBlank(dwFlags,hEvent);
}
HRESULT WINAPI glDirectDraw2::GetAvailableVidMem(LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	FIXME("glDirectDraw2::GetAvailableVidMem: stub\n");
	return DDERR_GENERIC;
}
#pragma endregion
#pragma region DDRAW4 wrapper
glDirectDraw4::glDirectDraw4(glDirectDraw7 *gl_DD7)
{
	glDD7 = gl_DD7;
	refcount = 1;
}
glDirectDraw4::~glDirectDraw4()
{
	glDD7->Release();
}
HRESULT WINAPI glDirectDraw4::QueryInterface(REFIID riid, void** ppvObj)
{
	return glDD7->QueryInterface(riid,ppvObj);
}
ULONG WINAPI glDirectDraw4::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirectDraw4::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}
HRESULT WINAPI glDirectDraw4::Compact()
{
	return glDD7->Compact();
}
HRESULT WINAPI glDirectDraw4::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreateClipper(dwFlags,lplpDDClipper,pUnkOuter);
}
HRESULT WINAPI glDirectDraw4::CreatePalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter)
{
	return glDD7->CreatePalette(dwFlags,lpDDColorArray,lplpDDPalette,pUnkOuter);
}
HRESULT WINAPI glDirectDraw4::CreateSurface(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE4 FAR *lplpDDSurface, IUnknown FAR *pUnkOuter)
{
	FIXME("glDirectDraw4::CreateSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::DuplicateSurface(LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface)
{
	FIXME("glDirectDraw4::DuplicateSurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	FIXME("glDirectDraw4::EnumDisplayModes: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw4::EnumSurfaces: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::FlipToGDISurface()
{
	return glDD7->FlipToGDISurface();
}
HRESULT WINAPI glDirectDraw4::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	return glDD7->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}
HRESULT WINAPI glDirectDraw4::GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	FIXME("glDirectDraw4::GetDisplayMode: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw4::GetGDISurface(LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw4::GetGDISurface: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	return glDD7->GetMonitorFrequency(lpdwFrequency);
}
HRESULT WINAPI glDirectDraw4::GetScanLine(LPDWORD lpdwScanLine)
{
	return glDD7->GetScanLine(lpdwScanLine);
}
HRESULT WINAPI glDirectDraw4::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	return glDD7->GetVerticalBlankStatus(lpbIsInVB);
}
HRESULT WINAPI glDirectDraw4::Initialize(GUID FAR *lpGUID)
{
	FIXME("IDirectDraw4::Initialize: stub\n");
	return DDERR_DIRECTDRAWALREADYCREATED;
}
HRESULT WINAPI glDirectDraw4::RestoreDisplayMode()
{
	return glDD7->RestoreDisplayMode();
}
HRESULT WINAPI glDirectDraw4::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	return glDD7->SetCooperativeLevel(hWnd,dwFlags);
}
HRESULT WINAPI glDirectDraw4::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	return glDD7->SetDisplayMode(dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
}
HRESULT WINAPI glDirectDraw4::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	return glDD7->WaitForVerticalBlank(dwFlags,hEvent);
}
HRESULT WINAPI glDirectDraw4::GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	FIXME("IDirectDraw4::GetAvailableVidMem: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS)
{
	FIXME("IDirectDraw4::GetSurfaceFromDC: stub\n");
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirectDraw4::RestoreAllSurfaces()
{
	return glDD7->RestoreAllSurfaces();
}
HRESULT WINAPI glDirectDraw4::TestCooperativeLevel()
{
	return glDD7->TestCooperativeLevel();
}
HRESULT WINAPI glDirectDraw4::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER lpdddi, DWORD dwFlags)
{
	FIXME("IDirectDraw4::GetDeviceIdentifier: stub\n");
	return DDERR_GENERIC;
}
#pragma endregion

// Render Window event handler
static LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	glDirectDraw7* instance = reinterpret_cast<glDirectDraw7*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
	if(!instance)
	{
		if(msg == WM_CREATE)
			instance = reinterpret_cast<glDirectDraw7*>(*(LONG_PTR*)lParam);
		else return DefWindowProc(hwnd,msg,wParam,lParam);
	}
	return instance->WndProc(hwnd,msg,wParam,lParam);
}
