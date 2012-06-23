// DXGL
// Copyright (C) 2012 William Feely

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

#include "common.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glRenderWindow.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include "glutil.h"
#include "ddraw.h"
#include "scalers.h"
#include <string>
using namespace std;
#include "shadergen.h"
#include "matrix.h"

GLuint backbuffer = 0;
int backx = 0;
int backy = 0;
BltVertex bltvertices[4];
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

int oldswap = 0;
int swapinterval = 0;

/**
  * Sets the Windows OpenGL swap interval
  * @param swap
  *  Number of vertical retraces to wait per frame, 0 disable vsync
  */
inline void SetSwap(int swap)
{
	if(swap != oldswap)
	{
		wglSwapIntervalEXT(swap);
		oldswap = wglGetSwapIntervalEXT();
		oldswap = swap;
	}
}

/**
  * Internal function for uploading surface content to an OpenGL texture
  * @param buffer
  *  Contains the contents of the surface
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.
  * @param texture
  *  OpenGL texture name to upload to
  * @param x,y
  *  Width and height of the surface
  * @param bigx,bigy
  *  Width and height of the scaled surface buffer
  * @param pitch
  *  Bytes from one line of graphics to the next in the surface
  * @param bigpitch
  *  Pitch of the scaled surface buffer
  * @param bpp
  *  Number of bits per surface pixel
  * @param texformat
  *  OpenGL format parameter for glTexImage2D
  * @param texformat2
  *  OpenGL type parameter for glTexImage2D
  * @param texformat3
  *  OpenGL internalformat parameter for glTexImage2D
  */
void glRenderer::_UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y,
	int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3)
{
	if(bpp == 15) bpp = 16;
	glBindTexture(GL_TEXTURE_2D,texture);  // Select surface's texture
	if((x == bigx && y == bigy) || !bigbuffer)
	{
		glTexImage2D(GL_TEXTURE_2D,0,texformat3,x,y,0,texformat,texformat2,buffer);
	}
	else
	{
		switch(bpp)
		{
		case 8:
			ScaleNearest8(bigbuffer,buffer,bigx,bigy,x,y,pitch,bigpitch);
			break;
		case 16:
			ScaleNearest16(bigbuffer,buffer,bigx,bigy,x,y,pitch/2,bigpitch/2);
			break;
		case 24:
			ScaleNearest24(bigbuffer,buffer,bigx,bigy,x,y,pitch,bigpitch);
			break;
		case 32:
			ScaleNearest32(bigbuffer,buffer,bigx,bigy,x,y,pitch/4,bigpitch/4);
			break;
		break;
		}
		glTexImage2D(GL_TEXTURE_2D,0,texformat3,bigx,bigy,0,texformat,texformat2,bigbuffer);
	}
}

/**
  * Internal function for downloading surface content from an OpenGL texture
  * @param buffer
  *  Buffer to receive the surface contents
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.
  * @param texture
  *  OpenGL texture name to download from
  * @param x,y
  *  Width and height of the surface
  * @param bigx,bigy
  *  Width and height of the scaled surface buffer
  * @param pitch
  *  Bytes from one line of graphics to the next in the surface
  * @param bigpitch
  *  Pitch of the scaled surface buffer
  * @param bpp
  *  Number of bits per surface pixel
  * @param texformat
  *  OpenGL format parameter for glGetTexImage
  * @param texformat2
  *  OpenGL type parameter for glGetTexImage
  */
void glRenderer::_DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y,
	int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2)
{
	glBindTexture(GL_TEXTURE_2D,texture);  // Select surface's texture
	if((bigx == x && bigy == y) || !bigbuffer)
	{
		glGetTexImage(GL_TEXTURE_2D,0,texformat,texformat2,buffer); // Shortcut for no scaling
	}
	else
	{
		glGetTexImage(GL_TEXTURE_2D,0,texformat,texformat2,bigbuffer);
		switch(bpp)
		{
		case 8:
			ScaleNearest8(buffer,bigbuffer,x,y,bigx,bigy,bigpitch,pitch);
			break;
		case 15:
		case 16:
			ScaleNearest16(buffer,bigbuffer,x,y,bigx,bigy,bigpitch/2,pitch/2);
			break;
		case 24:
			ScaleNearest24(buffer,bigbuffer,x,y,bigx,bigy,bigpitch,pitch);
			break;
		case 32:
			ScaleNearest32(buffer,bigbuffer,x,y,bigx,bigy,bigpitch/4,pitch/4);
			break;
		break;
		}
	}
}

/**
  * Constructor for the glRenderer object
  * @param width,height,bpp
  *  Width, height, and BPP of the rendering window
  * @param fullscreen
  *  True if fullscreen mode is required, false for windowed
  * @param hwnd
  *  Handle of the window to render into.  If this value is NULL, then a transparent
  *  layered window will be created for the renderer.
  * @param glDD7
  *  Pointer to the glDirectDraw7 object that is managing the glRenderer object
  */
glRenderer::glRenderer(int width, int height, int bpp, bool fullscreen, HWND hwnd, glDirectDraw7 *glDD7)
{
	hDC = NULL;
	hRC = NULL;
	PBO = 0;
	dib.enabled = false;
	hWnd = hwnd;
	InitializeCriticalSection(&cs);
	busy = CreateEvent(NULL,FALSE,FALSE,NULL);
	start = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(fullscreen)
	{
		SetWindowLongPtrA(hWnd,GWL_EXSTYLE,WS_EX_APPWINDOW);
		SetWindowLongPtrA(hWnd,GWL_STYLE,WS_OVERLAPPED);
		ShowWindow(hWnd,SW_MAXIMIZE);
	}
	if(width)
	{
		// TODO:  Adjust window rect
	}
	SetWindowPos(hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	RenderWnd = new glRenderWindow(width,height,fullscreen,hWnd,glDD7);
	inputs[0] = (void*)width;
	inputs[1] = (void*)height;
	inputs[2] = (void*)bpp;
	inputs[3] = (void*)fullscreen;
	inputs[4] = (void*)hWnd;
	inputs[5] = glDD7;
	inputs[6] = this;
	hThread = CreateThread(NULL,0,ThreadEntry,inputs,0,NULL);
	WaitForSingleObject(busy,INFINITE);
}

/**
  * Destructor for the glRenderer object
  */
glRenderer::~glRenderer()
{
	EnterCriticalSection(&cs);
	opcode = OP_DELETE;
	SetEvent(start);
	WaitForObjectAndMessages(busy);
	CloseHandle(start);
	CloseHandle(busy);
	LeaveCriticalSection(&cs);
	DeleteCriticalSection(&cs);
	CloseHandle(hThread);
}

/**
  * Entry point for the renderer thread
  * @param entry
  *  Pointer to the inputs passed by the CreateThread function
  */
DWORD WINAPI glRenderer::ThreadEntry(void *entry)
{
	void **inputsin = (void**)entry;
	glRenderer *This = (glRenderer*)inputsin[6];
	return This->_Entry();
}

/**
  * Creates an OpenGL texture.  
  * @param min,mag
  *  Minification and magnification filters for the OpenGL texture
  * @param wraps,wrapt
  *  OpenGL texture wrap parameters
  * @param width,height
  *  Width and height of the texture.
  * @param texformat
  *  OpenGL format parameter for glTexImage2D
  * @param texformat2
  *  OpenGL type parameter for glTexImage2D
  * @param texformat3
  *  OpenGL internalformat parameter for glTexImage2D
  * @return
  *  Number representing the texture created by OpenGL.
  */
GLuint glRenderer::MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3)
{
	EnterCriticalSection(&cs);
	inputs[0] = (void*)min;
	inputs[1] = (void*)mag;
	inputs[2] = (void*)wraps;
	inputs[3] = (void*)wrapt;
	inputs[4] = (void*)width;
	inputs[5] = (void*)height;
	inputs[6] = (void*)texformat1;
	inputs[7] = (void*)texformat2;
	inputs[8] = (void*)texformat3;
	opcode = OP_CREATE;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
	return (GLuint)outputs[0];
}

/**
  * Uploads the content of a surface to an OpenGL texture.
  * @param buffer
  *  Contains the contents of the surface
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.
  * @param texture
  *  OpenGL texture name to upload to
  * @param x,y
  *  Width and height of the surface
  * @param bigx,bigy
  *  Width and height of the scaled surface buffer
  * @param pitch
  *  Bytes from one line of graphics to the next in the surface
  * @param bigpitch
  *  Pitch of the scaled surface buffer
  * @param bpp
  *  Number of bits per surface pixel
  * @param texformat
  *  OpenGL format parameter for glTexImage2D
  * @param texformat2
  *  OpenGL type parameter for glTexImage2D
  * @param texformat3
  *  OpenGL internalformat parameter for glTexImage2D
  */
void glRenderer::UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y,
	int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3)
{
	EnterCriticalSection(&cs);
	inputs[0] = buffer;
	inputs[1] = bigbuffer;
	inputs[2] = (void*)texture;
	inputs[3] = (void*)x;
	inputs[4] = (void*)y;
	inputs[5] = (void*)bigx;
	inputs[6] = (void*)bigy;
	inputs[7] = (void*)pitch;
	inputs[8] = (void*)bigpitch;
	inputs[9] = (void*)bpp;
	inputs[10] = (void*)texformat;
	inputs[11] = (void*)texformat2;
	inputs[12] = (void*)texformat3;
	opcode = OP_UPLOAD;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Downloads the contents of an OpenGL texture to a surface buffer.
  * @param buffer
  *  Buffer to receive the surface contents
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.
  * @param texture
  *  OpenGL texture name to download from
  * @param x,y
  *  Width and height of the surface
  * @param bigx,bigy
  *  Width and height of the scaled surface buffer
  * @param pitch
  *  Bytes from one line of graphics to the next in the surface
  * @param bigpitch
  *  Pitch of the scaled surface buffer
  * @param bpp
  *  Number of bits per surface pixel
  * @param texformat
  *  OpenGL format parameter for glGetTexImage
  * @param texformat2
  *  OpenGL type parameter for glGetTexImage
  */
void glRenderer::DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y,
	int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2)
{
	EnterCriticalSection(&cs);
	inputs[0] = buffer;
	inputs[1] = bigbuffer;
	inputs[2] = (void*)texture;
	inputs[3] = (void*)x;
	inputs[4] = (void*)y;
	inputs[5] = (void*)bigx;
	inputs[6] = (void*)bigy;
	inputs[7] = (void*)pitch;
	inputs[8] = (void*)bigpitch;
	inputs[9] = (void*)bpp;
	inputs[10] = (void*)texformat;
	inputs[11] = (void*)texformat2;
	opcode = OP_DOWNLOAD;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Deletes an OpenGL texture.
  * @param texture
  *  OpenGL texture to be deleted
  */
void glRenderer::DeleteTexture(GLuint texture)
{
	EnterCriticalSection(&cs);
	inputs[0] = (void*)texture;
	opcode = OP_DELETETEX;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Copies the contents of one surface to another.
  * @param lpDestRect
  *  Pointer to the coordinates to blit to.  If NULL, blits to the entire surface.
  * @param src
  *  Surface to be used as the source.
  * @param dest
  *  Surface to blit to.
  * @param lpSrcRect
  *  Pointer of the coordinates to blit from on the source surface.  If NULL, the
  *  entire surface will be used.
  * @param dwFlags
  *  Flags to determine the behavior of the blitter.  Certain flags control the
  *  synchronization of the operation: (not yet implemented)
  *  - DDBLT_ASYNC:  Adds the command to the queue.  If the queue is full, returns
  *    DDERR_WASSTILLDRAWING.
  *  - DDBLT_DONOTWAIT:  Fails and returns DDERR_WASSTILLDRAWING if the queue is full.
  *  - DDBLT_WAIT:  Waits until the Blt command is processed before returning.
  * @param lpDDBltFx
  *  Effect parameters for the Blt operation.
  * @return
  *  DD_OK if the call succeeds, or DDERR_WASSTILLDRAWING if busy.
  */
HRESULT glRenderer::Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	EnterCriticalSection(&cs);
	RECT r,r2;
	if(((dest->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(dest->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		GetClientRect(hWnd,&r);
		GetClientRect(RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	inputs[0] = lpDestRect;
	inputs[1] = src;
	inputs[2] = dest;
	inputs[3] = lpSrcRect;
	inputs[4] = (void*)dwFlags;
	inputs[5] = lpDDBltFx;
	opcode = OP_BLT;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
	return (HRESULT)outputs[0];
}

/**
  * Updates the display with the current primary texture.
  * @param texture
  *  Texture to use as the primary
  * @param paltex
  *  Texture that contains the color palette for 8-bit modes
  * @param dest
  *  Destination surface to be updated
  * @param src
  *  Source surface to be updated
  */
void glRenderer::DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src)
{
	EnterCriticalSection(&cs);
	inputs[0] = (void*)texture;
	inputs[1] = (void*)paltex;
	inputs[2] = dest;
	inputs[3] = src;
	opcode = OP_DRAWSCREEN;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Ensures the renderer is set up for handling Direct3D commands.
  * @param zbuffer
  *  Nonzero if a Z buffer is present.
  */
void glRenderer::InitD3D(int zbuffer)
{
	EnterCriticalSection(&cs);
	opcode = OP_INITD3D;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Clears the viewport.
  * @param target
  *  Surface to be cleared
  * @param dwCount
  *  Number of rects to use to clear the buffer, or 0 to clear the entire buffer.
  * @param lpRects
  *  Pointer to rects to clear.
  * @param dwFlags
  *  Flags to determine which surfaces to clear.
  * @param dwColor
  *  Color value to fill the surface with.
  * @param dvZ
  *  Value to fill the Z buffer with.
  * @param dwStencil
  *  Value to fill the stencil buffer with.
  * @return
  *  Returns D3D_OK
  */
HRESULT glRenderer::Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	EnterCriticalSection(&cs);
	inputs[0] = target;
	inputs[1] = (void*)dwCount;
	inputs[2] = lpRects;
	inputs[3] = (void*)dwFlags;
	inputs[4] = (void*)dwColor;
	memcpy(&inputs[5],&dvZ,4);
	inputs[6] = (void*)dwStencil;
	opcode = OP_CLEAR;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
	return (HRESULT)outputs[0];
}

/**
  * Instructs the OpenGL driver to send all queued commands to the GPU.
  */
void glRenderer::Flush()
{
	EnterCriticalSection(&cs);
	opcode = OP_FLUSH;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	LeaveCriticalSection(&cs);
}

/**
  * Changes the window used for rendering.
  * @param width,height
  *  Width and height of the new window.
  * @param fullscreen
  *  True if fullscreen
  * @param newwnd
  *  HWND of the new window
  */
void glRenderer::SetWnd(int width, int height, int bpp, int fullscreen, HWND newwnd)
{
	if(fullscreen && newwnd)
	{
		SetWindowLongPtrA(newwnd,GWL_EXSTYLE,WS_EX_APPWINDOW);
		SetWindowLongPtrA(newwnd,GWL_STYLE,WS_OVERLAPPED);
		ShowWindow(newwnd,SW_MAXIMIZE);
	}
	inputs[0] = (void*)width;
	inputs[1] = (void*)height;
	inputs[2] = (void*)bpp;
	inputs[3] = (void*)fullscreen;
	inputs[4] = (void*)newwnd;
	opcode = OP_SETWND;
	SetEvent(start);
	WaitForObjectAndMessages(busy);
	LeaveCriticalSection(&cs);
}
/**
  * Draws one or more primitives to the currently selected render target.
  * @param device
  *  glDirect3DDevice7 interface to use for drawing
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
HRESULT glRenderer::DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	EnterCriticalSection(&cs);
	inputs[0] = device;
	inputs[1] = (void*)mode;
	inputs[2] = vertices;
	inputs[3] = texformats;
	inputs[4] = (void*)count;
	inputs[5] = indices;
	inputs[6] = (void*)indexcount;
	inputs[7] = (void*)flags;
	opcode = OP_DRAWPRIMITIVES;
	SetEvent(start);
	WaitForSingleObject(busy,INFINITE);
	return (HRESULT)outputs[0];
}

/**
  * Main loop for glRenderer class
  * @return
  *  Returns 0 to signal successful thread termination
  */
DWORD glRenderer::_Entry()
{
	float tmpfloats[16];
	EnterCriticalSection(&cs);
	_InitGL((int)inputs[0],(int)inputs[1],(int)inputs[2],(int)inputs[3],(HWND)inputs[4],(glDirectDraw7*)inputs[5]);
	LeaveCriticalSection(&cs);
	SetEvent(busy);
	while(1)
	{
		WaitForSingleObject(start,INFINITE);
		switch(opcode)
		{
		case OP_DELETE:
			if(hRC)
			{
				if(dib.enabled)
				{
					if(dib.hbitmap)	DeleteObject(dib.hbitmap);
					if(dib.hdc)	DeleteDC(dib.hdc);
					ZeroMemory(&dib,sizeof(DIB));
				}
				DeleteShaders();
				DeleteFBO();
				if(PBO)
				{
					glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
					glDeleteBuffers(1,&PBO);
					PBO = 0;
				}
				if(backbuffer)
				{
					glDeleteTextures(1,&backbuffer);
					backbuffer = 0;
					backx = 0;
					backy = 0;
				}
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(hRC);
				hRC = NULL;
			};
			if(hDC) ReleaseDC(RenderWnd->GetHWnd(),hDC);
			hDC = NULL;
			delete RenderWnd;
			RenderWnd = NULL;
			SetEvent(busy);
			return 0;
			break;
		case OP_SETWND:
			_SetWnd((int)inputs[0],(int)inputs[1],(int)inputs[2],(int)inputs[3],(HWND)inputs[4]);
			break;
		case OP_CREATE:
			outputs[0] = (void*)_MakeTexture((GLint)inputs[0],(GLint)inputs[1],(GLint)inputs[2],(GLint)inputs[3],
				(DWORD)inputs[4],(DWORD)inputs[5],(GLint)inputs[6],(GLint)inputs[7],(GLint)inputs[8]);
			SetEvent(busy);
			break;
		case OP_UPLOAD:
			_UploadTexture((char*)inputs[0],(char*)inputs[1],(GLuint)inputs[2],(int)inputs[3],
				(int)inputs[4],(int)inputs[5],(int)inputs[6],(int)inputs[7],(int)inputs[8],(int)inputs[9],
				(int)inputs[10],(int)inputs[11],(int)inputs[12]);
			SetEvent(busy);
			break;
		case OP_DOWNLOAD:
			_DownloadTexture((char*)inputs[0],(char*)inputs[1],(GLuint)inputs[2],(int)inputs[3],
				(int)inputs[4],(int)inputs[5],(int)inputs[6],(int)inputs[7],(int)inputs[8],(int)inputs[9],
				(int)inputs[10],(int)inputs[11]);
			SetEvent(busy);
			break;
		case OP_DELETETEX:
			_DeleteTexture((GLuint)inputs[0]);
			break;
		case OP_BLT:
			_Blt((LPRECT)inputs[0],(glDirectDrawSurface7*)inputs[1],(glDirectDrawSurface7*)inputs[2],
				(LPRECT)inputs[3],(DWORD)inputs[4],(LPDDBLTFX)inputs[5]);
			break;
		case OP_DRAWSCREEN:
			_DrawScreen((GLuint)inputs[0],(GLuint)inputs[1],(glDirectDrawSurface7*)inputs[2],(glDirectDrawSurface7*)inputs[3],true);
			break;
		case OP_INITD3D:
			_InitD3D((int)inputs[0]);
			break;
		case OP_CLEAR:
			memcpy(&tmpfloats[0],&inputs[5],4);
			_Clear((glDirectDrawSurface7*)inputs[0],(DWORD)inputs[1],(LPD3DRECT)inputs[2],(DWORD)inputs[3],(DWORD)inputs[4],
				tmpfloats[0],(DWORD)inputs[6]);
			break;
		case OP_FLUSH:
			_Flush();
			break;
		case OP_DRAWPRIMITIVES:
			_DrawPrimitives((glDirect3DDevice7*)inputs[0],(GLenum)inputs[1],(GLVERTEX*)inputs[2],(int*)inputs[3],(DWORD)inputs[4],
				(LPWORD)inputs[5],(DWORD)inputs[6],(DWORD)inputs[7]);
			break;
		}
	}
	return 0;
}

/**
  * Creates a render window and initializes OpenGL.
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
BOOL glRenderer::_InitGL(int width, int height, int bpp, int fullscreen, HWND hWnd, glDirectDraw7 *glDD7)
{
	ddInterface = glDD7;
	if(hRC)
	{
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(hRC);
	};
	PIXELFORMATDESCRIPTOR pfd;
	GLuint pf;
	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = bpp;
	pfd.iLayerType = PFD_MAIN_PLANE;
	gllock = true;
	hDC = GetDC(RenderWnd->GetHWnd());
	if(!hDC)
	{
		DEBUG("glRenderer::InitGL: Can not create hDC\n");
		return FALSE;
	}
	pf = ChoosePixelFormat(hDC,&pfd);
	if(!pf)
	{
		DEBUG("glRenderer::InitGL: Can not get pixelformat\n");
		return FALSE;
	}
	if(!SetPixelFormat(hDC,pf,&pfd))
		DEBUG("glRenderer::InitGL: Can not set pixelformat\n");
	hRC = wglCreateContext(hDC);
	if(!hRC)
	{
		DEBUG("glRenderer::InitGL: Can not create GL context\n");
		gllock = false;
		return FALSE;
	}
	if(!wglMakeCurrent(hDC,hRC))
	{
		DEBUG("glRenderer::InitGL: Can not activate GL context\n");
		wglDeleteContext(hRC);
		hRC = NULL;
		ReleaseDC(RenderWnd->GetHWnd(),hDC);
		hDC = NULL;
		gllock = false;
		return FALSE;
	}
	gllock = false;
	InitGLExt();
	SetSwap(1);
	SetSwap(0);
	glViewport(0,0,width,height);
	glDisable(GL_DEPTH_TEST);
	SetDepthComp(GL_LESS);
	const GLubyte *glver = glGetString(GL_VERSION);
	gl_caps.Version = (GLfloat)atof((char*)glver);
	if(gl_caps.Version >= 2)
	{
		glver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		gl_caps.ShaderVer = (GLfloat)atof((char*)glver);
	}
	else gl_caps.ShaderVer = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&gl_caps.TextureMax);
	CompileShaders();
	InitFBO();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	SwapBuffers(hDC);
	SetActiveTexture(0);
	if(hWnd)
	{
		dib.enabled = true;
		dib.width = width;
		dib.height = height;
		dib.pitch = (((width<<3)+31)&~31) >>3;
		dib.pixels = NULL;
		dib.hdc = CreateCompatibleDC(NULL);
		ZeroMemory(&dib.info,sizeof(BITMAPINFO));
		dib.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		dib.info.bmiHeader.biBitCount = 32;
		dib.info.bmiHeader.biWidth = width;
		dib.info.bmiHeader.biHeight = height;
		dib.info.bmiHeader.biCompression = BI_RGB;
		dib.info.bmiHeader.biPlanes = 1;
		dib.hbitmap = CreateDIBSection(dib.hdc,&dib.info,DIB_RGB_COLORS,(void**)&dib.pixels,NULL,0);
	}
	glGenBuffers(1,&PBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER,PBO);
	glBufferData(GL_PIXEL_PACK_BUFFER,width*height*4,NULL,GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
	return TRUE;
}

void glRenderer::_Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
	glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	LONG sizes[6];
	ddInterface->GetSizes(sizes);
	int error;
	error = SetFBO(dest->texture,0,false);
	glViewport(0,0,dest->fakex,dest->fakey);
	RECT destrect;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	dest->GetSurfaceDesc(&ddsd);
	if(!lpDestRect)
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else destrect = *lpDestRect;
	RECT srcrect;
	DDSURFACEDESC2 ddsdSrc;
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	if(src) src->GetSurfaceDesc(&ddsdSrc);
	if(!lpSrcRect)
	{
		srcrect.left = 0;
		srcrect.top = 0;
		srcrect.right = ddsdSrc.dwWidth;
		srcrect.bottom = ddsdSrc.dwHeight;
	}
	else srcrect = *lpSrcRect;
	bltvertices[1].x = bltvertices[3].x = (GLfloat)destrect.left * ((GLfloat)dest->fakex/(GLfloat)ddsd.dwWidth);
	bltvertices[0].x = bltvertices[2].x = (GLfloat)destrect.right * ((GLfloat)dest->fakex/(GLfloat)ddsd.dwWidth);
	bltvertices[0].y = bltvertices[1].y = (GLfloat)dest->fakey-((GLfloat)destrect.top * ((GLfloat)dest->fakey/(GLfloat)ddsd.dwHeight));
	bltvertices[2].y = bltvertices[3].y = (GLfloat)dest->fakey-((GLfloat)destrect.bottom * ((GLfloat)dest->fakey/(GLfloat)ddsd.dwHeight));
	bltvertices[1].s = bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
	bltvertices[0].s = bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
	bltvertices[0].t = bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
	bltvertices[2].t = bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
	if(dest->zbuffer) glClear(GL_DEPTH_BUFFER_BIT);
	if(dwFlags & DDBLT_COLORFILL)
	{
		SetShader(PROG_FILL,NULL,NULL,true);
		switch(ddInterface->GetBPP())
		{
		case 8:
			bltvertices[0].r = bltvertices[0].g = bltvertices[0].b =
				bltvertices[1].r = bltvertices[1].g = bltvertices[1].b =
				bltvertices[2].r = bltvertices[2].g = bltvertices[2].b =
				bltvertices[3].r = bltvertices[3].g = bltvertices[3].b = (GLubyte)lpDDBltFx->dwFillColor;
			break;
		case 15:
			bltvertices[0].r = bltvertices[1].r = bltvertices[2].r = bltvertices[3].r =
				_5to8bit((lpDDBltFx->dwFillColor>>10) & 31);
			bltvertices[0].g = bltvertices[1].g = bltvertices[2].g = bltvertices[3].g =
				_5to8bit((lpDDBltFx->dwFillColor>>5) & 31);
			bltvertices[0].b = bltvertices[1].b = bltvertices[2].b = bltvertices[3].b =
				_5to8bit(lpDDBltFx->dwFillColor & 31);
			break;
		case 16:
			bltvertices[0].r = bltvertices[1].r = bltvertices[2].r = bltvertices[3].r =
				_5to8bit((lpDDBltFx->dwFillColor>>11) & 31);
			bltvertices[0].g = bltvertices[1].g = bltvertices[2].g = bltvertices[3].g =
				_6to8bit((lpDDBltFx->dwFillColor>>5) & 63);
			bltvertices[0].b = bltvertices[1].b = bltvertices[2].b = bltvertices[3].b =
				_5to8bit(lpDDBltFx->dwFillColor & 31);
			break;
		case 24:
		case 32:
			bltvertices[0].r = bltvertices[1].r = bltvertices[2].r = bltvertices[3].r =
				((lpDDBltFx->dwFillColor>>16) & 255);
			bltvertices[0].g = bltvertices[1].g = bltvertices[2].g = bltvertices[3].g =
				((lpDDBltFx->dwFillColor>>8) & 255);
			bltvertices[0].b = bltvertices[1].b = bltvertices[2].b = bltvertices[3].b =
				(lpDDBltFx->dwFillColor & 255);
		default:
			break;
		}
	}
	if((dwFlags & DDBLT_KEYSRC) && (src && src->colorkey[0].enabled) && !(dwFlags & DDBLT_COLORFILL))
	{
		SetShader(PROG_CKEY,NULL,NULL,true);
		GLint keyloc = glGetUniformLocation(shaders[PROG_CKEY].prog,"keyIn");
		switch(ddInterface->GetBPP())
		{
		case 8:
			glUniform3i(keyloc,src->colorkey[0].key.dwColorSpaceHighValue,src->colorkey[0].key.dwColorSpaceHighValue,
				src->colorkey[0].key.dwColorSpaceHighValue);
			break;
		case 15:
			glUniform3i(keyloc,_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>10 & 31),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>5 & 31),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue & 31));
			break;
		case 16:
			glUniform3i(keyloc,_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>11 & 31),
				_6to8bit(src->colorkey[0].key.dwColorSpaceHighValue>>5 & 63),
				_5to8bit(src->colorkey[0].key.dwColorSpaceHighValue & 31));
			break;
		case 24:
		case 32:
		default:
			glUniform3i(keyloc,(src->colorkey[0].key.dwColorSpaceHighValue>>16 & 255),
				(src->colorkey[0].key.dwColorSpaceHighValue>>8 & 255),
				(src->colorkey[0].key.dwColorSpaceHighValue & 255));
			break;
		}
		GLint texloc = glGetUniformLocation(shaders[PROG_CKEY].prog,"myTexture");
		glUniform1i(texloc,0);
	}
	else if(!(dwFlags & DDBLT_COLORFILL))
	{
		SetShader(PROG_TEXTURE,NULL,NULL,true);
		GLint texloc = glGetUniformLocation(shaders[PROG_TEXTURE].prog,"Texture");
		glUniform1i(texloc,0);
	}
	SetActiveTexture(0);
	if(src) glBindTexture(GL_TEXTURE_2D,src->GetTexture());
	GLuint prog = GetProgram()&0xffffffff;
	GLint viewloc = glGetUniformLocation(prog,"view");
	glUniform4f(viewloc,0,(GLfloat)dest->fakex,0,(GLfloat)dest->fakey);
	dest->dirty |= 2;
	GLint xyloc = glGetAttribLocation(prog,"xy");
	glEnableVertexAttribArray(xyloc);
	glVertexAttribPointer(xyloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].x);
	GLint rgbloc = glGetAttribLocation(prog,"rgb");
	if(rgbloc != -1)
	{
		glEnableVertexAttribArray(rgbloc);
		glVertexAttribPointer(rgbloc,3,GL_UNSIGNED_BYTE,true,sizeof(BltVertex),&bltvertices[0].r);
	}
	if(!(dwFlags & DDBLT_COLORFILL))
	{
		GLint stloc = glGetAttribLocation(prog,"st");
		glEnableVertexAttribArray(stloc);
		glVertexAttribPointer(stloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].s);
	}
	glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	SetFBO(0,0,false);
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))_DrawScreen(dest->texture,dest->paltex,dest,dest,false);
	outputs[0] = DD_OK;
	SetEvent(busy);
}

GLuint glRenderer::_MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3)
{
	GLuint texture;
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D,texture);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,(GLfloat)min);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,(GLfloat)mag);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,(GLfloat)wraps);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,(GLfloat)wrapt);
	glTexImage2D(GL_TEXTURE_2D,0,texformat3,width,height,0,texformat1,texformat2,NULL);
	return texture;
}

void glRenderer::_DrawBackbuffer(GLuint *texture, int x, int y)
{
	GLfloat view[4];
	SetActiveTexture(0);
	if(!backbuffer)
	{
		backbuffer = _MakeTexture(GL_LINEAR,GL_LINEAR,GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE,x,y,GL_BGRA,GL_UNSIGNED_BYTE,GL_RGBA8);
		backx = x;
		backy = y;
	}
	if((backx != x) || (backy != y))
	{
		glBindTexture(GL_TEXTURE_2D,backbuffer);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,x,y,0,GL_BGRA,GL_UNSIGNED_BYTE,NULL);
		backx = x;
		backy = y;
	}
	SetFBO(backbuffer,0,false);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)x;
	view[3] = (GLfloat)y;
	glViewport(0,0,x,y);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D,*texture);
	*texture = backbuffer;
	GLuint prog = GetProgram();
	GLint viewloc = glGetUniformLocation(prog,"view");
	glUniform4f(viewloc,view[0],view[1],view[2],view[3]);

	bltvertices[0].s = bltvertices[0].t = bltvertices[1].t = bltvertices[2].s = 1.;
	bltvertices[1].s = bltvertices[2].t = bltvertices[3].s = bltvertices[3].t = 0.;
	bltvertices[0].y = bltvertices[1].y = bltvertices[1].x = bltvertices[3].x = 0.;
	bltvertices[0].x = bltvertices[2].x = (float)x;
	bltvertices[2].y = bltvertices[3].y = (float)y;
	GLint xyloc = glGetAttribLocation(prog,"xy");
	glEnableVertexAttribArray(xyloc);
	glVertexAttribPointer(xyloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].x);
	GLint stloc = glGetAttribLocation(prog,"st");
	glEnableVertexAttribArray(stloc);
	glVertexAttribPointer(stloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].s);
	glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	SetFBO(0,0,false);
}

void glRenderer::_DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, bool setsync)
{
	RECT r,r2;
	if((dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		GetClientRect(hWnd,&r);
		GetClientRect(RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	RECT *viewrect = &r2;
	SetSwap(swapinterval);
	LONG sizes[6];
	GLfloat view[4];
	GLint viewport[4];
	if(src->dirty & 1)
	{
		_UploadTexture(src->buffer,src->bigbuffer,texture,src->ddsd.dwWidth,src->ddsd.dwHeight,
			src->fakex,src->fakey,src->ddsd.lPitch,
			(NextMultipleOf4((ddInterface->GetBPPMultipleOf8()/8)*src->fakex)),
			src->ddsd.ddpfPixelFormat.dwRGBBitCount,src->texformat,src->texformat2,src->texformat3);
		src->dirty &= ~1;
	}
	if(dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(ddInterface->GetFullscreen())
		{
			ddInterface->GetSizes(sizes);
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
			viewport[0] = viewport[1] = 0;
			viewport[2] = viewrect->right;
			viewport[3] = viewrect->bottom;
			ClientToScreen(RenderWnd->GetHWnd(),(LPPOINT)&viewrect->left);
			ClientToScreen(RenderWnd->GetHWnd(),(LPPOINT)&viewrect->right);
			view[0] = (GLfloat)viewrect->left;
			view[1] = (GLfloat)viewrect->right;
			view[2] = (GLfloat)dest->fakey-(GLfloat)viewrect->top;
			view[3] = (GLfloat)dest->fakey-(GLfloat)viewrect->bottom;
		}
	}
	else
	{
		view[0] = 0;
		view[1] = (GLfloat)dest->fakex;
		view[2] = 0;
		view[3] = (GLfloat)dest->fakey;
	}
	SetFBO(0,0,false);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(ddInterface->GetBPP() == 8)
	{
		SetShader(PROG_PAL256,NULL,NULL,true);
		glBindTexture(GL_TEXTURE_2D,paltex);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,256,1,0,GL_RGBA,GL_UNSIGNED_BYTE,dest->palette->GetPalette(NULL));
		GLint palloc = glGetUniformLocation(shaders[PROG_PAL256].prog,"ColorTable");
		GLint texloc = glGetUniformLocation(shaders[PROG_PAL256].prog,"IndexTexture");
		glUniform1i(texloc,0);
		glUniform1i(palloc,1);
		SetActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D,texture);
		SetActiveTexture(1);
		glBindTexture(GL_TEXTURE_2D,paltex);
		SetActiveTexture(0);
		if(dxglcfg.scalingfilter)
		{
			_DrawBackbuffer(&texture,dest->fakex,dest->fakey);
			SetShader(PROG_TEXTURE,NULL,NULL,true);
			glBindTexture(GL_TEXTURE_2D,texture);
			GLuint prog = GetProgram() & 0xFFFFFFFF;
			GLint texloc = glGetUniformLocation(prog,"Texture");
			glUniform1i(texloc,0);
		}
	}
	else
	{
		SetShader(PROG_TEXTURE,NULL,NULL,true);
		glBindTexture(GL_TEXTURE_2D,texture);
		GLuint prog = GetProgram() & 0xFFFFFFFF;
		GLint texloc = glGetUniformLocation(prog,"Texture");
		glUniform1i(texloc,0);
	}
	glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
	GLuint prog = GetProgram();
	GLint viewloc = glGetUniformLocation(prog,"view");
	glUniform4f(viewloc,view[0],view[1],view[2],view[3]);
	if(ddInterface->GetFullscreen())
	{
		bltvertices[0].x = bltvertices[2].x = (float)sizes[0];
		bltvertices[0].y = bltvertices[1].y = bltvertices[1].x = bltvertices[3].x = 0.;
		bltvertices[2].y = bltvertices[3].y = (float)sizes[1];
	}
	else
	{
		bltvertices[0].x = bltvertices[2].x = (float)dest->fakex;
		bltvertices[0].y = bltvertices[1].y = bltvertices[1].x = bltvertices[3].x = 0.;
		bltvertices[2].y = bltvertices[3].y = (float)dest->fakey;
	}
	bltvertices[0].s = bltvertices[0].t = bltvertices[1].t = bltvertices[2].s = 1.;
	bltvertices[1].s = bltvertices[2].t = bltvertices[3].s = bltvertices[3].t = 0.;
	GLint xyloc = glGetAttribLocation(prog,"xy");
	glEnableVertexAttribArray(xyloc);
	glVertexAttribPointer(xyloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].x);
	GLint stloc = glGetAttribLocation(prog,"st");
	glEnableVertexAttribArray(stloc);
	glVertexAttribPointer(stloc,2,GL_FLOAT,false,sizeof(BltVertex),&bltvertices[0].s);
	GLint rgbloc = glGetAttribLocation(prog,"rgb");
	if(rgbloc != -1)
	{
		glEnableVertexAttribArray(rgbloc);
		glVertexAttribPointer(rgbloc,3,GL_UNSIGNED_BYTE,true,sizeof(BltVertex),&bltvertices[0].r);
	}
	glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glFlush();
	if(hWnd) SwapBuffers(hDC);
	else
	{
		glReadBuffer(GL_FRONT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER,PBO);
		GLint packalign;
		glGetIntegerv(GL_PACK_ALIGNMENT,&packalign);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		glReadPixels(0,0,sizes[4],sizes[5],GL_BGRA,GL_UNSIGNED_BYTE,0);
		GLubyte *pixels = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER,GL_READ_ONLY);
		for(int i = 0; i < sizes[5];i++)
		{
			memcpy(&dib.pixels[dib.pitch*i],
				&pixels[((sizes[5]-1)-i)*(sizes[4]*4)],sizes[4]*4);
		}
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
		glPixelStorei(GL_PACK_ALIGNMENT,packalign);
		HDC hRenderDC = (HDC)::GetDC(RenderWnd->GetHWnd());
		HGDIOBJ hPrevObj = 0;
		POINT dest = {0,0};
		POINT src = {0,0};
		SIZE wnd = {dib.width,dib.height};
		BLENDFUNCTION func = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		hPrevObj = SelectObject(dib.hdc,dib.hbitmap);
		ClientToScreen(RenderWnd->GetHWnd(),&dest);
		UpdateLayeredWindow(RenderWnd->GetHWnd(),hRenderDC,&dest,&wnd,
			dib.hdc,&src,0,&func,ULW_ALPHA);
		SelectObject(dib.hdc,hPrevObj);
		ReleaseDC(RenderWnd->GetHWnd(),hRenderDC);
	}
	if(setsync) SetEvent(busy);

}

void glRenderer::_DeleteTexture(GLuint texture)
{
	glDeleteTextures(1,&texture);
	SetEvent(busy);
}

void glRenderer::_InitD3D(int zbuffer)
{
	SetEvent(busy);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	if(zbuffer) glEnable(GL_DEPTH_TEST);
	SetDepthComp(GL_LEQUAL);
	glDisable(GL_DITHER);
}

void glRenderer::_Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	if(dwCount)
	{
		outputs[0] = (void*)DDERR_INVALIDPARAMS;
		FIXME("glDirect3DDevice7::Clear:  Cannot clear rects yet.");
		SetEvent(busy);
		return;
	}
	outputs[0] = (void*)D3D_OK;
	GLfloat color[4];
	dwordto4float(dwColor,color);
	if(target->zbuffer) SetFBO(target->texture,target->GetZBuffer()->texture,target->GetZBuffer()->hasstencil);
	else SetFBO(target->texture,0,false);
	int clearbits = 0;
	if(D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		glClearColor(color[0],color[1],color[2],color[3]);
	}
	if(D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		glClearDepth(dvZ);
	}
	if(D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		glClearStencil(dwStencil);
	}
	glClear(clearbits);
	if(target->zbuffer) target->zbuffer->dirty |= 2;
	target->dirty |= 2;
	SetEvent(busy);
}

void glRenderer::_Flush()
{
	glFlush();
	SetEvent(busy);
}

void glRenderer::_SetWnd(int width, int height, int bpp, int fullscreen, HWND newwnd)
{
	if(newwnd != hWnd)
	{
		wglMakeCurrent(NULL,NULL);
		ReleaseDC(hWnd,hDC);
		delete RenderWnd;
		RenderWnd = new glRenderWindow(width,height,fullscreen,newwnd,ddInterface);
		PIXELFORMATDESCRIPTOR pfd;
		GLuint pf;
		gllock = true;
		ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = bpp;
		pfd.iLayerType = PFD_MAIN_PLANE;
		hDC = GetDC(RenderWnd->GetHWnd());
		if(!hDC)
			DEBUG("glRenderer::SetWnd: Can not create hDC\n");
		pf = ChoosePixelFormat(hDC,&pfd);
		if(!pf)
			DEBUG("glRenderer::SetWnd: Can not get pixelformat\n");
		if(!SetPixelFormat(hDC,pf,&pfd))
			DEBUG("glRenderer::SetWnd: Can not set pixelformat\n");
		if(!wglMakeCurrent(hDC,hRC))
			DEBUG("glRenderer::SetWnd: Can not activate GL context\n");
		gllock = false;
		SetSwap(1);
		SetSwap(0);
		glViewport(0,0,width,height);
	}

	SetEvent(busy);
}

void glRenderer::_DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	bool transformed;
	char svar[] = "sX";
	char stvar[] = "stX";
	char strvar[] = "strX";
	char strqvar[] = "strqX";
	int i;
	if(vertices[1].data) transformed = true;
	else transformed = false;
	if(!vertices[0].data)
	{
		outputs[0] = (void*)DDERR_INVALIDPARAMS;
		SetEvent(busy);
		return;
	}
	__int64 shader = device->SelectShader(vertices);
	SetShader(shader,device->texstages,texformats,0);
	device->SetDepthComp();
	_GENSHADER prog = genshaders[current_genshader].shader;
	glEnableVertexAttribArray(prog.attribs[0]);
	glVertexAttribPointer(prog.attribs[0],3,GL_FLOAT,false,vertices[0].stride,vertices[0].data);
	if(transformed)
	{
		if(prog.attribs[1] != -1)
		{
			glEnableVertexAttribArray(prog.attribs[1]);
			glVertexAttribPointer(prog.attribs[1],4,GL_FLOAT,false,vertices[1].stride,vertices[1].data);
		}
	}
	for(i = 0; i < 5; i++)
	{
		if(vertices[i+2].data)
		{
			if(prog.attribs[i+2] != -1)
			{
				glEnableVertexAttribArray(prog.attribs[i+2]);
				glVertexAttribPointer(prog.attribs[i+2],1,GL_FLOAT,false,vertices[i+2].stride,vertices[i+2].data);
			}
		}
	}
	if(vertices[7].data)
	{
		if(prog.attribs[7] != -1)
		{
			glEnableVertexAttribArray(prog.attribs[7]);
			glVertexAttribPointer(prog.attribs[7],3,GL_FLOAT,false,vertices[7].stride,vertices[7].data);
		}
	}
	for(i = 0; i < 2; i++)
	{
		if(vertices[i+8].data)
		{
			if(prog.attribs[8+i] != -1)
			{
				glEnableVertexAttribArray(prog.attribs[8+i]);
				glVertexAttribPointer(prog.attribs[8+i],4,GL_UNSIGNED_BYTE,true,vertices[i+8].stride,vertices[i+8].data);
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
			case 0: // st
				if(prog.attribs[i+18] != -1)
				{
					glEnableVertexAttribArray(prog.attribs[i+18]);
					glVertexAttribPointer(prog.attribs[i+18],2,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 1: // str
				if(prog.attribs[i+26] != -1)
				{
					glEnableVertexAttribArray(prog.attribs[i+26]);
					glVertexAttribPointer(prog.attribs[i+26],3,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 2: // strq
				if(prog.attribs[i+34] != -1)
				{
					glEnableVertexAttribArray(prog.attribs[i+34]);
					glVertexAttribPointer(prog.attribs[i+34],4,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			case 3: // s
				if(prog.attribs[i+10] != -1)
				{
					glEnableVertexAttribArray(prog.attribs[i+10]);
					glVertexAttribPointer(prog.attribs[i+10],1,GL_FLOAT,false,vertices[i+10].stride,vertices[i+10].data);
				}
				break;
			}

		}
	}
	if(device->normal_dirty) device->UpdateNormalMatrix();
	if(prog.uniforms[0] != -1) glUniformMatrix4fv(prog.uniforms[0],1,false,device->matWorld);
	if(prog.uniforms[1] != -1) glUniformMatrix4fv(prog.uniforms[1],1,false,device->matView);
	if(prog.uniforms[2] != -1) glUniformMatrix4fv(prog.uniforms[2],1,false,device->matProjection);
	if(prog.uniforms[3] != -1) glUniformMatrix3fv(prog.uniforms[3],1,true,device->matNormal);

	if(prog.uniforms[15] != -1) glUniform4fv(prog.uniforms[15],1,(GLfloat*)&device->material.ambient);
	if(prog.uniforms[16] != -1) glUniform4fv(prog.uniforms[16],1,(GLfloat*)&device->material.diffuse);
	if(prog.uniforms[17] != -1) glUniform4fv(prog.uniforms[17],1,(GLfloat*)&device->material.specular);
	if(prog.uniforms[18] != -1) glUniform4fv(prog.uniforms[18],1,(GLfloat*)&device->material.emissive);
	if(prog.uniforms[19] != -1) glUniform1f(prog.uniforms[19],device->material.power);
	int lightindex = 0;
	char lightname[] = "lightX.xxxxxxxxxxxxxxxx";
	for(i = 0; i < 8; i++)
	{
		if(device->gllights[i] != -1)
		{
			if(prog.uniforms[20+(i*12)] != -1)
				glUniform4fv(prog.uniforms[20+(i*12)],1,(GLfloat*)&device->lights[device->gllights[i]]->light.dcvDiffuse);
			if(prog.uniforms[21+(i*12)] != -1)
				glUniform4fv(prog.uniforms[21+(i*12)],1,(GLfloat*)&device->lights[device->gllights[i]]->light.dcvSpecular);
			if(prog.uniforms[22+(i*12)] != -1)
				glUniform4fv(prog.uniforms[22+(i*12)],1,(GLfloat*)&device->lights[device->gllights[i]]->light.dcvAmbient);
			if(prog.uniforms[24+(i*12)] != -1)
				glUniform3fv(prog.uniforms[24+(i*12)],1,(GLfloat*)&device->lights[device->gllights[i]]->light.dvDirection);
			if(device->lights[device->gllights[i]]->light.dltType != D3DLIGHT_DIRECTIONAL)
			{
				if(prog.uniforms[23+(i*12)] != -1)
					glUniform3fv(prog.uniforms[23+(i*12)],1,(GLfloat*)&device->lights[device->gllights[i]]->light.dvPosition);
				if(prog.uniforms[25+(i*12)] != -1)
					glUniform1f(prog.uniforms[25+(i*12)],device->lights[device->gllights[i]]->light.dvRange);
				if(prog.uniforms[26+(i*12)] != -1)
					glUniform1f(prog.uniforms[26+(i*12)],device->lights[device->gllights[i]]->light.dvFalloff);
				if(prog.uniforms[27+(i*12)] != -1)
					glUniform1f(prog.uniforms[27+(i*12)],device->lights[device->gllights[i]]->light.dvAttenuation0);
				if(prog.uniforms[28+(i*12)] != -1)
					glUniform1f(prog.uniforms[28+(i*12)],device->lights[device->gllights[i]]->light.dvAttenuation1);
				if(prog.uniforms[29+(i*12)] != -1)
					glUniform1f(prog.uniforms[29+(i*12)],device->lights[device->gllights[i]]->light.dvAttenuation2);
				if(prog.uniforms[30+(i*12)] != -1)
					glUniform1f(prog.uniforms[30+(i*12)],device->lights[device->gllights[i]]->light.dvTheta);
				if(prog.uniforms[31+(i*12)] != -1)
					glUniform1f(prog.uniforms[31+(i*12)],device->lights[device->gllights[i]]->light.dvPhi);
			}
		}
		lightindex++;
	}

	DWORD ambient = device->renderstate[D3DRENDERSTATE_AMBIENT];
	if(prog.uniforms[136] != -1)
		glUniform4f(prog.uniforms[136],RGBA_GETRED(ambient),RGBA_GETGREEN(ambient),
			RGBA_GETBLUE(ambient),RGBA_GETALPHA(ambient));

	for(i = 0; i < 8; i++)
	{
		if(device->texstages[i].colorop == D3DTOP_DISABLE) break;
		if(device->texstages[i].texture)
		{
			if(device->texstages[i].texture->dirty & 1)
			{
				_UploadTexture(device->texstages[i].texture->buffer,device->texstages[i].texture->bigbuffer,
					device->texstages[i].texture->texture,device->texstages[i].texture->ddsd.dwWidth,
					device->texstages[i].texture->ddsd.dwHeight,device->texstages[i].texture->fakex,
					device->texstages[i].texture->fakey,device->texstages[i].texture->ddsd.lPitch,
					(device->texstages[i].texture->ddsd.ddpfPixelFormat.dwRGBBitCount/8*device->texstages[i].texture->fakex),
					device->texstages[i].texture->ddsd.ddpfPixelFormat.dwRGBBitCount,device->texstages[i].texture->texformat,
					device->texstages[i].texture->texformat2,device->texstages[i].texture->texformat3);
				device->texstages[i].texture->dirty &= ~1;
			}
			if(device->texstages[i].texture)
				device->texstages[i].texture->SetFilter(i,device->texstages[i].glmagfilter,device->texstages[i].glminfilter);
			SetTexture(i,device->texstages[i].texture->texture);
		}
		else SetTexture(i,0);
		glUniform1i(prog.uniforms[128+i],i);
	}

	if(device->glDDS7->zbuffer) SetFBO(device->glDDS7->texture,device->glDDS7->zbuffer->texture,device->glDDS7->zbuffer->hasstencil);
	else SetFBO(device->glDDS7->texture,0,false);
	glViewport(device->viewport.dwX,device->viewport.dwY,device->viewport.dwWidth,device->viewport.dwHeight);
	if(indices) glDrawElements(mode,indexcount,GL_UNSIGNED_SHORT,indices);
	else glDrawArrays(mode,0,count);
	if(device->glDDS7->zbuffer) device->glDDS7->zbuffer->dirty |= 2;
	device->glDDS7->dirty |= 2;
	if(flags & D3DDP_WAIT) glFlush();
	outputs[0] = (void*)D3D_OK;
	SetEvent(busy);
	return;
}