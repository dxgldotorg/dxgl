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


glDirect3DDevice7::glDirect3DDevice7(glDirect3D7 *glD3D7, glDirectDrawSurface7 *glDDS7)
{
	refcount = 1;
	this->glD3D7 = glD3D7;
	glD3D7->AddRef();
	this->glDDS7 = glDDS7;
	glDDS7->AddRef();
	ZeroMemory(&viewport,sizeof(D3DVIEWPORT7));
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
	FIXME("glDirect3DDevice7::Clear: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::EndScene: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::GetMaterial: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetRenderState(D3DRENDERSTATETYPE dwRenderStateType, LPDWORD lpdwRenderState)
{
	FIXME("glDirect3DDevice7::GetRenderState: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::GetTransform: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::GetViewport(LPD3DVIEWPORT7 lpViewport)
{
	memcpy(lpViewport,&viewport,sizeof(D3DVIEWPORT7));
	return D3D_OK;
}
HRESULT WINAPI glDirect3DDevice7::LightEnable(DWORD dwLightIndex, BOOL bEnable)
{
	FIXME("glDirect3DDevice7::LightEnable: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::SetLight: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetMaterial(LPD3DMATERIAL7 lpMaterial)
{
	FIXME("glDirect3DDevice7::SetMaterial: stub");
	ERR(DDERR_GENERIC);
}
HRESULT WINAPI glDirect3DDevice7::SetRenderState(D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	FIXME("glDirect3DDevice7::SetRenderState: stub");
	ERR(DDERR_GENERIC);
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
	FIXME("glDirect3DDevice7::SetTransform: stub");
	ERR(DDERR_GENERIC);
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