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

#include "common.h"
#include "glTexture.h"
#include "glUtil.h"
#include "dxglDirectDrawSurface.h"
#include "glDirectDrawGammaControl.h"
#include "ddraw.h"

extern "C" {

glDirectDrawGammaControlVtbl glDirectDrawGammaControl_iface =
{
	glDirectDrawGammaControl_QueryInterface,
	glDirectDrawGammaControl_AddRef,
	glDirectDrawGammaControl_Release,
	glDirectDrawGammaControl_GetGammaRamp,
	glDirectDrawGammaControl_SetGammaRamp
};

HRESULT glDirectDrawGammaControl_Create(dxglDirectDrawSurface7 *glDDS7, glDirectDrawGammaControl *gamma)
{
	TRACE_ENTER(2, 14, glDDS7, 14, gamma);
	if (!glDDS7) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	if (!gamma) TRACE_RET(HRESULT, 23, DDERR_INVALIDPARAMS);
	ZeroMemory(gamma, sizeof(glDirectDrawGammaControl));
	gamma->glDDS7 = glDDS7;
	gamma->lpVtbl = &glDirectDrawGammaControl_iface;
	TRACE_EXIT(23, DD_OK);
	return DD_OK;
}

HRESULT WINAPI glDirectDrawGammaControl_QueryInterface(glDirectDrawGammaControl *This, REFIID riid, void** ppvObj)
{
	TRACE_ENTER(3, 14, This, 24, &riid, 14, ppvObj);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	if (riid == IID_IUnknown)
	{
		glDirectDrawGammaControl_AddRef(This);
		*ppvObj = This;
		TRACE_VAR("*ppvObj", 14, *ppvObj);
		TRACE_EXIT(23, DD_OK);
		return DD_OK;
	}
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_QueryInterface(This->glDDS7, riid, ppvObj));
}
ULONG WINAPI glDirectDrawGammaControl_AddRef(glDirectDrawGammaControl *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_AddRefGamma(This->glDDS7));
}
ULONG WINAPI glDirectDrawGammaControl_Release(glDirectDrawGammaControl *This)
{
	TRACE_ENTER(1, 14, This);
	if (!This) TRACE_RET(ULONG, 8, 0);
	TRACE_RET(ULONG, 8, dxglDirectDrawSurface7_ReleaseGamma(This->glDDS7));
}
HRESULT WINAPI glDirectDrawGammaControl_GetGammaRamp(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_GetGammaRamp(This->glDDS7, dwFlags, lpRampData));
}
HRESULT WINAPI glDirectDrawGammaControl_SetGammaRamp(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData)
{
	TRACE_ENTER(3, 14, This, 9, dwFlags, 14, lpRampData);
	if (!This) TRACE_RET(HRESULT, 23, DDERR_INVALIDOBJECT);
	TRACE_RET(HRESULT, 23, dxglDirectDrawSurface7_SetGammaRamp(This->glDDS7, dwFlags, lpRampData));
}

}