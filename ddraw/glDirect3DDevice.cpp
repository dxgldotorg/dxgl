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
#include "ddraw.h"
#include "glDirect3D.h"
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DViewport.h"
#include "glDirect3DVertexBuffer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include "glDirect3DExecuteBuffer.h"
#include <string>
#include <cmath>
using namespace std;
#include "ShaderGen3D.h"
#include "matrix.h"

typedef struct _D3DDeviceDesc1 {
        DWORD           dwSize;
        DWORD           dwFlags;
        D3DCOLORMODEL   dcmColorModel;
        DWORD           dwDevCaps;
        D3DTRANSFORMCAPS dtcTransformCaps;
        BOOL            bClipping;
        D3DLIGHTINGCAPS dlcLightingCaps;
        D3DPRIMCAPS     dpcLineCaps;
        D3DPRIMCAPS     dpcTriCaps;
        DWORD           dwDeviceRenderBitDepth;
        DWORD           dwDeviceZBufferBitDepth;
        DWORD           dwMaxBufferSize;
        DWORD           dwMaxVertexCount;
} D3DDEVICEDESC1,*LPD3DDEVICEDESC1;

typedef struct _D3DDeviceDesc2 {
        DWORD           dwSize;
        DWORD           dwFlags;
        D3DCOLORMODEL   dcmColorModel;
        DWORD           dwDevCaps;
        D3DTRANSFORMCAPS dtcTransformCaps;
        BOOL            bClipping;
        D3DLIGHTINGCAPS dlcLightingCaps;
        D3DPRIMCAPS     dpcLineCaps;
        D3DPRIMCAPS     dpcTriCaps;
        DWORD           dwDeviceRenderBitDepth;
        DWORD           dwDeviceZBufferBitDepth;
        DWORD           dwMaxBufferSize;
        DWORD           dwMaxVertexCount;

        DWORD           dwMinTextureWidth,dwMinTextureHeight;
        DWORD           dwMaxTextureWidth,dwMaxTextureHeight;
        DWORD           dwMinStippleWidth,dwMaxStippleWidth;
        DWORD           dwMinStippleHeight,dwMaxStippleHeight;
} D3DDEVICEDESC2,*LPD3DDEVICEDESC2;

const DWORD renderstate_default[RENDERSTATE_COUNT] = {0, // 0
	NULL, //texturehandle
	D3DANTIALIAS_NONE, //antialias
	D3DTADDRESS_WRAP, //textureaddress
	TRUE, //textureperspective
	FALSE, //wrapu
	FALSE, //wrapv
	D3DZB_FALSE, //zenable
	D3DFILL_SOLID, //fillmode
	D3DSHADE_GOURAUD, //shademode
	0, //linepattern                                         10
	FALSE, //monoenable
	R2_COPYPEN, //rop2
	0xFFFFFFFF, //planemask
	TRUE, //zwriteenable
	FALSE, //alphastateenable
	TRUE, //lastpixel
	D3DFILTER_NEAREST, //texturemag
	D3DFILTER_NEAREST, //texturemin
	D3DBLEND_ONE, //srcblend
	D3DBLEND_ZERO, //destblend                               20
	0, //texturemapblend
	D3DCULL_CCW, //cullmode
	D3DCMP_LESSEQUAL, //zfunc
	0, //alpharef
	D3DCMP_ALWAYS, //alphafunc
	FALSE, //ditherenable
	FALSE, //alphablendenable
	FALSE, //fogenable
	FALSE, //specularenable
	FALSE, //zvisible                                        30
	FALSE, //subpixel
	FALSE, //subpixelx
	FALSE, //stippledalpha
	0, //fogcolor
	D3DFOG_NONE, //fogtablemode
	0, //fogstart = 0
	0x3f800000, //fogend = 0.0078125
	0x3f800000, //fogdensity = 0.0078125
	FALSE, //stippleenable
	FALSE, //edgeantialias                                   40
	FALSE, //colorkeyenable
	FALSE, // old blendenable
	0, //bordercolor
	D3DTADDRESS_WRAP, //textureaddressu
	D3DTADDRESS_WRAP, //textureaddressv
	0, //mipmaplodbias = 0
	0, //zbias
	FALSE, //rangefogenable
	1, //anisotropy
	0, //flushbatch                                          50
	FALSE, //translucentsortindependent
	FALSE, //stencilenable
	D3DSTENCILOP_KEEP, //stencilfail
	D3DSTENCILOP_KEEP, //stencilzfail
	D3DSTENCILOP_KEEP, //stencilpass
	D3DCMP_ALWAYS, //stencilfunc
	0,  //stencilref
	0xFFFFFFFF, //stencilmask
	0xFFFFFFFF, //stencilwritemask
	0xFFFFFFFF, //texturefactor                              60
	0,0,0,0,0,0,0,0,0,0,                                  // 70
	0,0,0,0,0,0,0,0,0,0,                                  // 80
	0,0,0,0,0,0,0,0,0,0,                                  // 90
	0,0,0,0,0,0,0,0,0,0,                                  //100
	0,0,0,0,0,0,0,0,0,0,                                  //110
	0,0,0,0,0,0,0,0,0,0,                                  //120
	0,0,0,0,0,0,0, // 127
	0, //wrap0
	0, //wrap1
	0, //wrap2                                              130
	0, //wrap3
	0, //wrap4
	0, //wrap5
	0, //wrap6
	0, //wrap7
	TRUE, //clipping
	TRUE, //lighting
	FALSE, //extents
	0, //ambient
	D3DFOG_NONE, //fogvertexmode                            140
	TRUE, //colorvertex
	TRUE, //localviewer
	FALSE, //normalizenormals
	0, //colorblendkeyenable
	D3DMCS_COLOR1, //diffusematerialsource
	D3DMCS_COLOR2, //specularmaterialsource
	D3DMCS_MATERIAL, //ambientmaterialsource
	D3DMCS_MATERIAL, //emissivematerialsource
	0,0,                                                  //150
	D3DVBLEND_DISABLE, //vertexblend
	FALSE, //clipplaneenable
};

const TEXTURESTAGE texstagedefault0 =
{
	D3DTOP_MODULATE,
	D3DTA_TEXTURE,
	D3DTA_CURRENT,
	D3DTOP_SELECTARG1,
	D3DTA_TEXTURE,
	D3DTA_CURRENT,
	0,0,0,0,
	0,
	D3DTADDRESS_WRAP,
	D3DTADDRESS_WRAP,
	0,
	D3DTFG_POINT,
	D3DTFN_POINT,
	D3DTFP_NONE,
	0,
	0,
	1,
	0,
	0,
	D3DTTFF_DISABLE,
	NULL,
	GL_NEAREST,
	GL_NEAREST
};
const TEXTURESTAGE texstagedefault1 =
{
	D3DTOP_DISABLE,
	D3DTA_TEXTURE,
	D3DTA_CURRENT,
	D3DTOP_DISABLE,
	D3DTA_TEXTURE,
	D3DTA_CURRENT,
	0,0,0,0,
	0,
	D3DTADDRESS_WRAP,
	D3DTADDRESS_WRAP,
	0,
	D3DTFG_POINT,
	D3DTFN_POINT,
	D3DTFP_NONE,
	0,
	0,
	1,
	0,
	0,
	D3DTTFF_DISABLE,
	NULL,
	GL_NEAREST,
	GL_NEAREST
};

int setdrawmode(D3DPRIMITIVETYPE d3dptPrimitiveType)
{
	switch(d3dptPrimitiveType)
	{
	case D3DPT_POINTLIST:
		return GL_POINTS;
	case D3DPT_LINELIST:
		return GL_LINES;
	case D3DPT_LINESTRIP:
		return GL_LINE_STRIP;
	case D3DPT_TRIANGLELIST:
		return GL_TRIANGLES;
	case D3DPT_TRIANGLESTRIP:
		return GL_TRIANGLE_STRIP;
	case D3DPT_TRIANGLEFAN:
		return GL_TRIANGLE_FAN;
	default:
		return -1;
	}
}

void AddStats(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwCount, D3DSTATS *stats)
{
	switch(d3dptPrimitiveType)
	{
	case D3DPT_POINTLIST:
		stats->dwPointsDrawn += dwCount;
		break;
	case D3DPT_LINELIST:
		stats->dwLinesDrawn += dwCount / 2;
		break;
	case D3DPT_LINESTRIP:
		if(dwCount > 1) stats->dwLinesDrawn += dwCount - 1;
		break;
	case D3DPT_TRIANGLELIST:
		stats->dwTrianglesDrawn += dwCount / 3;
		break;
	case D3DPT_TRIANGLESTRIP:
	case D3DPT_TRIANGLEFAN:
		if(dwCount > 2) stats->dwTrianglesDrawn += dwCount - 2;
		break;
	default:
		break;
	}
}

// IDirect3DDevice7 wrapper
glDirect3DDevice7Vtbl glDirect3DDevice7_iface =
{
	glDirect3DDevice7_QueryInterface,
	glDirect3DDevice7_AddRef,
	glDirect3DDevice7_Release,
	glDirect3DDevice7_GetCaps,
	glDirect3DDevice7_EnumTextureFormats,
	glDirect3DDevice7_BeginScene,
	glDirect3DDevice7_EndScene,
	glDirect3DDevice7_GetDirect3D,
	glDirect3DDevice7_SetRenderTarget,
	glDirect3DDevice7_GetRenderTarget,
	glDirect3DDevice7_Clear,
	glDirect3DDevice7_SetTransform,
	glDirect3DDevice7_GetTransform,
	glDirect3DDevice7_SetViewport,
	glDirect3DDevice7_MultiplyTransform,
	glDirect3DDevice7_GetViewport,
	glDirect3DDevice7_SetMaterial,
	glDirect3DDevice7_GetMaterial,
	glDirect3DDevice7_SetLight,
	glDirect3DDevice7_GetLight,
	glDirect3DDevice7_SetRenderState,
	glDirect3DDevice7_GetRenderState,
	glDirect3DDevice7_BeginStateBlock,
	glDirect3DDevice7_EndStateBlock,
	glDirect3DDevice7_PreLoad,
	glDirect3DDevice7_DrawPrimitive,
	glDirect3DDevice7_DrawIndexedPrimitive,
	glDirect3DDevice7_SetClipStatus,
	glDirect3DDevice7_GetClipStatus,
	glDirect3DDevice7_DrawPrimitiveStrided,
	glDirect3DDevice7_DrawIndexedPrimitiveStrided,
	glDirect3DDevice7_DrawPrimitiveVB,
	glDirect3DDevice7_DrawIndexedPrimitiveVB,
	glDirect3DDevice7_ComputeSphereVisibility,
	glDirect3DDevice7_GetTexture,
	glDirect3DDevice7_SetTexture,
	glDirect3DDevice7_GetTextureStageState,
	glDirect3DDevice7_SetTextureStageState,
	glDirect3DDevice7_ValidateDevice,
	glDirect3DDevice7_ApplyStateBlock,
	glDirect3DDevice7_CaptureStateBlock,
	glDirect3DDevice7_DeleteStateBlock,
	glDirect3DDevice7_CreateStateBlock,
	glDirect3DDevice7_Load,
	glDirect3DDevice7_LightEnable,
	glDirect3DDevice7_GetLightEnable,
	glDirect3DDevice7_SetClipPlane,
	glDirect3DDevice7_GetClipPlane,
	glDirect3DDevice7_GetInfo,
};

HRESULT glDirect3DDevice7_Create(REFCLSID rclsid, glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7,
	IUnknown *creator, int version, glDirect3DDevice7 **newdev)
{
	TRACE_ENTER(6,24,&rclsid,14,glD3D7,14,glDDS7,14,creator,11,version,14,newdev);
	glDirect3DDevice7 *This = (glDirect3DDevice7*)malloc(sizeof(glDirect3DDevice7));
	if (!This) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	This->lpVtbl = &glDirect3DDevice7_iface;
	*newdev = This;
	This->version = version;
	This->dx2init = This->dx5init = FALSE;
	This->d3ddesc = d3ddesc_default;
	This->d3ddesc3 = d3ddesc3_default;
	int zbuffer = 0;
	This->maxmaterials = 32;
	This->materials = (glDirect3DMaterial3**)malloc(32*sizeof(glDirect3DMaterial3*));
	if(!This->materials)
	{
		free(This);
		*newdev = NULL;
		TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	}
	This->materialcount = 1;
	This->materials[0] = NULL;
	This->currentmaterial = NULL;
	This->maxtextures = 32;
	This->textures = (glDirectDrawSurface7**)malloc(32*sizeof(glDirectDrawSurface7*));
	if(!This->textures)
	{
		free(This->materials);
		free(This);
		*newdev = NULL;
		TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	}
	This->textures[0] = NULL;
	This->texturecount = 1;
	This->maxviewports = 32;
	This->currentviewport = NULL;
	This->viewportcount = 0;
	This->viewports = (glDirect3DViewport3**)malloc(32*sizeof(glDirect3DViewport3*));
	if(!This->viewports)
	{
		free(This->materials);
		free(This->textures);
		free(This);
		*newdev = NULL;
		TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	}
	ZeroMemory(This->viewports,32*sizeof(glDirect3DViewport3*));
	This->vertices = This->normals = NULL;
	This->diffuse = This->specular = NULL;
	This->ebBuffer = NULL;
	This->ebBufferSize = 0;
	This->outbuffer = NULL;
	This->outbuffersize = 0;
	ZeroMemory(This->texcoords,8*sizeof(GLfloat*));
	memcpy(This->renderstate,renderstate_default,RENDERSTATE_COUNT*sizeof(DWORD));
	__gluMakeIdentityf(This->matWorld);
	__gluMakeIdentityf(This->matView);
	__gluMakeIdentityf(This->matProjection);
	This->transform_dirty = true;
	This->matrices = NULL;
	This->matrixcount = 0;
	This->texstages[0] = texstagedefault0;
	This->texstages[1] = This->texstages[2] = This->texstages[3] = This->texstages[4] =
		This->texstages[5] = This->texstages[6] = This->texstages[7] = texstagedefault1;
	This->refcount = 1;
	This->inscene = false;
	This->modelview_dirty = false;
	This->projection_dirty = false;
	This->glD3D7 = glD3D7;
	glD3D7->AddRef();
	This->glDDS7 = glDDS7;
	if(!creator) glDDS7->AddRef();
	This->renderer = This->glD3D7->glDD7->renderer;
	ZeroMemory(&This->viewport,sizeof(D3DVIEWPORT7));
	if(glDDS7->GetZBuffer()) zbuffer = 1;
	ZeroMemory(&This->material,sizeof(D3DMATERIAL7));
	This->lightsmax = 16;
	This->lights = (glDirect3DLight**) malloc(16*sizeof(glDirect3DLight*));
	if(!This->lights)
	{
		free(This->materials);
		free(This->textures);
		free(This->viewports);
		free(This);
		*newdev = NULL;
		TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	}
	ZeroMemory(This->lights,16*sizeof(glDirect3DLight*));
	memset(This->gllights,0xff,8*sizeof(int));
	memset(This->gltextures,0,8*sizeof(GLuint));
	ZeroMemory(&This->stats,sizeof(D3DSTATS));
	This->stats.dwSize = sizeof(D3DSTATS);
	This->d3ddesc.dwMaxTextureWidth = This->d3ddesc.dwMaxTextureHeight =
		This->d3ddesc.dwMaxTextureRepeat = This->d3ddesc.dwMaxTextureAspectRatio = This->renderer->gl_caps.TextureMax;
	This->d3ddesc3.dwMaxTextureWidth = This->d3ddesc3.dwMaxTextureHeight =
		This->d3ddesc3.dwMaxTextureRepeat = This->d3ddesc3.dwMaxTextureAspectRatio = This->renderer->gl_caps.TextureMax;
	This->scalex = This->scaley = 0;
	This->mhWorld = This->mhView = This->mhProjection = 0;
	glRenderer_InitD3D(This->renderer,zbuffer,glDDS7->ddsd.dwWidth,glDDS7->ddsd.dwHeight);
	glDirect3DDevice3_Create(This, &This->glD3DDev3);
	glDirect3DDevice2_Create(This, &This->glD3DDev2);
	glDirect3DDevice1_Create(This, &This->glD3DDev1);
	This->creator = creator;
	This->error = D3D_OK;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

void glDirect3DDevice7_Destroy(glDirect3DDevice7 *This)
{
	DWORD i;
	TRACE_ENTER(1,14,This);
	for(i = 0; i < This->lightsmax; i++)
		if(This->lights[i]) delete This->lights[i];
	free(This->lights);
	for(i = 0; i < 8; i++)
		if(This->texstages[i].surface) This->texstages[i].surface->Release();
	for(i = 0; i < This->materialcount; i++)
	{
		if(This->materials[i])
		{
			glDirect3DMaterial3_unbind(This->materials[i]);
			glDirect3DMaterial3_Release(This->materials[i]);
		}
	}
	for(int i = 0; i < This->viewportcount; i++)
	{
		if(This->viewports[i])
		{
			glDirect3DViewport3_SetDevice(This->viewports[i], NULL);
			glDirect3DViewport3_SetCurrent(This->viewports[i], false);
			glDirect3DViewport3_Release(This->viewports[i]);
		}
	}
	for(i = 0; i < This->texturecount; i++)
	{
		if(This->textures[i])
		{
			This->textures[i]->Release();
		}
	}
	free(This->viewports);
	free(This->materials);
	free(This->textures);
	if(This->matrices) free(This->matrices);
	if (This->glD3DDev3) free(This->glD3DDev3);
	if (This->glD3DDev2) free(This->glD3DDev2);
	if (This->glD3DDev1) free(This->glD3DDev1);
	This->glD3D7->Release();
	if(!This->creator) This->glDDS7->Release();
	free(This);
	TRACE_EXIT(0,0);
}

int ExpandLightBuffer(glDirect3DLight ***lights, DWORD *maxlights, DWORD newmax)
{
	if(newmax < *maxlights) return 1;
	glDirect3DLight **tmp = (glDirect3DLight**)realloc(*lights,newmax*sizeof(glDirect3DLight*));
	if(!tmp) return 0;
	*lights = tmp;
	for(DWORD i = *maxlights; i < newmax; i++)
		(*lights)[i] = NULL;
	*maxlights = newmax;
	return 1;
}

HRESULT WINAPI glDirect3DDevice7_QueryInterface(glDirect3DDevice7 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		glDirect3DDevice7_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if (This->creator) TRACE_RET(HRESULT, 23, This->creator->QueryInterface(riid, ppvObj));
	if (This->version == 7)
	{
		if (riid == IID_IDirect3DDevice7)
		{
			glDirect3DDevice7_AddRef(This);
			*ppvObj = This;
			TRACE_VAR("*ppvObj", 14, *ppvObj);
			TRACE_EXIT(23, D3D_OK);
			return D3D_OK;
		}
	}
	else
	{
		if ((riid == IID_IDirect3DDevice3) && (This->version >= 3))
		{
			*ppvObj = This->glD3DDev3;
			glDirect3DDevice3_AddRef(This->glD3DDev3);
			TRACE_VAR("*ppvObj", 14, *ppvObj);
			TRACE_EXIT(23, D3D_OK);
			return D3D_OK;
		}
		if ((riid == IID_IDirect3DDevice2) && (This->version >= 2))
		{
			*ppvObj = This->glD3DDev2;
			glDirect3DDevice2_AddRef(This->glD3DDev2);
			if (!This->dx5init)
			{
				glDirect3DDevice7_InitDX5(This);
				This->dx5init = TRUE;
			}
			TRACE_VAR("*ppvObj", 14, *ppvObj);
			TRACE_EXIT(23, D3D_OK);
			return D3D_OK;
		}
		if (riid == IID_IDirect3DDevice)
		{
			*ppvObj = This->glD3DDev1;
			glDirect3DDevice1_AddRef(This->glD3DDev1);
			if (!This->dx5init)
			{
				glDirect3DDevice7_InitDX5(This);
				This->dx5init = TRUE;
			}
			if (!This->dx2init)
			{
				glDirect3DDevice7_InitDX2(This);
				This->dx5init = TRUE;
			}
			TRACE_VAR("*ppvObj", 14, *ppvObj);
			TRACE_EXIT(23, D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DDevice7_AddRef(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	if (This->creator) TRACE_RET(ULONG, 8, This->creator->AddRef());
	TRACE_RET(ULONG, 8, glDirect3DDevice7_AddRefInternal(This));
}

ULONG WINAPI glDirect3DDevice7_Release(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1, 14, This);
	if (This) TRACE_RET(ULONG, 8, 0);
	if (This->creator) TRACE_RET(ULONG, 8, This->creator->Release());
	TRACE_RET(ULONG, 8, glDirect3DDevice7_ReleaseInternal(This));
}

ULONG glDirect3DDevice7_AddRefInternal(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	InterlockedIncrement(&This->refcount);
	TRACE_EXIT(8,This->refcount);
	return This->refcount;
}
ULONG glDirect3DDevice7_ReleaseInternal(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	ULONG ret;
	InterlockedDecrement(&This->refcount);
	ret = This->refcount;
	if (This->refcount == 0) glDirect3DDevice7_Destroy(This);
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice7_ApplyStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,This,9,dwBlockHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_ApplyStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_BeginScene(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(This->inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_IN_SCENE);
	This->inscene = true;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_BeginStateBlock(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_BeginStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_CaptureStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,This,9,dwBlockHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_CaptureStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_CreateStateBlock(glDirect3DDevice7 *This, D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle)
{
	TRACE_ENTER(3,14,This,9,d3dsbtype,14,lpdwBlockHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::CreateStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_Clear(glDirect3DDevice7 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	ClearCommand cmd;
	TRACE_ENTER(7,14,This,8,dwCount,14,lpRects,9,dwFlags,9,dwColor,19,&dvZ,9,dwStencil);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwCount && !lpRects) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	cmd.dwCount = dwCount;
	cmd.lpRects = lpRects;
	cmd.dwFlags = dwFlags;
	cmd.dwColor = dwColor;
	cmd.dvZ = dvZ;
	cmd.dwStencil = dwStencil;
	cmd.target = This->glDDS7->texture;
	cmd.targetlevel = This->glDDS7->miplevel;
	if (This->glDDS7->zbuffer)
	{
		cmd.zbuffer = This->glDDS7->zbuffer->texture;
		cmd.zlevel = This->glDDS7->zbuffer->miplevel;
	}
	else
	{
		cmd.zbuffer = NULL;
		cmd.zlevel = 0;
	}
	TRACE_RET(HRESULT,23,glRenderer_Clear(This->renderer,&cmd));
}

// ComputeSphereVisibility based on modified code from the Wine project, subject
// to the following license terms:
/*
* Copyright (c) 1998-2004 Lionel Ulmer
* Copyright (c) 2002-2005 Christian Costa
* Copyright (c) 2006-2009, 2011-2013 Stefan Dösinger
* Copyright (c) 2008 Alexander Dorofeyev
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/
static DWORD in_plane(UINT plane, D3DVECTOR normal, D3DVALUE origin_plane, D3DVECTOR center, D3DVALUE radius)
{
	float distance, norm;

	norm = sqrtf(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
	distance = (origin_plane + normal.x * center.x + normal.y * center.y + normal.z * center.z) / norm;

	if (fabs(distance) < radius) return D3DSTATUS_CLIPUNIONLEFT << plane;
	if (distance < -radius) return (D3DSTATUS_CLIPUNIONLEFT | D3DSTATUS_CLIPINTERSECTIONLEFT) << plane;
	return 0;
}


HRESULT WINAPI glDirect3DDevice7_ComputeSphereVisibility(glDirect3DDevice7 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	D3DMATRIX m, temp;
	D3DVALUE origin_plane[6];
	D3DVECTOR vec[6];
	HRESULT hr;
	UINT i, j;

	TRACE_ENTER(6,14,This,14,lpCenters,14,lpRadii,8,dwNumSpheres,9,dwFlags,14,lpdwReturnValues);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	hr = glDirect3DDevice7_GetTransform(This, D3DTRANSFORMSTATE_WORLD, &m);
	if (hr != DD_OK) return DDERR_INVALIDPARAMS;
	hr = glDirect3DDevice7_GetTransform(This, D3DTRANSFORMSTATE_VIEW, &temp);
	if (hr != DD_OK) return DDERR_INVALIDPARAMS;
	multiply_matrix((wined3d_matrix*)&m, (wined3d_matrix*)&temp, (wined3d_matrix*)&m);

	hr = glDirect3DDevice7_GetTransform(This, D3DTRANSFORMSTATE_PROJECTION, &temp);
	if (hr != DD_OK) return DDERR_INVALIDPARAMS;
	multiply_matrix((wined3d_matrix*)&m, (wined3d_matrix*)&temp, (wined3d_matrix*)&m);

	/* Left plane */
	vec[0].x = m._14 + m._11;
	vec[0].y = m._24 + m._21;
	vec[0].z = m._34 + m._31;
	origin_plane[0] = m._44 + m._41;

	/* Right plane */
	vec[1].z = m._14 - m._11;
	vec[1].y = m._24 - m._21;
	vec[1].z = m._34 - m._31;
	origin_plane[1] = m._44 - m._41;

	/* Top plane */
	vec[2].x = m._14 - m._12;
	vec[2].y = m._24 - m._22;
	vec[2].z = m._34 - m._32;
	origin_plane[2] = m._44 - m._42;

	/* Bottom plane */
	vec[3].x = m._14 + m._12;
	vec[3].y = m._24 + m._22;
	vec[3].z = m._34 + m._32;
	origin_plane[3] = m._44 + m._42;

	/* Front plane */
	vec[4].x = m._13;
	vec[4].y = m._23;
	vec[4].z = m._33;
	origin_plane[4] = m._43;

	/* Back plane*/
	vec[5].x = m._14 - m._13;
	vec[5].y = m._24 - m._23;
	vec[5].z = m._34 - m._33;
	origin_plane[5] = m._44 - m._43;

	for (i = 0; i < dwNumSpheres; ++i)
	{
		lpdwReturnValues[i] = 0;
		for (j = 0; j < 6; ++j)
			lpdwReturnValues[i] |= in_plane(j, vec[j], origin_plane[j], lpCenters[i], lpRadii[i]);
	}
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_DeleteStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,This,9,dwBlockHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_DeleteStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

void glDirect3DDevice7_SetArraySize(glDirect3DDevice7 *This, DWORD size, DWORD vertex, DWORD texcoord)
{
	TRACE_ENTER(4,14,This,8,size,8,vertex,8,texcoord);
	if(!This->vertices) This->vertices = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > This->maxarray) This->vertices = (GLfloat*)realloc(This->vertices,size*4*sizeof(GLfloat));
	if(!This->normals) This->normals = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > This->maxarray) This->normals = (GLfloat*)realloc(This->normals,size*4*sizeof(GLfloat));
	TRACE_EXIT(0,0);
}

/*__int64 glDirect3DDevice7_SelectShader(glDirect3DDevice7 *This, GLVERTEX *VertexType)
{
	TRACE_ENTER(2,14,This,14,VertexType);
	int i;
	__int64 shader = 0;
	switch(This->renderstate[D3DRENDERSTATE_SHADEMODE])
	{
	case D3DSHADE_FLAT:
	default:
		break;
	case D3DSHADE_GOURAUD:
		shader |= 1;
		break;
	case D3DSHADE_PHONG:
		shader |= 3;
		break;
	}
	if(This->renderstate[D3DRENDERSTATE_ALPHATESTENABLE]) shader |= 4;
	shader |= ((((__int64)This->renderstate[D3DRENDERSTATE_ALPHAFUNC]-1) & 7) << 3);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_FOGTABLEMODE] & 3) << 6);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_FOGVERTEXMODE] & 3) << 8);
	if(This->renderstate[D3DRENDERSTATE_RANGEFOGENABLE]) shader |= (1i64 << 10);
	if(This->renderstate[D3DRENDERSTATE_SPECULARENABLE]) shader |= (1i64 << 11);
	if(This->renderstate[D3DRENDERSTATE_STIPPLEDALPHA]) shader |= (1i64 << 12);
	if(This->renderstate[D3DRENDERSTATE_COLORKEYENABLE]) shader |= (1i64 << 13);
	//shader |= (((__int64)This->renderstate[D3DRENDERSTATE_ZBIAS] & 15) << 14);
	int numlights = 0;
	for(i = 0; i < 8; i++)
		if(This->gllights[i] != -1) numlights++;
	shader |= (__int64)numlights << 18;
	if(This->renderstate[D3DRENDERSTATE_LOCALVIEWER]) shader |= (1i64 << 21);
	if(This->renderstate[D3DRENDERSTATE_COLORKEYBLENDENABLE]) shader |= (1i64 << 22);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] & 3) << 23);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_SPECULARMATERIALSOURCE] & 3) << 25);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] & 3) << 27);
	shader |= (((__int64)This->renderstate[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] & 3) << 29);
	int numtextures = 0;
	for(i = 0; i < 8; i++)
		if(VertexType[i+10].data) numtextures++;
	shader |= (__int64)numtextures << 31;
	if(VertexType[8].data) shader |= (1i64<<35);
	if(VertexType[9].data) shader |= (1i64<<36);
	if(VertexType[7].data) shader |= (1i64 << 37);
	int lightindex = 0;
	for(i = 0; i < 8; i++)
	{
		if(This->gllights[i] != -1)
		{
			if(This->lights[This->gllights[i]]->light.dltType != D3DLIGHT_DIRECTIONAL)
				shader |= (1i64 << (38+lightindex));
			if(This->lights[This->gllights[i]]->light.dltType == D3DLIGHT_SPOT)
				shader |= (1i64 << (51+lightindex));
			lightindex++;
		}
	}
	int blendweights = 0;
	for(i = 0; i < 5; i++)
		if(VertexType[i+2].data) blendweights++;
	shader |= (__int64)blendweights << 46;
	if(This->renderstate[D3DRENDERSTATE_NORMALIZENORMALS]) shader |= (1i64 << 49);
	if(VertexType[1].data) shader |= (1i64 << 50);
	if(This->renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] == D3DTBLEND_MODULATE)
	{
		bool noalpha = false;
		if(!This->texstages[0].surface) noalpha = true;
		if(This->texstages[0].surface)
			if(!(This->texstages[0].surface->ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
				noalpha = true;
		if(noalpha) This->texstages[0].alphaop = D3DTOP_SELECTARG2;
		else This->texstages[0].alphaop = D3DTOP_MODULATE;
	}
	if(This->renderstate[D3DRENDERSTATE_LIGHTING]) shader |= (1i64 << 59);
	if(This->renderstate[D3DRENDERSTATE_COLORVERTEX]) shader |= (1i64 << 60);
	if(This->renderstate[D3DRENDERSTATE_FOGENABLE]) shader |= (1i64 << 61);
	if(This->renderstate[D3DRENDERSTATE_DITHERENABLE]) shader |= (1i64 << 62);
	for(i = 0; i < 8; i++)
	{
		if(!This->texstages[i].dirty) continue;
		This->texstages[i].shaderid = This->texstages[i].colorop & 31;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].colorarg1 & 63) << 5;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].colorarg2 & 63) << 11;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].alphaop & 31) << 17;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].alphaarg1 & 63) << 22;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].alphaarg2 & 63) << 28;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].texcoordindex & 7) << 34;
		This->texstages[i].shaderid |= (__int64)((This->texstages[i].texcoordindex >> 16) & 3) << 37;
		This->texstages[i].shaderid |= (__int64)((This->texstages[i].addressu - 1) & 3) << 39;
		This->texstages[i].shaderid |= (__int64)((This->texstages[i].addressv - 1) & 3) << 41;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].magfilter & 7) << 43;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].minfilter & 3) << 46;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].mipfilter & 3) << 48;
		if(This->texstages[i].textransform & 7)
		{
			This->texstages[i].shaderid |= 1i64 << 50;
			This->texstages[i].shaderid |= (__int64)(((This->texstages[i].textransform & 7) - 1)& 3) << 51;
		}
		if(This->texstages[i].textransform & D3DTTFF_PROJECTED) This->texstages[i].shaderid |= 1i64 << 53;
		This->texstages[i].shaderid |= (__int64)(This->texstages[i].texcoordindex&7) << 54;
		This->texstages[i].shaderid |= (__int64)((This->texstages[i].texcoordindex>>16)&3) << 57;
		if(This->texstages[i].surface)
		{
			This->texstages[i].shaderid |= 1i64 << 59;
			if(This->texstages[i].surface->ddsd.dwFlags & DDSD_CKSRCBLT) This->texstages[i].shaderid |= 1i64 << 60;
		}
	}
	TRACE_EXIT(10,&shader);
	return shader;
}*/

HRESULT glDirect3DDevice7_fvftoglvertex(glDirect3DDevice7 *This, DWORD dwVertexTypeDesc,LPDWORD vertptr)
{
	TRACE_ENTER(3,14,This,9,dwVertexTypeDesc,14,vertptr);
	int i;
	int texformats1[8];
	int ptr = 0;
	if((dwVertexTypeDesc & D3DFVF_XYZ) && (dwVertexTypeDesc & D3DFVF_XYZRHW))
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwVertexTypeDesc & D3DFVF_XYZ)
	{
		This->vertdata[0].data = vertptr;
		This->vertdata[1].data = NULL;
		ptr += 3;
		if(dwVertexTypeDesc & D3DFVF_RESERVED1) ptr++;
	}
	else if(dwVertexTypeDesc & D3DFVF_XYZRHW)
	{
		This->vertdata[0].data = vertptr;
		This->vertdata[1].data = &vertptr[3];
		ptr += 4;
	}
	else TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(i = 0; i < 5; i++)
		This->vertdata[i+2].data = NULL;
	if(((dwVertexTypeDesc >> 1) & 7) >= 3)
	{
		for(i = 0; i < (signed)(((dwVertexTypeDesc >> 1) & 7) - 2); i++)
		{
			This->vertdata[((dwVertexTypeDesc >> 1) & 7)].data = &vertptr[ptr];
			ptr++;
		}
	}
	if(dwVertexTypeDesc & D3DFVF_NORMAL)
	{
		This->vertdata[7].data = &vertptr[ptr];
		ptr += 3;
	}
	else This->vertdata[7].data = NULL;
	if(dwVertexTypeDesc & D3DFVF_DIFFUSE)
	{
		This->vertdata[8].data = &vertptr[ptr];
		ptr++;
	}
	else This->vertdata[8].data = NULL;
	if(dwVertexTypeDesc & D3DFVF_SPECULAR)
	{
		This->vertdata[9].data = &vertptr[ptr];
		ptr++;
	}
	else This->vertdata[9].data = NULL;
	for(i = 0; i < 8; i++)
		This->vertdata[i+10].data = NULL;
	int numtex = (dwVertexTypeDesc&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT;
	for(i = 0; i < 8; i++)
	{
		This->vertdata[i+10].data = &vertptr[ptr];
		if(i >= numtex) texformats1[i] = -1;
		else texformats1[i] = (dwVertexTypeDesc>>(16+(2*i))&3);
		switch(texformats1[i])
		{
		case 0: // st
			ptr += 2;
			This->texformats[i] = 2;
			break;
		case 1: // str
			ptr += 3;
			This->texformats[i] = 3;
			break;
		case 2: // strq
			ptr += 4;
			This->texformats[i] = 4;
			break;
		case 3: // s
			ptr++;
			This->texformats[i] = 1;
			break;
		default:
			This->texformats[i] = 2;
			break;
		}
	}
	int stride = ptr*4;
	for(i = 0; i < 17; i++)
		This->vertdata[i].stride = stride;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitive(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	RenderTarget target;
	TRACE_ENTER(8,9,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_NOT_IN_SCENE);
	HRESULT err = glDirect3DDevice7_fvftoglvertex(This,dwVertexTypeDesc,(LPDWORD)lpvVertices);
	if(lpwIndices) AddStats(d3dptPrimitiveType,dwIndexCount,&This->stats);
	else AddStats(d3dptPrimitiveType,dwVertexCount,&This->stats);
	if(err != D3D_OK) TRACE_RET(HRESULT,23,err);
	target.target = This->glDDS7->texture;
	target.level = This->glDDS7->miplevel;
	target.mulx = This->glDDS7->mulx;
	target.muly = This->glDDS7->muly;
	if (This->glDDS7->zbuffer)
	{
		target.zbuffer = This->glDDS7->zbuffer->texture;
		target.zlevel = This->glDDS7->zbuffer->miplevel;
	}
	else
	{
		target.zbuffer = NULL;
		target.zlevel = 0;
	}
	TRACE_RET(HRESULT,23,glRenderer_DrawPrimitives(This->renderer,&target,setdrawmode(d3dptPrimitiveType), This->vertdata, This->texformats,
		dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}
HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitiveStrided(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,9,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertexArray,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_DrawIndexedPrimitiveStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitiveVB(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,9,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_DrawIndexedPrimitiveVB: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_DrawPrimitive(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,9,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertices,8,dwVertexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23, glDirect3DDevice7_DrawIndexedPrimitive(This, 
		dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,NULL,0,dwFlags));
}
HRESULT WINAPI glDirect3DDevice7_DrawPrimitiveStrided(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,9,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_DrawPrimitiveStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_DrawPrimitiveVB(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(5,14,This,9,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_DrawPrimitiveVB: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_EndScene(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!This->inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_NOT_IN_SCENE);
	This->inscene = false;
	glRenderer_Flush(This->renderer);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_EndStateBlock(glDirect3DDevice7 *This, LPDWORD lpdwBlockHandle)
{
	TRACE_ENTER(2,14,This,14,lpdwBlockHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_EndStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

// Use EXACTLY one line per entry.  Don't change layout of the list.
// This list is a subset of the list found in glTexture to be used when limited
// texture support is enabled.
static const int START_LIMITEDTEXFORMATS = __LINE__;
const DDPIXELFORMAT limitedtexformats[] =
{ // Size					Flags							FOURCC	bits	R/Ymask		G/U/Zmask	B/V/STmask	A/Zmask
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},  // 15 bit 555
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0x7c00,		0x3E0,		0x1F,		0x8000},  // 16-bit 1555
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},  // 16 bit 565
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xF00,		0xF0,		0xF,		0xF000},  // 16-bit 4444
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},  // 32 bit 888
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		32,		0xFF0000,	0xFF00,		0xFF,		0xFF000000},  // 32-bit 8888
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},  // 8 bit 332
	{sizeof(DDPIXELFORMAT),	DDPF_RGB|DDPF_PALETTEINDEXED8,	0,		8,		0,			0,			0,			0},  // 8-bit paletted
};
static const int END_LIMITEDTEXFORMATS = __LINE__ - 4;
int numlimitedtexformats;


HRESULT WINAPI glDirect3DDevice7_EnumTextureFormats(glDirect3DDevice7 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,This,14,lpd3dEnumPixelProc,14,lpArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT result;
	DDPIXELFORMAT fmt;
	if (dxglcfg.LimitTextureFormats >= 2)
	{
		numlimitedtexformats = END_LIMITEDTEXFORMATS - START_LIMITEDTEXFORMATS;
		for (int i = 0; i < numlimitedtexformats; i++)
		{
			if (limitedtexformats[i].dwFlags & DDPF_ZBUFFER) continue;
			//FIXME:  Remove this line after implementing palette textures
			if (limitedtexformats[i].dwFlags & DDPF_PALETTEINDEXED8) continue;
			memcpy(&fmt, &limitedtexformats[i], sizeof(DDPIXELFORMAT));
			result = lpd3dEnumPixelProc(&fmt, lpArg);
			if (result != D3DENUMRET_OK) TRACE_RET(HRESULT, 23, D3D_OK);
		}
	}
	else
	{
		for (int i = 0; i < numtexformats; i++)
		{
			//FIXME: Remove this line after implementing RGB3328 textures
			if (i == 11) continue;
			// Exclude Z buffer formats
			if (texformats[i].dwFlags & DDPF_ZBUFFER) continue;
			//FIXME:  Remove these lines after implementing palette textures
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED1) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED2) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED4) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED8) continue;
			memcpy(&fmt, &texformats[i], sizeof(DDPIXELFORMAT));
			result = lpd3dEnumPixelProc(&fmt, lpArg);
			if (result != D3DENUMRET_OK) TRACE_RET(HRESULT, 23, D3D_OK);
		}
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7_EnumTextureFormats2(glDirect3DDevice7 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg)
{
	TRACE_ENTER(3, 14, This, 14, lpd3dEnumTextureProc, 14, lpArg);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	HRESULT result;
	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	if (dxglcfg.LimitTextureFormats >= 1)
	{
		numlimitedtexformats = END_LIMITEDTEXFORMATS - START_LIMITEDTEXFORMATS;
		for (int i = 0; i < numlimitedtexformats; i++)
		{
			// Exclude FOURCC formats
			if (limitedtexformats[i].dwFlags & DDPF_FOURCC) continue;
			//FIXME:  Remove this line after implementing palette textures
			if (limitedtexformats[i].dwFlags & DDPF_PALETTEINDEXED8) continue;
			memcpy(&ddsd.ddpfPixelFormat, &limitedtexformats[i], sizeof(DDPIXELFORMAT));
			result = lpd3dEnumTextureProc(&ddsd, lpArg);
			if (result != D3DENUMRET_OK) TRACE_RET(HRESULT, 23, D3D_OK);
		}
	}
	else
	{
		for (int i = 0; i < numtexformats; i++)
		{
			//FIXME: Remove this line after implementing RGB3328 textures
			if (i == 11) continue;
			// Exclude Z buffer formats
			if (texformats[i].dwFlags & DDPF_ZBUFFER) continue;
			// Exclude FOURCC formats
			if (texformats[i].dwFlags & DDPF_FOURCC) continue;
			//FIXME:  Remove these lines after implementing palette textures
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED1) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED2) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED4) continue;
			if (texformats[i].dwFlags & DDPF_PALETTEINDEXED8) continue;
			memcpy(&ddsd.ddpfPixelFormat, &::texformats[i], sizeof(DDPIXELFORMAT));
			result = lpd3dEnumTextureProc(&ddsd, lpArg);
			if (result != D3DENUMRET_OK) TRACE_RET(HRESULT, 23, D3D_OK);
		}
	}
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7_GetCaps(glDirect3DDevice7 *This, LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	TRACE_ENTER(2,14,This,14,lpD3DDevDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	D3DDEVICEDESC7 desc = This->d3ddesc;
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	desc.deviceGUID = IID_IDirect3DTnLHalDevice;
	memcpy(lpD3DDevDesc,&desc,sizeof(D3DDEVICEDESC7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetClipPlane(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE *pPlaneEquation)
{
	TRACE_ENTER(3,14,This,8,dwIndex,9,pPlaneEquation);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_GetClipPlane: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetClipStatus(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_GetClipStatus: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetDirect3D(glDirect3DDevice7 *This, LPDIRECT3D7 *lplpD3D)
{
	TRACE_ENTER(2,14,This,14,lplpD3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3D) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lplpD3D = This->glD3D7;
	This->glD3D7->AddRef();
	TRACE_VAR("*lplpD3D",14,*lplpD3D);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetInfo(glDirect3DDevice7 *This, DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize)
{
	TRACE_ENTER(4,14,This,9,dwDevInfoID,14,pDevInfoStruct,8,dwSize);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_GetInfo: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetLight(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	TRACE_ENTER(3,14,This,8,dwLightIndex,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwLightIndex >= This->lightsmax) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->lights[dwLightIndex]) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DLight_GetLight7(This->lights[dwLightIndex], lpLight);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetLightEnable(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL* pbEnable)
{
	TRACE_ENTER(3,14,This,8,dwLightIndex,14,pbEnable);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwLightIndex >= This->lightsmax) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->lights[dwLightIndex]) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!pbEnable) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*pbEnable = FALSE;
	for(int i = 0; i < 8; i++)
		if(This->gllights[i] == dwLightIndex) *pbEnable = TRUE;
	TRACE_VAR("*pbEnable",22,*pbEnable);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetMaterial(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial)
{
	TRACE_ENTER(2,14,This,14,lpMaterial);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpMaterial,&This->material,sizeof(D3DMATERIAL7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetRenderState(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,This,27,dwRenderStateType,14,lpdwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwRenderStateType <= 152)
	{
		*lpdwRenderState = This->renderstate[dwRenderStateType];
		TRACE_VAR("*lpdwRenderState",9,*lpdwRenderState);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirect3DDevice7_GetRenderTarget(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 *lplpRenderTarget)
{
	TRACE_ENTER(2,14,This,14,lplpRenderTarget);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpRenderTarget) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->glDDS7->AddRef();
	*lplpRenderTarget = This->glDDS7;
	TRACE_VAR("*lplpRenderTarget",14,*lplpRenderTarget);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetStateData(glDirect3DDevice7 *This, DWORD dwState, LPVOID* lplpStateData)
{
	TRACE_ENTER(3,14,This,9,dwState,14,lplpStateData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_GetStateData: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetTexture(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	TRACE_ENTER(3,14,This,8,dwStage,14,lplpTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpTexture) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->texstages[dwStage].surface) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	*lplpTexture = This->texstages[dwStage].surface;
	This->texstages[dwStage].surface->AddRef();
	TRACE_VAR("*lplpTexture",14,*lplpTexture);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_GetTextureStageState(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	TRACE_ENTER(4,14,This,8,dwStage,28,dwState,14,lpdwValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpdwValue) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		*lpdwValue = This->texstages[dwStage].colorop;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_COLORARG1:
		*lpdwValue = This->texstages[dwStage].colorarg1;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_COLORARG2:
		*lpdwValue = This->texstages[dwStage].colorarg2;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAOP:
		*lpdwValue = This->texstages[dwStage].alphaop;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAARG1:
		*lpdwValue = This->texstages[dwStage].alphaarg1;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAARG2:
		*lpdwValue = This->texstages[dwStage].alphaarg2;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT00:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenv00,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT01:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenv01,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT10:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenv10,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT11:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenv11,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_TEXCOORDINDEX:
		*lpdwValue = This->texstages[dwStage].texcoordindex;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ADDRESS:
	case D3DTSS_ADDRESSU:
		*lpdwValue = This->texstages[dwStage].addressu;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ADDRESSV:
		*lpdwValue = This->texstages[dwStage].addressv;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BORDERCOLOR:
		*lpdwValue = This->texstages[dwStage].bordercolor;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAGFILTER:
		*lpdwValue = This->texstages[dwStage].magfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MINFILTER:
		*lpdwValue = This->texstages[dwStage].minfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MIPFILTER:
		*lpdwValue = This->texstages[dwStage].mipfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(lpdwValue,&This->texstages[dwStage].lodbias,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAXMIPLEVEL:
		*lpdwValue = This->texstages[dwStage].miplevel;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAXANISOTROPY:
		*lpdwValue = This->texstages[dwStage].anisotropy;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVLSCALE:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenvlscale,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(lpdwValue,&This->texstages[dwStage].bumpenvloffset,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		*lpdwValue = This->texstages[dwStage].textransform;
		TRACE_RET(HRESULT,23,D3D_OK);
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(lpD3DMatrix,&This->matWorld,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(lpD3DMatrix,&This->matView,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(lpD3DMatrix,&This->matProjection,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_GetViewport(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport)
{
	TRACE_ENTER(2,14,This,14,lpViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	memcpy(lpViewport,&This->viewport,sizeof(D3DVIEWPORT7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_LightEnable(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL bEnable)
{
	TRACE_ENTER(3,14,This,8,dwLightIndex,22,bEnable);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	int i;
	BOOL foundlight = FALSE;
	if(dwLightIndex >= This->lightsmax)
	{
		if(!ExpandLightBuffer(&This->lights,&This->lightsmax,dwLightIndex+1)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	}
	if(!This->lights[dwLightIndex]) This->lights[dwLightIndex] = new glDirect3DLight;
	if(bEnable)
	{
		for(i = 0; i < 8; i++)
			if(This->gllights[i] == dwLightIndex) TRACE_RET(HRESULT,23,D3D_OK);
		for(i = 0; i < 8; i++)
		{
			if(This->gllights[i] == -1)
			{
				foundlight = TRUE;
				This->gllights[i] = dwLightIndex;
				glRenderer_SetLight(This->renderer, i, &This->lights[This->gllights[i]]->light);
				break;
			}
		}
		if(!foundlight) TRACE_RET(HRESULT,23,D3DERR_LIGHT_SET_FAILED);
	}
	else
	{
		for(i = 0; i < 8; i++)
		{
			if(This->gllights[i] == dwLightIndex)
			{
				This->gllights[i] = -1;
				glRenderer_RemoveLight(This->renderer, i);
			}
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_Load(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
	LPRECT lprcSrcRect, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,14,lpDestTex,25,lpDestPoint,14,lpSrcTex,26,lprcSrcRect,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_Load: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_MultiplyTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_MultiplyTransform: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_PreLoad(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpddsTexture)
{
	TRACE_ENTER(2,14,This,14,lpddsTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_PreLoad: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_SetClipPlane(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
	TRACE_ENTER(3,14,This,8,dwIndex,14,pPlaneEquation);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_SetClipPland: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_SetClipStatus(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_SetClipStatus: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_SetLight(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	TRACE_ENTER(3,14,This,8,dwLightIndex,14,lpLight);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpLight) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if ((lpLight->dltType < D3DLIGHT_POINT) || (lpLight->dltType > D3DLIGHT_GLSPOT))
		TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	bool foundlight = false;
	if(dwLightIndex >= This->lightsmax)
	{
		if(!ExpandLightBuffer(&This->lights,&This->lightsmax,dwLightIndex+1)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	}
	if(!This->lights[dwLightIndex]) This->lights[dwLightIndex] = new glDirect3DLight;
	glDirect3DLight_SetLight7(This->lights[dwLightIndex], lpLight);
	for (int i = 0; i < 8; i++)
	{
		if (This->gllights[i] == dwLightIndex)
			glRenderer_SetLight(This->renderer, i, &This->lights[This->gllights[i]]->light);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetMaterial(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial)
{
	TRACE_ENTER(2,14,This,14,lpMaterial);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&This->material,lpMaterial,sizeof(D3DMATERIAL7));
	glRenderer_SetMaterial(This->renderer, lpMaterial);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7_SetRenderState(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	BOOL devstate = TRUE;
	BOOL noalpha = FALSE;
	TRACE_ENTER(3, 14, This, 27, dwRendStateType, 9, dwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dwRendStateType)
	{
	case D3DRENDERSTATE_TEXTUREHANDLE:
		devstate = FALSE;
		if(dwRenderState > This->texturecount-1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwRenderState)
		{
			if(!This->textures[dwRenderState]) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
			glDirect3DDevice7_SetTexture(This,0,This->textures[dwRenderState]);
		}
		else glDirect3DDevice7_SetTexture(This,0,NULL);
		break;
	case D3DRENDERSTATE_TEXTUREADDRESS:
		devstate = FALSE;
		glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_TEXTUREADDRESSU,dwRenderState);
		glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_TEXTUREADDRESSV,dwRenderState);
		break;
	case D3DRENDERSTATE_WRAPU:
		devstate = FALSE;
		if(dwRenderState) This->renderstate[D3DRENDERSTATE_WRAP0] |= D3DWRAP_U;
		else This->renderstate[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_U;
		glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_WRAP0, This->renderstate[D3DRENDERSTATE_WRAP0]);
		break;
	case D3DRENDERSTATE_WRAPV:
		devstate = FALSE;
		if (dwRenderState) This->renderstate[D3DRENDERSTATE_WRAP0] |= D3DWRAP_V;
		else This->renderstate[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_V;
		glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_WRAP0, This->renderstate[D3DRENDERSTATE_WRAP0]);
		break;
	case D3DRENDERSTATE_TEXTUREMAG:
		devstate = FALSE;
		switch(dwRenderState)
		{
		case D3DFILTER_NEAREST:
		default:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MAGFILTER,D3DTFG_POINT);
			break;
		case D3DFILTER_LINEAR:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MAGFILTER,D3DTFG_LINEAR);
		}
		break;
	case D3DRENDERSTATE_TEXTUREMIN:
		devstate = FALSE;
		switch(dwRenderState)
		{
		case D3DFILTER_NEAREST:
		default:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_POINT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_NONE);
			break;
		case D3DFILTER_LINEAR:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_NONE);
			break;
		case D3DFILTER_MIPNEAREST:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_POINT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_POINT);
			break;
		case D3DFILTER_MIPLINEAR:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_POINT);
			break;
		case D3DFILTER_LINEARMIPNEAREST:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_POINT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_LINEAR);
			break;
		case D3DFILTER_LINEARMIPLINEAR:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_MIPFILTER,D3DTFP_LINEAR);
			break;
		}
		break;
	case D3DRENDERSTATE_TEXTUREMAPBLEND:
		if(!dwRenderState || (dwRenderState > D3DTBLEND_ADD)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		switch(dwRenderState)
		{
		case D3DTBLEND_DECAL:
		case D3DTBLEND_COPY:
		default:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
			break;
		case D3DTBLEND_MODULATE:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			if (!This->texstages[0].surface) noalpha = TRUE;
			if (This->texstages[0].surface)
				if (!(This->texstages[0].surface->ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
					noalpha = TRUE;
			if (noalpha) glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
			else glDirect3DDevice7_SetTextureStageState(This, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		case D3DTBLEND_DECALALPHA:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLOROP,D3DTOP_BLENDTEXTUREALPHA);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
			break;
		case D3DTBLEND_MODULATEALPHA:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
			break;
		case D3DTBLEND_DECALMASK:
		case D3DTBLEND_MODULATEMASK:
			FIXME("DX5 masked blend modes not supported.");
			TRACE_RET(HRESULT,23,DDERR_UNSUPPORTED);
		case D3DTBLEND_ADD:
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_COLOROP,D3DTOP_ADD);
			glDirect3DDevice7_SetTextureStageState(This,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
			break;
		}
		break;
	}
	if(dwRendStateType > 152) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwRendStateType < 0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	This->renderstate[dwRendStateType] = dwRenderState;
	if (devstate) glRenderer_SetRenderState(This->renderer, dwRendStateType, dwRenderState);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetRenderTarget(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpNewRenderTarget,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpNewRenderTarget) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpNewRenderTarget->GetSurfaceDesc(&ddsd);
	if(!(ddsd.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)) TRACE_RET(HRESULT,23,DDERR_INVALIDSURFACETYPE);
	if(This->glDDS7) This->glDDS7->Release();
	This->glDDS7 = (glDirectDrawSurface7*)lpNewRenderTarget;
	This->glDDS7->AddRef();
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetStateData(glDirect3DDevice7 *This, DWORD dwState, LPVOID lpStateData)
{
	TRACE_ENTER(3,14,This,8,dwState,14,lpStateData);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_SetStateData: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7_SetTexture(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
	TRACE_ENTER(3,14,This,8,dwStage,14,lpTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->texstages[dwStage].surface) This->texstages[dwStage].surface->Release();
	This->texstages[dwStage].surface = (glDirectDrawSurface7*)lpTexture;
	if(lpTexture) lpTexture->AddRef();
	if(This->texstages[dwStage].surface) glRenderer_SetTexture(This->renderer, dwStage, This->texstages[dwStage].surface->texture);
	else glRenderer_SetTexture(This->renderer, dwStage, NULL);
	if (This->renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] == D3DTBLEND_MODULATE)
	{
		bool noalpha = false;
		if (!This->texstages[0].surface) noalpha = true;
		if (This->texstages[0].surface)
			if (!(This->texstages[0].surface->ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
				noalpha = true;
		if (noalpha) glDirect3DDevice7_SetTextureStageState(This, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		else glDirect3DDevice7_SetTextureStageState(This, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetTextureStageState(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	BOOL devstate = TRUE;
	TRACE_ENTER(4,14,This,8,dwStage,28,dwState,9,dwValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		if(!dwValue || (dwValue > 24)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if (dwStage == 0) glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREMAPBLEND, 0);
		This->texstages[dwStage].colorop = (D3DTEXTUREOP)dwValue;
		break;
	case D3DTSS_COLORARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].colorarg1 = dwValue;
		break;
	case D3DTSS_COLORARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].colorarg2 = dwValue;
		break;
	case D3DTSS_ALPHAOP:
		if(!dwValue || (dwValue > 24)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if (dwStage == 0) glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREMAPBLEND, 0);
		This->texstages[dwStage].alphaop = (D3DTEXTUREOP)dwValue;
		break;
	case D3DTSS_ALPHAARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].alphaarg1 = dwValue;
		break;
	case D3DTSS_ALPHAARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].alphaarg2 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT00:
		memcpy(&This->texstages[dwStage].bumpenv00,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT01:
		memcpy(&This->texstages[dwStage].bumpenv01,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT10:
		memcpy(&This->texstages[dwStage].bumpenv10,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT11:
		memcpy(&This->texstages[dwStage].bumpenv11,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_TEXCOORDINDEX:
		if((dwValue & 0xFFFF) > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if((dwValue >> 16) > 3) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].texcoordindex = dwValue;
		break;
	case D3DTSS_ADDRESS:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		devstate = FALSE;
		glDirect3DDevice7_SetTextureStageState(This, dwStage, D3DTSS_ADDRESSU, dwValue);
		glDirect3DDevice7_SetTextureStageState(This, dwStage, D3DTSS_ADDRESSV, dwValue);
		break;
	case D3DTSS_ADDRESSU:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		break;
	case D3DTSS_ADDRESSV:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		break;
	case D3DTSS_BORDERCOLOR:
		This->texstages[dwStage].bordercolor = dwValue;
		break;
	case D3DTSS_MAGFILTER:
		if(!dwValue || (dwValue > 5)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].magfilter = (D3DTEXTUREMAGFILTER)dwValue;
		break;
	case D3DTSS_MINFILTER:
		if(!dwValue || (dwValue > 3)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].minfilter = (D3DTEXTUREMINFILTER)dwValue;
		break;
	case D3DTSS_MIPFILTER:
		if(!dwValue || (dwValue > 3)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].mipfilter = (D3DTEXTUREMIPFILTER)dwValue;
		break;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(&This->texstages[dwStage].lodbias,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_MAXMIPLEVEL:
		This->texstages[dwStage].miplevel = dwValue;
		break;
	case D3DTSS_MAXANISOTROPY:
		This->texstages[dwStage].anisotropy = dwValue;
		break;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(&This->texstages[dwStage].bumpenvlscale,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(&This->texstages[dwStage].bumpenvloffset,&dwValue,sizeof(D3DVALUE));
		break;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		if((dwValue & 0xFF) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if((dwValue >> 8) > 1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		This->texstages[dwStage].textransform = (D3DTEXTURETRANSFORMFLAGS)dwValue;
		break;
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	if (devstate) glRenderer_SetTextureStageState(This->renderer, dwStage, dwState, dwValue);
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(&This->matWorld,lpD3DMatrix,sizeof(D3DMATRIX));
		This->modelview_dirty = true;
		This->transform_dirty = true;
		break;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(&This->matView,lpD3DMatrix,sizeof(D3DMATRIX));
		This->modelview_dirty = true;
		This->transform_dirty = true;
		break;
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(&This->matProjection,lpD3DMatrix,sizeof(D3DMATRIX));
		This->projection_dirty = true;
		This->transform_dirty = true;
		break;
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	glRenderer_SetTransform(This->renderer, dtstTransformStateType, lpD3DMatrix);
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_SetViewport(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport)
{
	TRACE_ENTER(2,14,This,14,lpViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	memcpy(&This->viewport,lpViewport,sizeof(D3DVIEWPORT7));
	This->transform_dirty = true;
	glRenderer_SetD3DViewport(This->renderer, lpViewport);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7_ValidateDevice(glDirect3DDevice7 *This, LPDWORD lpdwPasses)
{
	TRACE_ENTER(2,14,This,14,lpdwPasses);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	for(int i = 0; i < 8; i++)
	{
		switch(This->texstages[i].colorop)
		{
		case D3DTOP_DISABLE:
		case D3DTOP_SELECTARG1:
		case D3DTOP_SELECTARG2:
		case D3DTOP_MODULATE:
		case D3DTOP_MODULATE2X:
		case D3DTOP_MODULATE4X:
		case D3DTOP_ADD:
		case D3DTOP_ADDSIGNED:
		case D3DTOP_ADDSIGNED2X:
		case D3DTOP_SUBTRACT:
		case D3DTOP_ADDSMOOTH:
		case D3DTOP_BLENDDIFFUSEALPHA:
		case D3DTOP_BLENDTEXTUREALPHA:
		case D3DTOP_BLENDTEXTUREALPHAPM:
		case D3DTOP_BLENDCURRENTALPHA:
			break;
		default:
			TRACE_RET(HRESULT,23,D3DERR_UNSUPPORTEDCOLOROPERATION);
		}
	}
	for(int i = 0; i < 8; i++)
	{
		switch(This->texstages[i].alphaop)
		{
		case D3DTOP_DISABLE:
		case D3DTOP_SELECTARG1:
		case D3DTOP_SELECTARG2:
		case D3DTOP_MODULATE:
		case D3DTOP_MODULATE2X:
		case D3DTOP_MODULATE4X:
		case D3DTOP_ADD:
		case D3DTOP_ADDSIGNED:
		case D3DTOP_ADDSIGNED2X:
		case D3DTOP_SUBTRACT:
		case D3DTOP_ADDSMOOTH:
		case D3DTOP_BLENDDIFFUSEALPHA:
		case D3DTOP_BLENDTEXTUREALPHA:
		case D3DTOP_BLENDTEXTUREALPHAPM:
		case D3DTOP_BLENDCURRENTALPHA:
			break;
		default:
			TRACE_RET(HRESULT,23,D3DERR_UNSUPPORTEDALPHAOPERATION);
		}
	}
	if(lpdwPasses) *lpdwPasses = 1;
	TRACE_VAR("*lpdwPasses",8,*lpdwPasses);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}


D3DMATERIALHANDLE glDirect3DDevice7_AddMaterial(glDirect3DDevice7 *This, glDirect3DMaterial3 *material)
{
	TRACE_ENTER(2,14,This,14,material);
	This->materials[This->materialcount] = material;
	glDirect3DMaterial3_AddRef(material);
	This->materialcount++;
	if(This->materialcount >= This->maxmaterials)
	{
		glDirect3DMaterial3 **newmat;
		This->maxmaterials += 32;
		newmat = (glDirect3DMaterial3**)realloc(This->materials,This->maxmaterials*sizeof(glDirect3DMaterial3*));
		if(!newmat)
		{
			This->maxmaterials -= 32;
			This->materialcount--;
			This->materials[This->materialcount] = NULL;
			glDirect3DMaterial3_Release(material);
			TRACE_EXIT(9,0xFFFFFFFF);
			return 0xFFFFFFFF;
		}
		This->materials = newmat;
	}
	TRACE_EXIT(9,(This->materialcount-1));
	return This->materialcount-1;
}

D3DTEXTUREHANDLE glDirect3DDevice7_AddTexture(glDirect3DDevice7 *This, glDirectDrawSurface7 *texture)
{
	TRACE_ENTER(2,14,This,14,texture);
	This->textures[This->texturecount] = texture;
	texture->AddRef();
	This->texturecount++;
	if(This->texturecount >= This->maxtextures)
	{
		glDirectDrawSurface7 **newtex;
		This->maxtextures += 32;
		newtex = (glDirectDrawSurface7**)realloc(This->textures,This->maxtextures*sizeof(glDirectDrawSurface7*));
		if(!newtex)
		{
			This->maxtextures -= 32;
			This->texturecount--;
			This->textures[This->texturecount] = NULL;
			texture->Release();
			TRACE_EXIT(9,0xFFFFFFFF);
			return 0xFFFFFFFF;
		}
		This->textures = newtex;
	}
	TRACE_EXIT(9,(This->texturecount-1));
	return This->texturecount-1;
}

HRESULT glDirect3DDevice7_AddViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(int i = 0; i < This->maxviewports; i++)
	{
		if(This->viewports[i] == (glDirect3DViewport3*)lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	This->viewports[This->viewportcount] = (glDirect3DViewport3*)lpDirect3DViewport;
	glDirect3DViewport3_AddRef(This->viewports[This->viewportcount]);
	This->viewportcount++;
	if(This->viewportcount >= This->maxviewports)
	{
		glDirect3DViewport3 **newviewport;
		This->maxviewports += 32;
		newviewport = (glDirect3DViewport3**)realloc(This->viewports,This->maxviewports*sizeof(glDirect3DViewport3*));
		if(!newviewport)
		{
			This->viewports--;
			glDirect3DViewport3_Release(This->viewports[This->viewportcount]);
			This->viewports[This->viewportcount] = NULL;
			This->maxviewports -= 32;
			TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		}
	}
	glDirect3DViewport3_SetDevice(This->viewports[This->viewportcount-1],This);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_DeleteViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(int i = 0; i < This->maxviewports; i++)
	{
		if(This->viewports[i] == (glDirect3DViewport3*)lpDirect3DViewport)
		{
			glDirect3DViewport3_SetCurrent(This->viewports[i],false);
			glDirect3DViewport3_SetDevice(This->viewports[i],NULL);
			glDirect3DViewport3_Release(This->viewports[i]);
			if(This->currentviewport == This->viewports[i]) This->currentviewport = NULL;
			This->viewports[i] = NULL;
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7_NextViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DViewport,14,lplpAnotherViewport,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpAnotherViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7_NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7_GetCurrentViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	TRACE_ENTER(2,14,This,14,lplpd3dViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpd3dViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lplpd3dViewport = (LPDIRECT3DVIEWPORT3)This->currentviewport;
	if (!This->currentviewport) TRACE_RET(HRESULT, 23, D3DERR_NOCURRENTVIEWPORT);
	glDirect3DViewport3_AddRef(This->currentviewport);
	TRACE_VAR("*lplpd3dViewport",14,*lplpd3dViewport);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_SetCurrentViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	TRACE_ENTER(2,14,This,14,lpd3dViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(This->currentviewport == (glDirect3DViewport3*)lpd3dViewport) TRACE_RET(HRESULT,23,D3D_OK);
	for(int i = 0; i < This->maxviewports; i++)
	{
		if(lpd3dViewport == (LPDIRECT3DVIEWPORT3)This->viewports[i])
		{
			glDirect3DViewport3_SetCurrent(This->viewports[i],true);
			This->currentviewport = (glDirect3DViewport3*)lpd3dViewport;
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7_Begin(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,8,d3dpt,9,dwVertexTypeDesc,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_Begin: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7_BeginIndexed(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpvVertices) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7_BeginIndexed: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7_Index(glDirect3DDevice7 *This, WORD wVertexIndex)
{
	TRACE_ENTER(2,14,This,5,wVertexIndex);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_Index: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7_Vertex(glDirect3DDevice7 *This, LPVOID lpVertex)
{
	TRACE_ENTER(2,14,This,14,lpVertex);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpVertex) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7_Vertex: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7_End(glDirect3DDevice7 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7_End: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7_GetCaps3(glDirect3DDevice7 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,This,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DHWDevDesc && !lpD3DHELDevDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	D3DDEVICEDESC desc = This->d3ddesc3;
	if(lpD3DHELDevDesc)
	{
		desc.dwSize = lpD3DHELDevDesc->dwSize;
		if(desc.dwSize < sizeof(D3DDEVICEDESC1)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(desc.dwSize > sizeof(D3DDEVICEDESC1)) desc.dwSize = sizeof(D3DDEVICEDESC);
		memcpy(lpD3DHELDevDesc, &desc, desc.dwSize);
	}
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION;
	if(lpD3DHWDevDesc)
	{
		desc.dwSize = lpD3DHWDevDesc->dwSize;
		if(desc.dwSize < sizeof(D3DDEVICEDESC1)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(desc.dwSize > sizeof(D3DDEVICEDESC1)) desc.dwSize = sizeof(D3DDEVICEDESC);
		memcpy(lpD3DHWDevDesc, &desc, desc.dwSize);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_GetLightState(glDirect3DDevice7 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,14,lpdwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdwLightState)TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwLightStateType)
	{
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(This->currentmaterial) *lpdwLightState = This->currentmaterial->handle;
		else *lpdwLightState = 0;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_AMBIENT,lpdwLightState));
	case D3DLIGHTSTATE_COLORMODEL:
		*lpdwLightState = D3DCOLOR_RGB;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_FOGVERTEXMODE,lpdwLightState));
	case D3DLIGHTSTATE_FOGSTART:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_FOGSTART,lpdwLightState));
	case D3DLIGHTSTATE_FOGEND:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_FOGEND,lpdwLightState));
	case D3DLIGHTSTATE_FOGDENSITY:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_FOGDENSITY,lpdwLightState));
	case D3DLIGHTSTATE_COLORVERTEX:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This,D3DRENDERSTATE_COLORVERTEX,lpdwLightState));
	}
}
HRESULT glDirect3DDevice7_SetLightState(glDirect3DDevice7 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,9,dwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dwLightStateType)
	{
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(!dwLightState) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwLightState < This->materialcount)
		{
			if(This->materials[dwLightState] == This->currentmaterial) TRACE_RET(HRESULT,23,D3D_OK);
			if(This->materials[dwLightState])
			{
				if(This->currentmaterial)glDirect3DMaterial3_SetCurrent(This->currentmaterial, FALSE);
				glDirect3DMaterial3_SetCurrent(This->materials[dwLightState], TRUE);
				This->currentmaterial = This->materials[dwLightState];
			}
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		TRACE_RET(HRESULT,23,glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_AMBIENT,dwLightState));
	case D3DLIGHTSTATE_COLORMODEL:
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		TRACE_RET(HRESULT,23, glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_FOGVERTEXMODE,dwLightState));
	case D3DLIGHTSTATE_FOGSTART:
		TRACE_RET(HRESULT,23, glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_FOGSTART,dwLightState));
	case D3DLIGHTSTATE_FOGEND:
		TRACE_RET(HRESULT,23, glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_FOGEND,dwLightState));
	case D3DLIGHTSTATE_FOGDENSITY:
		TRACE_RET(HRESULT,23, glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_FOGDENSITY,dwLightState));
	case D3DLIGHTSTATE_COLORVERTEX:
		TRACE_RET(HRESULT,23, glDirect3DDevice7_SetRenderState(This,D3DRENDERSTATE_COLORVERTEX,dwLightState));
	}
}

HRESULT glDirect3DDevice7_GetStats(glDirect3DDevice7 *This, LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,This,14,lpD3DStats);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DStats) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpD3DStats->dwSize < sizeof(D3DSTATS)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpD3DStats,&This->stats,sizeof(D3DSTATS));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_SwapTextureHandles(glDirect3DDevice7 *This, LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	TRACE_ENTER(3,14,This,14,lpD3DTex1,14,lpD3DTex2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTex1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpD3DTex2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7_SwapTextureHandles: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

void glDirect3DDevice7_InitDX2(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1, 14, This);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
	TRACE_EXIT(0, 0);
}

void glDirect3DDevice7_InitDX5(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREHANDLE,0);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREADDRESS,D3DTADDRESS_WRAP);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_WRAPU,FALSE);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_WRAPV,FALSE);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREMAG,D3DFILTER_NEAREST);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREMIN,D3DFILTER_NEAREST);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATE);
	glDirect3DDevice7_SetRenderState(This, D3DRENDERSTATE_SPECULARENABLE,TRUE);
	TRACE_EXIT(0,0);
}

void glDirect3DDevice7_SetScale(glDirect3DDevice7 *This, D3DVALUE x, D3DVALUE y) 
{
	This->scalex = x;
	This->scaley = y;
}

HRESULT glDirect3DDevice7_CreateMatrix(glDirect3DDevice7 *This, LPD3DMATRIXHANDLE lpD3DMatHandle)
{
	TRACE_ENTER(2,14,This,14,lpD3DMatHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	D3DMATRIXHANDLE i;
	int foundslot = 0;
	if(!This->matrices)
	{
		This->matrices = (D3D1MATRIX*)malloc(16*sizeof(D3D1MATRIX));
		if(!This->matrices) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		ZeroMemory(This->matrices,16*sizeof(D3D1MATRIX));
		This->matrixcount = 16;
	}
	for(i = 0; i < This->matrixcount; i++)
	{
		if(i == 0) continue;
		if(!This->matrices[i].active)
		{
			foundslot = i;
			break;
		}
	}
	if(!foundslot)
	{
		int newcount;
		D3D1MATRIX *newmatrices;
		newcount = This->matrixcount + 16;
		newmatrices = (D3D1MATRIX*)realloc(This->matrices,newcount*sizeof(D3D1MATRIX));
		if(!newmatrices) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		ZeroMemory(&newmatrices[This->matrixcount],16*sizeof(D3D1MATRIX));
		This->matrices = newmatrices;
		foundslot = This->matrixcount;
		This->matrixcount = newcount;
	}
	*lpD3DMatHandle = foundslot;
	__gluMakeIdentityf((GLfloat*)&This->matrices[foundslot].matrix);
	This->matrices[foundslot].active = TRUE;
	TRACE_VAR("*lpD3DMatHandle",9,*lpD3DMatHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_DeleteMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE d3dMatHandle)
{
	TRACE_ENTER(2,14,This,9,d3dMatHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!d3dMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(d3dMatHandle >= This->matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->matrices[d3dMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_DESTROY_FAILED);
	This->matrices[d3dMatHandle].active = FALSE;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_GetMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,9,lpD3DMatHandle,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DMatHandle)
	{
		__gluMakeIdentityf((GLfloat*)lpD3DMatrix);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(lpD3DMatHandle >= This->matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->matrices[lpD3DMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_GETDATA_FAILED);
	memcpy(lpD3DMatrix,&This->matrices[lpD3DMatHandle].matrix,sizeof(D3DMATRIX));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_SetMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,9,d3dMatHandle,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!d3dMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(d3dMatHandle >= This->matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!This->matrices[d3dMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_SETDATA_FAILED);
	memcpy(&This->matrices[d3dMatHandle].matrix,lpD3DMatrix,sizeof(D3DMATRIX));
	if(d3dMatHandle == This->mhWorld) glDirect3DDevice7_SetTransform(This,D3DTRANSFORMSTATE_WORLD,lpD3DMatrix);
	if(d3dMatHandle == This->mhView) glDirect3DDevice7_SetTransform(This,D3DTRANSFORMSTATE_VIEW,lpD3DMatrix);
	if(d3dMatHandle == This->mhProjection) glDirect3DDevice7_SetTransform(This,D3DTRANSFORMSTATE_PROJECTION,lpD3DMatrix);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_CreateExecuteBuffer(glDirect3DDevice7 *This, LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
	IUnknown* pUnkOuter)
{
	HRESULT error;
	TRACE_ENTER(4,14,This,14,lpDesc,14,lplpDirect3DExecuteBuffer,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDesc->dwSize != sizeof(D3DEXECUTEBUFFERDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!(lpDesc->dwFlags & D3DDEB_BUFSIZE)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	error = glDirect3DExecuteBuffer_Create(lpDesc, (glDirect3DExecuteBuffer**)lplpDirect3DExecuteBuffer);
	TRACE_VAR("*lplpDirect3DExecuteBuffer",14,*lplpDirect3DExecuteBuffer);
	TRACE_EXIT(23,error);
	return error;
}

BOOL ExpandBuffer(void **buffer, DWORD *size, DWORD increment)
{
	void *ptr = realloc(*buffer,*size+increment);
	if(!ptr) return FALSE;
	*buffer = ptr;
	*size += increment;
	return TRUE;
}

INT AddTriangle(unsigned char **buffer, DWORD *buffersize, DWORD *offset, const D3DTRIANGLE *triangle)
{
	if(*buffersize < (*offset + sizeof(D3DTRIANGLE)))
	{
		if(!ExpandBuffer((void**)buffer,buffersize,1024)) return -1;
	}
	// FIXME:  Process triangle strips and fans.
	memcpy(*buffer+*offset,triangle,3*sizeof(WORD));
	*offset += 3*sizeof(WORD);
	return 0;
}

void glDirect3DDevice7_UpdateTransform(glDirect3DDevice7 *This)
{
	TRACE_ENTER(1,14,This);
	GLfloat mat1[16];
	__gluMultMatricesf(This->matWorld,This->matView,mat1);
	__gluMultMatricesf(mat1,This->matProjection,This->matTransform);
	This->transform_dirty = false;
	TRACE_EXIT(0,0);
}

void CalculateExtents(D3DRECT *extents, D3DTLVERTEX *vertices, DWORD count)
{
	if(!count) return;
	DWORD i;
	D3DVALUE minX,minY,maxX,maxY;
	minX = maxX = vertices[0].dvSX;
	minY = maxY = vertices[0].dvSY;
	for(i = 0; i < count; i++)
	{
		if(vertices[i].dvSX < minX) minX = vertices[i].dvSX;
		if(vertices[i].dvSX > maxX) maxX = vertices[i].dvSX;
		if(vertices[i].dvSY < minY) minY = vertices[i].dvSY;
		if(vertices[i].dvSY > maxY) maxY = vertices[i].dvSY;
	}
	if((LONG)minX < extents->x1) extents->x1 = (LONG)minX;
	if((LONG)maxX > extents->x2) extents->x2 = (LONG)maxX;
	if((LONG)minY < extents->y1) extents->y1 = (LONG)minY;
	if((LONG)maxY > extents->y2) extents->y2 = (LONG)maxY;
}

inline void glDirect3DDevice7_TransformViewport(glDirect3DDevice7 *This, D3DTLVERTEX *vertex)
{
	vertex->dvSX = vertex->dvSX / vertex->dvRHW * This->scalex + This->viewport.dwX + This->viewport.dwWidth / 2;
	vertex->dvSY = vertex->dvSY / vertex->dvRHW * This->scaley + This->viewport.dwY + This->viewport.dwHeight / 2;
	vertex->dvSZ /= vertex->dvRHW;
	vertex->dvRHW = 1 / vertex->dvRHW;
}

// function from project.c from Mesa source code.  See matrix.cpp for license.
static void normalize(float v[3])
{
    float r;

    r = sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
    if (r == 0.0) return;

    v[0] /= r;
    v[1] /= r;
    v[2] /= r;
}

inline void AddD3DCV(D3DCOLORVALUE *dest, D3DCOLORVALUE *src)
{
	dest->r += src->r;
	dest->g += src->g;
	dest->b += src->b;
	dest->a += src->a;
}

inline void MulD3DCV(D3DCOLORVALUE *dest, D3DCOLORVALUE *src)
{
	dest->r *= src->r;
	dest->g *= src->g;
	dest->b *= src->b;
	dest->a *= src->a;
}

inline void MulD3DCVFloat(D3DCOLORVALUE *dest, float src)
{
	dest->r *= src;
	dest->g *= src;
	dest->b *= src;
	dest->a *= src;
}

inline void NegativeVec3(float v[3])
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

inline void SubVec3(float dest[3], float src[3])
{
	dest[0] -= src[0];
	dest[1] -= src[1];
	dest[2] -= src[2];
}

inline void AddVec3(float dest[3], float src[3])
{
	dest[0] += src[0];
	dest[1] += src[1];
	dest[2] += src[2];
}

inline float dot3(float a[3], float b[3])
{
	return (a[0]*b[0])+(a[1]*b[1])+(a[2]*b[2]);
}

inline float len3(float in[3])
{
	return sqrt((in[0]*in[0])+(in[1]*in[1])+(in[2]*in[2]));
}

INT glDirect3DDevice7_TransformAndLight(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,This,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	D3DVALUE dir[3];
	D3DVALUE eye[3] = {0.0,0.0,1.0};
	D3DVALUE P[4];
	D3DVALUE L[4];
	D3DVALUE V[4];
	D3DVALUE in[4];
	D3DCOLORVALUE ambient;
	D3DCOLORVALUE diffuse;
	D3DCOLORVALUE specular;
	D3DCOLORVALUE color1;
	D3DCOLORVALUE color2;
	D3DVALUE NdotHV;
	D3DVALUE NdotV;
	D3DVALUE NdotL;
	D3DVALUE length;
	D3DVALUE attenuation;
	D3DVALUE pf;
	DWORD i;
	in[3] = 1.0f;
	if(This->transform_dirty) glDirect3DDevice7_UpdateTransform(This);
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(This->matTransform,in,&(*output)[i+dest].dvSX);
		glDirect3DDevice7_TransformViewport(This, &(*output)[i+dest]);
		(*output)[i+dest].dvRHW = 1.0f/(*output)[i+dest].dvRHW;
		(*output)[i+dest].dvTU = input[i+start].dvTU;
		(*output)[i+dest].dvTV = input[i+start].dvTV;
		diffuse.r = diffuse.g = diffuse.b = diffuse.a = 0;
		specular.r = specular.g = specular.b = specular.a = 0;
		ambient.r = (D3DVALUE)RGBA_GETRED(This->renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0f;
		ambient.g = (D3DVALUE)RGBA_GETGREEN(This->renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0f;
		ambient.b = (D3DVALUE)RGBA_GETBLUE(This->renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0f;
		ambient.a = (D3DVALUE)RGBA_GETALPHA(This->renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0f;
		for(int l = 0; l < 8; l++)
		{
			if(This->gllights[l] != -1)
			{
				AddD3DCV(&ambient,&This->lights[This->gllights[l]]->light.dcvAmbient);
				switch(This->lights[This->gllights[l]]->light.dltType)
				{
				case D3DLIGHT_DIRECTIONAL:
					NdotHV = 0;
					memcpy(dir,&This->lights[This->gllights[l]]->light.dvDirection,3*sizeof(D3DVALUE));
					normalize(dir);
					NdotL = max(dot3((float*)&input[i+start].dvNX,(float*)&dir),0.0f);
					color1 = This->lights[This->gllights[l]]->light.dcvDiffuse;
					MulD3DCVFloat(&color1,NdotL);
					AddD3DCV(&diffuse,&color1);
					if((NdotL > 0.0) && (This->material.dvPower != 0.0))
					{
						__gluMultMatrixVecf(This->matWorld,&input[i+start].dvX,P);
						memcpy(L,&This->lights[This->gllights[l]]->light.dvDirection,3*sizeof(D3DVALUE));
						NegativeVec3(L);
						SubVec3(L,P);
						normalize(L);
						memcpy(V,eye,3*sizeof(D3DVALUE));
						SubVec3(V,P);
						normalize(V);
						AddVec3(L,V);
						NdotHV = max(dot3((float*)&input[i+start].dvNX,L),0.0f);
						color1 = This->lights[This->gllights[l]]->light.dcvSpecular;
						MulD3DCVFloat(&color1,pow(NdotHV,This->material.dvPower));
						AddD3DCV(&specular,&color1);
					}
				break;
				case D3DLIGHT_POINT:
					__gluMultMatrixVecf(This->matWorld,&input[i+start].dvX,P);
					memcpy(V,&This->lights[This->gllights[l]]->light.dvPosition,3*sizeof(D3DVALUE));
					SubVec3(V,P);
					length = len3(V);
					if((length > This->lights[This->gllights[l]]->light.dvRange) && (This->lights[This->gllights[l]]->light.dvRange != 0.0)) continue;
					normalize(V);
					attenuation = 1.0f/(This->lights[This->gllights[l]]->light.dvAttenuation0+(length*This->lights[This->gllights[l]]->light.dvAttenuation1)
						+((length*length)*This->lights[This->gllights[l]]->light.dvAttenuation2));
					NdotV = max(0.0f,dot3((float*)&input[i+start].dvNX,V));
					AddVec3(V,eye);
					normalize(V);
					NdotHV = max(0.0f,dot3((float*)&input[i+start].dvNX,V));
					if(NdotV == 0.0f) pf = 0.0f;
					else if(This->material.dvPower != 0.0f) pf = pow(NdotHV, This->material.dvPower);
					else pf = 0.0f;
					color1 = This->lights[This->gllights[l]]->light.dcvDiffuse;
					MulD3DCVFloat(&color1,NdotV*attenuation);
					AddD3DCV(&diffuse,&color1);
					color1 = This->lights[This->gllights[l]]->light.dcvSpecular;
					MulD3DCVFloat(&color1,pf*attenuation);
					AddD3DCV(&specular,&color1);
					break;
				case D3DLIGHT_SPOT:
					break;
				case D3DLIGHT_PARALLELPOINT:
					break;
				case D3DLIGHT_GLSPOT:
					break;
				default:
					break;
				}
			}
			color1 = This->material.dcvDiffuse;
			MulD3DCV(&color1,&diffuse);
			color2 = This->material.dcvAmbient;
			MulD3DCV(&color2,&ambient);
			AddD3DCV(&color1,&color2);
			AddD3DCV(&color1,&This->material.dcvEmissive);
			color2 = This->material.dcvSpecular;
			MulD3DCV(&color2,&specular);
			AddD3DCV(&color1,&color2);
			(*output)[i+dest].dcColor = D3DRGBA(color1.r,color1.g,color1.b,color1.a);
			(*output)[i+dest].dcSpecular = D3DRGBA(color2.r,color2.g,color2.b,color2.a);
		}
	}
	if(extents) CalculateExtents(extents,*output,count);
	TRACE_EXIT(11,0);
	return 0;
}
INT glDirect3DDevice7_TransformOnly(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,This,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	GLfloat in[4];
	DWORD i;
	in[3] = 1.0f;
	if(This->transform_dirty) glDirect3DDevice7_UpdateTransform(This);
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(This->matTransform,in,&(*output)[i+dest].dvSX);
		glDirect3DDevice7_TransformViewport(This, &(*output)[i+dest]);
		(*output)[i+dest].dvRHW = 1.0f/(*output)[i+dest].dvRHW;
		(*output)[i+dest].dcColor = 0xFFFFFFFF;
		(*output)[i+dest].dcSpecular = 0;
		(*output)[i+dest].dvTU = input[i+start].dvTU;
		(*output)[i+dest].dvTV = input[i+start].dvTV;
	}
	if(extents) CalculateExtents(extents,*output,count);
	TRACE_EXIT(11,0);
	return 0;
}
INT glDirect3DDevice7_TransformOnlyLit(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,This,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	GLfloat in[4];
	DWORD i;
	in[3] = 1.0f;
	if(This->transform_dirty) glDirect3DDevice7_UpdateTransform(This);
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(This->matTransform,in,&(*output)[i+dest].dvSX);
		glDirect3DDevice7_TransformViewport(This, &(*output)[i+dest]);
		(*output)[i+dest].dvRHW = 1.0f/(*output)[i+dest].dvRHW;
		(*output)[i+dest].dcColor = input[i+start].dcColor;
		(*output)[i+dest].dcSpecular = input[i+start].dcSpecular;
		(*output)[i+dest].dvTU = input[i+start].dvTU;
		(*output)[i+dest].dvTV = input[i+start].dvTV;
	}
	if(extents) CalculateExtents(extents,*output,count);
	TRACE_EXIT(11,0);
	return 0;
}
INT glDirect3DDevice7_CopyVertices(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DTLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,This,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	if(This->transform_dirty) glDirect3DDevice7_UpdateTransform(This);
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	memcpy(&(*output)[dest],&input[start],count*sizeof(D3DTLVERTEX));
	if(extents) CalculateExtents(extents,*output,count);
	TRACE_EXIT(11,0);
	return 0;
}

HRESULT glDirect3DDevice7_Execute(glDirect3DDevice7 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DVIEWPORT3 vp;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&vp);
	if(FAILED(glDirect3DDevice7_SetCurrentViewport(This, vp)))
	{
		vp->Release();
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	vp->Release();
	D3DEXECUTEBUFFERDESC desc;
	D3DEXECUTEDATA data;
	HRESULT err = glDirect3DExecuteBuffer_ExecuteLock((glDirect3DExecuteBuffer*)lpDirect3DExecuteBuffer, &desc, &data);
	if(FAILED(err)) TRACE_RET(HRESULT,23,err);
	unsigned char *opptr = (unsigned char *)desc.lpData + data.dwInstructionOffset;
	unsigned char *in_vertptr = (unsigned char *)desc.lpData + data.dwVertexOffset;
	DWORD offset;
	INT result;
	D3DMATRIX mat1,mat2,mat3;
	bool ebExit = false;
	int i;
	if(This->outbuffersize < desc.dwBufferSize)
	{
		unsigned char *tmpbuffer = (unsigned char *)realloc(This->outbuffer,desc.dwBufferSize);
		if(!tmpbuffer) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		This->outbuffer = tmpbuffer;
		This->outbuffersize = desc.dwBufferSize;
	}	
	D3DVERTEX *vert_ptr = (D3DVERTEX*)This->outbuffer;
	while(1)
	{
		D3DINSTRUCTION *instruction = (D3DINSTRUCTION*) opptr;
		offset = 0;
		switch(instruction->bOpcode)
		{
		case D3DOP_POINT:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DPOINT))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				if(This->ebBufferSize < (offset + ((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX)))
				{
					if(!ExpandBuffer((void**)&This->ebBuffer,&This->ebBufferSize,(((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX) > 1024) ?
						((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX) : 1024))
						TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				}
				memcpy(&This->ebBuffer+offset,&vert_ptr[((D3DPOINT*)opptr)->wFirst],((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX));
				offset+=((D3DPOINT*)opptr)->wCount;
				opptr += instruction->bSize;
			}
			glDirect3DDevice7_DrawPrimitive(This, D3DPT_POINTLIST,D3DFVF_TLVERTEX, This->ebBuffer,offset/sizeof(D3DVERTEX),0);
			break;
		case D3DOP_LINE:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DLINE))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				if(This->ebBufferSize < (offset + sizeof(D3DLINE)))
				{
					if(!ExpandBuffer((void**)&This->ebBuffer,&This->ebBufferSize,1024)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				}
				memcpy(&This->ebBuffer+offset,opptr,sizeof(D3DLINE));
				offset += sizeof(D3DLINE);
				opptr += instruction->bSize;
			}
			glDirect3DDevice7_DrawIndexedPrimitive(This, D3DPT_LINELIST,D3DFVF_TLVERTEX,vert_ptr,(desc.dwBufferSize-data.dwVertexOffset)/sizeof(D3DVERTEX),
				(WORD*)This->ebBuffer,instruction->wCount*2,0);
			break;
		case D3DOP_TRIANGLE:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DTRIANGLE))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				result = AddTriangle(&This->ebBuffer,&This->ebBufferSize,&offset,(D3DTRIANGLE*)opptr);
				if(result == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				opptr += instruction->bSize;
			}
			if(instruction->wCount) glDirect3DDevice7_DrawIndexedPrimitive(This, D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vert_ptr,
				(desc.dwBufferSize-data.dwVertexOffset)/sizeof(D3DVERTEX), (WORD*)This->ebBuffer,instruction->wCount*3,0);
			break;
		case D3DOP_MATRIXLOAD:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DMATRIXLOAD))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				glDirect3DDevice7_GetMatrix(This, ((D3DMATRIXLOAD*)opptr)->hSrcMatrix,&mat1);
				glDirect3DDevice7_SetMatrix(This, ((D3DMATRIXLOAD*)opptr)->hDestMatrix,&mat1);
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_MATRIXMULTIPLY:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DMATRIXMULTIPLY))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				glDirect3DDevice7_GetMatrix(This, ((D3DMATRIXMULTIPLY*)opptr)->hSrcMatrix1,&mat1);
				glDirect3DDevice7_GetMatrix(This, ((D3DMATRIXMULTIPLY*)opptr)->hSrcMatrix2,&mat2);
				__gluMultMatricesf((GLfloat*)&mat1,(GLfloat*)&mat2,(GLfloat*)&mat3);
				glDirect3DDevice7_SetMatrix(This, ((D3DMATRIXMULTIPLY*)opptr)->hDestMatrix,&mat3);
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_STATETRANSFORM:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DSTATE))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				glDirect3DDevice7_GetMatrix(This, ((D3DSTATE*)opptr)->dwArg[0],&mat1);
				glDirect3DDevice7_SetTransform(This, ((D3DSTATE*)opptr)->dtstTransformStateType,&mat1);
				switch(((D3DSTATE*)opptr)->dtstTransformStateType)
				{
				case D3DTRANSFORMSTATE_WORLD:
					This->mhWorld = ((D3DSTATE*)opptr)->dwArg[0];
					break;
				case D3DTRANSFORMSTATE_VIEW:
					This->mhView = ((D3DSTATE*)opptr)->dwArg[0];
					break;
				case D3DTRANSFORMSTATE_PROJECTION:
					This->mhProjection = ((D3DSTATE*)opptr)->dwArg[0];
					break;
				default:
					break;
				}
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_STATELIGHT:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DSTATE))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				glDirect3DDevice7_SetLightState(This, ((D3DSTATE*)opptr)->dlstLightStateType,((D3DSTATE*)opptr)->dwArg[0]);
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_STATERENDER:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DSTATE))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				glDirect3DDevice7_SetRenderState(This, ((D3DSTATE*)opptr)->drstRenderStateType,((D3DSTATE*)opptr)->dwArg[0]);
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_PROCESSVERTICES:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DPROCESSVERTICES))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				switch(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_OPMASK)
				{
				case D3DPROCESSVERTICES_TRANSFORM:
					if (((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
						glDirect3DDevice7_TransformOnlyLit(This, (D3DTLVERTEX**)&This->outbuffer, &This->outbuffersize, (D3DLVERTEX*)in_vertptr, ((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					else glDirect3DDevice7_TransformOnlyLit(This, (D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					break;
				case D3DPROCESSVERTICES_TRANSFORMLIGHT:
					if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
					{
						if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
							glDirect3DDevice7_TransformOnly(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
						else glDirect3DDevice7_TransformOnly(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					}
					else
					{
						if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
							glDirect3DDevice7_TransformAndLight(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
						else glDirect3DDevice7_TransformAndLight(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					}
					break;
				case D3DPROCESSVERTICES_COPY:
					if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
						glDirect3DDevice7_CopyVertices(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DTLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					else glDirect3DDevice7_CopyVertices(This,(D3DTLVERTEX**)&This->outbuffer,&This->outbuffersize,(D3DTLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					break;
				default:
					break;
				}
				This->stats.dwVerticesProcessed += ((D3DPROCESSVERTICES*)opptr)->dwCount;
				opptr += instruction->bSize;
			}
			break;
		case D3DOP_TEXTURELOAD:
			opptr += sizeof(D3DINSTRUCTION)+(instruction->bSize*instruction->wCount);
			FIXME("D3DOP_TEXTURELOAD: stub");
			break;
		case D3DOP_EXIT:
			ebExit = true;
			break;
		case D3DOP_BRANCHFORWARD:
			opptr += sizeof(D3DINSTRUCTION)+(instruction->bSize*instruction->wCount);
			FIXME("D3DOP_BRANCHFORWARD: stub");
			break;
		case D3DOP_SPAN:
			opptr += sizeof(D3DINSTRUCTION)+(instruction->bSize*instruction->wCount);
			FIXME("D3DOP_SPAN: stub");
			break;
		case D3DOP_SETSTATUS:
			opptr += sizeof(D3DINSTRUCTION);
			if(instruction->bSize < sizeof(D3DSTATUS))
			{
				opptr += (instruction->bSize*instruction->wCount);
				break;
			}
			for(i = 0; i < instruction->wCount; i++)
			{
				if(((D3DSTATUS*)opptr)->dwFlags & D3DSETSTATUS_STATUS)
					data.dsStatus.dwStatus = ((D3DSTATUS*)opptr)->dwStatus;
				if(((D3DSTATUS*)opptr)->dwFlags & D3DSETSTATUS_EXTENTS)
					data.dsStatus.drExtent = ((D3DSTATUS*)opptr)->drExtent;
				opptr += instruction->bSize;
			}
			break;
		default:
			opptr += sizeof(D3DINSTRUCTION)+(instruction->bSize*instruction->wCount);
			break;
		}
		if(ebExit) break;
	}
	glDirect3DExecuteBuffer_ExecuteUnlock((glDirect3DExecuteBuffer*)lpDirect3DExecuteBuffer, &data);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7_GetPickRecords(glDirect3DDevice7 *This, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec)
{
	TRACE_ENTER(3,14,This,14,lpCount,14,lpD3DPickRec);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpCount) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice1_GetPickRecords: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT glDirect3DDevice7_Pick(glDirect3DDevice7 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, 
	LPD3DRECT lpRect)
{
	TRACE_ENTER(5,14,This,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpRect) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice1_Pick: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

// IDirect3DDevice3 wrapper
glDirect3DDevice3Vtbl glDirect3DDevice3_iface =
{
	glDirect3DDevice3_QueryInterface,
	glDirect3DDevice3_AddRef,
	glDirect3DDevice3_Release,
	glDirect3DDevice3_GetCaps,
	glDirect3DDevice3_GetStats,
	glDirect3DDevice3_AddViewport,
	glDirect3DDevice3_DeleteViewport,
	glDirect3DDevice3_NextViewport,
	glDirect3DDevice3_EnumTextureFormats,
	glDirect3DDevice3_BeginScene,
	glDirect3DDevice3_EndScene,
	glDirect3DDevice3_GetDirect3D,
	glDirect3DDevice3_SetCurrentViewport,
	glDirect3DDevice3_GetCurrentViewport,
	glDirect3DDevice3_SetRenderTarget,
	glDirect3DDevice3_GetRenderTarget,
	glDirect3DDevice3_Begin,
	glDirect3DDevice3_BeginIndexed,
	glDirect3DDevice3_Vertex,
	glDirect3DDevice3_Index,
	glDirect3DDevice3_End,
	glDirect3DDevice3_GetRenderState,
	glDirect3DDevice3_SetRenderState,
	glDirect3DDevice3_GetLightState,
	glDirect3DDevice3_SetLightState,
	glDirect3DDevice3_SetTransform,
	glDirect3DDevice3_GetTransform,
	glDirect3DDevice3_MultiplyTransform,
	glDirect3DDevice3_DrawPrimitive,
	glDirect3DDevice3_DrawIndexedPrimitive,
	glDirect3DDevice3_SetClipStatus,
	glDirect3DDevice3_GetClipStatus,
	glDirect3DDevice3_DrawPrimitiveStrided,
	glDirect3DDevice3_DrawIndexedPrimitiveStrided,
	glDirect3DDevice3_DrawPrimitiveVB,
	glDirect3DDevice3_DrawIndexedPrimitiveVB,
	glDirect3DDevice3_ComputeSphereVisibility,
	glDirect3DDevice3_GetTexture,
	glDirect3DDevice3_SetTexture,
	glDirect3DDevice3_GetTextureStageState,
	glDirect3DDevice3_SetTextureStageState,
	glDirect3DDevice3_ValidateDevice
};

HRESULT glDirect3DDevice3_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice3 **newdev)
{
	TRACE_ENTER(1, 14, glD3DDev7, 14, newdev);
	*newdev = (glDirect3DDevice3*)malloc(sizeof(glDirect3DDevice3));
	if (!(*newdev)) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	(*newdev)->glD3DDev7 = glD3DDev7;
	(*newdev)->lpVtbl = &glDirect3DDevice3_iface;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}


HRESULT WINAPI glDirect3DDevice3_QueryInterface(glDirect3DDevice3 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DDevice3_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DDevice7_QueryInterface(This->glD3DDev7,riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice3_AddRef(glDirect3DDevice3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_AddRef(This->glD3DDev7));
}

ULONG WINAPI glDirect3DDevice3_Release(glDirect3DDevice3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_Release(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice3_AddViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_AddViewport(This->glD3DDev7, lpDirect3DViewport));
}

HRESULT WINAPI glDirect3DDevice3_Begin(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,8,d3dpt,9,dwVertexTypeDesc,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Begin(This->glD3DDev7, d3dpt,dwVertexTypeDesc,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_BeginIndexed(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_BeginIndexed(This->glD3DDev7, dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwNumVertices,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_BeginScene(glDirect3DDevice3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_BeginScene(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice3_ComputeSphereVisibility(glDirect3DDevice3 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	TRACE_ENTER(6,14,This,14,lpCenters,14,lpRadii,8,dwNumSpheres,9,dwFlags,14,lpdwReturnValues);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_ComputeSphereVisibility(This->glD3DDev7, lpCenters,lpRadii,dwNumSpheres,dwFlags,lpdwReturnValues));
}

HRESULT WINAPI glDirect3DDevice3_DeleteViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DeleteViewport(This->glD3DDev7, lpDirect3DViewport));
}

HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitive(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,8,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawIndexedPrimitive(This->glD3DDev7, d3dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitiveStrided(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,8,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawIndexedPrimitiveStrided(This->glD3DDev7, d3dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitiveVB(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,d3dptPrimitiveType,14,lpd3dVertexBuffer,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dVertexBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawIndexedPrimitiveVB(This->glD3DDev7, d3dptPrimitiveType,
		(LPDIRECT3DVERTEXBUFFER7)lpd3dVertexBuffer,0,-1,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_DrawPrimitive(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertices,8,dwVertexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawPrimitive(This->glD3DDev7, dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_DrawPrimitiveStrided(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawPrimitiveStrided(This->glD3DDev7, dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_DrawPrimitiveVB(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dVertexBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT, 23, glDirect3DDevice7_DrawPrimitiveVB(This->glD3DDev7, d3dptPrimitiveType, (LPDIRECT3DVERTEXBUFFER7)lpd3dVertexBuffer,
		dwStartVertex, dwNumVertices, dwFlags));
}
HRESULT WINAPI glDirect3DDevice3_End(glDirect3DDevice3 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_End(This->glD3DDev7, dwFlags));
}
	
HRESULT WINAPI glDirect3DDevice3_EndScene(glDirect3DDevice3 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_EndScene(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice3_EnumTextureFormats(glDirect3DDevice3 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,This,14,lpd3dEnumPixelProc,14,lpArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_EnumTextureFormats(This->glD3DDev7, lpd3dEnumPixelProc,lpArg));
}

HRESULT WINAPI glDirect3DDevice3_GetCaps(glDirect3DDevice3 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,This,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetCaps3(This->glD3DDev7, lpD3DHWDevDesc,lpD3DHELDevDesc));
}

HRESULT WINAPI glDirect3DDevice3_GetClipStatus(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetClipStatus(This->glD3DDev7, lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice3_GetCurrentViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	TRACE_ENTER(2,14,This,14,lplpd3dViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetCurrentViewport(This->glD3DDev7, lplpd3dViewport));
}

HRESULT WINAPI glDirect3DDevice3_GetDirect3D(glDirect3DDevice3 *This, LPDIRECT3D3 *lplpD3D)
{
	TRACE_ENTER(2,14,This,14,lplpD3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glDirect3DDevice7_GetDirect3D(This->glD3DDev7, &d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D3,(void**)lplpD3D);
	d3d7->Release();
	TRACE_VAR("*lplpD3D",14,*lplpD3D);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3_GetLightState(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,14,lpdwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetLightState(This->glD3DDev7, dwLightStateType,lpdwLightState));
}

HRESULT WINAPI glDirect3DDevice3_GetRenderState(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,This,27,dwRenderStateType,14,lpdwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This->glD3DDev7, dwRenderStateType,lpdwRenderState));
}

HRESULT WINAPI glDirect3DDevice3_GetRenderTarget(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 *lplpRenderTarget)
{
	TRACE_ENTER(2,14,This,14,lplpRenderTarget);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glDirect3DDevice7_GetRenderTarget(This->glD3DDev7, &dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirectDrawSurface4,(void**)lplpRenderTarget);
	dds7->Release();
	TRACE_VAR("*lplpRenderTarget",14,*lplpRenderTarget);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3_GetStats(glDirect3DDevice3 *This, LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,This,14,lpD3DStats);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetStats(This->glD3DDev7, lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice3_GetTexture(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 *lplpTexture)
{
	TRACE_ENTER(3,14,This,8,dwStage,14,lplpTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glDirect3DDevice7_GetTexture(This->glD3DDev7, dwStage,&dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirect3DTexture2,(void**)lplpTexture);
	dds7->Release();
	TRACE_VAR("*lplpTexture",14,*lplpTexture);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3_GetTextureStageState(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	TRACE_ENTER(4,14,This,8,dwStage,28,dwState,14,lpdwValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetTextureStageState(This->glD3DDev7, dwStage,dwState,lpdwValue));
}

HRESULT WINAPI glDirect3DDevice3_GetTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice3_Index(glDirect3DDevice3 *This, WORD wVertexIndex)
{
	TRACE_ENTER(2,14,This,5,wVertexIndex);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Index(This->glD3DDev7, wVertexIndex));
}

HRESULT WINAPI glDirect3DDevice3_MultiplyTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_MultiplyTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}
HRESULT WINAPI glDirect3DDevice3_NextViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DViewport,14,lplpAnotherViewport,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_NextViewport(This->glD3DDev7, lpDirect3DViewport,lplpAnotherViewport,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_SetClipStatus(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetClipStatus(This->glD3DDev7, lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice3_SetCurrentViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	TRACE_ENTER(2,14,This,14,lpd3dViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetCurrentViewport(This->glD3DDev7, lpd3dViewport));
}

HRESULT WINAPI glDirect3DDevice3_SetLightState(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,8,dwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetLightState(This->glD3DDev7, dwLightStateType,dwLightState));
}

HRESULT WINAPI glDirect3DDevice3_SetRenderState(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	TRACE_ENTER(3,14,This,27,dwRendStateType,9,dwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetRenderState(This->glD3DDev7, dwRendStateType,dwRenderState));
}
	
HRESULT WINAPI glDirect3DDevice3_SetRenderTarget(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpNewRenderTarget,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetRenderTarget(This->glD3DDev7, ((glDirectDrawSurface4*)lpNewRenderTarget)->GetDDS7(),dwFlags));
}

HRESULT WINAPI glDirect3DDevice3_SetTexture(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture)
{
	TRACE_ENTER(3,14,This,8,dwStage,14,lpTexture);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDrawSurface7 *dds7;
	if(lpTexture) dds7 = ((glDirect3DTexture2*)lpTexture)->glDDS7;
	else dds7 = NULL;
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetTexture(This->glD3DDev7, dwStage,dds7));
}

HRESULT WINAPI glDirect3DDevice3_SetTextureStageState(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	TRACE_ENTER(4,14,This,8,dwStage,28,dwState,9,dwValue);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetTextureStageState(This->glD3DDev7, dwStage,dwState,dwValue));
}

HRESULT WINAPI glDirect3DDevice3_SetTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,27,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice3_ValidateDevice(glDirect3DDevice3 *This, LPDWORD lpdwPasses)
{
	TRACE_ENTER(2,14,This,14,lpdwPasses);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_ValidateDevice(This->glD3DDev7, lpdwPasses));
}

HRESULT WINAPI glDirect3DDevice3_Vertex(glDirect3DDevice3 *This, LPVOID lpVertex)
{
	TRACE_ENTER(2,14,This,14,lpVertex);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Vertex(This->glD3DDev7, lpVertex));
}

// IDirect3DDevice2 wrapper
glDirect3DDevice2Vtbl glDirect3DDevice2_iface =
{
	glDirect3DDevice2_QueryInterface,
	glDirect3DDevice2_AddRef,
	glDirect3DDevice2_Release,
	glDirect3DDevice2_GetCaps,
	glDirect3DDevice2_SwapTextureHandles,
	glDirect3DDevice2_GetStats,
	glDirect3DDevice2_AddViewport,
	glDirect3DDevice2_DeleteViewport,
	glDirect3DDevice2_NextViewport,
	glDirect3DDevice2_EnumTextureFormats,
	glDirect3DDevice2_BeginScene,
	glDirect3DDevice2_EndScene,
	glDirect3DDevice2_GetDirect3D,
	glDirect3DDevice2_SetCurrentViewport,
	glDirect3DDevice2_GetCurrentViewport,
	glDirect3DDevice2_SetRenderTarget,
	glDirect3DDevice2_GetRenderTarget,
	glDirect3DDevice2_Begin,
	glDirect3DDevice2_BeginIndexed,
	glDirect3DDevice2_Vertex,
	glDirect3DDevice2_Index,
	glDirect3DDevice2_End,
	glDirect3DDevice2_GetRenderState,
	glDirect3DDevice2_SetRenderState,
	glDirect3DDevice2_GetLightState,
	glDirect3DDevice2_SetLightState,
	glDirect3DDevice2_SetTransform,
	glDirect3DDevice2_GetTransform,
	glDirect3DDevice2_MultiplyTransform,
	glDirect3DDevice2_DrawPrimitive,
	glDirect3DDevice2_DrawIndexedPrimitive,
	glDirect3DDevice2_SetClipStatus,
	glDirect3DDevice2_GetClipStatus
};

HRESULT glDirect3DDevice2_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice2 **newdev)
{
	TRACE_ENTER(1, 14, glD3DDev7, 14, newdev);
	*newdev = (glDirect3DDevice2*)malloc(sizeof(glDirect3DDevice2));
	if (!(*newdev)) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	(*newdev)->glD3DDev7 = glD3DDev7;
	(*newdev)->lpVtbl = &glDirect3DDevice2_iface;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice2_QueryInterface(glDirect3DDevice2 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DDevice2_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DDevice7_QueryInterface(This->glD3DDev7, riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice2_AddRef(glDirect3DDevice2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_AddRef(This->glD3DDev7));
}

ULONG WINAPI glDirect3DDevice2_Release(glDirect3DDevice2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_Release(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice2_AddViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glDirect3DDevice7_AddViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3)glD3DV3);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_EXIT(23,ret);
	return ret;
}

DWORD d3dvttofvf(D3DVERTEXTYPE d3dvt)
{
	switch(d3dvt)
	{
	case D3DVT_VERTEX:
		return D3DFVF_VERTEX;
	case D3DVT_LVERTEX:
		return D3DFVF_LVERTEX;
	case D3DVT_TLVERTEX:
		return D3DFVF_TLVERTEX;
	default:
		return 0;
	}
}

HRESULT WINAPI glDirect3DDevice2_Begin(glDirect3DDevice2 *This, D3DPRIMITIVETYPE d3dpt, D3DVERTEXTYPE d3dvt, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,8,d3dpt,8,d3dvt,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(d3dvt);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Begin(This->glD3DDev7, d3dpt,vertextype,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_BeginIndexed(glDirect3DDevice2 *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,8,dvtVertexType,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_BeginIndexed(This->glD3DDev7, dptPrimitiveType,vertextype,lpvVertices,dwNumVertices,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_BeginScene(glDirect3DDevice2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_BeginScene(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice2_DeleteViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDirect3DViewport2) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glDirect3DDevice7_DeleteViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3)glD3DV3);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2_DrawIndexedPrimitive(glDirect3DDevice2 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,This,8,d3dptPrimitiveType,8,d3dvtVertexType,14,lpvVertices,8,dwVertexCount,14,dwIndices,8,dwIndexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(d3dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawIndexedPrimitive(This->glD3DDev7, d3dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_DrawPrimitive(glDirect3DDevice2 *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,This,8,dptPrimitiveType,8,dvtVertexType,14,lpvVertices,8,dwVertexCount,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DrawPrimitive(This->glD3DDev7, dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_End(glDirect3DDevice2 *This, DWORD dwFlags)
{
	TRACE_ENTER(2,14,This,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_End(This->glD3DDev7, dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_EndScene(glDirect3DDevice2 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_EndScene(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice2_EnumTextureFormats(glDirect3DDevice2 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,This,14,lpd3dEnumTextureProc,14,lpArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirect3DDevice7_EnumTextureFormats2(This->glD3DDev7, lpd3dEnumTextureProc, lpArg));
}

HRESULT WINAPI glDirect3DDevice2_GetCaps(glDirect3DDevice2 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,This,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetCaps3(This->glD3DDev7, lpD3DHWDevDesc,lpD3DHELDevDesc));
}

HRESULT WINAPI glDirect3DDevice2_GetClipStatus(glDirect3DDevice2 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetClipStatus(This->glD3DDev7, lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice2_GetCurrentViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 *lplpd3dViewport2)
{
	TRACE_ENTER(2,14,This,14,lplpd3dViewport2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirect3DViewport3 *glD3DV3;
	HRESULT ret = glDirect3DDevice7_GetCurrentViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3*)&glD3DV3);
	if(!glD3DV3) TRACE_RET(HRESULT,23,ret);
	glDirect3DViewport3_QueryInterface(glD3DV3,IID_IDirect3DViewport2,(void**)lplpd3dViewport2);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_VAR("*lplpd3dViewport2",14,*lplpd3dViewport2);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2_GetDirect3D(glDirect3DDevice2 *This, LPDIRECT3D2 *lplpD3D2)
{
	TRACE_ENTER(2,14,This,14,lplpD3D2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glDirect3DDevice7_GetDirect3D(This->glD3DDev7, &d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D2,(void**)lplpD3D2);
	d3d7->Release();
	TRACE_VAR("*lplpD3D2",14,*lplpD3D2);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice2_GetLightState(glDirect3DDevice2 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,14,lpdwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetLightState(This->glD3DDev7, dwLightStateType,lpdwLightState));
}

HRESULT WINAPI glDirect3DDevice2_GetRenderState(glDirect3DDevice2 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,This,27,dwRenderStateType,14,lpdwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetRenderState(This->glD3DDev7, dwRenderStateType,lpdwRenderState));
}

HRESULT WINAPI glDirect3DDevice2_GetRenderTarget(glDirect3DDevice2 *This, LPDIRECTDRAWSURFACE *lplpRenderTarget)
{
	TRACE_ENTER(2,14,This,14,lplpRenderTarget);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glDirect3DDevice7_GetRenderTarget(This->glD3DDev7, &dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirectDrawSurface,(void**)lplpRenderTarget);
	dds7->Release();
	TRACE_VAR("*lplpRenderTarget",14,*lplpRenderTarget);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice2_GetStats(glDirect3DDevice2 *This, LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,This,14,lpD3DStats);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetStats(This->glD3DDev7, lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice2_GetTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2_Index(glDirect3DDevice2 *This, WORD wVertexIndex)
{
	TRACE_ENTER(2,14,This,5,wVertexIndex);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Index(This->glD3DDev7, wVertexIndex));
}

HRESULT WINAPI glDirect3DDevice2_MultiplyTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_MultiplyTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2_NextViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DViewport2,14,lplpDirect3DViewport2,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice2_NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT WINAPI glDirect3DDevice2_SetClipStatus(glDirect3DDevice2 *This, LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,This,14,lpD3DClipStatus);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetClipStatus(This->glD3DDev7, lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice2_SetCurrentViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpd3dViewport2)
{
	TRACE_ENTER(2,14,This,14,lpd3dViewport2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dViewport2) TRACE_RET(HRESULT,23,glDirect3DDevice7_SetCurrentViewport(This->glD3DDev7, NULL));
	glDirect3DViewport3 *glD3DV3;
	lpd3dViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glDirect3DDevice7_SetCurrentViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3)glD3DV3);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2_SetLightState(glDirect3DDevice2 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,This,30,dwLightStateType,9,dwLightState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetLightState(This->glD3DDev7, dwLightStateType,dwLightState));
}

HRESULT WINAPI glDirect3DDevice2_SetRenderState(glDirect3DDevice2 *This, D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState)
{
	TRACE_ENTER(3,14,This,27,dwRenderStateType,9,dwRenderState);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetRenderState(This->glD3DDev7, dwRenderStateType,dwRenderState));
}

HRESULT WINAPI glDirect3DDevice2_SetRenderTarget(glDirect3DDevice2 *This, LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,This,14,lpNewRenderTarget,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetRenderTarget(This->glD3DDev7, ((glDirectDrawSurface1*)lpNewRenderTarget)->GetDDS7(),dwFlags));
}

HRESULT WINAPI glDirect3DDevice2_SetTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetTransform(This->glD3DDev7, dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2_SwapTextureHandles(glDirect3DDevice2 *This, LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	TRACE_ENTER(3,14,This,14,lpD3DTex1,14,lpD3DTex2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SwapTextureHandles(This->glD3DDev7, lpD3DTex1,lpD3DTex2));
}

HRESULT WINAPI glDirect3DDevice2_Vertex(glDirect3DDevice2 *This, LPVOID lpVertexType)
{
	TRACE_ENTER(2,14,This,14,lpVertexType);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Vertex(This->glD3DDev7, lpVertexType));
}

// IDirect3DDevice wrapper
glDirect3DDevice1Vtbl glDirect3DDevice1_iface =
{
	glDirect3DDevice1_QueryInterface,
	glDirect3DDevice1_AddRef,
	glDirect3DDevice1_Release,
	glDirect3DDevice1_Initialize,
	glDirect3DDevice1_GetCaps,
	glDirect3DDevice1_SwapTextureHandles,
	glDirect3DDevice1_CreateExecuteBuffer,
	glDirect3DDevice1_GetStats,
	glDirect3DDevice1_Execute,
	glDirect3DDevice1_AddViewport,
	glDirect3DDevice1_DeleteViewport,
	glDirect3DDevice1_NextViewport,
	glDirect3DDevice1_Pick,
	glDirect3DDevice1_GetPickRecords,
	glDirect3DDevice1_EnumTextureFormats,
	glDirect3DDevice1_CreateMatrix,
	glDirect3DDevice1_SetMatrix,
	glDirect3DDevice1_GetMatrix,
	glDirect3DDevice1_DeleteMatrix,
	glDirect3DDevice1_BeginScene,
	glDirect3DDevice1_EndScene,
	glDirect3DDevice1_GetDirect3D
};

HRESULT glDirect3DDevice1_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice1 **newdev)
{ 
	TRACE_ENTER(1, 14, glD3DDev7, 14, newdev);
	*newdev = (glDirect3DDevice1*)malloc(sizeof(glDirect3DDevice1));
	if (!(*newdev)) TRACE_RET(HRESULT, 23, DDERR_OUTOFMEMORY);
	(*newdev)->glD3DDev7 = glD3DDev7;
	(*newdev)->lpVtbl = &glDirect3DDevice1_iface;
	TRACE_EXIT(23, D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice1_QueryInterface(glDirect3DDevice1 *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,This,24,&riid,14,ppvObj);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		glDirect3DDevice1_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glDirect3DDevice7_QueryInterface(This->glD3DDev7, riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice1_AddRef(glDirect3DDevice1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_AddRef(This->glD3DDev7));
}

ULONG WINAPI glDirect3DDevice1_Release(glDirect3DDevice1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(ULONG,8,0);
	TRACE_RET(ULONG, 8, glDirect3DDevice7_Release(This->glD3DDev7));
}

HRESULT WINAPI glDirect3DDevice1_AddViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glDirect3DDevice7_AddViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3)glD3DV3);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DDevice1_BeginScene(glDirect3DDevice1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_BeginScene(This->glD3DDev7));
}
HRESULT WINAPI glDirect3DDevice1_CreateExecuteBuffer(glDirect3DDevice1 *This, LPD3DEXECUTEBUFFERDESC lpDesc,
		LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,	IUnknown* pUnkOuter)
{
	TRACE_ENTER(4,14,This,14,lpDesc,14,lplpDirect3DExecuteBuffer,14,pUnkOuter);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_CreateExecuteBuffer(This->glD3DDev7, lpDesc,lplpDirect3DExecuteBuffer,pUnkOuter));
}
HRESULT WINAPI glDirect3DDevice1_CreateMatrix(glDirect3DDevice1 *This, LPD3DMATRIXHANDLE lpD3DMatHandle)
{
	TRACE_ENTER(2,14,This,14,lpD3DMatHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_CreateMatrix(This->glD3DDev7, lpD3DMatHandle));
}
HRESULT WINAPI glDirect3DDevice1_DeleteMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle)
{
	TRACE_ENTER(2,14,This,9,d3dMatHandle);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_DeleteMatrix(This->glD3DDev7, d3dMatHandle));
}
HRESULT WINAPI glDirect3DDevice1_DeleteViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
	TRACE_ENTER(2,14,This,14,lpDirect3DViewport);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if (!lpDirect3DViewport) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glDirect3DDevice7_DeleteViewport(This->glD3DDev7, (LPDIRECT3DVIEWPORT3)glD3DV3);
	glDirect3DViewport3_Release(glD3DV3);
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DDevice1_EndScene(glDirect3DDevice1 *This)
{
	TRACE_ENTER(1,14,This);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_EndScene(This->glD3DDev7));
}
HRESULT WINAPI glDirect3DDevice1_EnumTextureFormats(glDirect3DDevice1 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,This,14,lpd3dEnumTextureProc,14,lpArg);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, glDirect3DDevice7_EnumTextureFormats2(This->glD3DDev7, lpd3dEnumTextureProc, lpArg));
}
HRESULT WINAPI glDirect3DDevice1_Execute(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Execute(This->glD3DDev7, lpDirect3DExecuteBuffer,lpDirect3DViewport,dwFlags));
}
HRESULT WINAPI glDirect3DDevice1_GetCaps(glDirect3DDevice1 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,This,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetCaps3(This->glD3DDev7, lpD3DHWDevDesc,lpD3DHELDevDesc));
}
HRESULT WINAPI glDirect3DDevice1_GetDirect3D(glDirect3DDevice1 *This, LPDIRECT3D* lpD3D)
{
	TRACE_ENTER(2,14,This,14,lpD3D);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glDirect3DDevice7_GetDirect3D(This->glD3DDev7, &d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D,(void**)lpD3D);
	d3d7->Release();
	TRACE_VAR("*lpD3D",14,*lpD3D);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice1_GetMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,9,lpD3DMatHandle,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetMatrix(This->glD3DDev7, lpD3DMatHandle,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice1_GetPickRecords(glDirect3DDevice1 *This, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec)
{
	TRACE_ENTER(3,14,This,14,lpCount,14,lpD3DPickRec);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetPickRecords(This->glD3DDev7, lpCount,lpD3DPickRec));
}

HRESULT WINAPI glDirect3DDevice1_GetStats(glDirect3DDevice1 *This, LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,This,14,lpD3DStats);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_GetStats(This->glD3DDev7, lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice1_Initialize(glDirect3DDevice1 *This, LPDIRECT3D lpd3d, LPGUID lpGUID, LPD3DDEVICEDESC lpd3ddvdesc)
{
	TRACE_ENTER(4,14,This,14,lpd3d,24,lpGUID,14,lpd3ddvdesc);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
}
HRESULT WINAPI glDirect3DDevice1_NextViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport, LPDIRECT3DVIEWPORT *lplpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,This,14,lpDirect3DViewport,14,lplpDirect3DViewport,9,dwFlags);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice1_NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice1_Pick(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport,
	DWORD dwFlags, LPD3DRECT lpRect)
{
	TRACE_ENTER(5,14,This,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags,26,lpRect);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_Pick(This->glD3DDev7, lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags, lpRect));
}

HRESULT WINAPI glDirect3DDevice1_SetMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,This,9,d3dMatHandle,14,lpD3DMatrix);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glDirect3DDevice7_SetMatrix(This->glD3DDev7, d3dMatHandle,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice1_SwapTextureHandles(glDirect3DDevice1 *This, LPDIRECT3DTEXTURE lpD3DTex1, LPDIRECT3DTEXTURE lpD3DTex2)
{
	TRACE_ENTER(3,14,This,14,lpD3DTex1,14,lpD3DTex2);
	if(!This) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTex1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpD3DTex2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DTEXTURE2 tex1, tex2;
	lpD3DTex1->QueryInterface(IID_IDirect3DTexture2,(void**)&tex1);
	lpD3DTex2->QueryInterface(IID_IDirect3DTexture2,(void**)&tex2);
	HRESULT ret = glDirect3DDevice7_SwapTextureHandles(This->glD3DDev7, tex1,tex2);
	tex2->Release();
	tex1->Release();
	TRACE_EXIT(23,ret);
	return ret;
}