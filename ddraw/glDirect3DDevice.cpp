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
#include "ddraw.h"
#include "glDirect3D.h"
#include "texture.h"
#include "glRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DTexture.h"
#include "glDirect3DMaterial.h"
#include "glDirect3DViewport.h"
#include "glDirect3DVertexBuffer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include <string>
using namespace std;
#include "shadergen.h"
#include "glutil.h"
#include "matrix.h"

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
	D3DTBLEND_MODULATE, //texturemapblend
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
	D3DMCS_COLOR2, //ambientmaterialsource
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


glDirect3DDevice7::glDirect3DDevice7(glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7)
{
	int zbuffer = 0;
	maxmaterials = 32;
	materials = (glDirect3DMaterial3**)malloc(32*sizeof(glDirect3DMaterial3*));
	materialcount = 1;
	materials[0] = NULL;
	currentmaterial = NULL;
	maxtextures = 32;
	textures = (glDirectDrawSurface7**)malloc(32*sizeof(glDirectDrawSurface7*));
	texturecount = 1;
	textures[0] = NULL;
	maxviewports = 32;
	currentviewport = NULL;
	viewportcount = 0;
	viewports = (glDirect3DViewport3**)malloc(32*sizeof(glDirect3DViewport3*));
	ZeroMemory(viewports,32*sizeof(glDirect3DViewport3*));
	vertices = normals = NULL;
	diffuse = specular = NULL;
	ZeroMemory(texcoords,8*sizeof(GLfloat*));
	memcpy(renderstate,renderstate_default,153*sizeof(DWORD));
	__gluMakeIdentityf(matWorld);
	__gluMakeIdentityf(matView);
	__gluMakeIdentityf(matProjection);
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
	ZeroMemory(lights,16*sizeof(glDirect3DLight*));
	memset(gllights,0xff,8*sizeof(int));
	memset(gltextures,0,8*sizeof(GLuint));
	d3ddesc.dwMaxTextureWidth = d3ddesc.dwMaxTextureHeight =
		d3ddesc.dwMaxTextureRepeat = d3ddesc.dwMaxTextureAspectRatio = renderer->gl_caps.TextureMax;
	renderer->InitD3D(zbuffer);
}
glDirect3DDevice7::~glDirect3DDevice7()
{
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
	free(viewports);
	free(materials);
	glD3D7->Release();
	glDDS7->Release();
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
	if(!this) return DDERR_INVALIDOBJECT;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	if(riid == IID_IDirect3DDevice7)
	{
		this->AddRef();
		*ppvObj = this;
		return D3D_OK;
	}
	if(riid == IID_IDirect3DDevice3)
	{
		if(glD3DDev3)
		{
			*ppvObj = glD3DDev3;
			glD3DDev3->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DDevice3(this);
			glD3DDev3 = (glDirect3DDevice3*)*ppvObj;
			return D3D_OK;
		}
	}
	if(riid == IID_IDirect3DDevice2)
	{
		if(glD3DDev2)
		{
			*ppvObj = glD3DDev2;
			glD3DDev2->AddRef();
			return D3D_OK;
		}
		else
		{
			this->AddRef();
			*ppvObj = new glDirect3DDevice2(this);
			glD3DDev2 = (glDirect3DDevice2*)*ppvObj;
			return D3D_OK;
		}
	}
	ERR(E_NOINTERFACE);
}

ULONG WINAPI glDirect3DDevice7::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DDevice7::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DDevice7::ApplyStateBlock(DWORD dwBlockHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::ApplyStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::BeginScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(inscene) return D3DERR_SCENE_IN_SCENE;
	inscene = true;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::BeginStateBlock()
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::BeginStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CaptureStateBlock(DWORD dwBlockHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::CaptureStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::CreateStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwCount && !lpRects) return DDERR_INVALIDPARAMS;
	return renderer->Clear(glDDS7,dwCount,lpRects,dwFlags,dwColor,dvZ,dwStencil);
}
HRESULT WINAPI glDirect3DDevice7::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::ComputeSphereVisibility: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DeleteStateBlock(DWORD dwBlockHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::DeleteStateBlock: stub");
	ERR(DDERR_GENERIC);
}

void glDirect3DDevice7::SetArraySize(DWORD size, DWORD vertex, DWORD texcoord)
{
	if(!vertices) vertices = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > maxarray) vertices = (GLfloat*)realloc(vertices,size*4*sizeof(GLfloat));
	if(!normals) normals = (GLfloat*)malloc(size*4*sizeof(GLfloat));
	else if(size > maxarray) normals = (GLfloat*)realloc(normals,size*4*sizeof(GLfloat));
}

__int64 glDirect3DDevice7::SelectShader(GLVERTEX *VertexType)
{
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
	shader |= (((renderstate[D3DRENDERSTATE_ALPHAFUNC]-1) & 7) << 3);
	shader |= ((renderstate[D3DRENDERSTATE_FOGTABLEMODE] & 3) << 6);
	shader |= ((renderstate[D3DRENDERSTATE_FOGVERTEXMODE] & 3) << 8);
	if(renderstate[D3DRENDERSTATE_RANGEFOGENABLE]) shader |= (1 << 10);
	if(renderstate[D3DRENDERSTATE_SPECULARENABLE]) shader |= (1 << 11);
	if(renderstate[D3DRENDERSTATE_STIPPLEDALPHA]) shader |= (1 << 12);
	if(renderstate[D3DRENDERSTATE_COLORKEYENABLE]) shader |= (1 << 13);
	shader |= ((renderstate[D3DRENDERSTATE_ZBIAS] & 15) << 14);
	int numlights = 0;
	for(i = 0; i < 8; i++)
		if(gllights[i] != -1) numlights++;
	shader |= numlights << 18;
	if(renderstate[D3DRENDERSTATE_LOCALVIEWER]) shader |= (1 << 21);
	if(renderstate[D3DRENDERSTATE_COLORKEYBLENDENABLE]) shader |= (1 << 22);
	shader |= ((renderstate[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] & 3) << 23);
	shader |= ((renderstate[D3DRENDERSTATE_SPECULARMATERIALSOURCE] & 3) << 25);
	shader |= ((renderstate[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] & 3) << 27);
	shader |= ((renderstate[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] & 3) << 29);
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
	//TODO:  Implement texture stages.
	for(i = 0; i < 8; i++)
	{
		if(!texstages[i].dirty) continue;
		texstages[i].shaderid = texstages[i].colorop & 31;
		texstages[i].shaderid |= (texstages[i].colorarg1 & 63) << 5;
		texstages[i].shaderid |= (texstages[i].colorarg2 & 63) << 11;
		texstages[i].shaderid |= (texstages[i].alphaop & 31) << 17;
		texstages[i].shaderid |= (texstages[i].alphaarg1 & 63) << 22;
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
		if(texstages[i].texture) texstages[i].shaderid |= 1i64 << 59;
	}
	return shader;
}

HRESULT glDirect3DDevice7::fvftoglvertex(DWORD dwVertexTypeDesc,LPDWORD vertptr)
{
	int i;
	int ptr = 0;
	if((dwVertexTypeDesc & D3DFVF_XYZ) && (dwVertexTypeDesc & D3DFVF_XYZRHW))
		return DDERR_INVALIDPARAMS;
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
	else return DDERR_INVALIDPARAMS;
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
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!inscene) return D3DERR_SCENE_NOT_IN_SCENE;
	HRESULT err = fvftoglvertex(dwVertexTypeDesc,(LPDWORD)lpvVertices);
	if(err != D3D_OK) return err;
	return renderer->DrawPrimitives(this,setdrawmode(d3dptPrimitiveType),vertdata,texformats,
		dwVertexCount,lpwIndices,dwIndexCount,dwFlags);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVerticexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return DrawIndexedPrimitive(dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,NULL,0,dwFlags);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::DrawPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::DrawPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::EndScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!inscene) return D3DERR_SCENE_NOT_IN_SCENE;
	inscene = false;
	renderer->Flush();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::EndStateBlock(LPDWORD lpdwBlockHandle)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::EndStateBlock: stub");
	ERR(DDERR_GENERIC);
}

// Use EXACTLY one line per entry.  Don't change layout of the list.
const int TEXFMT_START = __LINE__;
const DDPIXELFORMAT texpixelformats[] = 
{
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0xF00,0xF0,0xF,0xF000},
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0x7C00,0x3E0,0x1F,0x8000},
	{sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0x7C00,0x3E0,0x1F,0},
	{sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0xF800,0x7E0,0x1F,0},
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,32,0xFF0000,0xFF00,0xFF,0xFF000000},
	{sizeof(DDPIXELFORMAT),DDPF_RGB,0,32,0xFF0000,0xFF00,0xFF,0},
	{sizeof(DDPIXELFORMAT),DDPF_RGB,0,24,0xFF0000,0xFF00,0xFF,0}
};
const int TEXFMT_END = __LINE__ - 4;
const int numtexfmt = TEXFMT_END-TEXFMT_START;

HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	if(!this) return DDERR_INVALIDOBJECT;
	HRESULT result;
	DDPIXELFORMAT fmt;
	for(int i = 0; i < numtexfmt; i++)
	{
		memcpy(&fmt,&texpixelformats[i],sizeof(DDPIXELFORMAT));
		result = lpd3dEnumPixelProc(&fmt,lpArg);
		if(result != D3DENUMRET_OK) return D3D_OK;
	}
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	if(!this) return DDERR_INVALIDOBJECT;
	D3DDEVICEDESC7 desc = d3ddesc;
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	desc.deviceGUID = IID_IDirect3DTnLHalDevice;
	memcpy(lpD3DDevDesc,&desc,sizeof(D3DDEVICEDESC7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::GetClipPlane: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::GetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetDirect3D(LPDIRECT3D7 *lplpD3D)
{
	if(!this) return DDERR_INVALIDOBJECT;
	*lplpD3D = glD3D7;
	glD3D7->AddRef();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::GetInfo: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpLight) return DDERR_INVALIDPARAMS;
	if(dwLightIndex >= lightsmax) ERR(DDERR_INVALIDOBJECT);
	if(!lights[dwLightIndex]) ERR(DDERR_INVALIDOBJECT);
	lights[dwLightIndex]->GetLight7(lpLight);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwLightIndex >= lightsmax) ERR(DDERR_INVALIDOBJECT);
	if(!lights[dwLightIndex]) ERR(DDERR_INVALIDOBJECT);
	if(!pbEnable) return DDERR_INVALIDPARAMS;
	*pbEnable = FALSE;
	for(int i = 0; i < 8; i++)
		if(gllights[i] == dwLightIndex) *pbEnable = TRUE;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(lpMaterial,&material,sizeof(D3DMATERIAL7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwRenderStateType <= 152)
	{
		*lpdwRenderState = renderstate[dwRenderStateType];
		return D3D_OK;
	}
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderTarget(LPDIRECTDRAWSURFACE7 *lplpRenderTarget)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lplpRenderTarget) return DDERR_INVALIDPARAMS;
	glDDS7->AddRef();
	*lplpRenderTarget = glDDS7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::GetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	if(!lplpTexture) return DDERR_INVALIDPARAMS;
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwStage > 7) return DDERR_INVALIDPARAMS;
	if(!texstages[dwStage].texture) return DDERR_INVALIDOBJECT;
	*lplpTexture = texstages[dwStage].texture;
	texstages[dwStage].texture->AddRef();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	if(dwStage > 7) return DDERR_INVALIDPARAMS;
	if(!lpdwValue) return DDERR_INVALIDPARAMS;
	if(!this) return DDERR_INVALIDOBJECT;
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		*lpdwValue = texstages[dwStage].colorop;
		return D3D_OK;
	case D3DTSS_COLORARG1:
		*lpdwValue = texstages[dwStage].colorarg1;
		return D3D_OK;
	case D3DTSS_COLORARG2:
		*lpdwValue = texstages[dwStage].colorarg2;
		return D3D_OK;
	case D3DTSS_ALPHAOP:
		*lpdwValue = texstages[dwStage].alphaop;
		return D3D_OK;
	case D3DTSS_ALPHAARG1:
		*lpdwValue = texstages[dwStage].alphaarg1;
		return D3D_OK;
	case D3DTSS_ALPHAARG2:
		*lpdwValue = texstages[dwStage].alphaarg2;
		return D3D_OK;
	case D3DTSS_BUMPENVMAT00:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv00,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_BUMPENVMAT01:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv01,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_BUMPENVMAT10:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv10,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_BUMPENVMAT11:
		memcpy(lpdwValue,&texstages[dwStage].bumpenv11,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_TEXCOORDINDEX:
		*lpdwValue = texstages[dwStage].texcoordindex;
		return D3D_OK;
	case D3DTSS_ADDRESS:
	case D3DTSS_ADDRESSU:
		*lpdwValue = texstages[dwStage].addressu;
		return D3D_OK;
	case D3DTSS_ADDRESSV:
		*lpdwValue = texstages[dwStage].addressv;
		return D3D_OK;
	case D3DTSS_BORDERCOLOR:
		*lpdwValue = texstages[dwStage].bordercolor;
		return D3D_OK;
	case D3DTSS_MAGFILTER:
		*lpdwValue = texstages[dwStage].magfilter;
		return D3D_OK;
	case D3DTSS_MINFILTER:
		*lpdwValue = texstages[dwStage].minfilter;
		return D3D_OK;
	case D3DTSS_MIPFILTER:
		*lpdwValue = texstages[dwStage].mipfilter;
		return D3D_OK;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(lpdwValue,&texstages[dwStage].lodbias,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_MAXMIPLEVEL:
		*lpdwValue = texstages[dwStage].miplevel;
		return D3D_OK;
	case D3DTSS_MAXANISOTROPY:
		*lpdwValue = texstages[dwStage].anisotropy;
		return D3D_OK;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(lpdwValue,&texstages[dwStage].bumpenvlscale,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(lpdwValue,&texstages[dwStage].bumpenvloffset,sizeof(D3DVALUE));
		return D3D_OK;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		*lpdwValue = texstages[dwStage].textransform;
		return D3D_OK;
	default:
		return DDERR_INVALIDPARAMS;
	}
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(lpD3DMatrix,&matWorld,sizeof(D3DMATRIX));
		return D3D_OK;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(lpD3DMatrix,&matView,sizeof(D3DMATRIX));
		return D3D_OK;
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(lpD3DMatrix,&matProjection,sizeof(D3DMATRIX));
		return D3D_OK;
	default:
		ERR(DDERR_INVALIDPARAMS);
	}
}
HRESULT WINAPI glDirect3DDevice7::GetViewport(LPD3DVIEWPORT7 lpViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	memcpy(lpViewport,&viewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
	if(!this) return DDERR_INVALIDOBJECT;
	int i;
	D3DLIGHT7 light;
	bool foundlight = false;
	if(dwLightIndex >= lightsmax)
	{
		if(!ExpandLightBuffer(&lights,&lightsmax,dwLightIndex-1)) return DDERR_OUTOFMEMORY;
	}
	if(!lights[dwLightIndex]) lights[dwLightIndex] = new glDirect3DLight;
	if(bEnable)
	{
		for(i = 0; i < 8; i++)
			if(gllights[i] == dwLightIndex) return D3D_OK;
		for(i = 0; i < 8; i++)
		{
			if(gllights[i] == -1)
			{
				foundlight = true;
				gllights[i] = dwLightIndex;
				break;
			}
		}
		if(!foundlight) return D3DERR_LIGHT_SET_FAILED;
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
		return D3D_OK;
	}
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::Load(LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
	LPRECT lprcSrcRect, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::Load: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::MultiplyTransform: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::PreLoad: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::SetClipPland: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::SetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	if(!this) return DDERR_INVALIDOBJECT;
	bool foundlight = false;
	if(dwLightIndex >= lightsmax)
	{
		if(!ExpandLightBuffer(&lights,&lightsmax,dwLightIndex-1)) return DDERR_OUTOFMEMORY;
	}
	if(!lights[dwLightIndex]) lights[dwLightIndex] = new glDirect3DLight;
	lights[dwLightIndex]->SetLight7(lpLight);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(&material,lpMaterial,sizeof(D3DMATERIAL7));
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwRendStateType > 152) return DDERR_INVALIDPARAMS;
	if(dwRendStateType < 0) return DDERR_INVALIDPARAMS;
	renderstate[dwRendStateType] = dwRenderState;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpNewRenderTarget) return DDERR_INVALIDPARAMS;
	if(dwFlags) return DDERR_INVALIDPARAMS;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	lpNewRenderTarget->GetSurfaceDesc(&ddsd);
	if(!(ddsd.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)) return DDERR_INVALIDSURFACETYPE;
	glDDS7->Release();
	glDDS7 = (glDirectDrawSurface7*)lpNewRenderTarget;
	glDDS7->AddRef();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetStateData(DWORD dwState, LPVOID lpStateData)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::SetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwStage > 7) return DDERR_INVALIDPARAMS;
	if(texstages[dwStage].texture) texstages[dwStage].texture->Release();
	texstages[dwStage].texture = (glDirectDrawSurface7*)lpTexture;
	texstages[dwStage].dirty = true;
	if(lpTexture) lpTexture->AddRef();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(dwStage > 7) return DDERR_INVALIDPARAMS;
	switch(dwState)
	{
	case D3DTSS_COLOROP:
		if(!dwValue || (dwValue > 24)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].colorop = (D3DTEXTUREOP)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_COLORARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) return DDERR_INVALIDPARAMS;
		if(dwValue > 0x34) return DDERR_INVALIDPARAMS;
		texstages[dwStage].colorarg1 = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_COLORARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) return DDERR_INVALIDPARAMS;
		if(dwValue > 0x34) return DDERR_INVALIDPARAMS;
		texstages[dwStage].colorarg2 = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ALPHAOP:
		if(!dwValue || (dwValue > 24)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].alphaop = (D3DTEXTUREOP )dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ALPHAARG1:
		if((dwValue & D3DTA_SELECTMASK) > 4) return DDERR_INVALIDPARAMS;
		if(dwValue > 0x34) return DDERR_INVALIDPARAMS;
		texstages[dwStage].alphaarg1 = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ALPHAARG2:
		if((dwValue & D3DTA_SELECTMASK) > 4) return DDERR_INVALIDPARAMS;
		if(dwValue > 0x34) return DDERR_INVALIDPARAMS;
		texstages[dwStage].alphaarg2 = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVMAT00:
		memcpy(&texstages[dwStage].bumpenv00,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVMAT01:
		memcpy(&texstages[dwStage].bumpenv01,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVMAT10:
		memcpy(&texstages[dwStage].bumpenv10,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVMAT11:
		memcpy(&texstages[dwStage].bumpenv11,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_TEXCOORDINDEX:
		if((dwValue & 0xFFFF) > 7) return DDERR_INVALIDPARAMS;
		if((dwValue >> 16) > 3) return DDERR_INVALIDPARAMS;
		texstages[dwStage].texcoordindex = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ADDRESS:
		if(!dwValue || (dwValue > 4)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ADDRESSU:
		if(!dwValue || (dwValue > 4)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_ADDRESSV:
		if(!dwValue || (dwValue > 4)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BORDERCOLOR:
		texstages[dwStage].bordercolor = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_MAGFILTER:
		if(!dwValue || (dwValue > 5)) return DDERR_INVALIDPARAMS;
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
		return D3D_OK;
	case D3DTSS_MINFILTER:
		if(!dwValue || (dwValue > 3)) return DDERR_INVALIDPARAMS;
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
		return D3D_OK;
	case D3DTSS_MIPFILTER:
		if(!dwValue || (dwValue > 3)) return DDERR_INVALIDPARAMS;
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
		return D3D_OK;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(&texstages[dwStage].lodbias,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_MAXMIPLEVEL:
		texstages[dwStage].miplevel = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_MAXANISOTROPY:
		texstages[dwStage].anisotropy = dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(&texstages[dwStage].bumpenvlscale,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(&texstages[dwStage].bumpenvloffset,&dwValue,sizeof(D3DVALUE));
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		if((dwValue & 0xFF) > 4) return DDERR_INVALIDPARAMS;
		if((dwValue >> 8) > 1) return DDERR_INVALIDPARAMS;
		texstages[dwStage].textransform = (D3DTEXTURETRANSFORMFLAGS)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	default:
		return DDERR_INVALIDPARAMS;
	}
	FIXME("glDirect3DDevice7::SetTextureStageState: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(&matWorld,lpD3DMatrix,sizeof(D3DMATRIX));
		modelview_dirty = true;
		return D3D_OK;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(&matView,lpD3DMatrix,sizeof(D3DMATRIX));
		modelview_dirty = true;
		return D3D_OK;
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(&matProjection,lpD3DMatrix,sizeof(D3DMATRIX));
		projection_dirty = true;
		return D3D_OK;
	default:
		ERR(DDERR_INVALIDPARAMS);
	}
}
HRESULT WINAPI glDirect3DDevice7::SetViewport(LPD3DVIEWPORT7 lpViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	memcpy(&viewport,lpViewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::ValidateDevice(LPDWORD lpdwPasses)
{
	if(!this) return DDERR_INVALIDOBJECT;
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
			return D3DERR_UNSUPPORTEDCOLOROPERATION;
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
			return D3DERR_UNSUPPORTEDALPHAOPERATION;
		}
	}
	if(lpdwPasses) *lpdwPasses = 1;
	return D3D_OK;
}

void glDirect3DDevice7::SetDepthComp()
{
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
}

D3DMATERIALHANDLE glDirect3DDevice7::AddMaterial(glDirect3DMaterial3 *material)
{
	materials[materialcount] = material;
	material->AddRef();
	materialcount++;
	if(materialcount >= maxmaterials)
	{
		maxmaterials += 32;
		materials = (glDirect3DMaterial3**)realloc(materials,maxmaterials*sizeof(glDirect3DMaterial3*));
	}
	return materialcount-1;
}

D3DTEXTUREHANDLE glDirect3DDevice7::AddTexture(glDirectDrawSurface7 *texture)
{
	textures[texturecount] = texture;
	texture->AddRef();
	texturecount++;
	if(texturecount >= maxtextures)
	{
		maxtextures += 32;
		textures = (glDirectDrawSurface7**)realloc(textures,maxtextures*sizeof(glDirectDrawSurface7*));
	}
	return texturecount-1;
}

HRESULT glDirect3DDevice7::AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DViewport) return DDERR_INVALIDPARAMS;
	for(int i = 0; i < maxviewports; i++)
	{
		if(viewports[i] == lpDirect3DViewport) return DDERR_INVALIDPARAMS;
	}
	viewports[viewportcount] = (glDirect3DViewport3*)lpDirect3DViewport;
	viewports[viewportcount]->AddRef();
	viewports[viewportcount]->SetDevice(this);
	viewportcount++;
	if(viewportcount >= maxviewports)
	{
		maxviewports += 32;
		viewports = (glDirect3DViewport3**)realloc(viewports,maxviewports*sizeof(glDirect3DViewport3*));
	}
	return D3D_OK;
}

HRESULT glDirect3DDevice7::DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DViewport) return DDERR_INVALIDPARAMS;
	for(int i = 0; i < maxviewports; i++)
	{
		if(viewports[i] == lpDirect3DViewport)
		{
			viewports[i]->SetCurrent(false);
			viewports[i]->SetDevice(NULL);
			viewports[i]->Release();
			if(currentviewport == viewports[i]) currentviewport = NULL;
			viewports[i] = NULL;
			return D3D_OK;
		}
	}
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7::NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DViewport) return DDERR_INVALIDPARAMS;
	if(!lplpAnotherViewport) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::NextViewport: stub");
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lplpd3dViewport) return DDERR_INVALIDPARAMS;
	if(!currentviewport) return D3DERR_NOCURRENTVIEWPORT;
	*lplpd3dViewport = currentviewport;
	currentviewport->AddRef();
	return D3D_OK;
}

HRESULT glDirect3DDevice7::SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpd3dViewport) return DDERR_INVALIDPARAMS;
	if(currentviewport == lpd3dViewport) return D3D_OK;
	for(int i = 0; i < maxviewports; i++)
	{
		if(lpd3dViewport == viewports[i])
		{
			viewports[i]->SetCurrent(true);
			currentviewport = (glDirect3DViewport3*)lpd3dViewport;
			return D3D_OK;
		}
	}
	return DDERR_INVALIDPARAMS;
}

HRESULT glDirect3DDevice7::Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::Begin: stub");
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpvVertices) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::BeginIndexed: stub");
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::Index(WORD wVertexIndex)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::Index: stub");
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::Vertex(LPVOID lpVertex)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpVertex) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::Vertex: stub");
	return DDERR_GENERIC;
}
HRESULT glDirect3DDevice7::End(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice7::End: stub");
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::ComputeSphereVisibility3(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	if(!this) return DDERR_INVALIDOBJECT;
	FIXME("glDirect3DDevice3::ComputeSphereVisibility: stub");
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::GetCaps3(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpD3DHWDevDesc && !lpD3DHELDevDesc) return DDERR_INVALIDPARAMS;
	D3DDEVICEDESC desc = d3ddesc3;
	if(lpD3DHELDevDesc) *lpD3DHELDevDesc = desc;
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION;
	if(lpD3DHWDevDesc) *lpD3DHWDevDesc = desc;
	return D3D_OK;
}

HRESULT glDirect3DDevice7::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpdwLightState)return DDERR_INVALIDPARAMS;
	switch(dwLightStateType)
	{
	default:
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(currentmaterial) *lpdwLightState = currentmaterial->handle;
		else *lpdwLightState = 0;
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		return GetRenderState(D3DRENDERSTATE_AMBIENT,lpdwLightState);
	case D3DLIGHTSTATE_COLORMODEL:
		*lpdwLightState = D3DCOLOR_RGB;
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		return GetRenderState(D3DRENDERSTATE_FOGVERTEXMODE,lpdwLightState);
	case D3DLIGHTSTATE_FOGSTART:
		return GetRenderState(D3DRENDERSTATE_FOGSTART,lpdwLightState);
	case D3DLIGHTSTATE_FOGEND:
		return GetRenderState(D3DRENDERSTATE_FOGEND,lpdwLightState);
	case D3DLIGHTSTATE_FOGDENSITY:
		return GetRenderState(D3DRENDERSTATE_FOGDENSITY,lpdwLightState);
	case D3DLIGHTSTATE_COLORVERTEX:
		return GetRenderState(D3DRENDERSTATE_COLORVERTEX,lpdwLightState);
	}
}
HRESULT glDirect3DDevice7::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	switch(dwLightStateType)
	{
	default:
		return DDERR_INVALIDPARAMS;
	case D3DLIGHTSTATE_MATERIAL:
		if(!dwLightState) return DDERR_INVALIDPARAMS;
		if(dwLightState < materialcount)
		{
			if(materials[dwLightState] == currentmaterial) return D3D_OK;
			if(materials[dwLightState])
			{
				if(currentmaterial)currentmaterial->SetCurrent(false);
				materials[dwLightState]->SetCurrent(true);
				currentmaterial = materials[dwLightState];
			}
		}
		return D3D_OK;
	case D3DLIGHTSTATE_AMBIENT:
		return SetRenderState(D3DRENDERSTATE_AMBIENT,dwLightState);
	case D3DLIGHTSTATE_COLORMODEL:
		return D3D_OK;
	case D3DLIGHTSTATE_FOGMODE:
		return SetRenderState(D3DRENDERSTATE_FOGVERTEXMODE,dwLightState);
	case D3DLIGHTSTATE_FOGSTART:
		return SetRenderState(D3DRENDERSTATE_FOGSTART,dwLightState);
	case D3DLIGHTSTATE_FOGEND:
		return SetRenderState(D3DRENDERSTATE_FOGEND,dwLightState);
	case D3DLIGHTSTATE_FOGDENSITY:
		return SetRenderState(D3DRENDERSTATE_FOGDENSITY,dwLightState);
	case D3DLIGHTSTATE_COLORVERTEX:
		return SetRenderState(D3DRENDERSTATE_COLORVERTEX,dwLightState);
	}
}

HRESULT glDirect3DDevice7::GetStats(LPD3DSTATS lpD3DStats)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpD3DStats) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetStats: stub");
	return DDERR_GENERIC;
}

HRESULT glDirect3DDevice7::SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpD3DTex1) return DDERR_INVALIDPARAMS;
	if(!lpD3DTex2) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::SwapTextureHandles: stub");
	return DDERR_GENERIC;
}

// IDirect3DDevice3 wrapper
glDirect3DDevice3::glDirect3DDevice3(glDirect3DDevice7 *glD3DDev7)
{
	this->glD3DDev7 = glD3DDev7;
	refcount = 1;
}

glDirect3DDevice3::~glDirect3DDevice3()
{
	glD3DDev7->glD3DDev3 = NULL;
	glD3DDev7->Release();
}

HRESULT WINAPI glDirect3DDevice3::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glD3DDev7->QueryInterface(riid,ppvObj);
}

ULONG WINAPI glDirect3DDevice3::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DDevice3::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DDevice3::AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->AddViewport(lpDirect3DViewport);	
}

HRESULT WINAPI glDirect3DDevice3::Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->Begin(d3dpt,dwVertexTypeDesc,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->BeginIndexed(dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwNumVertices,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::BeginScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->BeginScene();
}

HRESULT WINAPI glDirect3DDevice3::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->ComputeSphereVisibility3(lpCenters,lpRadii,dwNumSpheres,dwFlags,lpdwReturnValues);
}

HRESULT WINAPI glDirect3DDevice3::DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->DeleteViewport(lpDirect3DViewport);
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD  dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->DrawIndexedPrimitive(d3dptPrimitiveType,dwVertexTypeDesc,lpvVertices,dwVertexCount,lpwIndices,dwIndexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->DrawIndexedPrimitiveStrided(d3dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,lpwIndices,dwIndexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpd3dVertexBuffer) return DDERR_INVALIDPARAMS;
	return glD3DDev7->DrawIndexedPrimitiveVB(d3dptPrimitiveType,
		((glDirect3DVertexBuffer1*)lpd3dVertexBuffer)->GetGLD3DVB7(),0,-1,lpwIndices,dwIndexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->DrawPrimitive(dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return  DDERR_INVALIDOBJECT;
	return glD3DDev7->DrawPrimitiveStrided(dptPrimitiveType,dwVertexTypeDesc,lpVertexArray,dwVertexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpd3dVertexBuffer) return DDERR_INVALIDPARAMS;
	return glD3DDev7->DrawPrimitiveVB(d3dptPrimitiveType,((glDirect3DVertexBuffer1*)lpd3dVertexBuffer)->GetGLD3DVB7(),
		dwStartVertex,dwNumVertices,dwFlags);
}
HRESULT WINAPI glDirect3DDevice3::End(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->End(dwFlags);
}
	
HRESULT WINAPI glDirect3DDevice3::EndScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->EndScene();
}

HRESULT WINAPI glDirect3DDevice3::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->EnumTextureFormats(lpd3dEnumPixelProc,lpArg);
}

HRESULT WINAPI glDirect3DDevice3::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetCaps3(lpD3DHWDevDesc,lpD3DHELDevDesc);
}

HRESULT WINAPI glDirect3DDevice3::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetClipStatus(lpD3DClipStatus);
}

HRESULT WINAPI glDirect3DDevice3::GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetCurrentViewport(lplpd3dViewport);
}

HRESULT WINAPI glDirect3DDevice3::GetDirect3D(LPDIRECT3D3 *lplpD3D)
{
	if(!this) return DDERR_INVALIDOBJECT;
	LPDIRECT3D7 d3d7;
	HRESULT err = glD3DDev7->GetDirect3D(&d3d7);
	if(!d3d7) return err;
	d3d7->QueryInterface(IID_IDirect3D3,(void**)lplpD3D);
	d3d7->Release();
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetLightState(dwLightStateType,lpdwLightState);
}

HRESULT WINAPI glDirect3DDevice3::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetRenderState(dwRenderStateType,lpdwRenderState);
}

HRESULT WINAPI glDirect3DDevice3::GetRenderTarget(LPDIRECTDRAWSURFACE4 *lplpRenderTarget)
{
	if(!this) return DDERR_INVALIDOBJECT;
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetRenderTarget(&dds7);
	if(!dds7) return err;
	dds7->QueryInterface(IID_IDirectDrawSurface4,(void**)lplpRenderTarget);
	dds7->Release();
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetStats(LPD3DSTATS lpD3DStats)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetStats(lpD3DStats);
}

HRESULT WINAPI glDirect3DDevice3::GetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 * lplpTexture)
{
	if(!this) return DDERR_INVALIDOBJECT;
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetTexture(dwStage,&dds7);
	if(!dds7) return err;
	dds7->QueryInterface(IID_IDirect3DTexture2,(void**)lplpTexture);
	dds7->Release();
	return err;
}

HRESULT WINAPI glDirect3DDevice3::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetTextureStageState(dwStage,dwState,lpdwValue);
}

HRESULT WINAPI glDirect3DDevice3::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetTransform(dtstTransformStateType,lpD3DMatrix);
}

HRESULT WINAPI glDirect3DDevice3::Index(WORD wVertexIndex)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->Index(wVertexIndex);
}

HRESULT WINAPI glDirect3DDevice3::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->MultiplyTransform(dtstTransformStateType,lpD3DMatrix);
}
HRESULT WINAPI glDirect3DDevice3::NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->NextViewport(lpDirect3DViewport,lplpAnotherViewport,dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetClipStatus(lpD3DClipStatus);
}

HRESULT WINAPI glDirect3DDevice3::SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetCurrentViewport(lpd3dViewport);
}

HRESULT WINAPI glDirect3DDevice3::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetLightState(dwLightStateType,dwLightState);
}

HRESULT WINAPI glDirect3DDevice3::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetRenderState(dwRendStateType,dwRenderState);
}
	
HRESULT WINAPI glDirect3DDevice3::SetRenderTarget(LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetRenderTarget(((glDirectDrawSurface4*)lpNewRenderTarget)->GetDDS7(),dwFlags);
}

HRESULT WINAPI glDirect3DDevice3::SetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture)
{
	if(!this) return DDERR_INVALIDOBJECT;
	glDirectDrawSurface7 *dds7;
	if(lpTexture) dds7 = ((glDirect3DTexture2*)lpTexture)->GetDDS7();
	else dds7 = NULL;
	return glD3DDev7->SetTexture(dwStage,dds7);
}

HRESULT WINAPI glDirect3DDevice3::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetTextureStageState(dwStage,dwState,dwValue);
}

HRESULT WINAPI glDirect3DDevice3::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetTransform(dtstTransformStateType,lpD3DMatrix);
}

HRESULT WINAPI glDirect3DDevice3::ValidateDevice(LPDWORD lpdwPasses)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->ValidateDevice(lpdwPasses);
}

HRESULT WINAPI glDirect3DDevice3::Vertex(LPVOID lpVertex)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->Vertex(lpVertex);
}

// IDirect3DDevice2 wrapper
glDirect3DDevice2::glDirect3DDevice2(glDirect3DDevice7 *glD3DDev7)
{
	this->glD3DDev7 = glD3DDev7;
	refcount = 1;
}

glDirect3DDevice2::~glDirect3DDevice2()
{
	glD3DDev7->glD3DDev2 = NULL;
	glD3DDev7->Release();
}

HRESULT WINAPI glDirect3DDevice2::QueryInterface(REFIID riid, void** ppvObj)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(riid == IID_IUnknown)
	{
		this->AddRef();
		*ppvObj = this;
		return DD_OK;
	}
	return glD3DDev7->QueryInterface(riid,ppvObj);
}

ULONG WINAPI glDirect3DDevice2::AddRef()
{
	if(!this) return 0;
	refcount++;
	return refcount;
}

ULONG WINAPI glDirect3DDevice2::Release()
{
	if(!this) return 0;
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::AddViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DViewport2) return DDERR_INVALIDPARAMS;
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->AddViewport(glD3DV3);
	glD3DV3->Release();
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
	if(!this) return DDERR_INVALIDOBJECT;
	DWORD vertextype = d3dvttofvf(d3dvt);
	if(!vertextype) return DDERR_INVALIDPARAMS;
	return glD3DDev7->Begin(d3dpt,vertextype,dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) return DDERR_INVALIDPARAMS;
	return glD3DDev7->BeginIndexed(dptPrimitiveType,vertextype,lpvVertices,dwNumVertices,dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::BeginScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->BeginScene();
}

HRESULT WINAPI glDirect3DDevice2::DeleteViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	glDirect3DViewport3 *glD3DV3;
	lpDirect3DViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->DeleteViewport(glD3DV3);
	glD3DV3->Release();
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	DWORD vertextype = d3dvttofvf(d3dvtVertexType);
	if(!vertextype) return DDERR_INVALIDPARAMS;
	return glD3DDev7->DrawIndexedPrimitive(d3dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwIndices,dwIndexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	DWORD vertextype = d3dvttofvf(dvtVertexType);
	if(!vertextype) return DDERR_INVALIDPARAMS;
	return glD3DDev7->DrawPrimitive(dptPrimitiveType,vertextype,lpvVertices,dwVertexCount,dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::End(DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->End(dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::EndScene()
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->EndScene();
}

HRESULT WINAPI EnumTex2(LPDDPIXELFORMAT ddpf, LPVOID lpUserArg)
{
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
	if(!this) return DDERR_INVALIDOBJECT;
	LPVOID context[2];
	context[0] = (LPVOID)lpd3dEnumTextureProc;
	context[1] = lpArg;
	return glD3DDev7->EnumTextureFormats(EnumTex2,&context);
}

HRESULT WINAPI glDirect3DDevice2::GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetCaps3(lpD3DHWDevDesc,lpD3DHELDevDesc);
}

HRESULT WINAPI glDirect3DDevice2::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetClipStatus(lpD3DClipStatus);
}

HRESULT WINAPI glDirect3DDevice2::GetCurrentViewport(LPDIRECT3DVIEWPORT2 *lplpd3dViewport2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	glDirect3DViewport3 *glD3DV3;
	HRESULT ret = glD3DDev7->GetCurrentViewport((LPDIRECT3DVIEWPORT3*)&glD3DV3);
	if(!glD3DV3) return ret;
	glD3DV3->QueryInterface(IID_IDirect3DViewport2,(void**)lplpd3dViewport2);
	glD3DV3->Release();
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::GetDirect3D(LPDIRECT3D2 *lplpD3D2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	LPDIRECT3D7 d3d7;
	HRESULT err = glD3DDev7->GetDirect3D(&d3d7);
	if(!d3d7) return err;
	d3d7->QueryInterface(IID_IDirect3D2,(void**)lplpD3D2);
	d3d7->Release();
	return err;
}

HRESULT WINAPI glDirect3DDevice2::GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetLightState(dwLightStateType,lpdwLightState);
}

HRESULT WINAPI glDirect3DDevice2::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetRenderState(dwRenderStateType,lpdwRenderState);
}

HRESULT WINAPI glDirect3DDevice2::GetRenderTarget(LPDIRECTDRAWSURFACE *lplpRenderTarget)
{
	if(!this) return DDERR_INVALIDOBJECT;
	LPDIRECTDRAWSURFACE7 dds7;
	HRESULT err = glD3DDev7->GetRenderTarget(&dds7);
	if(!dds7) return err;
	dds7->QueryInterface(IID_IDirectDrawSurface,(void**)lplpRenderTarget);
	dds7->Release();
	return err;
}

HRESULT WINAPI glDirect3DDevice2::GetStats(LPD3DSTATS lpD3DStats)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetStats(lpD3DStats);
}

HRESULT WINAPI glDirect3DDevice2::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->GetTransform(dtstTransformStateType,lpD3DMatrix);
}

HRESULT WINAPI glDirect3DDevice2::Index(WORD wVertexIndex)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->Index(wVertexIndex);
}

HRESULT WINAPI glDirect3DDevice2::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->MultiplyTransform(dtstTransformStateType,lpD3DMatrix);
}

HRESULT WINAPI glDirect3DDevice2::NextViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpDirect3DViewport2) return DDERR_INVALIDPARAMS;
	if(!lplpDirect3DViewport2) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice2::NextViewport: stub");
	ERR(DDERR_GENERIC);
}

HRESULT WINAPI glDirect3DDevice2::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetClipStatus(lpD3DClipStatus);
}

HRESULT WINAPI glDirect3DDevice2::SetCurrentViewport(LPDIRECT3DVIEWPORT2 lpd3dViewport2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	if(!lpd3dViewport2) return glD3DDev7->SetCurrentViewport(NULL);
	glDirect3DViewport3 *glD3DV3;
	lpd3dViewport2->QueryInterface(IID_IDirect3DViewport3,(void**)&glD3DV3);
	HRESULT ret = glD3DDev7->SetCurrentViewport(glD3DV3);
	glD3DV3->Release();
	return ret;
}

HRESULT WINAPI glDirect3DDevice2::SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetLightState(dwLightStateType,dwLightState);
}

HRESULT WINAPI glDirect3DDevice2::SetRenderState(D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetRenderState(dwRenderStateType,dwRenderState);
}

HRESULT WINAPI glDirect3DDevice2::SetRenderTarget(LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetRenderTarget(((glDirectDrawSurface1*)lpNewRenderTarget)->GetDDS7(),dwFlags);
}

HRESULT WINAPI glDirect3DDevice2::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SetTransform(dtstTransformStateType,lpD3DMatrix);
}

HRESULT WINAPI glDirect3DDevice2::SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->SwapTextureHandles(lpD3DTex1,lpD3DTex2);
}

HRESULT WINAPI glDirect3DDevice2::Vertex(LPVOID lpVertexType)
{
	if(!this) return DDERR_INVALIDOBJECT;
	return glD3DDev7->Vertex(lpVertexType);
}
