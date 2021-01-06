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

#include "common.h"
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3D.h"
#include "glDirect3DDevice.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DVertexBuffer.h"
#include "glDirect3DViewport.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DLight.h"
#include "ddraw.h"

const D3DDEVICEDESC7 d3ddesc_default = 
{
	D3DDEVCAPS_CANBLTSYSTONONLOCAL | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | 
		D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
		D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
		D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_EXECUTEVIDEOMEMORY, // dwDevCaps
	{ //dpcLineCaps
		sizeof(D3DPRIMCAPS),
		D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLNONE, // dwMiscCaps
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_FOGTABLE |
		D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_WFOG, //dwRasterCaps
		D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
		D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL, //dwZCmpCaps
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
		D3DPSHADECAPS_ALPHAFLATBLEND | D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORFLATRGB |
		D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_FOGFLAT | D3DPSHADECAPS_FOGGOURAUD, //dwShadeCaps
		D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_TRANSPARENCY, //dwTextureCaps
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
		D3DPRASTERCAPS_SUBPIXEL | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_FOGTABLE |
		D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_WFOG, //dwRasterCaps
		D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_LESSEQUAL, //dwZCmpCaps
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
		D3DPSHADECAPS_ALPHAFLATBLEND | D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORFLATRGB |
		D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_FOGFLAT | D3DPSHADECAPS_FOGGOURAUD, //dwShadeCaps
		D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_TRANSPARENCY, //dwTextureCaps
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
	D3DVTXPCAPS_DIRECTIONALLIGHTS|D3DVTXPCAPS_POSITIONALLIGHTS|D3DVTXPCAPS_VERTEXFOG, //dwVertexProcessingCaps 
	0,0,0,0 //dwReserved1 through dwReserved4
};

const D3DDEVICEDESC d3ddesc3_default =
{
	sizeof(D3DDEVICEDESC), // dwSize
	D3DDD_BCLIPPING|D3DDD_COLORMODEL|D3DDD_DEVCAPS|D3DDD_DEVICERENDERBITDEPTH|
	D3DDD_DEVICEZBUFFERBITDEPTH|D3DDD_LIGHTINGCAPS|D3DDD_LINECAPS|D3DDD_MAXBUFFERSIZE|
	D3DDD_MAXVERTEXCOUNT|D3DDD_TRANSFORMCAPS|D3DDD_TRICAPS, // dwFlags
	D3DCOLOR_RGB, // dcmColorModel
	d3ddesc_default.dwDevCaps,
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
	d3ddesc_default.dpcLineCaps,
	d3ddesc_default.dpcTriCaps,
	d3ddesc_default.dwDeviceRenderBitDepth,
	d3ddesc_default.dwDeviceZBufferBitDepth,
	0, // dwMaxBufferSize
	65536, // dwMaxVertexCount
	d3ddesc_default.dwMinTextureWidth,
	d3ddesc_default.dwMinTextureHeight,
	d3ddesc_default.dwMaxTextureWidth,
	d3ddesc_default.dwMaxTextureHeight,
	0, // dwMinStippleWidth
	32, // dwMaxStippleWidth
	0, // dwMinStippleHeight
	32, // dwMaxStippleHeight
	d3ddesc_default.dwMaxTextureRepeat,
	d3ddesc_default.dwMaxTextureAspectRatio,
	d3ddesc_default.dwMaxAnisotropy,
	d3ddesc_default.dvGuardBandLeft,
	d3ddesc_default.dvGuardBandTop,
	d3ddesc_default.dvGuardBandRight,
	d3ddesc_default.dvGuardBandBottom,
	d3ddesc_default.dvExtentsAdjust,
	d3ddesc_default.dwStencilCaps,
	d3ddesc_default.dwFVFCaps,
	d3ddesc_default.dwTextureOpCaps,
	d3ddesc_default.wMaxTextureBlendStages,
	d3ddesc_default.wMaxSimultaneousTextures
};

glDirect3D7Vtbl glDirect3D7_impl =
{
	glDirect3D7_QueryInterface,
	glDirect3D7_AddRef,
	glDirect3D7_Release,
	glDirect3D7_EnumDevices,
	glDirect3D7_CreateDevice,
	glDirect3D7_EnumZBufferFormats,
	glDirect3D7_EvictManagedTextures
};

HRESULT glDirect3D7_Create(glDirectDraw7 *gl_DD7, glDirect3D7 **glD3D7)
{
	TRACE_ENTER(1,14,gl_DD7,14,glD3D7);
	glDirect3D7 *This = (glDirect3D7*)malloc(sizeof(glDirect3D7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3D7_impl;
	This->glDD7 = gl_DD7;
	*glD3D7 = This;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

ULONG WINAPI glDirect3D7_AddRef(glDirect3D7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDD7->AddRef());
}
ULONG WINAPI glDirect3D7_Release(glDirect3D7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glDD7->Release());
}

HRESULT WINAPI glDirect3D7_QueryInterface(glDirect3D7 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, This->glDD7->QueryInterface(riid, ppvObj));
}

HRESULT WINAPI glDirect3D7_CreateDevice(glDirect3D7 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice)
{
	TRACE_ENTER(4, 14, This, 24, &rclsid, 14, lpDDS, 14, lplpD3DDevice);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirect3D7_CreateDevice2(This, rclsid, lpDDS, lplpD3DDevice, 7));
}

HRESULT glDirect3D7_CreateDevice2(glDirect3D7 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE7 lpDDS, LPDIRECT3DDEVICE7 *lplpD3DDevice, int version)
{
	TRACE_ENTER(5,14,This,24,&rclsid,14,lpDDS,14,lplpD3DDevice,11,version);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT ret;
	glDirect3DDevice7 *glD3DDev7 = NULL;
	ret = glDirect3DDevice7_Create(rclsid, This, (glDirectDrawSurface7*)lpDDS, NULL, version, &glD3DDev7);
	if(FAILED(ret))
	{
		if (glD3DDev7) free(glD3DDev7);
		TRACE_EXIT(23,ret);
		return ret;
	}
	*lplpD3DDevice = (LPDIRECT3DDEVICE7) glD3DDev7;
	TRACE_VAR("*lplpD3DDevice",14,*lplpD3DDevice);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7_CreateLight(glDirect3D7 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	HRESULT ret;
	TRACE_ENTER(3,14,This,14,lplpDirect3DLight,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDirect3DLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	ret = glDirect3DLight_CreateNoLight((glDirect3DLight**)lplpDirect3DLight);
	TRACE_VAR("*lplpDirect3DLight",14,*lplpDirect3DLight);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3D7_CreateMaterial(glDirect3D7 *This, LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter)
{
	HRESULT ret;
	TRACE_ENTER(3,14,This,14,lplpDirect3DMaterial,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDirect3DMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	ret = glDirect3DMaterial3_Create((glDirect3DMaterial3**)lplpDirect3DMaterial);
	TRACE_VAR("*lplpDirect3DMaterial",14,*lplpDirect3DMaterial);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3D7_CreateVertexBuffer(glDirect3D7 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER7* lplpD3DVertexBuffer, DWORD dwFlags)
{
	HRESULT ret;
	TRACE_ENTER(4,14,This,14,lpVBDesc,14,lplpD3DVertexBuffer,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DVertexBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpVBDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	ret = glDirect3DVertexBuffer7_Create(This, *lpVBDesc, dwFlags, (glDirect3DVertexBuffer7**)lplpD3DVertexBuffer);
	TRACE_VAR("*lplpD3DVertexBuffer",14,*lplpD3DVertexBuffer);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3D7_CreateViewport(glDirect3D7 *This, LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpD3DViewport,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT ret = glDirect3DViewport3_Create(lplpD3DViewport);
	TRACE_VAR("*lplpD3DViewport",14,*lplpD3DViewport);
	TRACE_EXIT(23,ret);
	return ret;
}

void FixCapsTexture(D3DDEVICEDESC7 *d3ddesc, D3DDEVICEDESC *d3ddesc3, glRenderer *renderer)
{
	if (!d3ddesc->dwMaxTextureWidth)
	{
		if (!renderer)
		{
			HWND hGLWnd = CreateWindow(_T("Test"), NULL, WS_POPUP, 0, 0, 16, 16, NULL, NULL, NULL, NULL);
			glRenderer *tmprenderer = (glRenderer*)malloc(sizeof(glRenderer));
			DEVMODE mode;
			mode.dmSize = sizeof(DEVMODE);
			EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &mode);
			glRenderer_Init(tmprenderer, 16, 16, mode.dmBitsPerPel, false, mode.dmDisplayFrequency, hGLWnd, NULL, FALSE);
			d3ddesc->dwMaxTextureWidth = d3ddesc->dwMaxTextureHeight =
				d3ddesc->dwMaxTextureRepeat = d3ddesc->dwMaxTextureAspectRatio = tmprenderer->gl_caps.TextureMax;
			d3ddesc3->dwMaxTextureWidth = d3ddesc3->dwMaxTextureHeight =
				d3ddesc3->dwMaxTextureRepeat = d3ddesc3->dwMaxTextureAspectRatio = tmprenderer->gl_caps.TextureMax;
			glRenderer_Delete(tmprenderer);
			free(tmprenderer);
		}
		else
		{
			d3ddesc->dwMaxTextureWidth = d3ddesc->dwMaxTextureHeight =
				d3ddesc->dwMaxTextureRepeat = d3ddesc->dwMaxTextureAspectRatio = renderer->gl_caps.TextureMax;
			d3ddesc3->dwMaxTextureWidth = d3ddesc3->dwMaxTextureHeight =
				d3ddesc3->dwMaxTextureRepeat = d3ddesc3->dwMaxTextureAspectRatio = renderer->gl_caps.TextureMax;
		}
	}
}

HRESULT WINAPI glDirect3D7_EnumDevices(glDirect3D7 *This, LPD3DENUMDEVICESCALLBACK7 lpEnumDevicesCallback, LPVOID lpUserArg)
{
	TRACE_ENTER(3,14,This,14,lpEnumDevicesCallback,14,lpUserArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpEnumDevicesCallback) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT result;
	FixCapsTexture(&This->glDD7->d3ddesc, &This->glDD7->d3ddesc3, This->glDD7->renderer);
	D3DDEVICEDESC7 desc = This->glDD7->d3ddesc;
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
		result = lpEnumDevicesCallback(This->glDD7->stored_devices[i].name,This->glDD7->stored_devices[i].devname,&desc,lpUserArg);
		if(result != D3DENUMRET_OK) break;
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D7_EnumDevices3(glDirect3D7 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	TRACE_ENTER(3,14,This,14,lpEnumDevicesCallback,14,lpUserArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpEnumDevicesCallback) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	HRESULT result;
	FixCapsTexture(&This->glDD7->d3ddesc, &This->glDD7->d3ddesc3, This->glDD7->renderer);
	D3DDEVICEDESC desc = This->glDD7->d3ddesc3;
	GUID guid = IID_IDirect3DRGBDevice;
	result = lpEnumDevicesCallback(&guid,This->glDD7->stored_devices[0].name,This->glDD7->stored_devices[0].devname,&desc,&desc,lpUserArg);
	if(result != D3DENUMRET_OK) TRACE_RET(HRESULT,23,D3D_OK);
	guid = IID_IDirect3DHALDevice;
	result = lpEnumDevicesCallback(&guid,This->glDD7->stored_devices[1].name,This->glDD7->stored_devices[1].devname,&desc,&desc,lpUserArg);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7_EnumZBufferFormats(glDirect3D7 *This, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	TRACE_ENTER(4,14,This,24,&riidDevice,14,lpEnumCallback,14,lpContext);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DDPIXELFORMAT ddpf;
	ZeroMemory(&ddpf,sizeof(DDPIXELFORMAT));
	ddpf.dwSize = sizeof(DDPIXELFORMAT);
	ddpf.dwFlags = DDPF_ZBUFFER;
	ddpf.dwZBufferBitDepth = 16;
	ddpf.dwZBitMask = 0xffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT,23,D3D_OK);
	ddpf.dwZBufferBitDepth = 24;
	ddpf.dwZBitMask = 0xffffff00;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT,23,D3D_OK);
	ddpf.dwZBufferBitDepth = 32;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT,23,D3D_OK);
	ddpf.dwZBitMask = 0xffffffff;
	if(lpEnumCallback(&ddpf,lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT,23,D3D_OK);
	if (This->glDD7->renderer)
	{
		if (This->glDD7->renderer->ext->GLEXT_EXT_packed_depth_stencil || This->glDD7->renderer->ext->GLEXT_NV_packed_depth_stencil)
		{
			ddpf.dwZBufferBitDepth = 32;
			ddpf.dwStencilBitDepth = 8;
			ddpf.dwZBitMask = 0xffffff00;
			ddpf.dwStencilBitMask = 0xff;
			if (lpEnumCallback(&ddpf, lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT, 23, D3D_OK);
			ddpf.dwZBitMask = 0x00ffffff;
			ddpf.dwStencilBitMask = 0xff000000;
			if (lpEnumCallback(&ddpf, lpContext) == D3DENUMRET_CANCEL) TRACE_RET(HRESULT, 23, D3D_OK);
		}
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3D7_EvictManagedTextures(glDirect3D7 *This)
{
	TRACE_ENTER(1,14,This);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D7_FindDevice(glDirect3D7 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	TRACE_ENTER(3,14,This,14,lpD3DFDS,14,lpD3DFDR);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DFDS) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpD3DFDR) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpD3DFDR->dwSize < sizeof(D3DFINDDEVICERESULT)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpD3DFDS->dwSize < sizeof(D3DFINDDEVICESEARCH)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FixCapsTexture(&This->glDD7->d3ddesc, &This->glDD7->d3ddesc3, This->glDD7->renderer);
	bool found = true;
	GUID guid = IID_IDirect3DHALDevice;
	if((lpD3DFDS->dwFlags & D3DFDS_LINES) || (lpD3DFDS->dwFlags & D3DFDS_TRIANGLES))
	{
		if(lpD3DFDS->dwFlags & D3DFDS_ALPHACMPCAPS)
		{
			if((This->glDD7->d3ddesc.dpcTriCaps.dwAlphaCmpCaps & lpD3DFDS->dpcPrimCaps.dwAlphaCmpCaps)
				!= lpD3DFDS->dpcPrimCaps.dwAlphaCmpCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_DSTBLENDCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwDestBlendCaps & lpD3DFDS->dpcPrimCaps.dwDestBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwDestBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_MISCCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwMiscCaps & lpD3DFDS->dpcPrimCaps.dwMiscCaps)
				!= lpD3DFDS->dpcPrimCaps.dwMiscCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_RASTERCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwRasterCaps & lpD3DFDS->dpcPrimCaps.dwRasterCaps)
				!= lpD3DFDS->dpcPrimCaps.dwRasterCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_SHADECAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwShadeCaps & lpD3DFDS->dpcPrimCaps.dwShadeCaps)
				!= lpD3DFDS->dpcPrimCaps.dwShadeCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_SRCBLENDCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwSrcBlendCaps & lpD3DFDS->dpcPrimCaps.dwSrcBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwSrcBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTUREBLENDCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwTextureBlendCaps & lpD3DFDS->dpcPrimCaps.dwTextureBlendCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureBlendCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTURECAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwTextureCaps & lpD3DFDS->dpcPrimCaps.dwTextureCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureCaps) found = false;
		}
		if(lpD3DFDS->dwFlags & D3DFDS_TEXTUREFILTERCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwTextureFilterCaps & lpD3DFDS->dpcPrimCaps.dwTextureFilterCaps)
				!= lpD3DFDS->dpcPrimCaps.dwTextureFilterCaps) found = false;
		}
		if(lpD3DFDS->dwCaps & D3DFDS_ZCMPCAPS)
		{
			if ((This->glDD7->d3ddesc.dpcTriCaps.dwZCmpCaps & lpD3DFDS->dpcPrimCaps.dwZCmpCaps)
				!= lpD3DFDS->dpcPrimCaps.dwZCmpCaps) found = false;
		}
	}
	if(lpD3DFDS->dwFlags & D3DFDS_COLORMODEL)
	{
		if ((This->glDD7->d3ddesc3.dcmColorModel & lpD3DFDS->dcmColorModel) != lpD3DFDS->dcmColorModel) found = false;
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
	if(!found) TRACE_RET(HRESULT,23,DDERR_NOTFOUND);
	if(guid == IID_IDirect3DRGBDevice) lpD3DFDR->ddSwDesc = This->glDD7->d3ddesc3;
	else lpD3DFDR->ddHwDesc = This->glDD7->d3ddesc3;
	lpD3DFDR->guid = guid;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}


// IDirect3D3 wrapper
glDirect3D3Vtbl glDirect3D3_impl =
{
	glDirect3D3_QueryInterface,
	glDirect3D3_AddRef,
	glDirect3D3_Release,
	glDirect3D3_EnumDevices,
	glDirect3D3_CreateLight,
	glDirect3D3_CreateMaterial,
	glDirect3D3_CreateViewport,
	glDirect3D3_FindDevice,
	glDirect3D3_CreateDevice,
	glDirect3D3_CreateVertexBuffer,
	glDirect3D3_EnumZBufferFormats,
	glDirect3D3_EvictManagedTextures,
};

HRESULT glDirect3D3_Create(glDirect3D7 *glD3D7, glDirect3D3 **glD3D3)
{
	TRACE_ENTER(2, 14, glD3D7, 14, glD3D3);
	glDirect3D3 *This = (glDirect3D3*)malloc(sizeof(glDirect3D3));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3D3_impl;
	This->glD3D7 = glD3D7;
	*glD3D3 = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D3_QueryInterface(glDirect3D3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3D3))
	{
		glDirect3D3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3D7_QueryInterface(This->glD3D7, riid,ppvObj));
}

ULONG WINAPI glDirect3D3_AddRef(glDirect3D3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->AddRef1());
}

ULONG WINAPI glDirect3D3_Release(glDirect3D3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->Release1());
}

HRESULT WINAPI glDirect3D3_CreateDevice(glDirect3D3 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE4 lpDDS, LPDIRECT3DDEVICE3 *lplpD3DDevice, LPUNKNOWN pUnkOuter)
{
	TRACE_ENTER(5,14,This,24,&rclsid,14,lpDDS,14,lplpD3DDevice,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DDEVICE7 lpD3DDev7;
	HRESULT err = glDirect3D7_CreateDevice2(This->glD3D7, rclsid,((glDirectDrawSurface4*)lpDDS)->GetDDS7(),&lpD3DDev7,3);
	if(err == D3D_OK)
	{
		lpD3DDev7->QueryInterface(IID_IDirect3DDevice3,(LPVOID*)lplpD3DDevice);
		lpD3DDev7->Release();
		TRACE_VAR("*lplpD3DDevice",14,*lplpD3DDevice);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3D3_CreateLight(glDirect3D3 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpDirect3DLight,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_CreateLight(This->glD3D7, lplpDirect3DLight,pUnkOuter));
}
HRESULT WINAPI glDirect3D3_CreateMaterial(glDirect3D3 *This, LPDIRECT3DMATERIAL3* lplpDirect3DMaterial, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpDirect3DMaterial,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_CreateMaterial(This->glD3D7, lplpDirect3DMaterial,pUnkOuter));
}
HRESULT WINAPI glDirect3D3_CreateVertexBuffer(glDirect3D3 *This, LPD3DVERTEXBUFFERDESC lpVBDesc, LPDIRECT3DVERTEXBUFFER* lplpD3DVertexBuffer, DWORD dwFlags, LPUNKNOWN pUnkOuter)
{
	HRESULT ret;
	TRACE_ENTER(5,14,This,14,lpVBDesc,14,lplpD3DVertexBuffer,9,dwFlags,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if (!lplpD3DVertexBuffer) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!lpVBDesc) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ret = glDirect3DVertexBuffer1_Create(This, *lpVBDesc, dwFlags, (glDirect3DVertexBuffer7**)lplpD3DVertexBuffer);
	TRACE_VAR("*lplpD3DVertexBuffer", 14, *lplpD3DVertexBuffer);
	TRACE_EXIT(23, ret);
	return ret;
}
HRESULT WINAPI glDirect3D3_CreateViewport(glDirect3D3 *This, LPDIRECT3DVIEWPORT3* lplpD3DViewport, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpD3DViewport,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_CreateViewport(This->glD3D7, lplpD3DViewport,pUnkOuter));
}

HRESULT WINAPI glDirect3D3_EnumDevices(glDirect3D3 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	TRACE_ENTER(3,14,This,14,lpEnumDevicesCallback,14,lpUserArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_EnumDevices3(This->glD3D7, lpEnumDevicesCallback,lpUserArg));
}

HRESULT WINAPI glDirect3D3_EnumZBufferFormats(glDirect3D3 *This, REFCLSID riidDevice, LPD3DENUMPIXELFORMATSCALLBACK lpEnumCallback, LPVOID lpContext)
{
	TRACE_ENTER(4,14,This,24,&riidDevice,14,lpEnumCallback,14,lpContext);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_EnumZBufferFormats(This->glD3D7, riidDevice,lpEnumCallback,lpContext));
}
HRESULT WINAPI glDirect3D3_EvictManagedTextures(glDirect3D3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_EvictManagedTextures(This->glD3D7));
}
HRESULT WINAPI glDirect3D3_FindDevice(glDirect3D3 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	TRACE_ENTER(3,14,This,14,lpD3DFDS,14,lpD3DFDR);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_FindDevice(This->glD3D7, lpD3DFDS,lpD3DFDR));
}

glDirect3D2Vtbl glDirect3D2_impl =
{
	glDirect3D2_QueryInterface,
	glDirect3D2_AddRef,
	glDirect3D2_Release,
	glDirect3D2_EnumDevices,
	glDirect3D2_CreateLight,
	glDirect3D2_CreateMaterial,
	glDirect3D2_CreateViewport,
	glDirect3D2_FindDevice,
	glDirect3D2_CreateDevice
};

HRESULT glDirect3D2_Create(glDirect3D7 *glD3D7, glDirect3D2 **glD3D2)
{
	TRACE_ENTER(2, 14, glD3D7, 14, glD3D2);
	glDirect3D2 *This = (glDirect3D2*)malloc(sizeof(glDirect3D2));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3D2_impl;
	This->glD3D7 = glD3D7;
	*glD3D2 = This;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D2_QueryInterface(glDirect3D2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if ((riid == IID_IUnknown) || (riid == IID_IDirect3D2))
	{
		glDirect3D2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_ENTER(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT, 23, glDirect3D7_QueryInterface(This->glD3D7, riid, ppvObj));
}

ULONG WINAPI glDirect3D2_AddRef(glDirect3D2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->AddRef1());
}

ULONG WINAPI glDirect3D2_Release(glDirect3D2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->Release1());
}

HRESULT WINAPI glDirect3D2_CreateDevice(glDirect3D2 *This, REFCLSID rclsid, LPDIRECTDRAWSURFACE lpDDS, LPDIRECT3DDEVICE2 *lplpD3DDevice)
{
	TRACE_ENTER(4,14,This,24,&rclsid,14,lpDDS,14,lplpD3DDevice);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DDevice) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DDEVICE7 lpD3DDev7;
	HRESULT err = glDirect3D7_CreateDevice2(This->glD3D7, rclsid,((glDirectDrawSurface1*)lpDDS)->GetDDS7(),&lpD3DDev7,2);
	if(err == D3D_OK)
	{
		lpD3DDev7->QueryInterface(IID_IDirect3DDevice2,(LPVOID*)lplpD3DDevice);
		lpD3DDev7->Release();
		TRACE_VAR("*lplpD3DDevice",14,*lplpD3DDevice);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3D2_CreateLight(glDirect3D2 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpDirect3DLight,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_CreateLight(This->glD3D7, lplpDirect3DLight,pUnkOuter));
}

HRESULT WINAPI glDirect3D2_CreateMaterial(glDirect3D2 *This, LPDIRECT3DMATERIAL2* lplpDirect3DMaterial2, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpDirect3DMaterial2,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDirect3DMaterial2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DMaterial3 *glD3DM3;
	HRESULT error = glDirect3D7_CreateMaterial(This->glD3D7, (LPDIRECT3DMATERIAL3*)&glD3DM3,pUnkOuter);
	if(FAILED(error)) TRACE_RET(HRESULT,23,error);
	glDirect3DMaterial3_QueryInterface(glD3DM3,IID_IDirect3DMaterial2,(void**)lplpDirect3DMaterial2);
	glDirect3DMaterial3_Release(glD3DM3);
	TRACE_VAR("*lplpDirect3DMaterial2",14,*lplpDirect3DMaterial2);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D2_CreateViewport(glDirect3D2 *This, LPDIRECT3DVIEWPORT2* lplpD3DViewport2, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpD3DViewport2,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	HRESULT error = glDirect3D7_CreateViewport(This->glD3D7, (LPDIRECT3DVIEWPORT3*)&glD3DV3,pUnkOuter);
	if(FAILED(error)) TRACE_RET(HRESULT,23,error);
	glDirect3DViewport3_QueryInterface(glD3DV3,IID_IDirect3DViewport2,(void**)lplpD3DViewport2);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_VAR("*lplpD3DViewport2",14,*lplpD3DViewport2);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D2_EnumDevices(glDirect3D2 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	TRACE_ENTER(3,14,This,14,lpEnumDevicesCallback,14,lpUserArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_EnumDevices3(This->glD3D7, lpEnumDevicesCallback,lpUserArg));
}

HRESULT WINAPI glDirect3D2_FindDevice(glDirect3D2 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	TRACE_ENTER(3,14,This,14,lpD3DFDS,14,lpD3DFDR);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_FindDevice(This->glD3D7, lpD3DFDS,lpD3DFDR));
}

glDirect3D1Vtbl glDirect3D1_impl =
{
	glDirect3D1_QueryInterface,
	glDirect3D1_AddRef,
	glDirect3D1_Release,
	glDirect3D1_Initialize,
	glDirect3D1_EnumDevices,
	glDirect3D1_CreateLight,
	glDirect3D1_CreateMaterial,
	glDirect3D1_CreateViewport,
	glDirect3D1_FindDevice
};

HRESULT glDirect3D1_Create(glDirect3D7 *glD3D7, glDirect3D1 **glD3D1)
{
	TRACE_ENTER(2, 14, glD3D7, 14, glD3D1);
	glDirect3D1 *This = (glDirect3D1*)malloc(sizeof(glDirect3D1));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3D1_impl;
	This->glD3D7 = glD3D7;
	*glD3D1 = This;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D1_QueryInterface(glDirect3D1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if((riid == IID_IUnknown) || (riid == IID_IDirect3D))
	{
		glDirect3D1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3D7_QueryInterface(This->glD3D7, riid,ppvObj));
}

ULONG WINAPI glDirect3D1_AddRef(glDirect3D1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->AddRef1());
}

ULONG WINAPI glDirect3D1_Release(glDirect3D1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, This->glD3D7->glDD7->Release1());
}

HRESULT WINAPI glDirect3D1_CreateLight(glDirect3D1 *This, LPDIRECT3DLIGHT* lplpDirect3DLight, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_CreateLight(This->glD3D7, lplpDirect3DLight,pUnkOuter));
}

HRESULT WINAPI glDirect3D1_CreateMaterial(glDirect3D1 *This, LPDIRECT3DMATERIAL* lplpDirect3DMaterial, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpDirect3DMaterial,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpDirect3DMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DMaterial3 *glD3DM3;
	HRESULT error = glDirect3D7_CreateMaterial(This->glD3D7, (LPDIRECT3DMATERIAL3*)&glD3DM3, pUnkOuter);
	if(FAILED(error)) TRACE_RET(HRESULT,23,error);
	glDirect3DMaterial3_QueryInterface(glD3DM3,IID_IDirect3DMaterial,(void**)lplpDirect3DMaterial);
	glDirect3DMaterial3_Release(glD3DM3);
	TRACE_VAR("*lplpDirect3DMaterial",14,*lplpDirect3DMaterial);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D1_CreateViewport(glDirect3D1 *This, LPDIRECT3DVIEWPORT* lplpD3DViewport, IUnknown* pUnkOuter)
{
	TRACE_ENTER(3,14,This,14,lplpD3DViewport,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	HRESULT error = glDirect3D7_CreateViewport(This->glD3D7, (LPDIRECT3DVIEWPORT3*)&glD3DV3,pUnkOuter);
	if(FAILED(error)) TRACE_RET(HRESULT,23,error);
	glDirect3DViewport3_QueryInterface(glD3DV3,IID_IDirect3DViewport,(void**)lplpD3DViewport);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_VAR("*lplpD3DViewport",14,*lplpD3DViewport);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3D1_EnumDevices(glDirect3D1 *This, LPD3DENUMDEVICESCALLBACK lpEnumDevicesCallback, LPVOID lpUserArg)
{
	TRACE_ENTER(3,14,This,14,lpEnumDevicesCallback,14,lpUserArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_EnumDevices3(This->glD3D7, lpEnumDevicesCallback,lpUserArg));
}

HRESULT WINAPI glDirect3D1_FindDevice(glDirect3D1 *This, LPD3DFINDDEVICESEARCH lpD3DFDS, LPD3DFINDDEVICERESULT lpD3DFDR)
{
	TRACE_ENTER(3,14,This,14,lpD3DFDS,14,lpD3DFDR);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3D7_FindDevice(This->glD3D7, lpD3DFDS,lpD3DFDR));
}

HRESULT WINAPI glDirect3D1_Initialize(glDirect3D1 *This, REFIID lpREFIID)
{
	TRACE_ENTER(2,14,This,24,&lpREFIID);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
}
