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
#include "glDirectDrawSurface.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include "glutil.h"

inline void dwordto4float(DWORD in, GLfloat *out)
{
	out[0] = (GLfloat)((in>>16) & 0xff) / 255.0f;
	out[1] = (GLfloat)((in>>8) & 0xff) / 255.0f;
	out[2] = (GLfloat)(in& 0xff) / 255.0f;
	out[3] = (GLfloat)((in>>24) & 0xff) / 255.0f;
}

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

D3DMATRIX identity;

glDirect3DDevice7::glDirect3DDevice7(glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7)
{
	memcpy(renderstate,renderstate_default,153*sizeof(DWORD));
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	identity._11 = identity._22 = identity._33 = identity._44 = 1.0;
	identity._12 = identity._13 = identity._14 =
		identity._21 = identity._23 = identity._24 = 
		identity._31 = identity._32 = identity._34 =
		identity._41 = identity._42 = identity._43 = 0.0;
	matWorld = matView = matProjection = identity;
	refcount = 1;
	inscene = false;
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	this->glDDS7 = glDDS7;
	glDDS7->AddRef();
	ZeroMemory(&viewport,sizeof(D3DVIEWPORT7));
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	if(glDDS7->GetZBuffer()) glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DITHER);
	glEnable(GL_LIGHTING);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
	ZeroMemory(&material,sizeof(D3DMATERIAL7));
	lightsmax = 16;
	lights = (glDirect3DLight**) malloc(16*sizeof(glDirect3DLight*));
	ZeroMemory(lights,16*sizeof(glDirect3DLight*));
	memset(gllights,0xff,8*sizeof(int));

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
	ERR(E_NOINTERFACE);
}

ULONG WINAPI glDirect3DDevice7::AddRef()
{
	refcount++;
	return refcount;
}
ULONG WINAPI glDirect3DDevice7::Release()
{
	ULONG ret;
	refcount--;
	ret = refcount;
	if(refcount == 0) delete this;
	return ret;
}

HRESULT WINAPI glDirect3DDevice7::ApplyStateBlock(DWORD dwBlockHandle)
{
	FIXME("glDirect3DDevice7::ApplyStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::BeginScene()
{
	if(inscene) return D3DERR_SCENE_IN_SCENE;
	inscene = true;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::BeginStateBlock()
{
	FIXME("glDirect3DDevice7::BeginStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CaptureStateBlock(DWORD dwBlockHandle)
{
	FIXME("glDirect3DDevice7::CaptureStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle)
{
	FIXME("glDirect3DDevice7::CreateStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	if(dwCount && !lpRects) return DDERR_INVALIDPARAMS;
	if(dwCount) ERR(DDERR_INVALIDPARAMS);
	GLfloat color[4];
	dwordto4float(dwColor,color);
	SetFBO(glDDS7->texture,glDDS7->GetZBuffer()->texture,glDDS7->GetZBuffer()->hasstencil);
	int clearbits = 0;
	if(D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		glClearColor(color[0],color[1],color[2],color[3]);
	}
	if(D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		glClearDepth(dvZ);
	}
	if(D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		glClearStencil(dwStencil);
	}
	glClear(clearbits);
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues)
{
	FIXME("glDirect3DDevice7::ComputeSphereVisibility: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DeleteStateBlock(DWORD dwBlockHandle)
{
	FIXME("glDirect3DDevice7::DeleteStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawIndexedPrimitive: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVerticexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawIndexedPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawPrimitive: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawPrimitiveStrided: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::DrawPrimitiveVB: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::EndScene()
{
	if(!inscene) return D3DERR_SCENE_NOT_IN_SCENE;
	inscene = false;
	glFlush();
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::EndStateBlock(LPDWORD lpdwBlockHandle)
{
	FIXME("glDirect3DDevice7::EndStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg)
{
	FIXME("glDirect3DDevice7::EnumTextureFormats: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc)
{
	FIXME("glDirect3DDevice7::GetCaps: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation)
{
	FIXME("glDirect3DDevice7::GetClipPlane: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	FIXME("glDirect3DDevice7::GetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetDirect3D(LPDIRECT3D7 *lplpD3D)
{
	*lplpD3D = glD3D7;
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize)
{
	FIXME("glDirect3DDevice7::GetInfo: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
	FIXME("glDirect3DDevice7::GetLight: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable)
{
	FIXME("glDirect3DDevice7::GetLightEnalbe: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(lpMaterial,&material,sizeof(D3DMATERIAL7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	if(dwRenderStateType <= 152)
	{
		*lpdwRenderState = renderstate[dwRenderStateType];
		return D3D_OK;
	}
	return DDERR_INVALIDPARAMS;
}
HRESULT WINAPI glDirect3DDevice7::GetRenderTarget(LPDIRECTDRAWSURFACE7 *lplpRenderTarget)
{
	FIXME("glDirect3DDevice7::GetRenderTarget: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData)
{
	FIXME("glDirect3DDevice7::GetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture)
{
	FIXME("glDirect3DDevice7::GetTexture: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue)
{
	FIXME("glDirect3DDevice7::GetTextureStageState: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
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
	memcpy(lpViewport,&viewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
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
		lights[dwLightIndex]->SetGLLight(i);
	}
	else
	{
		for(i = 0; i < 8; i++)
		{
			if(gllights[i] == dwLightIndex)
			{
				lights[dwLightIndex]->SetGLLight(-1);
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
	FIXME("glDirect3DDevice7::Load: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	FIXME("glDirect3DDevice7::MultiplyTransform: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture)
{
	FIXME("glDirect3DDevice7::PreLoad: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation)
{
	FIXME("glDirect3DDevice7::SetClipPland: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus)
{
	FIXME("glDirect3DDevice7::SetClipStatus: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight)
{
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
	if(!lpMaterial) return DDERR_INVALIDPARAMS;
	memcpy(&material,lpMaterial,sizeof(D3DMATERIAL7));
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,(GLfloat*)&material.ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,(GLfloat*)&material.diffuse);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,(GLfloat*)&material.specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,(GLfloat*)&material.emissive);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,material.power);
	return D3D_OK;
}

HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	GLfloat floats[4];
	if(dwRendStateType > 152) return DDERR_INVALIDPARAMS;
	if(dwRendStateType < 0) return DDERR_INVALIDPARAMS;
	renderstate[dwRendStateType] = dwRenderState;
	switch(dwRendStateType)
	{
	case D3DRENDERSTATE_ANTIALIAS:
		if(dwRenderState == 0) glDisable(GL_MULTISAMPLE);
		else glEnable(GL_MULTISAMPLE);
		return D3D_OK;
	case D3DRENDERSTATE_TEXTUREPERSPECTIVE:
		if(dwRenderState) glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
		else glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);
		return D3D_OK;
	case D3DRENDERSTATE_ZENABLE:
		switch(dwRenderState)
		{
		case D3DZB_FALSE:
			glDisable(GL_DEPTH_TEST);
			break;
		case D3DZB_TRUE:
		default:
		case D3DZB_USEW:
			glEnable(GL_DEPTH_TEST);
		}
		return D3D_OK;
	case D3DRENDERSTATE_LIGHTING:
		if(dwRenderState) glEnable(GL_LIGHTING);
		else glDisable(GL_LIGHTING);
		return D3D_OK;
	case D3DRENDERSTATE_AMBIENT:
		dwordto4float(dwRenderState,floats);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,floats);
		return D3D_OK;
	default:
		ERR(DDERR_INVALIDPARAMS);
	}
}
HRESULT WINAPI glDirect3DDevice7::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags)
{
	FIXME("glDirect3DDevice7::SetRenderTarget: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetStateData(DWORD dwState, LPVOID lpStateData)
{
	FIXME("glDirect3DDevice7::SetStateData: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture)
{
	FIXME("glDirect3DDevice7::SetTexture: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	FIXME("glDirect3DDevice7::SetTextureStageState: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	switch(dtstTransformStateType)
	{
	case D3DTRANSFORMSTATE_WORLD:
		memcpy(&matWorld,lpD3DMatrix,sizeof(D3DMATRIX));
		return D3D_OK;
	case D3DTRANSFORMSTATE_VIEW:
		memcpy(&matView,lpD3DMatrix,sizeof(D3DMATRIX));
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
	memcpy(&viewport,lpViewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::ValidateDevice(LPDWORD lpdwPasses)
{
	FIXME("glDirect3DDevice7::ValidateDevice: stub");
	ERR(DDERR_GENERIC);
}