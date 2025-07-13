// DXGL
// Copyright (C) 2012-2024 William Feely

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



// This file will be removed upon completion of the new render backend.

#include "common.h"
#include "BufferObject.h"
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "DXGLTexture.h"
#include "DXGLRenderer.h"
#include "glDirectDraw.h"
#include "glRenderWindow.h"
#include "glRenderer.h"
#include "ddraw.h"
#include "ShaderGen3D.h"
#include "matrix.h"
#include "util.h"
#include <stdarg.h>
#include "hooks.h"

extern "C" {

static const DDSURFACEDESC2 ddsdbackbuffer =
{
	sizeof(DDSURFACEDESC2),
	DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	NULL,
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{
		sizeof(DDPIXELFORMAT),
		DDPF_RGB,
		0,
		32,
		0xFF,
		0xFF00,
		0xFF0000,
		0
	},
	{
		DDSCAPS_TEXTURE,
		0,
		0,
		0
	},
	0,
};

const GLushort bltindices[4] = {0,1,2,3};

/**
  * Expands a 5-bit value to 8 bits.
  * @param number
  *  5-bit value to convert to 8 bits.
  * @return
  *  Converted 8-bit value
  */
inline int _5to8bit(int number)
{
	return (number << 3)+(number>>2);
}

/**
  * Expands a 6-bit value to 8 bits.
  * @param number
  *  6-bit value to convert to 8 bits.
  * @return
  *  Converted 8-bit value
  */
inline int _6to8bit(int number)
{
	return (number<<2)+(number>>4);
}

/**
  * Sets the Windows OpenGL swap interval
  * @param This
  *  Pointer to glRenderer object
  * @param swap
  *  Number of vertical retraces to wait per frame, 0 disable vsync
  */
void glRenderer__SetSwap(glRenderer *This, int swap)
{
	if (dxglcfg.vsync == 1) swap = 0;
	else if (dxglcfg.vsync == 2) swap = 1;
	if(swap != This->oldswap)
	{
		This->ext->wglSwapIntervalEXT(swap);
		This->ext->wglGetSwapIntervalEXT();
		This->oldswap = swap;
	}
}

/**
  * glRenderer wrapper for glTexture__Upload
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to upload to
  * @param level
  *  Mipmap level of texture to write
  */
void glRenderer__UploadTexture(glRenderer *This, glTexture *texture, GLint level)
{
	glTexture__Upload(texture, level);
}

/**
  * glRenderer wrapper function for glTexture__Download
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to download from
  * @param level
  *  Mipmap level of texture to read
  */
void glRenderer__DownloadTexture(glRenderer *This, glTexture *texture, GLint level) 
{
	glTexture__Download(texture, level);
}

/**
  * Initializes a command buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param buffer
  *  Pointer to command buffer structure
  */
void glRenderer_InitCmdBuffer(glRenderer *This, CmdBuffer *buffer)
{
	ZeroMemory(buffer, sizeof(CmdBuffer));
	BufferObject_Create(&buffer->vertices, This->ext, This->util);
	BufferObject_Create(&buffer->indices, This->ext, This->util);
	BufferObject_Create(&buffer->pixelunpack, This->ext, This->util);
	BufferObject_SetData(buffer->vertices, GL_ARRAY_BUFFER, dxglcfg.VertexBufferSize * 1024, NULL, GL_STREAM_DRAW);
	BufferObject_SetData(buffer->indices, GL_ARRAY_BUFFER, dxglcfg.IndexBufferSize * 1024, NULL, GL_STREAM_DRAW);
	BufferObject_SetData(buffer->pixelunpack, GL_ARRAY_BUFFER, dxglcfg.UnpackBufferSize * 1024, NULL, GL_STREAM_DRAW);
	buffer->cmdsize = dxglcfg.CmdBufferSize * 1024;
	buffer->cmdbuffer = (DWORD*)malloc(dxglcfg.CmdBufferSize);
}

/**
  * Initializes a glRenderer object
  * @param This
  *  Pointer to glRenderer object to initialize
  * @param width,height,bpp
  *  Width, height, and BPP of the rendering window
  * @param fullscreen
  *  True if fullscreen mode is required, false for windowed
  * @param hwnd
  *  Handle of the window to render into.  If this value is NULL, then a transparent
  *  layered window will be created for the renderer.
  * @param glDD7
  *  Pointer to the glDirectDraw7 object that is managing the glRenderer object
  * @param devwnd
  *  True if creating window with name "DirectDrawDeviceWnd"
  */
void glRenderer_Init(glRenderer *This, int width, int height, int bpp, BOOL fullscreen, unsigned int frequency, HWND hwnd, glDirectDraw7 *glDD7, BOOL devwnd)
{
	RECT wndrect;
	WINDOWPLACEMENT wndplace;
	int screenx, screeny;
	LONG_PTR winstyle, winstyleex;
	This->oldswap = 0;
	This->fogcolor = 0;
	This->fogstart = 0.0f;
	This->fogend = 1.0f;
	This->fogdensity = 1.0f;
	ZeroMemory(&This->backbuffers, 16 * sizeof(glTexture));
	This->hDC = NULL;
	This->hRC = NULL;
	This->pbo = NULL;
	This->overlays = NULL;
	This->overlaycount = 0;
	This->last_fvf = 0xFFFFFFFF; // Bogus value to force initial FVF change
	This->mode_3d = FALSE;
	ZeroMemory(&This->dib, sizeof(DIB));
	This->hWnd = hwnd;
	InitializeCriticalSection(&This->cs);
	This->busy = CreateEvent(NULL,FALSE,FALSE,NULL);
	This->start = CreateEvent(NULL,FALSE,FALSE,NULL);
	HWND hTempWnd;
	DWORD threadid;
	if(fullscreen)
	{
		switch (dxglcfg.fullmode)
		{
		case 0:    // Fullscreen
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
			ShowWindow(This->hWnd, SW_MAXIMIZE);
			break;
		case 1:    // Non-exclusive Fullscreen
		case 5:    // Windowed borderless scaled
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWnd, SW_MAXIMIZE);
			break;
		case 2:     // Windowed non-resizable
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					(winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP | WS_VISIBLE),
					CW_USEDEFAULT, CW_USEDEFAULT, wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL,
					GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
			}
			SetWindowPos(This->hWnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		case 3:     // Windowed resizable
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW), FALSE, (winstyleex | WS_EX_APPWINDOW));
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					((winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_POPUP | WS_VISIBLE)), CW_USEDEFAULT, CW_USEDEFAULT,
					wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
				AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
			}
			wndplace.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(This->hWnd, &wndplace);
			wndplace.flags = WPF_ASYNCWINDOWPLACEMENT;
			if (dxglcfg.WindowMaximized == 1) wndplace.showCmd = SW_SHOWMAXIMIZED;
			else wndplace.showCmd = SW_SHOWNORMAL;
			wndplace.rcNormalPosition = wndrect;
			SetWindowPlacement(This->hWnd, &wndplace);
			break;
		case 4:     // Windowed borderless
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(This->hWnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(This->hWnd, SW_NORMAL);
			if (dxglcfg.WindowPosition == 1)
			{
				wndrect.left = dxglcfg.WindowX;
				wndrect.top = dxglcfg.WindowY;
				wndrect.right = dxglcfg.WindowX + dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowY + dxglcfg.WindowHeight;
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
			}
			else if (dxglcfg.WindowPosition == 3)
			{
				if (!wndclassdxgltempatom) RegisterDXGLTempWindowClass();
				wndrect.left = wndrect.top = 0;
				wndrect.right = dxglcfg.WindowWidth;
				wndrect.bottom = dxglcfg.WindowHeight;
				hTempWnd = CreateWindow(wndclassdxgltemp.lpszClassName, _T("DXGL Sizing Window"),
					winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP | WS_VISIBLE), CW_USEDEFAULT, CW_USEDEFAULT,
					wndrect.right - wndrect.left, wndrect.bottom - wndrect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
				GetWindowRect(hTempWnd, &wndrect);
				DestroyWindow(hTempWnd);
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.right = dxglcfg.WindowWidth + (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.bottom = dxglcfg.WindowHeight + (screeny / 2) - (dxglcfg.WindowHeight / 2);
				wndrect.left = (screenx / 2) - (dxglcfg.WindowWidth / 2);
				wndrect.top = (screeny / 2) - (dxglcfg.WindowHeight / 2);
			}
			SetWindowPos(This->hWnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			break;
		}
	}
	if(width)
	{
		// TODO:  Adjust window rect
	}
	
	SetWindowPos(This->hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	glRenderWindow_Create(width, height, fullscreen, This->hWnd, glDD7, devwnd, &This->RenderWnd);
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)This->hWnd;
	This->inputs[6] = glDD7;
	This->inputs[7] = This;
	This->inputs[8] = (void*)devwnd;
	This->hThread = CreateThread(NULL, 0, glRenderer_ThreadEntry, This->inputs, 0, &threadid);
	WaitForSingleObject(This->busy,INFINITE);
}

__inline BOOL UnadjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	RECT r;
	ZeroMemory(&r, sizeof(RECT));
	BOOL ret = AdjustWindowRectEx(&r, dwStyle, bMenu, dwExStyle);
	if (!ret) return ret;
	else
	{
		lpRect->left -= r.left;
		lpRect->top -= r.top;
		lpRect->right -= r.right;
		lpRect->bottom -= r.bottom;
		return ret;
	}
}

/**
  * Deletes a glRenderer object
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Delete(glRenderer *This)
{
	BOOL hasmenu;
	RECT wndrect;
	LONG_PTR winstyle, winstyleex;
	WINDOWPLACEMENT wndplace;
	switch (dxglcfg.fullmode)
	{
	case 2:
	case 3:
	case 4:
		wndplace.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(This->hWnd, &wndplace);
		wndrect = wndplace.rcNormalPosition;
		if (dxglcfg.fullmode != 4)
		{
			winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
			if (GetMenu(This->hWnd)) hasmenu = TRUE;
			else hasmenu = FALSE;
			UnadjustWindowRectEx(&wndrect, winstyle, hasmenu, winstyleex);
		}
		dxglcfg.WindowX = wndrect.left;
		dxglcfg.WindowY = wndrect.top;
		dxglcfg.WindowWidth = wndrect.right - wndrect.left;
		dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
		if (dxglcfg.fullmode == 3)
		{
			if (wndplace.showCmd == SW_MAXIMIZE) dxglcfg.WindowMaximized = TRUE;
			else dxglcfg.WindowMaximized = FALSE;
		}
		SaveWindowSettings(&dxglcfg);
		break;
	default:
		break;
	}
	EnterCriticalSection(&This->cs);
	This->opcode = OP_DELETE;
	SetEvent(This->start);
	WaitForObjectAndMessages(This->busy);
	CloseHandle(This->start);
	CloseHandle(This->busy);
	LeaveCriticalSection(&This->cs);
	DeleteCriticalSection(&This->cs);
	CloseHandle(This->hThread);
}

/**
  * Gets the BPP of the DirectDraw object that owns the renderer
  * @param This
  *  Pointer to glRenderer object
  */
DWORD glRenderer_GetBPP(glRenderer *This)
{
	return This->ddInterface->primarybpp;
}

/**
  * Entry point for the renderer thread
  * @param entry
  *  Pointer to the inputs passed by the CreateThread function
  */
DWORD WINAPI glRenderer_ThreadEntry(void *entry)
{
	void **inputsin = (void**)entry;
	glRenderer *This = (glRenderer*)inputsin[7];
	return glRenderer__Entry(This);
}

/**
  * Finishes creating an OpenGL texture.  
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to finish creating
  * @param wraps,wrapt
  *  OpenGL texture wrap parameters
  */
void glRenderer_MakeTexture(glRenderer *This, glTexture *texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_CREATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Uploads the content of a surface to an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to upload to
  * @param level
  *  Mipmap level of texture to write
  */
void glRenderer_UploadTexture(glRenderer *This, glTexture *texture, GLint level) 
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)level;
	This->opcode = OP_UPLOAD;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Downloads the contents of an OpenGL texture to a the texture object's buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture object to download from
  * @param level
  *  Mipmap level of texture to read
  */
void glRenderer_DownloadTexture(glRenderer *This, glTexture *texture, GLint level)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)level;
	This->opcode = OP_DOWNLOAD;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Deletes an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  OpenGL texture to be deleted
  */
void glRenderer_DeleteTexture(glRenderer *This, glTexture * texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_DELETETEX;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Copies part or all of the contents of one texture to another.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to structure contaning all paramaters for a Blt operation.
  * @return
  *  DD_OK if the call succeeds, or DDERR_WASSTILLDRAWING if queue is full and not waiting.
  */
HRESULT glRenderer_Blt(glRenderer *This, BltCommand *cmd)
{
	/*BltCmd bltcmd;
	bltcmd.opcode = OP_DELETE;
	bltcmd.zise = sizeof(BltCmd) - 8;
	bltcmd.cmd = *cmd;
	glRenderer_AddCommand(This, (BYTE*)&bltcmd);*/
	EnterCriticalSection(&This->cs);
	RECT r,r2;
	if(((cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(cmd->dest->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		_GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->hWnd,&r2);
		if(memcmp(&r2,&r,sizeof(RECT)) != 0)
			SetWindowPos(This->RenderWnd->hWnd,NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	This->inputs[0] = cmd;
	This->opcode = OP_BLT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Updates the display with the current primary texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture to use as the primary
  * @param paltex
  *  Texture that contains the color palette for 8-bit modes
  * @param vsync
  *  Vertical sync count
  * @param previous
  *  Texture previously used as primary before a flip
  */
void glRenderer_DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = paltex;
	This->inputs[2] = (void*)vsync;
	This->inputs[3] = previous;
	This->inputs[4] = (void*)settime;
	This->inputs[5] = overlays;
	This->inputs[6] = (void*)overlaycount;
	This->opcode = OP_DRAWSCREEN;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Ensures the renderer is set up for handling Direct3D commands.
  * @param This
  *  Pointer to glRenderer object
  * @param zbuffer
  *  Nonzero if a Z buffer is present.
  * @param x
  *  Width of the initial viewport
  * @param y
  *  Height of the initial viewport
  */
void glRenderer_InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)zbuffer;
	This->inputs[1] = (void*)x;
	This->inputs[2] = (void*)y;
	This->opcode = OP_INITD3D;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Clears the viewport.
  * @param This
  *  Pointer to glRenderer object
  * @param cmd
  *  Pointer to structure contaning all paramaters for a Clear operation.
  * @return
  *  Returns D3D_OK
  */
HRESULT glRenderer_Clear(glRenderer *This, ClearCommand *cmd)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = cmd;
	This->opcode = OP_CLEAR;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Instructs the OpenGL driver to send all queued commands to the GPU.
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Flush(glRenderer *This)
{
	EnterCriticalSection(&This->cs);
	This->opcode = OP_FLUSH;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Changes the window used for rendering.
  * @param This
  *  Pointer to glRenderer object
  * @param width,height
  *  Width and height of the new window.
  * @param fullscreen
  *  True if fullscreen
  * @param newwnd
  *  HWND of the new window
  * @param devwnd
  *  True if creating window with name "DirectDrawDeviceWnd"
  */
void glRenderer_SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd)
{
	RECT wndrect, wndrect2;
	WINDOWPLACEMENT wndplace;
	BOOL hasmenu;
	int screenx, screeny;
	LONG_PTR winstyle, winstyleex;
	EnterCriticalSection(&This->cs);
	if(fullscreen && newwnd)
	{
		switch (dxglcfg.fullmode)
		{
		case 0:    // Fullscreen
			/*winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_POPUP) & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER));
			ShowWindow(newwnd, SW_MAXIMIZE);*/  //This seems to cause a black screen in some cases
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, WS_OVERLAPPED|WS_POPUP);
			ShowWindow(newwnd, SW_MAXIMIZE);
			break;
		case 1:    // Non-exclusive Fullscreen
		case 5:    // Windowed borderless scaled
			/*winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(newwnd, SW_MAXIMIZE);*/  //This seems to cause a black screen in some cases
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, WS_OVERLAPPED);
			ShowWindow(newwnd, SW_MAXIMIZE);
			break;
		case 2:     // Windowed
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP));
			ShowWindow(newwnd, SW_NORMAL);
			if ((dxglcfg.WindowPosition == 1) || (dxglcfg.WindowPosition == 3))
			{
				GetWindowRect(newwnd, &wndrect);
				if (GetMenu(newwnd)) hasmenu = TRUE;
				else hasmenu = FALSE;
				UnadjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX | WS_POPUP),
					hasmenu, winstyleex | WS_EX_APPWINDOW);
				wndrect.right = wndrect.left + width;
				wndrect.bottom = wndrect.top + height;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = 0;
				wndrect.top = 0;
				wndrect.right = width;
				wndrect.bottom = height;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
				wndrect.right = wndrect.left + width;
				wndrect.bottom = wndrect.top + height;
				AdjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), FALSE,
					(winstyleex | WS_EX_APPWINDOW));
			}
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			SetWindowPos(newwnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			SaveWindowSettings(&dxglcfg);
			break;
		case 3:     // Windowed resizable
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex | WS_EX_APPWINDOW);
			SetWindowLongPtrA(newwnd, GWL_STYLE, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP);
			wndplace.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(newwnd, &wndplace);
			if(wndplace.showCmd == SW_SHOWMAXIMIZED) ShowWindow(newwnd, SW_SHOWMAXIMIZED);
			else ShowWindow(newwnd, SW_NORMAL);
			if ((dxglcfg.WindowPosition == 1) || (dxglcfg.WindowPosition == 3))
			{
				wndrect = wndplace.rcNormalPosition;
				GetWindowRect(newwnd, &wndrect2);
				if (GetMenu(newwnd)) hasmenu = TRUE;
				else hasmenu = FALSE;
				UnadjustWindowRectEx(&wndrect, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
					hasmenu, (winstyleex | WS_EX_APPWINDOW));
				UnadjustWindowRectEx(&wndrect2, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
					hasmenu, (winstyleex | WS_EX_APPWINDOW));
				if (!dxglcfg.NoResizeWindow)
				{
					wndrect.right = wndrect.left + width;
					wndrect.bottom = wndrect.top + height;
				}
			}
			else if (dxglcfg.WindowPosition == 2)
			{
				if (dxglcfg.NoResizeWindow)
				{
					GetWindowRect(newwnd, &wndrect2);
					if (GetMenu(newwnd)) hasmenu = TRUE;
					else hasmenu = FALSE;
					UnadjustWindowRectEx(&wndrect2, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
						hasmenu, (winstyleex | WS_EX_APPWINDOW));
					glDirectDraw7_SetWindowSize(This->ddInterface,
						wndrect2.right - wndrect2.left, wndrect2.bottom - wndrect2.top);
					break;
				}
				wndrect.left = 0;
				wndrect.top = 0;
				wndrect.right = width;
				wndrect.bottom = height;
				AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
				wndrect.right -= wndrect.left;
				wndrect.left = 0;
				wndrect.bottom -= wndrect.top;
				wndrect.top = 0;
			}
			else
			{
				if (dxglcfg.NoResizeWindow)
				{
					GetWindowRect(newwnd, &wndrect2);
					if (GetMenu(newwnd)) hasmenu = TRUE;
					else hasmenu = FALSE;
					UnadjustWindowRectEx(&wndrect2, (winstyle | WS_OVERLAPPEDWINDOW) & ~WS_POPUP,
						hasmenu, (winstyleex | WS_EX_APPWINDOW));
					glDirectDraw7_SetWindowSize(This->ddInterface,
						wndrect2.right - wndrect2.left, wndrect2.bottom - wndrect2.top);
					break;
				}
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
				if (!dxglcfg.NoResizeWindow)
				{
					wndrect.right = wndrect.left + width;
					wndrect.bottom = wndrect.top + height;
				}
			}
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			if(dxglcfg.WindowPosition != 2)
				AdjustWindowRectEx(&wndrect, winstyle | WS_OVERLAPPEDWINDOW, FALSE, (winstyleex | WS_EX_APPWINDOW));
			wndplace.flags = WPF_ASYNCWINDOWPLACEMENT;
			if (!dxglcfg.NoResizeWindow) wndplace.showCmd = SW_SHOWNORMAL;
			wndplace.rcNormalPosition = wndrect;
			SetWindowPlacement(newwnd, &wndplace);
			if(dxglcfg.NoResizeWindow) glDirectDraw7_SetWindowSize(This->ddInterface,
				wndrect2.right - wndrect2.left, wndrect2.bottom - wndrect2.top);
			SaveWindowSettings(&dxglcfg);
			break;
		case 4:     // Windowed borderless
			winstyle = GetWindowLongPtrA(newwnd, GWL_STYLE);
			winstyleex = GetWindowLongPtrA(newwnd, GWL_EXSTYLE);
			SetWindowLongPtrA(newwnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
			SetWindowLongPtrA(newwnd, GWL_STYLE, winstyle & ~(WS_CAPTION | WS_THICKFRAME | WS_BORDER | WS_POPUP));
			ShowWindow(newwnd, SW_NORMAL);
			screenx = GetSystemMetrics(SM_CXSCREEN);
			screeny = GetSystemMetrics(SM_CYSCREEN);
			if ((dxglcfg.WindowPosition == 1) || (dxglcfg.WindowPosition == 3))
				GetWindowRect(newwnd, &wndrect);
			else if (dxglcfg.WindowPosition == 2)
			{
				wndrect.left = 0;
				wndrect.top = 0;
			}
			else
			{
				screenx = GetSystemMetrics(SM_CXSCREEN);
				screeny = GetSystemMetrics(SM_CYSCREEN);
				wndrect.left = (screenx / 2) - (width / 2);
				wndrect.top = (screeny / 2) - (height / 2);
			}
			wndrect.right = wndrect.left + width;
			wndrect.bottom = wndrect.top + height;
			dxglcfg.WindowX = wndrect.left;
			dxglcfg.WindowY = wndrect.top;
			dxglcfg.WindowWidth = wndrect.right - wndrect.left;
			dxglcfg.WindowHeight = wndrect.bottom - wndrect.top;
			SetWindowPos(newwnd, 0, wndrect.left, wndrect.top, wndrect.right - wndrect.left,
				wndrect.bottom - wndrect.top, SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
			SaveWindowSettings(&dxglcfg);
			break;
		}
	}
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)newwnd;
	This->inputs[6] = (void*)devwnd;
	This->opcode = OP_SETWND;
	SetEvent(This->start);
	WaitForObjectAndMessages(This->busy);
	LeaveCriticalSection(&This->cs);
}
/**
  * Draws one or more primitives to the currently selected render target.
  * @param This
  *  Pointer to glRenderer object
  * @param target
  *  Textures and mip levels of the current render target
  * @param mode
  *  OpenGL primitive drawing mode to use
  * @param vertices
  *  Pointer to vertex data
  * @param packed
  *  True if vertex data is packed (e.g. xyz,normal,texcoord,xyz,normal,etc.)
  * @param texformats
  *  Pointer to texture coordinate formats used in the call
  * @param count
  *  Number of vertices to copy to the draw command
  * @param indices
  *  List of vertex indices to use in the drawing command, may be NULL for
  *  non-indexed mode.
  * @param indexcount
  *  Number of vertex indices.  May be 0 for non-indexed mode.
  * @param flags
  *  Set to D3DDP_WAIT to wait until the queue has processed the call. (not yet
  *  implemented)
  * @return
  *  D3D_OK if the call succeeds, or D3DERR_INVALIDVERTEXTYPE if the vertex format
  *  has no position coordinates.
  */
HRESULT glRenderer_DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	EnterCriticalSection(&This->cs);
	This->inputs[1] = (void*)mode;
	This->inputs[2] = vertices;
	This->inputs[3] = texformats;
	This->inputs[4] = (void*)count;
	This->inputs[5] = indices;
	This->inputs[6] = (void*)indexcount;
	This->inputs[7] = (void*)flags;
	memcpy(&This->inputs[8], target, sizeof(RenderTarget));
	This->opcode = OP_DRAWPRIMITIVES;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Updates a clipping stencil.
  * @param This
  *  Pointer to glRenderer object
  * @param stencil
  *  Stencil texture to update
  * @param indices
  *  Pointer to array of indices representing the clip list
  * @param vertices
  *  Pointer to array of vertices representing the clip list
  * @param count
  *  Number of entries in the clip list
  * @param width
  *  Width of surface the stencil is attached to
  * @param height
  *  Height of surface the stencil is attached to
  */
void glRenderer_UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = stencil;
	This->inputs[1] = indices;
	This->inputs[2] = vertices;
	This->inputs[3] = (void*)count;
	This->inputs[4] = (void*)width;
	This->inputs[5] = (void*)height;
	This->opcode = OP_UPDATECLIPPER;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}


/**
  * Gets an estimate of the scanline currently being drawn.
  * @param This
  *  Pointer to glRenderer object
  */
unsigned int glRenderer_GetScanLine(glRenderer *This)
{
	return DXGLTimer_GetScanLine(&This->timer);
}

/**
* Fills a depth surface with a specified value.
* @param This
*  Pointer to glRenderer object
* @param cmd
*  Pointer to structure contaning all paramaters for a Blt operation, with
*  appropriate depth fill parameters filled in.
* @param parent
*  Texture representing parent surface
* @param parentlevel
*  Mipmap level of parent surface
* @return
*  DD_OK if the depth fill succeeded.
*/
HRESULT glRenderer_DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = cmd;
	This->inputs[1] = parent;
	This->inputs[2] = (void*)parentlevel;
	This->opcode = OP_DEPTHFILL;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
  * Sets a render state within the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwRendStateType
  *  Render state to change
  * @param dwRenderState
  *  New render state value
  */
void glRenderer_SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwRendStateType;
	This->inputs[1] = (void*)dwRenderState;
	This->opcode = OP_SETRENDERSTATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Binds a surface to a texture stage in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwStage
  *  Texture stage to bind (8 through 11 are reserved for 2D drawing)
  * @param Texture
  *  Texture to bind to the stage; NULL to unbind
  */
void glRenderer_SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwStage;
	This->inputs[1] = Texture;
	This->opcode = OP_SETTEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a texture stage state within the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dwStage
  *  Texture stage to modify
  * @param dwState
  *  Texture stage state to modify
  * @param dwValue
  *  New value for texture stage state.
  */
void glRenderer_SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dwStage;
	This->inputs[1] = (void*)dwState;
	This->inputs[2] = (void*)dwValue;
	This->opcode = OP_SETTEXTURESTAGESTATE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a transform matrix in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param dtstTransformStateType
  *  Transform matrix to replace
  * @param lpD3DMatrix
  *  New transform matrix
  */
void glRenderer_SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)dtstTransformStateType;
	This->inputs[1] = lpD3DMatrix;
	This->opcode = OP_SETTRANSFORM;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets the material in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param lpMaterial
  *  New material parameters
  */
void glRenderer_SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = lpMaterial;
	This->opcode = OP_SETMATERIAL;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets a light in the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param index
  *  Index of light to set
  * @param light
  *  Pointer to light to change, ignored if remove is TRUE
  */

void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)index;
	This->inputs[1] = light;
	This->opcode = OP_SETLIGHT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Removes a light from the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param index
  *  Index of light to remove
  */
void glRenderer_RemoveLight(glRenderer *This, DWORD index)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)index;
	This->opcode = OP_REMOVELIGHT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets the Direct3D viewport for the renderer.
  * @param This
  *  Pointer to glRenderer object
  * @param lpViewport
  *  New viewport parameters for renderer.
  */
void glRenderer_SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = lpViewport;
	This->opcode = OP_SETD3DVIEWPORT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets a color key for a texture object.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set color key on
* @param dwFlags
*  DDraw color key flags to select color key to add or update
* @param lpDDColorKey
*  Pointer to a DDraw color key structure to set in the texture
* @param level
*  Mip level of color key to set for DirectDraw; Direct3D colorkey operations use level 0
*/
void glRenderer_SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)dwFlags;
	This->inputs[2] = lpDDColorKey;
	This->inputs[3] = (void*)level;
	This->opcode = OP_SETTEXTURECOLORKEY;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets whether a texure has primary scaling
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set primary scaling
* @param parent
*  Parent texture this one is attached to, needed only if primary is TRUE
* @param primary
*  TRUE if texture should have primary scaling, FALSE to remove scaling
*/
void glRenderer_MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = parent;
	This->inputs[2] = (void*)primary;
	This->opcode = OP_MAKETEXTUREPRIMARY;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Generates a glFrameTerminatorGREMEDY command in OpenGL if the 
  * glFrameTerminatorGREMEDY command is available
  * (i.e. running under gDebugger or CodeXL).
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_DXGLBreak(glRenderer *This)
{
	EnterCriticalSection(&This->cs);
	This->opcode = OP_DXGLBREAK;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Frees a pointer via the backend to ensure textures are not freed early.
  * @param This
  *  Pointer to glRenderer object
  * @param ptr
  *  Pointer to free in backend thread
  */
void glRenderer_FreePointer(glRenderer *This, void *ptr)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = ptr;
	This->opcode = OP_FREEPOINTER;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Sets the surface description of a texture object.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Pointer to texture object to modify
  * @param ddsd
  *  Pointer to DDSURFACEDESC2 structure to modify the texture with
  */
void glRenderer_SetTextureSurfaceDesc(glRenderer* This, glTexture* texture, LPDDSURFACEDESC2 ddsd)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = ddsd;
	This->opcode = OP_SETTEXTURESURFACEDESC;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Main loop for glRenderer class
  * @param This
  *  Pointer to glRenderer object
  * @return
  *  Returns 0 to signal successful thread termination
  */
DWORD glRenderer__Entry(glRenderer *This)
{
	int i;
	EnterCriticalSection(&This->cs);
	glRenderer__InitGL(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2],
		(int)This->inputs[3],(unsigned int)This->inputs[4],(HWND)This->inputs[5],
		(glDirectDraw7*)This->inputs[6]);
	LeaveCriticalSection(&This->cs);
	SetEvent(This->busy);
	while(1)
	{
		WaitForSingleObject(This->start,INFINITE);
		switch(This->opcode)
		{
		case OP_DELETE:
			if(This->hRC)
			{
				if(This->dib.enabled)
				{
					if(This->dib.hbitmap) DeleteObject(This->dib.hbitmap);
					if(This->dib.hdc) DeleteDC(This->dib.hdc);
					if (This->dib.info) free(This->dib.info);
					ZeroMemory(&This->dib,sizeof(DIB));
				}
				glUtil_DeleteFBO(This->util, &This->fbo);
				if(This->pbo)
				{
					BufferObject_Release(This->pbo);
					This->pbo = NULL;
				}
				for (i = 0; i < 16; i++)
				{
					if (This->backbuffers[i].initialized)
					{
						glTexture_Release(&This->backbuffers[i], TRUE);
					}
				}
				ZeroMemory(&This->backbuffers, 16 * sizeof(glTexture));
				//glRenderer__DeleteCommandBuffer(&This->cmd1);
				//glRenderer__DeleteCommandBuffer(&This->cmd2);
				ShaderManager_Delete(This->shaders);
				glUtil_Release(This->util);
				free(This->util);
				free(This->shaders);
				free(This->ext);
				if (This->overlays) free(This->overlays);
				This->ext = NULL;
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(This->hRC);
				This->hRC = NULL;
			};
			if(This->hDC) ReleaseDC(This->RenderWnd->hWnd,This->hDC);
			This->hDC = NULL;
			if (This->dib.info) free(This->dib.info);
			This->dib.info = NULL;
			glRenderWindow_Delete(This->RenderWnd);
			This->RenderWnd = NULL;
			SetEvent(This->busy);
			return 0;
			break;
		case OP_SETWND:
			glRenderer__SetWnd(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2],
				(int)This->inputs[3],(unsigned int)This->inputs[4],(HWND)This->inputs[5],(BOOL)This->inputs[6]);
			break;
		case OP_CREATE:
			glRenderer__MakeTexture(This,(glTexture*)This->inputs[0]);
			SetEvent(This->busy);
			break;
		case OP_UPLOAD:
			glRenderer__UploadTexture(This,(glTexture*)This->inputs[0],(GLint)This->inputs[1]);
			SetEvent(This->busy);
			break;
		case OP_DOWNLOAD:
			glRenderer__DownloadTexture(This,(glTexture*)This->inputs[0],(GLint)This->inputs[1]);
			SetEvent(This->busy);
			break;
		case OP_DELETETEX:
			glRenderer__DeleteTexture(This,(glTexture*)This->inputs[0]);
			break;
		case OP_BLT:
			glRenderer__Blt(This, (BltCommand*)This->inputs[0], FALSE, FALSE);
			break;
		case OP_DRAWSCREEN:
			glRenderer__DrawScreen(This,(glTexture*)This->inputs[0],(glTexture*)This->inputs[1],
				(GLint)This->inputs[2],(glTexture*)This->inputs[3],TRUE,(BOOL)This->inputs[4],
				(OVERLAY*)This->inputs[5],(int)This->inputs[6]);
			break;
		case OP_INITD3D:
			glRenderer__InitD3D(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2]);
			break;
		case OP_CLEAR:
			glRenderer__Clear(This,(ClearCommand*)This->inputs[0]);
			break;
		case OP_FLUSH:
			glRenderer__Flush(This);
			break;
		case OP_DRAWPRIMITIVES:
			glRenderer__DrawPrimitivesOld(This,(RenderTarget*)&This->inputs[8],(GLenum)This->inputs[1],
				(GLVERTEX*)This->inputs[2],(int*)This->inputs[3],(DWORD)This->inputs[4],(LPWORD)This->inputs[5],
				(DWORD)This->inputs[6],(DWORD)This->inputs[7]);
			break;
		case OP_UPDATECLIPPER:
			glRenderer__UpdateClipper(This,(glTexture*)This->inputs[0], (GLushort*)This->inputs[1],
				(BltVertex*)This->inputs[2], (GLsizei)This->inputs[3], (GLsizei)This->inputs[4], (GLsizei)This->inputs[5]);
			break;
		case OP_DEPTHFILL:
			glRenderer__DepthFill(This, (BltCommand*)This->inputs[0], (glTexture*)This->inputs[1], (GLint)This->inputs[2]);
			break;
		case OP_SETRENDERSTATE:
			glRenderer__SetRenderState(This, (D3DRENDERSTATETYPE)(DWORD)This->inputs[0], (DWORD)This->inputs[1]);
			break;
		case OP_SETTEXTURE:
			glRenderer__SetTexture(This, (DWORD)This->inputs[0], (glTexture*)This->inputs[1]);
			break;
		case OP_SETTEXTURESTAGESTATE:
			glRenderer__SetTextureStageState(This, (DWORD)This->inputs[0], (D3DTEXTURESTAGESTATETYPE)(DWORD)This->inputs[1],
				(DWORD)This->inputs[2]);
			break;
		case OP_SETTRANSFORM:
			glRenderer__SetTransform(This, (D3DTRANSFORMSTATETYPE)(DWORD)This->inputs[0], (LPD3DMATRIX)This->inputs[1]);
			break;
		case OP_SETMATERIAL:
			glRenderer__SetMaterial(This, (LPD3DMATERIAL7)This->inputs[0]);
			break;
		case OP_SETLIGHT:
			glRenderer__SetLight(This, (DWORD)This->inputs[0], (LPD3DLIGHT7)This->inputs[1]);
			break;
		case OP_REMOVELIGHT:
			glRenderer__RemoveLight(This, (DWORD)This->inputs[0]);
			break;
		case OP_SETD3DVIEWPORT:
			glRenderer__SetD3DViewport(This, (LPD3DVIEWPORT7)This->inputs[0]);
			break;
		case OP_SETTEXTURECOLORKEY:
			glRenderer__SetTextureColorKey(This, (glTexture*)This->inputs[0], (DWORD)This->inputs[1],
				(LPDDCOLORKEY)This->inputs[2], (GLint)This->inputs[3]);
			break;
		case OP_MAKETEXTUREPRIMARY:
			glRenderer__MakeTexturePrimary(This, (glTexture*)This->inputs[0], (glTexture*)This->inputs[1], (DWORD)This->inputs[2]);
			break;
		case OP_DXGLBREAK:
			glRenderer__DXGLBreak(This, TRUE);
			break;
		case OP_ENDCOMMAND:
			glRenderer__EndCommand(This, (BOOL)This->inputs[0]);
			break;
		case OP_FREEPOINTER:
			glRenderer__FreePointer(This, (void*)This->inputs[0]);
			break;
		case OP_SETTEXTURESURFACEDESC:
			glRenderer__SetTextureSurfaceDesc(This, (glTexture*)This->inputs[0], (LPDDSURFACEDESC2)This->inputs[1]);
			break;
		}
	}
	return 0;
}

/**
  * Creates a render window and initializes OpenGL.
  * @param This
  *  Pointer to glRenderer object
  * @param width,height
  *  Width and height of the render window.
  * @param bpp
  *  Color depth of the screen.
  * @param fullscreen
  *  True if full screen mode is requested.
  * @param hWnd
  *  Handle to the window to use as the renderer.  If NULL, then creates a
  *  transparent overlay window.
  * @param glDD7
  *  Pointer to the glDirectDraw7 interface that creates the renderer.
  * @return
  *  TRUE if OpenGL has been initialized, FALSE otherwise.
  */
BOOL glRenderer__InitGL(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7)
{
	EnterCriticalSection(&dll_cs);
	This->ddInterface = glDD7;
	if(This->hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(This->hRC);
	};
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	if (dxglcfg.SingleBufferDevice) pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	else pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = bpp;
	pfd.iLayerType = PFD_MAIN_PLANE;
	InterlockedIncrement((LONG*)&gllock);
	This->hDC = GetDC(This->RenderWnd->hWnd);
	if(!This->hDC)
	{
		DEBUG("glRenderer::InitGL: Can not create hDC\n");
		InterlockedDecrement((LONG*)&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	pf = ChoosePixelFormat(This->hDC,&pfd);
	if(!pf)
	{
		DEBUG("glRenderer::InitGL: Can not get pixelformat\n");
		InterlockedDecrement((LONG*)&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!SetPixelFormat(This->hDC,pf,&pfd))
		DEBUG("glRenderer::InitGL: Can not set pixelformat\n");
	This->hRC = wglCreateContext(This->hDC);
	if(!This->hRC)
	{
		DEBUG("glRenderer::InitGL: Can not create GL context\n");
		InterlockedDecrement((LONG*)&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!wglMakeCurrent(This->hDC,This->hRC))
	{
		DEBUG("glRenderer::InitGL: Can not activate GL context\n");
		wglDeleteContext(This->hRC);
		This->hRC = NULL;
		ReleaseDC(This->RenderWnd->hWnd,This->hDC);
		This->hDC = NULL;
		InterlockedDecrement((LONG*)&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	InterlockedDecrement((LONG*)&gllock);
	LeaveCriticalSection(&dll_cs);
	This->ext = (glExtensions *)malloc(sizeof(glExtensions));
	glExtensions_Init(This->ext, This->hDC, FALSE);
	This->util = (glUtil*)malloc(sizeof(glUtil));
	glUtil_Create(This->ext, This->util);
	glRenderer__SetSwap(This,1);
	glFinish();
	DXGLTimer_Init(&This->timer);
	DXGLTimer_Calibrate(&This->timer, height, frequency);
	if (dxglcfg.vsync == 1) This->oldswap = 1;
	glRenderer__SetSwap(This,0);
	glUtil_SetViewport(This->util,0,0,width,height);
	glViewport(0,0,width,height);
	glUtil_SetDepthRange(This->util,0.0,1.0);
	glUtil_DepthWrite(This->util,TRUE);
	glUtil_DepthTest(This->util,FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	glUtil_SetDepthComp(This->util,GL_LESS);
	const GLubyte *glver = glGetString(GL_VERSION);
	This->gl_caps.Version = (GLfloat)atof((char*)glver);
	if(This->gl_caps.Version >= 2)
	{
		glver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		This->gl_caps.ShaderVer = (GLfloat)atof((char*)glver);
	}
	else This->gl_caps.ShaderVer = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&This->gl_caps.TextureMax);
	This->shaders = (ShaderManager*)malloc(sizeof(ShaderManager));
	ShaderManager_Init(This->ext, This->shaders);
	This->fbo.fbo = 0;
	glUtil_InitFBO(This->util,&This->fbo);
	glUtil_ClearColor(This->util, 0.0f, 0.0f, 0.0f, 0.0f);
	glUtil_ClearDepth(This->util, 1.0);
	glUtil_ClearStencil(This->util, 0);
	glUtil_EnableArray(This->util,-1,FALSE);
	glUtil_BlendFunc(This->util,GL_ONE,GL_ZERO);
	glUtil_BlendEnable(This->util,FALSE);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	glUtil_SetScissor(This->util,FALSE,0,0,0,0);
	glDisable(GL_SCISSOR_TEST);
	glUtil_SetCull(This->util,D3DCULL_CCW);
	glEnable(GL_CULL_FACE);
	SwapBuffers(This->hDC);
	glUtil_SetActiveTexture(This->util,0);
	glRenderer__SetFogColor(This,0);
	glRenderer__SetFogStart(This,0);
	glRenderer__SetFogEnd(This,1);
	glRenderer__SetFogDensity(This,1);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	glUtil_SetShadeMode(This->util, D3DSHADE_GOURAUD);
	if(hWnd)
	{
		This->dib.enabled = TRUE;
		This->dib.width = width;
		This->dib.height = height;
		This->dib.pitch = (((width<<3)+31)&~31) >>3;
		This->dib.pixels = NULL;
		This->dib.hdc = CreateCompatibleDC(NULL);
		if(!This->dib.info)
			This->dib.info = (BITMAPINFO*)malloc(sizeof(BITMAPINFO));
		ZeroMemory(This->dib.info,sizeof(BITMAPINFO));
		This->dib.info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		This->dib.info->bmiHeader.biBitCount = 32;
		This->dib.info->bmiHeader.biWidth = width;
		This->dib.info->bmiHeader.biHeight = height;
		This->dib.info->bmiHeader.biCompression = BI_RGB;
		This->dib.info->bmiHeader.biPlanes = 1;
		This->dib.hbitmap = CreateDIBSection(This->dib.hdc,This->dib.info,
			DIB_RGB_COLORS,(void**)&This->dib.pixels,NULL,0);
	}
	BufferObject_Create(&This->pbo, This->ext, This->util);
	BufferObject_SetData(This->pbo, GL_PIXEL_PACK_BUFFER, width*height * 4, NULL, GL_STREAM_READ);
	if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
	{
		This->postsizex = 1.0f;
		This->postsizey = 1.0f;
	}
	else
	{
		This->postsizex = dxglcfg.postsizex;
		This->postsizey = dxglcfg.postsizey;
	}
	TRACE_SYSINFO();
	return TRUE;
}

void SetColorFillUniform(DWORD color, DWORD *colorsizes, int colororder, DWORD *colorbits, GLint uniform, glExtensions *ext)
{
	DWORD r, g, b, a;
	switch (colororder)
	{
	case 0:
		r = color & colorsizes[0];
		color >>= colorbits[0];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		b = color & colorsizes[2];
		color >>= colorbits[2];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 1:
		b = color & colorsizes[2];
		color >>= colorbits[2];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		r = color & colorsizes[0];
		color >>= colorbits[0];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 2:
		a = color & colorsizes[3];
		color >>= colorbits[3];
		r = color & colorsizes[0];
		color >>= colorbits[0];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		b = color & colorsizes[2];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 3:
		a = color & colorsizes[3];
		color >>= colorbits[3];
		b = color & colorsizes[2];
		color >>= colorbits[2];
		g = color & colorsizes[1];
		color >>= colorbits[1];
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, g, b, a);
		break;
	case 4:
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, r, r, r);
		break;
	case 5:
		r = color & colorsizes[0];
		ext->glUniform4i(uniform, r, r, r, r);
		break;
	case 6:
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, a, a, a, a);
		break;
	case 7:
		r = color & colorsizes[0];
		color >>= colorbits[0];
		a = color & colorsizes[3];
		ext->glUniform4i(uniform, r, r, r, a);
		break;
	}
}

void SetColorKeyUniform(DWORD key, DWORD *colorsizes, int colororder, GLint uniform, DWORD *colorbits, glExtensions *ext)
{
	DWORD r, g, b, a;
	switch (colororder)
	{
	case 0:
		r = key & colorsizes[0];
		key >>= colorbits[0];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		b = key & colorsizes[2];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 1:
		b = key & colorsizes[2];
		key >>= colorbits[2];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 2:
		a = key & colorsizes[3];
		key >>= colorbits[3];
		r = key & colorsizes[0];
		key >>= colorbits[0];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		b = key & colorsizes[2];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 3:
		a = key & colorsizes[3];
		key >>= colorbits[3];
		b = key & colorsizes[2];
		key >>= colorbits[2];
		g = key & colorsizes[1];
		key >>= colorbits[1];
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, g, b);
		break;
	case 4:
		r = key & colorsizes[0];
		if (ext->glver_major >= 3) ext->glUniform3i(uniform, r, 0, 0);
		else ext->glUniform3i(uniform, r, r, r);
		break;
	case 5:
		r = key & colorsizes[0];
		ext->glUniform3i(uniform, r, r, r);
		break;
	case 6:
		a = key & colorsizes[3];
		ext->glUniform4i(uniform, 0, 0, 0, a);
		break;
	case 7:
		r = key & colorsizes[0];
		key >>= colorbits[0];
		a = key & colorsizes[3];
		ext->glUniform4i(uniform, r, r, r, a);
		break;
	}
}

void BltFlipLR(BltVertex *vertices)
{
	GLfloat s1, s2;
	s1 = vertices[0].s;
	s2 = vertices[2].s;
	vertices[0].s = vertices[1].s;
	vertices[2].s = vertices[3].s;
	vertices[1].s = s1;
	vertices[3].s = s2;
}

void BltFlipUD(BltVertex *vertices)
{
	GLfloat t1, t2;
	t1 = vertices[0].t;
	t2 = vertices[1].t;
	vertices[0].t = vertices[2].t;
	vertices[1].t = vertices[3].t;
	vertices[2].t = t1;
	vertices[3].t = t2;
}

void RotateBlt90(BltVertex *vertices, int times)
{
	GLfloat s0, s1, s2, s3;
	GLfloat t0, t1, t2, t3;
	switch (times)
	{
	case 0:
	default:
		return;
	case 1:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s1; vertices[0].t = t1;
		vertices[1].s = s3; vertices[1].t = t3;
		vertices[2].s = s0; vertices[2].t = t0;
		vertices[3].s = s2; vertices[3].t = t2;
		break;
	case 2:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s3; vertices[0].t = t3;
		vertices[1].s = s2; vertices[1].t = t2;
		vertices[2].s = s1; vertices[2].t = t1;
		vertices[3].s = s0; vertices[3].t = t0;
		break;
	case 3:
		s0 = vertices[0].s; t0 = vertices[0].t;
		s1 = vertices[1].s; t1 = vertices[1].t;
		s2 = vertices[2].s; t2 = vertices[2].t;
		s3 = vertices[3].s; t3 = vertices[3].t;
		vertices[0].s = s2; vertices[0].t = t2;
		vertices[1].s = s0; vertices[1].t = t0;
		vertices[2].s = s3; vertices[2].t = t3;
		vertices[3].s = s1; vertices[3].t = t1;
		break;
	}
}

void glRenderer__Blt(glRenderer *This, BltCommand *cmd, BOOL backend, BOOL bltbig)
{
	int rotates = 0;
	BOOL usedest = FALSE;
	BOOL usepattern = FALSE;
	LONG sizes[6];
	GLfloat xoffset, yoffset, xmul, ymul;
	RECT srcrect;
	RECT destrect, destrect2;
	RECT wndrect;
	BltCommand cmd2;
	glDirectDraw7_GetSizes(This->ddInterface, sizes);
	unsigned __int64 shaderid;
	DDSURFACEDESC2 ddsd;
	ddsd = cmd->dest->levels[cmd->destlevel].ddsd;
	if (!memcmp(&cmd->destrect, &nullrect, sizeof(RECT)))
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
		destrect2 = destrect;
	}
	else
	{
		destrect = cmd->destrect;
		destrect2.left = 0;
		destrect2.top = 0;
		destrect2.right = destrect.right - destrect.left;
		destrect2.bottom = destrect.bottom - destrect.top;
	}
	if ((cmd->bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->flags & DDBLT_ROP))
	{
		shaderid = PackROPBits(cmd->bltfx.dwROP, cmd->flags);
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 2) usedest = TRUE;
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 4) usepattern = TRUE;
	}
	else shaderid = cmd->flags & 0xF2FAADFF;
	if (cmd->src) shaderid |= ((long long)cmd->src->blttype << 32);
	if (!(cmd->flags & 0x80000000) && cmd->dest)
		shaderid |= ((long long)cmd->dest->blttype << 40);
	if (cmd->flags & DDBLT_KEYDEST) usedest = TRUE;
	if (IsAlphaCKey())
	{

	}
	else if (usedest)
	{
		if (cmd->flags & 0x80000000)
		{
			This->bltvertices[1].dests = This->bltvertices[3].dests = (GLfloat)(destrect.left) / (GLfloat)cmd->dest->levels[0].ddsd.dwWidth;
			This->bltvertices[0].dests = This->bltvertices[2].dests = (GLfloat)(destrect.right) / (GLfloat)cmd->dest->levels[0].ddsd.dwWidth;
			This->bltvertices[0].destt = This->bltvertices[1].destt = (GLfloat)(destrect.top) / (GLfloat)cmd->dest->levels[0].ddsd.dwHeight;
			This->bltvertices[2].destt = This->bltvertices[3].destt = (GLfloat)(destrect.bottom) / (GLfloat)cmd->dest->levels[0].ddsd.dwHeight;
		}
		else
		{
			ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
			glRenderer__DrawBackbufferRect(This, cmd->dest, destrect, destrect2, PROG_TEXTURE, 0);
			This->bltvertices[1].dests = This->bltvertices[3].dests = 0.0f;
			This->bltvertices[0].dests = This->bltvertices[2].dests = (GLfloat)(destrect.right - destrect.left) / (GLfloat)This->backbuffers[0].levels[0].ddsd.dwWidth;
			This->bltvertices[0].destt = This->bltvertices[1].destt = 1.0f;
			This->bltvertices[2].destt = This->bltvertices[3].destt = 1.0f - ((GLfloat)(destrect.bottom - destrect.top) / (GLfloat)This->backbuffers[0].levels[0].ddsd.dwHeight);
		}
	}
	ShaderManager_SetShader(This->shaders, shaderid, NULL, 1);
	GenShader2D *shader = (GenShader2D*)This->shaders->gen3d->current_genshader;
	glUtil_BlendEnable(This->util, FALSE);
	if (cmd->flags & 0x80000000)
	{
		glUtil_SetFBO(This->util, NULL);
		glUtil_SetViewport(This->util, 0, 0, sizes[4], sizes[5]);
	}
	else
	{
		do
		{
			if (glUtil_SetFBOSurface(This->util, cmd->dest, NULL, cmd->destlevel, 0, TRUE) == GL_FRAMEBUFFER_COMPLETE) break;
			if (!cmd->dest->internalformats[1]) break;
			glTexture__Repair(cmd->dest, TRUE);
			glUtil_SetFBO(This->util, NULL);
			cmd->dest->levels[cmd->destlevel].fbo.fbcolor = NULL;
			cmd->dest->levels[cmd->destlevel].fbo.fbz = NULL;
		} while (1);
		glUtil_SetViewport(This->util, 0, 0, cmd->dest->levels[cmd->destlevel].ddsd.dwWidth,
			cmd->dest->levels[cmd->destlevel].ddsd.dwHeight);
	}
	glUtil_DepthTest(This->util, FALSE);
	DDSURFACEDESC2 ddsdSrc;
	ZeroMemory(&ddsdSrc, sizeof(DDSURFACEDESC2));
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	if (cmd->src)
	{
		if(cmd->src->bigtexture && !bltbig) cmd->src = cmd->src->bigtexture;
		ddsdSrc = cmd->src->levels[cmd->srclevel].ddsd;
		if (cmd->src->levels[cmd->srclevel].dirty & 1) glTexture__Upload(cmd->src, cmd->srclevel);
	}
	if ((cmd->dest->levels[cmd->destlevel].dirty & 4) && !bltbig)
	{
		cmd2.dest = cmd->dest;
		cmd2.destrect = nullrect;
		cmd2.src = cmd->dest->bigparent;
		cmd2.srcrect = nullrect;
		cmd2.destlevel = cmd->destlevel;
		cmd2.srclevel = cmd->destlevel;
		cmd2.flags = DDBLT_WAIT;
		glRenderer__Blt(This, &cmd2, TRUE, TRUE);
		cmd->dest->levels[cmd->destlevel].dirty &= ~4;
	}
	if (cmd->dest->levels[cmd->destlevel].dirty & 1)
		glTexture__Upload(cmd->dest, cmd->destlevel);
	if (!memcmp(&cmd->srcrect, &nullrect, sizeof(RECT)))
	{
		srcrect.left = 0;
		srcrect.top = 0;
		srcrect.right = ddsdSrc.dwWidth;
		srcrect.bottom = ddsdSrc.dwHeight;
	}
	else srcrect = cmd->srcrect;
	if (cmd->flags & 0x80000000)
	{
		if (glDirectDraw7_GetFullscreen(This->ddInterface))
		{
			xmul = (GLfloat)sizes[0] / (GLfloat)sizes[2];
			ymul = (GLfloat)sizes[1] / (GLfloat)sizes[3];
			xoffset = ((GLfloat)sizes[4] - (GLfloat)sizes[0]) / 2.0f;
			yoffset = ((GLfloat)sizes[5] - (GLfloat)sizes[1]) / 2.0f;
			This->bltvertices[1].x = This->bltvertices[3].x = ((GLfloat)destrect.left * xmul) + xoffset;
			This->bltvertices[0].x = This->bltvertices[2].x = ((GLfloat)destrect.right * xmul) + xoffset;
			This->bltvertices[0].y = This->bltvertices[1].y = ((GLfloat)destrect.top * ymul) + yoffset;
			This->bltvertices[2].y = This->bltvertices[3].y = ((GLfloat)destrect.bottom * ymul) + yoffset;
		}
		else
		{
			xmul = ((GLfloat)sizes[0] / (GLfloat)sizes[2]) * dxglcfg.WindowScaleX;
			ymul = (GLfloat)sizes[1] / (GLfloat)sizes[3] * dxglcfg.WindowScaleY;
			_GetClientRect(This->hWnd, &wndrect);
			ClientToScreen(This->hWnd, (LPPOINT)&wndrect.left);
			ClientToScreen(This->hWnd, (LPPOINT)&wndrect.right);
			xoffset = (GLfloat)wndrect.left * dxglcfg.WindowScaleX;
			yoffset = ((GLfloat)sizes[1] - wndrect.bottom) * dxglcfg.WindowScaleY;
			This->bltvertices[1].x = This->bltvertices[3].x = ((GLfloat)destrect.left * xmul) - xoffset;
			This->bltvertices[0].x = This->bltvertices[2].x = ((GLfloat)destrect.right * xmul) - xoffset;
			This->bltvertices[0].y = This->bltvertices[1].y = ((GLfloat)destrect.top * ymul) + yoffset;
			This->bltvertices[2].y = This->bltvertices[3].y = ((GLfloat)destrect.bottom * ymul) + yoffset;
		}
	}
	else
	{
		This->bltvertices[1].x = This->bltvertices[3].x = (GLfloat)destrect.left;
		This->bltvertices[0].x = This->bltvertices[2].x = (GLfloat)destrect.right;
		This->bltvertices[0].y = This->bltvertices[1].y = (GLfloat)ddsd.dwHeight - (GLfloat)destrect.top;
		This->bltvertices[2].y = This->bltvertices[3].y = (GLfloat)ddsd.dwHeight - (GLfloat)destrect.bottom;
	}
	if (cmd->src && (cmd->src->target == GL_TEXTURE_RECTANGLE))
	{
		This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left;
		This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right;
		This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top;
		This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom;
	}
	else
	{
		This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
		This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
		This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
		This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
	}
	if ((cmd->bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->flags & DDBLT_DDFX))
	{
		if (cmd->bltfx.dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
			BltFlipLR(This->bltvertices);
		if (cmd->bltfx.dwDDFX & DDBLTFX_MIRRORUPDOWN)
			BltFlipUD(This->bltvertices);
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE90) rotates++;
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE180) rotates += 2;
		if (cmd->bltfx.dwDDFX & DDBLTFX_ROTATE270) rotates += 3;
		rotates &= 3;
		if (rotates)
		{
			RotateBlt90(This->bltvertices, rotates);
		}
	}
	if (cmd->flags & 0x10000000)
	{
		This->bltvertices[1].stencils = This->bltvertices[3].stencils = This->bltvertices[1].x / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
		This->bltvertices[0].stencils = This->bltvertices[2].stencils = This->bltvertices[0].x / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
		This->bltvertices[0].stencilt = This->bltvertices[1].stencilt = This->bltvertices[0].y / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
		This->bltvertices[2].stencilt = This->bltvertices[3].stencilt = This->bltvertices[2].y / (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
	}
	if (cmd->dest->levels[cmd->destlevel].fbo.fbz) glClear(GL_DEPTH_BUFFER_BIT);
	if (cmd->flags & DDBLT_COLORFILL) SetColorFillUniform(cmd->bltfx.dwFillColor, cmd->dest->colorsizes,
		cmd->dest->colororder, cmd->dest->colorbits, shader->shader.uniforms[12], This->ext);
	if ((cmd->flags & DDBLT_KEYSRC) && (cmd->src && ((cmd->src->levels[cmd->srclevel].ddsd.dwFlags & DDSD_CKSRCBLT))
		|| (cmd->flags & DDBLT_KEYSRCOVERRIDE)) && !(cmd->flags & DDBLT_COLORFILL))
	{
		if (cmd->flags & DDBLT_KEYSRCOVERRIDE)
		{
			SetColorKeyUniform(cmd->bltfx.ddckSrcColorkey.dwColorSpaceLowValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[5], cmd->src->colorbits, This->ext);
			if (cmd->flags & 0x20000000) SetColorKeyUniform(cmd->bltfx.ddckSrcColorkey.dwColorSpaceHighValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[7], cmd->src->colorbits, This->ext);
		}
		else
		{
			SetColorKeyUniform(cmd->src->levels[cmd->srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[5], cmd->src->colorbits, This->ext);
			if (cmd->flags & 0x20000000) SetColorKeyUniform(cmd->src->levels[cmd->srclevel].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue, cmd->src->colorsizes,
				cmd->src->colororder, shader->shader.uniforms[7], cmd->src->colorbits, This->ext);
		}
	}
	if (!(cmd->flags & DDBLT_COLORFILL)) This->ext->glUniform1i(shader->shader.uniforms[1], 8);
	if (cmd->flags & 0x80000000)
	{
		if ((cmd->flags & DDBLT_KEYDEST) && (This && ((cmd->dest->levels[cmd->destlevel].ddsd.dwFlags & DDSD_CKDESTOVERLAY)
			|| (cmd->flags & DDBLT_KEYDESTOVERRIDE))))
		{
			if (cmd->flags & DDBLT_KEYDESTOVERRIDE)
			{
				SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceLowValue, cmd->dest->colorsizes,
					cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
				if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceHighValue, cmd->dest->colorsizes,
					cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
			}
			else
			{
				SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestOverlay.dwColorSpaceLowValue, cmd->dest->colorsizes,
					cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
				if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestOverlay.dwColorSpaceHighValue, cmd->dest->colorsizes,
					cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
			}
		}
	}
	else if ((cmd->flags & DDBLT_KEYDEST) && (This && ((cmd->dest->levels[cmd->destlevel].ddsd.dwFlags & DDSD_CKDESTBLT)
		|| (cmd->flags & DDBLT_KEYDESTOVERRIDE))))
	{
		if (cmd->flags & DDBLT_KEYDESTOVERRIDE)
		{
			SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceLowValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
			if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->bltfx.ddckDestColorkey.dwColorSpaceHighValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
		}
		else
		{
			SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestBlt.dwColorSpaceLowValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
			if (cmd->flags & 0x40000000) SetColorKeyUniform(cmd->dest->levels[cmd->destlevel].ddsd.ddckCKDestBlt.dwColorSpaceHighValue, cmd->dest->colorsizes,
				cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
		}
	}
	if (usedest && (shader->shader.uniforms[2] != -1))
	{
		if(cmd->flags & 0x80000000)	glUtil_SetTexture(This->util, 9, (DXGLTexture*)cmd->dest);
		else glUtil_SetTexture(This->util, 9, (DXGLTexture*) &This->backbuffers[0]);
		This->ext->glUniform1i(shader->shader.uniforms[2], 9);
	}
	if (usepattern && (shader->shader.uniforms[3] != -1))
	{
		if (cmd->pattern->levels[cmd->patternlevel].dirty & 1) glTexture__Upload(cmd->pattern, cmd->patternlevel);
		glUtil_SetTexture(This->util, 10, (DXGLTexture*)cmd->pattern);
		This->ext->glUniform1i(shader->shader.uniforms[3], 10);
		This->ext->glUniform2i(shader->shader.uniforms[9],
			cmd->pattern->levels[cmd->patternlevel].ddsd.dwWidth, cmd->pattern->levels[cmd->patternlevel].ddsd.dwHeight);
	}
	if (cmd->flags & 0x10000000)  // Use clipper
	{
		glUtil_SetTexture(This->util, 11, (DXGLTexture*)cmd->dest->stencil);
		This->ext->glUniform1i(shader->shader.uniforms[4],11);
		glUtil_EnableArray(This->util, shader->shader.attribs[5], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[5], 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].stencils);
	}
	switch ((shaderid >> 32) & 0xFF)
	{
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13: // Use palette
		if (cmd->src && cmd->src->palette)
		{
			if (cmd->src->palette->levels[0].dirty & 1) glTexture__Upload(cmd->src->palette, 0);
			glUtil_SetTexture(This->util, 12, (DXGLTexture*)cmd->src->palette);
			This->ext->glUniform1i(shader->shader.uniforms[13], 12);
		}
		break;
	default:
		break;
	}
	if (cmd->src)
	{
		glUtil_SetTexture(This->util, 8, (DXGLTexture*)cmd->src);
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			if((dxglcfg.BltScale == 0) || (This->ddInterface->primarybpp == 8) || bltbig)
				glTexture__SetFilter(cmd->src, 8, GL_NEAREST, GL_NEAREST, This);
			else glTexture__SetFilter(cmd->src, 8, GL_LINEAR, GL_LINEAR, This);
		}
	}
	else glUtil_SetTexture(This->util,8,NULL);
	if (cmd->flags & 0x80000000) This->ext->glUniform4f(shader->shader.uniforms[0], 0,
		(GLfloat)sizes[4], 0, (GLfloat)sizes[5]);
	else This->ext->glUniform4f(shader->shader.uniforms[0], 0,
		(GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwWidth, 0, (GLfloat)cmd->dest->levels[cmd->destlevel].ddsd.dwHeight);
	if (cmd->src) This->ext->glUniform4i(shader->shader.uniforms[10], cmd->src->colorsizes[0], cmd->src->colorsizes[1],
		cmd->src->colorsizes[2], cmd->src->colorsizes[3]);
	if (cmd->dest) This->ext->glUniform4i(shader->shader.uniforms[11], cmd->dest->colorsizes[0], cmd->dest->colorsizes[1],
		cmd->dest->colorsizes[2], cmd->dest->colorsizes[3]);
	cmd->dest->levels[cmd->destlevel].dirty |= 2;
	glUtil_EnableArray(This->util, shader->shader.attribs[0], TRUE);
	This->ext->glVertexAttribPointer(shader->shader.attribs[0],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	if((!(cmd->flags & DDBLT_COLORFILL)) && (shader->shader.attribs[3] != -1))
	{
		glUtil_EnableArray(This->util, shader->shader.attribs[3], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[3],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	}
	if (usedest)
	{
		glUtil_EnableArray(This->util, shader->shader.attribs[4], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[4],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].dests);
	}
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glUtil_SetFBO(This->util, NULL);
	if (cmd->dest->bigparent)
	{
		ddsd.dwFlags = cmd->dest->bigparent->levels[cmd->destlevel].ddsd.dwFlags;
		ddsd.ddsCaps.dwCaps = cmd->dest->bigparent->levels[cmd->destlevel].ddsd.ddsCaps.dwCaps;
	}
	if (((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		if (!bltbig && !(cmd->flags & 0x80000000))
		{
			if(cmd->dest->bigparent)
				glRenderer__DrawScreen(This, cmd->dest, cmd->dest->bigparent->palette, 0, NULL, FALSE, TRUE, NULL, 0);
			else glRenderer__DrawScreen(This, cmd->dest, cmd->dest->palette, 0, NULL, FALSE, TRUE, NULL, 0);
		}
	This->outputs[0] = DD_OK;
	if(!backend) SetEvent(This->busy);
}

void glRenderer__MakeTexture(glRenderer *This, glTexture *texture)
{
	glTexture__FinishCreate(texture);
}

void glRenderer__DrawBackbuffer(glRenderer *This, glTexture **texture, int x, int y, int progtype, BOOL paletted, BOOL firstpass, int index)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	DWORD x2, y2;
	x2 = (DWORD)((float)x * This->postsizex);
	y2 = (DWORD)((float)y * This->postsizey);
	glUtil_SetActiveTexture(This->util,8);
	if(!This->backbuffers[index].initialized)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, &ddsdbackbuffer, sizeof(DDSURFACEDESC2));
		ddsd.dwWidth = x2;
		ddsd.lPitch = x2 * 4;
		ddsd.dwHeight = y2;
		glTexture_Create(&ddsd, &This->backbuffers[index], This, TRUE, 0);
		This->backbuffers[index].freeonrelease = FALSE;
		glUtil_InitFBO(This->util, &This->backbuffers[index].levels[0].fbo);
	}
	if((This->backbuffers[index].levels[0].ddsd.dwWidth != x2) || (This->backbuffers[index].levels[0].ddsd.dwHeight != y2))
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = x2;
		ddsd.dwHeight = y2;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(&This->backbuffers[index], &ddsd);
	}
	if (texture)
	{
		glUtil_SetFBOTextures(This->util, &This->fbo, &This->backbuffers[index], NULL, 0, 0, FALSE);
		view[0] = view[2] = 0;
		view[1] = (GLfloat)x2;
		view[3] = (GLfloat)y2;
		glUtil_SetViewport(This->util, 0, 0, x2, y2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUtil_SetTexture(This->util, 8, (DXGLTexture*)*texture);
		*texture = &This->backbuffers[index];
		if (!paletted && firstpass && (dxglcfg.postfilter == 1))
			glTexture__SetFilter(*texture, 8, GL_LINEAR, GL_LINEAR, This);
		else glTexture__SetFilter(*texture, 8, GL_NEAREST, GL_NEAREST, This);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0, 8);
		This->ext->glUniform4f(This->shaders->shaders[progtype].view, view[0], view[1], view[2], view[3]);
		This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
		This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[0].x = This->bltvertices[2].x = (float)x2;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)y2;
		glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
		This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].x);
		glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
		This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].s);
		glUtil_SetCull(This->util, D3DCULL_NONE);
		glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
		This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_SHORT, bltindices);
		glUtil_SetFBO(This->util, NULL);
	}
}

void glRenderer__DrawBackbufferRect(glRenderer *This, glTexture *texture, RECT srcrect, RECT destrect, int progtype, int index)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	int x1 = destrect.left;
	int x2 = destrect.right;
	int y1 = destrect.top;
	int y2 = destrect.bottom;
	if (index > 15) return;
	glUtil_SetActiveTexture(This->util, 0);
	if (!This->backbuffers[index].initialized)
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		memcpy(&ddsd, &ddsdbackbuffer, sizeof(DDSURFACEDESC2));
		ddsd.dwWidth = x2;
		ddsd.lPitch = x2 * 4;
		ddsd.dwHeight = y2;
		glTexture_Create(&ddsd, &This->backbuffers[index], This, TRUE, 0);
	}
	if ((This->backbuffers[index].levels[0].ddsd.dwWidth < x2) || (This->backbuffers[index].levels[0].ddsd.dwHeight < y2))
	{
		if (This->backbuffers[index].levels[0].ddsd.dwWidth > x2) x2 = This->backbuffers[index].levels[0].ddsd.dwWidth;
		if (This->backbuffers[index].levels[0].ddsd.dwHeight > y2) y2 = This->backbuffers[index].levels[0].ddsd.dwHeight;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = x2;
		ddsd.dwHeight = y2;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(&This->backbuffers[index], &ddsd);
		x2 = destrect.right;
		y2 = destrect.bottom;
	}
	glUtil_SetFBOTextures(This->util, &This->fbo, &This->backbuffers[index], NULL, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)This->backbuffers[index].levels[0].ddsd.dwWidth;
	view[3] = (GLfloat)This->backbuffers[index].levels[0].ddsd.dwHeight;
	glUtil_SetViewport(This->util, 0, 0, This->backbuffers[index].levels[0].ddsd.dwWidth, This->backbuffers[index].levels[0].ddsd.dwHeight);
	glUtil_SetScissor(This->util, TRUE, 0, 0, This->backbuffers[index].levels[0].ddsd.dwWidth, This->backbuffers[index].levels[0].ddsd.dwHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUtil_SetScissor(This->util, FALSE, 0, 0, 0, 0);
	glUtil_SetTexture(This->util, 8, (DXGLTexture*)texture);
	This->ext->glUniform1i(This->shaders->shaders[progtype].tex0, 8);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view, view[0], view[1], view[2], view[3]);
	if (texture->target == GL_TEXTURE_RECTANGLE)
	{
		This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left;
		This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right;
		This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top;
		This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom;
	}
	else
	{
		This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)texture->levels[0].ddsd.dwWidth;
		This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)texture->levels[0].ddsd.dwWidth;
		This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)texture->levels[0].ddsd.dwHeight;
		This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)texture->levels[0].ddsd.dwHeight;
	}
	This->bltvertices[1].x = This->bltvertices[3].x = (float)x1;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x2;
	This->bltvertices[0].y = This->bltvertices[1].y = (float)y1;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y2;
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_SHORT, bltindices);
	glUtil_SetFBO(This->util, NULL);
}

static DWORD dummypalette[256];

BOOL CompareRGB(DWORD rgb, BOOL greater)
{
	RGBQUAD *pixel = (RGBQUAD*)&rgb;
	RGBQUAD *comp = (RGBQUAD*)&dxglcfg.HackAutoExpandViewportValue;
	if (greater)
	{
		if ((pixel->rgbBlue >= comp->rgbBlue) && (pixel->rgbGreen >= comp->rgbGreen)
			&& (pixel->rgbRed >= comp->rgbRed)) return TRUE;
		else return FALSE;
	}
	else
	{
		if ((pixel->rgbBlue <= comp->rgbBlue) && (pixel->rgbGreen <= comp->rgbGreen)
			&& (pixel->rgbRed <= comp->rgbRed)) return TRUE;
		else return FALSE;
	}
}

DWORD RGB555toRGB888(WORD pixel)
{
	DWORD red, green, blue;
	red = _5to8bit((pixel & 0x7C00) >> 10) << 16;
	green = _5to8bit((pixel & 0x3E0) >> 5) << 8;
	blue = _5to8bit(pixel & 0x1F);
	return red | green | blue;
}

DWORD RGB565toRGB888(WORD pixel)
{
	DWORD red, green, blue;
	red = _5to8bit((pixel & 0xF800) >> 11) << 16;
	green = _6to8bit((pixel & 0x7E0) >> 5) << 8;
	blue = _5to8bit(pixel & 0x1F);
	return red | green | blue;
}

BOOL BorderColorCompare(glRenderer *This, DWORD pixel, DWORD *palette, DWORD bpp)
{
	if (!palette) palette = dummypalette;
	switch (bpp)
	{
	case 8:
		switch (dxglcfg.HackAutoExpandViewportCompare)
		{
		case 0: // Match color
			if ((palette[pixel & 0xFF] & 0xFFFFFF) == dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF) return TRUE;
			else return FALSE;
			break;
		case 1: // Color less than or equal
			return CompareRGB(palette[pixel & 0xFF], FALSE);
			break;
		case 2: // Color greater than or equal
			return CompareRGB(palette[pixel & 0xFF], TRUE);
			break;
		case 3: // Palette/value equal
			if ((pixel & 0xFF) == (dxglcfg.HackAutoExpandViewportValue & 0xFF)) return TRUE;
			else return FALSE;
			break;
		case 4: // Palette/value less than or equal
			if ((pixel & 0xFF) <= (dxglcfg.HackAutoExpandViewportValue & 0xFF)) return TRUE;
			else return FALSE;
			break;
		case 5: // Palette/value greater than or equal
			if ((pixel & 0xFF) >= (dxglcfg.HackAutoExpandViewportValue & 0xFF)) return TRUE;
			else return FALSE;
			break;
		case 6: // Match 3 palette entries
			if (((pixel & 0xFF) == (dxglcfg.HackAutoExpandViewportValue & 0xFF)) ||
				((pixel & 0xFF) == ((dxglcfg.HackAutoExpandViewportValue & 0xFF00) >> 8)) ||
				((pixel & 0xFF) == ((dxglcfg.HackAutoExpandViewportValue & 0xFF0000) >> 16))) return TRUE;
			else return FALSE;
			break;
		default:
			return FALSE;
		}
		break;
	case 15:
		switch (dxglcfg.HackAutoExpandViewportCompare)
		{
		case 0: // Match color
			if (RGB555toRGB888(pixel) == dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF) return TRUE;
			else return FALSE;
			break;
		case 1: // Color less than or equal
			return CompareRGB(RGB555toRGB888(pixel), FALSE);
			break;
		case 2: // Color greater than or equal
			return CompareRGB(RGB555toRGB888(pixel), TRUE);
			break;
		case 3: // Palette/value equal
		case 6: // Match 3 palette entries
			if ((pixel & 0x7FFF) == (dxglcfg.HackAutoExpandViewportValue & 0x7FFF)) return TRUE;
			else return FALSE;
			break;
		case 4: // Palette/value less than or equal
			if ((pixel & 0x7FFF) <= (dxglcfg.HackAutoExpandViewportValue & 0x7FFF)) return TRUE;
			else return FALSE;
			break;
		case 5: // Palette/value greater than or equal
			if ((pixel & 0x7FFF) >= (dxglcfg.HackAutoExpandViewportValue & 0x7FFF)) return TRUE;
			else return FALSE;
			break;
		default:
			return FALSE;
		}
		break;
	case 16:
		switch (dxglcfg.HackAutoExpandViewportCompare)
		{
		case 0: // Match color
			if (RGB565toRGB888(pixel) == dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF) return TRUE;
			else return FALSE;
			break;
		case 1: // Color less than or equal
			return CompareRGB(RGB565toRGB888(pixel), FALSE);
			break;
		case 2: // Color greater than or equal
			return CompareRGB(RGB565toRGB888(pixel), TRUE);
			break;
		case 3: // Palette/value equal
		case 6: // Match 3 palette entries
			if ((pixel & 0xFFFF) == (dxglcfg.HackAutoExpandViewportValue & 0xFFFF)) return TRUE;
			else return FALSE;
			break;
		case 4: // Palette/value less than or equal
			if ((pixel & 0xFFFF) <= (dxglcfg.HackAutoExpandViewportValue & 0xFFFF)) return TRUE;
			else return FALSE;
			break;
		case 5: // Palette/value greater than or equal
			if ((pixel & 0xFFFF) >= (dxglcfg.HackAutoExpandViewportValue & 0xFFFF)) return TRUE;
			else return FALSE;
			break;
		default:
			return FALSE;
		}
		break;
	case 24:
	case 32:
		switch (dxglcfg.HackAutoExpandViewportCompare)
		{
		case 0: // Match color
			if (pixel == dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF) return TRUE;
			else return FALSE;
			break;
		case 1: // Color less than or equal
			return CompareRGB(pixel, FALSE);
			break;
		case 2: // Color greater than or equal
			return CompareRGB(pixel, TRUE);
			break;
		case 3: // Palette/value equal
		case 6: // Match 3 palette entries
			if ((pixel & 0xFFFFFF) == (dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF)) return TRUE;
			else return FALSE;
			break;
		case 4: // Palette/value less than or equal
			if ((pixel & 0xFFFFFF) <= (dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF)) return TRUE;
			else return FALSE;
			break;
		case 5: // Palette/value greater than or equal
			if ((pixel & 0xFFFFFF) >= (dxglcfg.HackAutoExpandViewportValue & 0xFFFFFF)) return TRUE;
			else return FALSE;
			break;
		default:
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}
}

BOOL Is512448Scale(glRenderer *This, glTexture *primary, glTexture *palette)
{
	if (dxglcfg.HackAutoExpandViewport)
	{
		if ((primary->levels[0].ddsd.dwWidth == 640) && (primary->levels[0].ddsd.dwHeight == 480))
		{
			if (primary->levels[0].dirty & 2) glTexture__Download(primary, 0);
			if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			{
				return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, (DWORD*)palette->levels[0].buffer, 8);
			}
			else if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
			{
				if (primary->levels[0].ddsd.ddpfPixelFormat.dwRBitMask == 0x7FFF)
					return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 15);
				else return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 16);
			}
			else return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 24);
		}
		else if ((primary->levels[0].ddsd.dwWidth == 320) && (primary->levels[0].ddsd.dwHeight == 240))
		{
			if (primary->levels[0].dirty & 2) glTexture__Download(primary, 0);
			if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			{
				return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, (DWORD*)palette->levels[0].buffer, 8);
			}
			else if (primary->levels[0].ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
			{
				if (primary->levels[0].ddsd.ddpfPixelFormat.dwRBitMask == 0x7FFF)
					return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 15);
				else return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 16);
			}
			else return BorderColorCompare(This, *(DWORD*)primary->levels[0].buffer, NULL, 24);
		}
		else return FALSE;
	}
	else return FALSE;
}

static BOOL(WINAPI *__UpdateLayeredWindow)(HWND hWnd, HDC hdcDst, POINT *pptDst, SIZE *psize,
	HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags) = NULL;
static BOOL UpdateLayeredWindowFail = FALSE;
static BOOL WINAPI _UpdateLayeredWindow(HWND hWnd, HDC hdcDst, POINT *pptDst, SIZE *psize,
	HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend, DWORD dwFlags)
{
	HANDLE hUser32;
	if (!__UpdateLayeredWindow)
	{
		if (UpdateLayeredWindowFail) return FALSE;
		hUser32 = GetModuleHandle(_T("user32.dll"));
		if (!hUser32)
		{
			UpdateLayeredWindowFail = TRUE;
			return FALSE;
		}
		__UpdateLayeredWindow = (BOOL(WINAPI*)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD))
			GetProcAddress((HMODULE)hUser32, "UpdateLayeredWindow");
		if (!__UpdateLayeredWindow)
		{
			UpdateLayeredWindowFail = TRUE;
			return NULL;
		}
	}
	return __UpdateLayeredWindow(hWnd, hdcDst, pptDst, psize, hdcSrc, pptSrc, crKey, pblend, dwFlags);
}

void glRenderer__DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous, BOOL setsync, BOOL settime, OVERLAY *overlays, int overlaycount)
{
	int progtype;
	RECT r, r2;
	int i;
	unsigned __int64 shaderid;
	BltCommand bltcmd;
	glTexture *primary = texture;
	BOOL isprimary = FALSE;
	BOOL scale512448;
	if(texture->bigparent) scale512448 = Is512448Scale(This, texture->bigparent, paltex);
	else scale512448 = Is512448Scale(This, texture, paltex);
	if (overlays && overlaycount)
	{
		if (!This->overlays)
		{
			This->overlays = (OVERLAY *)malloc(overlaycount * sizeof(OVERLAY));
			This->overlaycount = overlaycount;
		}
		else
		{
			if (overlaycount != This->overlaycount)
			{
				This->overlays = (OVERLAY*)realloc(This->overlays, overlaycount * sizeof(OVERLAY));
				This->overlaycount = overlaycount;
			}
		}
		memcpy(This->overlays, overlays, overlaycount * sizeof(OVERLAY));
	}
	if (overlaycount == -1)
	{
		ZeroMemory(This->overlays, This->overlaycount * sizeof(OVERLAY));
		This->overlaycount = 0;
	}
	glUtil_BlendEnable(This->util, FALSE);
	if (previous) previous->levels[0].ddsd.ddsCaps.dwCaps &= ~DDSCAPS_FRONTBUFFER;
	texture->levels[0].ddsd.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	if (texture->bigparent)
	{
		if ((texture->bigparent->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) isprimary = TRUE;
	}
	else if ((texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) isprimary = TRUE;
	if(isprimary)
	{
		_GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->hWnd,&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(This->RenderWnd->hWnd,NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	glUtil_DepthTest(This->util, FALSE);
	RECT *viewrect = &r2;
	glRenderer__SetSwap(This,vsync);
	LONG sizes[6];
	GLfloat view[4];
	GLint viewport[4];
	if (texture->levels[0].dirty & 4)
	{
		bltcmd.dest = texture;
		bltcmd.destrect = nullrect;
		bltcmd.src = texture->bigparent;
		bltcmd.srcrect = nullrect;
		bltcmd.destlevel = 0;
		bltcmd.srclevel = 0;
		bltcmd.flags = DDBLT_WAIT;
		glRenderer__Blt(This, &bltcmd, TRUE, TRUE);
		texture->levels[0].dirty &= ~4;
	}
	if(texture->levels[0].dirty & 1) glTexture__Upload(texture, 0);
	if((texture->bigparent && texture->bigparent->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		|| (texture->levels[0].ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		if(glDirectDraw7_GetFullscreen(This->ddInterface))
		{
			glDirectDraw7_GetSizes(This->ddInterface, sizes);
			if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
				(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
			{
				if (sizes[2] <= 400) This->postsizex = 2.0f;
				else This->postsizex = 1.0f;
				if (sizes[3] <= 300) This->postsizey = 2.0f;
				else This->postsizey = 1.0f;
			}
			else
			{
				This->postsizex = dxglcfg.postsizex;
				This->postsizey = dxglcfg.postsizey;
			}
			viewport[0] = viewport[1] = 0;
			viewport[2] = sizes[4];
			viewport[3] = sizes[5];
			view[0] = (GLfloat)-(sizes[4]-sizes[0])/2;
			view[1] = (GLfloat)(sizes[4]-sizes[0])/2+sizes[0];
			view[2] = (GLfloat)(sizes[5]-sizes[1])/2+sizes[1];
			view[3] = (GLfloat)-(sizes[5]-sizes[1])/2;
		}
		else
		{
			This->postsizex = 1.0f;
			This->postsizey = 1.0f;
			viewport[0] = viewport[1] = 0;
			viewport[2] = viewrect->right;
			viewport[3] = viewrect->bottom;
			ClientToScreen(This->RenderWnd->hWnd,(LPPOINT)&viewrect->left);
			ClientToScreen(This->RenderWnd->hWnd,(LPPOINT)&viewrect->right);
			OffsetRect(viewrect, 0 - This->xoffset, 0 -	This->yoffset);
			if ((dxglcfg.WindowScaleX != 1.0f) || (dxglcfg.WindowScaleY != 1.0f))
			{
				viewrect->left = (LONG)((float)viewrect->left / dxglcfg.WindowScaleX);
				viewrect->top = (LONG)((float)viewrect->top / dxglcfg.WindowScaleY);
				viewrect->right = (LONG)((float)viewrect->right / dxglcfg.WindowScaleX);
				viewrect->bottom = (LONG)((float)viewrect->bottom / dxglcfg.WindowScaleY);
			}
			view[0] = (GLfloat)viewrect->left;
			view[1] = (GLfloat)viewrect->right;
			view[2] = (GLfloat)texture->levels[0].ddsd.dwHeight-(GLfloat)viewrect->top;
			view[3] = (GLfloat)texture->levels[0].ddsd.dwHeight-(GLfloat)viewrect->bottom;
		}
	}
	else
	{
		view[0] = 0;
		view[1] = (GLfloat)texture->levels[0].ddsd.dwWidth;
		view[2] = 0;
		view[3] = (GLfloat)texture->levels[0].ddsd.dwHeight;
	}
	glUtil_SetFBO(This->util, NULL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(This->ddInterface->primarybpp == 8)
	{
		ShaderManager_SetShader(This->shaders,PROG_PAL256,NULL,0);
		progtype = PROG_PAL256;
		if(paltex) glTexture__Upload(paltex, 0);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
		This->ext->glUniform1i(This->shaders->shaders[progtype].pal,9);
		glUtil_SetTexture(This->util,8, (DXGLTexture*)texture);
		glUtil_SetTexture(This->util,9, (DXGLTexture*)paltex);
		if(dxglcfg.scalingfilter || (This->postsizex != 1.0f) || (This->postsizey != 1.0f))
		{
			glRenderer__DrawBackbuffer(This,&texture,texture->levels[0].ddsd.dwWidth,texture->levels[0].ddsd.dwHeight,progtype,TRUE,TRUE,0);
			ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
			progtype = PROG_TEXTURE;
			glUtil_SetTexture(This->util,8, (DXGLTexture*)texture);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
		}
	}
	else
	{
		if ((This->postsizex != 1.0f) || (This->postsizey != 1.0f))
		{
			progtype = PROG_TEXTURE;
			ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
			glRenderer__DrawBackbuffer(This, &texture, texture->levels[0].ddsd.dwWidth, texture->levels[0].ddsd.dwHeight, progtype, FALSE, TRUE, 0);
			glUtil_SetTexture(This->util, 8, (DXGLTexture*)texture);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0, 0);
		}
		ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
		progtype = PROG_TEXTURE;
		glUtil_SetTexture(This->util,8, (DXGLTexture*)texture);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,8);
	}
	if (dxglcfg.scalingfilter) glTexture__SetFilter(texture, 8, GL_LINEAR, GL_LINEAR, This);
	else glTexture__SetFilter(texture, 8, GL_NEAREST, GL_NEAREST, This);
	glUtil_SetViewport(This->util,viewport[0],viewport[1],viewport[2],viewport[3]);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	if(glDirectDraw7_GetFullscreen(This->ddInterface))
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)sizes[0];
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)sizes[1];
	}
	else
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)texture->levels[0].ddsd.dwWidth;
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)texture->levels[0].ddsd.dwHeight;
	}
	if (scale512448)
	{
		if (dxglcfg.HackAutoExpandViewport == 1)
		{
			This->bltvertices[0].s = This->bltvertices[2].s = 0.9f;
			This->bltvertices[0].t = This->bltvertices[1].t = 0.966666667f;
			This->bltvertices[1].s = This->bltvertices[3].s = 0.1f;
			This->bltvertices[2].t = This->bltvertices[3].t = 0.0333333333f;
		}
		else if (dxglcfg.HackAutoExpandViewport == 2)
		{
			This->bltvertices[0].s = This->bltvertices[2].s = 0.9f;
			This->bltvertices[1].s = This->bltvertices[3].s = 0.1f;
		}
	}
	else
	{
		This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
		This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	}
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	if (This->overlays)
	{
		ZeroMemory(&bltcmd, sizeof(BltCommand));
		for (i = 0; i < This->overlaycount; i++)
		{
			if (This->overlays[i].enabled)
			{
				bltcmd.flags = 0x80000000;
				if (This->overlays[i].flags & DDOVER_DDFX)
				{
					if (This->overlays[i].flags & DDOVER_KEYDEST) bltcmd.flags |= DDBLT_KEYDEST;
					if (This->overlays[i].flags & DDOVER_KEYDESTOVERRIDE)
					{
						bltcmd.flags |= DDBLT_KEYDESTOVERRIDE;
						bltcmd.destkey = This->overlays[i].fx.dckDestColorkey;
					}
					if (This->overlays[i].flags & DDOVER_KEYSRC) bltcmd.flags |= DDBLT_KEYSRC;
					if (This->overlays[i].flags & DDOVER_KEYSRCOVERRIDE)
					{
						bltcmd.flags |= DDBLT_KEYSRCOVERRIDE;
						bltcmd.srckey = This->overlays[i].fx.dckSrcColorkey;
					}
				}
				if (primary->levels[0].ddsd.dwFlags & DDSD_CKDESTOVERLAY)
				{
					bltcmd.flags |= DDBLT_KEYDEST;
				}
				shaderid = bltcmd.flags;
				//shaderid |= ((long long)This->overlays[i].texture->blttype << 32);  // FIXME:  Use this for new renderer
				shaderid |= ((long long)texture->blttype << 40);
				//bltcmd.src = This->overlays[i].texture;  // FIXME:  Use this for new renderer
				bltcmd.srclevel = 0;
				bltcmd.srcrect = This->overlays[i].srcrect;
				bltcmd.dest = primary;
				bltcmd.destlevel = 0;
				bltcmd.destrect = This->overlays[i].destrect;
				glRenderer__Blt(This, &bltcmd, TRUE, FALSE);
			}
		}
	}
	if(dxglcfg.SingleBufferDevice) glFlush();
	if(This->hWnd) SwapBuffers(This->hDC);
	else
	{
		glReadBuffer(GL_FRONT);
		BufferObject_Bind(This->pbo, GL_PIXEL_PACK_BUFFER);
		GLint packalign;
		glGetIntegerv(GL_PACK_ALIGNMENT,&packalign);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		glDirectDraw7_GetSizes(This->ddInterface, sizes);
		glReadPixels(0,0,sizes[4],sizes[5],GL_BGRA,GL_UNSIGNED_BYTE,0);
		BufferObject_Map(This->pbo, GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		GLubyte *pixels = (GLubyte*)This->pbo->pointer;
		for(int i = 0; i < sizes[5];i++)
		{
			memcpy(&This->dib.pixels[This->dib.pitch*i],
				&pixels[((sizes[5]-1)-i)*(sizes[4]*4)],sizes[4]*4);
		}
		BufferObject_Unmap(This->pbo, GL_PIXEL_PACK_BUFFER);
		BufferObject_Unbind(This->pbo, GL_PIXEL_PACK_BUFFER);
		glPixelStorei(GL_PACK_ALIGNMENT,packalign);
		HDC hRenderDC = (HDC)::GetDC(This->RenderWnd->hWnd);
		HGDIOBJ hPrevObj = 0;
		POINT dest = {0,0};
		POINT srcpoint = {0,0};
		SIZE wnd = {This->dib.width,This->dib.height};
		BLENDFUNCTION func = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		hPrevObj = SelectObject(This->dib.hdc,This->dib.hbitmap);
		ClientToScreen(This->RenderWnd->hWnd,&dest);
		_UpdateLayeredWindow(This->RenderWnd->hWnd,hRenderDC,&dest,&wnd,
			This->dib.hdc,&srcpoint,0,&func,ULW_ALPHA);
		SelectObject(This->dib.hdc,hPrevObj);
		ReleaseDC(This->RenderWnd->hWnd,hRenderDC);
	}
	if(setsync) SetEvent(This->busy);
	if(settime) DXGLTimer_SetLastDraw(&This->timer);
}

void glRenderer__DeleteTexture(glRenderer *This, glTexture *texture)
{
	glTexture__Destroy(texture);
	SetEvent(This->busy);
}

__int64 InitShaderState(glRenderer *renderer, DWORD *renderstate, TEXTURESTAGE *texstages, D3DLIGHT7 *lights)
{
	int i;
	__int64 shader = 0;
	switch (renderstate[D3DRENDERSTATE_SHADEMODE])
	{
	case D3DSHADE_FLAT:
	default:
		break;
	case D3DSHADE_GOURAUD:
		shader |= 1;
		break;
	case D3DSHADE_PHONG:
		shader |= 3;
		break;
	}
	if (renderstate[D3DRENDERSTATE_ALPHATESTENABLE]) shader |= 4;
	shader |= ((((__int64)renderstate[D3DRENDERSTATE_ALPHAFUNC] - 1) & 7) << 3);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGTABLEMODE] & 3) << 6);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_FOGVERTEXMODE] & 3) << 8);
	if (renderstate[D3DRENDERSTATE_RANGEFOGENABLE]) shader |= (1i64 << 10);
	if (renderstate[D3DRENDERSTATE_SPECULARENABLE]) shader |= (1i64 << 11);
	if (renderstate[D3DRENDERSTATE_STIPPLEDALPHA]) shader |= (1i64 << 12);
	if (renderstate[D3DRENDERSTATE_COLORKEYENABLE]) shader |= (1i64 << 13);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_ZBIAS] & 15) << 14);
	int numlights = 0;
	for (i = 0; i < 8; i++)
		if (lights[i].dltType) numlights++;
	shader |= (__int64)numlights << 18;
	if (renderstate[D3DRENDERSTATE_LOCALVIEWER]) shader |= (1i64 << 21);
	if (renderstate[D3DRENDERSTATE_COLORKEYBLENDENABLE]) shader |= (1i64 << 22);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] & 3) << 23);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_SPECULARMATERIALSOURCE] & 3) << 25);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] & 3) << 27);
	shader |= (((__int64)renderstate[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] & 3) << 29);
	int lightindex = 0;
	for (i = 0; i < 8; i++)
	{
		if (lights[i].dltType)
		{
			if (lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				shader |= (1i64 << (38 + lightindex));
			if (lights[i].dltType == D3DLIGHT_SPOT)
				shader |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}
	if (renderstate[D3DRENDERSTATE_NORMALIZENORMALS]) shader |= (1i64 << 49);
	if (renderstate[D3DRENDERSTATE_TEXTUREMAPBLEND] == D3DTBLEND_MODULATE)
	{
		bool noalpha = false;;
		if (!texstages[0].texture) noalpha = true;
		if (texstages[0].texture)
			if (!(texstages[0].texture->levels[0].ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
				noalpha = true;
		if (noalpha) texstages[0].alphaop = D3DTOP_SELECTARG2;
		else texstages[0].alphaop = D3DTOP_MODULATE;
	}
	if (renderstate[D3DRENDERSTATE_LIGHTING]) shader |= (1i64 << 59);
	if (renderstate[D3DRENDERSTATE_COLORVERTEX]) shader |= (1i64 << 60);
	if (renderstate[D3DRENDERSTATE_FOGENABLE]) shader |= (1i64 << 61);
	if (renderstate[D3DRENDERSTATE_DITHERENABLE]) shader |= (1i64 << 62);
	for (i = 0; i < 8; i++)
	{
		renderer->shaderstate3d.texstageid[i] = texstages[i].colorop & 31;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].colorarg1 & 63) << 5;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].colorarg2 & 63) << 11;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaop & 31) << 17;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaarg1 & 63) << 22;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].alphaarg2 & 63) << 28;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].texcoordindex & 7) << 34;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].texcoordindex >> 16) & 3) << 37;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].addressu - 1) & 3) << 39;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].addressv - 1) & 3) << 41;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].magfilter & 7) << 43;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].minfilter & 3) << 46;
		//renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].mipfilter & 3) << 48;
		if (texstages[i].textransform & 7)
		{
			renderer->shaderstate3d.texstageid[i] |= 1i64 << 50;
			renderer->shaderstate3d.texstageid[i] |= (__int64)(((texstages[i].textransform & 7) - 1) & 3) << 51;
		}
		if (texstages[i].textransform & D3DTTFF_PROJECTED) renderer->shaderstate3d.texstageid[i] |= 1i64 << 53;
		renderer->shaderstate3d.texstageid[i] |= (__int64)(texstages[i].texcoordindex & 7) << 54;
		renderer->shaderstate3d.texstageid[i] |= (__int64)((texstages[i].texcoordindex >> 16) & 3) << 57;
		if (texstages[i].texture)
		{
			renderer->shaderstate3d.texstageid[i] |= 1i64 << 59;
			if (texstages[i].texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT) renderer->shaderstate3d.texstageid[i] |= 1i64 << 60;
		}
	}
	return shader;
}

void glRenderer__InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	SetEvent(This->busy);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	if (zbuffer) glUtil_DepthTest(This->util, TRUE);
	glUtil_SetDepthComp(This->util, GL_LEQUAL);
	GLfloat identity[16];
	__gluMakeIdentityf(identity);
	for (int i = 0; i < 24; i++)
		memcpy(&This->transform[i], identity, sizeof(D3DMATRIX));
	GLfloat one[4] = {1,1,1,1};
	GLfloat zero[4] = {0,0,0,1};
	glUtil_SetMaterial(This->util, one, one, zero, zero, 0);
	ZeroMemory(&This->material, sizeof(D3DMATERIAL7));
	ZeroMemory(&This->lights, 8 * sizeof(D3DLIGHT7));
	memcpy(&This->renderstate, &renderstate_default, RENDERSTATE_COUNT * sizeof(DWORD));
	This->texstages[0] = texstagedefault0;
	This->texstages[1] = This->texstages[2] = This->texstages[3] = This->texstages[4] =
		This->texstages[5] = This->texstages[6] = This->texstages[7] = This->texstages[8] =
		This->texstages[9] = This->texstages[10] = This->texstages[11] = texstagedefault1;
	This->viewport.dwX = 0;
	This->viewport.dwY = 0;
	This->viewport.dwWidth = x;
	This->viewport.dwHeight = y;
	This->viewport.dvMinZ = 0.0f;
	This->viewport.dvMaxZ = 1.0f;
	This->shaderstate3d.stateid = InitShaderState(This, This->renderstate, This->texstages, This->lights);
}

void glRenderer__Clear(glRenderer *This, ClearCommand *cmd)
{
	This->outputs[0] = (void*)D3D_OK;
	GLfloat color[4];
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	GLfloat mulx, muly;
	GLsizei x1, y1, x2, y2;
	if (cmd->zbuffer)
	{
		ztexture = cmd->zbuffer;
		zlevel = cmd->zlevel;
	}
	dwordto4float(cmd->dwColor,color);
	do
	{
		if (glUtil_SetFBOSurface(This->util, cmd->target, ztexture,
			cmd->targetlevel, zlevel, FALSE) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!cmd->target->internalformats[1]) break;
		glTexture__Repair(cmd->target, TRUE);
		glUtil_SetFBO(This->util, NULL);
		cmd->target->levels[cmd->targetlevel].fbo.fbcolor = NULL;
		cmd->target->levels[cmd->targetlevel].fbo.fbz = NULL;
	} while (1);
	int clearbits = 0;
	if(cmd->dwFlags & D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		glUtil_ClearColor(This->util, color[0], color[1], color[2], color[3]);
	}
	if(cmd->dwFlags & D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		glUtil_ClearDepth(This->util, cmd->dvZ);
		glUtil_DepthWrite(This->util, TRUE);
	}
	if(cmd->dwFlags & D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		glUtil_ClearStencil(This->util, cmd->dwStencil);
	}
	if(cmd->dwCount)
	{
		if (cmd->targetlevel == 0 && (cmd->target->levels[0].ddsd.dwWidth != cmd->target->levels[0].ddsd.dwWidth) ||
			(cmd->target->levels[0].ddsd.dwHeight != cmd->target->levels[0].ddsd.dwHeight))
		{
			mulx = (GLfloat)cmd->target->levels[0].ddsd.dwWidth / (GLfloat)cmd->target->levels[0].ddsd.dwWidth;
			muly = (GLfloat)cmd->target->levels[0].ddsd.dwHeight / (GLfloat)cmd->target->levels[0].ddsd.dwHeight;
			for (DWORD i = 0; i < cmd->dwCount; i++)
			{
				x1 = (GLsizei)(((GLfloat)cmd->lpRects[i].x1) * mulx);
				x2 = ((GLsizei)(((GLfloat)cmd->lpRects[i].x2) * mulx)) - x1;
				y1 = (GLsizei)(((GLfloat)cmd->lpRects[i].y1) * muly);
				y2 = ((GLsizei)(((GLfloat)cmd->lpRects[i].y2) * muly)) - y1;
				glUtil_SetScissor(This->util, TRUE, x1, y1, x2, y2);
				glClear(clearbits);
			}
		}
		else
		{
			if (cmd->target->bigparent)
			{
				mulx = (GLfloat)cmd->target->levels[0].ddsd.dwWidth / (GLfloat)cmd->target->bigparent->levels[0].ddsd.dwWidth;
				muly = (GLfloat)cmd->target->levels[0].ddsd.dwHeight / (GLfloat)cmd->target->bigparent->levels[0].ddsd.dwHeight;
				for (DWORD i = 0; i < cmd->dwCount; i++)
				{
					glUtil_SetScissor(This->util, TRUE, cmd->lpRects[i].x1 * mulx, cmd->lpRects[i].y1 * muly,
						(cmd->lpRects[i].x2 * mulx - cmd->lpRects[i].x1 * mulx), cmd->lpRects[i].y2 * muly - cmd->lpRects[i].y1 * muly);
					glClear(clearbits);
				}
			}
			else
			{
				for (DWORD i = 0; i < cmd->dwCount; i++)
				{
					glUtil_SetScissor(This->util, TRUE, cmd->lpRects[i].x1, cmd->lpRects[i].y1,
						(cmd->lpRects[i].x2 - cmd->lpRects[i].x1), cmd->lpRects[i].y2 - cmd->lpRects[i].y1);
					glClear(clearbits);
				}
			}
		}
		glUtil_SetScissor(This->util, false, 0, 0, 0, 0);
	}
	else glClear(clearbits);
	if(cmd->zbuffer) cmd->zbuffer->levels[zlevel].dirty |= 2;
	cmd->target->levels[cmd->targetlevel].dirty |= 2;
	SetEvent(This->busy);
}

void glRenderer__Flush(glRenderer *This)
{
	glFlush();
	SetEvent(This->busy);
}

void glRenderer__SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd)
{
	if(newwnd != This->hWnd)
	{
		EnterCriticalSection(&dll_cs);
		wglMakeCurrent(NULL, NULL);
		ReleaseDC(This->hWnd,This->hDC);
		glRenderWindow_Delete(This->RenderWnd);
		glRenderWindow_Create(width, height, fullscreen, newwnd, This->ddInterface, devwnd, &This->RenderWnd);
		PIXELFORMATDESCRIPTOR pfd;
		GLuint pf;
		InterlockedIncrement((LONG*)&gllock);
		ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		if (dxglcfg.SingleBufferDevice) pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
		else pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = bpp;
		pfd.iLayerType = PFD_MAIN_PLANE;
		This->hDC = GetDC(This->RenderWnd->hWnd);
		if(!This->hDC)
			DEBUG("glRenderer::SetWnd: Can not create hDC\n");
		pf = ChoosePixelFormat(This->hDC,&pfd);
		if(!pf)
			DEBUG("glRenderer::SetWnd: Can not get pixelformat\n");
		if(!SetPixelFormat(This->hDC,pf,&pfd))
			DEBUG("glRenderer::SetWnd: Can not set pixelformat\n");
		if(!wglMakeCurrent(This->hDC,This->hRC))
			DEBUG("glRenderer::SetWnd: Can not activate GL context\n");
		InterlockedDecrement((LONG*)&gllock);
		LeaveCriticalSection(&dll_cs);
		glRenderer__SetSwap(This,1);
		SwapBuffers(This->hDC);
		DXGLTimer_Init(&This->timer);
		DXGLTimer_Calibrate(&This->timer, height, frequency);
		glRenderer__SetSwap(This,0);
		glUtil_SetViewport(This->util, 0, 0, width, height);
	}
	if (_isnan(dxglcfg.postsizex) || _isnan(dxglcfg.postsizey) ||
		(dxglcfg.postsizex < 0.25f) || (dxglcfg.postsizey < 0.25f))
	{
		if (width <= 400) This->postsizex = 2.0f;
		else This->postsizex = 1.0f;
		if (height <= 300) This->postsizey = 2.0f;
		else This->postsizey = 1.0f;
	}
	else
	{
		This->postsizex = dxglcfg.postsizex;
		This->postsizey = dxglcfg.postsizey;
	}
	SetEvent(This->busy);
}

void glRenderer__SetBlend(glRenderer *This, DWORD src, DWORD dest)
{
	GLenum glsrc, gldest;
	bool bothalpha = false;
	switch(src)
	{
	case D3DBLEND_ZERO:
		glsrc = GL_ZERO;
		break;
	case D3DBLEND_ONE:
	default:
		glsrc = GL_ONE;
		break;
	case D3DBLEND_SRCCOLOR:
		glsrc = GL_SRC_COLOR;
		break;
	case D3DBLEND_INVSRCCOLOR:
		glsrc = GL_ONE_MINUS_SRC_COLOR;
		break;
	case D3DBLEND_SRCALPHA:
		glsrc = GL_SRC_ALPHA;
		break;
	case D3DBLEND_INVSRCALPHA:
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_DESTALPHA:
		glsrc = GL_DST_ALPHA;
		break;
	case D3DBLEND_INVDESTALPHA:
		glsrc = GL_ONE_MINUS_DST_ALPHA;
		break;
	case D3DBLEND_DESTCOLOR:
		glsrc = GL_DST_COLOR;
		break;
	case D3DBLEND_INVDESTCOLOR:
		glsrc = GL_ONE_MINUS_DST_COLOR;
		break;
	case D3DBLEND_SRCALPHASAT:
		glsrc = GL_SRC_ALPHA_SATURATE;
		break;
	case D3DBLEND_BOTHSRCALPHA:
		bothalpha = true;
		glsrc = GL_SRC_ALPHA;
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
		bothalpha = true;
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		gldest = GL_SRC_ALPHA;
		break;
	}

	if(!bothalpha) switch(dest)
	{
	case D3DBLEND_ZERO:
	default:
		gldest = GL_ZERO;
		break;
	case D3DBLEND_ONE:
		gldest = GL_ONE;
		break;
	case D3DBLEND_SRCCOLOR:
		gldest = GL_SRC_COLOR;
		break;
	case D3DBLEND_INVSRCCOLOR:
		gldest = GL_ONE_MINUS_SRC_COLOR;
		break;
	case D3DBLEND_SRCALPHA:
		gldest = GL_SRC_ALPHA;
		break;
	case D3DBLEND_INVSRCALPHA:
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_DESTALPHA:
		gldest = GL_DST_ALPHA;
		break;
	case D3DBLEND_INVDESTALPHA:
		gldest = GL_ONE_MINUS_DST_ALPHA;
		break;
	case D3DBLEND_DESTCOLOR:
		gldest = GL_DST_COLOR;
		break;
	case D3DBLEND_INVDESTCOLOR:
		gldest = GL_ONE_MINUS_DST_COLOR;
		break;
	case D3DBLEND_SRCALPHASAT:
		gldest = GL_SRC_ALPHA_SATURATE;
		break;
	case D3DBLEND_BOTHSRCALPHA:
		bothalpha = true;
		glsrc = GL_SRC_ALPHA;
		gldest = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
		bothalpha = true;
		glsrc = GL_ONE_MINUS_SRC_ALPHA;
		gldest = GL_SRC_ALPHA;
		break;
	}
	glUtil_BlendFunc(This->util, glsrc, gldest);
}

void glRenderer__UpdateFVF(glRenderer *This, DWORD fvf)
{
}

void glRenderer__DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, DWORD fvf,
	BYTE *vertices, BOOL strided, DWORD count, LPWORD indices, DWORD indexcount, DWORD flags)
{
	//BOOL transformed;
	//int i;
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	if (!vertices)
	{
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	}
	if (fvf != This->last_fvf) glRenderer__UpdateFVF(This, fvf);
	glRenderer__SetDepthComp(This);
	glUtil_DepthTest(This->util, This->renderstate[D3DRENDERSTATE_ZENABLE]);
	glUtil_DepthWrite(This->util, This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]);
	_GENSHADER *prog = &This->shaders->gen3d->current_genshader->shader;
	switch (fvf & D3DFVF_POSITION_MASK)
	{
	case 0: // Missing vertex position
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	case D3DFVF_XYZ:
		glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
		This->ext->glVertexAttribPointer(prog->attribs[0], 3, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices);
		break;
	case D3DFVF_XYZRHW:
		glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
		This->ext->glVertexAttribPointer(prog->attribs[0], 3, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices);
		if (prog->attribs[1] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[1], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[1], 1, GL_FLOAT, GL_FALSE, This->fvf_stride, vertices+(3*sizeof(float)));
		}
		break;
	case D3DFVF_XYZB1:
	case D3DFVF_XYZB2:
	case D3DFVF_XYZB3:
	case D3DFVF_XYZB4:
	case D3DFVF_XYZB5:
		FIXME("glRenderer__DrawPrimitives: Blend weights not yet supported");
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
		break;
	}
}

void glRenderer__DrawPrimitivesOld(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	BOOL haslights = FALSE;
	bool transformed;
	int i;
	glTexture *ztexture = NULL;
	GLint zlevel = 0;
	if (target->zbuffer)
	{
		ztexture = target->zbuffer;
		zlevel = target->zlevel;
	}
	if(vertices[1].data) transformed = true;
	else transformed = false;
	if(!vertices[0].data)
	{
		This->outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(This->busy);
		return;
	}
	This->shaderstate3d.stateid &= 0xFFFA3FF87FFFFFFFi64;
	int numtextures = 0;
	for (i = 0; i < 8; i++)
	{
		This->shaderstate3d.texstageid[i] &= 0xFFE7FFFFFFFFFFFFi64;
		This->shaderstate3d.texstageid[i] |= (__int64)(texformats[i] - 1) << 51;
		if (vertices[i + 10].data) numtextures++;
	}
	This->shaderstate3d.stateid |= (__int64)((numtextures-1)&7) << 31;
	if (numtextures) This->shaderstate3d.stateid |= (1i64 << 34);
	int blendweights = 0;
	for (i = 0; i < 5; i++)
		if (vertices[i + 2].data) blendweights++;
	This->shaderstate3d.stateid |= (__int64)blendweights << 46;
	if (vertices[1].data) This->shaderstate3d.stateid |= (1i64 << 50);
	if (vertices[8].data) This->shaderstate3d.stateid |= (1i64 << 35);
	if (vertices[9].data) This->shaderstate3d.stateid |= (1i64 << 36);
	if (vertices[7].data) This->shaderstate3d.stateid |= (1i64 << 37);
	ShaderManager_SetShader(This->shaders,This->shaderstate3d.stateid,This->shaderstate3d.texstageid,2);
	glRenderer__SetDepthComp(This);
	glUtil_DepthTest(This->util, This->renderstate[D3DRENDERSTATE_ZENABLE]);
	glUtil_DepthWrite(This->util, This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]);
	_GENSHADER *prog = &This->shaders->gen3d->current_genshader->shader;
	glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
	This->ext->glVertexAttribPointer(prog->attribs[0],3,GL_FLOAT,GL_FALSE,vertices[0].stride,vertices[0].data);
	if(transformed)
	{
		if(prog->attribs[1] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[1], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[1],1,GL_FLOAT,GL_FALSE,vertices[1].stride,vertices[1].data);
		}
	}
	for(i = 0; i < 5; i++)
	{
		if(vertices[i+2].data)
		{
			if(prog->attribs[i+2] != -1)
			{
				glUtil_EnableArray(This->util, prog->attribs[i + 2], TRUE);
				This->ext->glVertexAttribPointer(prog->attribs[i+2],1,GL_FLOAT,GL_FALSE,vertices[i+2].stride,vertices[i+2].data);
			}
		}
	}
	if(vertices[7].data)
	{
		if(prog->attribs[7] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[7], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[7],3,GL_FLOAT,GL_FALSE,vertices[7].stride,vertices[7].data);
		}
	}
	for(i = 0; i < 2; i++)
	{
		if(vertices[i+8].data)
		{
			if(prog->attribs[8+i] != -1)
			{
				glUtil_EnableArray(This->util, prog->attribs[8 + i], TRUE);
				This->ext->glVertexAttribPointer(prog->attribs[8+i],4,GL_UNSIGNED_BYTE,GL_TRUE,vertices[i+8].stride,vertices[i+8].data);
			}
		}
	}
	for(i = 0; i < 8; i++)
	{
		{
			switch(texformats[i])
			{
			case -1: // Null
				break;
			case 1: // s
				if (prog->attribs[i + 10] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 10], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i + 10], 1, GL_FLOAT, GL_FALSE, vertices[i + 10].stride, vertices[i + 10].data);
				}
				break;
			case 2: // st
				if(prog->attribs[i+18] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 18], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+18],2,GL_FLOAT,GL_FALSE,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 3: // str
				if(prog->attribs[i+26] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 26], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+26],3,GL_FLOAT,GL_FALSE,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 4: // strq
				if(prog->attribs[i+34] != -1)
				{
					glUtil_EnableArray(This->util, prog->attribs[i + 34], TRUE);
					This->ext->glVertexAttribPointer(prog->attribs[i+34],4,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			}

		}
	}
	glUtil_SetMaterial(This->util, (GLfloat*)&This->material.ambient, (GLfloat*)&This->material.diffuse, (GLfloat*)&This->material.specular,
		(GLfloat*)&This->material.emissive, This->material.power);

	int lightindex = 0;
	char lightname[] = "lightX.xxxxxxxxxxxxxxxx";
	for(i = 0; i < 8; i++)
	{
		if(This->lights[i].dltType)
		{
			haslights = TRUE;
			if(prog->uniforms[20+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[20+(i*12)],1,(GLfloat*)&This->lights[i].dcvDiffuse);
			if(prog->uniforms[21+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[21+(i*12)],1,(GLfloat*)&This->lights[i].dcvSpecular);
			if(prog->uniforms[22+(i*12)] != -1)
				This->ext->glUniform4fv(prog->uniforms[22+(i*12)],1,(GLfloat*)&This->lights[i].dcvAmbient);
			if(prog->uniforms[24+(i*12)] != -1)
				This->ext->glUniform3fv(prog->uniforms[24+(i*12)],1,(GLfloat*)&This->lights[i].dvDirection);
			if(This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
			{
				if(prog->uniforms[23+(i*12)] != -1)
					This->ext->glUniform3fv(prog->uniforms[23+(i*12)],1,(GLfloat*)&This->lights[i].dvPosition);
				if(prog->uniforms[25+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[25+(i*12)],This->lights[i].dvRange);
				if(prog->uniforms[26+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[26+(i*12)],This->lights[i].dvFalloff);
				if(prog->uniforms[27+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[27+(i*12)],This->lights[i].dvAttenuation0);
				if(prog->uniforms[28+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[28+(i*12)],This->lights[i].dvAttenuation1);
				if(prog->uniforms[29+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[29+(i*12)],This->lights[i].dvAttenuation2);
				if(prog->uniforms[30+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[30+(i*12)],This->lights[i].dvTheta);
				if(prog->uniforms[31+(i*12)] != -1)
					This->ext->glUniform1f(prog->uniforms[31+(i*12)],This->lights[i].dvPhi);
			}
		}
		lightindex++;
	}
	if (haslights)
	{
		if (prog->uniforms[0] != -1) This->ext->glUniformMatrix4fv(prog->uniforms[0], 1, false,
			(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD]);
		This->ext->glUniform4fv(prog->uniforms[161], 1, This->util->materialambient);
		This->ext->glUniform4fv(prog->uniforms[162], 1, This->util->materialdiffuse);
		This->ext->glUniform4fv(prog->uniforms[163], 1, This->util->materialspecular);
		This->ext->glUniform4fv(prog->uniforms[164], 1, This->util->materialemission);
		This->ext->glUniform1f(prog->uniforms[165], This->util->materialshininess);
	}
	if(prog->uniforms[1] != -1) This->ext->glUniformMatrix4fv(prog->uniforms[1], 1, false,
		(GLfloat*)&This->transform[4]);
	if (prog->uniforms[2] != -1) This->ext->glUniformMatrix4fv(prog->uniforms[2], 1, false,
		(GLfloat*)&This->transform[D3DTRANSFORMSTATE_PROJECTION]);
	if (prog->uniforms[3] != -1) This->ext->glUniformMatrix3fv(prog->uniforms[3], 1, true,
		(GLfloat*)&This->transform[5]);
	DWORD ambient = This->renderstate[D3DRENDERSTATE_AMBIENT];
	if (prog->uniforms[136] != -1)
		This->ext->glUniform4f(prog->uniforms[136], (GLfloat)RGBA_GETRED(ambient),
			(GLfloat)RGBA_GETGREEN(ambient), (GLfloat)RGBA_GETBLUE(ambient),
			(GLfloat)RGBA_GETALPHA(ambient));
	for(i = 0; i < 8; i++)
	{
		if(This->texstages[i].colorop == D3DTOP_DISABLE) break;
		if(This->texstages[i].texture)
		{
			if(This->texstages[i].texture->levels[0].dirty & 1)
			{
				glTexture__Upload(This->texstages[i].texture, 0);
			}
			if (This->texstages[i].texture)
				glTexture__SetFilter(This->texstages[i].texture, i, This->texstages[i].glmagfilter, This->texstages[i].glminfilter, This);
			glUtil_SetTexture(This->util,i, (DXGLTexture*)This->texstages[i].texture);
			glUtil_SetWrap(This->util, i, 0, This->texstages[i].addressu);
			glUtil_SetWrap(This->util, i, 1, This->texstages[i].addressv);
		}
		else glUtil_SetTexture(This->util,i,0);
		This->ext->glUniform1i(prog->uniforms[128+i],i);
		if(This->renderstate[D3DRENDERSTATE_COLORKEYENABLE] && This->texstages[i].texture && (prog->uniforms[142+i] != -1))
		{
			if(This->texstages[i].texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT)
			{
				SetColorKeyUniform(This->texstages[i].texture->levels[0].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,
					This->texstages[i].texture->colorsizes, This->texstages[i].texture->colororder,
					prog->uniforms[142 + i], This->texstages[i].texture->colorbits, This->ext);
				This->ext->glUniform4i(prog->uniforms[153+i], This->texstages[i].texture->colorsizes[0], 
					This->texstages[i].texture->colorsizes[1],
					This->texstages[i].texture->colorsizes[2],
					This->texstages[i].texture->colorsizes[3]);
			}
		}
	}
	if(prog->uniforms[137]!= -1) This->ext->glUniform1f(prog->uniforms[137],(GLfloat)This->viewport.dwWidth);
	if(prog->uniforms[138]!= -1) This->ext->glUniform1f(prog->uniforms[138],(GLfloat)This->viewport.dwHeight);
	if(prog->uniforms[139]!= -1) This->ext->glUniform1f(prog->uniforms[139],(GLfloat)This->viewport.dwX);
	if(prog->uniforms[140]!= -1) This->ext->glUniform1f(prog->uniforms[140],(GLfloat)This->viewport.dwY);
	if(prog->uniforms[141]!= -1) This->ext->glUniform1i(prog->uniforms[141],This->renderstate[D3DRENDERSTATE_ALPHAREF]);
	if(prog->uniforms[150]!= -1) This->ext->glUniform4iv(prog->uniforms[150],1,(GLint*)target->target->colorbits);
	if (prog->uniforms[166] != -1) This->ext->glUniform4fv(prog->uniforms[166], 1, This->fogcolorfloat);
	if (prog->uniforms[167] != -1) This->ext->glUniform1f(prog->uniforms[167], This->fogstart);
	if (prog->uniforms[168] != -1) This->ext->glUniform1f(prog->uniforms[168], This->fogend);
	if (prog->uniforms[169] != -1) This->ext->glUniform1f(prog->uniforms[169], This->fogdensity);
	do
	{
		if (glUtil_SetFBOSurface(This->util, target->target, ztexture,
			target->level, zlevel, FALSE) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!target->target->internalformats[1]) break;
		glTexture__Repair(target->target, TRUE);
		glUtil_SetFBO(This->util, NULL);
		target->target->levels[target->level].fbo.fbcolor = NULL;
		target->target->levels[target->level].fbo.fbz = NULL;
	} while (1);
	glUtil_SetViewport(This->util, (int)((float)This->viewport.dwX*target->mulx),
		(int)((float)This->viewport.dwY*target->muly),
		(int)((float)This->viewport.dwWidth*target->mulx),
		(int)((float)This->viewport.dwHeight*target->muly));
	glUtil_SetDepthRange(This->util, This->viewport.dvMinZ, This->viewport.dvMaxZ);
	if (This->renderstate[D3DRENDERSTATE_ALPHABLENDENABLE]) glUtil_BlendEnable(This->util, TRUE);
	else glUtil_BlendEnable(This->util, FALSE);
	glRenderer__SetBlend(This,This->renderstate[D3DRENDERSTATE_SRCBLEND],This->renderstate[D3DRENDERSTATE_DESTBLEND]);
	glUtil_SetCull(This->util, (D3DCULL)This->renderstate[D3DRENDERSTATE_CULLMODE]);
	glRenderer__SetFogColor(This,This->renderstate[D3DRENDERSTATE_FOGCOLOR]);
	glRenderer__SetFogStart(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGSTART]));
	glRenderer__SetFogEnd(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGEND]));
	glRenderer__SetFogDensity(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGDENSITY]));
	glUtil_SetPolyMode(This->util, (D3DFILLMODE)This->renderstate[D3DRENDERSTATE_FILLMODE]);
	glUtil_SetShadeMode(This->util, (D3DSHADEMODE)This->renderstate[D3DRENDERSTATE_SHADEMODE]);
	if(indices) glDrawElements(mode,indexcount,GL_UNSIGNED_SHORT,indices);
	else glDrawArrays(mode,0,count);
	if(target->zbuffer) target->zbuffer->levels[target->zlevel].dirty |= 2;
	target->target->levels[target->level].dirty |= 2;
	if(flags & D3DDP_WAIT) glFlush();
	This->outputs[0] = (void*)D3D_OK;
	SetEvent(This->busy);
	return;
}

void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo)
{
	glUtil_DeleteFBO(This->util, fbo);
	SetEvent(This->busy);
}

void glRenderer__UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height)
{
	GLfloat view[4];
	DDSURFACEDESC2 ddsd;
	if ((width != stencil->levels[0].ddsd.dwWidth) || (height != stencil->levels[0].ddsd.dwHeight))
	{
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		glTexture__SetSurfaceDesc(stencil, &ddsd);
	}
	glUtil_SetFBOTextures(This->util, &stencil->levels[0].fbo, stencil, NULL, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)width;
	view[3] = (GLfloat)height;
	glUtil_SetViewport(This->util, 0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
	ShaderManager_SetShader(This->shaders,PROG_CLIPSTENCIL,NULL,0);
	This->ext->glUniform4f(This->shaders->shaders[PROG_CLIPSTENCIL].view,view[0],view[1],view[2],view[3]);
	glUtil_EnableArray(This->util, This->shaders->shaders[PROG_CLIPSTENCIL].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[PROG_CLIPSTENCIL].pos,
		2,GL_FLOAT,false,sizeof(BltVertex),&vertices[0].x);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLES, 0, (6 * count) - 1,
		6 * count, GL_UNSIGNED_SHORT, indices);
	glUtil_SetFBO(This->util, NULL);
	SetEvent(This->busy);
}

void glRenderer__DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel)
{
	RECT destrect;
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 tmpddsd;
	BOOL usedestrect = FALSE;
	ddsd = cmd->dest->levels[cmd->destlevel].ddsd;
	if (!memcmp(&cmd->destrect, &nullrect, sizeof(RECT)))
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else
	{
		destrect = cmd->destrect;
		usedestrect = TRUE;
	}
	if (parent)
	{
		do
		{
			if (glUtil_SetFBOSurface(This->util, parent, NULL, parentlevel, 0, TRUE) == GL_FRAMEBUFFER_COMPLETE) break;
			if (!parent->internalformats[1]) break;
			glTexture__Repair(parent, TRUE);
			glUtil_SetFBO(This->util, NULL);
			parent->levels[parentlevel].fbo.fbcolor = NULL;
			parent->levels[parentlevel].fbo.fbz = NULL;
		} while (1);
	}
	else
	{
		if (!cmd->dest->dummycolor)
		{
			glTexture_CreateDummyColor(cmd->dest, TRUE);
		}
		if ((cmd->dest->levels[cmd->destlevel].ddsd.dwWidth != cmd->dest->dummycolor->levels[0].ddsd.dwWidth) ||
			(cmd->dest->levels[cmd->destlevel].ddsd.dwHeight != cmd->dest->dummycolor->levels[0].ddsd.dwHeight))
		{
			ZeroMemory(&tmpddsd, sizeof(DDSURFACEDESC2));
			tmpddsd.dwSize = sizeof(DDSURFACEDESC2);
			tmpddsd.dwWidth = cmd->dest->levels[cmd->destlevel].ddsd.dwWidth;
			tmpddsd.dwHeight = cmd->dest->levels[cmd->destlevel].ddsd.dwHeight;
			tmpddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
			glTexture__SetSurfaceDesc(cmd->dest->dummycolor, &tmpddsd);
		}			
		glUtil_SetFBOTextures(This->util, &cmd->dest->dummycolor->levels[0].fbo, cmd->dest->dummycolor,
			cmd->dest, cmd->destlevel, 0, FALSE);
	}
	glUtil_SetViewport(This->util, 0, 0, cmd->dest->levels[cmd->destlevel].ddsd.dwWidth,
		cmd->dest->levels[cmd->destlevel].ddsd.dwHeight);
	if (usedestrect) glUtil_SetScissor(This->util, TRUE, cmd->destrect.left, cmd->destrect.top,
		cmd->destrect.right, cmd->destrect.bottom);
	glUtil_DepthWrite(This->util, TRUE);
	glUtil_ClearDepth(This->util, cmd->bltfx.dwFillDepth / (double)0xFFFF); // FIXME:  SOTE depth workaround
	glClear(GL_DEPTH_BUFFER_BIT);
	if (usedestrect)glUtil_SetScissor(This->util, false, 0, 0, 0, 0);
	This->outputs[0] = DD_OK;
	SetEvent(This->busy);
}

void glRenderer__SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	SetEvent(This->busy);
	if (This->renderstate[dwRendStateType] == dwRenderState) return;
	This->renderstate[dwRendStateType] = dwRenderState;
	switch (dwRendStateType)
	{
	case D3DRENDERSTATE_SHADEMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFFCi64;
		switch (dwRenderState)
		{
		case D3DSHADE_FLAT:
		default:
			break;
		case D3DSHADE_GOURAUD:
			This->shaderstate3d.stateid |= 1;
			break;
		case D3DSHADE_PHONG:
			This->shaderstate3d.stateid |= 3;
			break;
		}
		break;
	case D3DRENDERSTATE_ALPHATESTENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= 4;
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFFBi64;
		break;
	case D3DRENDERSTATE_ALPHAFUNC:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFFC7i64;
		This->shaderstate3d.stateid |= ((((__int64)dwRenderState - 1) & 7) << 3);
		break;
	case D3DRENDERSTATE_FOGTABLEMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFF3Fi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 6);
		break;
	case D3DRENDERSTATE_FOGVERTEXMODE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFCFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 8);
		break;
	case D3DRENDERSTATE_RANGEFOGENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 10);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFFBFFi64;
		break;
	case D3DRENDERSTATE_SPECULARENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 11);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFF7FFi64;
		break;
	case D3DRENDERSTATE_STIPPLEDALPHA:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 12);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFEFFFi64;
		break;
	case D3DRENDERSTATE_COLORKEYENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 13);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFFFDFFFi64;
		break;
	case D3DRENDERSTATE_LOCALVIEWER:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 21);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFDFFFFFi64;
		break;
	case D3DRENDERSTATE_COLORKEYBLENDENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 22);
		else This->shaderstate3d.stateid &= 0xFFFFFFFFFFBFFFFFi64;
		break;
	case D3DRENDERSTATE_DIFFUSEMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFFE7FFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 23);
		break;
	case D3DRENDERSTATE_SPECULARMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFF9FFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 25);
		break;
	case D3DRENDERSTATE_AMBIENTMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFE7FFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 27);
		break;
	case D3DRENDERSTATE_EMISSIVEMATERIALSOURCE:
		This->shaderstate3d.stateid &= 0xFFFFFFFFBFFFFFFFi64;
		This->shaderstate3d.stateid |= (((__int64)dwRenderState & 3) << 29);
		break;
	case D3DRENDERSTATE_NORMALIZENORMALS:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 49);
		else This->shaderstate3d.stateid &= 0xFFFDFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_LIGHTING:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 59);
		else This->shaderstate3d.stateid &= 0xF7FFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_COLORVERTEX:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 60);
		else This->shaderstate3d.stateid &= 0xEFFFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_FOGENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 61);
		else This->shaderstate3d.stateid &= 0xDFFFFFFFFFFFFFFFi64;
		break;
	case D3DRENDERSTATE_DITHERENABLE:
		if (dwRenderState) This->shaderstate3d.stateid |= (1i64 << 62);
		else This->shaderstate3d.stateid &= 0xBFFFFFFFFFFFFFFFi64;
		break;
	default:
		break;
	}
}

void glRenderer__RemoveTextureFromD3D(glRenderer *This, glTexture *texture)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		if (This->texstages[i].texture == texture)
		{
			This->texstages[i].texture = NULL;
			This->shaderstate3d.texstageid[i] &= 0xE7FFFFFFFFFFFFFFi64;
		}
	}
}

void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture)
{
	if (This->texstages[dwStage].texture == Texture)
	{
		SetEvent(This->busy);
		return;
	}
	This->texstages[dwStage].texture = Texture;
	if (Texture)
	{
		This->shaderstate3d.texstageid[dwStage] |= 1i64 << 59;
		if (Texture->levels[0].ddsd.dwFlags & DDSD_CKSRCBLT) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 60;
		else This->shaderstate3d.texstageid[dwStage] &= 0xEFFFFFFFFFFFFFFFi64;
	}
	else This->shaderstate3d.texstageid[dwStage] &= 0xE7FFFFFFFFFFFFFFi64;
	SetEvent(This->busy);
}

void glRenderer__SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
	SetEvent(This->busy);
	switch (dwState)
	{
	case D3DTSS_COLOROP:
		This->texstages[dwStage].colorop = (D3DTEXTUREOP)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFFFFE0i64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)dwValue & 31);
		break;
	case D3DTSS_COLORARG1:
		This->texstages[dwStage].colorarg1 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFFF81Fi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 5);
		break;
	case D3DTSS_COLORARG2:
		This->texstages[dwStage].colorarg2 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFFE07FFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 11);
		break;
	case D3DTSS_ALPHAOP:
		This->texstages[dwStage].alphaop = (D3DTEXTUREOP)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFFFC1FFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 31) << 17);
		break;
	case D3DTSS_ALPHAARG1:
		This->texstages[dwStage].alphaarg1 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFFF03FFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 22);
		break;
	case D3DTSS_ALPHAARG2:
		This->texstages[dwStage].alphaarg2 = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFFFC0FFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 63) << 28);
		break;
	case D3DTSS_BUMPENVMAT00:
		memcpy(&This->texstages[dwStage].bumpenv00, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT01:
		memcpy(&This->texstages[dwStage].bumpenv01, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT10:
		memcpy(&This->texstages[dwStage].bumpenv10, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVMAT11:
		memcpy(&This->texstages[dwStage].bumpenv11, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_TEXCOORDINDEX:
		This->texstages[dwStage].texcoordindex = dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFF83FFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 34);
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)((dwValue>>16) & 7) << 37);
		break;
	case D3DTSS_ADDRESSU:
		This->texstages[dwStage].addressu = (D3DTEXTUREADDRESS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFFE7FFFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 3) << 39);
		break;
	case D3DTSS_ADDRESSV:
		This->texstages[dwStage].addressv = (D3DTEXTUREADDRESS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFFFF9FFFFFFFFFFi64;
		This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 3) << 41);
		break;
	case D3DTSS_BORDERCOLOR:
		This->texstages[dwStage].bordercolor = dwValue;
		break;
	case D3DTSS_MAGFILTER:
		This->texstages[dwStage].magfilter = (D3DTEXTUREMAGFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFFC7FFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 43);
		switch (This->texstages[dwStage].magfilter)
		{
		case 1:
		default:
			This->texstages[dwStage].glmagfilter = GL_NEAREST;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			This->texstages[dwStage].glmagfilter = GL_LINEAR;
			break;
		}
		break;
	case D3DTSS_MINFILTER:
		This->texstages[dwStage].minfilter = (D3DTEXTUREMINFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFF3FFFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 46);
		switch (This->texstages[dwStage].minfilter)
		{
		case 1:
		default:
			switch (This->texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST;
				break;
			case 2:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
				break;
			case 3:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
				break;
			}
			break;
		case 2:
		case 3:
			switch (This->texstages[dwStage].mipfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_LINEAR;
				break;
			case 2:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
				break;
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
				break;
			}
			break;
		}
		break;
	case D3DTSS_MIPFILTER:
		This->texstages[dwStage].mipfilter = (D3DTEXTUREMIPFILTER)dwValue;
		//This->shaderstate3d.texstageid[dwStage] &= 0xFFFCFFFFFFFFFFFFi64;
		//This->shaderstate3d.texstageid[dwStage] |= ((__int64)(dwValue & 7) << 48);
		switch (This->texstages[dwStage].mipfilter)
		{
		case 1:
		default:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR;
			}
			break;
		case 2:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_NEAREST;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_NEAREST;
			}
			break;
		case 3:
			switch (This->texstages[dwStage].minfilter)
			{
			case 1:
			default:
				This->texstages[dwStage].glminfilter = GL_NEAREST_MIPMAP_LINEAR;
			case 2:
			case 3:
				This->texstages[dwStage].glminfilter = GL_LINEAR_MIPMAP_LINEAR;
			}
			break;
		}
		break;
	case D3DTSS_MIPMAPLODBIAS:
		memcpy(&This->texstages[dwStage].lodbias, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_MAXMIPLEVEL:
		This->texstages[dwStage].miplevel = dwValue;
		break;
	case D3DTSS_MAXANISOTROPY:
		This->texstages[dwStage].anisotropy = dwValue;
		break;
	case D3DTSS_BUMPENVLSCALE:
		memcpy(&This->texstages[dwStage].bumpenvlscale, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_BUMPENVLOFFSET:
		memcpy(&This->texstages[dwStage].bumpenvloffset, &dwValue, sizeof(D3DVALUE));
		break;
	case D3DTSS_TEXTURETRANSFORMFLAGS:
		This->texstages[dwStage].textransform = (D3DTEXTURETRANSFORMFLAGS)dwValue;
		This->shaderstate3d.texstageid[dwStage] &= 0xFFC3FFFFFFFFFFFFi64;
		if (dwValue & 7)
		{
			This->shaderstate3d.texstageid[dwStage] |= 1i64 << 50;
			This->shaderstate3d.texstageid[dwStage] |= (__int64)(((dwValue & 7) - 1) & 3) << 51;
		}
		if (dwValue & D3DTTFF_PROJECTED) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 53;
		break;
	default:
		break;
	}
}

void glRenderer__SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix)
{
	int x, y;
	GLfloat temp[16];
	GLfloat* out;
	if (dtstTransformStateType > 23)
	{
		SetEvent(This->busy);
		return;
	}
	memcpy(&This->transform[dtstTransformStateType], lpD3DMatrix, sizeof(D3DMATRIX));
	if ((dtstTransformStateType == D3DTRANSFORMSTATE_WORLD) || (dtstTransformStateType == D3DTRANSFORMSTATE_VIEW))
	{
		__gluMultMatricesf((const GLfloat *)&This->transform[D3DTRANSFORMSTATE_WORLD],
			(const GLfloat *)&This->transform[D3DTRANSFORMSTATE_VIEW], (GLfloat *)&This->transform[4]);
		__gluInvertMatrixf((const GLfloat *)&This->transform[4], (GLfloat*)&temp);
		out = (GLfloat *)&This->transform[5];
		for (y = 0; y < 3; y++)
			for (x = 0; x < 3; x++)
				out[x + (y * 3)] = temp[x + (y * 4)];
	}
	SetEvent(This->busy);
}

void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	memcpy(&This->material, lpMaterial, sizeof(D3DMATERIAL7));
	SetEvent(This->busy);
}

void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light)
{
	int numlights = 0;
	int lightindex = 0;
	memcpy(&This->lights[index], light, sizeof(D3DLIGHT7));
	SetEvent(This->busy);
	for (int i = 0; i < 8; i++)
		if (This->lights[i].dltType) numlights++;
	This->shaderstate3d.stateid &= 0xF807C03FFFE3FFFFi64;
	This->shaderstate3d.stateid |= ((__int64)numlights << 18);
	for (int i = 0; i < 8; i++)
	{
		if (This->lights[i].dltType != 1)
		{
			if (This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				This->shaderstate3d.stateid |= (1i64 << (38 + lightindex));
			if (This->lights[i].dltType == D3DLIGHT_SPOT)
				This->shaderstate3d.stateid |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}

}

void glRenderer__RemoveLight(glRenderer *This, DWORD index)
{
	int numlights = 0;
	int lightindex = 0;
	ZeroMemory(&This->lights[index], sizeof(D3DLIGHT7));
	SetEvent(This->busy);
	for (int i = 0; i < 8; i++)
		if (This->lights[i].dltType) numlights++;
	This->shaderstate3d.stateid &= 0xF807C03FFFE3FFFFi64;
	This->shaderstate3d.stateid |= ((__int64)numlights << 18);
	for (int i = 0; i < 8; i++)
	{
		if (This->lights[i].dltType != 1)
		{
			if (This->lights[i].dltType != D3DLIGHT_DIRECTIONAL)
				This->shaderstate3d.stateid |= (1i64 << (38 + lightindex));
			if (This->lights[i].dltType == D3DLIGHT_SPOT)
				This->shaderstate3d.stateid |= (1i64 << (51 + lightindex));
			lightindex++;
		}
	}
}

void glRenderer__SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	memcpy(&This->viewport, lpViewport, sizeof(D3DVIEWPORT7));
	SetEvent(This->busy);
}

void glRenderer__SetFogColor(glRenderer *This, DWORD color)
{
	if (color == This->fogcolor) return;
	This->fogcolor = color;
	This->fogcolorfloat[0] = (GLfloat)((color >> 16) & 255) / 255.0f;
	This->fogcolorfloat[1] = (GLfloat)((color >> 8) & 255) / 255.0f;
	This->fogcolorfloat[2] = (GLfloat)(color & 255) / 255.0f;
	This->fogcolorfloat[3] = (GLfloat)((color >> 24) & 255) / 255.0f;
}

void glRenderer__SetFogStart(glRenderer *This, GLfloat start)
{
	if (start == This->fogstart) return;
	This->fogstart = start;
}

void glRenderer__SetFogEnd(glRenderer *This, GLfloat end)
{
	if (end == This->fogend) return;
	This->fogend = end;
}

void glRenderer__SetFogDensity(glRenderer *This, GLfloat density)
{
	if (density == This->fogdensity) return;
	This->fogdensity = density;
}

void glRenderer__SetDepthComp(glRenderer *This)
{
	switch (This->renderstate[D3DRENDERSTATE_ZFUNC])
	{
	case D3DCMP_NEVER:
		glUtil_SetDepthComp(This->util, GL_NEVER);
		break;
	case D3DCMP_LESS:
		glUtil_SetDepthComp(This->util, GL_LESS);
		break;
	case D3DCMP_EQUAL:
		glUtil_SetDepthComp(This->util, GL_EQUAL);
		break;
	case D3DCMP_LESSEQUAL:
		glUtil_SetDepthComp(This->util, GL_LEQUAL);
		break;
	case D3DCMP_GREATER:
		glUtil_SetDepthComp(This->util, GL_GREATER);
		break;
	case D3DCMP_NOTEQUAL:
		glUtil_SetDepthComp(This->util, GL_NOTEQUAL);
		break;
	case D3DCMP_GREATEREQUAL:
		glUtil_SetDepthComp(This->util, GL_GEQUAL);
		break;
	case D3DCMP_ALWAYS:
	default:
		glUtil_SetDepthComp(This->util, GL_ALWAYS);
		break;
	}
}

void glRenderer__SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level)
{
	if (dwFlags & DDCKEY_SRCBLT)
	{
		if (lpDDColorKey)
		{
			texture->levels[level].ddsd.dwFlags |= DDSD_CKSRCBLT;
			texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
			if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
			else texture->levels[level].ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else texture->levels[level].ddsd.dwFlags &= ~DDSD_CKSRCBLT;
	}
	if (dwFlags & DDCKEY_DESTBLT)
	{
		if (lpDDColorKey)
		{
			texture->levels[level].ddsd.dwFlags |= DDSD_CKDESTBLT;
			texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
			if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
			else texture->levels[level].ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else texture->levels[level].ddsd.dwFlags &= ~DDSD_CKDESTBLT;
	}
	if (dwFlags & DDCKEY_SRCOVERLAY)
	{
		if (lpDDColorKey)
		{
			texture->levels[level].ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
			texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
			if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
			else texture->levels[level].ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else texture->levels[level].ddsd.dwFlags &= ~DDSD_CKSRCOVERLAY;
	}
	if (dwFlags & DDCKEY_DESTOVERLAY)
	{
		if (lpDDColorKey)
		{
			texture->levels[level].ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
			texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
			if (DDCKEY_COLORSPACE) texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
			else texture->levels[level].ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		}
		else texture->levels[level].ddsd.dwFlags &= ~DDSD_CKDESTOVERLAY;
	}
	SetEvent(This->busy);
}

void glRenderer__MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary)
{
	/*if (primary)
	{
		if (!parent) return;
		glTexture__SetPrimaryScale(texture, parent->levels[0].ddsd.dwWidth, parent->levels[0].ddsd.dwHeight, TRUE);
	}
	else glTexture__SetPrimaryScale(texture, 0, 0, FALSE);*/
	if (!texture->bigtexture)
	{

	}
	SetEvent(This->busy);
}

void glRenderer__DXGLBreak(glRenderer *This, BOOL setbusy)
{
	if (This->ext->GLEXT_GREMEDY_frame_terminator) This->ext->glFrameTerminatorGREMEDY();
	if(setbusy) SetEvent(This->busy);
}

void glRenderer__EndCommand(glRenderer *This, BOOL wait)
{
	// Do set-up and flip here
	if (!wait) SetEvent(This->busy);
	// Do command execution here
	if (wait) SetEvent(This->busy);
}

void glRenderer__SetMode3D(glRenderer *This, BOOL enabled)
{
// TODO:  Set 3D mode
}

void glRenderer__FreePointer(glRenderer *This, void *ptr)
{
	SetEvent(This->busy);
	free(ptr);
}

void glRenderer__SetTextureSurfaceDesc(glRenderer* This, glTexture* texture, LPDDSURFACEDESC2 ddsd)
{
	DDSURFACEDESC2 ddsd_copy;
	memcpy(&ddsd_copy, ddsd, sizeof(DDSURFACEDESC2));
	SetEvent(This->busy);
	glTexture__SetSurfaceDesc(texture, &ddsd_copy);
}

}