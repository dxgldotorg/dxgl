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

#include "common.h"
#include "shaders.h"
#include "ddraw.h"
#include "glDirect3D.h"
#include "glDirectDraw.h"
#include "glDirectDrawClipper.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glutil.h"
#include "../common/version.h"

bool directdraw_created = false; // emulate only one ddraw device
bool wndclasscreated = false;

DDDEVICEIDENTIFIER2 devid = {
	"ddraw.dll",
	"DXGL DDraw Wrapper",
	0,
	0,0,0,0,
	0,0};

void DiscardDuplicateModes(DEVMODE **array, DWORD *count)
{
	DEVMODE *newarray = (DEVMODE *)malloc(sizeof(DEVMODE)*(*count));
	DWORD newcount = 0;
	bool match;
	for(DWORD x = 0; x < (*count); x++)
	{
		match = false;
		memcpy(&newarray[newcount],&(*array)[x],sizeof(DEVMODE));
		for(int y = newcount; y > 0; y--)
		{
			if((*array)[x].dmBitsPerPel == newarray[y-1].dmBitsPerPel &&
				(*array)[x].dmDisplayFrequency == newarray[y-1].dmDisplayFrequency &&
				(*array)[x].dmPelsWidth == newarray[y-1].dmPelsWidth &&
				(*array)[x].dmPelsHeight == newarray[y-1].dmPelsHeight)
			{
				match = true;
				break;
			}
		}
		if(!match) newcount++;
	}
	realloc(newarray,sizeof(DEVMODE)*newcount);
	free(*array);
	*array = newarray;
	*count = newcount;
}

bool ScanModeList(DEVMODE *array, DEVMODE comp, DWORD count)
{
	for(DWORD i = 0; i < count; i++)
	{
		if(!memcmp(&array[i],&comp,sizeof(DEVMODE))) return true;
	}
	return false;
}

const int ExtraModes[30] [3] = {
	{320,175,70},
	{320,200,70},
	{320,240,60},
	{320,400,70},
	{320,480,60},
	{360,200,70},
	{360,240,60},
	{360,400,70},
	{360,480,60},
	{400,300,60},
	{416,312,75},
	{512,384,60},
	{576,432,60},
	{640,350,70},
	{640,400,70},
	{640,512,60},
	{680,384,60},
	{700,525,60},
	{720,350,70},
	{720,400,70},
	{720,450,60},
	{720,480,60},
	{800,512,60},
	{832,624,75},
	{840,525,60},
	{896,672,60},
	{928,696,60},
	{960,540,60},
	{960,600,60},
	{960,720,60}
};

bool ScanColorMode(DEVMODE *array, DWORD count, int bpp)
{
	for(DWORD i = 0; i < count; i++)
	{
		if(array[i].dmBitsPerPel == bpp) return true;
	}
	return false;
}

bool ScanModeListNoRefresh(DEVMODE *array, DEVMODE comp, DWORD count)
{
	for(DWORD i = 0; i < count; i++)
	{
		if((array[i].dmBitsPerPel == comp.dmBitsPerPel) &&
			(array[i].dmPelsWidth == comp.dmPelsWidth) &&
			(array[i].dmPelsHeight == comp.dmPelsHeight)) return true;
	}
	return false;
} 

void AddExtraResolutions(DEVMODE **array, DWORD *count)
{
	DEVMODE *array2 = (DEVMODE *)malloc(sizeof(DEVMODE)*5*30);
	DEVMODE compmode = *array[0];
	DWORD newcount = 0;
	int i;
	if(ScanColorMode(*array,*count,8))
	{
		compmode.dmBitsPerPel = 8;
		for(i = 0; i < 30; i++)
		{
			compmode.dmPelsWidth = ExtraModes[i][0];
			compmode.dmPelsHeight = ExtraModes[i][1];
			compmode.dmDisplayFrequency = ExtraModes[i][2];
			if(!ScanModeListNoRefresh(*array,compmode,*count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if(ScanColorMode(*array,*count,15))
	{
		compmode.dmBitsPerPel = 15;
		for(i = 0; i < 30; i++)
		{
			compmode.dmPelsWidth = ExtraModes[i][0];
			compmode.dmPelsHeight = ExtraModes[i][1];
			compmode.dmDisplayFrequency = ExtraModes[i][2];
			if(!ScanModeListNoRefresh(*array,compmode,*count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if(ScanColorMode(*array,*count,16))
	{
		compmode.dmBitsPerPel = 16;
		for(i = 0; i < 30; i++)
		{
			compmode.dmPelsWidth = ExtraModes[i][0];
			compmode.dmPelsHeight = ExtraModes[i][1];
			compmode.dmDisplayFrequency = ExtraModes[i][2];
			if(!ScanModeListNoRefresh(*array,compmode,*count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if(ScanColorMode(*array,*count,24))
	{
		compmode.dmBitsPerPel = 24;
		for(i = 0; i < 30; i++)
		{
			compmode.dmPelsWidth = ExtraModes[i][0];
			compmode.dmPelsHeight = ExtraModes[i][1];
			compmode.dmDisplayFrequency = ExtraModes[i][2];
			if(!ScanModeListNoRefresh(*array,compmode,*count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	if(ScanColorMode(*array,*count,32))
	{
		compmode.dmBitsPerPel = 32;
		for(i = 0; i < 30; i++)
		{
			compmode.dmPelsWidth = ExtraModes[i][0];
			compmode.dmPelsHeight = ExtraModes[i][1];
			compmode.dmDisplayFrequency = ExtraModes[i][2];
			if(!ScanModeListNoRefresh(*array,compmode,*count))
			{
				array2[newcount] = compmode;
				newcount++;
			}
		}
	}
	*array = (DEVMODE *)realloc(*array,(*count+newcount)*sizeof(DEVMODE));
	memcpy(&(*array)[*count-1],array2,newcount*sizeof(DEVMODE));
	free(array2);
	*count += newcount;
}

void AddExtraColorModes(DEVMODE **array, DWORD *count)
{
	DEVMODE *array2 = (DEVMODE *)malloc(sizeof(DEVMODE)*(7*(*count)));
	DEVMODE compmode;
	DWORD count2 = 0;
	for(DWORD i = 0; i < *count; i++)
	{
		switch((*array)[i].dmBitsPerPel)
		{
		case 15:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 16;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 16:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 15;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 24:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 32;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		case 32:
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 24;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 16;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 15;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			compmode = (*array)[i];
			compmode.dmBitsPerPel = 8;
			if(!ScanModeList(*array,compmode,*count))
			{
				array2[count2] = compmode;
				count2++;
			}
			break;
		default:
			break;
		}
	}
	*array = (DEVMODE *)realloc(*array,(*count+count2)*sizeof(DEVMODE));
	memcpy(&(*array)[*count-1],array2,count2*sizeof(DEVMODE));
	free(array2);
	*count += count2;
}

int SortDepth(const DEVMODE *mode1, const DEVMODE *mode2)
{
	unsigned __int64 mode1num = 
		(mode1->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode1->dmPelsWidth & 0xFFFF)<< 16) |
		((unsigned __int64)(mode1->dmPelsHeight & 0xFFFF)<< 32) |
		((unsigned __int64)(mode1->dmBitsPerPel & 0xFFFF)<< 48);
	unsigned __int64 mode2num = 
		(mode2->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode2->dmPelsWidth & 0xFFFF)<< 16) |
		((unsigned __int64)(mode2->dmPelsHeight & 0xFFFF)<< 32) |
		((unsigned __int64)(mode2->dmBitsPerPel & 0xFFFF)<< 48);
	if(mode2num > mode1num) return -1;
	else if(mode2num < mode1num) return  1;
	else return 0;
}
int SortRes(const DEVMODE *mode1, const DEVMODE *mode2)
{
	unsigned __int64 mode1num = 
		(mode1->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode1->dmPelsWidth & 0xFFFF)<< 32) |
		((unsigned __int64)(mode1->dmPelsHeight & 0xFFFF)<< 48) |
		((unsigned __int64)(mode1->dmBitsPerPel & 0xFFFF)<< 16);
	unsigned __int64 mode2num = 
		(mode2->dmDisplayFrequency & 0xFFFF) |
		((unsigned __int64)(mode2->dmPelsWidth & 0xFFFF)<< 32) |
		((unsigned __int64)(mode2->dmPelsHeight & 0xFFFF)<< 48) |
		((unsigned __int64)(mode2->dmBitsPerPel & 0xFFFF)<< 16);
	if(mode2num > mode1num) return -1;
	else if(mode2num < mode1num) return  1;
	else return 0;
}

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
	if(!modes) ERR(DDERR_OUTOFMEMORY);
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
				ERR(DDERR_OUTOFMEMORY);
			}
			modes = tmp;
		}
	}
	DiscardDuplicateModes(&modes,&modenum);
	if(dxglcfg.AllColorDepths) AddExtraColorModes(&modes,&modenum);
	DiscardDuplicateModes(&modes,&modenum);
	if(dxglcfg.ExtraModes && (dxglcfg.scaler != 0)) AddExtraResolutions(&modes,&modenum);
	modenum--;
	switch(dxglcfg.SortModes)
	{
	case 0:
	default:
		break;
	case 1:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortDepth);
		break;
	case 2:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
		break;
	}
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
HRESULT EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	bool match;
	DWORD modenum = 0;
	DWORD modemax = 128;
	DEVMODE mode;
	ZeroMemory(&mode,sizeof(DEVMODE));
	mode.dmSize = sizeof(DEVMODE);
	DEVMODE *modes = (DEVMODE*)malloc(128*sizeof(DEVMODE));
	DEVMODE *tmp;
	if(!modes) ERR(DDERR_OUTOFMEMORY);
	DDSURFACEDESC2 ddmode;
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
				ERR(DDERR_OUTOFMEMORY);
			}
			modes = tmp;
		}
	}
	DiscardDuplicateModes(&modes,&modenum);
	if(dxglcfg.AllColorDepths) AddExtraColorModes(&modes,&modenum);
	if(dxglcfg.ExtraModes && (dxglcfg.scaler != 0)) AddExtraResolutions(&modes,&modenum);
	modenum--;
	switch(dxglcfg.SortModes)
	{
	case 0:
	default:
		break;
	case 1:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortDepth);
		break;
	case 2:
		qsort(modes,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
		break;
	}
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

// DDRAW7/common routines

glDirectDraw7::glDirectDraw7()
{
	initialized = false;
	devid.liDriverVersion.QuadPart = DXGLVERQWORD;
	refcount = 1;
}

glDirectDraw7::glDirectDraw7(GUID FAR* lpGUID, IUnknown FAR* pUnkOuter)
{
	initialized = false;
	if(pUnkOuter)
	{
		error = DDERR_INVALIDPARAMS ;
		return;
	}
	devid.liDriverVersion.QuadPart = DXGLVERQWORD;
	error = glDirectDraw7::Initialize(lpGUID);
	refcount = 1;
}

glDirectDraw7::~glDirectDraw7()
{
	if(glD3D7) glD3D7->Release();
	RestoreDisplayMode();
	if(clippers)
	{
		for(int i = 0; i < clippercount; i++)
			clippers[i]->Release();
		free(clippers);
	}
	if(surfaces)
	{

		free(surfaces);
	}
	DeleteGL();
	ddenabled = false;
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
		ERR(DDERR_GENERIC);
	}
	if(riid == IID_IDirect3D2)
	{
		FIXME("Add IDirect3D Interfaces\n");
		ERR(DDERR_GENERIC);
	}
	if(riid == IID_IDirect3D3)
	{
		FIXME("Add IDirect3D Interfaces\n");
		ERR(DDERR_GENERIC);
	}
	if(riid == IID_IDirect3D7)
	{
		#ifdef _DEBUG
		this->AddRef();
		*ppvObj = new glDirect3D7(this);
		return DD_OK;
		#else
		FIXME("Add IDirect3D Interfaces\n");
		ERR(DDERR_GENERIC);
		#endif
	}
	if(riid == IID_IDirectDrawGammaControl)
	{
		FIXME("Add IDirect3D Interfaces\n");
		ERR(DDERR_GENERIC);
	}
	/*if(riid == IID_IDDVideoPortContainer)
	{
		ERR(DDERR_GENERIC);
	}*/
	ERR(E_NOINTERFACE);
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
			ERR(DDERR_OUTOFMEMORY);
		clippers = clippers2;
		ZeroMemory(&clippers[clippercountmax],1024*sizeof(glDirectDrawClipper *));
		clippercountmax += 1024;
	}
	clippers[clippercount-1] = new glDirectDrawClipper(dwFlags,this);
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
	if(primary && (lpDDSurfaceDesc2->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) && (hRC == primary->hrc) )
		ERR(DDERR_PRIMARYSURFACEALREADYEXISTS);
	surfacecount++;
	if(surfacecount > surfacecountmax)
	{
		glDirectDrawSurface7 **surfaces2;
		surfaces2 = (glDirectDrawSurface7 **)realloc(surfaces,(surfacecountmax+1024)*sizeof(glDirectDrawSurface7 *));
		if(!surfaces2)
			ERR(DDERR_OUTOFMEMORY);
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
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc2, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc2,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw7::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD2, LPVOID lpContext, LPDDENUMSURFACESCALLBACK7 lpEnumSurfacesCallback)
{
	FIXME("IDirectDraw::EnumSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::FlipToGDISurface()
{
	HRESULT error = DD_OK;
	if(primary)
	{
		if(primary->flipcount)
		{
			while(primary->flipcount != 0)
			{
				error = primary->Flip(NULL,DDFLIP_WAIT);
				if(error != DD_OK) break;
			}
		}
		return(error);
	}
	else ERR(DDERR_NOTFOUND);
}
HRESULT WINAPI glDirectDraw7::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	//TODO:  Fill in as implemented.
	DDCAPS_DX7 ddCaps;
	ZeroMemory(&ddCaps,sizeof(DDCAPS_DX7));
	if(lpDDDriverCaps) ddCaps.dwSize = lpDDDriverCaps->dwSize;
	else if(lpDDHELCaps) ddCaps.dwSize = lpDDHELCaps->dwSize;
	else ERR(DDERR_INVALIDPARAMS);
	ddCaps.dwCaps = DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH |
		DDCAPS_GDI | DDCAPS_PALETTE | DDCAPS_CANBLTSYSMEM;
	ddCaps.dwCaps2 = DDCAPS2_CANRENDERWINDOWED | DDCAPS2_WIDESURFACES | DDCAPS2_NOPAGELOCKREQUIRED |
		DDCAPS2_FLIPINTERVAL | DDCAPS2_FLIPNOVSYNC;
	ddCaps.dwFXCaps = DDFXCAPS_BLTSHRINKX | DDFXCAPS_BLTSHRINKY |
		DDFXCAPS_BLTSTRETCHX | DDFXCAPS_BLTSTRETCHY;
	ddCaps.dwPalCaps = DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE;
	ddCaps.ddsOldCaps.dwCaps = ddCaps.ddsCaps.dwCaps =
		DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDCAPS_COLORKEY | DDSCAPS_FLIP |
		DDSCAPS_FRONTBUFFER | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE |
		DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY;
	ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT;
	if(lpDDDriverCaps) memcpy(lpDDDriverCaps,&ddCaps,lpDDDriverCaps->dwSize);
	if(lpDDHELCaps) memcpy(lpDDHELCaps,&ddCaps,lpDDHELCaps->dwSize);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetDisplayMode(LPDDSURFACEDESC2 lpDDSurfaceDesc2)
{
	if(!lpDDSurfaceDesc2) ERR(DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsdMode;
	ZeroMemory(&ddsdMode, sizeof(DDSURFACEDESC2));
	ddsdMode.dwSize = sizeof(DDSURFACEDESC2);
	DEVMODE currmode;
	if(fullscreen)
	{
		if(primarybpp == 8)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0;
			ddsdMode.ddpfPixelFormat.dwBBitMask = 0;
		}
		else if(primarybpp == 15)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0x7C00;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0x3E0;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0x1F;
		}
		else if(primarybpp == 16)
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xF800;
			ddsdMode.ddpfPixelFormat.dwGBitMask = 0x7E0;
			ddsdMode.ddpfPixelFormat.dwBBitMask = 0x1F;
		}
		else
		{
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF00;
			ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF;
		}
		ddsdMode.ddpfPixelFormat.dwRGBBitCount = GetBPPMultipleOf8();
		ddsdMode.dwWidth = primaryx;
		ddsdMode.dwHeight = primaryy;
		if(primarybpp == 15) ddsdMode.lPitch = primaryx * 2;
			else if(primarybpp == 4) ddsdMode.lPitch = primaryx / 2;
			else ddsdMode.lPitch = primaryx * (primarybpp / 8);
		if(lpDDSurfaceDesc2->dwSize < sizeof(DDSURFACEDESC)) ERR(DDERR_INVALIDPARAMS);
		if(lpDDSurfaceDesc2->dwSize > sizeof(DDSURFACEDESC2))
			lpDDSurfaceDesc2->dwSize = sizeof(DDSURFACEDESC2);
		memcpy(lpDDSurfaceDesc2,&ddsdMode,lpDDSurfaceDesc2->dwSize);
		return DD_OK;
	}
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&currmode);
	if(currmode.dmBitsPerPel == 8) ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
	else if(currmode.dmBitsPerPel == 4) ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED4;
	else ddsdMode.ddpfPixelFormat.dwFlags = DDPF_RGB;
	if(currmode.dmBitsPerPel == 8)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0;
		ddsdMode.ddpfPixelFormat.dwBBitMask = 0;
	}
	else if(currmode.dmBitsPerPel == 15)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0x7C00;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0x3E0;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0x1F;
	}
	else if(currmode.dmBitsPerPel == 16)
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xF800;
		ddsdMode.ddpfPixelFormat.dwGBitMask = 0x7E0;
		ddsdMode.ddpfPixelFormat.dwBBitMask = 0x1F;
	}
	else
	{
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF0000;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF00;
		ddsdMode.ddpfPixelFormat.dwRBitMask = 0xFF;
	}
	ddsdMode.ddpfPixelFormat.dwRGBBitCount = currmode.dmBitsPerPel;
	ddsdMode.dwWidth = currmode.dmPelsWidth;
	ddsdMode.dwHeight = currmode.dmPelsHeight;
	if(currmode.dmBitsPerPel == 15) ddsdMode.lPitch = currmode.dmPelsWidth * 2;
		else if(currmode.dmBitsPerPel == 4) ddsdMode.lPitch = currmode.dmPelsWidth / 2;
		else ddsdMode.lPitch = currmode.dmPelsWidth * (currmode.dmBitsPerPel / 8);
	if(lpDDSurfaceDesc2->dwSize < sizeof(DDSURFACEDESC)) ERR(DDERR_INVALIDPARAMS);
	if(lpDDSurfaceDesc2->dwSize > sizeof(DDSURFACEDESC2))
		lpDDSurfaceDesc2->dwSize = sizeof(DDSURFACEDESC2);
	memcpy(lpDDSurfaceDesc2,&ddsdMode,lpDDSurfaceDesc2->dwSize);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	FIXME("IDirectDraw::GetFourCCCodes: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::GetGDISurface(LPDIRECTDRAWSURFACE7 FAR *lplpGDIDDSurface)
{
	FIXME("IDirectDraw::GetGDISurface: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::GetMonitorFrequency(LPDWORD lpdwFrequency)
{
	DEBUG("IDirectDraw::GetMonitorFrequency: support multi-monitor\n");
	DEVMODE devmode;
	devmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
	*lpdwFrequency = devmode.dmDisplayFrequency;
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetScanLine(LPDWORD lpdwScanLine)
{
	FIXME("IDirectDraw::GetScanLine: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::GetVerticalBlankStatus(LPBOOL lpbIsInVB)
{
	FIXME("IDirectDraw::GetVerticalBlankStatis: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::Initialize(GUID FAR *lpGUID)
{
	if(initialized) return DDERR_ALREADYINITIALIZED;
	hDC = NULL;
	hRC = NULL;
	PBO = 0;
	hasHWnd = false;
	dib.enabled = false;
	glD3D7 = NULL;
	hRenderWnd = NULL;
	primary = NULL;
	fullscreen = false;
	fpupreserve = false;
	fpusetup = false;
	threadsafe = false;
	nowindowchanges = false;
	timer = timeGetTime();
	ZeroMemory(&oldmode,sizeof(DEVMODE));
	surfaces = (glDirectDrawSurface7 **)malloc(1024*sizeof(glDirectDrawSurface7 *));
	if(!surfaces) return DDERR_OUTOFMEMORY;
	ZeroMemory(surfaces,1024*sizeof(glDirectDrawSurface7 *));
	surfacecount = 0;
	surfacecountmax = 1024;
	clippers = (glDirectDrawClipper **)malloc(1024*sizeof(glDirectDrawClipper *));
	if(!clippers) return DDERR_OUTOFMEMORY;
	ZeroMemory(clippers,1024*sizeof(glDirectDrawClipper *));
	clippercount = 0;
	clippercountmax = 1024;
	if(directdraw_created) error = DDERR_DIRECTDRAWALREADYCREATED;
	bool useguid = false;
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
	initialized = true;
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::RestoreDisplayMode()
{
	if(oldmode.dmSize != 0)
	{
		ChangeDisplaySettingsEx(NULL,&oldmode,NULL,0,NULL);
	}
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::SetCooperativeLevel(HWND hWnd, DWORD dwFlags)
{
	this->hWnd = hWnd;
	if(hRC) DeleteGL();
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
		if(!fullscreen) ERR(DDERR_INVALIDPARAMS);
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
		if(exclusive) ERR(DDERR_INVALIDPARAMS);
	if(dwFlags & DDSCL_NOWINDOWCHANGES)
		nowindowchanges = true;
	else nowindowchanges = false;
	if(dwFlags & DDSCL_SETDEVICEWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETDEVICEWINDOW unsupported\n");
	if(dwFlags & DDSCL_SETFOCUSWINDOW)
		FIXME("IDirectDraw::SetCooperativeLevel: DDSCL_SETFOCUSWINDOW unsupported\n");
	DEVMODE devmode;
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&devmode);
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
	ERR(DDERR_GENERIC);
}

void DiscardUndersizedModes(DEVMODE **array, DWORD *count, DEVMODE comp)
{
	DEVMODE *newarray = (DEVMODE *)malloc(sizeof(DEVMODE)*(*count));
	DWORD newcount = 0;
	bool match;
	for(DWORD x = 0; x < (*count); x++)
	{
		match = false;
		memcpy(&newarray[newcount],&(*array)[x],sizeof(DEVMODE));
		for(int y = newcount; y > 0; y--)
		{
			if((*array)[x].dmBitsPerPel != comp.dmBitsPerPel ||
				(*array)[x].dmPelsWidth < comp.dmPelsWidth ||
				(*array)[x].dmPelsHeight < comp.dmPelsHeight)
			{
				match = true;
				break;
			}
		}
		if(!match) newcount++;
	}
	realloc(newarray,sizeof(DEVMODE)*newcount);
	free(*array);
	*array = newarray;
	*count = newcount;
}


DEVMODE FindClosestMode(const DEVMODE in)
{
	DEVMODE newmode;
	DEVMODE *candidates = (DEVMODE *)malloc(128*sizeof(DEVMODE));
	DEVMODE mode;
	DEVMODE *tmp = NULL;
	DWORD modenum = 0;
	DWORD modemax = 128;
	while(EnumDisplaySettings(NULL,modenum++,&mode))
	{
		candidates[modenum-1] = mode;
		if(modenum >= modemax)
		{
			modemax += 128;
			tmp = (DEVMODE*)realloc(candidates,modemax*sizeof(DEVMODE));
			if(tmp == NULL)
			{
				free(candidates);
				return in;
			}
			candidates = tmp;
		}
	}
	DiscardDuplicateModes(&candidates,&modenum);
	DiscardUndersizedModes(&candidates,&modenum,in);
	qsort(candidates,modenum,sizeof(DEVMODE),(int(*)(const void*, const void*))SortRes);
	newmode = candidates[0];
	newmode.dmFields = DM_BITSPERPEL| DM_PELSWIDTH | DM_PELSHEIGHT;
	free(candidates);
	return newmode;
}

HRESULT WINAPI glDirectDraw7::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
	DEBUG("IDirectDraw::SetDisplayMode: implement multiple monitors\n");
	DEVMODE newmode,newmode2;
	DEVMODE currmode;
	float aspect,xmul,ymul;
	LONG error;
	DWORD flags;
	if(!oldmode.dmSize)
	{
		oldmode.dmSize = sizeof(DEVMODE);
		EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&oldmode);
	}
	currmode.dmSize = sizeof(DEVMODE);
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&currmode);
	switch(dxglcfg.scaler)
	{
	case 0: // No scaling, switch mode
	default:
		newmode.dmSize = sizeof(DEVMODE);
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
		error = ChangeDisplaySettingsEx(NULL,&newmode,NULL,flags,NULL);
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
			ERR(DDERR_INVALIDMODE);
		case DISP_CHANGE_BADFLAGS:
			ERR(DDERR_INVALIDPARAMS);
		case DISP_CHANGE_BADDUALVIEW:
			ERR(DDERR_GENERIC);
		case DISP_CHANGE_BADPARAM:
			ERR(DDERR_INVALIDPARAMS);
		case DISP_CHANGE_FAILED:
			ERR(DDERR_GENERIC);
		case DISP_CHANGE_NOTUPDATED:
			ERR(DDERR_GENERIC);
		case DISP_CHANGE_RESTART:
			ERR(DDERR_UNSUPPORTEDMODE);
		default:
			ERR(DDERR_GENERIC);
		}
		ERR(DDERR_GENERIC);
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
	case 3: // Center image
		primaryx = internalx = dwWidth;
		screenx = currmode.dmPelsWidth;
		primaryy = internaly = dwHeight;
		screeny = currmode.dmPelsHeight;
		primarybpp = dwBPP;
		if(dxglcfg.colormode) internalbpp = screenbpp = dwBPP;
		else internalbpp = screenbpp = currmode.dmBitsPerPel;
		DeleteGL();
		InitGL(screenx,screeny,screenbpp,true,hWnd);
		break;
	case 4: // Switch then stretch
	case 5: // Switch then scale
	case 6: // Switch then center
		newmode.dmSize = sizeof(DEVMODE);
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
		error = ChangeDisplaySettingsEx(NULL,&newmode,NULL,flags,NULL);
		if(error != DISP_CHANGE_SUCCESSFUL)
		{
			newmode2 = FindClosestMode(newmode);
			error = ChangeDisplaySettingsEx(NULL,&newmode2,NULL,flags,NULL);
		}
		else newmode2 = newmode;
		switch(dxglcfg.scaler)
		{
		case 4:
			primaryx = dwWidth;
			internalx = screenx = newmode2.dmPelsWidth;
			primaryy = dwHeight;
			internaly = screeny = newmode2.dmPelsHeight;
			if(dxglcfg.colormode) internalbpp = screenbpp = dwBPP;
			else internalbpp = screenbpp = newmode2.dmBitsPerPel;
			primarybpp = dwBPP;
			DeleteGL();
			InitGL(screenx,screeny,screenbpp,true,hWnd);
			return DD_OK;
			break;
		case 5:
			primaryx = dwWidth;
			screenx = newmode2.dmPelsWidth;
			primaryy = dwHeight;
			screeny = newmode2.dmPelsHeight;
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
			else internalbpp = screenbpp = newmode2.dmBitsPerPel;
			primarybpp = dwBPP;
			DeleteGL();
			InitGL(screenx,screeny,screenbpp,true,hWnd);
			return DD_OK;
			break;
		case 6:
		default:
			primaryx = internalx = dwWidth;
			screenx = newmode2.dmPelsWidth;
			primaryy = internaly = dwHeight;
			screeny = newmode2.dmPelsHeight;
			primarybpp = dwBPP;
			if(dxglcfg.colormode) internalbpp = screenbpp = dwBPP;
			else internalbpp = screenbpp = newmode2.dmBitsPerPel;
			DeleteGL();
			InitGL(screenx,screeny,screenbpp,true,hWnd);
			break;
		}
		break;
	}
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::WaitForVerticalBlank(DWORD dwFlags, HANDLE hEvent)
{
	if(dwFlags & DDWAITVB_BLOCKBEGINEVENT) return DDERR_UNSUPPORTED;
	SetSwap(1);
	primary->RenderScreen(primary->texture,primary);
	SetSwap(0);
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::GetAvailableVidMem(LPDDSCAPS2 lpDDSCaps2, LPDWORD lpdwTotal, LPDWORD lpdwFree)
{
	FIXME("IDirectDraw::GetAvailableVidMem: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE7 *lpDDS)
{
	FIXME("IDirectDraw::GetSurfaceFromDC: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::RestoreAllSurfaces()
{
	FIXME("IDirectDraw::RestoreAllSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::TestCooperativeLevel()
{
	FIXME("IDirectDraw::TestCooperativeLevel: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER2 lpdddi, DWORD dwFlags)
{
	devid.guidDeviceIdentifier = device_template;
	memcpy(lpdddi,&devid,sizeof(DDDEVICEIDENTIFIER2));
	return DD_OK;
}
HRESULT WINAPI glDirectDraw7::StartModeTest(LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags)
{
	FIXME("IDirectDraw::StartModeTest: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw7::EvaluateMode(DWORD dwFlags, DWORD *pSecondsUntilTimeout)
{
	FIXME("IDirectDraw::EvaluateMode: stub\n");
	ERR(DDERR_GENERIC);
}

void glDirectDraw7::DeleteGL()
{
	if(hRC)
	{
		if(dib.enabled)
		{
			if(dib.hbitmap)	DeleteObject(dib.hbitmap);
			if(dib.hdc)	DeleteDC(dib.hdc);
			ZeroMemory(&dib,sizeof(DIB));
		}
		DeleteShaders();
		DeleteFBO();
		if(PBO)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
			glDeleteBuffers(1,&PBO);
			PBO = 0;
		}
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
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = "DXGLRenderWindow";
		wndclass.hIconSm = NULL;
		if(!RegisterClassExA(&wndclass)) ERR(DDERR_GENERIC);
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
	if(hWnd)
	{
		hRenderWnd = CreateWindowA("DXGLRenderWindow","Renderer",WS_CHILD|WS_VISIBLE,0,0,rectRender.right - rectRender.left,
			rectRender.bottom - rectRender.top,hWnd,NULL,NULL,this);
		hasHWnd = true;
	}
	else
	{
		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);
		hRenderWnd = CreateWindowExA(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST,"DXGLRenderWindow","Renderer",
			WS_POPUP,0,0,width,height,0,0,NULL,this);
		hasHWnd = false;
	}
	SetWindowPos(hRenderWnd,HWND_TOP,0,0,rectRender.right,rectRender.bottom,SWP_SHOWWINDOW);
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	if(hasHWnd) pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	else pfd.dwFlags = pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	if(hasHWnd) pfd.cColorBits = bpp;
	else
	{
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
	}
	pfd.iLayerType = PFD_MAIN_PLANE;
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
	InitGLExt();
	SetSwap(1);
	SetSwap(0);
	glViewport(0,0,width,height);
	glDisable(GL_DEPTH_TEST);
	const GLubyte *glver = glGetString(GL_VERSION);
	gl_caps.Version = (GLfloat)atof((char*)glver);
	if(gl_caps.Version >= 2)
	{
		glver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		gl_caps.ShaderVer = (GLfloat)atof((char*)glver);
	}
	else gl_caps.ShaderVer = 0;
	CompileShaders();
	InitFBO();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	SwapBuffers(hDC);
	if(!hasHWnd)
	{
		dib.enabled = true;
		dib.width = width;
		dib.height = height;
		dib.pitch = (((width<<3)+31)&~31) >>3;
		dib.pixels = NULL;
		dib.hdc = CreateCompatibleDC(NULL);
		ZeroMemory(&dib.info,sizeof(BITMAPINFO));
		dib.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		dib.info.bmiHeader.biBitCount = 32;
		dib.info.bmiHeader.biWidth = width;
		dib.info.bmiHeader.biHeight = height;
		dib.info.bmiHeader.biCompression = BI_RGB;
		dib.info.bmiHeader.biPlanes = 1;
		dib.hbitmap = CreateDIBSection(dib.hdc,&dib.info,DIB_RGB_COLORS,(void**)&dib.pixels,NULL,0);
		glGenBuffers(1,&PBO);
		glBindBuffer(GL_PIXEL_PACK_BUFFER,PBO);
		glBufferData(GL_PIXEL_PACK_BUFFER,width*height*4,NULL,GL_STREAM_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
	}
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
	int oldx,oldy;
	float mulx, muly;
	int translatex, translatey;
	LPARAM newpos;
	HWND hParent;
	LONG sizes[6];
	HCURSOR cursor;
	if(msg == WM_CREATE)
	{
		SetWindowLongPtr(hwnd,GWLP_USERDATA,(LONG_PTR)this);
		return 0;
	}
	if(msg == WM_SETCURSOR)
	{
		hParent = GetParent(hwnd);
		cursor = (HCURSOR)GetClassLong(hParent,GCL_HCURSOR);
		SetCursor(cursor);
		return SendMessage(hParent,msg,wParam,lParam);
	}
	if((msg >= WM_MOUSEFIRST) && (msg <= WM_MOUSELAST))
	{
		hParent = GetParent(hwnd);
		if((dxglcfg.scaler != 0) && fullscreen)
		{
			oldx = LOWORD(lParam);
			oldy = HIWORD(lParam);
			GetSizes(sizes);
			mulx = (float)sizes[2] / (float)sizes[0];
			muly = (float)sizes[3] / (float)sizes[1];
			translatex = (sizes[4]-sizes[0])/2;
			translatey = (sizes[5]-sizes[1])/2;
			oldx -= translatex;
			oldy -= translatey;
			oldx = (int)((float)oldx * mulx);
			oldy = (int)((float)oldy * muly);
			if(oldx < 0) oldx = 0;
			if(oldy < 0) oldy = 0;
			if(oldx >= sizes[2]) oldx = sizes[2]-1;
			if(oldy >= sizes[3]) oldy = sizes[3]-1;
			newpos = oldx + (oldy << 16);
			return SendMessage(hParent,msg,wParam,newpos);
		}
		else return SendMessage(hParent,msg,wParam,lParam);
	}
	return DefWindowProc(hwnd,msg,wParam,lParam);
}
void glDirectDraw7::GetHandles(HWND *hwnd, HWND *hrender)
{
	if(hwnd) *hwnd = hWnd;
	if(hrender) *hrender = hRenderWnd;
}

// DDRAW1 wrapper
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
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw1::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw1::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw1::EnumSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw1::FlipToGDISurface()
{
	return glDD7->FlipToGDISurface();
}
HRESULT WINAPI glDirectDraw1::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	return glDD7->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}
HRESULT WINAPI glDirectDraw1::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return glDD7->GetDisplayMode((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDraw1::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw1::GetGDISurface(LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw1::GetGDISurface: stub\n");
	ERR(DDERR_GENERIC);
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
	return glDD7->Initialize(lpGUID);
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
// DDRAW2 wrapper
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
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw2::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw2::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw2::EnumSurfaces: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw2::FlipToGDISurface()
{
	return glDD7->FlipToGDISurface();
}
HRESULT WINAPI glDirectDraw2::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	return glDD7->GetCaps(lpDDDriverCaps,lpDDHELCaps);
}
HRESULT WINAPI glDirectDraw2::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	return glDD7->GetDisplayMode((LPDDSURFACEDESC2)lpDDSurfaceDesc);
}
HRESULT WINAPI glDirectDraw2::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw2::GetGDISurface(LPDIRECTDRAWSURFACE FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw2::GetGDISurface: stub\n");
	ERR(DDERR_GENERIC);
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
	return glDD7->Initialize(lpGUID);
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
	MEMORYSTATUS memstatus;
	memstatus.dwLength = sizeof(MEMORYSTATUS);
	*lpdwTotal = memstatus.dwTotalVirtual;
	*lpdwFree = memstatus.dwAvailVirtual;
	return DD_OK;
}
// DDRAW4 wrapper
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
	LPDIRECTDRAWSURFACE7 lpDDS7;
	HRESULT err = glDD7->CreateSurface((LPDDSURFACEDESC2)lpDDSurfaceDesc,&lpDDS7,pUnkOuter);
	if(err == DD_OK)
	{
		lpDDS7->QueryInterface(IID_IDirectDrawSurface4,(LPVOID*) lplpDDSurface);
		lpDDS7->Release();
		return DD_OK;
	}
	return err;}
HRESULT WINAPI glDirectDraw4::DuplicateSurface(LPDIRECTDRAWSURFACE4 lpDDSurface, LPDIRECTDRAWSURFACE4 FAR *lplpDupDDSurface)
{
	FIXME("glDirectDraw4::DuplicateSurface: stub\n");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw4::EnumDisplayModes(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback)
{
	return ::EnumDisplayModes(dwFlags,lpDDSurfaceDesc,lpContext,lpEnumModesCallback);
}
HRESULT WINAPI glDirectDraw4::EnumSurfaces(DWORD dwFlags, LPDDSURFACEDESC2 lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumSurfacesCallback)
{
	FIXME("glDirectDraw4::EnumSurfaces: stub\n");
	ERR(DDERR_GENERIC);
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
	return glDD7->GetDisplayMode(lpDDSurfaceDesc2);
}
HRESULT WINAPI glDirectDraw4::GetFourCCCodes(LPDWORD lpNumCodes, LPDWORD lpCodes)
{
	return glDD7->GetFourCCCodes(lpNumCodes,lpCodes);
}
HRESULT WINAPI glDirectDraw4::GetGDISurface(LPDIRECTDRAWSURFACE4 FAR *lplpGDIDDSurface)
{
	FIXME("glDirectDraw4::GetGDISurface: stub\n");
	ERR(DDERR_GENERIC);
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
	return glDD7->Initialize(lpGUID);
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
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirectDraw4::GetSurfaceFromDC(HDC hdc, LPDIRECTDRAWSURFACE4 *lpDDS)
{
	FIXME("IDirectDraw4::GetSurfaceFromDC: stub\n");
	ERR(DDERR_GENERIC);
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
	devid.guidDeviceIdentifier = device_template;
	memcpy(lpdddi,&devid,sizeof(DDDEVICEIDENTIFIER));
	return DD_OK;
}

// Render Window event handler
LRESULT CALLBACK RenderWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
