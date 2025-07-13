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
#define DXGLRENDERMODE_STATICSHADER 0
#define DXGLRENDERMODE_STATE2D 1
#define DXGLRENDERMODE_STATE3D 2
#define DXGLRENDERMODE_POSTSHADER 3

// struct defines
typedef struct IDXGLRenderer *LPDXGLRENDERER;

typedef struct DXGLRenderState2D
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
    unsigned char blttypesrc, blttypedest;
} DXGLRenderState2D;


// Structures
typedef struct DXGLRenderState
{
    INT_PTR mode;
    union
    {
        DXGLRenderState2D state2D;
        struct
        {
            DWORD count;
            D3DRENDERSTATETYPE index;
            DWORD statevalue;
        } state3D;
        int staticindex;  // Reserved for pre-compiled shaders
    } state;
} DXGLRenderState;

// Interfaces
struct IDXGLRendererVtbl;
typedef struct IDXGLRenderer
{
    struct IDXGLRendererVtbl *lpVtbl;
} IDXGLRenderer;
typedef struct IDXGLRendererVtbl
{
    // IUnknwon
    HRESULT (WINAPI *QueryInterface)(IDXGLRenderer *This, REFIID riid, LPVOID* ppvObject);
    ULONG (WINAPI *AddRef)(IDXGLRenderer *This);
    ULONG (WINAPI *Release)(IDXGLRenderer *This);
    // IDXGLInterface
    HRESULT (WINAPI *GetAttachedDevice)(IDXGLRenderer *This, struct glDirectDraw7 **glDD7);
    HRESULT (WINAPI *SetAttachedDevice)(IDXGLRenderer *This, struct glDirectDraw7 *glDD7);
    HRESULT (WINAPI *GetCaps)(IDXGLRenderer *This, DWORD index, void *output);
    HRESULT (WINAPI *Reset)(IDXGLRenderer *This);
    HRESULT (WINAPI *PostCommand)(IDXGLRenderer *This, struct DXGLPostQueueCmd *cmd);
    HRESULT (WINAPI *Break)(IDXGLRenderer *This);
    HRESULT (WINAPI *FreePointer)(IDXGLRenderer *This, void *ptr);
    HRESULT (WINAPI *SetCooperativeLevel)(IDXGLRenderer *This, HWND hWnd, DWORD flags);
    HRESULT (WINAPI *CreateTexture)(IDXGLRenderer *This, LPDDSURFACEDESC2 desc, DWORD bpp, DXGLTexture *out);
    HRESULT (WINAPI *DeleteTexture)(IDXGLRenderer *This, DXGLTexture *texture);
    HRESULT (WINAPI *SetTexture)(IDXGLRenderer *This, GLuint level, DXGLTexture *texture);
    HRESULT (WINAPI *SetTarget)(IDXGLRenderer *This, DXGLTexture *texture, GLuint miplevel);
    HRESULT (WINAPI *Lock)(IDXGLRenderer *This, DXGLTexture *texture, GLuint miplevel, BYTE **pointer);
    HRESULT (WINAPI *Unlock)(IDXGLRenderer *This, DXGLTexture *texture, GLuint miplevel);
    HRESULT (WINAPI *Clear)(IDXGLRenderer *This, D3DRECT *rect, DWORD flags, DWORD color, D3DVALUE z, DWORD stencil);
    HRESULT (WINAPI *SetRenderState)(IDXGLRenderer *This, DXGLRenderState *state);
    HRESULT (WINAPI *SetFVF)(IDXGLRenderer *This, DWORD fvf);
    HRESULT (WINAPI *DrawPrimitives2D)(IDXGLRenderer *This, D3DPRIMITIVETYPE type, const BYTE *vertices,
        DWORD vertexcount);
    HRESULT (WINAPI *DrawPrimitives)(IDXGLRenderer *This, D3DPRIMITIVETYPE type, const BYTE *vertices,
        DWORD vertexcount, const WORD *indices, DWORD indexcount);
    HRESULT (WINAPI *SwapBuffers)(IDXGLRenderer *This, GLuint interval);
    HRESULT (WINAPI *Sync)(IDXGLRenderer *This, void *pointer);
    HRESULT (WINAPI *GetWindow)(IDXGLRenderer *This, HWND *hwnd);
    HRESULT (WINAPI *SetWindowSize)(IDXGLRenderer *This, RECT *r);
} IDXGLRendererVtbl;
#undef INTERFACE

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
#define IDXGLRenderer_DrawPrimitives2D(p,a,b,c)          (p)->lpVtbl->DrawPrimitives2D(p,a,b,c)
#define IDXGLRenderer_DrawPrimitives(p,a,b,c,d,e)        (p)->lpVtbl->DrawPrimitives(p,a,b,c,d,e)
#define IDXGLRenderer_SwapBuffers(p,a)                   (p)->lpVtbl->SwapBuffers(p,a)
#define IDXGLRenderer_Sync(p,a)                          (p)->lpVtbl->Sync(p,a);
#define IDXGLRenderer_GetWindow(p,a)                     (p)->lpVtbl->GetWindow(p,a);
#define IDXGLRenderer_SetWindowSize(p,a)                 (p)->lpVtbl->SetWindowSize(p,a);

// Constants


// Functions
HRESULT CreateDXGLRenderer(GUID *guid, LPDXGLRENDERER *out, int index);


#ifdef __cplusplus
}
#endif


#endif //_DXGLRENDERER_H
