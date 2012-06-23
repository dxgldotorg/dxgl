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
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"

D3DDEVICEDESC7 d3ddesc = 
{
	D3DDEVCAPS_CANBLTSYSTONONLOCAL | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | 
		D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
		D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY, // dwDevCaps
	{ //dpcLineCaps
		sizeof(D3DPRIMCAPS),
		0, // dwMiscCaps
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST, //dwRasterCaps
		D3DPCMPCAPS_LESSEQUAL, //dwZCmpCaps
		0, //dwSrcBlendCaps
		0, //dwDestBlendCaps
		0, //dwAlphaCmpCaps
		D3DPSHADECAPS_COLORGOURAUDRGB, //dwShadeCaps
		D3DPTEXTURECAPS_PERSPECTIVE, //dwTextureCaps
		D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
		D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
		D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT  | D3DPTFILTERCAPS_MINFLINEAR  |
		D3DPTFILTERCAPS_MINFPOINT, //dwTextureFilterCaps
		0, //dwTextureBlendCaps
		0, //dwTextureAddressCaps
		0, //dwStippleWidth
		0  //dwStippleHeight
	},
	{ //dpcTriCaps
		sizeof(D3DPRIMCAPS),
		0, // dwMiscCaps
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST, //dwRasterCaps
		D3DPCMPCAPS_LESSEQUAL, //dwZCmpCaps
		0, //dwSrcBlendCaps
		0, //dwDestBlendCaps
		0, //dwAlphaCmpCaps
		D3DPSHADECAPS_COLORGOURAUDRGB, //dwShadeCaps
		D3DPTEXTURECAPS_PERSPECTIVE, //dwTextureCaps
		D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
		D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
		D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFPOINT  | D3DPTFILTERCAPS_MINFLINEAR  |
		D3DPTFILTERCAPS_MINFPOINT, //dwTextureFilterCaps
		0, //dwTextureBlendCaps
		0, //dwTextureAddressCaps
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
	0, //dwMaxAnisotropy
	0.0f, //dvGuardBandLeft
	0.0f, //dvGuardBandTop
	0.0f, //dvGuardBandRight
	0.0f, //dvGuardBandBottom
	0.0f, //dvExtentsAdjust 
	0, //dwStencilCaps
	8, //dwFVFCaps 
	D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 | D3DTEXOPCAPS_MODULATE |
	D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X | D3DTEXOPCAPS_ADD |
	D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X | D3DTEXOPCAPS_SUBTRACT |
	D3DTEXOPCAPS_ADDSMOOTH, //dwTextureOpCaps
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
}

glDirect3D7::~glDirect3D7()
{
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
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	FIXME("glDirect3D7::QueryInterface: stub");
	return E_NOINTERFACE;
}


HRESULT WINAPI glDirect3D7::CreateDevice(REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice)
{
	if(!this) return DDERR_INVALIDPARAMS;
	glDirect3DDevice7 *glD3DDev7 = new glDirect3DDevice7(this,(glDirectDrawSurface7*)lpDDS);
	*lplpD3DDevice = (LPDIRECT3DDEVICE7) glD3DDev7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7::CreateVertexBuffer(LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::CreateVertexBuffer: stub");
	return DDERR_GENERIC;
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
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3D7::EvictManagedTextures: stub");
	return DDERR_GENERIC;
}
