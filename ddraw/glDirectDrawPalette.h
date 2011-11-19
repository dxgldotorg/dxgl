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
#pragma once
#ifndef _GLDIRECTDRAWPALETTE_H
#define _GLDIRECTDRAWPALETTE_H

class glDirectDrawPalette : public IDirectDrawPalette
{
public:
	glDirectDrawPalette(DWORD dwFlags, LPPALETTEENTRY lpDDColorArray, LPDIRECTDRAWPALETTE FAR *lplpDDPalette);
	virtual ~glDirectDrawPalette();
	// DirectDraw (all versions) API
	HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObj);
	ULONG WINAPI AddRef();
	ULONG WINAPI Release();

	HRESULT WINAPI GetCaps(LPDWORD lpdwCaps);
	HRESULT WINAPI GetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries);
	HRESULT WINAPI Initialize(LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable);
	HRESULT WINAPI SetEntries(DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries);

	// Internal function
	LPPALETTEENTRY GetPalette(DWORD *flags);
private:
	PALETTEENTRY palette[256];
	DWORD flags;
	ULONG refcount;
};

#endif //_GLDIRECTDRAWPALETTE_H