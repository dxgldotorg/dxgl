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
#include "shadergen.h"
#include "glutil.h"
#include "matrix.h"

const DWORD renderstate_default[153] = {0,                 // 0
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
	//TODO:  Implement texture stages.
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
	for(i = 0; i < numtex; i++)
	{
		vertdata[i+10].data = &vertptr[ptr];
		texformats[i] = (dwVertexTypeDesc>>(16+(2*i))&3);
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
HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::EnumTextureFormats: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetCaps: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::GetLight: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetLightEnalbe: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::GetRenderTarget: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetTexture: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	if(!this) return DDERR_INVALIDPARAMS;
	FIXME("glDirect3DDevice7::GetTextureStageState: stub");
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
	FIXME("glDirect3DDevice7::SetRenderTarget: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::SetTexture: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	if(!this) return DDERR_INVALIDPARAMS;
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
	__gluMultMatricesf(matView,matWorld,worldview);	// Get worldview
	if(__gluInvertMatrixf(worldview,tmp)) // Invert
		memcpy(matNormal,tmp,16*sizeof(GLfloat));
	else memcpy(matNormal,worldview,16*sizeof(GLfloat));
	normal_dirty = false;
}