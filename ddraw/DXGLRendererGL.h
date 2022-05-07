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

	// OpenGL context resources
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	HGLRC hrc;
	glExtensions ext;


} IDXGLRendererGL;

// Implementation
HRESULT WINAPI DXGLRendererGL_QueryInterface(LPDXGLRENDERERGL This, REFIID riid, LPVOID *ppvObject);
ULONG WINAPI DXGLRendererGL_AddRef(LPDXGLRENDERERGL This);
ULONG WINAPI DXGLRendererGL_Release(LPDXGLRENDERERGL This);


#ifdef __cplusplus
}
#endif

#endif //_DXGLRENDERERGL_H