// DXGL
// Copyright (C) 2011-2015 William Feely

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

struct glDirectDrawSurface7;
class glDirect3D7;

struct D3D1MATRIX
{
	BOOL active;
	D3DMATRIX matrix;
};

class glDirect3DLight;
class glDirect3DMaterial3;
struct glDirect3DViewport3;
class glDirect3DDevice3;
class glDirect3DDevice2;
class glDirect3DDevice1;
class glDirect3DDevice7 : public IDirect3DDevice7
{
public:
	glDirect3DDevice7(REFCLSID rclsid, glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7, IUnknown *creator, int version);
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
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
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
	HRESULT err() {return error;}
	void SetArraySize(DWORD size, DWORD vertex, DWORD texcoord);
	D3DMATERIALHANDLE AddMaterial(glDirect3DMaterial3* material);
	D3DTEXTUREHANDLE AddTexture(glDirectDrawSurface7* texture);
	HRESULT AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags);
	HRESULT GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport);
	HRESULT SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport);
	HRESULT Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags);
	HRESULT BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT Index(WORD wVertexIndex);
	HRESULT Vertex(LPVOID lpVertex);
	HRESULT End(DWORD dwFlags);
	HRESULT GetCaps3(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
	HRESULT SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
	HRESULT GetStats(LPD3DSTATS lpD3DStats);
	HRESULT SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2);
	HRESULT CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
		IUnknown* pUnkOuter);
	HRESULT Execute(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags);
	HRESULT CreateMatrix(LPD3DMATRIXHANDLE lpD3DMatHandle);
	HRESULT DeleteMatrix(D3DMATRIXHANDLE d3dMatHandle);
	HRESULT GetMatrix(D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT SetMatrix(D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT GetPickRecords(LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec); 
	HRESULT Pick(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, 
		LPD3DRECT lpRect);
	inline void TransformViewport(D3DTLVERTEX *vertex);
	INT TransformAndLight(D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
	INT TransformOnly(D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
	INT TransformOnly(D3DTLVERTEX **output, DWORD *outsize, D3DLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
	INT CopyVertices(D3DTLVERTEX **output, DWORD *outsize, D3DTLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
	void UpdateTransform();
	void InitDX2();
	void InitDX5();
	//__int64 SelectShader(GLVERTEX *VertexType);
	void SetScale(D3DVALUE x, D3DVALUE y){scalex = x; scaley = y;}
	ULONG AddRefInternal();
	ULONG ReleaseInternal();
	GLfloat matWorld[16];
	GLfloat matView[16];
	GLfloat matProjection[16];
	GLfloat matTransform[16];
	bool transform_dirty;
	D3D1MATRIX *matrices;
	int matrixcount;
	D3DMATERIAL7 material;
	D3DVIEWPORT7 viewport;
	glDirect3DLight **lights;
	int gllights[8];
	glDirectDrawSurface7 *glDDS7;
	DWORD renderstate[153];
	glDirectDrawSurface7 *texstagesurfaces[8];
	TEXTURESTAGE texstages[8];
	glDirect3D7 *glD3D7;
	glDirect3DMaterial3 **materials;
	glDirect3DMaterial3 *currentmaterial;
	int materialcount;
	glDirectDrawSurface7 **textures;
	int texturecount;
	bool modelview_dirty;
	bool projection_dirty;
	D3DSTATS stats;
	glDirect3DDevice3 *glD3DDev3;
	glDirect3DDevice2 *glD3DDev2;
	glDirect3DDevice1 *glD3DDev1;
private:
	HRESULT error;
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
	int texformats[8];
	int maxmaterials;
	int maxtextures;
	glDirect3DViewport3 **viewports;
	glDirect3DViewport3 *currentviewport;
	int viewportcount;
	int maxviewports;
	unsigned char *ebBuffer;
	DWORD ebBufferSize;
	unsigned char *outbuffer;
	DWORD outbuffersize;
	D3DVALUE scalex;
	D3DVALUE scaley;
	D3DMATRIXHANDLE mhWorld;
	D3DMATRIXHANDLE mhView;
	D3DMATRIXHANDLE mhProjection;
	D3DDEVICEDESC7 d3ddesc;
	D3DDEVICEDESC d3ddesc3;
	glRenderer *renderer;
	IUnknown *creator;
	int version;
	BOOL dx5init;
	BOOL dx2init;
};

class glDirect3DDevice3 : public IDirect3DDevice3
{
public:
	glDirect3DDevice3(glDirect3DDevice7 *glD3DDev7);
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT WINAPI Begin(D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags);
	HRESULT WINAPI BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues); 
	HRESULT WINAPI DeleteViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPVOID lpvVertices, DWORD  dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
		LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT WINAPI End(DWORD dwFlags);
	HRESULT WINAPI EndScene();
	HRESULT WINAPI EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
	HRESULT WINAPI GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT WINAPI GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI GetCurrentViewport(LPDIRECT3DVIEWPORT3 *lplpd3dViewport);
	HRESULT WINAPI GetDirect3D(LPDIRECT3D3 *lplpD3D);
	HRESULT WINAPI GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
	HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT WINAPI GetRenderTarget(LPDIRECTDRAWSURFACE4 *lplpRenderTarget);
	HRESULT WINAPI GetStats(LPD3DSTATS lpD3DStats);
	HRESULT WINAPI GetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 * lplpTexture); 
	HRESULT WINAPI GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
	HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI Index(WORD wVertexIndex);
	HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI NextViewport(LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags);
	HRESULT WINAPI SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI SetCurrentViewport(LPDIRECT3DVIEWPORT3 lpd3dViewport);
	HRESULT WINAPI SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
	HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
	HRESULT WINAPI SetRenderTarget(LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags);
	HRESULT WINAPI SetTexture(DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture); 
	HRESULT WINAPI SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
	HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI ValidateDevice(LPDWORD lpdwPasses);
	HRESULT WINAPI Vertex(LPVOID lpVertex);
	glDirect3DDevice7 *GetGLD3DDev7(){return glD3DDev7;}
private:
	glDirect3DDevice7 *glD3DDev7;
};


class glDirect3DDevice2 : public IDirect3DDevice2
{
public:
	glDirect3DDevice2(glDirect3DDevice7 *glD3DDev7);
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
	HRESULT WINAPI Begin(D3DPRIMITIVETYPE d3dpt, D3DVERTEXTYPE d3dvt, DWORD dwFlags);
	HRESULT WINAPI BeginIndexed(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI DeleteViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
	HRESULT WINAPI DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
		LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT WINAPI DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT WINAPI End(DWORD dwFlags);
	HRESULT WINAPI EndScene();
	HRESULT WINAPI EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
	HRESULT WINAPI GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT WINAPI GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI GetCurrentViewport(LPDIRECT3DVIEWPORT2 *lplpd3dViewport2);
	HRESULT WINAPI GetDirect3D(LPDIRECT3D2 *lplpD3D2);
	HRESULT WINAPI GetLightState(D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
	HRESULT WINAPI GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT WINAPI GetRenderTarget(LPDIRECTDRAWSURFACE *lplpRenderTarget);
	HRESULT WINAPI GetStats(LPD3DSTATS lpD3DStats);
	HRESULT WINAPI GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI Index(WORD wVertexIndex);
	HRESULT WINAPI MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI NextViewport(LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags);
	HRESULT WINAPI SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT WINAPI SetCurrentViewport(LPDIRECT3DVIEWPORT2 lpd3dViewport2);
	HRESULT WINAPI SetLightState(D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
	HRESULT WINAPI SetRenderState(D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState);
	HRESULT WINAPI SetRenderTarget(LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags);
	HRESULT WINAPI SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI SwapTextureHandles(LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2);
	HRESULT WINAPI Vertex(LPVOID lpVertexType);
	glDirect3DDevice7 *GetGLD3DDev7(){ return glD3DDev7; }
private:
	glDirect3DDevice7 *glD3DDev7;
};

class glDirect3DDevice1 : public IDirect3DDevice
{
public:
	glDirect3DDevice1(glDirect3DDevice7 *glD3DDev7);
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();
	HRESULT WINAPI AddViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport);
	HRESULT WINAPI BeginScene();
	HRESULT WINAPI CreateExecuteBuffer(LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
		IUnknown* pUnkOuter);
	HRESULT WINAPI CreateMatrix(LPD3DMATRIXHANDLE lpD3DMatHandle);
	HRESULT WINAPI DeleteMatrix(D3DMATRIXHANDLE d3dMatHandle);
	HRESULT WINAPI DeleteViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport);
	HRESULT WINAPI EndScene();
	HRESULT WINAPI EnumTextureFormats(LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
	HRESULT WINAPI Execute(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags);
	HRESULT WINAPI GetCaps(LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT WINAPI GetDirect3D(LPDIRECT3D* lpD3D);
	HRESULT WINAPI GetMatrix(D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI GetPickRecords(LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec); 
	HRESULT WINAPI GetStats(LPD3DSTATS lpD3DStats);
	HRESULT WINAPI Initialize(LPDIRECT3D lpd3d, LPGUID lpGUID, LPD3DDEVICEDESC lpd3ddvdesc); 
	HRESULT WINAPI NextViewport(LPDIRECT3DVIEWPORT lpDirect3DViewport, LPDIRECT3DVIEWPORT* lplpDirect3DViewport, DWORD dwFlags);
	HRESULT WINAPI Pick(LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, 
		LPD3DRECT lpRect);
	HRESULT WINAPI SetMatrix(D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT WINAPI SwapTextureHandles(LPDIRECT3DTEXTURE lpD3DTex1, LPDIRECT3DTEXTURE lpD3DTex2);
	glDirect3DDevice7 *GetGLD3DDev7(){ return glD3DDev7; }
private:
	glDirect3DDevice7 *glD3DDev7;
};

#endif //__GLDIRECT3DDEVICE_H
