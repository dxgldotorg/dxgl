// DXGL
// Copyright (C) 2022 William Feely

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
#ifndef _DXGLRENDERER_H
#define _DXGLRENDERER_H

#define COM_NO_WINDOWS_H
#include <objbase.h>

#ifdef __cplusplus
extern "C" {
#endif

// GUID defines
DEFINE_GUID(CLSID_DXGLRenderer, 0x671445ff, 0xe857, 0x4d4f, 0x84, 0xf3, 0xe2, 0xbe, 0x8, 0xda, 0x8f, 0x8c);
DEFINE_GUID(CLSID_DXGLRendererGL, 0x67144500, 0xe857, 0x4d4f, 0x84, 0xf3, 0xe2, 0xbe, 0x8, 0xda, 0x8f, 0x8c);


// struct defines
typedef struct IDXGLRenderer *LPDXGLRENDERER;

// Interfaces
#undef INTERFACE
#define INTERFACE IDXGLRenderer
DECLARE_INTERFACE_(IDXGLRenderer, IUnknown)
{
    // IUnknwon
    STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    // IDXGLInterface
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND hWnd, DWORD flags);
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
// IUnknown
#define IDXGLRenderer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDXGLRenderer_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDXGLRenderer_Release(p)            (p)->lpVtbl->Release(p)
// IDXGLRenderer
#define IDXGLRenderer_SetCooperativeLevel(p,a,b)         (p)->lpVtbl->Initialize(p,a,b)
#else
// IUnknown
#define IDXGLRenderer_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IDXGLRenderer_AddRef(p)             (p)->AddRef()
#define IDXGLRenderer_Release(p)            (p)->Release()
// IDXGLRenderer
#define IDXGLRenderer_SetCooperativeLevel(p,a,b)         (p)->Initialize(p,a,b)
#endif


// Functions
HRESULT CreateDXGLRenderer(GUID *guid, LPDXGLRENDERER *out);


#ifdef __cplusplus
}
#endif


#endif //_DXGLRENDERER_H
