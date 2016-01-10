// DXGL
// Copyright (C) 2011-2014 William Feely

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
#ifndef _GLDIRECTDRAWPALETTE_H
#define _GLDIRECTDRAWPALETTE_H

#ifdef __cplusplus
extern "C" {
#endif

struct glDirectDrawPaletteVtbl;

typedef struct glDirectDrawPalette
{
	struct glDirectDrawPaletteVtbl *lpVtbl;

	ULONG refcount;
	PALETTEENTRY palette[256];
	DWORD flags;
	IUnknown *creator;
} glDirectDrawPalette;

typedef struct glDirectDrawPaletteVtbl
{
	// DirectDraw (all versions) API
	HRESULT(WINAPI *QueryInterface)(glDirectDrawPalette *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDrawPalette *This);
	ULONG(WINAPI *Release)(glDirectDrawPalette *This);

	HRESULT(WINAPI *GetCaps)(glDirectDrawPalette *This, LPDWORD lpdwCaps);
	HRESULT(WINAPI *GetEntries)(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries);
	HRESULT(WINAPI *Initialize)(glDirectDrawPalette *This, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable);
	HRESULT(WINAPI *SetEntries)(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries);
} glDirectDrawPaletteVtbl;

HRESULT glDirectDrawPalette_Create(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
LPPALETTEENTRY glDirectDrawPalette_GetPalette(glDirectDrawPalette *This, DWORD *flags);

HRESULT WINAPI glDirectDrawPalette_QueryInterface(glDirectDrawPalette *This, REFIID riid, void** ppvObj);
ULONG WINAPI glDirectDrawPalette_AddRef(glDirectDrawPalette *This);
ULONG WINAPI glDirectDrawPalette_Release(glDirectDrawPalette *This);
HRESULT WINAPI glDirectDrawPalette_GetCaps(glDirectDrawPalette *This, LPDWORD lpdwCaps);
HRESULT WINAPI glDirectDrawPalette_GetEntries(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries);
HRESULT WINAPI glDirectDrawPalette_Initialize(glDirectDrawPalette *This, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable);
HRESULT WINAPI glDirectDrawPalette_SetEntries(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries);
LPPALETTEENTRY glDirectDrawPalette_GetPalette(glDirectDrawPalette *This, DWORD *flags);



#ifdef __cplusplus
}
#endif

#endif //_GLDIRECTDRAWPALETTE_H
