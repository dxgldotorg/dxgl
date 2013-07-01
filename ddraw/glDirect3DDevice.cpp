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
#include "ddraw.h"
#include "glDirect3D.h"
#include "texture.h"
#include "glutil.h"
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
#include "shadergen.h"
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

extern D3DDEVICEDESC7 d3ddesc;

const DWORD renderstate_default[153] = {0, // 0
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
	false,
	0,
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
	false,
	0,
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

glDirect3DDevice7::glDirect3DDevice7(REFCLSID rclsid, glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7)
{
	TRACE_ENTER(4,14,this,24,&rclsid,14,glD3D7,14,glDDS7);
	int zbuffer = 0;
	glD3DDev3 = NULL;
	glD3DDev2 = NULL;
	glD3DDev1 = NULL;
	maxmaterials = 32;
	materials = (glDirect3DMaterial3**)malloc(32*sizeof(glDirect3DMaterial3*));
	if(!materials)
	{
		error = DDERR_OUTOFMEMORY;
		TRACE_EXIT(-1,0);
		return;
	}
	materialcount = 1;
	materials[0] = NULL;
	currentmaterial = NULL;
	maxtextures = 32;
	textures = (glDirectDrawSurface7**)malloc(32*sizeof(glDirectDrawSurface7*));
	if(!textures)
	{
		free(materials);
		error = DDERR_OUTOFMEMORY;
		TRACE_EXIT(-1,0);
		return;
	}
	texturecount = 1;
	textures[0] = NULL;
	maxviewports = 32;
	currentviewport = NULL;
	viewportcount = 0;
	viewports = (glDirect3DViewport3**)malloc(32*sizeof(glDirect3DViewport3*));
	if(!viewports)
	{
		free(materials);
		free(textures);
		error = DDERR_OUTOFMEMORY;
		TRACE_EXIT(-1,0);
		return;
	}
	ZeroMemory(viewports,32*sizeof(glDirect3DViewport3*));
	vertices = normals = NULL;
	diffuse = specular = NULL;
	ebBuffer = NULL;
	ebBufferSize = 0;
	outbuffer = NULL;
	outbuffersize = 0;
	ZeroMemory(texcoords,8*sizeof(GLfloat*));
	memcpy(renderstate,renderstate_default,153*sizeof(DWORD));
	__gluMakeIdentityf(matWorld);
	__gluMakeIdentityf(matView);
	__gluMakeIdentityf(matProjection);
	transform_dirty = true;
	matrices = NULL;
	matrixcount = 0;
	texstages[0] = texstagedefault0;
	texstages[1] = texstages[2] = texstages[3] = texstages[4] = 
		texstages[5] = texstages[6] = texstages[7] = texstagedefault1;
	refcount = 1;
	inscene = false;
	modelview_dirty = false;
	projection_dirty = false;
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	this->glDDS7 = glDDS7;
	glDDS7->AddRef();
	ZeroMemory(&viewport,sizeof(D3DVIEWPORT7));
	if(glDDS7->GetZBuffer()) zbuffer = 1;
	ZeroMemory(&material,sizeof(D3DMATERIAL7));
	lightsmax = 16;
	lights = (glDirect3DLight**) malloc(16*sizeof(glDirect3DLight*));
	if(!lights)
	{
		free(materials);
		free(textures);
		free(viewports);
		error = DDERR_OUTOFMEMORY;
		TRACE_EXIT(-1,0);
		return;
	}
	ZeroMemory(lights,16*sizeof(glDirect3DLight*));
	memset(gllights,0xff,8*sizeof(int));
	memset(gltextures,0,8*sizeof(GLuint));
	ZeroMemory(&stats,sizeof(D3DSTATS));
	stats.dwSize = sizeof(D3DSTATS);
	d3ddesc.dwMaxTextureWidth = d3ddesc.dwMaxTextureHeight =
		d3ddesc.dwMaxTextureRepeat = d3ddesc.dwMaxTextureAspectRatio = renderer->gl_caps.TextureMax;
	d3ddesc3.dwMaxTextureWidth = d3ddesc3.dwMaxTextureHeight =
		d3ddesc3.dwMaxTextureRepeat = d3ddesc3.dwMaxTextureAspectRatio = renderer->gl_caps.TextureMax;
	scalex = scaley = 0;
	mhWorld = mhView = mhProjection = 0;
	renderer->InitD3D(zbuffer);
	error = D3D_OK;
	TRACE_EXIT(-1,0);
}
glDirect3DDevice7::~glDirect3DDevice7()
{
	TRACE_ENTER(1,14,this);
	for(int i = 0; i < lightsmax; i++)
		if(lights[i]) delete lights[i];
	delete lights;
	for(int i = 0; i < 8; i++)
		if(texstages[i].texture) texstages[i].texture->Release();
	for(int i = 0; i < materialcount; i++)
	{
		if(materials[i])
		{
			materials[i]->unbind();
			materials[i]->Release();
		}
	}
	for(int i = 0; i < viewportcount; i++)
	{
		if(viewports[i])
		{
			viewports[i]->SetDevice(NULL);
			viewports[i]->SetCurrent(false);
			viewports[i]->Release();
		}
	}
	for(int i = 0; i < texturecount; i++)
	{
		if(textures[i])
		{
			textures[i]->Release();
		}
	}
	free(viewports);
	free(materials);
	free(textures);
	if(matrices) free(matrices);
	glD3D7->Release();
	glDDS7->Release();
	TRACE_EXIT(-1,0);
}

int ExpandLightBuffer(glDirect3DLight ***lights, DWORD *maxlights, DWORD newmax)
{
	if(newmax < *maxlights) return 1;
	glDirect3DLight **tmp = (glDirect3DLight**)realloc(*lights,newmax*sizeof(glDirect3DLight*));
	if(!tmp) return 0;
	*lights = tmp;
	for(DWORD i = *maxlights; i < newmax; i++)
		lights[i] = NULL;
	*maxlights = newmax;
	return 1;
}

HRESULT WINAPI glDirect3DDevice7::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!ppvObj) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DDevice7)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(riid == IID_IDirect3DDevice3)
	{
		if(glD3DDev3)
		{
			*ppvObj = glD3DDev3;
			glD3DDev3->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DDevice3(this);
			glD3DDev3 = (glDirect3DDevice3*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DDevice2)
	{
		if(glD3DDev2)
		{
			*ppvObj = glD3DDev2;
			glD3DDev2->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DDevice2(this);
			glD3DDev2 = (glDirect3DDevice2*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DDevice)
	{
		if(glD3DDev1)
		{
			*ppvObj = glD3DDev1;
			glD3DDev1->AddRef();
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DDevice1(this);
			glD3DDev1 = (glDirect3DDevice1*)*ppvObj;
			TRACE_VAR("*ppvObj",14,*ppvObj);
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,E_NOINTERFACE);
	return E_NOINTERFACE;
}

ULONG WINAPI glDirect3DDevice7::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}
ULONG WINAPI glDirect3DDevice7::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice7::ApplyStateBlock(DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,this,9,dwBlockHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::ApplyStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::BeginScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_IN_SCENE);
	inscene = true;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::BeginStateBlock()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::BeginStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CaptureStateBlock(DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,this,9,dwBlockHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::CaptureStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle)
{
	TRACE_ENTER(3,14,this,9,d3dsbtype,14,lpdwBlockHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::CreateStateBlock: stub");
	TRACE_EXIT(23,D3D_OK);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	TRACE_ENTER(7,14,this,8,dwCount,14,lpRects,9,dwFlags,9,dwColor,19,&dvZ,9,dwStencil);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwCount && !lpRects) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,renderer->Clear(glDDS7,dwCount,lpRects,dwFlags,dwColor,dvZ,dwStencil));
}
HRESULT WINAPI glDirect3DDevice7::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	TRACE_ENTER(6,14,this,14,lpCenters,14,lpRadii,8,dwNumSpheres,9,dwFlags,14,lpdwReturnValues);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::ComputeSphereVisibility: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DeleteStateBlock(DWORD dwBlockHandle)
{
	TRACE_ENTER(2,14,this,9,dwBlockHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::DeleteStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

void glDirect3DDevice7::SetArraySize(DWORD size, DWORD vertex, DWORD texcoord)
{
	TRACE_ENTER(4,14,this,8,size,8,vertex,8,texcoord);
	if(!vertices) vertices = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > maxarray) vertices = (GLfloat*)realloc(vertices,size*4*sizeof(GLfloat));
	if(!normals) normals = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > maxarray) normals = (GLfloat*)realloc(normals,size*4*sizeof(GLfloat));
	TRACE_EXIT(0,0);
}

__int64 glDirect3DDevice7::SelectShader(GLVERTEX *VertexType)
{
	TRACE_ENTER(2,14,this,14,VertexType);
	int i;
	__int64 shader = 0;
	switch(renderstate[D3DRENDERSTATE_SHADEMODE])
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
	if(renderstate[D3DRENDERSTATE_ALPHATESTENABLE]) shader |= 4;
	shader |= ((((__int64)renderstate[D3DRENDERSTATE_ALPHAFUNC]-1) & 7) << 3);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGTABLEMODE] & 3) << 6);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGVERTEXMODE] & 3) << 8);
	if(renderstate[D3DRENDERSTATE_RANGEFOGENABLE]) shader |= (1i64 << 10);
	if(renderstate[D3DRENDERSTATE_SPECULARENABLE]) shader |= (1i64 << 11);
	if(renderstate[D3DRENDERSTATE_STIPPLEDALPHA]) shader |= (1i64 << 12);
	if(renderstate[D3DRENDERSTATE_COLORKEYENABLE]) shader |= (1i64 << 13);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_ZBIAS] & 15) << 14);
	int numlights = 0;
	for(i = 0; i < 8; i++)
		if(gllights[i] != -1) numlights++;
	shader |= (__int64)numlights << 18;
	if(renderstate[D3DRENDERSTATE_LOCALVIEWER]) shader |= (1i64 << 21);
	if(renderstate[D3DRENDERSTATE_COLORKEYBLENDENABLE]) shader |= (1i64 << 22);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] & 3) << 23);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_SPECULARMATERIALSOURCE] & 3) << 25);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] & 3) << 27);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] & 3) << 29);
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
		if(gllights[i] != -1)
		{
			if(lights[gllights[i]]->light.dltType != D3DLIGHT_DIRECTIONAL)
				shader |= (1i64 << (38+lightindex));
			if(lights[gllights[i]]->light.dltType == D3DLIGHT_SPOT)
				shader |= (1i64 << (51+lightindex));
			lightindex++;
		}
	}
	int blendweights = 0;
	for(i = 0; i < 5; i++)
		if(VertexType[i+2].data) blendweights++;
	shader |= (__int64)blendweights << 46;
	if(renderstate[D3DRENDERSTATE_NORMALIZENORMALS]) shader |= (1i64 << 49);
	if(VertexType[1].data) shader |= (1i64 << 50);
	if(renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] == D3DTBLEND_MODULATE)
	{
		bool noalpha = false;;
		if(!texstages[0].texture) noalpha = true;
		if(texstages[0].texture)
			if(!(texstages[0].texture->ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
				noalpha = true;
		if(noalpha) texstages[0].alphaop = D3DTOP_SELECTARG2;
		else texstages[0].alphaop = D3DTOP_MODULATE;
	}
	if(renderstate[D3DRENDERSTATE_LIGHTING]) shader |= (1i64 << 59);
	if(renderstate[D3DRENDERSTATE_COLORVERTEX]) shader |= (1i64 << 60);
	if(renderstate[D3DRENDERSTATE_FOGENABLE]) shader |= (1i64 << 61);
	for(i = 0; i < 8; i++)
	{
		if(!texstages[i].dirty) continue;
		texstages[i].shaderid = texstages[i].colorop & 31;
		texstages[i].shaderid |= (__int64)(texstages[i].colorarg1 & 63) << 5;
		texstages[i].shaderid |= (__int64)(texstages[i].colorarg2 & 63) << 11;
		texstages[i].shaderid |= (__int64)(texstages[i].alphaop & 31) << 17;
		texstages[i].shaderid |= (__int64)(texstages[i].alphaarg1 & 63) << 22;
		texstages[i].shaderid |= (__int64)(texstages[i].alphaarg2 & 63) << 28;
		texstages[i].shaderid |= (__int64)(texstages[i].texcoordindex & 7) << 34;
		texstages[i].shaderid |= (__int64)((texstages[i].texcoordindex >> 16) & 3) << 37;
		texstages[i].shaderid |= (__int64)((texstages[i].addressu - 1) & 3) << 39;
		texstages[i].shaderid |= (__int64)((texstages[i].addressv - 1) & 3) << 41;
		texstages[i].shaderid |= (__int64)(texstages[i].magfilter & 7) << 43;
		texstages[i].shaderid |= (__int64)(texstages[i].minfilter & 3) << 46;
		texstages[i].shaderid |= (__int64)(texstages[i].mipfilter & 3) << 48;
		if(texstages[i].textransform & 7)
		{
			texstages[i].shaderid |= 1i64 << 50;
			texstages[i].shaderid |= (__int64)(((texstages[i].textransform & 7) - 1)& 3) << 51;
		}
		if(texstages[i].textransform & D3DTTFF_PROJECTED) texstages[i].shaderid |= 1i64 << 53;
		texstages[i].shaderid |= (__int64)(texstages[i].texcoordindex&7) << 54;
		texstages[i].shaderid |= (__int64)((texstages[i].texcoordindex>>16)&3) << 57;
		if(texstages[i].texture)
		{
			texstages[i].shaderid |= 1i64 << 59;
			if(texstages[i].texture->ddsd.dwFlags & DDSD_CKSRCBLT) texstages[i].shaderid |= 1i64 << 60;
		}
	}
	TRACE_EXIT(10,&shader);
	return shader;
}

HRESULT glDirect3DDevice7::fvftoglvertex(DWORD dwVertexTypeDesc,LPDWORD vertptr)
{
	TRACE_ENTER(3,14,this,9,dwVertexTypeDesc,14,vertptr);
	int i;
	int ptr = 0;
	if((dwVertexTypeDesc & D3DFVF_XYZ) && (dwVertexTypeDesc & D3DFVF_XYZRHW))
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwVertexTypeDesc & D3DFVF_XYZ)
	{
		vertdata[0].data = vertptr;
		vertdata[1].data = NULL;
		ptr += 3;
		if(dwVertexTypeDesc & D3DFVF_RESERVED1) ptr++;
	}
	else if(dwVertexTypeDesc & D3DFVF_XYZRHW)
	{
		vertdata[0].data = vertptr;
		vertdata[1].data = &vertptr[3];
		ptr += 4;
	}
	else TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(i = 0; i < 5; i++)
		vertdata[i+2].data = NULL;
	if(((dwVertexTypeDesc >> 1) & 7) >= 3)
	{
		for(i = 0; i < (signed)(((dwVertexTypeDesc >> 1) & 7) - 2); i++)
		{
			vertdata[((dwVertexTypeDesc >> 1) & 7)].data = &vertptr[ptr];
			ptr++;
		}
	}
	if(dwVertexTypeDesc & D3DFVF_NORMAL)
	{
		vertdata[7].data = &vertptr[ptr];
		ptr += 3;
	}
	else vertdata[7].data = NULL;
	if(dwVertexTypeDesc & D3DFVF_DIFFUSE)
	{
		vertdata[8].data = &vertptr[ptr];
		ptr++;
	}
	else vertdata[8].data = NULL;
	if(dwVertexTypeDesc & D3DFVF_SPECULAR)
	{
		vertdata[9].data = &vertptr[ptr];
		ptr++;
	}
	else vertdata[9].data = NULL;
	for(i = 0; i < 8; i++)
		vertdata[i+10].data = NULL;
	int numtex = (dwVertexTypeDesc&D3DFVF_TEXCOUNT_MASK)>>D3DFVF_TEXCOUNT_SHIFT;
	for(i = 0; i < 8; i++)
	{
		vertdata[i+10].data = &vertptr[ptr];
		if(i >= numtex) texformats[i] = -1;
		else texformats[i] = (dwVertexTypeDesc>>(16+(2*i))&3);
		switch(texformats[i])
		{
		case 0: // st
			ptr += 2;
			break;
		case 1: // str
			ptr += 3;
			break;
		case 2: // strq
			ptr += 4;
			break;
		case 3: // s
			ptr++;
			break;
		}
	}
	int stride = NextMultipleOf8(ptr*4);
	for(i = 0; i < 17; i++)
		vertdata[i].stride = stride;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,9,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_NOT_IN_SCENE);
	HRESULT err = fvftoglvertex(dwVertexTypeDesc,(LPDWORD)lpvVertices);
	if(lpwIndices) AddStats(d3dptPrimitiveType,dwIndexCount,&stats);
	else AddStats(d3dptPrimitiveType,dwVertexCount,&stats);
	if(err != D3D_OK) TRACE_RET(HRESULT,23,err);
	TRACE_RET(HRESULT,23,renderer->DrawPrimitives(this,setdrawmode(d3dptPrimitiveType),vertdata,texformats,
		dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,9,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertexArray,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,9,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveVB: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,9,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertices,8,dwVertexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,DrawIndexedPrimitive(dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,NULL,0,dwFlags));
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,9,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::DrawPrimitiveStrided: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(5,14,this,9,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::DrawPrimitiveVB: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::EndScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!inscene) TRACE_RET(HRESULT,23,D3DERR_SCENE_NOT_IN_SCENE);
	inscene = false;
	renderer->Flush();
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::EndStateBlock(LPDWORD lpdwBlockHandle)
{
	TRACE_ENTER(2,14,this,14,lpdwBlockHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::EndStateBlock: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,this,14,lpd3dEnumPixelProc,14,lpArg);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	HRESULT result;
	DDPIXELFORMAT fmt;
	for(int i = 0; i < numtexformats; i++)
	{
		if(::texformats[i].dwFlags & DDPF_ZBUFFER) continue;
		if(::texformats[i].dwFlags & DDPF_PALETTEINDEXED8) continue;
		memcpy(&fmt,&::texformats[i],sizeof(DDPIXELFORMAT));
		result = lpd3dEnumPixelProc(&fmt,lpArg);
		if(result != D3DENUMRET_OK) TRACE_RET(HRESULT,23,D3D_OK);
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	TRACE_ENTER(2,14,this,14,lpD3DDevDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	D3DDEVICEDESC7 desc = d3ddesc;
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	desc.deviceGUID = IID_IDirect3DTnLHalDevice;
	memcpy(lpD3DDevDesc,&desc,sizeof(D3DDEVICEDESC7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation)
{
	TRACE_ENTER(3,14,this,8,dwIndex,9,pPlaneEquation);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::GetClipPlane: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::GetClipStatus: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetDirect3D(LPDIRECT3D7 *lplpD3D)
{
	TRACE_ENTER(2,14,this,14,lplpD3D);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpD3D) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lplpD3D = glD3D7;
	glD3D7->AddRef();
	TRACE_VAR("*lplpD3D",14,*lplpD3D);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize)
{
	TRACE_ENTER(4,14,this,9,dwDevInfoID,14,pDevInfoStruct,8,dwSize);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::GetInfo: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	TRACE_ENTER(3,14,this,8,dwLightIndex,14,lpLight);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpLight) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwLightIndex >= lightsmax) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lights[dwLightIndex]) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	lights[dwLightIndex]->GetLight7(lpLight);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable)
{
	TRACE_ENTER(3,14,this,8,dwLightIndex,14,pbEnable);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwLightIndex >= lightsmax) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lights[dwLightIndex]) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!pbEnable) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*pbEnable = FALSE;
	for(int i = 0; i < 8; i++)
		if(gllights[i] == dwLightIndex) *pbEnable = TRUE;
	TRACE_VAR("*pbEnable",22,*pbEnable);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	TRACE_ENTER(2,14,this,14,lpMaterial);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpMaterial,&material,sizeof(D3DMATERIAL7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRenderStateType,14,lpdwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwRenderStateType <= 152)
	{
		*lpdwRenderState = renderstate[dwRenderStateType];
		TRACE_VAR("*lpdwRenderState",9,*lpdwRenderState);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderTarget(LPDIRECTDRAWSURFACE7 *lplpRenderTarget)
{
	TRACE_ENTER(2,14,this,14,lplpRenderTarget);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpRenderTarget) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDDS7->AddRef();
	*lplpRenderTarget = glDDS7;
	TRACE_VAR("*lplpRenderTarger",14,*lplpRenderTarget);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData)
{
	TRACE_ENTER(3,14,this,9,dwState,14,lplpStateData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::GetStateData: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	TRACE_ENTER(3,14,this,8,dwStage,14,lplpTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpTexture) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!texstages[dwStage].texture) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	*lplpTexture = texstages[dwStage].texture;
	texstages[dwStage].texture->AddRef();
	TRACE_VAR("*lplpTexture",14,*lplpTexture);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	TRACE_ENTER(4,14,this,8,dwStage,28,dwState,14,lpdwValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpdwValue) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		*lpdwValue = texstages[dwStage].colorop;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_COLORARG1:
		*lpdwValue = texstages[dwStage].colorarg1;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_COLORARG2:
		*lpdwValue = texstages[dwStage].colorarg2;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAOP:
		*lpdwValue = texstages[dwStage].alphaop;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAARG1:
		*lpdwValue = texstages[dwStage].alphaarg1;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ALPHAARG2:
		*lpdwValue = texstages[dwStage].alphaarg2;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT00:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv00,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT01:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv01,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT10:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv10,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVMAT11:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv11,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_TEXCOORDINDEX:
		*lpdwValue = texstages[dwStage].texcoordindex;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ADDRESS:
	case D3DTSS_ADDRESSU:
		*lpdwValue = texstages[dwStage].addressu;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_ADDRESSV:
		*lpdwValue = texstages[dwStage].addressv;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BORDERCOLOR:
		*lpdwValue = texstages[dwStage].bordercolor;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAGFILTER:
		*lpdwValue = texstages[dwStage].magfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MINFILTER:
		*lpdwValue = texstages[dwStage].minfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MIPFILTER:
		*lpdwValue = texstages[dwStage].mipfilter;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(lpdwValue,&texstages[dwStage].lodbias,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAXMIPLEVEL:
		*lpdwValue = texstages[dwStage].miplevel;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_MAXANISOTROPY:
		*lpdwValue = texstages[dwStage].anisotropy;
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVLSCALE:
		memcpy(lpdwValue,&texstages[dwStage].bumpenvlscale,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(lpdwValue,&texstages[dwStage].bumpenvloffset,sizeof(D3DVALUE));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		*lpdwValue = texstages[dwStage].textransform;
		TRACE_RET(HRESULT,23,D3D_OK);
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(lpD3DMatrix,&matWorld,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(lpD3DMatrix,&matView,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(lpD3DMatrix,&matProjection,sizeof(D3DMATRIX));
		TRACE_RET(HRESULT,23,D3D_OK);
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetViewport(LPD3DVIEWPORT7 lpViewport)
{
	TRACE_ENTER(2,14,this,14,lpViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	memcpy(lpViewport,&viewport,sizeof(D3DVIEWPORT7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
	TRACE_ENTER(3,14,this,8,dwLightIndex,22,bEnable);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	int i;
	D3DLIGHT7 light;
	bool foundlight = false;
	if(dwLightIndex >= lightsmax)
	{
		if(!ExpandLightBuffer(&lights,&lightsmax,dwLightIndex-1)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	}
	if(!lights[dwLightIndex]) lights[dwLightIndex] = new glDirect3DLight;
	if(bEnable)
	{
		for(i = 0; i < 8; i++)
			if(gllights[i] == dwLightIndex) TRACE_RET(HRESULT,23,D3D_OK);
		for(i = 0; i < 8; i++)
		{
			if(gllights[i] == -1)
			{
				foundlight = true;
				gllights[i] = dwLightIndex;
				break;
			}
		}
		if(!foundlight) TRACE_RET(HRESULT,23,D3DERR_LIGHT_SET_FAILED);
	}
	else
	{
		for(i = 0; i < 8; i++)
		{
			if(gllights[i] == dwLightIndex)
			{
				gllights[i] = -1;
			}
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::Load(LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
	LPRECT lprcSrcRect, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,14,lpDestTex,25,lpDestPoint,14,lpSrcTex,26,lprcSrcRect,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::Load: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::MultiplyTransform: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture)
{
	TRACE_ENTER(2,14,this,14,lpddsTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::PreLoad: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
	TRACE_ENTER(3,14,this,8,dwIndex,14,pPlaneEquation);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::SetClipPland: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::SetClipStatus: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	TRACE_ENTER(3,14,this,8,dwLightIndex,14,lpLight);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	bool foundlight = false;
	if(dwLightIndex >= lightsmax)
	{
		if(!ExpandLightBuffer(&lights,&lightsmax,dwLightIndex-1)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	}
	if(!lights[dwLightIndex]) lights[dwLightIndex] = new glDirect3DLight;
	lights[dwLightIndex]->SetLight7(lpLight);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	TRACE_ENTER(2,14,this,14,lpMaterial);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpMaterial) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(&material,lpMaterial,sizeof(D3DMATERIAL7));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRendStateType,9,dwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dwRendStateType)
	{
	case D3DRENDERSTATE_TEXTUREHANDLE:
		if(dwRenderState > texturecount-1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwRenderState)
		{
			if(!textures[dwRenderState]) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
			SetTexture(0,textures[dwRenderState]);
		}
		else SetTexture(0,NULL);
		break;
	case D3DRENDERSTATE_TEXTUREADDRESS:
		SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU,dwRenderState);
		SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV,dwRenderState);
		break;
	case D3DRENDERSTATE_WRAPU:
		if(dwRenderState) renderstate[D3DRENDERSTATE_WRAP0] |= D3DWRAP_U;
		else renderstate[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_U;
		break;
	case D3DRENDERSTATE_WRAPV:
		if(dwRenderState) renderstate[D3DRENDERSTATE_WRAP0] |= D3DWRAP_V;
		else renderstate[D3DRENDERSTATE_WRAP0] &= ~D3DWRAP_V;
		break;
	case D3DRENDERSTATE_TEXTUREMAG:
		switch(dwRenderState)
		{
		case D3DFILTER_NEAREST:
		default:
			SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTFG_POINT);
			break;
		case D3DFILTER_LINEAR:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFG_POINT);
		}
		break;
	case D3DRENDERSTATE_TEXTUREMIN:
		switch(dwRenderState)
		{
		case D3DFILTER_NEAREST:
		default:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_POINT);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_NONE);
			break;
		case D3DFILTER_LINEAR:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_NONE);
			break;
		case D3DFILTER_MIPNEAREST:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_POINT);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_POINT);
			break;
		case D3DFILTER_MIPLINEAR:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_POINT);
			break;
		case D3DFILTER_LINEARMIPNEAREST:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_POINT);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_LINEAR);
			break;
		case D3DFILTER_LINEARMIPLINEAR:
			SetTextureStageState(0,D3DTSS_MINFILTER,D3DTFN_LINEAR);
			SetTextureStageState(0,D3DTSS_MIPFILTER,D3DTFP_LINEAR);
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
			SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
			SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
			break;
		case D3DTBLEND_MODULATE:
			SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			break; // Automatically selected based on texture
		case D3DTBLEND_DECALALPHA:
			SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_BLENDTEXTUREALPHA);
			SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
			break;
		case D3DTBLEND_MODULATEALPHA:
			SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
			SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
			break;
		case D3DTBLEND_DECALMASK:
		case D3DTBLEND_MODULATEMASK:
			FIXME("DX5 masked blend modes not supported.");
			TRACE_RET(HRESULT,23,DDERR_UNSUPPORTED);
		case D3DTBLEND_ADD:
			SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
			SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_CURRENT);
			SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_ADD);
			SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG2);
			break;
		}
		break;
	}
	if(dwRendStateType > 152) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwRendStateType < 0) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	renderstate[dwRendStateType] = dwRenderState;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpNewRenderTarget,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpNewRenderTarget) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(dwFlags) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpNewRenderTarget->GetSurfaceDesc(&ddsd);
	if(!(ddsd.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)) TRACE_RET(HRESULT,23,DDERR_INVALIDSURFACETYPE);
	glDDS7->Release();
	glDDS7 = (glDirectDrawSurface7*)lpNewRenderTarget;
	glDDS7->AddRef();
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetStateData(DWORD dwState, LPVOID lpStateData)
{
	TRACE_ENTER(3,14,this,8,dwState,14,lpStateData);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::SetStateData: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
	TRACE_ENTER(3,14,this,8,dwStage,14,lpTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(texstages[dwStage].texture) texstages[dwStage].texture->Release();
	texstages[dwStage].texture = (glDirectDrawSurface7*)lpTexture;
	texstages[dwStage].dirty = true;
	if(lpTexture) lpTexture->AddRef();
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	TRACE_ENTER(4,14,this,8,dwStage,28,dwState,9,dwValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(dwStage > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		if(!dwValue || (dwValue > 24)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwStage == 0)renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] = 0;
		texstages[dwStage].colorop = (D3DTEXTUREOP)dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_COLORARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].colorarg1 = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_COLORARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].colorarg2 = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ALPHAOP:
		if(!dwValue || (dwValue > 24)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwStage == 0)renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] = 0;
		texstages[dwStage].alphaop = (D3DTEXTUREOP )dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ALPHAARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].alphaarg1 = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_RET(HRESULT,23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ALPHAARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwValue > 0x34) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].alphaarg2 = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVMAT00:
		memcpy(&texstages[dwStage].bumpenv00,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVMAT01:
		memcpy(&texstages[dwStage].bumpenv01,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVMAT10:
		memcpy(&texstages[dwStage].bumpenv10,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVMAT11:
		memcpy(&texstages[dwStage].bumpenv11,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_TEXCOORDINDEX:
		if((dwValue & 0xFFFF) > 7) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if((dwValue >> 16) > 3) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].texcoordindex = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ADDRESS:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ADDRESSU:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_ADDRESSV:
		if(!dwValue || (dwValue > 4)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BORDERCOLOR:
		texstages[dwStage].bordercolor = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MAGFILTER:
		if(!dwValue || (dwValue > 5)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].magfilter = (D3DTEXTUREMAGFILTER)dwValue;
		texstages[dwStage].dirty = true;
		switch(texstages[dwStage].magfilter)
		{
		case 1:
		default:
			texstages[dwStage].glmagfilter = GL_NEAREST;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			texstages[dwStage].glmagfilter = GL_LINEAR;
			break;
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MINFILTER:
		if(!dwValue || (dwValue > 3)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].minfilter = (D3DTEXTUREMINFILTER)dwValue;
		texstages[dwStage].dirty = true;
		switch(texstages[dwStage].minfilter)
		{
		case 1:
		default:
			switch(texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				texstages[dwStage].glminfilter = GL_NEAREST;
				break;
			case 2:
				texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
				break;
			case 3:
				texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
				break;
			}
			break;
		case 2:
		case 3:
			switch(texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				texstages[dwStage].glminfilter = GL_LINEAR;
				break;
			case 2:
				texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
				break;
			case 3:
				texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
				break;
			}
			break;
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MIPFILTER:
		if(!dwValue || (dwValue > 3)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].mipfilter = (D3DTEXTUREMIPFILTER)dwValue;
		texstages[dwStage].dirty = true;
		switch(texstages[dwStage].mipfilter)
		{
		case 1:
		default:
			switch(texstages[dwStage].minfilter)
			{
			case 1:
			default:
				texstages[dwStage].glminfilter = GL_NEAREST;
			case 2:
			case 3:
				texstages[dwStage].glminfilter = GL_LINEAR;
			}
			break;
		case 2:
			switch(texstages[dwStage].minfilter)
			{
			case 1:
			default:
				texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
			case 2:
			case 3:
				texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
			}
			break;
		case 3:
			switch(texstages[dwStage].minfilter)
			{
			case 1:
			default:
				texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
			case 2:
			case 3:
				texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
			}
			break;
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(&texstages[dwStage].lodbias,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MAXMIPLEVEL:
		texstages[dwStage].miplevel = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_MAXANISOTROPY:
		texstages[dwStage].anisotropy = dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(&texstages[dwStage].bumpenvlscale,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(&texstages[dwStage].bumpenvloffset,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		if((dwValue & 0xFF) > 4) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if((dwValue >> 8) > 1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		texstages[dwStage].textransform = (D3DTEXTURETRANSFORMFLAGS)dwValue;
		texstages[dwStage].dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	TRACE_RET(HRESULT,23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(&matWorld,lpD3DMatrix,sizeof(D3DMATRIX));
		modelview_dirty = true;
		transform_dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(&matView,lpD3DMatrix,sizeof(D3DMATRIX));
		modelview_dirty = true;
		transform_dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(&matProjection,lpD3DMatrix,sizeof(D3DMATRIX));
		projection_dirty = true;
		transform_dirty = true;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	default:
		TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT WINAPI glDirect3DDevice7::SetViewport(LPD3DVIEWPORT7 lpViewport)
{
	TRACE_ENTER(2,14,this,14,lpViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	memcpy(&viewport,lpViewport,sizeof(D3DVIEWPORT7));
	transform_dirty = true;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::ValidateDevice(LPDWORD lpdwPasses)
{
	TRACE_ENTER(2,14,this,14,lpdwPasses);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	for(int i = 0; i < 8; i++)
	{
		switch(texstages[i].colorop)
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
		switch(texstages[i].alphaop)
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

void glDirect3DDevice7::SetDepthComp()
{
	TRACE_ENTER(1,14,this);
	switch(renderstate[D3DRENDERSTATE_ZFUNC])
	{
	case D3DCMP_NEVER:
		::SetDepthComp(GL_NEVER);
		break;
	case D3DCMP_LESS:
		::SetDepthComp(GL_LESS);
		break;
	case D3DCMP_EQUAL:
		::SetDepthComp(GL_EQUAL);
		break;
	case D3DCMP_LESSEQUAL:
		::SetDepthComp(GL_LEQUAL);
		break;
	case D3DCMP_GREATER:
		::SetDepthComp(GL_GREATER);
		break;
	case D3DCMP_NOTEQUAL:
		::SetDepthComp(GL_NOTEQUAL);
		break;
	case D3DCMP_GREATEREQUAL:
		::SetDepthComp(GL_GEQUAL);
		break;
	case D3DCMP_ALWAYS:
	default:
		::SetDepthComp(GL_ALWAYS);
		break;
	}
	TRACE_EXIT(0,0);
}

D3DMATERIALHANDLE glDirect3DDevice7::AddMaterial(glDirect3DMaterial3 *material)
{
	TRACE_ENTER(2,14,this,14,material);
	materials[materialcount] = material;
	material->AddRef();
	materialcount++;
	if(materialcount >= maxmaterials)
	{
		glDirect3DMaterial3 **newmat;
		maxmaterials += 32;
		newmat = (glDirect3DMaterial3**)realloc(materials,maxmaterials*sizeof(glDirect3DMaterial3*));
		if(!newmat)
		{
			maxmaterials -= 32;
			materialcount--;
			materials[materialcount] = NULL;
			material->Release();
			TRACE_EXIT(9,0xFFFFFFFF);
			return 0xFFFFFFFF;
		}
		materials = newmat;
	}
	TRACE_EXIT(9,(materialcount-1));
	return materialcount-1;
}

D3DTEXTUREHANDLE glDirect3DDevice7::AddTexture(glDirectDrawSurface7 *texture)
{
	TRACE_ENTER(2,14,this,14,texture);
	textures[texturecount] = texture;
	texture->AddRef();
	texturecount++;
	if(texturecount >= maxtextures)
	{
		glDirectDrawSurface7 **newtex;
		maxtextures += 32;
		newtex = (glDirectDrawSurface7**)realloc(textures,maxtextures*sizeof(glDirectDrawSurface7*));
		if(!newtex)
		{
			maxtextures -= 32;
			texturecount--;
			textures[texturecount] = NULL;
			texture->Release();
			TRACE_EXIT(9,0xFFFFFFFF);
			return 0xFFFFFFFF;
		}
		textures = newtex;
	}
	TRACE_EXIT(9,(texturecount-1));
	return texturecount-1;
}

HRESULT glDirect3DDevice7::AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(int i = 0; i < maxviewports; i++)
	{
		if(viewports[i] == lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	}
	viewports[viewportcount] = (glDirect3DViewport3*)lpDirect3DViewport;
	viewports[viewportcount]->AddRef();
	viewportcount++;
	if(viewportcount >= maxviewports)
	{
		glDirect3DViewport3 **newviewport;
		maxviewports += 32;
		newviewport = (glDirect3DViewport3**)realloc(viewports,maxviewports*sizeof(glDirect3DViewport3*));
		if(!newviewport)
		{
			viewports--;
			viewports[viewportcount]->Release();
			viewports[viewportcount] = NULL;
			maxviewports -= 32;
			TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		}
	}
	viewports[viewportcount-1]->SetDevice(this);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	for(int i = 0; i < maxviewports; i++)
	{
		if(viewports[i] == lpDirect3DViewport)
		{
			viewports[i]->SetCurrent(false);
			viewports[i]->SetDevice(NULL);
			viewports[i]->Release();
			if(currentviewport == viewports[i]) currentviewport = NULL;
			viewports[i] = NULL;
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7::NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DViewport,14,lplpAnotherViewport,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpAnotherViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7::NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	TRACE_ENTER(2,14,this,14,lplpd3dViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lplpd3dViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!currentviewport) TRACE_RET(HRESULT,23,D3DERR_NOCURRENTVIEWPORT);
	*lplpd3dViewport = currentviewport;
	currentviewport->AddRef();
	TRACE_VAR("*lplpd3dViewport",14,*lplpd3dViewport);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	TRACE_ENTER(2,14,this,14,lpd3dViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(currentviewport == lpd3dViewport) TRACE_RET(HRESULT,23,D3D_OK);
	for(int i = 0; i < maxviewports; i++)
	{
		if(lpd3dViewport == viewports[i])
		{
			viewports[i]->SetCurrent(true);
			currentviewport = (glDirect3DViewport3*)lpd3dViewport;
			TRACE_EXIT(23,D3D_OK);
			return D3D_OK;
		}
	}
	TRACE_EXIT(23,DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7::Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,8,d3dpt,9,dwVertexTypeDesc,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::Begin: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpvVertices) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7::BeginIndexed: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::Index(WORD wVertexIndex)
{
	TRACE_ENTER(2,14,this,5,wVertexIndex);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::Index: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::Vertex(LPVOID lpVertex)
{
	TRACE_ENTER(2,14,this,14,lpVertex);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpVertex) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7::Vertex: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::End(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice7::End: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::ComputeSphereVisibility3(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	TRACE_ENTER(6,14,this,14,lpCenters,14,lpRadii,8,dwNumSpheres,9,dwFlags,14,lpdwReturnValues);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice3::ComputeSphereVisibility: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::GetCaps3(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,this,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DHWDevDesc && !lpD3DHELDevDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	D3DDEVICEDESC desc = d3ddesc3;
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

HRESULT glDirect3DDevice7::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,14,lpdwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpdwLightState)TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	switch(dwLightStateType)
	{
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(currentmaterial) *lpdwLightState = currentmaterial->handle;
		else *lpdwLightState = 0;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_AMBIENT,lpdwLightState));
	case D3DLIGHTSTATE_COLORMODEL:
		*lpdwLightState = D3DCOLOR_RGB;
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_FOGVERTEXMODE,lpdwLightState));
	case D3DLIGHTSTATE_FOGSTART:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_FOGSTART,lpdwLightState));
	case D3DLIGHTSTATE_FOGEND:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_FOGEND,lpdwLightState));
	case D3DLIGHTSTATE_FOGDENSITY:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_FOGDENSITY,lpdwLightState));
	case D3DLIGHTSTATE_COLORVERTEX:
		TRACE_RET(HRESULT,23,GetRenderState(D3DRENDERSTATE_COLORVERTEX,lpdwLightState));
	}
}
HRESULT glDirect3DDevice7::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,9,dwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	switch(dwLightStateType)
	{
	default:
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(!dwLightState) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
		if(dwLightState < materialcount)
		{
			if(materials[dwLightState] == currentmaterial) TRACE_RET(HRESULT,23,D3D_OK);
			if(materials[dwLightState])
			{
				if(currentmaterial)currentmaterial->SetCurrent(false);
				materials[dwLightState]->SetCurrent(true);
				currentmaterial = materials[dwLightState];
			}
		}
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_AMBIENT,dwLightState));
	case D3DLIGHTSTATE_COLORMODEL:
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE,dwLightState));
	case D3DLIGHTSTATE_FOGSTART:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_FOGSTART,dwLightState));
	case D3DLIGHTSTATE_FOGEND:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_FOGEND,dwLightState));
	case D3DLIGHTSTATE_FOGDENSITY:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_FOGDENSITY,dwLightState));
	case D3DLIGHTSTATE_COLORVERTEX:
		TRACE_RET(HRESULT,23,SetRenderState(D3DRENDERSTATE_COLORVERTEX,dwLightState));
	}
}

HRESULT glDirect3DDevice7::GetStats(LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,this,14,lpD3DStats);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DStats) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpD3DStats->dwSize < sizeof(D3DSTATS)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	memcpy(lpD3DStats,&stats,sizeof(D3DSTATS));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	TRACE_ENTER(3,14,this,14,lpD3DTex1,14,lpD3DTex2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTex1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpD3DTex2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice7::SwapTextureHandles: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	return DDERR_GENERIC;
}

void glDirect3DDevice7::InitDX5()
{
	TRACE_ENTER(1,14,this);
	SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,0);
	SetRenderState(D3DRENDERSTATE_TEXTUREADDRESS,D3DTADDRESS_WRAP);
	SetRenderState(D3DRENDERSTATE_WRAPU,FALSE);
	SetRenderState(D3DRENDERSTATE_WRAPV,FALSE);
	SetRenderState(D3DRENDERSTATE_TEXTUREMAG,D3DFILTER_NEAREST);
	SetRenderState(D3DRENDERSTATE_TEXTUREMIN,D3DFILTER_NEAREST);
	SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND,D3DTBLEND_MODULATE);
	SetRenderState(D3DRENDERSTATE_SPECULARENABLE,TRUE);
	TRACE_EXIT(0,0);
}

HRESULT glDirect3DDevice7::CreateMatrix(LPD3DMATRIXHANDLE lpD3DMatHandle)
{
	TRACE_ENTER(2,14,this,14,lpD3DMatHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	int foundslot = 0;
	if(!matrices)
	{
		matrices = (D3D1MATRIX*)malloc(16*sizeof(D3D1MATRIX));
		if(!matrices) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		ZeroMemory(matrices,16*sizeof(D3D1MATRIX));
		matrixcount = 16;
	}
	for(int i = 0; i < matrixcount; i++)
	{
		if(i == 0) continue;
		if(!matrices[i].active)
		{
			foundslot = i;
			break;
		}
	}
	if(!foundslot)
	{
		int newcount;
		D3D1MATRIX *newmatrices;
		newcount = matrixcount + 16;
		newmatrices = (D3D1MATRIX*)realloc(matrices,newcount*sizeof(D3D1MATRIX));
		if(!newmatrices) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		ZeroMemory(&newmatrices[matrixcount],16*sizeof(D3D1MATRIX));
		matrices = newmatrices;
		foundslot = matrixcount;
		matrixcount = newcount;
	}
	*lpD3DMatHandle = foundslot;
	__gluMakeIdentityf((GLfloat*)&matrices[foundslot].matrix);
	matrices[foundslot].active = TRUE;
	TRACE_VAR("*lpD3DMatHandle",9,*lpD3DMatHandle);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::DeleteMatrix(D3DMATRIXHANDLE d3dMatHandle)
{
	TRACE_ENTER(2,14,this,9,d3dMatHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!d3dMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(d3dMatHandle >= matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!matrices[d3dMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_DESTROY_FAILED);
	matrices[d3dMatHandle].active = FALSE;
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::GetMatrix(D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,9,lpD3DMatHandle,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DMatHandle)
	{
		__gluMakeIdentityf((GLfloat*)lpD3DMatrix);
		TRACE_EXIT(23,D3D_OK);
		return D3D_OK;
	}
	if(lpD3DMatHandle >= matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!matrices[lpD3DMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_GETDATA_FAILED);
	memcpy(lpD3DMatrix,&matrices[lpD3DMatHandle].matrix,sizeof(D3DMATRIX));
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::SetMatrix(D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,9,d3dMatHandle,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!d3dMatHandle) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(d3dMatHandle >= matrixcount) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!matrices[d3dMatHandle].active) TRACE_RET(HRESULT,23,D3DERR_MATRIX_SETDATA_FAILED);
	memcpy(&matrices[d3dMatHandle].matrix,lpD3DMatrix,sizeof(D3DMATRIX));
	if(d3dMatHandle == mhWorld) SetTransform(D3DTRANSFORMSTATE_WORLD,lpD3DMatrix);
	if(d3dMatHandle == mhView) SetTransform(D3DTRANSFORMSTATE_VIEW,lpD3DMatrix);
	if(d3dMatHandle == mhProjection) SetTransform(D3DTRANSFORMSTATE_PROJECTION,lpD3DMatrix);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
	IUnknown* pUnkOuter)
{
	TRACE_ENTER(4,14,this,14,lpDesc,14,lplpDirect3DExecuteBuffer,14,pUnkOuter);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDesc) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(pUnkOuter) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(lpDesc->dwSize != sizeof(D3DEXECUTEBUFFERDESC)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!(lpDesc->dwFlags & D3DDEB_BUFSIZE)) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	*lplpDirect3DExecuteBuffer = new glDirect3DExecuteBuffer(lpDesc);
	if(!*lplpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
	TRACE_VAR("*lplpDirect3DExecuteBuffer",14,*lplpDirect3DExecuteBuffer);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
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

void glDirect3DDevice7::UpdateTransform()
{
	TRACE_ENTER(1,14,this);
	GLfloat mat1[16];
	__gluMultMatricesf(matWorld,matView,mat1);
	__gluMultMatricesf(mat1,matProjection,matTransform);
	transform_dirty = false;
	TRACE_EXIT(0,0);
}

void CalculateExtents(D3DRECT *extents, D3DTLVERTEX *vertices, DWORD count)
{
	if(!count) return;
	D3DVALUE minX,minY,maxX,maxY;
	minX = maxX = vertices[0].dvSX;
	minY = maxY = vertices[0].dvSY;
	for(int i = 0; i < count; i++)
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

inline void glDirect3DDevice7::TransformViewport(D3DTLVERTEX *vertex)
{
	vertex->dvSX = vertex->dvSX / vertex->dvRHW * scalex + viewport.dwX + viewport.dwWidth / 2;
	vertex->dvSY = vertex->dvSY / vertex->dvRHW * scaley + viewport.dwY + viewport.dwHeight / 2;
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

INT glDirect3DDevice7::TransformAndLight(D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,this,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
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
	in[3] = 1.0f;
	if(transform_dirty) UpdateTransform();
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(int i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(matTransform,in,&(*output)[i+dest].dvSX);
		TransformViewport(&(*output)[i+dest]);
		(*output)[i+dest].dvRHW = 1.0f/(*output)[i+dest].dvRHW;
		(*output)[i+dest].dvTU = input[i+start].dvTU;
		(*output)[i+dest].dvTV = input[i+start].dvTV;
		diffuse.r = diffuse.g = diffuse.b = diffuse.a = 0;
		specular.r = specular.g = specular.b = specular.a = 0;
		ambient.r = (D3DVALUE)RGBA_GETRED(renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0;
		ambient.g = (D3DVALUE)RGBA_GETGREEN(renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0;
		ambient.b = (D3DVALUE)RGBA_GETBLUE(renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0;
		ambient.a = (D3DVALUE)RGBA_GETALPHA(renderstate[D3DRENDERSTATE_AMBIENT]) / 255.0;
		for(int l = 0; l < 8; l++)
		{
			if(gllights[l] != -1)
			{
				AddD3DCV(&ambient,&lights[gllights[l]]->light.dcvAmbient);
				switch(lights[gllights[l]]->light.dltType)
				{
				case D3DLIGHT_DIRECTIONAL:
					NdotHV = 0;
					memcpy(dir,&lights[gllights[l]]->light.dvDirection,3*sizeof(D3DVALUE));
					normalize(dir);
					NdotL = max(dot3((float*)&input[i+start].dvNX,(float*)&dir),0.0f);
					color1 = lights[gllights[l]]->light.dcvDiffuse;
					MulD3DCVFloat(&color1,NdotL);
					AddD3DCV(&diffuse,&color1);
					if((NdotL > 0.0) && (material.dvPower != 0.0))
					{
						__gluMultMatrixVecf(matWorld,&input[i+start].dvX,P);
						memcpy(L,&lights[gllights[l]]->light.dvDirection,3*sizeof(D3DVALUE));
						NegativeVec3(L);
						SubVec3(L,P);
						normalize(L);
						memcpy(V,eye,3*sizeof(D3DVALUE));
						SubVec3(V,P);
						normalize(V);
						AddVec3(L,V);
						NdotHV = max(dot3((float*)&input[i+start].dvNX,L),0.0f);
						color1 = lights[gllights[l]]->light.dcvSpecular;
						MulD3DCVFloat(&color1,pow(NdotHV,material.dvPower));
						AddD3DCV(&specular,&color1);
					}
				break;
				case D3DLIGHT_POINT:
					__gluMultMatrixVecf(matWorld,&input[i+start].dvX,P);
					memcpy(V,&lights[gllights[l]]->light.dvPosition,3*sizeof(D3DVALUE));
					SubVec3(V,P);
					length = len3(V);
					if((length > lights[gllights[l]]->light.dvRange) && (lights[gllights[l]]->light.dvRange != 0.0)) continue;
					normalize(V);
					attenuation = 1.0/(lights[gllights[l]]->light.dvAttenuation0+(length*lights[gllights[l]]->light.dvAttenuation1)
						+((length*length)*lights[gllights[l]]->light.dvAttenuation2));
					NdotV = max(0.0,dot3((float*)&input[i+start].dvNX,V));
					AddVec3(V,eye);
					normalize(V);
					NdotHV = max(0.0,dot3((float*)&input[i+start].dvNX,V));
					if(NdotV == 0.0) pf = 0.0;
					else if(material.dvPower != 0.0) pf = pow(NdotHV,material.dvPower);
					else pf = 0.0;
					color1 = lights[gllights[l]]->light.dcvDiffuse;
					MulD3DCVFloat(&color1,NdotV*attenuation);
					AddD3DCV(&diffuse,&color1);
					color1 = lights[gllights[l]]->light.dcvSpecular;
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
			color1 = material.dcvDiffuse;
			MulD3DCV(&color1,&diffuse);
			color2 = material.dcvAmbient;
			MulD3DCV(&color2,&ambient);
			AddD3DCV(&color1,&color2);
			AddD3DCV(&color1,&material.dcvEmissive);
			color2 = material.dcvSpecular;
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
INT glDirect3DDevice7::TransformOnly(D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,this,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	GLfloat in[4];
	in[3] = 1.0f;
	if(transform_dirty) UpdateTransform();
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(int i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(matTransform,in,&(*output)[i+dest].dvSX);
		TransformViewport(&(*output)[i+dest]);
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
INT glDirect3DDevice7::TransformOnly(D3DTLVERTEX **output, DWORD *outsize, D3DLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,this,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	GLfloat in[4];
	in[3] = 1.0f;
	if(transform_dirty) UpdateTransform();
	if(*outsize < (dest+count)*sizeof(D3DTLVERTEX))
	{
		D3DTLVERTEX *tmpptr = (D3DTLVERTEX*)realloc(*output,(dest+count)*sizeof(D3DTLVERTEX));
		if(!tmpptr) TRACE_RET(INT,11,-1);
		*output = tmpptr;
		*outsize = (dest+count)*sizeof(D3DTLVERTEX);
	}
	for(int i = 0; i < count; i++)
	{
		in[0] = input[i+start].dvX;
		in[1] = input[i+start].dvY;
		in[2] = input[i+start].dvZ;
		__gluMultMatrixVecf(matTransform,in,&(*output)[i+dest].dvSX);
		TransformViewport(&(*output)[i+dest]);
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
INT glDirect3DDevice7::CopyVertices(D3DTLVERTEX **output, DWORD *outsize, D3DTLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents)
{
	TRACE_ENTER(8,14,this,14,output,14,outsize,14,input,5,start,5,dest,8,count,14,extents);
	if(transform_dirty) UpdateTransform();
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

HRESULT glDirect3DDevice7::Execute(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DVIEWPORT3 vp;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&vp);
	if(FAILED(SetCurrentViewport(vp)))
	{
		vp->Release();
		TRACE_EXIT(23,DDERR_INVALIDPARAMS);
		return DDERR_INVALIDPARAMS;
	}
	vp->Release();
	D3DEXECUTEBUFFERDESC desc;
	D3DEXECUTEDATA data;
	HRESULT err = ((glDirect3DExecuteBuffer*)lpDirect3DExecuteBuffer)->ExecuteLock(&desc,&data);
	if(FAILED(err)) TRACE_RET(HRESULT,23,err);
	unsigned char *opptr = (unsigned char *)desc.lpData + data.dwInstructionOffset;
	unsigned char *in_vertptr = (unsigned char *)desc.lpData + data.dwVertexOffset;
	DWORD offset;
	INT result;
	D3DMATRIX mat1,mat2,mat3;
	bool ebExit = false;
	int i;
	if(outbuffersize < desc.dwBufferSize)
	{
		unsigned char *tmpbuffer = (unsigned char *)realloc(outbuffer,desc.dwBufferSize);
		if(!tmpbuffer) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
		outbuffer = tmpbuffer;
		outbuffersize = desc.dwBufferSize;
	}	
	D3DVERTEX *vert_ptr = (D3DVERTEX*)outbuffer;
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
				if(ebBufferSize < (offset + ((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX)))
				{
					if(!ExpandBuffer((void**)&ebBuffer,&ebBufferSize,(((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX) > 1024) ?
						((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX) : 1024))
						TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				}
				memcpy(&ebBuffer+offset,&vert_ptr[((D3DPOINT*)opptr)->wFirst],((D3DPOINT*)opptr)->wCount*sizeof(D3DVERTEX));
				offset+=((D3DPOINT*)opptr)->wCount;
				opptr += instruction->bSize;
			}
			DrawPrimitive(D3DPT_POINTLIST,D3DFVF_TLVERTEX,ebBuffer,offset/sizeof(D3DVERTEX),0);
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
				if(ebBufferSize < (offset + sizeof(D3DLINE)))
				{
					if(!ExpandBuffer((void**)&ebBuffer,&ebBufferSize,1024)) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				}
				memcpy(&ebBuffer+offset,opptr,sizeof(D3DLINE));
				offset += sizeof(D3DLINE);
				opptr += instruction->bSize;
			}
			DrawIndexedPrimitive(D3DPT_LINELIST,D3DFVF_TLVERTEX,vert_ptr,(desc.dwBufferSize-data.dwVertexOffset)/sizeof(D3DVERTEX),
				(WORD*)ebBuffer,instruction->wCount*2,0);
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
				result = AddTriangle(&ebBuffer,&ebBufferSize,&offset,(D3DTRIANGLE*)opptr);
				if(result == -1) TRACE_RET(HRESULT,23,DDERR_OUTOFMEMORY);
				opptr += instruction->bSize;
			}
			if(instruction->wCount) DrawIndexedPrimitive(D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,vert_ptr,
				(desc.dwBufferSize-data.dwVertexOffset)/sizeof(D3DVERTEX), (WORD*)ebBuffer,instruction->wCount*3,0);
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
				GetMatrix(((D3DMATRIXLOAD*)opptr)->hSrcMatrix,&mat1);
				SetMatrix(((D3DMATRIXLOAD*)opptr)->hDestMatrix,&mat1);
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
				GetMatrix(((D3DMATRIXMULTIPLY*)opptr)->hSrcMatrix1,&mat1);
				GetMatrix(((D3DMATRIXMULTIPLY*)opptr)->hSrcMatrix2,&mat2);
				__gluMultMatricesf((GLfloat*)&mat1,(GLfloat*)&mat2,(GLfloat*)&mat3);
				SetMatrix(((D3DMATRIXMULTIPLY*)opptr)->hDestMatrix,&mat3);
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
				GetMatrix(((D3DSTATE*)opptr)->dwArg[0],&mat1);
				SetTransform(((D3DSTATE*)opptr)->dtstTransformStateType,&mat1);
				switch(((D3DSTATE*)opptr)->dtstTransformStateType)
				{
				case D3DTRANSFORMSTATE_WORLD:
					mhWorld = ((D3DSTATE*)opptr)->dwArg[0];
					break;
				case D3DTRANSFORMSTATE_VIEW:
					mhView = ((D3DSTATE*)opptr)->dwArg[0];
					break;
				case D3DTRANSFORMSTATE_PROJECTION:
					mhProjection = ((D3DSTATE*)opptr)->dwArg[0];
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
				SetLightState(((D3DSTATE*)opptr)->dlstLightStateType,((D3DSTATE*)opptr)->dwArg[0]);
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
				SetRenderState(((D3DSTATE*)opptr)->drstRenderStateType,((D3DSTATE*)opptr)->dwArg[0]);
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
					if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
						TransformOnly((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					else TransformOnly((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					break;
				case D3DPROCESSVERTICES_TRANSFORMLIGHT:
					if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
					{
						if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
							TransformOnly((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
						else TransformOnly((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					}
					else
					{
						if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
							TransformAndLight((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
						else TransformAndLight((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
							((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					}
					break;
				case D3DPROCESSVERTICES_COPY:
					if(((D3DPROCESSVERTICES*)opptr)->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
						CopyVertices((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DTLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					else CopyVertices((D3DTLVERTEX**)&outbuffer,&outbuffersize,(D3DTLVERTEX*)in_vertptr,((D3DPROCESSVERTICES*)opptr)->wStart,
						((D3DPROCESSVERTICES*)opptr)->wDest,((D3DPROCESSVERTICES*)opptr)->dwCount,&data.dsStatus.drExtent);
					break;
				default:
					break;
				}
				stats.dwVerticesProcessed += ((D3DPROCESSVERTICES*)opptr)->dwCount;
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
	((glDirect3DExecuteBuffer*)lpDirect3DExecuteBuffer)->ExecuteUnlock(&data);
	TRACE_EXIT(23,D3D_OK);
	return D3D_OK;
}

HRESULT glDirect3DDevice7::GetPickRecords(LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec)
{
	TRACE_ENTER(3,14,this,14,lpCount,14,lpD3DPickRec);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpCount) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	FIXME("glDirect3DDevice1::GetPickRecords: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT glDirect3DDevice7::Pick(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, 
	LPD3DRECT lpRect)
{
	TRACE_ENTER(5,14,this,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(lpDirect3DExecuteBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpRect) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice1::Pick: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

// IDirect3DDevice3 wrapper
glDirect3DDevice3::glDirect3DDevice3(glDirect3DDevice7 *glD3DDev7)
{
	TRACE_ENTER(2,14,this,14,glD3DDev7);
	this->glD3DDev7 = glD3DDev7;
	refcount = 1;
	TRACE_EXIT(-1,0);
}

glDirect3DDevice3::~glDirect3DDevice3()
{
	TRACE_ENTER(1,14,this);
	glD3DDev7->glD3DDev3 = NULL;
	glD3DDev7->Release();
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DDevice3::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glD3DDev7->QueryInterface(riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice3::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DDevice3::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice3::AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->AddViewport(lpDirect3DViewport));
}

HRESULT WINAPI glDirect3DDevice3::Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,8,d3dpt,9,dwVertexTypeDesc,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Begin(d3dpt,dwVertexTypeDesc,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->BeginIndexed(dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwNumVertices,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::BeginScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->BeginScene());
}

HRESULT WINAPI glDirect3DDevice3::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	TRACE_ENTER(6,14,this,14,lpCenters,14,lpRadii,8,dwNumSpheres,9,dwFlags,14,lpdwReturnValues);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->ComputeSphereVisibility3(lpCenters,lpRadii,dwNumSpheres,dwFlags,lpdwReturnValues));
}

HRESULT WINAPI glDirect3DDevice3::DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DeleteViewport(lpDirect3DViewport));
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,8,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpvVertices,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawIndexedPrimitive(d3dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,8,d3dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawIndexedPrimitiveStrided(d3dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,d3dptPrimitiveType,14,lpd3dVertexBuffer,14,lpwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dVertexBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawIndexedPrimitiveVB(d3dptPrimitiveType,
		((glDirect3DVertexBuffer1*)lpd3dVertexBuffer)->GetGLD3DVB7(),0,-1,lpwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertices,8,dwVertexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawPrimitive(dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,9,dwVertexTypeDesc,14,lpVertexArray,8,dwVertexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawPrimitiveStrided(dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,d3dptPrimitiveType,14,lpd3dVertexBuffer,8,dwStartVertex,8,dwNumVertices,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dVertexBuffer) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawPrimitiveVB(d3dptPrimitiveType,((glDirect3DVertexBuffer1*)lpd3dVertexBuffer)->GetGLD3DVB7(),
		dwStartVertex,dwNumVertices,dwFlags));
}
HRESULT WINAPI glDirect3DDevice3::End(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->End(dwFlags));
}
	
HRESULT WINAPI glDirect3DDevice3::EndScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->EndScene());
}

HRESULT WINAPI glDirect3DDevice3::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,this,14,lpd3dEnumPixelProc,14,lpArg);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->EnumTextureFormats(lpd3dEnumPixelProc,lpArg));
}

HRESULT WINAPI glDirect3DDevice3::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,this,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetCaps3(lpD3DHWDevDesc,lpD3DHELDevDesc));
}

HRESULT WINAPI glDirect3DDevice3::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetClipStatus(lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice3::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	TRACE_ENTER(2,14,this,14,lplpd3dViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetCurrentViewport(lplpd3dViewport));
}

HRESULT WINAPI glDirect3DDevice3::GetDirect3D(LPDIRECT3D3 *lplpD3D)
{
	TRACE_ENTER(2,14,this,14,lplpD3D);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glD3DDev7->GetDirect3D(&d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D3,(void**)lplpD3D);
	d3d7->Release();
	TRACE_VAR("*lplpD3D",14,*lplpD3D);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,14,lpdwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetLightState(dwLightStateType,lpdwLightState));
}

HRESULT WINAPI glDirect3DDevice3::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRenderStateType,14,lpdwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetRenderState(dwRenderStateType,lpdwRenderState));
}

HRESULT WINAPI glDirect3DDevice3::GetRenderTarget(LPDIRECTDRAWSURFACE4 *lplpRenderTarget)
{
	TRACE_ENTER(2,14,this,14,lplpRenderTarget);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetRenderTarget(&dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirectDrawSurface4,(void**)lplpRenderTarget);
	dds7->Release();
	TRACE_VAR("*lplpRenderTarget",14,*lplpRenderTarget);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetStats(LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,this,14,lpD3DStats);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetStats(lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice3::GetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 *lplpTexture)
{
	TRACE_ENTER(3,14,this,8,dwStage,14,lplpTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetTexture(dwStage,&dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirect3DTexture2,(void**)lplpTexture);
	dds7->Release();
	TRACE_VAR("*lplpTexture",14,*lplpTexture);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	TRACE_ENTER(4,14,this,8,dwStage,28,dwState,14,lpdwValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetTextureStageState(dwStage,dwState,lpdwValue));
}

HRESULT WINAPI glDirect3DDevice3::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetTransform(dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice3::Index(WORD wVertexIndex)
{
	TRACE_ENTER(2,14,this,5,wVertexIndex);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Index(wVertexIndex));
}

HRESULT WINAPI glDirect3DDevice3::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->MultiplyTransform(dtstTransformStateType,lpD3DMatrix));
}
HRESULT WINAPI glDirect3DDevice3::NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DViewport,14,lplpAnotherViewport,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->NextViewport(lpDirect3DViewport,lplpAnotherViewport,dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetClipStatus(lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice3::SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	TRACE_ENTER(2,14,this,14,lpd3dViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetCurrentViewport(lpd3dViewport));
}

HRESULT WINAPI glDirect3DDevice3::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,8,dwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetLightState(dwLightStateType,dwLightState));
}

HRESULT WINAPI glDirect3DDevice3::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRendStateType,9,dwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetRenderState(dwRendStateType,dwRenderState));
}
	
HRESULT WINAPI glDirect3DDevice3::SetRenderTarget(LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpNewRenderTarget,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetRenderTarget(((glDirectDrawSurface4*)lpNewRenderTarget)->GetDDS7(),dwFlags));
}

HRESULT WINAPI glDirect3DDevice3::SetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture)
{
	TRACE_ENTER(3,14,this,8,dwStage,14,lpTexture);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirectDrawSurface7 *dds7;
	if(lpTexture) dds7 = ((glDirect3DTexture2*)lpTexture)->GetDDS7();
	else dds7 = NULL;
	TRACE_RET(HRESULT,23,glD3DDev7->SetTexture(dwStage,dds7));
}

HRESULT WINAPI glDirect3DDevice3::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	TRACE_ENTER(4,14,this,8,dwStage,28,dwState,9,dwValue);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetTextureStageState(dwStage,dwState,dwValue));
}

HRESULT WINAPI glDirect3DDevice3::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,27,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetTransform(dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice3::ValidateDevice(LPDWORD lpdwPasses)
{
	TRACE_ENTER(2,14,this,14,lpdwPasses);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->ValidateDevice(lpdwPasses));
}

HRESULT WINAPI glDirect3DDevice3::Vertex(LPVOID lpVertex)
{
	TRACE_ENTER(2,14,this,14,lpVertex);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Vertex(lpVertex));
}

// IDirect3DDevice2 wrapper
glDirect3DDevice2::glDirect3DDevice2(glDirect3DDevice7 *glD3DDev7)
{
	TRACE_ENTER(2,14,this,14,glD3DDev7);
	this->glD3DDev7 = glD3DDev7;
	glD3DDev7->InitDX5();
	refcount = 1;
	TRACE_EXIT(-1,0);
}

glDirect3DDevice2::~glDirect3DDevice2()
{
	TRACE_ENTER(1,14,this);
	glD3DDev7->glD3DDev2 = NULL;
	glD3DDev7->Release();
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DDevice2::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glD3DDev7->QueryInterface(riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice2::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DDevice2::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::AddViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->AddViewport(glD3DV3);
	glD3DV3->Release();
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

HRESULT WINAPI glDirect3DDevice2::Begin(D3DPRIMITIVETYPE d3dpt, D3DVERTEXTYPE d3dvt, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,8,d3dpt,8,d3dvt,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(d3dvt);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->Begin(d3dpt,vertextype,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,8,dvtVertexType,14,lpvVertices,8,dwNumVertices,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->BeginIndexed(dptPrimitiveType,vertextype,lpvVertices,dwNumVertices,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::BeginScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->BeginScene());
}

HRESULT WINAPI glDirect3DDevice2::DeleteViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->DeleteViewport(glD3DV3);
	glD3DV3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	TRACE_ENTER(8,14,this,8,d3dptPrimitiveType,8,d3dvtVertexType,14,lpvVertices,8,dwVertexCount,14,dwIndices,8,dwIndexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(d3dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawIndexedPrimitive(d3dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwIndices,dwIndexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	TRACE_ENTER(6,14,this,8,dptPrimitiveType,8,dvtVertexType,14,lpvVertices,8,dwVertexCount,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	TRACE_RET(HRESULT,23,glD3DDev7->DrawPrimitive(dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::End(DWORD dwFlags)
{
	TRACE_ENTER(2,14,this,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->End(dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::EndScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->EndScene());
}

HRESULT WINAPI EnumTex2(LPDDPIXELFORMAT ddpf, LPVOID lpUserArg)
{
	if(ddpf->dwFlags & DDPF_LUMINANCE) return D3DENUMRET_OK;
	if(ddpf->dwFlags & DDPF_ALPHA) return D3DENUMRET_OK;
	int *args = (int*)lpUserArg;
	LPD3DENUMTEXTUREFORMATSCALLBACK callback = (LPD3DENUMTEXTUREFORMATSCALLBACK)args[0];
	DDSURFACEDESC ddsd;
	ZeroMemory(&ddsd,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS|DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddpfPixelFormat = *ddpf;
	HRESULT ret = callback(&ddsd,(LPVOID)args[1]);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,this,14,lpd3dEnumTextureProc,14,lpArg);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPVOID context[2];
	context[0] = (LPVOID)lpd3dEnumTextureProc;
	context[1] = lpArg;
	TRACE_RET(HRESULT,23,glD3DDev7->EnumTextureFormats(EnumTex2,&context));
}

HRESULT WINAPI glDirect3DDevice2::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,this,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetCaps3(lpD3DHWDevDesc,lpD3DHELDevDesc));
}

HRESULT WINAPI glDirect3DDevice2::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetClipStatus(lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice2::GetCurrentViewport(LPDIRECT3DVIEWPORT2 *lplpd3dViewport2)
{
	TRACE_ENTER(2,14,this,14,lplpd3dViewport2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirect3DViewport3 *glD3DV3;
	HRESULT ret = glD3DDev7->GetCurrentViewport((LPDIRECT3DVIEWPORT3*)&glD3DV3);
	if(!glD3DV3) TRACE_RET(HRESULT,23,ret);
	glD3DV3->QueryInterface(IID_IDirect3DViewport2,(void**)lplpd3dViewport2);
	glD3DV3->Release();
	TRACE_VAR("*lplpd3dViewport2",14,*lplpd3dViewport2);
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::GetDirect3D(LPDIRECT3D2 *lplpD3D2)
{
	TRACE_ENTER(2,14,this,14,lplpD3D2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glD3DDev7->GetDirect3D(&d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D2,(void**)lplpD3D2);
	d3d7->Release();
	TRACE_VAR("*lplpD3D2",14,*lplpD3D2);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice2::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,14,lpdwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetLightState(dwLightStateType,lpdwLightState));
}

HRESULT WINAPI glDirect3DDevice2::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRenderStateType,14,lpdwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetRenderState(dwRenderStateType,lpdwRenderState));
}

HRESULT WINAPI glDirect3DDevice2::GetRenderTarget(LPDIRECTDRAWSURFACE *lplpRenderTarget)
{
	TRACE_ENTER(2,14,this,14,lplpRenderTarget);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetRenderTarget(&dds7);
	if(!dds7) TRACE_RET(HRESULT,23,err);
	dds7->QueryInterface(IID_IDirectDrawSurface,(void**)lplpRenderTarget);
	dds7->Release();
	TRACE_VAR("*lplpRenderTarget",14,*lplpRenderTarget);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice2::GetStats(LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,this,14,lpD3DStats);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetStats(lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice2::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetTransform(dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2::Index(WORD wVertexIndex)
{
	TRACE_ENTER(2,14,this,5,wVertexIndex);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Index(wVertexIndex));
}

HRESULT WINAPI glDirect3DDevice2::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->MultiplyTransform(dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2::NextViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DViewport2,14,lplpDirect3DViewport2,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DViewport2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice2::NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}

HRESULT WINAPI glDirect3DDevice2::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	TRACE_ENTER(2,14,this,14,lpD3DClipStatus);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetClipStatus(lpD3DClipStatus));
}

HRESULT WINAPI glDirect3DDevice2::SetCurrentViewport(LPDIRECT3DVIEWPORT2 lpd3dViewport2)
{
	TRACE_ENTER(2,14,this,14,lpd3dViewport2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpd3dViewport2) TRACE_RET(HRESULT,23,glD3DDev7->SetCurrentViewport(NULL));
	glDirect3DViewport3 *glD3DV3;
	lpd3dViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->SetCurrentViewport(glD3DV3);
	glD3DV3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	TRACE_ENTER(3,14,this,30,dwLightStateType,9,dwLightState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetLightState(dwLightStateType,dwLightState));
}

HRESULT WINAPI glDirect3DDevice2::SetRenderState(D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState)
{
	TRACE_ENTER(3,14,this,27,dwRenderStateType,9,dwRenderState);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetRenderState(dwRenderStateType,dwRenderState));
}

HRESULT WINAPI glDirect3DDevice2::SetRenderTarget(LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags)
{
	TRACE_ENTER(3,14,this,14,lpNewRenderTarget,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetRenderTarget(((glDirectDrawSurface1*)lpNewRenderTarget)->GetDDS7(),dwFlags));
}

HRESULT WINAPI glDirect3DDevice2::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,29,dtstTransformStateType,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetTransform(dtstTransformStateType,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice2::SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	TRACE_ENTER(3,14,this,14,lpD3DTex1,14,lpD3DTex2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SwapTextureHandles(lpD3DTex1,lpD3DTex2));
}

HRESULT WINAPI glDirect3DDevice2::Vertex(LPVOID lpVertexType)
{
	TRACE_ENTER(2,14,this,14,lpVertexType);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Vertex(lpVertexType));
}

// IDirect3DDevice wrapper
glDirect3DDevice1::glDirect3DDevice1(glDirect3DDevice7 *glD3DDev7)
{
	TRACE_ENTER(2,14,this,14,glD3DDev7);
	this->glD3DDev7 = glD3DDev7;
	glD3DDev7->InitDX5();
	refcount = 1;
	TRACE_EXIT(-1,0);
}

glDirect3DDevice1::~glDirect3DDevice1()
{
	TRACE_ENTER(1,14,this);
	glD3DDev7->glD3DDev1 = NULL;
	glD3DDev7->Release();
	TRACE_EXIT(-1,0);
}

HRESULT WINAPI glDirect3DDevice1::QueryInterface(REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3,14,this,24,&riid,14,ppvObj);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		TRACE_VAR("*ppvObj",14,*ppvObj);
		TRACE_EXIT(23,DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT,23,glD3DDev7->QueryInterface(riid,ppvObj));
}

ULONG WINAPI glDirect3DDevice1::AddRef()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	refcount++;
	TRACE_EXIT(8,refcount);
	return refcount;
}

ULONG WINAPI glDirect3DDevice1::Release()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(ULONG,8,0);
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	TRACE_EXIT(8,ret);
	return ret;
}

HRESULT WINAPI glDirect3DDevice1::AddViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->AddViewport(glD3DV3);
	glD3DV3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DDevice1::BeginScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->BeginScene());
}
HRESULT WINAPI glDirect3DDevice1::CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc,
		LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,	IUnknown* pUnkOuter)
{
	TRACE_ENTER(4,14,this,14,lpDesc,14,lplpDirect3DExecuteBuffer,14,pUnkOuter);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->CreateExecuteBuffer(lpDesc,lplpDirect3DExecuteBuffer,pUnkOuter));
}
HRESULT WINAPI glDirect3DDevice1::CreateMatrix(LPD3DMATRIXHANDLE lpD3DMatHandle)
{
	TRACE_ENTER(2,14,this,14,lpD3DMatHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->CreateMatrix(lpD3DMatHandle));
}
HRESULT WINAPI glDirect3DDevice1::DeleteMatrix(D3DMATRIXHANDLE d3dMatHandle)
{
	TRACE_ENTER(2,14,this,9,d3dMatHandle);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->DeleteMatrix(d3dMatHandle));
}
HRESULT WINAPI glDirect3DDevice1::DeleteViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport)
{
	TRACE_ENTER(2,14,this,14,lpDirect3DViewport);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->DeleteViewport(glD3DV3);
	glD3DV3->Release();
	TRACE_EXIT(23,ret);
	return ret;
}
HRESULT WINAPI glDirect3DDevice1::EndScene()
{
	TRACE_ENTER(1,14,this);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->EndScene());
}
HRESULT WINAPI glDirect3DDevice1::EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg)
{
	TRACE_ENTER(3,14,this,14,lpd3dEnumTextureProc,14,lpArg);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPVOID context[2];
	context[0] = (LPVOID)lpd3dEnumTextureProc;
	context[1] = lpArg;
	TRACE_RET(HRESULT,23,glD3DDev7->EnumTextureFormats(EnumTex2,&context));
}
HRESULT WINAPI glDirect3DDevice1::Execute(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Execute(lpDirect3DExecuteBuffer,lpDirect3DViewport,dwFlags));
}
HRESULT WINAPI glDirect3DDevice1::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	TRACE_ENTER(3,14,this,14,lpD3DHWDevDesc,14,lpD3DHELDevDesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetCaps3(lpD3DHWDevDesc,lpD3DHELDevDesc));
}
HRESULT WINAPI glDirect3DDevice1::GetDirect3D(LPDIRECT3D* lpD3D)
{
	TRACE_ENTER(2,14,this,14,lpD3D);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	LPDIRECT3D7 d3d7;
	HRESULT err = glD3DDev7->GetDirect3D(&d3d7);
	if(!d3d7) TRACE_RET(HRESULT,23,err);
	d3d7->QueryInterface(IID_IDirect3D,(void**)lpD3D);
	d3d7->Release();
	TRACE_VAR("*lpD3D",14,*lpD3D);
	TRACE_EXIT(23,err);
	return err;
}

HRESULT WINAPI glDirect3DDevice1::GetMatrix(D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,9,lpD3DMatHandle,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetMatrix(lpD3DMatHandle,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice1::GetPickRecords(LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec)
{
	TRACE_ENTER(3,14,this,14,lpCount,14,lpD3DPickRec);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetPickRecords(lpCount,lpD3DPickRec));
}

HRESULT WINAPI glDirect3DDevice1::GetStats(LPD3DSTATS lpD3DStats)
{
	TRACE_ENTER(2,14,this,14,lpD3DStats);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->GetStats(lpD3DStats));
}

HRESULT WINAPI glDirect3DDevice1::Initialize(LPDIRECT3D lpd3d, LPGUID lpGUID, LPD3DDEVICEDESC lpd3ddvdesc)
{
	TRACE_ENTER(4,14,this,14,lpd3d,24,lpGUID,14,lpd3ddvdesc);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,DDERR_ALREADYINITIALIZED);
}
HRESULT WINAPI glDirect3DDevice1::NextViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport, LPDIRECT3DVIEWPORT *lplpDirect3DViewport, DWORD dwFlags)
{
	TRACE_ENTER(4,14,this,14,lpDirect3DViewport,14,lplpDirect3DViewport,9,dwFlags);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lplpDirect3DViewport) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	FIXME("glDirect3DDevice1::NextViewport: stub");
	TRACE_EXIT(23,DDERR_GENERIC);
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice1::Pick(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport,
	DWORD dwFlags, LPD3DRECT lpRect)
{
	TRACE_ENTER(5,14,this,14,lpDirect3DExecuteBuffer,14,lpDirect3DViewport,9,dwFlags,26,lpRect);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->Pick(lpDirect3DExecuteBuffer, lpDirect3DViewport, dwFlags, lpRect));
}

HRESULT WINAPI glDirect3DDevice1::SetMatrix(D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix)
{
	TRACE_ENTER(3,14,this,9,d3dMatHandle,14,lpD3DMatrix);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT,23,glD3DDev7->SetMatrix(d3dMatHandle,lpD3DMatrix));
}

HRESULT WINAPI glDirect3DDevice1::SwapTextureHandles(LPDIRECT3DTEXTURE lpD3DTex1, LPDIRECT3DTEXTURE lpD3DTex2)
{
	TRACE_ENTER(3,14,this,14,lpD3DTex1,14,lpD3DTex2);
	if(!this) TRACE_RET(HRESULT,23,DDERR_INVALIDOBJECT);
	if(!lpD3DTex1) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	if(!lpD3DTex2) TRACE_RET(HRESULT,23,DDERR_INVALIDPARAMS);
	LPDIRECT3DTEXTURE2 tex1, tex2;
	lpD3DTex1->QueryInterface(IID_IDirect3DTexture2,(void**)&tex1);
	lpD3DTex2->QueryInterface(IID_IDirect3DTexture2,(void**)&tex2);
	HRESULT ret = glD3DDev7->SwapTextureHandles(tex1,tex2);
	tex2->Release();
	tex1->Release();
	TRACE_EXIT(23,ret);
	return ret;
}