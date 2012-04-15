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
#include "glDirect3D.h"
#include "glRenderer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include <string>
using namespace std;
#include "shadergen.h"
#include "glutil.h"
#include "matrix.h"

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
	0,
	0
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
	0,
	0
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
	vertices = normals = NULL;
	diffuse = specular = NULL;
	ZeroMemory(texcoords,8*sizeof(GLfloat*));
	memcpy(renderstate,renderstate_default,153*sizeof(DWORD));
	__gluMakeIdentityf(matWorld);
	__gluMakeIdentityf(matView);
	__gluMakeIdentityf(matProjection);
	__gluMakeIdentityf(matNormal);
	texstages[0] = texstagedefault0;
	texstages[1] = texstages[2] = texstages[3] = texstages[4] = 
		texstages[5] = texstages[6] = texstages[7] = texstagedefault1;
	refcount = 1;
	inscene = false;
	normal_dirty = false;
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
	glD3D7->glDD7->renderer->InitD3D(zbuffer);
}
glDirect3DDevice7::~glDirect3DDevice7()
{
	for(int i = 0; i < lightsmax; i++)
		if(lights[i]) delete lights[i];
	delete lights;
	for(int i = 0; i < 8; i++)
		if(texstages[i].texture) texstages[i].texture->Release();
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
	if(!this) return DDERR_INVALIDPARAMS;
	if(!ppvObj) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::ApplyStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::BeginScene()
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(inscene) return D3DERR_SCENE_IN_SCENE;
	inscene = true;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::BeginStateBlock()
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::BeginStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CaptureStateBlock(DWORD dwBlockHandle)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::CaptureStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::CreateStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(dwCount && !lpRects) return DDERR_INVALIDPARAMS;
	return glD3D7->glDD7->renderer->Clear(glDDS7,dwCount,lpRects,dwFlags,dwColor,dvZ,dwStencil);
}
HRESULT WINAPI glDirect3DDevice7::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::ComputeSphereVisibility: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DeleteStateBlock(DWORD dwBlockHandle)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	for(int i = 0; i < 8; i++)
		if(VertexType[i+10].data) numtextures++;
	shader |= (__int64)numtextures << 31;
	if(VertexType[1].data) shader |= (1i64 << 34);
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
			lightindex++;
		}
	}
	int blendweights = 0;
	for(i = 0; i < 5; i++)
		if(VertexType[i+2].data) blendweights++;
	shader |= (__int64)blendweights << 46;
	if(renderstate[D3DRENDERSTATE_NORMALIZENORMALS]) shader |= (1i64 << 49);
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
	}
	texstages[i].shaderid |= (__int64)(texstages[i].texcoordindex&7) << 54;
	texstages[i].shaderid |= (__int64)((texstages[i].texcoordindex>>16)&3) << 57;
	if(texstages[i].texture) texstages[i].shaderid |= 1i64 << 59;
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
		vertdata[i+9].data = &vertptr[ptr];
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
	if(!this) return DDERR_INVALIDPARAMS;
	if(!inscene) return D3DERR_SCENE_NOT_IN_SCENE;
	HRESULT err = fvftoglvertex(dwVertexTypeDesc,(LPDWORD)lpvVertices);
	if(err != D3D_OK) return err;
	return glD3D7->glDD7->renderer->DrawPrimitives(this,setdrawmode(d3dptPrimitiveType),vertdata,texformats,
		dwVertexCount,lpwIndices,dwIndexCount,dwFlags);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVerticexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	return DrawIndexedPrimitive(dptPrimitiveType,dwVertexTypeDesc,lpVertices,dwVertexCount,NULL,0,dwFlags);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::DrawPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::DrawPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::EndScene()
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!inscene) return D3DERR_SCENE_NOT_IN_SCENE;
	inscene = false;
	glD3D7->glDD7->renderer->Flush();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::EndStateBlock(LPDWORD lpdwBlockHandle)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::EndStateBlock: stub");
	ERR(DDERR_GENERIC);
}

// Use EXACTLY one line per entry.  Don't change layout of the list.
const int TEXFMT_START = __LINE__;
const DDPIXELFORMAT texformats[] = 
{
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0xF00,0xF0,0xF,0xF000},
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,16,0x7C00,0x3E0,0x1F,0x8000},
	{sizeof(DDPIXELFORMAT),DDPF_RGB,0,16,0xF800,0x7E0,0x1F,0},
	{sizeof(DDPIXELFORMAT),DDPF_RGB|DDPF_ALPHAPIXELS,0,32,0xFF0000,0xFF00,0xFF,0xFF000000}
};
const int TEXFMT_END = __LINE__ - 4;
const int numtexfmt = TEXFMT_END-TEXFMT_START;

HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	HRESULT result;
	DDPIXELFORMAT fmt;
	for(int i = 0; i < numtexfmt; i++)
	{
		memcpy(&fmt,&texformats[i],sizeof(DDPIXELFORMAT));
		result = lpd3dEnumPixelProc(&fmt,lpArg);
		if(result != D3DENUMRET_OK) return D3D_OK;
	}
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	D3DDEVICEDESC7 desc = d3ddesc;
	desc.dwDevCaps |= D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_HWTRANSFORMANDLIGHT;
	desc.deviceGUID = IID_IDirect3DTnLHalDevice;
	memcpy(lpD3DDevDesc,&desc,sizeof(D3DDEVICEDESC7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetClipPlane: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetDirect3D(LPDIRECT3D7 *lplpD3D)
{
	if(!this) return DDERR_INVALIDPARAMS;
	*lplpD3D = glD3D7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetInfo: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpLight) return DDERR_INVALIDPARAMS;
	if(dwLightIndex >= lightsmax) ERR(DDERR_INVALIDOBJECT);
	if(!lights[dwLightIndex]) ERR(DDERR_INVALIDOBJECT);
	lights[dwLightIndex]->GetLight7(lpLight);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(lpMaterial,&material,sizeof(D3DMATERIAL7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(dwRenderStateType <= 152)
	{
		*lpdwRenderState = renderstate[dwRenderStateType];
		return D3D_OK;
	}
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderTarget(LPDIRECTDRAWSURFACE7 *lplpRenderTarget)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lplpRenderTarget) return DDERR_INVALIDPARAMS;
	*lplpRenderTarget = glDDS7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	if(!lplpTexture) return DDERR_INVALIDPARAMS;
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	memcpy(lpViewport,&viewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::Load: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::MultiplyTransform: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::PreLoad: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::SetClipPland: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::SetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(&material,lpMaterial,sizeof(D3DMATERIAL7));
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(dwRendStateType > 152) return DDERR_INVALIDPARAMS;
	if(dwRendStateType < 0) return DDERR_INVALIDPARAMS;
	renderstate[dwRendStateType] = dwRenderState;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::SetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
	if(!this) return DDERR_INVALIDPARAMS;
	if(dwStage > 7) return DDERR_INVALIDPARAMS;
	if(texstages[dwStage].texture) texstages[dwStage].texture->Release();
	texstages[dwStage].texture = (glDirectDrawSurface7*)lpTexture;
	texstages[dwStage].dirty = true;
	if(lpTexture) lpTexture->AddRef();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
		return D3D_OK;
	case D3DTSS_MINFILTER:
		if(!dwValue || (dwValue > 3)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].minfilter = (D3DTEXTUREMINFILTER)dwValue;
		texstages[dwStage].dirty = true;
		return D3D_OK;
	case D3DTSS_MIPFILTER:
		if(!dwValue || (dwValue > 3)) return DDERR_INVALIDPARAMS;
		texstages[dwStage].mipfilter = (D3DTEXTUREMIPFILTER)dwValue;
		texstages[dwStage].dirty = true;
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
	if(!this) return DDERR_INVALIDPARAMS;
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(&matWorld,lpD3DMatrix,sizeof(D3DMATRIX));
		normal_dirty = true;
		return D3D_OK;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(&matView,lpD3DMatrix,sizeof(D3DMATRIX));
		normal_dirty = true;
		return D3D_OK;
	case D3DTRANSFORMSTATE_PROJECTION:
		memcpy(&matProjection,lpD3DMatrix,sizeof(D3DMATRIX));
		return D3D_OK;
	default:
		ERR(DDERR_INVALIDPARAMS);
	}
}
HRESULT WINAPI glDirect3DDevice7::SetViewport(LPD3DVIEWPORT7 lpViewport)
{
	if(!this) return DDERR_INVALIDPARAMS;
	memcpy(&viewport,lpViewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::ValidateDevice(LPDWORD lpdwPasses)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::ValidateDevice: stub");
	ERR(DDERR_GENERIC);
}

void glDirect3DDevice7::UpdateNormalMatrix()
{
	GLfloat worldview[16];
	GLfloat tmp[16];

	ZeroMemory(&worldview,sizeof(D3DMATRIX));
	ZeroMemory(&tmp,sizeof(D3DMATRIX));
	__gluMultMatricesf(matWorld,matView,worldview);	// Get worldview
	if(__gluInvertMatrixf(worldview,tmp)) // Invert
	{
		memcpy(matNormal,tmp,3*sizeof(GLfloat));
		memcpy(matNormal+3,tmp+4,3*sizeof(GLfloat));
		memcpy(matNormal+6,tmp+8,3*sizeof(GLfloat));
	}
	else
	{
		memcpy(matNormal,worldview,3*sizeof(GLfloat));
		memcpy(matNormal+3,worldview+4,3*sizeof(GLfloat));
		memcpy(matNormal+6,worldview+8,3*sizeof(GLfloat));
	}

	normal_dirty = false;
}