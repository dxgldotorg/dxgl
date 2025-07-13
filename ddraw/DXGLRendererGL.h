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

#ifndef _DXGLRENDERERGL_H
#define _DXGLRENDERERGL_H

#ifdef __cplusplus
extern "C" {
#endif

// struct defines
typedef struct IDXGLRendererGL *LPDXGLRENDERERGL;

typedef struct IDXGLRendererGL
{
	// COM interface
	IDXGLRenderer base;
	// Reference count
	ULONG refcount;

	// Attached DirectDraw device
	struct glDirectDraw7 *glDD7;

	// DirectDraw caps
	DDCAPS_DX7 ddcaps;

	// Direct3D caps
	D3DDEVICEDESC d3ddesc6;
	D3DDEVICEDESC7 d3ddesc7;

	// UI resources
	HANDLE ThreadHandle;
	DWORD ThreadID;
	HANDLE WindowThreadHandle;
	DWORD WindowThreadID;
	BOOL WindowDead;
	HWND hWndContext;
	HWND hWndTarget;
	LONG_PTR winstyle, winstyleex;
	HDC hdc;

	// Sync objects
	HANDLE SyncEvent;
	HANDLE StartEvent;
	CRITICAL_SECTION cs;
	DWORD_PTR syncptr;
	BOOL waitsync;
	BOOL running;
	BOOL shutdown;
	DWORD synclock;

	// OpenGL context resources
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	HGLRC hrc;
	glExtensions ext;
	glUtil util;

	// Window state
	GLsizei width;
	GLsizei height;
	BOOL fullscreen;

	// Display mode
	DWORD vidwidth, vidheight, vidbpp, vidrefresh;
	DWORD restorewidth, restoreheight, restorebpp, restorerefresh;
	DWORD cooplevel;

	// Command Queue
	DXGLQueue commandqueue[2];
	DXGLQueue *currentqueue;
	int queueindexread, queueindexwrite;

	// OpenGL state
	GLboolean depthwrite;
	GLboolean depthtest;
	GLenum depthcomp;
	GLenum alphacomp;
	GLboolean scissor;
	GLint scissorx, scissory;
	GLsizei scissorwidth, scissorheight;
	GLfloat clearr, clearg, clearb, cleara;
	GLdouble cleardepth;
	GLint clearstencil;
	GLenum blendsrc;
	GLenum blenddest;
	GLboolean blendenabled;
	GLenum polymode;
	GLenum shademode;
	GLenum texlevel;
	ShaderManager shaders;

	// Render state
	int rendermode;  // Last used render mode
	int staticshader;  // Last used static shader
	DXGLRenderState2D renderstate2D;  // Last used 2D render state
	unsigned __int64 shader2d;  // Last used 2D shader
	DWORD fvf;  // Currently used FVF
	FBO *target;
	DXGLTexture *textures[32]; // Currently set textures

} IDXGLRendererGL;

// Device creation
HRESULT DXGLRendererGL_Create(GUID *guid, LPDXGLRENDERERGL *out, int index);

// Implementation
HRESULT WINAPI DXGLRendererGL_QueryInterface(LPDXGLRENDERERGL This, REFIID riid, LPVOID *ppvObject);
ULONG WINAPI DXGLRendererGL_AddRef(LPDXGLRENDERERGL This);
ULONG WINAPI DXGLRendererGL_Release(LPDXGLRENDERERGL This);
HRESULT WINAPI DXGLRendererGL_GetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 **glDD7);
HRESULT WINAPI DXGLRendererGL_SetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 *glDD7);
HRESULT WINAPI DXGLRendererGL_GetCaps(LPDXGLRENDERERGL This, DWORD index, void *output);
HRESULT WINAPI DXGLRendererGL_Reset(LPDXGLRENDERERGL This);
HRESULT WINAPI DXGLRendererGL_PostCommand(LPDXGLRENDERERGL This, struct DXGLPostQueueCmd *cmd);
HRESULT WINAPI DXGLRendererGL_Break(LPDXGLRENDERERGL This);
HRESULT WINAPI DXGLRendererGL_FreePointer(LPDXGLRENDERERGL This, void *ptr);
HRESULT WINAPI DXGLRendererGL_SetCooperativeLevel(LPDXGLRENDERERGL This, HWND hWnd, DWORD flags);
HRESULT WINAPI DXGLRendererGL_CreateTexture(LPDXGLRENDERERGL This, LPDDSURFACEDESC2 desc, DWORD bpp, DXGLTexture *out);
HRESULT WINAPI DXGLRendererGL_DeleteTexture(LPDXGLRENDERERGL This, DXGLTexture *texture);
HRESULT WINAPI DXGLRendererGL_SetTexture(LPDXGLRENDERERGL This, GLuint level, DXGLTexture *texture);
HRESULT WINAPI DXGLRendererGL_SetTarget(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel);
HRESULT WINAPI DXGLRendererGL_Lock(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel, BYTE **pointer);
HRESULT WINAPI DXGLRendererGL_Unlock(LPDXGLRENDERERGL This, DXGLTexture *texture, GLuint miplevel);
HRESULT WINAPI DXGLRendererGL_Clear(LPDXGLRENDERERGL This, DWORD count, D3DRECT *rects, DWORD flags, DWORD color, D3DVALUE z, DWORD stencil);
HRESULT WINAPI DXGLRendererGL_SetRenderState(LPDXGLRENDERERGL This, DXGLRenderState *state);
HRESULT WINAPI DXGLRendererGL_SetFVF(LPDXGLRENDERERGL This, DWORD fvf);
HRESULT WINAPI DXGLRendererGL_DrawPrimitives2D(LPDXGLRENDERERGL This, D3DPRIMITIVETYPE type, const BYTE* vertices, DWORD vertexcount);
HRESULT WINAPI DXGLRendererGL_DrawPrimitives(LPDXGLRENDERERGL This, D3DPRIMITIVETYPE type, const BYTE* vertices, DWORD vertexcount, const WORD* indices, DWORD indexcount);
HRESULT WINAPI DXGLRendererGL_SwapBuffers(LPDXGLRENDERERGL This, int interval);
HRESULT WINAPI DXGLRendererGL_Sync(LPDXGLRENDERERGL This, void* ptr);
HRESULT WINAPI DXGLRendererGL_GetWindow(LPDXGLRENDERERGL This, HWND *hWnd);
HRESULT WINAPI DXGLRendererGL_SetWindowSize(LPDXGLRENDERERGL This, RECT r);


// Internal functions
HRESULT WINAPI DXGLRendererGL_PostCommand2(LPDXGLRENDERERGL This, struct DXGLPostQueueCmd* cmd, BOOL inner);
void DXGLRendererGL__Reset(LPDXGLRENDERERGL This);
void DXGLRendererGL__SetTexture(LPDXGLRENDERERGL This, GLuint level, DXGLTexture *texture);
void DXGLRendererGL__SetTarget(LPDXGLRENDERERGL This, DXGLTexture* texture, GLuint miplevel);
void DXGLRendererGL__SetRenderState(LPDXGLRENDERERGL This, DXGLRenderState *state);
void DXGLRendererGL__SwapBuffers(LPDXGLRENDERERGL This, int interval);
void DXGLRendererGL__DrawPrimitives(LPDXGLRENDERERGL This, D3DPRIMITIVETYPE type, const BYTE* vertices, DWORD vertexcount, const WORD* indices, DWORD indexcount);

#ifdef __cplusplus
}
#endif

#endif //_DXGLRENDERERGL_H