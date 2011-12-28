// DXGL
// Copyright (C) 2011 William Feely

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

/*
glDirect3DDevice7::glDirect3DDevice7(glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7)
{
	refcount = 1;
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	this->glDDS7 = glDDS7;
	glDDS7->AddRef();
}
glDirect3DDevice7::~glDirect3DDevice7()
{
	glD3D7->Release();
	glDDS7->Release();
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
	FIXME("glDirect3DDevice7::BeginScene: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::BeginStateBlock()
{
	FIXME("glDirect3DDevice7::BeginStateBlock: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::CaptureStateBlock(DWORD dwBlockHandle)
{
	FIXME("glDirectDraw::CaptureStateBlock: stub");
	ERR(DDERR_GENERIC);
}*/
/*
HRESULT WINAPI glDirect3DDevice7::CreateStateBlock(D3DSTATEBLOCKTYPE d3dsbtype, LPDWORD lpdwBlockHandle);
HRESULT WINAPI glDirect3DDevice7::Clear(DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
HRESULT WINAPI glDirect3DDevice7::ComputeSphereVisibility(LPD3DVECTOR lpCenters, LPD3DVALUE lpRadii, DWORD dwNumSpheres, DWORD dwFlags, LPDWORD lpdwReturnValues);
HRESULT WINAPI glDirect3DDevice7::DeleteStateBlock(DWORD dwBlockHandle);
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitive(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpvVertices, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveStrided(D3DPRIMITIVETYPE d3dptPrimitiveType, DWORD dwVertexTypeDesc, LPD3DDRAWPRIMITIVESTRIDEDDATA lpvVerticexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::DrawIndexedPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer, DWORD dwStartVertex, DWORD dwNumVertices, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::DrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPVOID lpVertices, DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveStrided(D3DPRIMITIVETYPE dptPrimitiveType, DWORD dwVertexTypeDesc, LPD3DDRAWPRIMITIVESTRIDEDDATA lpVertexArray, DWORD dwVertexCount, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::DrawPrimitiveVB(D3DPRIMITIVETYPE d3dptPrimitiveType, LPDIRECT3DVERTEXBUFFER7 lpd3dVertexBuffer, DWORD dwStartVertex, DWORD dwNumVertices, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::EndScene();
HRESULT WINAPI glDirect3DDevice7::EndStateBlock(LPDWORD lpdwBlockHandle);
HRESULT WINAPI glDirect3DDevice7::EnumTextureFormats(LPD3DENUMPIXELFORMATSCALLBACK lpd3dEnumPixelProc, LPVOID lpArg);
HRESULT WINAPI glDirect3DDevice7::GetCaps(LPD3DDEVICEDESC7 lpD3DDevDesc);
HRESULT WINAPI glDirect3DDevice7::GetClipPlane(DWORD dwIndex, D3DVALUE *pPlaneEquation);
HRESULT WINAPI glDirect3DDevice7::GetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice7::GetDirect3D(LPDIRECT3D7 *lplpD3D);
HRESULT WINAPI glDirect3DDevice7::GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct, DWORD dwSize);
HRESULT WINAPI glDirect3DDevice7::GetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
HRESULT WINAPI glDirect3DDevice7::GetLightEnable(DWORD dwLightIndex, BOOL* pbEnable);
HRESULT WINAPI glDirect3DDevice7::GetMaterial(LPD3DMATERIAL7 lpMaterial);
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState);
HRESULT WINAPI glDirect3DDevice7::GetRenderTarget(LPDIRECTDRAWSURFACE7 lplpRenderTarget);
HRESULT WINAPI glDirect3DDevice7::GetStateData(DWORD dwState, LPVOID* lplpStateData);
HRESULT WINAPI glDirect3DDevice7::GetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 *lplpTexture);
HRESULT WINAPI glDirect3DDevice7::GetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, LPDWORD lpdwValue);
HRESULT WINAPI glDirect3DDevice7::GetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7::GetViewport(LPD3DVIEWPORT7 lpViewport);
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable);
HRESULT WINAPI glDirect3DDevice7::Load(LPDIRECTDRAWSURFACE7 lpDestTex, LPPOINT lpDestPoint, LPDIRECTDRAWSURFACE7 lpSrcTex, LPRECT lprcSrcRect, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::MultiplyTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7::PreLoad(LPDIRECTDRAWSURFACE7 lpddsTexture);
HRESULT WINAPI glDirect3DDevice7::SetClipPlane(DWORD dwIndex, D3DVALUE* pPlaneEquation);
HRESULT WINAPI glDirect3DDevice7::SetClipStatus(LPD3DCLIPSTATUS lpD3DClipStatus);
HRESULT WINAPI glDirect3DDevice7::SetLight(DWORD dwLightIndex, LPD3DLIGHT7 lpLight);
HRESULT WINAPI glDirect3DDevice7::SetMaterial(LPD3DMATERIAL7 lpMaterial);
HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
HRESULT WINAPI glDirect3DDevice7::SetRenderTarget(LPDIRECTDRAWSURFACE7 lpNewRenderTarget, DWORD dwFlags);
HRESULT WINAPI glDirect3DDevice7::SetStateData(DWORD dwState, LPVOID lpStateData);
HRESULT WINAPI glDirect3DDevice7::SetTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 lpTexture);
HRESULT WINAPI glDirect3DDevice7::SetTextureStageState(DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
HRESULT WINAPI glDirect3DDevice7::SetTransform(D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
HRESULT WINAPI glDirect3DDevice7::SetViewport(LPD3DVIEWPORT7 lpViewport);
HRESULT WINAPI glDirect3DDevice7::ValidateDevice(LPDWORD lpdwPasses);
*/