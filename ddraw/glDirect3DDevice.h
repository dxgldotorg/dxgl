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

#pragma once
#ifndef __GLDIRECT3DDEVICE_H
#define __GLDIRECT3DDEVICE_H

struct glDirect3D7;

struct D3D1MATRIX
{
	BOOL active;
	D3DMATRIX matrix;
};

struct glDirect3DLight;
struct glDirectDrawSurface7;
struct glDirect3DMaterial3;
struct glDirect3DViewport3;
struct glDirect3DDevice3;
struct glDirect3DDevice2;
struct glDirect3DDevice1;

struct glDirect3DDevice7Vtbl;
// Structure for glDirect3DDevice7, emulates IDirect3DDevice7
typedef struct glDirect3DDevice7
{
	struct glDirect3DDevice7Vtbl *lpVtbl;

	GLfloat matWorld[16];
	GLfloat matView[16];
	GLfloat matProjection[16];
	GLfloat matTransform[16];
	bool transform_dirty;
	D3D1MATRIX *matrices;
	D3DMATRIXHANDLE matrixcount;
	D3DMATERIAL7 material;
	D3DVIEWPORT7 viewport;
	glDirect3DLight **lights;
	int gllights[8];
	glDirectDrawSurface7 *glDDS7;
	DWORD renderstate[RENDERSTATE_COUNT];
	TEXTURESTAGE texstages[8];
	glDirect3D7 *glD3D7;
	glDirect3DMaterial3 **materials;
	glDirect3DMaterial3 *currentmaterial;
	DWORD materialcount;
	glDirectDrawSurface7 **textures;
	DWORD texturecount;
	bool modelview_dirty;
	bool projection_dirty;
	D3DSTATS stats;
	glDirect3DDevice3 *glD3DDev3;
	glDirect3DDevice2 *glD3DDev2;
	glDirect3DDevice1 *glD3DDev1;

	HRESULT error;
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
	DWORD maxmaterials;
	DWORD maxtextures;
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

} glDirect3DDevice7;

typedef struct glDirect3DDevice7Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DDevice7 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI  *AddRef)(glDirect3DDevice7 *This);
	ULONG(WINAPI * Release)(glDirect3DDevice7  *This);

	HRESULT(WINAPI *GetCaps)(glDirect3DDevice7 *This, LPD3DDEVICEDESC7 lpD3DDevDesc);
	HRESULT(WINAPI *EnumTextureFormats)(glDirect3DDevice7 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
	HRESULT(WINAPI *BeginScene)(glDirect3DDevice7 *This);
	HRESULT(WINAPI *EndScene)(glDirect3DDevice7 *This);
	HRESULT(WINAPI *GetDirect3D)(glDirect3DDevice7 *This, LPDIRECT3D7 *lplpD3D);
	HRESULT(WINAPI *SetRenderTarget)(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags);
	HRESULT(WINAPI *GetRenderTarget)(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 *lplpRenderTarget);
	HRESULT(WINAPI *Clear)(glDirect3DDevice7 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	HRESULT(WINAPI *SetTransform)(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *GetTransform)(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *SetViewport)(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport);
	HRESULT(WINAPI *MultiplyTransform)(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *GetViewport)(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport);
	HRESULT(WINAPI *SetMaterial)(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial);
	HRESULT(WINAPI *GetMaterial)(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial);
	HRESULT(WINAPI *SetLight)(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
	HRESULT(WINAPI *GetLight)(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
	HRESULT(WINAPI *SetRenderState)(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
	HRESULT(WINAPI *GetRenderState)(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT(WINAPI *BeginStateBlock)(glDirect3DDevice7 *This);
	HRESULT(WINAPI *EndStateBlock)(glDirect3DDevice7 *This, LPDWORD lpdwBlockHandle);
	HRESULT(WINAPI *PreLoad)(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpddsTexture);
	HRESULT(WINAPI *DrawPrimitive)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitive)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPVOID lpvVertices, DWORD  dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *SetClipStatus)(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT(WINAPI *GetClipStatus)(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT(WINAPI *DrawPrimitiveStrided)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitiveStrided)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawPrimitiveVB)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitiveVB)(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *ComputeSphereVisibility)(glDirect3DDevice7 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues);
	HRESULT(WINAPI *GetTexture)(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 * lplpTexture);
	HRESULT(WINAPI *SetTexture)(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture);
	HRESULT(WINAPI *GetTextureStageState)(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
	HRESULT(WINAPI *SetTextureStageState)(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
	HRESULT(WINAPI *ValidateDevice)(glDirect3DDevice7 *This, LPDWORD lpdwPasses);
	HRESULT(WINAPI *ApplyStateBlock)(glDirect3DDevice7 *This, DWORD dwBlockHandle);
	HRESULT(WINAPI *CaptureStateBlock)(glDirect3DDevice7 *This, DWORD dwBlockHandle);
	HRESULT(WINAPI *DeleteStateBlock)(glDirect3DDevice7 *This, DWORD dwBlockHandle);
	HRESULT(WINAPI *CreateStateBlock)(glDirect3DDevice7 *This, D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle);
	HRESULT(WINAPI *Load)(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
		LPRECT lprcSrcRect, DWORD dwFlags);
	HRESULT(WINAPI *LightEnable)(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL bEnable);
	HRESULT(WINAPI *GetLightEnable)(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL* pbEnable);
	HRESULT(WINAPI *SetClipPlane)(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE* pPlaneEquation);
	HRESULT(WINAPI *GetClipPlane)(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE *pPlaneEquation);
	HRESULT(WINAPI *GetInfo)(glDirect3DDevice7 *This, DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
} glDirect3DDevice7Vtbl;


HRESULT glDirect3DDevice7_Create(REFCLSID rclsid, glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7,
	IUnknown *creator, int version, glDirect3DDevice7 **newdev);
void glDirect3DDevice7_Destroy(glDirect3DDevice7 *This);

HRESULT WINAPI glDirect3DDevice7_QueryInterface(glDirect3DDevice7 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DDevice7_AddRef(glDirect3DDevice7 *This);
ULONG WINAPI glDirect3DDevice7_Release(glDirect3DDevice7 *This);
HRESULT WINAPI glDirect3DDevice7_ApplyStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle);
HRESULT WINAPI glDirect3DDevice7_BeginScene(glDirect3DDevice7 *This);
HRESULT WINAPI glDirect3DDevice7_BeginStateBlock(glDirect3DDevice7 *This);
HRESULT WINAPI glDirect3DDevice7_CaptureStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle);
HRESULT WINAPI glDirect3DDevice7_CreateStateBlock(glDirect3DDevice7 *This, D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle);
HRESULT WINAPI glDirect3DDevice7_Clear(glDirect3DDevice7 *This, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
HRESULT WINAPI glDirect3DDevice7_ComputeSphereVisibility(glDirect3DDevice7 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres,
	DWORD dwFlags, LPDWORD lpdwReturnValues);
HRESULT WINAPI glDirect3DDevice7_DeleteStateBlock(glDirect3DDevice7 *This, DWORD dwBlockHandle);
HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitive(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitiveStrided(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_DrawIndexedPrimitiveVB(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_DrawPrimitive(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_DrawPrimitiveStrided(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_DrawPrimitiveVB(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_EndScene(glDirect3DDevice7 *This);
HRESULT WINAPI glDirect3DDevice7_EndStateBlock(glDirect3DDevice7 *This, LPDWORD lpdwBlockHandle);
HRESULT WINAPI glDirect3DDevice7_EnumTextureFormats(glDirect3DDevice7 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
HRESULT WINAPI glDirect3DDevice7_GetCaps(glDirect3DDevice7 *This, LPD3DDEVICEDESC7 lpD3DDevDesc);
HRESULT WINAPI glDirect3DDevice7_GetClipPlane(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE *pPlaneEquation);
HRESULT WINAPI glDirect3DDevice7_GetClipStatus(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice7_GetDirect3D(glDirect3DDevice7 *This, LPDIRECT3D7 *lplpD3D);
HRESULT WINAPI glDirect3DDevice7_GetInfo(glDirect3DDevice7 *This, DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
HRESULT WINAPI glDirect3DDevice7_GetLight(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
HRESULT WINAPI glDirect3DDevice7_GetLightEnable(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL* pbEnable);
HRESULT WINAPI glDirect3DDevice7_GetMaterial(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial);
HRESULT WINAPI glDirect3DDevice7_GetRenderState(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
HRESULT WINAPI glDirect3DDevice7_GetRenderTarget(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 *lplpRenderTarget);
HRESULT WINAPI glDirect3DDevice7_GetStateData(glDirect3DDevice7 *This, DWORD dwState, LPVOID* lplpStateData);
HRESULT WINAPI glDirect3DDevice7_GetTexture(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture);
HRESULT WINAPI glDirect3DDevice7_GetTextureStageState(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
HRESULT WINAPI glDirect3DDevice7_GetTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7_GetViewport(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport);
HRESULT WINAPI glDirect3DDevice7_LightEnable(glDirect3DDevice7 *This, DWORD dwLightIndex, BOOL bEnable);
HRESULT WINAPI glDirect3DDevice7_Load(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex,
	LPRECT lprcSrcRect, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_MultiplyTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7_PreLoad(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpddsTexture);
HRESULT WINAPI glDirect3DDevice7_SetClipPlane(glDirect3DDevice7 *This, DWORD dwIndex, D3DVALUE* pPlaneEquation);
HRESULT WINAPI glDirect3DDevice7_SetClipStatus(glDirect3DDevice7 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice7_SetLight(glDirect3DDevice7 *This, DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
HRESULT WINAPI glDirect3DDevice7_SetMaterial(glDirect3DDevice7 *This, LPD3DMATERIAL7 lpMaterial);
HRESULT WINAPI glDirect3DDevice7_SetRenderState(glDirect3DDevice7 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
HRESULT WINAPI glDirect3DDevice7_SetRenderTarget(glDirect3DDevice7 *This, LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7_SetStateData(glDirect3DDevice7 *This, DWORD dwState, LPVOID lpStateData);
HRESULT WINAPI glDirect3DDevice7_SetTexture(glDirect3DDevice7 *This, DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture);
HRESULT WINAPI glDirect3DDevice7_SetTextureStageState(glDirect3DDevice7 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
HRESULT WINAPI glDirect3DDevice7_SetTransform(glDirect3DDevice7 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7_SetViewport(glDirect3DDevice7 *This, LPD3DVIEWPORT7 lpViewport);
HRESULT WINAPI glDirect3DDevice7_ValidateDevice(glDirect3DDevice7 *This, LPDWORD lpdwPasses);
void glDirect3DDevice7_SetArraySize(glDirect3DDevice7 *This, DWORD size, DWORD vertex, DWORD texcoord);
D3DMATERIALHANDLE glDirect3DDevice7_AddMaterial(glDirect3DDevice7 *This, glDirect3DMaterial3* material);
D3DTEXTUREHANDLE glDirect3DDevice7_AddTexture(glDirect3DDevice7 *This, glDirectDrawSurface7* texture);
HRESULT glDirect3DDevice7_AddViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
HRESULT glDirect3DDevice7_DeleteViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
HRESULT WINAPI glDirect3DDevice7_EnumTextureFormats2(glDirect3DDevice7 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
HRESULT glDirect3DDevice7_NextViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags);
HRESULT glDirect3DDevice7_GetCurrentViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 *lplpd3dViewport);
HRESULT glDirect3DDevice7_SetCurrentViewport(glDirect3DDevice7 *This, LPDIRECT3DVIEWPORT3 lpd3dViewport);
HRESULT glDirect3DDevice7_Begin(glDirect3DDevice7 *This, D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags);
HRESULT glDirect3DDevice7_BeginIndexed(glDirect3DDevice7 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
HRESULT glDirect3DDevice7_Index(glDirect3DDevice7 *This, WORD wVertexIndex);
HRESULT glDirect3DDevice7_Vertex(glDirect3DDevice7 *This, LPVOID lpVertex);
HRESULT glDirect3DDevice7_End(glDirect3DDevice7 *This, DWORD dwFlags);
HRESULT glDirect3DDevice7_GetCaps3(glDirect3DDevice7 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
HRESULT glDirect3DDevice7_GetLightState(glDirect3DDevice7 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
HRESULT glDirect3DDevice7_SetLightState(glDirect3DDevice7 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
HRESULT glDirect3DDevice7_GetStats(glDirect3DDevice7 *This, LPD3DSTATS lpD3DStats);
HRESULT glDirect3DDevice7_SwapTextureHandles(glDirect3DDevice7 *This, LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2);
HRESULT glDirect3DDevice7_CreateExecuteBuffer(glDirect3DDevice7 *This, LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
	IUnknown* pUnkOuter);
HRESULT glDirect3DDevice7_Execute(glDirect3DDevice7 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags);
HRESULT glDirect3DDevice7_CreateMatrix(glDirect3DDevice7 *This, LPD3DMATRIXHANDLE lpD3DMatHandle);
HRESULT glDirect3DDevice7_DeleteMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE d3dMatHandle);
HRESULT glDirect3DDevice7_GetMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix);
HRESULT glDirect3DDevice7_SetMatrix(glDirect3DDevice7 *This, D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix);
HRESULT glDirect3DDevice7_GetPickRecords(glDirect3DDevice7 *This, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec);
HRESULT glDirect3DDevice7_Pick(glDirect3DDevice7 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags,
	LPD3DRECT lpRect);
inline void glDirect3DDevice7_TransformViewport(glDirect3DDevice7 *This, D3DTLVERTEX *vertex);
INT glDirect3DDevice7_TransformAndLight(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
INT glDirect3DDevice7_TransformOnly(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
INT glDirect3DDevice7_TransformOnlyLit(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
INT glDirect3DDevice7_CopyVertices(glDirect3DDevice7 *This, D3DTLVERTEX **output, DWORD *outsize, D3DTLVERTEX *input, WORD start, WORD dest, DWORD count, D3DRECT *extents);
void glDirect3DDevice7_UpdateTransform(glDirect3DDevice7 *This);
void glDirect3DDevice7_InitDX2(glDirect3DDevice7 *This);
void glDirect3DDevice7_InitDX5(glDirect3DDevice7 *This);
//__int64 glDirect3DDevice7_SelectShader(glDirect3DDevice7 *This, GLVERTEX *VertexType);
void glDirect3DDevice7_SetScale(glDirect3DDevice7 *This, D3DVALUE x, D3DVALUE y);
ULONG glDirect3DDevice7_AddRefInternal(glDirect3DDevice7 *This);
ULONG glDirect3DDevice7_ReleaseInternal(glDirect3DDevice7 *This);
HRESULT glDirect3DDevice7_fvftoglvertex(glDirect3DDevice7 *This, DWORD dwVertexTypeDesc,LPDWORD vertptr);


struct glDirect3DDevice3Vtbl;
// Structure for glDirect3DDevice3, emulates IDirect3DDevice3
typedef struct glDirect3DDevice3
{
	struct glDirect3DDevice3Vtbl *lpVtbl;

	glDirect3DDevice7 *glD3DDev7;
} glDirect3DDevice3;

typedef struct glDirect3DDevice3Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DDevice3 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI  *AddRef)(glDirect3DDevice3 *This);
	ULONG(WINAPI * Release)(glDirect3DDevice3  *This);

	HRESULT(WINAPI *GetCaps)(glDirect3DDevice3 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT(WINAPI *GetStats)(glDirect3DDevice3 *This, LPD3DSTATS lpD3DStats);
	HRESULT(WINAPI *AddViewport)(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT(WINAPI *DeleteViewport)(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
	HRESULT(WINAPI *NextViewport)(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags);
	HRESULT(WINAPI *EnumTextureFormats)(glDirect3DDevice3 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
	HRESULT(WINAPI *BeginScene)(glDirect3DDevice3 *This);
	HRESULT(WINAPI *EndScene)(glDirect3DDevice3 *This);
	HRESULT(WINAPI *GetDirect3D)(glDirect3DDevice3 *This, LPDIRECT3D3 *lplpD3D);
	HRESULT(WINAPI *SetCurrentViewport)(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpd3dViewport);
	HRESULT(WINAPI *GetCurrentViewport)(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 *lplpd3dViewport);
	HRESULT(WINAPI *SetRenderTarget)(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags);
	HRESULT(WINAPI *GetRenderTarget)(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 *lplpRenderTarget);
	HRESULT(WINAPI *Begin)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags);
	HRESULT(WINAPI *BeginIndexed)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT(WINAPI *Vertex)(glDirect3DDevice3 *This, LPVOID lpVertex);
	HRESULT(WINAPI *Index)(glDirect3DDevice3 *This, WORD wVertexIndex);
	HRESULT(WINAPI *End)(glDirect3DDevice3 *This, DWORD dwFlags);
	HRESULT(WINAPI *GetRenderState)(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT(WINAPI *SetRenderState)(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
	HRESULT(WINAPI *GetLightState)(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
	HRESULT(WINAPI *SetLightState)(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
	HRESULT(WINAPI *SetTransform)(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *GetTransform)(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *MultiplyTransform)(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *DrawPrimitive)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitive)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPVOID lpvVertices, DWORD  dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *SetClipStatus)(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT(WINAPI *GetClipStatus)(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT(WINAPI *DrawPrimitiveStrided)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitiveStrided)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
		LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawPrimitiveVB)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
		DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitiveVB)(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
		LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *ComputeSphereVisibility)(glDirect3DDevice3 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues);
	HRESULT(WINAPI *GetTexture)(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 * lplpTexture);
	HRESULT(WINAPI *SetTexture)(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture);
	HRESULT(WINAPI *GetTextureStageState)(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
	HRESULT(WINAPI *SetTextureStageState)(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
	HRESULT(WINAPI *ValidateDevice)(glDirect3DDevice3 *This, LPDWORD lpdwPasses);
} glDirect3DDevice3Vtbl;

HRESULT glDirect3DDevice3_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice3 **newdev);

HRESULT WINAPI glDirect3DDevice3_QueryInterface(glDirect3DDevice3 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DDevice3_AddRef(glDirect3DDevice3 *This);
ULONG WINAPI glDirect3DDevice3_Release(glDirect3DDevice3 *This);
HRESULT WINAPI glDirect3DDevice3_AddViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
HRESULT WINAPI glDirect3DDevice3_Begin(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dpt, DWORD dwVertexTypeDesc, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_BeginIndexed(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_BeginScene(glDirect3DDevice3 *This);
HRESULT WINAPI glDirect3DDevice3_ComputeSphereVisibility(glDirect3DDevice3 *This, LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues);
HRESULT WINAPI glDirect3DDevice3_DeleteViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport);
HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitive(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPVOID lpvVertices, DWORD  dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitiveStrided(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_DrawIndexedPrimitiveVB(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_DrawPrimitive(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices,
	DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_DrawPrimitiveStrided(glDirect3DDevice3 *This, D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc,
	LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_DrawPrimitiveVB(glDirect3DDevice3 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER lpd3dVertexBuffer,
	DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_End(glDirect3DDevice3 *This, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_EndScene(glDirect3DDevice3 *This);
HRESULT WINAPI glDirect3DDevice3_EnumTextureFormats(glDirect3DDevice3 *This, LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
HRESULT WINAPI glDirect3DDevice3_GetCaps(glDirect3DDevice3 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
HRESULT WINAPI glDirect3DDevice3_GetClipStatus(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice3_GetCurrentViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 *lplpd3dViewport);
HRESULT WINAPI glDirect3DDevice3_GetDirect3D(glDirect3DDevice3 *This, LPDIRECT3D3 *lplpD3D);
HRESULT WINAPI glDirect3DDevice3_GetLightState(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
HRESULT WINAPI glDirect3DDevice3_GetRenderState(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
HRESULT WINAPI glDirect3DDevice3_GetRenderTarget(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 *lplpRenderTarget);
HRESULT WINAPI glDirect3DDevice3_GetStats(glDirect3DDevice3 *This, LPD3DSTATS lpD3DStats);
HRESULT WINAPI glDirect3DDevice3_GetTexture(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 * lplpTexture);
HRESULT WINAPI glDirect3DDevice3_GetTextureStageState(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
HRESULT WINAPI glDirect3DDevice3_GetTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice3_Index(glDirect3DDevice3 *This, WORD wVertexIndex);
HRESULT WINAPI glDirect3DDevice3_MultiplyTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice3_NextViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpDirect3DViewport, LPDIRECT3DVIEWPORT3 *lplpAnotherViewport, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_SetClipStatus(glDirect3DDevice3 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice3_SetCurrentViewport(glDirect3DDevice3 *This, LPDIRECT3DVIEWPORT3 lpd3dViewport);
HRESULT WINAPI glDirect3DDevice3_SetLightState(glDirect3DDevice3 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
HRESULT WINAPI glDirect3DDevice3_SetRenderState(glDirect3DDevice3 *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
HRESULT WINAPI glDirect3DDevice3_SetRenderTarget(glDirect3DDevice3 *This, LPDIRECTDRAWSURFACE4 lpNewRenderTarget, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice3_SetTexture(glDirect3DDevice3 *This, DWORD dwStage, LPDIRECT3DTEXTURE2 lpTexture);
HRESULT WINAPI glDirect3DDevice3_SetTextureStageState(glDirect3DDevice3 *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
HRESULT WINAPI glDirect3DDevice3_SetTransform(glDirect3DDevice3 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice3_ValidateDevice(glDirect3DDevice3 *This, LPDWORD lpdwPasses);
HRESULT WINAPI glDirect3DDevice3_Vertex(glDirect3DDevice3 *This, LPVOID lpVertex);

struct glDirect3DDevice2Vtbl;

// Structure for glDirect3DDevice2, emulates IDirect3DDevice2
typedef struct glDirect3DDevice2
{
	struct glDirect3DDevice2Vtbl *lpVtbl;

	glDirect3DDevice7 *glD3DDev7;
} glDirect3DDevice2;

// Function pointer table for glDirect3DDevice2
typedef struct glDirect3DDevice2Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DDevice2 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI  *AddRef)(glDirect3DDevice2 *This);
	ULONG(WINAPI * Release)(glDirect3DDevice2  *This);

	HRESULT(WINAPI *GetCaps)(glDirect3DDevice2  *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT(WINAPI *SwapTextureHandles)(glDirect3DDevice2  *This, LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2);
	HRESULT(WINAPI *GetStats)(glDirect3DDevice2  *This, LPD3DSTATS lpD3DStats);
	HRESULT(WINAPI *AddViewport)(glDirect3DDevice2  *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
	HRESULT(WINAPI *DeleteViewport)(glDirect3DDevice2  *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
	HRESULT(WINAPI *NextViewport)(glDirect3DDevice2  *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags);
	HRESULT(WINAPI *EnumTextureFormats)(glDirect3DDevice2  *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
	HRESULT(WINAPI *BeginScene)(glDirect3DDevice2  *This);
	HRESULT(WINAPI *EndScene)(glDirect3DDevice2  *This);
	HRESULT(WINAPI *GetDirect3D)(glDirect3DDevice2  *This, LPDIRECT3D2 *lplpD3D2);
	HRESULT(WINAPI *SetCurrentViewport)(glDirect3DDevice2  *This, LPDIRECT3DVIEWPORT2 lpd3dViewport2);
	HRESULT(WINAPI *GetCurrentViewport)(glDirect3DDevice2  *This, LPDIRECT3DVIEWPORT2 *lplpd3dViewport2);
	HRESULT(WINAPI *SetRenderTarget)(glDirect3DDevice2  *This, LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags);
	HRESULT(WINAPI *GetRenderTarget)(glDirect3DDevice2  *This, LPDIRECTDRAWSURFACE *lplpRenderTarget);
	HRESULT(WINAPI *Begin)(glDirect3DDevice2  *This, D3DPRIMITIVETYPE d3dpt, D3DVERTEXTYPE d3dvt, DWORD dwFlags);
	HRESULT(WINAPI *BeginIndexed)(glDirect3DDevice2  *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
	HRESULT(WINAPI *Vertex)(glDirect3DDevice2  *This, LPVOID lpVertexType);
	HRESULT(WINAPI *Index)(glDirect3DDevice2  *This, WORD wVertexIndex);
	HRESULT(WINAPI *End)(glDirect3DDevice2  *This, DWORD dwFlags);
	HRESULT(WINAPI *GetRenderState)(glDirect3DDevice2  *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
	HRESULT(WINAPI *SetRenderState)(glDirect3DDevice2  *This, D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState);
	HRESULT(WINAPI *GetLightState)(glDirect3DDevice2  *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
	HRESULT(WINAPI *SetLightState)(glDirect3DDevice2  *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
	HRESULT(WINAPI *SetTransform)(glDirect3DDevice2  *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *GetTransform)(glDirect3DDevice2  *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *MultiplyTransform)(glDirect3DDevice2  *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *DrawPrimitive)(glDirect3DDevice2  *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
		DWORD dwVertexCount, DWORD dwFlags);
	HRESULT(WINAPI *DrawIndexedPrimitive)(glDirect3DDevice2  *This, D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
		LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags);
	HRESULT(WINAPI *SetClipStatus)(glDirect3DDevice2  *This, LPD3DCLIPSTATUS lpD3DClipStatus);
	HRESULT(WINAPI *GetClipStatus)(glDirect3DDevice2  *This, LPD3DCLIPSTATUS lpD3DClipStatus);
} glDirect3DDevice2Vtbl;

HRESULT glDirect3DDevice2_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice2 **newdev);

HRESULT WINAPI glDirect3DDevice2_QueryInterface(glDirect3DDevice2 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DDevice2_AddRef(glDirect3DDevice2 *This);
ULONG WINAPI glDirect3DDevice2_Release(glDirect3DDevice2 *This);
HRESULT WINAPI glDirect3DDevice2_AddViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
HRESULT WINAPI glDirect3DDevice2_Begin(glDirect3DDevice2 *This, D3DPRIMITIVETYPE d3dpt, D3DVERTEXTYPE d3dvt, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_BeginIndexed(glDirect3DDevice2 *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices, DWORD dwNumVertices, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_BeginScene(glDirect3DDevice2 *This);
HRESULT WINAPI glDirect3DDevice2_DeleteViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2);
HRESULT WINAPI glDirect3DDevice2_DrawIndexedPrimitive(glDirect3DDevice2 *This, D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
	LPVOID lpvVertices, DWORD dwVertexCount, LPWORD dwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_DrawPrimitive(glDirect3DDevice2 *This, D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dvtVertexType, LPVOID lpvVertices,
	DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_End(glDirect3DDevice2 *This, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_EndScene(glDirect3DDevice2 *This);
HRESULT WINAPI glDirect3DDevice2_EnumTextureFormats(glDirect3DDevice2 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
HRESULT WINAPI glDirect3DDevice2_GetCaps(glDirect3DDevice2 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
HRESULT WINAPI glDirect3DDevice2_GetClipStatus(glDirect3DDevice2 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice2_GetCurrentViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 *lplpd3dViewport2);
HRESULT WINAPI glDirect3DDevice2_GetDirect3D(glDirect3DDevice2 *This, LPDIRECT3D2 *lplpD3D2);
HRESULT WINAPI glDirect3DDevice2_GetLightState(glDirect3DDevice2 *This, D3DLIGHTSTATETYPE dwLightStateType, LPDWORD lpdwLightState);
HRESULT WINAPI glDirect3DDevice2_GetRenderState(glDirect3DDevice2 *This, D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
HRESULT WINAPI glDirect3DDevice2_GetRenderTarget(glDirect3DDevice2 *This, LPDIRECTDRAWSURFACE *lplpRenderTarget);
HRESULT WINAPI glDirect3DDevice2_GetStats(glDirect3DDevice2 *This, LPD3DSTATS lpD3DStats);
HRESULT WINAPI glDirect3DDevice2_GetTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice2_Index(glDirect3DDevice2 *This, WORD wVertexIndex);
HRESULT WINAPI glDirect3DDevice2_MultiplyTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice2_NextViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpDirect3DViewport2, LPDIRECT3DVIEWPORT2 *lplpDirect3DViewport2, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_SetClipStatus(glDirect3DDevice2 *This, LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice2_SetCurrentViewport(glDirect3DDevice2 *This, LPDIRECT3DVIEWPORT2 lpd3dViewport2);
HRESULT WINAPI glDirect3DDevice2_SetLightState(glDirect3DDevice2 *This, D3DLIGHTSTATETYPE dwLightStateType, DWORD dwLightState);
HRESULT WINAPI glDirect3DDevice2_SetRenderState(glDirect3DDevice2 *This, D3DRENDERSTATETYPE dwRenderStateType, DWORD dwRenderState);
HRESULT WINAPI glDirect3DDevice2_SetRenderTarget(glDirect3DDevice2 *This, LPDIRECTDRAWSURFACE lpNewRenderTarget, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice2_SetTransform(glDirect3DDevice2 *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice2_SwapTextureHandles(glDirect3DDevice2 *This, LPDIRECT3DTEXTURE2 lpD3DTex1, LPDIRECT3DTEXTURE2 lpD3DTex2);
HRESULT WINAPI glDirect3DDevice2_Vertex(glDirect3DDevice2 *This, LPVOID lpVertexType);

struct glDirect3DDevice1Vtbl;

// Structure for glDirect3DDevice1, emulates IDirect3DDevice
typedef struct glDirect3DDevice1
{
	struct glDirect3DDevice1Vtbl *lpVtbl;

	glDirect3DDevice7 *glD3DDev7;
} glDirect3DDevice1;

// Function pointer table for glDirect3DDevice1
typedef struct glDirect3DDevice1Vtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirect3DDevice1 *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI  *AddRef)(glDirect3DDevice1 *This);
	ULONG(WINAPI * Release)(glDirect3DDevice1 *This);

	HRESULT(WINAPI *Initialize)(glDirect3DDevice1 *This, LPDIRECT3D lpd3d, LPGUID lpGUID, LPD3DDEVICEDESC lpd3ddvdesc);
	HRESULT(WINAPI *GetCaps)(glDirect3DDevice1 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
	HRESULT(WINAPI *SwapTextureHandles)(glDirect3DDevice1 *This, LPDIRECT3DTEXTURE lpD3DTex1, LPDIRECT3DTEXTURE lpD3DTex2);
	HRESULT(WINAPI *CreateExecuteBuffer)(glDirect3DDevice1 *This, LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
		IUnknown* pUnkOuter);
	HRESULT(WINAPI *GetStats)(glDirect3DDevice1 *This, LPD3DSTATS lpD3DStats);
	HRESULT(WINAPI *Execute)(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags);
	HRESULT(WINAPI *AddViewport)(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport);
	HRESULT(WINAPI *DeleteViewport)(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport);
	HRESULT(WINAPI *NextViewport)(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport, LPDIRECT3DVIEWPORT* lplpDirect3DViewport, DWORD dwFlags);
	HRESULT(WINAPI *Pick)(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags,
		LPD3DRECT lpRect);
	HRESULT(WINAPI *GetPickRecords)(glDirect3DDevice1 *This, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec);
	HRESULT(WINAPI *EnumTextureFormats)(glDirect3DDevice1 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
	HRESULT(WINAPI *CreateMatrix)(glDirect3DDevice1 *This, LPD3DMATRIXHANDLE lpD3DMatHandle);
	HRESULT(WINAPI *SetMatrix)(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *GetMatrix)(glDirect3DDevice1 *This, D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix);
	HRESULT(WINAPI *DeleteMatrix)(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle);
	HRESULT(WINAPI *BeginScene)(glDirect3DDevice1 *This);
	HRESULT(WINAPI *EndScene)(glDirect3DDevice1 *This);
	HRESULT(WINAPI *GetDirect3D)(glDirect3DDevice1 *This, LPDIRECT3D *lpD3D);
} glDirect3DDevice1Vtbl;

struct glDirect3D1Vtbl;

HRESULT glDirect3DDevice1_Create(glDirect3DDevice7 *glD3DDev7, glDirect3DDevice1 **newdev);

HRESULT WINAPI glDirect3DDevice1_QueryInterface(glDirect3DDevice1 *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirect3DDevice1_AddRef(glDirect3DDevice1 *This);
ULONG WINAPI glDirect3DDevice1_Release(glDirect3DDevice1 *This);
HRESULT WINAPI glDirect3DDevice1_AddViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport);
HRESULT WINAPI glDirect3DDevice1_BeginScene(glDirect3DDevice1 *This);
HRESULT WINAPI glDirect3DDevice1_CreateExecuteBuffer(glDirect3DDevice1 *This, LPD3DEXECUTEBUFFERDESC lpDesc, LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
	IUnknown* pUnkOuter);
HRESULT WINAPI glDirect3DDevice1_CreateMatrix(glDirect3DDevice1 *This, LPD3DMATRIXHANDLE lpD3DMatHandle);
HRESULT WINAPI glDirect3DDevice1_DeleteMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle);
HRESULT WINAPI glDirect3DDevice1_DeleteViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport);
HRESULT WINAPI glDirect3DDevice1_EndScene(glDirect3DDevice1 *This);
HRESULT WINAPI glDirect3DDevice1_EnumTextureFormats(glDirect3DDevice1 *This, LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc, LPVOID lpArg);
HRESULT WINAPI glDirect3DDevice1_Execute(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice1_GetCaps(glDirect3DDevice1 *This, LPD3DDEVICEDESC lpD3DHWDevDesc, LPD3DDEVICEDESC lpD3DHELDevDesc);
HRESULT WINAPI glDirect3DDevice1_GetDirect3D(glDirect3DDevice1 *This, LPDIRECT3D* lpD3D);
HRESULT WINAPI glDirect3DDevice1_GetMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE lpD3DMatHandle, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice1_GetPickRecords(glDirect3DDevice1 *This, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec);
HRESULT WINAPI glDirect3DDevice1_GetStats(glDirect3DDevice1 *This, LPD3DSTATS lpD3DStats);
HRESULT WINAPI glDirect3DDevice1_Initialize(glDirect3DDevice1 *This, LPDIRECT3D lpd3d, LPGUID lpGUID, LPD3DDEVICEDESC lpd3ddvdesc);
HRESULT WINAPI glDirect3DDevice1_NextViewport(glDirect3DDevice1 *This, LPDIRECT3DVIEWPORT lpDirect3DViewport, LPDIRECT3DVIEWPORT* lplpDirect3DViewport, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice1_Pick(glDirect3DDevice1 *This, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags,
	LPD3DRECT lpRect);
HRESULT WINAPI glDirect3DDevice1_SetMatrix(glDirect3DDevice1 *This, D3DMATRIXHANDLE d3dMatHandle, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice1_SwapTextureHandles(glDirect3DDevice1 *This, LPDIRECT3DTEXTURE lpD3DTex1, LPDIRECT3DTEXTURE lpD3DTex2);


#endif //__GLDIRECT3DDEVICE_H
