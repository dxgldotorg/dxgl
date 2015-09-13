// DXGL
// Copyright (C) 2014-2015 William Feely

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
#ifndef _GLDIRECTDRAWGAMMACONTROL_H
#define _GLDIRECTDRAWGAMMACONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

struct glDirectDrawSurface7;
struct glDirectDrawGammaControlVtbl;

typedef struct glDirectDrawGammaControl
{
	glDirectDrawGammaControlVtbl *lpVtbl;
	glDirectDrawSurface7 *glDDS7;
} glDirectDrawGammaControl;

typedef struct glDirectDrawGammaControlVtbl
{
	HRESULT(WINAPI *QueryInterface)(glDirectDrawGammaControl *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDrawGammaControl *This);
	ULONG(WINAPI *Release)(glDirectDrawGammaControl *This);
	HRESULT(WINAPI *GetGammaRamp)(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);
	HRESULT(WINAPI *SetGammaRamp)(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);
} glDirectDrawGammaControlVtbl;

HRESULT glDirectDrawGammaControl_Create(glDirectDrawSurface7 *glDDS7, LPDIRECTDRAWGAMMACONTROL *gamma);

HRESULT WINAPI glDirectDrawGammaControl_QueryInterface(glDirectDrawGammaControl *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDrawGammaControl_AddRef(glDirectDrawGammaControl *This);
ULONG WINAPI glDirectDrawGammaControl_Release(glDirectDrawGammaControl *This);
HRESULT WINAPI glDirectDrawGammaControl_GetGammaRamp(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);
HRESULT WINAPI glDirectDrawGammaControl_SetGammaRamp(glDirectDrawGammaControl *This, DWORD dwFlags, LPDDGAMMARAMP lpRampData);


#ifdef __cplusplus
}
#endif

#endif //_GLDIRECTDRAWGAMMACONTROL_H
