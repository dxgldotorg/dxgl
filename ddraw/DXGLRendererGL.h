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
	HDC hdc;

	// Sync objects
	HANDLE SyncEvent;
	HANDLE StartEvent;
	CRITICAL_SECTION cs;
	BOOL running;
	BOOL shutdown;

	// OpenGL context resources
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	HGLRC hrc;
	glExtensions ext;

	// Window state
	GLsizei width;
	GLsizei height;
	GLsizei winwidth;
	GLsizei winheight;

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


} IDXGLRendererGL;

// Device creation
HRESULT DXGLRendererGL_Create(GUID *guid, LPDXGLRENDERERGL *out);

// Implementation
HRESULT WINAPI DXGLRendererGL_QueryInterface(LPDXGLRENDERERGL This, REFIID riid, LPVOID *ppvObject);
ULONG WINAPI DXGLRendererGL_AddRef(LPDXGLRENDERERGL This);
ULONG WINAPI DXGLRendererGL_Release(LPDXGLRENDERERGL This);
HRESULT WINAPI DXGLRendererGL_GetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 **glDD7);
HRESULT WINAPI DXGLRendererGL_SetAttachedDevice(LPDXGLRENDERERGL This, struct glDirectDraw7 *glDD7);
HRESULT WINAPI DXGLRendererGL_Reset(LPDXGLRENDERERGL This);
HRESULT WINAPI DXGLRendererGL_PostCommand(LPDXGLRENDERERGL This, struct DXGLPostQueueCmd *cmd);


// Internal functions
void DXGLRendererGL__Reset(LPDXGLRENDERERGL This);

#ifdef __cplusplus
}
#endif

#endif //_DXGLRENDERERGL_H