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

#pragma once
#ifndef __GLDIRECT3DDEVICE_H
#define __GLDIRECT3DDEVICE_H

class glDirectDrawSurface7;
class glDirect3D7;

struct TEXTURESTAGE
{
	D3DTEXTUREOP colorop;
	DWORD colorarg1;
	DWORD colorarg2;
	D3DTEXTUREOP alphaop;
	DWORD alphaarg1;
	DWORD alphaarg2;
	D3DVALUE bumpenv00;
	D3DVALUE bumpenv01;
	D3DVALUE bumpenv10;
	D3DVALUE bumpenv11;
	DWORD texcoordindex;
	D3DTEXTUREADDRESS addressu;
	D3DTEXTUREADDRESS addressv;
	DWORD bordercolor;
	D3DTEXTUREMAGFILTER magfilter;
	D3DTEXTUREMINFILTER minfilter;
	D3DTEXTUREMIPFILTER mipfilter;
	D3DVALUE lodbias;
	DWORD miplevel;
	DWORD anisotropy;
	D3DVALUE bumpenvlscale;
	D3DVALUE bumpenvloffset;
	D3DTEXTURETRANSFORMFLAGS textransform;
	glDirectDrawSurface7 *texture;
	bool dirty;
	__int64 shaderid;
	GLint glmagfilter;
	GLint glminfilter;
};

class glDirect3DLight;
class glDirectDrawSurface7;
class glDirect3DDevice7 : public IDirect3DDevice7
{
public:
	glDirect3DDevice7(glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7);
	virtual ~glDirect3DDevice7();
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI ApplyStateBlock(DWORD dwBlockHandle);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI BeginStateBlock();
	HRESULT WINAPI CaptureStateBlock(DWORD dwBlockHandle);
	HRESULT WINAPI CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle);
	HRESULT WINAPI Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	HRESULT WINAPI ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
		DWORD dwFlags, LPDWORD lpdwReturnValues);
	HRESULT WINAPI DeleteStateBlock(DWORD dwBlockHandle);
	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVerticexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT WINAPI EndScene();
	HRESULT WINAPI EndStateBlock(LPDWORD lpdwBlockHandle);
	HRESULT WINAPI EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
	HRESULT WINAPI GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc);
	HRESULT WINAPI GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation);
	HRESULT WINAPI GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI GetDirect3D(LPDIRECT3D7 *lplpD3D);
	HRESULT WINAPI GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
	HRESULT WINAPI GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
	HRESULT WINAPI GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable);
	HRESULT WINAPI GetMaterial(LPD3DMATERIAL7 lpMaterial);
	HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT WINAPI GetRenderTarget(LPDIRECTDRAWSURFACE7 *lplpRenderTarget);
	HRESULT WINAPI GetStateData(DWORD dwState, LPVOID* lplpStateData);
	HRESULT WINAPI GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture);
	HRESULT WINAPI GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
	HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI GetViewport(LPD3DVIEWPORT7 lpViewport);
	HRESULT WINAPI LightEnable(DWORD dwLightIndex, BOOL bEnable);
	HRESULT WINAPI Load(LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
		LPRECT lprcSrcRect, DWORD dwFlags);
	HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture);
	HRESULT WINAPI SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation);
	HRESULT WINAPI SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
	HRESULT WINAPI SetMaterial(LPD3DMATERIAL7 lpMaterial);
	HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
	HRESULT WINAPI SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags);
	HRESULT WINAPI SetStateData(DWORD dwState, LPVOID lpStateData);
	HRESULT WINAPI SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture);
	HRESULT WINAPI SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
	HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI SetViewport(LPD3DVIEWPORT7 lpViewport);
	HRESULT WINAPI ValidateDevice(LPDWORD lpdwPasses);
	void SetArraySize(DWORD size, DWORD vertex, DWORD texcoord);
	__int64 SelectShader(GLVERTEX *VertexType);
	D3DMATRIX matrices[24];
	D3DMATERIAL7 material;
	D3DVIEWPORT7 viewport;
	glDirect3DLight **lights;
	int gllights[8];
	glDirectDrawSurface7 *glDDS7;
	DWORD renderstate[153];
	TEXTURESTAGE texstages[8];
	glDirect3D7 *glD3D7;

private:
	HRESULT fvftoglvertex(DWORD dwVertexTypeDesc,LPDWORD vertptr);
	ULONG refcount;
	GLuint gltextures[8];
	DWORD lightsmax;
	bool inscene;
	DWORD maxarray;
	DWORD vertexsize;
	DWORD texcoordsize;
	GLfloat *vertices;
	GLfloat *normals;
	GLubyte *diffuse;
	GLubyte *specular;
	GLubyte *ambient;
	GLfloat *texcoords[8];
	GLVERTEX vertdata[18];
	DWORD texformats[8];
};

#endif //__GLDIRECT3DDEVICE_H