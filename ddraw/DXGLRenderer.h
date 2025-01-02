// DXGL
// Copyright (C) 2023-2025 William Feely

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

// API defines
#define DXGLRENDERERAPI_OPENGL 0  // Default OpenGL

// Enum defines
#define DXGLRENDERMODE_STATE2D 1
#define DXGLRENDERMODE_STATE3D 2

// struct defines
typedef struct IDXGLRenderer *LPDXGLRENDERER;

// Structures
typedef struct DXGLRenderState
{
    INT_PTR mode;
    union
    {
        struct
        {
            DWORD flags;
            DDBLTFX bltfx;
            DDCOLORKEY srckey;
            DDCOLORKEY destkey;
            GLint srclevel;
            GLint destlevel;
            GLint patternlevel;
            DDPIXELFORMAT srcformat;
            DDPIXELFORMAT destformat;
            DDPIXELFORMAT patternformat;
        } state2D;
        struct
        {
            D3DRENDERSTATETYPE index;
            DWORD statevalue;
        } state3D;
        int staticindex;
    } state;
} DXGLRenderState;

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
    STDMETHOD(GetAttachedDevice)(THIS_ struct glDirectDraw7 **glDD7);
    STDMETHOD(SetAttachedDevice)(THIS_ struct glDirectDraw7 *glDD7);
    STDMETHOD(GetCaps)(THIS_ DWORD index, void *output);
    STDMETHOD(Reset)(THIS);
    STDMETHOD(PostCommand)(THIS_ struct DXGLPostQueueCmd *cmd);
    STDMETHOD(Break)(THIS);
    STDMETHOD(FreePointer)(THIS_ void *);
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND hWnd, DWORD flags);
    STDMETHOD(CreateTexture)(THIS_ LPDDSURFACEDESC2 desc, DWORD bpp, DXGLTexture *out);
    STDMETHOD(DeleteTexture)(THIS_ DXGLTexture *texture);
    STDMETHOD(SetTexture)(THIS_ GLuint level, DXGLTexture *texture);
    STDMETHOD(SetTarget)(THIS_ DXGLTexture *texture, GLuint miplevel);
    STDMETHOD(Lock)(THIS_ DXGLTexture *texture, GLuint miplevel, BYTE **pointer);
    STDMETHOD(Unlock)(THIS_ DXGLTexture *texture, GLuint miplevel);
    STDMETHOD(Clear)(THIS_ D3DRECT * rect, DWORD flags, DWORD color, D3DVALUE z, DWORD stencil);
    STDMETHOD(SetRenderState)(THIS_ DXGLRenderState *state);
    STDMETHOD(SetFVF)(THIS_ DWORD fvf);
    STDMETHOD(DrawPrimitives)(THIS_ D3DPRIMITIVETYPE type, const BYTE *vertices,
        DWORD vertexcount, const WORD *indices, DWORD indexcount);
    STDMETHOD(Sync)(THIS_ void *pointer);
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
// IUnknown
#define IDXGLRenderer_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IDXGLRenderer_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IDXGLRenderer_Release(p)            (p)->lpVtbl->Release(p)
// IDXGLRenderer
#define IDXGLRenderer_GetAttachedDevice(p,a)             (p)->lpVtbl->GetAttachedDevice(p,a) 
#define IDXGLRenderer_SetAttachedDevice(p,a)             (p)->lpVtbl->SetAttachedDevice(p,a) 
#define IDXGLRenderer_GetCaps(p,a,b)                     (p)->lpVtbl->GetCaps(p,a,b)
#define IDXGLRenderer_Reset(p)                           (p)->lpVtbl->Reset(p)
#define IDXGLRenderer_PostCommand(p,a)                   (p)->lpVtbl->PostCommand(p,a)
#define IDXGLRenderer_Break(p)                           (p)->lpVtbl->Break(p)
#define IDXGLRenderer_FreePointer(p,a)                   (p)->lpVtbl->FreePointer(p,a)
#define IDXGLRenderer_SetCooperativeLevel(p,a,b)         (p)->lpVtbl->SetCooperativeLevel(p,a,b)
#define IDXGLRenderer_CreateTexture(p,a,b,c)             (p)->lpVtbl->CreateTexture(p,a,b,c)
#define IDXGLRenderer_DeleteTexture(p,a)                 (p)->lpVtbl->DeleteTexture(p,a)
#define IDXGLRenderer_SetTexture(p,a,b)                  (p)->lpVtbl->SetTexture(p,a,b)
#define IDXGLRenderer_SetTarget(p,a,b)                   (p)->lpVtbl->SetTarget(p,a,b)
#define IDXGLRenderer_Lock(p,a,b,c)                      (p)->lpVtbl->Lock(p,a,b,c)
#define IDXGLRenderer_Unlock(p,a,b)                      (p)->lpVtbl->Unlock(p,a,b)
#define IDXGLRenderer_Clear(p,a,b,c,d,e)                 (p)->lpVtbl->Clear(p,a,b,c,d,e)
#define IDXGLRenderer_SetRenderState(p,a)                (p)->lpVtbl->SetRenderState(p,a)
#define IDXGLRenderer_SetFVF(p,a)                        (p)->lpVtbl->SetFVF(p,a)
#define IDXGLRenderer_DrawPrimitives(p,a,b,c,d,e)        (p)->lpVtbl->DrawPrimitives(p,a,b,c,d,e)
#define IDXGLRenderer_Sync(p,a)                          (p)->lpVtbl->Sync(p,a);
#else
// IUnknown
#define IDXGLRenderer_QueryInterface(p,a,b) (p)->QueryInterface(a,b)
#define IDXGLRenderer_AddRef(p)             (p)->AddRef()
#define IDXGLRenderer_Release(p)            (p)->Release()
// IDXGLRenderer
#define IDXGLRenderer_GetAttachedDevice(p,a)             (p)->GetAttachedDevice(a) 
#define IDXGLRenderer_SetAttachedDevice(p,a)             (p)->SetAttachedDevice(a) 
#define IDXGLRenderer_GetCaps(p,a,b)                     (p)->GetCaps(a,b)
#define IDXGLRenderer_Reset(p)                           (p)->Reset()
#define IDXGLRenderer_PostCommand(p,a)                   (p)->PostCommand(a)
#define IDXGLRenderer_Break(p)                           (p)->Break()
#define IDXGLRenderer_FreePointer(p,a)                   (p)->FreePointer(a)
#define IDXGLRenderer_SetCooperativeLevel(p,a,b)         (p)->SetCooperativeLevel(a,b)
#define IDXGLRenderer_CreateTexture(p,a,b,c)             (p)->CreateTexture(a,b,c)
#define IDXGLRenderer_DeleteTexture(p,a)                 (p)->CreateTexture(a)
#define IDXGLRenderer_SetTexture(p,a,b)                  (p)->SetTexture(a,b)
#define IDXGLRenderer_SetTarget(p,a,b)                   (p)->SetTarget(a,b)
#define IDXGLRenderer_Lock(p,a,b,c)                      (p)->Lock(a,b,c)
#define IDXGLRenderer_Unlock(p,a,b)                      (p)->Unlock(a,b)
#define IDXGLRenderer_Clear(p,a,b,c,d,e)                 (p)->Clear(a,b,c,d,e)
#define IDXGLRenderer_SetRenderState(p,a)                (p)->SetRenderState(a)
#define IDXGLRenderer_SetFVF(p,a)                        (p)->SetFVF(a)
#define IDXGLRenderer_DrawPrimitives(p,a,b,c,d,e)        (p)->DrawPrimitives(a,b,c,d,e)
#define IDXGLRenderer_Sync(p,a)                          (p)->Sync(a);
#endif

// Constants


// Functions
HRESULT CreateDXGLRenderer(GUID *guid, LPDXGLRENDERER *out, int index);


#ifdef __cplusplus
}
#endif


#endif //_DXGLRENDERER_H
