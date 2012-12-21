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
#include "texture.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DVertexBuffer.h"
#include "glDirect3DViewport.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DLight.h"

D3DDEVICEDESC7 d3ddesc = 
{
	D3DDEVCAPS_CANBLTSYSTONONLOCAL | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | 
		D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
		D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY, // dwDevCaps
	{ //dpcLineCaps
		sizeof(D3DPRIMCAPS),
		D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLNONE, // dwMiscCaps
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST, //dwRasterCaps
		D3DPCMPCAPS_LESSEQUAL, //dwZCmpCaps
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA |
		D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
		D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA, //dwSrcBlendCaps
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA |
		D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
		D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA, //dwDestBlendCaps
		D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
		D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL, //dwAlphaCmpCaps
		D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORGOURAUDRGB, //dwShadeCaps
		D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_PERSPECTIVE, //dwTextureCaps
		D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
		D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
		D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT  | D3DPTFILTERCAPS_MINFLINEAR  |
		D3DPTFILTERCAPS_MINFPOINT, //dwTextureFilterCaps
		D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_DECALALPHA |
		D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA, //dwTextureBlendCaps
		D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_INDEPENDENTUV |D3DPTADDRESSCAPS_MIRROR |
		D3DPTADDRESSCAPS_WRAP, //dwTextureAddressCaps
		0, //dwStippleWidth
		0  //dwStippleHeight
	},
	{ //dpcTriCaps
		sizeof(D3DPRIMCAPS),
		D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLNONE, // dwMiscCaps
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST, //dwRasterCaps
		D3DPCMPCAPS_LESSEQUAL, //dwZCmpCaps
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA |
		D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
		D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA, //dwSrcBlendCaps
		D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
		D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA |
		D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
		D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA, //dwDestBlendCaps
		D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
		D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL, //dwAlphaCmpCaps
		D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORGOURAUDRGB, //dwShadeCaps
		D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_PERSPECTIVE, //dwTextureCaps
		D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
		D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
		D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT  | D3DPTFILTERCAPS_MINFLINEAR  |
		D3DPTFILTERCAPS_MINFPOINT, //dwTextureFilterCaps
		D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_DECALALPHA |
		D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA, //dwTextureBlendCaps
		D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_INDEPENDENTUV |D3DPTADDRESSCAPS_MIRROR |
		D3DPTADDRESSCAPS_WRAP, //dwTextureAddressCaps
		0, //dwStippleWidth
		0  //dwStippleHeight
	},
	DDBD_16|DDBD_24|DDBD_32, //dwDeviceRenderBitDepth 
	DDBD_16|DDBD_24|DDBD_32, //dwDeviceZBufferBitDepth
	1, //dwMinTextureWidth
	1, //dwMinTextureHeight
	0, //dwMaxTextureWidth
	0, //dwMaxTextureHeight
	0, //dwMaxTextureRepeat
	0, //dwMaxTextureAspectRatio
	1, //dwMaxAnisotropy
	0.0f, //dvGuardBandLeft
	0.0f, //dvGuardBandTop
	0.0f, //dvGuardBandRight
	0.0f, //dvGuardBandBottom
	0.0f, //dvExtentsAdjust 
	0, //dwStencilCaps
	8, //dwFVFCaps 
	D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 | D3DTEXOPCAPS_MODULATE |
	D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X | D3DTEXOPCAPS_ADD |
	D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X | D3DTEXOPCAPS_SUBTRACT |
	D3DTEXOPCAPS_ADDSMOOTH | D3DTEXOPCAPS_BLENDDIFFUSEALPHA | D3DTEXOPCAPS_BLENDTEXTUREALPHA | 
	D3DTEXOPCAPS_BLENDTEXTUREALPHAPM | D3DTEXOPCAPS_BLENDCURRENTALPHA, //dwTextureOpCaps
	8, //wMaxTextureBlendStages
	8, //wMaxSimultaneousTextures
	8, //dwMaxActiveLights
	0.0f, //dvMaxVertexW 
	IID_IDirect3DHALDevice, //deviceGUID
	0, //wMaxUserClipPlanes
	0, //wMaxVertexBlendMatrices
	D3DVTXPCAPS_DIRECTIONALLIGHTS|D3DVTXPCAPS_POSITIONALLIGHTS, //dwVertexProcessingCaps 
	0,0,0,0 //dwReserved1 through dwReserved4
};

D3DDEVICEDESC d3ddesc3 =
{
	sizeof(D3DDEVICEDESC), // dwSize
	D3DDD_BCLIPPING|D3DDD_COLORMODEL|D3DDD_DEVCAPS|D3DDD_DEVICERENDERBITDEPTH|
	D3DDD_DEVICEZBUFFERBITDEPTH|D3DDD_LIGHTINGCAPS|D3DDD_LINECAPS|D3DDD_MAXBUFFERSIZE|
	D3DDD_MAXVERTEXCOUNT|D3DDD_TRANSFORMCAPS|D3DDD_TRICAPS, // dwFlags
	D3DCOLOR_RGB, // dcmColorModel
	d3ddesc.dwDevCaps,
	{ //dtcTransformCaps
		sizeof(D3DTRANSFORMCAPS), //dwSize
		0 // dwCaps
	},
	FALSE, //bClipping
	{ // dlcLightingCaps
		sizeof(D3DLIGHTINGCAPS), //dwSize
		D3DLIGHTCAPS_DIRECTIONAL|D3DLIGHTCAPS_POINT|D3DLIGHTCAPS_SPOT, // dwCaps
		D3DLIGHTINGMODEL_RGB, // dwLightingModel
		8  //dwNumLights
	},
	d3ddesc.dpcLineCaps,
	d3ddesc.dpcTriCaps,
	d3ddesc.dwDeviceRenderBitDepth,
	d3ddesc.dwDeviceZBufferBitDepth,
	0, // dwMaxBufferSize
	65536, // dwMaxVertexCount
	d3ddesc.dwMinTextureWidth,
	d3ddesc.dwMinTextureHeight,
	d3ddesc.dwMaxTextureWidth,
	d3ddesc.dwMaxTextureHeight,
	0, // dwMinStippleWidth
	32, // dwMaxStippleWidth
	0, // dwMinStippleHeight
	32, // dwMaxStippleHeight
	d3ddesc.dwMaxTextureRepeat,
	d3ddesc.dwMaxTextureAspectRatio,
	d3ddesc.dwMaxAnisotropy,
	d3ddesc.dvGuardBandLeft,
	d3ddesc.dvGuardBandTop,
	d3ddesc.dvGuardBandRight,
	d3ddesc.dvGuardBandBottom,
	d3ddesc.dvExtentsAdjust,
	d3ddesc.dwStencilCaps,
	d3ddesc.dwFVFCaps,
	d3ddesc.dwTextureOpCaps,
	d3ddesc.wMaxTextureBlendStages,
	d3ddesc.wMaxSimultaneousTextures
};

struct D3DDevice
{
	char *name;
	char *devname;
};
D3DDevice devices[3] =
{
	{
		"Simulated RGB Rasterizer",
		"DXGL RGB Rasterizer",
	},
	{
		"DXGL Hardware Accelerator",
		"DXGL D3D HAL",
	},
	{
		"DXGL Hardware Accelerator with Transform and Lighting",
		"DXGL D3D T&L HAL",
	}
};
glDirect3D7::glDirect3D7(glDirectDraw7 *glDD7)
{
	refcount=1;
	this->glDD7 = glDD7;
	glDD7->AddRef();
	glD3D3 = NULL;
	glD3D2 = NULL;
}

glDirect3D7::~glDirect3D7()
{
	if(glD3D3) glD3D3->Release();
	if(glD3D2) glD3D2->Release();
	glDD7->Release();
}

ULONG WINAPI glDirect3D7::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3D7::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3D7::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	if(riid == IID_IDirect3D3)
	{
		if(glD3D3)
		{
			*ppvObj = glD3D3;
			glD3D3->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3D3(this);
			glD3D3 = (glDirect3D3*)*ppvObj;
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3D2)
	{
		if(glD3D2)
		{
			*ppvObj = glD3D2;
			glD3D2->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3D2(this);
			glD3D2 = (glDirect3D2*)*ppvObj;
			return D3D_OK;
		}
	}
	return E_NOINTERFACE;
}


HRESULT WINAPI glDirect3D7::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice)
{
	if(!this) return DDERR_INVALIDPARAMS;
	glDirect3DDevice7 *glD3DDev7 = new glDirect3DDevice7(this,(glDirectDrawSurface7*)lpDDS);
	*lplpD3DDevice = (LPDIRECT3DDEVICE7) glD3DDev7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpDirect3DLight) return DDERR_INVALIDPARAMS;
	if(pUnkOuter) return DDERR_INVALIDPARAMS;
	*lplpDirect3DLight = new glDirect3DLight();
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateMaterial(LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpDirect3DMaterial) return D3D_OK;
	if(pUnkOuter) return DDERR_INVALIDPARAMS;
	*lplpDirect3DMaterial = new glDirect3DMaterial3();
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpD3DVertexBuffer) return DDERR_INVALIDPARAMS;
	if(!lpVBDesc) return DDERR_INVALIDPARAMS;
	*lplpD3DVertexBuffer = (LPDIRECT3DVERTEXBUFFER7)new glDirect3DVertexBuffer7(this,*lpVBDesc,dwFlags);
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateViewport(LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpD3DViewport) return DDERR_INVALIDPARAMS;
	if(pUnkOuter) return DDERR_INVALIDPARAMS;
	*lplpD3DViewport = new glDirect3DViewport3();
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::EnumDevices(LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpEnumDevicesCallback) return DDERR_INVALIDPARAMS;
	HRESULT result;
	D3DDEVICEDESC7 desc = d3ddesc;
	for(int i = 0; i < 3; i++)
	{
		switch(i)
		{
		case 0:
			desc.deviceGUID = IID_IDirect3DRGBDevice;
			break;
		case 1:
			desc.deviceGUID = IID_IDirect3DHALDevice;
			desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION;
			break;
		case 2:
			desc.deviceGUID = IID_IDirect3DTnLHalDevice;
			desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_HWTRANSFORMANDLIGHT;
			break;
		}
		result = lpEnumDevicesCallback(devices[i].name,devices[i].devname,&desc,lpUserArg);
		if(result != D3DENUMRET_OK) break;
	}
	return D3D_OK;
}

HRESULT WINAPI glDirect3D7::EnumDevices3(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpEnumDevicesCallback) return DDERR_INVALIDPARAMS;
	HRESULT result;
	D3DDEVICEDESC desc = d3ddesc3;
	GUID guid = IID_IDirect3DRGBDevice;
	result = lpEnumDevicesCallback(&guid,devices[0].name,devices[0].devname,&desc,&desc,lpUserArg);
	if(result != D3DENUMRET_OK) return D3D_OK;
	guid = IID_IDirect3DHALDevice;
	result = lpEnumDevicesCallback(&guid,devices[1].name,devices[1].devname,&desc,&desc,lpUserArg);
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::EnumZBufferFormats(REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	if(!this) return DDERR_INVALIDPARAMS;
	DDPIXELFORMAT ddpf;
	ZeroMemory(&ddpf,sizeof(DDPIXELFORMAT));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	ddpf.dwFlags = DDPF_ZBUFFER;
	ddpf.dwZBufferBitDepth = 16;
	ddpf.dwZBitMask = 0xffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBufferBitDepth = 24;
	ddpf.dwZBitMask = 0xffffff00;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBufferBitDepth = 32;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	ddpf.dwZBitMask = 0xffffffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	if(GLEXT_EXT_packed_depth_stencil || GLEXT_NV_packed_depth_stencil)
	{
		ddpf.dwZBufferBitDepth = 32;
		ddpf.dwStencilBitDepth = 8;
		ddpf.dwZBitMask = 0xffffff00;
		ddpf.dwStencilBitMask = 0xff;
		if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
		ddpf.dwZBitMask = 0x00ffffff;
		ddpf.dwStencilBitMask = 0xff000000;
		if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) return D3D_OK;
	}
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::EvictManagedTextures()
{
	return D3D_OK;
}

HRESULT WINAPI glDirect3D7::FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpD3DFDS) return DDERR_INVALIDPARAMS;
	if(!lpD3DFDR) return DDERR_INVALIDPARAMS;
	if(lpD3DFDR->dwSize < sizeof(D3DFINDDEVICERESULT)) return DDERR_INVALIDPARAMS;
	if(lpD3DFDS->dwSize < sizeof(D3DFINDDEVICESEARCH)) return DDERR_INVALIDPARAMS;
	bool found = true;
	GUID guid = IID_IDirect3DHALDevice;
	if((lpD3DFDS->dwFlags & D3DFDS_LINES) || (lpD3DFDS->dwFlags & D3DFDS_TRIANGLES))
	{
		if(lpD3DFDS->dwFlags & D3DFDS_ALPHACMPCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwAlphaCmpCaps & lpD3DFDS->dpcPrimCaps.dwAlphaCmpCaps)
				!= lpD3DFDS->dpcPrimCaps.dwAlphaCmpCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_DSTBLENDCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwDestBlendCaps & lpD3DFDS->dpcPrimCaps.dwDestBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwDestBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_MISCCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwMiscCaps & lpD3DFDS->dpcPrimCaps.dwMiscCaps)
				!= lpD3DFDS->dpcPrimCaps.dwMiscCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_RASTERCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwRasterCaps & lpD3DFDS->dpcPrimCaps.dwRasterCaps)
				!= lpD3DFDS->dpcPrimCaps.dwRasterCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_SHADECAPS)
		{
			if((d3ddesc.dpcTriCaps.dwShadeCaps & lpD3DFDS->dpcPrimCaps.dwShadeCaps)
				!= lpD3DFDS->dpcPrimCaps.dwShadeCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_SRCBLENDCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwSrcBlendCaps & lpD3DFDS->dpcPrimCaps.dwSrcBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwSrcBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTUREBLENDCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwTextureBlendCaps & lpD3DFDS->dpcPrimCaps.dwTextureBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTURECAPS)
		{
			if((d3ddesc.dpcTriCaps.dwTextureCaps & lpD3DFDS->dpcPrimCaps.dwTextureCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTUREFILTERCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwTextureFilterCaps & lpD3DFDS->dpcPrimCaps.dwTextureFilterCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureFilterCaps) found = false;
		}
		if(lpD3DFDS->dwCaps & D3DFDS_ZCMPCAPS)
		{
			if((d3ddesc.dpcTriCaps.dwZCmpCaps & lpD3DFDS->dpcPrimCaps.dwZCmpCaps)
				!= lpD3DFDS->dpcPrimCaps.dwZCmpCaps) found = false;
		}
	}
	if(lpD3DFDS->dwFlags & D3DFDS_COLORMODEL)
	{
		if((d3ddesc3.dcmColorModel & lpD3DFDS->dcmColorModel) != lpD3DFDS->dcmColorModel) found = false;
	}
	if(lpD3DFDS->dwFlags & D3DFDS_GUID)
	{
		if((lpD3DFDS->guid != IID_IDirect3DRGBDevice) && (lpD3DFDS->guid != IID_IDirect3DHALDevice))
			found = false;
		else guid = lpD3DFDS->guid;
	}
	if(lpD3DFDS->dwFlags & D3DFDS_HARDWARE)
	{
		if(lpD3DFDS->dwFlags & D3DFDS_GUID)
		{
			if(lpD3DFDS->bHardware && (lpD3DFDS->guid != IID_IDirect3DHALDevice)) found = false;
			if(!lpD3DFDS->bHardware && (lpD3DFDS->guid != IID_IDirect3DRGBDevice)) found = false;
		}
		else if(!lpD3DFDS->bHardware) guid = IID_IDirect3DRGBDevice;
	}
	if(!found) return DDERR_NOTFOUND;
	if(guid == IID_IDirect3DRGBDevice) lpD3DFDR->ddSwDesc = d3ddesc3;
	else lpD3DFDR->ddHwDesc = d3ddesc3;
	lpD3DFDR->guid = guid;
	return D3D_OK;
}


// IDirect3D3 wrapper
glDirect3D3::glDirect3D3(glDirect3D7 *glD3D7)
{
	this->glD3D7 = glD3D7;
	refcount = 1;
}

glDirect3D3::~glDirect3D3()
{
	glD3D7->Release();
}

HRESULT WINAPI glDirect3D3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glD3D7->QueryInterface(riid,ppvObj);
}

ULONG WINAPI glDirect3D3::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3D3::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3D3::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice, LPUNKNOWN pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(pUnkOuter) return DDERR_INVALIDPARAMS;
	LPDIRECT3DDEVICE7 lpD3DDev7;
	HRESULT err = glD3D7->CreateDevice(rclsid,((glDirectDrawSurface4*)lpDDS)->GetDDS7(),&lpD3DDev7);
	if(err == D3D_OK)
	{
		lpD3DDev7->QueryInterface(IID_IDirect3DDevice3,(LPVOID*) lplpD3DDevice);
		lpD3DDev7->Release();
		return D3D_OK;
	}
	return err;
}

HRESULT WINAPI glDirect3D3::CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->CreateLight(lplpDirect3DLight,pUnkOuter);
}
HRESULT WINAPI glDirect3D3::CreateMaterial(LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->CreateMaterial(lplpDirect3DMaterial,pUnkOuter);
}
HRESULT WINAPI glDirect3D3::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER* lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(pUnkOuter) return DDERR_INVALIDPARAMS;
	LPDIRECT3DVERTEXBUFFER7 lpD3DVB7;
	HRESULT err = glD3D7->CreateVertexBuffer(lpVBDesc,&lpD3DVB7,dwFlags);
	if(err == D3D_OK)
	{
		lpD3DVB7->QueryInterface(IID_IDirect3DVertexBuffer,(LPVOID*)lplpD3DVertexBuffer);
		lpD3DVB7->Release();
		return D3D_OK;
	}
	return err;
}
HRESULT WINAPI glDirect3D3::CreateViewport(LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->CreateViewport(lplpD3DViewport,pUnkOuter);
}

HRESULT WINAPI glDirect3D3::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->EnumDevices3(lpEnumDevicesCallback,lpUserArg);
}

HRESULT WINAPI glDirect3D3::EnumZBufferFormats(REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->EnumZBufferFormats(riidDevice,lpEnumCallback,lpContext);
}
HRESULT WINAPI glDirect3D3::EvictManagedTextures()
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->EvictManagedTextures();
}
HRESULT WINAPI glDirect3D3::FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->FindDevice(lpD3DFDS,lpD3DFDR);
}


glDirect3D2::glDirect3D2(glDirect3D7 *glD3D7)
{
	this->glD3D7 = glD3D7;
	refcount = 1;
}

glDirect3D2::~glDirect3D2()
{
	glD3D7->Release();
}

HRESULT WINAPI glDirect3D2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glD3D7->QueryInterface(riid,ppvObj);
}

ULONG WINAPI glDirect3D2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3D2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3D2::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice)
{
	if(!this) return DDERR_INVALIDPARAMS;
	LPDIRECT3DDEVICE7 lpD3DDev7;
	HRESULT err = glD3D7->CreateDevice(rclsid,((glDirectDrawSurface1*)lpDDS)->GetDDS7(),&lpD3DDev7);
	if(err == D3D_OK)
	{
		lpD3DDev7->QueryInterface(IID_IDirect3DDevice2,(LPVOID*)lplpD3DDevice);
		lpD3DDev7->Release();
		return D3D_OK;
	}
	return err;
}

HRESULT WINAPI glDirect3D2::CreateLight(LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->CreateLight(lplpDirect3DLight,pUnkOuter);
}

HRESULT WINAPI glDirect3D2::CreateMaterial(LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpDirect3DMaterial2) return DDERR_INVALIDPARAMS;
	glDirect3DMaterial3 *glD3DM3;
	HRESULT error = glD3D7->CreateMaterial((LPDIRECT3DMATERIAL3*)&glD3DM3,pUnkOuter);
	if(error) return error;
	glD3DM3->QueryInterface(IID_IDirect3DMaterial2,(void**)lplpDirect3DMaterial2);
	glD3DM3->Release();
	return D3D_OK;
}

HRESULT WINAPI glDirect3D2::CreateViewport(LPDIRECT3DVIEWPORT2* lplpD3DViewport2, IUnknown* pUnkOuter)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpD3DViewport2) return DDERR_INVALIDPARAMS;
	glDirect3DMaterial3 *glD3DV3;
	HRESULT error = glD3D7->CreateViewport((LPDIRECT3DVIEWPORT3*)&glD3DV3,pUnkOuter);
	if(error) return error;
	glD3DV3->QueryInterface(IID_IDirect3DViewport2,(void**)lplpD3DViewport2);
	glD3DV3->Release();
	return D3D_OK;
}

HRESULT WINAPI glDirect3D2::EnumDevices(LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->EnumDevices3(lpEnumDevicesCallback,lpUserArg);
}

HRESULT WINAPI glDirect3D2::FindDevice(LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return glD3D7->FindDevice(lpD3DFDS,lpD3DFDR);
}
