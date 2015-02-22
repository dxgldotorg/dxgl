// DXGL
// Copyright (C) 2012-2015 William Feely

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
#include "TextureManager.h"
#include "glUtil.h"
#include "timer.h"
#include "glDirectDraw.h"
#include "glDirectDrawSurface.h"
#include "glDirectDrawPalette.h"
#include "glRenderWindow.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "glDirect3DLight.h"
#include "glDirectDrawClipper.h"
#include "ddraw.h"
#include "scalers.h"
#include "ShaderGen3D.h"
#include "matrix.h"

extern "C" {

const GLushort bltindices[4] = {0,1,2,3};
const RECT nullrect = { -1, -1, -1, -1 };

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
inline void glRenderer__SetSwap(glRenderer *This, int swap)
{
	if(swap != This->oldswap)
	{
		This->ext->wglSwapIntervalEXT(swap);
		This->oldswap = This->ext->wglGetSwapIntervalEXT();
		This->oldswap = swap;
	}
}

/**
* Adds a command to the renderer queue.\n
* If the command requires more space than the queue buffer, the buffer will be
* expanded.  If there is no free space for the command, execution will pause
* until the queue has been sufficiently emptied.
* @param This
*  Pointer to glRenderer object
* @param opcode
*  Code that describes the command to be added to the queue.
* @param mode
*  Method to use for synchronization:
*  - 0:  Do not fail the call
*  - 1:  Fail if queue is full
*  - 2:  Fail if queue is not empty
* @param size
*  Size of the command in DWORDs
* @param paramcount
*  Number of parameters to add to the queue command.
* @param ...
*  Parameters for the command, when required.  This is given in pairs of two
*  arguments:
*  - size:  The size of the parameter, in bytes.  Note that parameters are DWORD
*    aligned.  The size cannot exceed the size of the command, minus data already
*    written to the queue.
*  - pointer:  A pointer to the data to be added to the command.\n
*  If no parameters are used for the opcode, then supply 0 and NULL for the ...
*  parameter.
* @return
*  Zero if the call succeeds, nonzero otherwise.
*/
int glRenderer_AddQueue(glRenderer *This, DWORD opcode, int mode, DWORD size, int paramcount, ...)
{
	EnterCriticalSection(&This->queuecs);
	if ((mode == 2) && This->queuelength)
	{
		LeaveCriticalSection(&This->queuecs);
		return 1;
	}
	va_list params;
	// Check queue size
	va_start(params, paramcount);
	int argsize;
	void *argptr;
	if (size > This->queuesize)
	{
		This->queue = (LPDWORD)realloc(This->queue, (This->queuesize + size)*sizeof(DWORD));
		This->queuesize += size;
	}
	if (This->queuesize - This->queue_write < size)
	{
		if (This->queue_read < size)
		{
			if (mode == 1)
			{
				LeaveCriticalSection(&This->queuecs);
				return 1;
			}
			if (This->queue_write < This->queuesize)
			{
				This->queue[This->queue_write] = OP_RESETQUEUE;
				This->queuelength++;
			}
			LeaveCriticalSection(&This->queuecs);
			glRenderer_Sync(This, size);
			EnterCriticalSection(&This->queuecs);
		}
	}
	if (This->queue_write < This->queue_read)
	{
		if (This->queue_read - This->queue_write < size)
		{
			LeaveCriticalSection(&This->queuecs);
			glRenderer_Sync(This, size);
			EnterCriticalSection(&This->queuecs);
		}
	}
	This->queue[This->queue_write++] = opcode;
	This->queue[This->queue_write++] = size;
	size -= 2;
	for (int i = 0; i < paramcount; i++)
	{
		argsize = va_arg(params, int);
		argptr = va_arg(params, void*);
		if (!argsize)
		{
			This->queue[This->queue_write++] = argsize;
			size -= 1;
			continue;
		}
		if ((NextMultipleOf4(argsize) / 4) > size) break;
		This->queue[This->queue_write++] = argsize;
		if (argptr) memcpy(This->queue + This->queue_write, argptr, argsize);
		This->queue_write += (NextMultipleOf4(argsize) / 4);
		size -= 1 + (NextMultipleOf4(argsize) / 4);
	}
	va_end(params);
	if (size != 0)
	{
		FIXME("Opcode size mismatch!\n")
	}
	This->queuelength++;
	if (!This->running) SetEvent(This->start);
	LeaveCriticalSection(&This->queuecs);
	return 0;
}

/**
* Waits until the specified amount of queue space is free
* @param This
*  Pointer to glRenderer object
* @param size
*  If nonzero, the number of DWORDs that must be available within the queue.
*  If zero, waits until the queue is empty.
*/
void glRenderer_Sync(glRenderer *This, int size)
{
	EnterCriticalSection(&This->queuecs);
	if (!This->queuelength && !This->running)
	{
		LeaveCriticalSection(&This->queuecs);
		return;
	}
	ResetEvent(This->sync);
	This->syncsize = size;
	if (!This->running) SetEvent(This->start);
	LeaveCriticalSection(&This->queuecs);
	WaitForSingleObject(This->sync, INFINITE);
}

/**
  * Internal function for uploading surface content to an OpenGL texture
  * @param This
  *  Pointer to glRenderer object
  * @param buffer
  *  Contains the contents of the surface
  * @param texture
  *  Texture object to upload to
  * @param x,y
  *  Width and height of the surface
  * @param pitch
  *  Bytes from one line of graphics to the next in the surface
  * @param miplevel
  *  Mipmap level of texture to write
  */
void glRenderer__UploadTexture(glRenderer *This, BYTE *buffer, TEXTURE *texture,
	DWORD x, DWORD y, DWORD pitch, DWORD miplevel)
{
	TextureManager__UploadTexture(This->texman, texture, miplevel, buffer, x, y, FALSE, FALSE);
}

/**
* Uploads the content of a surface to an OpenGL texture, called from the backend.
* @param This
*  Pointer to glRenderer object
* @param buffer
*  Contains the contents of the surface
* @param bigbuffer
*  Optional buffer to receive the rescaled surface contents, for when primary
*  scaling is enabled.  Scaling will be performed before the surface is uploaded.
* @param texture
*  Texture object to upload to
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
* @param miplevel
*  Mipmap level of texture to write
*/
void glRenderer__UploadTextureSurface(glRenderer *This, BYTE *buffer, BYTE *bigbuffer, TEXTURE *texture, DWORD x, DWORD y, DWORD bigx, DWORD bigy, DWORD pitch, DWORD bigpitch, DWORD bpp, DWORD miplevel)
{
	BYTE *outbuffer;
	DWORD outx, outy, outpitch, outbpp;
	if (bpp == 15) outbpp = 16;
	else outbpp = bpp;
	if ((x == bigx && y == bigy) || !bigbuffer)
	{
		outbuffer = buffer;
		outx = x;
		outy = y;
		outpitch = pitch;
	}
	else
	{
		switch (outbpp)
		{
		case 8:
			ScaleNearest8(bigbuffer, buffer, bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 16:
			ScaleNearest16(bigbuffer, buffer, bigx, bigy, x, y, pitch / 2, bigpitch / 2);
			break;
		case 24:
			ScaleNearest24(bigbuffer, buffer, bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 32:
			ScaleNearest32(bigbuffer, buffer, bigx, bigy, x, y, pitch / 4, bigpitch / 4);
			break;
			break;
		}
		outbuffer = bigbuffer;
		outx = bigx;
		outy = bigy;
		outpitch = bigpitch;
	}
	glRenderer__UploadTexture(This, outbuffer, texture, outx, outy, outpitch, miplevel);
}

/**
  * Internal function for downloading surface content from an OpenGL texture.
  * Forwards the request to the Texture Manager.
  * @param This
  *  Pointer to glRenderer object
  * @param buffer
  *  Buffer to receive the surface contents
  * @param texture
  *  Texture object to download from
  * @param miplevel
  *  Mipmap level of texture to read
  */
void glRenderer__DownloadTexture(glRenderer *This, BYTE *buffer, TEXTURE *texture, DWORD miplevel)
{
	TextureManager__DownloadTexture(This->texman,texture,miplevel,buffer);
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
void glRenderer_Init(glRenderer *This, DWORD width, DWORD height, DWORD bpp, BOOL fullscreen, DWORD frequency, HWND hwnd, glDirectDraw7 *glDD7, BOOL devwnd)
{
	LONG_PTR winstyle, winstyleex;
	This->oldswap = 0;
	This->fogcolor = 0;
	This->fogstart = 0.0f;
	This->fogend = 1.0f;
	This->fogdensity = 1.0f;
	This->backbuffer = NULL;
	This->hDC = NULL;
	This->hRC = NULL;
	This->PBO = 0;
	This->dib.enabled = false;
	This->running = FALSE;
	This->hWnd = hwnd;
	This->busy = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&This->commandcs);
	InitializeCriticalSection(&This->queuecs);
	if (fullscreen)
	{
		winstyle = GetWindowLongPtrA(This->hWnd, GWL_STYLE);
		winstyleex = GetWindowLongPtrA(This->hWnd, GWL_EXSTYLE);
		SetWindowLongPtrA(This->hWnd, GWL_EXSTYLE, winstyleex & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE));
		SetWindowLongPtrA(This->hWnd, GWL_STYLE, (winstyle | WS_POPUP | WS_SYSMENU) & ~(WS_CAPTION | WS_THICKFRAME));
		ShowWindow(This->hWnd,SW_MAXIMIZE);
	}
	if(width)
	{
		// TODO:  Adjust window rect
	}
	EnterCriticalSection(&This->commandcs);
	SetWindowPos(This->hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	This->RenderWnd = new glRenderWindow(width,height,fullscreen,This->hWnd,glDD7,devwnd);
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)This->hWnd;
	This->inputs[6] = glDD7;
	This->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)glRenderer__Entry, This, 0, NULL);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Deletes a glRenderer object
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Delete(glRenderer *This)
{
	MSG Msg;
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_DELETE, 0, 2, 0);
	LeaveCriticalSection(&This->commandcs);
	WaitForObjectAndMessages(This->hThread);
	DeleteCriticalSection(&This->commandcs);
	CloseHandle(This->busy);
	CloseHandle(This->sync);
	CloseHandle(This->start);
	CloseHandle(This->hThread);
}

/**
  * Creates an OpenGL texture.  
  * @param This
  *  Pointer to glRenderer object
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
void glRenderer_MakeTexture(glRenderer *This, TEXTURE *texture, DWORD width, DWORD height)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_CREATE, 0, 8, 3, 4, &texture, 4, &width, 4, &height);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Uploads the content of a surface to an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param buffer
  *  Contains the contents of the surface
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.  Scaling will be performed before the surface is uploaded.
  * @param texture
  *  Texture object to upload to
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
  * @param miplevel
  *  Mipmap level of texture to write
  */
void glRenderer_UploadTexture(glRenderer *This, BYTE *buffer, BYTE *bigbuffer, TEXTURE *texture, DWORD x, DWORD y,
	DWORD bigx, DWORD bigy, DWORD pitch, DWORD bigpitch, DWORD bpp, DWORD miplevel)
{
	BYTE *outbuffer;
	DWORD outx, outy, outpitch, outbpp;
	DWORD *opbuffer;
	DWORD uploadopsize;
	if (bpp == 15) outbpp = 16;
	else outbpp = bpp;
	if ((x == bigx && y == bigy) || !bigbuffer)
	{
		outbuffer = buffer;
		outx = x;
		outy = y;
		outpitch = pitch;
	}
	else
	{
		switch (outbpp)
		{
		case 8:
			ScaleNearest8(bigbuffer, buffer, bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 16:
			ScaleNearest16(bigbuffer, buffer, bigx, bigy, x, y, pitch / 2, bigpitch / 2);
			break;
		case 24:
			ScaleNearest24(bigbuffer, buffer, bigx, bigy, x, y, pitch, bigpitch);
			break;
		case 32:
			ScaleNearest32(bigbuffer, buffer, bigx, bigy, x, y, pitch / 4, bigpitch / 4);
			break;
			break;
		}
		outbuffer = bigbuffer;
		outx = bigx;
		outy = bigy;
		outpitch = bigpitch;
	}
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_UPLOAD, 0, 14, 6, 4, &outbuffer, 4, &texture, 4, &outx, 4, &outy, 
		4, &outpitch, 4, &miplevel);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Downloads the contents of an OpenGL texture to a surface buffer.
  * @param This
  *  Pointer to glRenderer object
  * @param buffer
  *  Buffer to receive the surface contents
  * @param bigbuffer
  *  Optional buffer to receive the rescaled surface contents, for when primary
  *  scaling is enabled.  Scaling will be performed after the download is complete.
  * @param texture
  *  Texture object to download from
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
  * @param miplevel
  *  Mipmap level of texture to read
  */
void glRenderer_DownloadTexture(glRenderer *This, BYTE *buffer, BYTE *bigbuffer, TEXTURE *texture, DWORD x, DWORD y,
	DWORD bigx, DWORD bigy, DWORD pitch, DWORD bigpitch, DWORD bpp, DWORD miplevel)
{
	BOOL doscale;
	BYTE *inbuffer;
	DWORD inx, iny, inpitch;
	if ((x == bigx && y == bigy) || !bigbuffer)
	{
		doscale = FALSE;
		inbuffer = buffer;
		inx = x;
		iny = y;
		inpitch = pitch;
	}
	else
	{
		doscale = TRUE;
		inbuffer = bigbuffer;
		inx = bigx;
		iny = bigy;
		inpitch = bigpitch;
	}
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_DOWNLOAD, 0, 8, 3, 4, &inbuffer, 4, &texture, 4, &miplevel);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
	if (doscale)
	{
		switch (bpp)
		{
		case 8:
			ScaleNearest8(buffer, bigbuffer, x, y, bigx, bigy, bigpitch, pitch);
			break;
		case 15:
		case 16:
			ScaleNearest16(buffer, bigbuffer, x, y, bigx, bigy, bigpitch / 2, pitch / 2);
			break;
		case 24:
			ScaleNearest24(buffer, bigbuffer, x, y, bigx, bigy, bigpitch, pitch);
			break;
		case 32:
			ScaleNearest32(buffer, bigbuffer, x, y, bigx, bigy, bigpitch / 4, pitch / 4);
			break;
			break;
		}
	}
}

/**
  * Deletes an OpenGL texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  OpenGL texture to be deleted
  */
void glRenderer_DeleteTexture(glRenderer *This, TEXTURE * texture)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_DELETETEX, 0, 4, 1, 4, &texture);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Copies the contents of one surface to another.
  * @param This
  *  Pointer to glRenderer object
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
  *  synchronization of the operation: (not yet fully implemented)
  *  - DDBLT_ASYNC:  Adds the command to the queue.  If the queue is full, returns
  *    DDERR_WASSTILLDRAWING.
  *  - DDBLT_DONOTWAIT:  Fails and returns DDERR_WASSTILLDRAWING if the queue is full.
  *  - DDBLT_WAIT:  Waits until the Blt command is added to the queue before returning.
  * @param lpDDBltFx
  *  Effect parameters for the Blt operation.
  */
HRESULT glRenderer_Blt(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	DWORD nullfx = 0xFFFFFFFF;
	RECT emptyrect = nullrect;
	EnterCriticalSection(&This->commandcs);
	int syncmode = 0;
	if (dwFlags & DDBLT_ASYNC) syncmode = 1;
	if (dwFlags & DDBLT_DONOTWAIT) syncmode = 2;
	if (!lpSrcRect) lpSrcRect = &emptyrect;
	if (!lpDestRect) lpDestRect = &emptyrect;
	int fxsize = 4;
	if (lpDDBltFx) fxsize = sizeof(DDBLTFX);
	else lpDDBltFx = (LPDDBLTFX)&nullfx;
	if (glRenderer_AddQueue(This, OP_BLT, syncmode, 11 + (sizeof(RECT) / 2) + (fxsize / 4), 6, sizeof(RECT), lpDestRect, 4, &src,
		4, &dest, sizeof(RECT), lpSrcRect, 4, &dwFlags, fxsize, lpDDBltFx))
	{
		LeaveCriticalSection(&This->commandcs);
		return DDERR_WASSTILLDRAWING;
	}
	LeaveCriticalSection(&This->commandcs);
	return DD_OK;
}

/**
  * Updates the display with the current primary texture.
  * @param This
  *  Pointer to glRenderer object
  * @param texture
  *  Texture to use as the primary
  * @param paltex
  *  Texture that contains the color palette for 8-bit modes
  * @param dest
  *  Destination surface to be updated
  * @param src
  *  Source surface to be updated
  * @param vsync
  *  Vertical sync count
  */
void glRenderer_DrawScreen(glRenderer *This, TEXTURE *texture, TEXTURE *paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, GLint vsync)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_DRAWSCREEN, 0, 12, 5, 4, &texture, 4, &paltex, 4, &dest, 4, &src, 4, &vsync);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Ensures the renderer is set up for handling Direct3D commands.
  * @param This
  *  Pointer to glRenderer object
  * @param zbuffer
  *  Nonzero if a Z buffer is present.
  */
void glRenderer_InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_INITD3D, 0, 8, 3, 4, &zbuffer, 4, &x, 4, &y);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Clears the viewport.
  * @param This
  *  Pointer to glRenderer object
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
  */
void glRenderer_Clear(glRenderer *This, glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	EnterCriticalSection(&This->commandcs);
	int rectsize = dwCount * sizeof(D3DRECT);
	glRenderer_AddQueue(This, OP_CLEAR, 0, 15 + (rectsize / 4), 7, 4, &target, 4, &dwCount,
		4, &dwFlags, 4, &dwColor, 4, &dvZ, 4, &dwStencil, rectsize, lpRects);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Instructs the OpenGL driver to send all queued commands to the GPU, and empties the queue.
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Flush(glRenderer *This)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_CLEAR, 0, 2, 0, 0, NULL);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
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
void glRenderer_SetWnd(glRenderer *This, DWORD width, DWORD height, DWORD bpp, DWORD fullscreen, DWORD frequency, HWND newwnd, BOOL devwnd)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_SETWND, 0, 16, 7, 4, &width, 4, &height, 4, &bpp, 4, &fullscreen,
		4, &frequency, 4, &newwnd, 4, &devwnd);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}
/**
  * Draws one or more primitives to the currently selected render target.
  * @param This
  *  Pointer to glRenderer object
  * @param device
  *  glDirect3DDevice7 interface to use for drawing
  * @param mode
  *  OpenGL primitive drawing mode to use
  * @param stride
  *  Distance between vertex sets in bytes
  * @param vertices
  *  Pointer to vertex data
  * @param dwVertexTypeDesc
  *  FVF flags for vertex type.
  * @param count
  *  Number of vertices to copy to the draw command
  * @param indices
  *  List of vertex indices to use in the drawing command, may be NULL for
  *  non-indexed mode.
  * @param indexcount
  *  Number of vertex indices.  May be 0 for non-indexed mode.
  * @param flags
  *  Set to D3DDP_WAIT to wait until the queue has processed the call.
  * @return
  *  D3D_OK if the call succeeds, or D3DERR_INVALIDVERTEXTYPE if the vertex format
  *  has no position coordinates.
  */
void glRenderer_DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, DWORD stride, BYTE *vertices, DWORD dwVertexTypeDesc, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	EnterCriticalSection(&This->commandcs);
	DWORD vertsize = 0;
	DWORD indexsize = 0;
	if (!indices) indexcount = 0;
	vertsize = stride * count;
	indexsize = NextMultipleOf4(indexcount * 2);
	glRenderer_AddQueue(This, OP_DRAWPRIMITIVES, 0, 16 + ((vertsize + indexsize)/4), 8, 4, &device,
		4, &mode, 4, &stride, 4, &dwVertexTypeDesc, 4, &count, 4, &indexcount,
		vertsize, vertices, indexsize, indices);
	if (flags & D3DDP_WAIT) glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Deletes a framebuffer object.
  * @param This
  *  Pointer to glRenderer object
  * @param fbo
  *  FBO Structure containing framebuffer to delete
  */
void glRenderer_DeleteFBO(glRenderer *This, FBO *fbo)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_DELETEFBO, 0, 4, 1, 4, &fbo);
	glRenderer_Sync(This, 0);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Updates the clipper stencil for a surface.
  * @param This
  *  Pointer to glRenderer object
  * @param surface
  *  Surface to update clipper stencil on
  */
void glRenderer_UpdateClipper(glRenderer *This, glDirectDrawSurface7 *surface)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_UPDATECLIPPER, 0, 4, 1, 4, &surface);
	LeaveCriticalSection(&This->commandcs);
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
* @param lpDestRect
*  Pointer to bounding rectangle for depth fill.  If NULL, then fill entire surface
* @param dest
*  Destination surface to depth fill
* @param lpDDBltFx
*  Pointer to DDBLTFX structure with dwFillDepth defining the depth value.
*/
void glRenderer_DepthFill(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *dest, LPDDBLTFX lpDDBltFx)
{
	EnterCriticalSection(&This->commandcs);
	RECT emptyrect = nullrect;
	if (!lpDestRect) lpDestRect = &emptyrect;
	glRenderer_AddQueue(This, OP_DEPTHFILL, 0, 4 + (sizeof(RECT) / 4) + (sizeof(DDBLTFX) / 4),
		3, 4, dest, sizeof(RECT), lpDestRect, sizeof(DDBLTFX), lpDDBltFx);
	LeaveCriticalSection(&This->commandcs);
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
	EnterCriticalSection(&This->commandcs);
	if (!This->running) This->renderstate[dwRendStateType] = dwRenderState;
	else glRenderer_AddQueue(This, OP_SETRENDERSTATE, 0, 6, 2, 4, &dwRendStateType, 4, &dwRenderState);
	LeaveCriticalSection(&This->commandcs);
}

/**
* Binds a surface to a texture stage in the renderer.
* @param This
*  Pointer to glRenderer object
* @param dwStage
*  Texture stage to bind
* @param Texture
*  Texture to bind to the stage; old texture will be released; NULL to unbind
*/
void glRenderer_SetTexture(glRenderer *This, DWORD dwStage, glDirectDrawSurface7 *Texture)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_SETTEXTURE, 0, 6, 2, 4, &dwStage, 4, &Texture);
	LeaveCriticalSection(&This->commandcs);
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
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_SETTEXTURESTAGESTATE, 0, 8, 3, 4, &dwStage, 4, &dwState, 4, &dwValue);
	LeaveCriticalSection(&This->commandcs);
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
	EnterCriticalSection(&This->commandcs);
	if (!This->running) memcpy(&This->transform[dtstTransformStateType], lpD3DMatrix, sizeof(D3DMATRIX));
	else glRenderer_AddQueue(This, OP_SETTRANSFORM, 0, 5 + (sizeof(D3DMATRIX) / 4), 2,
		4, &dtstTransformStateType, sizeof(D3DMATRIX), lpD3DMatrix);
	LeaveCriticalSection(&This->commandcs);
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
	EnterCriticalSection(&This->commandcs);
	if (!This->running) memcpy(&This->material, lpMaterial, sizeof(D3DMATERIAL7));
	glRenderer_AddQueue(This, OP_SETMATERIAL, 0, 3 + (sizeof(D3DMATERIAL7) / 4), 1,
		sizeof(D3DMATERIAL7), lpMaterial);
	LeaveCriticalSection(&This->commandcs);
}

/**
* Sets a light in the renderer.
* @param This
*  Pointer to glRenderer object
* @param index
*  Index of light to set
* @param light
*  Pointer to light to change, ignored if remove is TRUE
* @param remove
*  TRUE to clear a light from the renderer.
*/

void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove)
{
	EnterCriticalSection(&This->commandcs);
	D3DLIGHT7 null_light;
	D3DLIGHT7 *light7;
	DWORD _remove = remove;
	if (light) light7 = light;
	else light7 = &null_light;
	glRenderer_AddQueue(This, OP_SETLIGHT, 0, 7 + (sizeof(D3DLIGHT7) / 4), 3, 4, &index,
		4, &_remove, sizeof(D3DLIGHT7), light7);
	LeaveCriticalSection(&This->commandcs);
}

/**
* Sets the viewport for the renderer.
* @param This
*  Pointer to glRenderer object
* @param lpViewport
*  New viewport parameters for renderer.
*/
void glRenderer_SetViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	EnterCriticalSection(&This->commandcs);
	glRenderer_AddQueue(This, OP_SETVIEWPORT, 0, 3 + (sizeof(D3DVIEWPORT7) / 4), 1,
		sizeof(D3DVIEWPORT7), lpViewport);
	LeaveCriticalSection(&This->commandcs);
}

/**
  * Main loop for glRenderer class
  * @param This
  *  Pointer to glRenderer object
  * @return
  *  Returns 0 to signal successful thread termination
  */
DWORD WINAPI glRenderer__Entry(glRenderer *This)
{
	float tmpfloats[16];
	RECT *r1;
	RECT *r2;
	DDBLTFX *fxptr;
	glRenderer__InitGL(This, (DWORD)This->inputs[0], (DWORD)This->inputs[1], (DWORD)This->inputs[2],
		(BOOL)This->inputs[3],(DWORD)This->inputs[4], (HWND)This->inputs[5], (glDirectDraw7*)This->inputs[6]);
	This->dead = false;
	This->queue = (LPDWORD)malloc(1048576);
	This->queuesize = 1048576 / sizeof(DWORD);
	This->queuelength = This->queue_read = This->queue_write = This->syncsize = 0;
	SetEvent(This->busy);
	This->start = CreateEvent(NULL, TRUE, FALSE, NULL);
	ResetEvent(This->start);
	This->sync = CreateEvent(NULL, TRUE, FALSE, NULL);
queueloop:
	WaitForSingleObject(&This->start, INFINITE);
	if (This->queuelength)
	{
		EnterCriticalSection(&This->queuecs);
		This->running = TRUE;
		LeaveCriticalSection(&This->queuecs);
		switch (This->queue[This->queue_read])
		{
		case OP_DELETE:
			if(This->hRC)
			{
				if(This->dib.enabled)
				{
					if(This->dib.hbitmap) DeleteObject(This->dib.hbitmap);
					if(This->dib.hdc) DeleteDC(This->dib.hdc);
					ZeroMemory(&This->dib,sizeof(DIB));
				}
				TextureManager_DeleteSamplers(This->texman);
				This->util->DeleteFBO(&This->fbo);
				if(This->PBO)
				{
					This->ext->glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
					This->ext->glDeleteBuffers(1,&This->PBO);
					This->PBO = 0;
				}
				if(This->backbuffer)
				{
					TextureManager__DeleteTexture(This->texman,This->backbuffer);
					free(This->backbuffer);
					This->backbuffer = NULL;
					This->backx = 0;
					This->backy = 0;
				}
				ShaderManager_Delete(This->shaders);
				free(This->shaders);
				free(This->texman);
				free(This->ext);
				delete This->util;
				This->ext = NULL;
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(This->hRC);
				This->hRC = NULL;
			};
			if(This->hDC) ReleaseDC(This->RenderWnd->GetHWnd(),This->hDC);
			This->hDC = NULL;
			delete This->RenderWnd;
			This->RenderWnd = NULL;
			This->dead = true;
			break;
		case OP_NULL:
			break;
		case OP_SETWND:
			if (This->queue[This->queue_read + 1] != 16) break;
			glRenderer__SetWnd(This, (DWORD)This->queue[This->queue_read + 3], (DWORD)This->queue[This->queue_read + 5],
				(DWORD)This->queue[This->queue_read + 7], (DWORD)This->queue[This->queue_read + 9], (DWORD)This->queue[This->queue_read + 11],
				(HWND)This->queue[This->queue_read + 13], (BOOL)This->queue[This->queue_read + 15]);
			break;
		case OP_CREATE:
			if (This->queue[This->queue_read + 1] != 8) break;
			glRenderer__MakeTexture(This, (TEXTURE*)This->queue[This->queue_read + 3],
				(DWORD)This->queue[This->queue_read + 5], (DWORD)This->queue[This->queue_read + 7]);
			break;
		case OP_UPLOAD:
			if (This->queue[This->queue_read + 1] != 14) break;
			glRenderer__UploadTexture(This, (BYTE*)This->queue[This->queue_read + 3], (TEXTURE*)This->queue[This->queue_read + 5],
				(DWORD)This->queue[This->queue_read + 7], (DWORD)This->queue[This->queue_read + 9],
				(DWORD)This->queue[This->queue_read + 11], (DWORD)This->queue[This->queue_read + 13]);
			break;
		case OP_DOWNLOAD:
			if (This->queue[This->queue_read + 1] != 8) break;
			glRenderer__DownloadTexture(This, (BYTE*)This->queue[This->queue_read + 3],
				(TEXTURE*)This->queue[This->queue_read + 5], (DWORD)This->queue[This->queue_read + 7]);
			break;
		case OP_DELETETEX:
			if (This->queue[This->queue_read + 1] != 4) break;
			glRenderer__DeleteTexture(This, (TEXTURE*)This->queue[This->queue_read + 3]);
			break;
		case OP_BLT:
			if (This->queue[This->queue_read + 1] < (10 + (sizeof(RECT) / 2))) break;
			r1 = (RECT*)&This->queue[This->queue_read + 3];
			r2 = (RECT*)&This->queue[This->queue_read + 8 + (sizeof(RECT) / 4)];
			if (!memcmp(r1, &nullrect, sizeof(RECT))) r1 = NULL;
			if (!memcmp(r2, &nullrect, sizeof(RECT))) r2 = NULL;
			if ((This->queue[This->queue_read + 10 + (sizeof(RECT) / 2)]) == 4) fxptr = NULL;
			else fxptr = (DDBLTFX*)&This->queue[This->queue_read + 11 + (sizeof(RECT) / 2)];
			glRenderer__Blt(This, r1, (glDirectDrawSurface7*)This->queue[This->queue_read + 4 + (sizeof(RECT) / 4)],
				(glDirectDrawSurface7*)This->queue[This->queue_read + 6 + (sizeof(RECT) / 4)], r2,
				(DWORD)This->queue[This->queue_read + 9 + (sizeof(RECT) / 2)], fxptr);
			break;
		case OP_DRAWSCREEN:
			if (This->queue[This->queue_read + 1] != 12) break;
			glRenderer__DrawScreen(This, (TEXTURE*)This->queue[This->queue_read + 3], (TEXTURE*)This->queue[This->queue_read + 5],
				(glDirectDrawSurface7*)This->queue[This->queue_read + 7], (glDirectDrawSurface7*)This->queue[This->queue_read + 9],
				(GLint)This->queue[This->queue_read + 11]);
			break;
		case OP_INITD3D:
			if (This->queue[This->queue_read + 1] != 8) break;
			glRenderer__InitD3D(This,(int)This->queue[This->queue_read+3],(int)This->queue[This->queue_read+5],
				(int)This->queue[This->queue_read + 7]);
			break;
		case OP_CLEAR:
			if (This->queue[This->queue_read + 1] < 15) break;
			if (This->queue[This->queue_read + 14] != 0) r1 = (RECT*)&This->queue[This->queue_read + 15];
			else r1 = NULL;
			glRenderer__Clear(This, (glDirectDrawSurface7*)This->queue[This->queue_read + 3], (DWORD)This->queue[This->queue_read + 5],
				(LPD3DRECT)r1, (DWORD)This->queue[This->queue_read + 7], (DWORD)This->queue[This->queue_read + 9],
				(D3DVALUE)This->queue[This->queue_read + 11], (DWORD)This->queue[This->queue_read + 13]);
			break;
		case OP_FLUSH:
			if (This->queue[This->queue_read + 1] != 2) break;
			glRenderer__Flush(This);
			break;
		case OP_DRAWPRIMITIVES:
			if (This->queue[This->queue_read + 1] < 16) break;
			glRenderer__DrawPrimitives(This, (glDirect3DDevice7*)This->queue[This->queue_read + 3], (GLenum)This->queue[This->queue_read+5],
				(DWORD)This->queue[This->queue_read + 7], (BYTE*)&This->queue[This->queue_read+15], (DWORD)This->queue[This->queue_read + 9],
				(DWORD)This->queue[This->queue_read + 11], (LPWORD)&This->queue[This->queue_read + 16+(This->queue[This->queue_read+14]/4)],
				(DWORD)This->queue[This->queue_read + 13], 0);
			break;
		case OP_DELETEFBO:
			if (This->queue[This->queue_read + 1] != 4) break;
			glRenderer__DeleteFBO(This, (FBO*)This->queue[This->queue_read + 3]);
			break;
		case OP_UPDATECLIPPER:
			if (This->queue[This->queue_read + 1] != 4) break;
			glRenderer__UpdateClipper(This, (glDirectDrawSurface7*)This->queue[This->queue_read + 3]);
			break;
		case OP_DEPTHFILL:
			if (This->queue[This->queue_read + 1] != (4 + (sizeof(RECT) / 4) + (sizeof(DDBLTFX) / 4))) break;
			r1 = (RECT*)&This->queue[This->queue_read + 5];
			if (!memcmp(r1, &nullrect, sizeof(RECT))) r1 = NULL;
			glRenderer__DepthFill(This, r1, (glDirectDrawSurface7*)This->queue[This->queue_read + 3],
				(LPDDBLTFX)&This->queue[This->queue_read + 4 + sizeof(RECT)]);
			break;
		case OP_SETRENDERSTATE:
			if (This->queue[This->queue_read + 1] != 6) break;
			glRenderer__SetRenderState(This, (D3DRENDERSTATETYPE)This->queue[This->queue_read + 3],
				(DWORD)This->queue[This->queue_read + 5]);
			break;
		case OP_SETTEXTURE:
			if (This->queue[This->queue_read + 1] != 6) break;
			glRenderer__SetTexture(This, (DWORD)This->queue[This->queue_read + 3],
				(glDirectDrawSurface7*)This->queue[This->queue_read + 5]);
			break;
		case OP_SETTEXTURESTAGESTATE:
			if (This->queue[This->queue_read + 1] != 8) break;
			glRenderer__SetTextureStageState(This, (DWORD)This->queue[This->queue_read + 3],
				(D3DTEXTURESTAGESTATETYPE)This->queue[This->queue_read + 5], (DWORD)This->queue[This->queue_read + 7]);
			break;
		case OP_SETTRANSFORM:
			if (This->queue[This->queue_read + 1] != (5 + (sizeof(D3DMATRIX) / 4))) break;
			glRenderer__SetTransform(This, (D3DTRANSFORMSTATETYPE)This->queue[This->queue_read + 3],
				(LPD3DMATRIX)&This->queue[This->queue_read + 5]);
			break;
		case OP_SETMATERIAL:
			if (This->queue[This->queue_read + 1] != (3 + (sizeof(D3DMATERIAL7) / 4))) break;
			glRenderer__SetMaterial(This, (LPD3DMATERIAL7)&This->queue[This->queue_read + 3]);
			break;
		case OP_SETLIGHT:
			if (This->queue[This->queue_read + 1] != (7 + (sizeof(D3DLIGHT7) / 4))) break;
			glRenderer__SetLight(This, (DWORD)This->queue[This->queue_read + 3], (LPD3DLIGHT7)&This->queue[This->queue_read + 7],
				(BOOL)This->queue[This->queue_read + 5]);
			break;
		case OP_SETVIEWPORT:
			if (This->queue[This->queue_read + 1] != (3 + (sizeof(D3DVIEWPORT7) / 4))) break;
			glRenderer__SetViewport(This, (LPD3DVIEWPORT7)&This->queue[This->queue_read + 3]);
			break;
		case OP_RESETQUEUE:
			break;
		default:
			FIXME("Invalid opcode detected");
			break;
		}
		EnterCriticalSection(&This->queuecs);
		This->queuelength--;
		This->queue_read += This->queue[This->queue_read + 1];
		if ((This->queue_read >= This->syncsize) && This->syncsize != 0) SetEvent(This->sync);
	}
	else EnterCriticalSection(&This->queuecs);
	if (!This->queuelength)
	{
		ResetEvent(This->start);
		This->queue_read = 0;
		This->queue_write = 0;
		This->running = false;
		SetEvent(This->sync);
	}
	LeaveCriticalSection(&This->queuecs);
	if (!This->dead) goto queueloop;
	free(This->queue);
	This->queue = NULL;
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
BOOL glRenderer__InitGL(glRenderer *This, DWORD width, DWORD height, DWORD bpp, BOOL fullscreen, DWORD frequency, HWND hWnd, glDirectDraw7 *glDD7)
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
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = bpp;
	pfd.iLayerType = PFD_MAIN_PLANE;
	InterlockedIncrement(&gllock);
	This->hDC = GetDC(This->RenderWnd->GetHWnd());
	if(!This->hDC)
	{
		DEBUG("glRenderer::InitGL: Can not create hDC\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	pf = ChoosePixelFormat(This->hDC,&pfd);
	if(!pf)
	{
		DEBUG("glRenderer::InitGL: Can not get pixelformat\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!SetPixelFormat(This->hDC,pf,&pfd))
		DEBUG("glRenderer::InitGL: Can not set pixelformat\n");
	This->hRC = wglCreateContext(This->hDC);
	if(!This->hRC)
	{
		DEBUG("glRenderer::InitGL: Can not create GL context\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	if(!wglMakeCurrent(This->hDC,This->hRC))
	{
		DEBUG("glRenderer::InitGL: Can not activate GL context\n");
		wglDeleteContext(This->hRC);
		This->hRC = NULL;
		ReleaseDC(This->RenderWnd->GetHWnd(),This->hDC);
		This->hDC = NULL;
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		return FALSE;
	}
	InterlockedDecrement(&gllock);
	LeaveCriticalSection(&dll_cs);
	This->ext = (glExtensions *)malloc(sizeof(glExtensions));
	glExtensions_Init(This->ext);
	This->util = new glUtil(This->ext);
	glRenderer__SetSwap(This,1);
	glFinish();
	DXGLTimer_Init(&This->timer);
	DXGLTimer_Calibrate(&This->timer, height, frequency);
	glRenderer__SetSwap(This,0);
	This->util->SetViewport(0,0,width,height);
	glViewport(0,0,width,height);
	This->util->SetDepthRange(0.0,1.0);
	This->util->DepthWrite(true);
	This->util->DepthTest(false);
	This->util->MatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DITHER);
	This->util->SetDepthComp(GL_LESS);
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
	This->util->InitFBO(&This->fbo);
	This->util->ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	This->util->ClearDepth(1.0);
	This->util->ClearStencil(0);
	This->util->EnableArray(-1,false);
	This->util->BlendFunc(GL_ONE,GL_ZERO);
	This->util->BlendEnable(false);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	This->util->SetScissor(false,0,0,0,0);
	glDisable(GL_SCISSOR_TEST);
	This->util->SetCull(D3DCULL_CCW);
	glEnable(GL_CULL_FACE);
	SwapBuffers(This->hDC);
	This->texman = TextureManager_Create(This->ext);
	TextureManager_SetActiveTexture(This->texman,0);
	glRenderer__SetFogColor(This,0);
	glRenderer__SetFogStart(This,0);
	glRenderer__SetFogEnd(This,1);
	glRenderer__SetFogDensity(This,1);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->util->SetShadeMode(D3DSHADE_GOURAUD);
	if(hWnd)
	{
		This->dib.enabled = true;
		This->dib.width = width;
		This->dib.height = height;
		This->dib.pitch = (((width<<3)+31)&~31) >>3;
		This->dib.pixels = NULL;
		This->dib.hdc = CreateCompatibleDC(NULL);
		ZeroMemory(&This->dib.info,sizeof(BITMAPINFO));
		This->dib.info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		This->dib.info.bmiHeader.biBitCount = 32;
		This->dib.info.bmiHeader.biWidth = width;
		This->dib.info.bmiHeader.biHeight = height;
		This->dib.info.bmiHeader.biCompression = BI_RGB;
		This->dib.info.bmiHeader.biPlanes = 1;
		This->dib.hbitmap = CreateDIBSection(This->dib.hdc,&This->dib.info,
			DIB_RGB_COLORS,(void**)&This->dib.pixels,NULL,0);
	}
	This->ext->glGenBuffers(1,&This->PBO);
	This->ext->glBindBuffer(GL_PIXEL_PACK_BUFFER,This->PBO);
	This->ext->glBufferData(GL_PIXEL_PACK_BUFFER,width*height*4,NULL,GL_STREAM_READ);
	This->ext->glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
	TextureManager_InitSamplers(This->texman);
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

void glRenderer__Blt(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *src,
	glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx)
{
	int rotates = 0;
	BOOL usedest = FALSE;
	BOOL usepattern = FALSE;
	LONG sizes[6];
	RECT srcrect;
	RECT destrect;
	This->ddInterface->GetSizes(sizes);
	DWORD shaderid;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	dest->GetSurfaceDesc(&ddsd);
	if (!lpDestRect)
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else destrect = *lpDestRect;
	if ((lpDDBltFx) && (dwFlags & DDBLT_ROP))
	{
		shaderid = PackROPBits(lpDDBltFx->dwROP, dwFlags);
		if (rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 2) usedest = TRUE;
		if (rop_texture_usage[(lpDDBltFx->dwROP >> 16) & 0xFF] & 4) usepattern = TRUE;
	}
	else shaderid = dwFlags & 0xF2FAADFF;
	if (dwFlags & DDBLT_KEYDEST) usedest = TRUE;
	if (usedest)
	{
		ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
		glRenderer__DrawBackbufferRect(This, dest->texture, destrect, PROG_TEXTURE);
		This->bltvertices[1].dests = This->bltvertices[3].dests = 0.;
		This->bltvertices[0].dests = This->bltvertices[2].dests = (GLfloat)(destrect.right - destrect.left) / (GLfloat)This->backx;
		This->bltvertices[0].destt = This->bltvertices[1].destt = 1.;
		This->bltvertices[2].destt = This->bltvertices[3].destt = 1.0-((GLfloat)(destrect.bottom - destrect.top) / (GLfloat)This->backy);
	}
	ShaderManager_SetShader(This->shaders, shaderid, NULL, 1);
	GenShader2D *shader = &This->shaders->gen2d->genshaders2D[This->shaders->gen3d->current_genshader];
	This->util->BlendEnable(false);
	do
	{
		if (This->util->SetFBO(dest) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!dest->texture->internalformats[1]) break;
		TextureManager_FixTexture(This->texman, dest->texture, (dest->bigbuffer ? dest->bigbuffer : dest->buffer), &dest->dirty, dest->miplevel);
		This->util->SetFBO((FBO*)NULL);
		dest->fbo.fbcolor = NULL;
		dest->fbo.fbz = NULL;
	} while (1);
	This->util->SetViewport(0,0,dest->fakex,dest->fakey);
	This->util->DepthTest(false);
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
	This->bltvertices[1].x = This->bltvertices[3].x = (GLfloat)destrect.left * ((GLfloat)dest->fakex/(GLfloat)ddsd.dwWidth);
	This->bltvertices[0].x = This->bltvertices[2].x = (GLfloat)destrect.right * ((GLfloat)dest->fakex/(GLfloat)ddsd.dwWidth);
	This->bltvertices[0].y = This->bltvertices[1].y = (GLfloat)dest->fakey-((GLfloat)destrect.top * ((GLfloat)dest->fakey/(GLfloat)ddsd.dwHeight));
	This->bltvertices[2].y = This->bltvertices[3].y = (GLfloat)dest->fakey-((GLfloat)destrect.bottom * ((GLfloat)dest->fakey/(GLfloat)ddsd.dwHeight));
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
	if ((lpDDBltFx) && (dwFlags & DDBLT_DDFX))
	{
		if (lpDDBltFx->dwDDFX & DDBLTFX_MIRRORLEFTRIGHT)
			BltFlipLR(This->bltvertices);
		if (lpDDBltFx->dwDDFX & DDBLTFX_MIRRORUPDOWN)
			BltFlipUD(This->bltvertices);
		if (lpDDBltFx->dwDDFX & DDBLTFX_ROTATE90) rotates++;
		if (lpDDBltFx->dwDDFX & DDBLTFX_ROTATE180) rotates += 2;
		if (lpDDBltFx->dwDDFX & DDBLTFX_ROTATE270) rotates += 3;
		rotates &= 3;
		if (rotates)
		{
			RotateBlt90(This->bltvertices, rotates);
		}
	}
	if (dwFlags & 0x10000000)
	{ 
		This->bltvertices[1].stencils = This->bltvertices[3].stencils = This->bltvertices[1].x / (GLfloat)dest->fakex;
		This->bltvertices[0].stencils = This->bltvertices[2].stencils = This->bltvertices[0].x / (GLfloat)dest->fakex;
		This->bltvertices[0].stencilt = This->bltvertices[1].stencilt = This->bltvertices[0].y / (GLfloat)dest->fakey;
		This->bltvertices[2].stencilt = This->bltvertices[3].stencilt = This->bltvertices[2].y / (GLfloat)dest->fakey;
	}
	if(dest->zbuffer) glClear(GL_DEPTH_BUFFER_BIT);
	if (dwFlags & DDBLT_COLORFILL) SetColorFillUniform(lpDDBltFx->dwFillColor, dest->texture->colorsizes,
		dest->texture->colororder, dest->texture->colorbits, shader->shader.uniforms[12], This->ext);
	if ((dwFlags & DDBLT_KEYSRC) && (src && src->colorkey[0].enabled) && !(dwFlags & DDBLT_COLORFILL))
	{
		SetColorKeyUniform(src->colorkey[0].key.dwColorSpaceLowValue, src->texture->colorsizes,
			src->texture->colororder, shader->shader.uniforms[5], src->texture->colorbits, This->ext);
		if (dwFlags & 0x20000000) SetColorKeyUniform(src->colorkey[0].key.dwColorSpaceHighValue, src->texture->colorsizes,
			src->texture->colororder, shader->shader.uniforms[7], src->texture->colorbits, This->ext);
	}
	if (!(dwFlags & DDBLT_COLORFILL)) This->ext->glUniform1i(shader->shader.uniforms[1], 0);
	if ((dwFlags & DDBLT_KEYDEST) && (This && dest->colorkey[1].enabled))
	{
		SetColorKeyUniform(dest->colorkey[1].key.dwColorSpaceLowValue, dest->texture->colorsizes,
			dest->texture->colororder, shader->shader.uniforms[6], dest->texture->colorbits, This->ext);
		if(dwFlags & 0x40000000) SetColorKeyUniform(dest->colorkey[1].key.dwColorSpaceHighValue, dest->texture->colorsizes,
			dest->texture->colororder, shader->shader.uniforms[8], dest->texture->colorbits, This->ext);
	}
	if (usedest && (shader->shader.uniforms[2] != -1))
	{
		TextureManager_SetTexture(This->texman, 1, This->backbuffer);
		This->ext->glUniform1i(shader->shader.uniforms[2], 1);
	}
	if (usepattern && (shader->shader.uniforms[3] != -1))
	{
		glDirectDrawSurface7 *pattern = (glDirectDrawSurface7*)lpDDBltFx->lpDDSPattern;
		TextureManager_SetTexture(This->texman, 2, pattern->texture);
		This->ext->glUniform1i(shader->shader.uniforms[3], 2);
		This->ext->glUniform2i(shader->shader.uniforms[9], pattern->texture->width, pattern->texture->height);
	}
	if (dwFlags & 0x10000000)  // Use clipper
	{
		TextureManager_SetTexture(This->texman, 3, dest->stencil);
		This->ext->glUniform1i(shader->shader.uniforms[4],3);
		This->util->EnableArray(shader->shader.attribs[5],true);
		This->ext->glVertexAttribPointer(shader->shader.attribs[5], 2, GL_FLOAT, false, sizeof(BltVertex), &This->bltvertices[0].stencils);
	}
	if(src)
	{
		TextureManager_SetTexture(This->texman,0,src->GetTexture());
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			if((dxglcfg.scalingfilter == 0) || (This->ddInterface->GetBPP() == 8))
				src->SetFilter(0,GL_NEAREST,GL_NEAREST,This->ext,This->texman);
			else src->SetFilter(0,GL_LINEAR,GL_LINEAR,This->ext,This->texman);
		}
	}
	else TextureManager_SetTexture(This->texman,0,NULL);
	This->ext->glUniform4f(shader->shader.uniforms[0],0,(GLfloat)dest->fakex,0,(GLfloat)dest->fakey);
	if(src) This->ext->glUniform4i(shader->shader.uniforms[10], src->texture->colorsizes[0], src->texture->colorsizes[1],
		src->texture->colorsizes[2], src->texture->colorsizes[3]);
	if(dest) This->ext->glUniform4i(shader->shader.uniforms[11], dest->texture->colorsizes[0], dest->texture->colorsizes[1],
		dest->texture->colorsizes[2], dest->texture->colorsizes[3]);
	dest->dirty |= 2;
	This->util->EnableArray(shader->shader.attribs[0],true);
	This->ext->glVertexAttribPointer(shader->shader.attribs[0],2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].x);
	if(!(dwFlags & DDBLT_COLORFILL))
	{
		This->util->EnableArray(shader->shader.attribs[3],true);
		This->ext->glVertexAttribPointer(shader->shader.attribs[3],2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].s);
	}
	if (usedest)
	{
		This->util->EnableArray(shader->shader.attribs[4], true);
		This->ext->glVertexAttribPointer(shader->shader.attribs[4],2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].dests);
	}
	This->util->SetCull(D3DCULL_NONE);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	This->util->SetFBO((FBO*)NULL);
	if(((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		glRenderer__DrawScreen(This,dest->texture,dest->paltex,dest,dest,0);
}

void glRenderer__MakeTexture(glRenderer *This, TEXTURE *texture, DWORD width, DWORD height)
{
	TextureManager__CreateTexture(This->texman,texture,width,height);
}

void glRenderer__DrawBackbuffer(glRenderer *This, TEXTURE **texture, int x, int y, int progtype)
{
	GLfloat view[4];
	TextureManager_SetActiveTexture(This->texman,0);
	if(!This->backbuffer)
	{
		This->backbuffer = (TEXTURE*)malloc(sizeof(TEXTURE));
		ZeroMemory(This->backbuffer,sizeof(TEXTURE));
			This->backbuffer->minfilter = This->backbuffer->magfilter = GL_LINEAR;
			This->backbuffer->wraps = This->backbuffer->wrapt = GL_CLAMP_TO_EDGE;
			This->backbuffer->pixelformat.dwFlags = DDPF_RGB;
			This->backbuffer->pixelformat.dwBBitMask = 0xFF;
			This->backbuffer->pixelformat.dwGBitMask = 0xFF00;
			This->backbuffer->pixelformat.dwRBitMask = 0xFF0000;
			This->backbuffer->pixelformat.dwRGBBitCount = 32;
		TextureManager__CreateTexture(This->texman,This->backbuffer,x,y);
		This->backx = x;
		This->backy = y;
	}
	if((This->backx != x) || (This->backy != y))
	{
		TextureManager__UploadTexture(This->texman,This->backbuffer,0,NULL,x,y, FALSE, TRUE);
		This->backx = x;
		This->backy = y;
	}
	This->util->SetFBO(&This->fbo,This->backbuffer,0,false);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)x;
	view[3] = (GLfloat)y;
	This->util->SetViewport(0,0,x,y);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	TextureManager_SetTexture(This->texman,0,*texture);
	*texture = This->backbuffer;
	if(This->ext->GLEXT_ARB_sampler_objects) ((glDirectDrawSurface7*)NULL)->SetFilter(0,GL_LINEAR,GL_LINEAR,This->ext,This->texman);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
	This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y;
	This->util->EnableArray(This->shaders->shaders[progtype].pos,true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].x);
	This->util->EnableArray(This->shaders->shaders[progtype].texcoord,true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].s);
	This->util->SetCull(D3DCULL_NONE);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	This->util->SetFBO((FBO*)NULL);
}

void glRenderer__DrawBackbufferRect(glRenderer *This, TEXTURE *texture, RECT srcrect, int progtype)
{
	GLfloat view[4];
	int x = srcrect.right - srcrect.left;
	int y = srcrect.bottom - srcrect.top;
	TextureManager_SetActiveTexture(This->texman, 0);
	if (!This->backbuffer)
	{
		This->backbuffer = (TEXTURE*)malloc(sizeof(TEXTURE));
		ZeroMemory(This->backbuffer, sizeof(TEXTURE));
		This->backbuffer->minfilter = This->backbuffer->magfilter = GL_LINEAR;
		This->backbuffer->wraps = This->backbuffer->wrapt = GL_CLAMP_TO_EDGE;
		This->backbuffer->pixelformat.dwFlags = DDPF_RGB;
		This->backbuffer->pixelformat.dwBBitMask = 0xFF;
		This->backbuffer->pixelformat.dwGBitMask = 0xFF00;
		This->backbuffer->pixelformat.dwRBitMask = 0xFF0000;
		This->backbuffer->pixelformat.dwRGBBitCount = 32;
		TextureManager__CreateTexture(This->texman, This->backbuffer, x, y);
		This->backx = x;
		This->backy = y;
	}
	if ((This->backx < x) || (This->backy < y))
	{
		if (This->backx > x) x = This->backx;
		if (This->backx > y) y = This->backx;
		TextureManager__UploadTexture(This->texman, This->backbuffer, 0, NULL, x, y, FALSE, TRUE);
		This->backx = x;
		This->backy = y;
	}
	This->util->SetFBO(&This->fbo, This->backbuffer, 0, false);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)This->backx;
	view[3] = (GLfloat)This->backy;
	This->util->SetViewport(0, 0, This->backx, This->backy);
	This->util->SetScissor(true, 0, 0, This->backx, This->backy);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	This->util->SetScissor(false, 0, 0, 0, 0);
	TextureManager_SetTexture(This->texman, 0, texture);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view, view[0], view[1], view[2], view[3]);
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)texture->width;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)texture->width;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)texture->height;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)texture->height;
	This->bltvertices[1].x = This->bltvertices[3].x = 0.;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x;
	This->bltvertices[0].y = This->bltvertices[1].y = 0.;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y;
	This->util->EnableArray(This->shaders->shaders[progtype].pos, true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos, 2, GL_FLOAT, false, sizeof(BltVertex), &This->bltvertices[0].x);
	This->util->EnableArray(This->shaders->shaders[progtype].texcoord, true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord, 2, GL_FLOAT, false, sizeof(BltVertex), &This->bltvertices[0].s);
	This->util->SetCull(D3DCULL_NONE);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_SHORT, bltindices);
	This->util->SetFBO((FBO*)NULL);
}

void glRenderer__DrawScreen(glRenderer *This, TEXTURE *texture, TEXTURE *paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, GLint vsync)
{
	int progtype;
	RECT r,r2;
	This->util->BlendEnable(false);
	if((dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(This->RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	This->util->DepthTest(false);
	RECT *viewrect = &r2;
	glRenderer__SetSwap(This,vsync);
	LONG sizes[6];
	GLfloat view[4];
	GLint viewport[4];
	if(src->dirty & 1)
	{
		glRenderer__UploadTextureSurface(This,src->buffer,src->bigbuffer,texture,src->ddsd.dwWidth,src->ddsd.dwHeight,
			src->fakex,src->fakey,src->ddsd.lPitch,
			(NextMultipleOf4((This->ddInterface->GetBPPMultipleOf8()/8)*src->fakex)),
			src->ddsd.ddpfPixelFormat.dwRGBBitCount,src->miplevel);
		src->dirty &= ~1;
	}
	if(dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{
		if(This->ddInterface->GetFullscreen())
		{
			This->ddInterface->GetSizes(sizes);
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
			ClientToScreen(This->RenderWnd->GetHWnd(),(LPPOINT)&viewrect->left);
			ClientToScreen(This->RenderWnd->GetHWnd(),(LPPOINT)&viewrect->right);
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
	This->util->SetFBO((FBO*)NULL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(This->ddInterface->GetBPP() == 8)
	{
		ShaderManager_SetShader(This->shaders,PROG_PAL256,NULL,0);
		progtype = PROG_PAL256;
		TextureManager__UploadTexture(This->texman,paltex,0,glDirectDrawPalette_GetPalette(dest->palette,NULL),256,1,FALSE,FALSE);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
		This->ext->glUniform1i(This->shaders->shaders[progtype].pal,1);
		TextureManager_SetTexture(This->texman,0,texture);
		TextureManager_SetTexture(This->texman,1,paltex);
		if(dxglcfg.scalingfilter)
		{
			glRenderer__DrawBackbuffer(This,&texture,dest->fakex,dest->fakey,progtype);
			ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
			progtype = PROG_TEXTURE;
			TextureManager_SetTexture(This->texman,0,texture);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
		}
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			((glDirectDrawSurface7*)NULL)->SetFilter(0,GL_NEAREST,GL_NEAREST,This->ext,This->texman);
			((glDirectDrawSurface7*)NULL)->SetFilter(1,GL_NEAREST,GL_NEAREST,This->ext,This->texman);
		}
	}
	else
	{
		ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
		progtype = PROG_TEXTURE;
		TextureManager_SetTexture(This->texman,0,texture);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
	}
	if(dxglcfg.scalingfilter && This->ext->GLEXT_ARB_sampler_objects)
		((glDirectDrawSurface7*)NULL)->SetFilter(0,GL_LINEAR,GL_LINEAR,This->ext,This->texman);
	else if(This->ext->GLEXT_ARB_sampler_objects)
		((glDirectDrawSurface7*)NULL)->SetFilter(0,GL_NEAREST,GL_NEAREST,This->ext,This->texman);
	This->util->SetViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	if(This->ddInterface->GetFullscreen())
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)sizes[0];
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)sizes[1];
	}
	else
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)dest->fakex;
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)dest->fakey;
	}
	This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
	This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	This->util->EnableArray(This->shaders->shaders[progtype].pos,true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].x);
	This->util->EnableArray(This->shaders->shaders[progtype].texcoord,true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,false,sizeof(BltVertex),&This->bltvertices[0].s);
	This->util->SetCull(D3DCULL_NONE);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glFlush();
	if(This->hWnd) SwapBuffers(This->hDC);
	else
	{
		glReadBuffer(GL_FRONT);
		This->ext->glBindBuffer(GL_PIXEL_PACK_BUFFER,This->PBO);
		GLint packalign;
		glGetIntegerv(GL_PACK_ALIGNMENT,&packalign);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		This->ddInterface->GetSizes(sizes);
		glReadPixels(0,0,sizes[4],sizes[5],GL_BGRA,GL_UNSIGNED_BYTE,0);
		GLubyte *pixels = (GLubyte*)This->ext->glMapBuffer(GL_PIXEL_PACK_BUFFER,GL_READ_ONLY);
		for(int i = 0; i < sizes[5];i++)
		{
			memcpy(&This->dib.pixels[This->dib.pitch*i],
				&pixels[((sizes[5]-1)-i)*(sizes[4]*4)],sizes[4]*4);
		}
		This->ext->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		This->ext->glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
		glPixelStorei(GL_PACK_ALIGNMENT,packalign);
		HDC hRenderDC = (HDC)::GetDC(This->RenderWnd->GetHWnd());
		HGDIOBJ hPrevObj = 0;
		POINT dest = {0,0};
		POINT srcpoint = {0,0};
		SIZE wnd = {This->dib.width,This->dib.height};
		BLENDFUNCTION func = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};
		hPrevObj = SelectObject(This->dib.hdc,This->dib.hbitmap);
		ClientToScreen(This->RenderWnd->GetHWnd(),&dest);
		UpdateLayeredWindow(This->RenderWnd->GetHWnd(),hRenderDC,&dest,&wnd,
			This->dib.hdc,&srcpoint,0,&func,ULW_ALPHA);
		SelectObject(This->dib.hdc,hPrevObj);
		ReleaseDC(This->RenderWnd->GetHWnd(),hRenderDC);
	}
}

void glRenderer__DeleteTexture(glRenderer *This, TEXTURE *texture)
{
	TextureManager__DeleteTexture(This->texman,texture);
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
			if (!(texstages[0].texture->ddsd.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS))
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
			if (texstages[i].texture->ddsd.dwFlags & DDSD_CKSRCBLT) renderer->shaderstate3d.texstageid[i] |= 1i64 << 60;
		}
	}
	return shader;
}

void glRenderer__InitD3D(glRenderer *This, int zbuffer, int x, int y)
{
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	if(zbuffer) This->util->DepthTest(true);
	This->util->SetDepthComp(GL_LEQUAL);
	GLfloat identity[16];
	__gluMakeIdentityf(identity);
	This->util->SetMatrix(GL_MODELVIEW,identity,identity,NULL);
	This->util->SetMatrix(GL_PROJECTION,identity,NULL,NULL);
	for (int i = 0; i < 24; i++)
		memcpy(&This->transform[i], identity, sizeof(D3DMATRIX));
	GLfloat one[4] = {1,1,1,1};
	GLfloat zero[4] = {0,0,0,1};
	This->util->SetMaterial(one,one,zero,zero,0);
	ZeroMemory(&This->material, sizeof(D3DMATERIAL7));
	ZeroMemory(&This->lights, 8 * sizeof(D3DLIGHT7));
	memcpy(&This->renderstate, &renderstate_default, 153 * sizeof(DWORD));
	This->texstages[0] = texstagedefault0;
	This->texstages[1] = This->texstages[2] = This->texstages[3] = This->texstages[4] =
		This->texstages[5] = This->texstages[6] = This->texstages[7] = texstagedefault1;
	This->viewport.dwX = 0;
	This->viewport.dwY = 0;
	This->viewport.dwWidth = x;
	This->viewport.dwHeight = y;
	This->viewport.dvMinZ = 0.0f;
	This->viewport.dvMaxZ = 1.0f;
	This->shaderstate3d.stateid = InitShaderState(This, This->renderstate, This->texstages, This->lights);
}

void glRenderer__Clear(glRenderer *This, glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	GLfloat color[4];
	dwordto4float(dwColor,color);
	do
	{
		if (This->util->SetFBO(target) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!target->texture->internalformats[1]) break;
		TextureManager_FixTexture(This->texman, target->texture, (target->bigbuffer ? target->bigbuffer : target->buffer), &target->dirty, target->miplevel);
		This->util->SetFBO((FBO*)NULL);
		target->fbo.fbcolor = NULL;
		target->fbo.fbz = NULL;
	} while (1);
	int clearbits = 0;
	if(dwFlags & D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		This->util->ClearColor(color[0],color[1],color[2],color[3]);
	}
	if(dwFlags & D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		This->util->ClearDepth(dvZ);
		This->util->DepthWrite(true);
	}
	if(dwFlags & D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		This->util->ClearStencil(dwStencil);
	}
	if(dwCount)
	{
		for(DWORD i = 0; i < dwCount; i++)
		{
			This->util->SetScissor(true,lpRects[i].x1,lpRects[i].y1,lpRects[i].x2,lpRects[i].y2);
			glClear(clearbits);
		}
		This->util->SetScissor(false,0,0,0,0);
	}
	else glClear(clearbits);
	if(target->zbuffer) target->zbuffer->dirty |= 2;
	target->dirty |= 2;
}

void glRenderer__Flush(glRenderer *This)
{
	glFlush();
}

void glRenderer__SetWnd(glRenderer *This, DWORD width, DWORD height, DWORD fullscreen, DWORD bpp, DWORD frequency, HWND newwnd, BOOL devwnd)
{
	if(newwnd != This->hWnd)
	{
		EnterCriticalSection(&dll_cs);
		wglMakeCurrent(NULL, NULL);
		ReleaseDC(This->hWnd,This->hDC);
		delete This->RenderWnd;
		This->RenderWnd = new glRenderWindow(width,height,fullscreen,newwnd,This->ddInterface, devwnd);
		PIXELFORMATDESCRIPTOR pfd;
		GLuint pf;
		InterlockedIncrement(&gllock);
		ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = bpp;
		pfd.iLayerType = PFD_MAIN_PLANE;
		This->hDC = GetDC(This->RenderWnd->GetHWnd());
		if(!This->hDC)
			DEBUG("glRenderer::SetWnd: Can not create hDC\n");
		pf = ChoosePixelFormat(This->hDC,&pfd);
		if(!pf)
			DEBUG("glRenderer::SetWnd: Can not get pixelformat\n");
		if(!SetPixelFormat(This->hDC,pf,&pfd))
			DEBUG("glRenderer::SetWnd: Can not set pixelformat\n");
		if(!wglMakeCurrent(This->hDC,This->hRC))
			DEBUG("glRenderer::SetWnd: Can not activate GL context\n");
		InterlockedDecrement(&gllock);
		LeaveCriticalSection(&dll_cs);
		glRenderer__SetSwap(This,1);
		SwapBuffers(This->hDC);
		DXGLTimer_Init(&This->timer);
		DXGLTimer_Calibrate(&This->timer, height, frequency);
		glRenderer__SetSwap(This,0);
		This->util->SetViewport(0,0,width,height);
	}
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
	This->util->BlendFunc(glsrc,gldest);
}

void glRenderer__DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, DWORD stride, BYTE *vertices, DWORD dwVertexTypeDesc, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	bool transformed;
	char svar[] = "sX";
	char stvar[] = "stX";
	char strvar[] = "strX";
	char strqvar[] = "strqX";
	int i;
	int ptr = 0;
	int numtex;
	int texcoords[8];
	if ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) transformed = true;
	else transformed = false;
	This->shaderstate3d.stateid &= 0xFFFA3FFC7FFFFFFFi64;
	This->shaderstate3d.stateid &= 0xFFFA3FF87FFFFFFFi64;
	int numtextures = (dwVertexTypeDesc & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	This->shaderstate3d.stateid |= (__int64)((numtextures-1)&7) << 31;
	if (numtextures) This->shaderstate3d.stateid |= (1i64 << 34);
	int blendweights = 0;
	if (((dwVertexTypeDesc & D3DFVF_POSITION_MASK) >> 1) > 2)
		blendweights = ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) >> 1) - 2;
	This->shaderstate3d.stateid |= (__int64)blendweights << 46;
	if (transformed) This->shaderstate3d.stateid |= (1i64 << 50);
	if (dwVertexTypeDesc & D3DFVF_DIFFUSE) This->shaderstate3d.stateid |= (1i64 << 35);
	if (dwVertexTypeDesc & D3DFVF_SPECULAR) This->shaderstate3d.stateid |= (1i64 << 36);
	if (dwVertexTypeDesc & D3DFVF_NORMAL) This->shaderstate3d.stateid |= (1i64 << 37);
	numtex = (dwVertexTypeDesc & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
	for(i = 0; i < numtex; i++)
	{
		This->shaderstate3d.texstageid[i] &= 0xFFE7FFFFFFFFFFFFi64;
		switch (dwVertexTypeDesc >> (16 + (2 * i)) & 3)
		{
		case 0: // st
			texcoords[i] = 2;
			break;
		case 1: // str
			texcoords[i] = 3;
			break;
		case 2: // strq
			texcoords[i] = 4;
			break;
		case 3: // s
			texcoords[i] = 1;
			break;
		}
		This->shaderstate3d.texstageid[i] |= (__int64)(texcoords[i] - 1) << 51;
	}
	ShaderManager_SetShader(This->shaders, This->shaderstate3d.stateid, This->shaderstate3d.texstageid, 2);
	_GENSHADER *prog = &This->shaders->gen3d->genshaders[This->shaders->gen3d->current_genshader].shader;
	This->util->EnableArray(prog->attribs[0], true);
	This->ext->glVertexAttribPointer(prog->attribs[0], 3, GL_FLOAT, false, stride, &vertices[ptr]);
	ptr += 12;
	if (transformed)
	{
		if (prog->attribs[1] != -1)
		{
			This->util->EnableArray(prog->attribs[1], true);
			This->ext->glVertexAttribPointer(prog->attribs[1], 4, GL_FLOAT, false, stride, &vertices[ptr]);
		}
		ptr += 4;
	}
	else if (dwVertexTypeDesc & D3DFVF_RESERVED1) ptr += 4;
	glRenderer__SetDepthComp(This);
	if(This->renderstate[D3DRENDERSTATE_ZENABLE]) This->util->DepthTest(true);
	else This->util->DepthTest(false);
	if(This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]) This->util->DepthWrite(true);
	else This->util->DepthWrite(false);
	for(i = 0; i < blendweights; i++)
	{
		if(prog->attribs[i+2] != -1)
		{
			This->util->EnableArray(prog->attribs[i+2],true);
			This->ext->glVertexAttribPointer(prog->attribs[i + 2], 1, GL_FLOAT, false, stride, &vertices[ptr]);
		}
		ptr += 4;
	}
	if (dwVertexTypeDesc & D3DFVF_NORMAL)
	{
		if(prog->attribs[7] != -1)
		{
			This->util->EnableArray(prog->attribs[7],true);
			This->ext->glVertexAttribPointer(prog->attribs[7], 3, GL_FLOAT, false, stride, &vertices[ptr]);
		}
		ptr += 12;
	}
	if (dwVertexTypeDesc & D3DFVF_DIFFUSE)
	{
		if (prog->attribs[8] != -1)
		{
			This->util->EnableArray(prog->attribs[8], true);
			This->ext->glVertexAttribPointer(prog->attribs[8], 4, GL_UNSIGNED_BYTE, true, stride, &vertices[ptr]);
		}
		ptr += 4;
	}
	if (dwVertexTypeDesc & D3DFVF_SPECULAR)
	{
		if (prog->attribs[9] != -1)
		{
			This->util->EnableArray(prog->attribs[9], true);
			This->ext->glVertexAttribPointer(prog->attribs[9], 4, GL_UNSIGNED_BYTE, true, stride, &vertices[ptr]);
		}
		ptr += 4;
	}
	for(i = 0; i < numtex; i++)
	{
		{
			switch(texcoords[i])
			{
			case -1: // Null
			default:
				break;
			case 1: // s
				if (prog->attribs[i + 10] != -1)
				{
					This->util->EnableArray(prog->attribs[i + 10], true);
					This->ext->glVertexAttribPointer(prog->attribs[i + 10], 1, GL_FLOAT, false, stride, &vertices[ptr]);
				}
				ptr += 4;
				break;
			case 2: // st
				if(prog->attribs[i+18] != -1)
				{
					This->util->EnableArray(prog->attribs[i+18],true);
					This->ext->glVertexAttribPointer(prog->attribs[i + 18], 2, GL_FLOAT, false, stride, &vertices[ptr]);
				}
				ptr += 8;
				break;
			case 3: // str
				if(prog->attribs[i+26] != -1)
				{
					This->util->EnableArray(prog->attribs[i+26],true);
					This->ext->glVertexAttribPointer(prog->attribs[i + 26], 3, GL_FLOAT, false, stride, &vertices[ptr]);
				}
				ptr += 12;
				break;
			case 4: // strq
				if(prog->attribs[i+34] != -1)
				{
					This->util->EnableArray(prog->attribs[i+34],true);
					This->ext->glVertexAttribPointer(prog->attribs[i + 34], 4, GL_FLOAT, false, stride, &vertices[ptr]);
				}
				ptr += 16;
				break;
			}

		}
	}
	This->util->SetMatrix(GL_MODELVIEW, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_VIEW], 
		(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD],NULL);
	This->util->SetMatrix(GL_PROJECTION, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_PROJECTION], NULL, NULL);

	This->util->SetMaterial((GLfloat*)&This->material.ambient,(GLfloat*)&This->material.diffuse,(GLfloat*)&This->material.specular,
		(GLfloat*)&This->material.emissive,This->material.power);

	int lightindex = 0;
	char lightname[] = "lightX.xxxxxxxxxxxxxxxx";
	for(i = 0; i < 8; i++)
	{
		if(This->lights[i].dltType)
		{
			if(prog->uniforms[0] != -1) This->ext->glUniformMatrix4fv(prog->uniforms[0],1,false,
				(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD]);
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

	DWORD ambient = This->renderstate[D3DRENDERSTATE_AMBIENT];
	if(prog->uniforms[136] != -1)
		This->ext->glUniform4f(prog->uniforms[136],RGBA_GETRED(ambient),RGBA_GETGREEN(ambient),
			RGBA_GETBLUE(ambient),RGBA_GETALPHA(ambient));
	GLint keycolor[4];
	for(i = 0; i < 8; i++)
	{
		if(This->texstages[i].colorop == D3DTOP_DISABLE) break;
		if(This->texstages[i].texture)
		{
			if(This->texstages[i].texture->dirty & 1)
			{
				glRenderer__UploadTextureSurface(This,This->texstages[i].texture->buffer,This->texstages[i].texture->bigbuffer,
					This->texstages[i].texture->texture,This->texstages[i].texture->ddsd.dwWidth,
					This->texstages[i].texture->ddsd.dwHeight,This->texstages[i].texture->fakex,
					This->texstages[i].texture->fakey,This->texstages[i].texture->ddsd.lPitch,
					(This->texstages[i].texture->ddsd.ddpfPixelFormat.dwRGBBitCount/8*This->texstages[i].texture->fakex),
					This->texstages[i].texture->ddsd.ddpfPixelFormat.dwRGBBitCount, This->texstages[i].texture->miplevel);
				This->texstages[i].texture->dirty &= ~1;
			}
			if(This->texstages[i].texture)
				This->texstages[i].texture->SetFilter(i,This->texstages[i].glmagfilter,This->texstages[i].glminfilter,This->ext,This->texman);
			TextureManager_SetTexture(This->texman,i,This->texstages[i].texture->texture);
			This->util->SetWrap(i,0,This->texstages[i].addressu,This->texman);
			This->util->SetWrap(i,1,This->texstages[i].addressv,This->texman);
		}
		TextureManager_SetTexture(This->texman,i,0);
		This->ext->glUniform1i(prog->uniforms[128+i],i);
		if(This->renderstate[D3DRENDERSTATE_COLORKEYENABLE] && This->texstages[i].texture && (prog->uniforms[142+i] != -1))
		{
			if(This->texstages[i].texture->ddsd.dwFlags & DDSD_CKSRCBLT)
			{
				SetColorKeyUniform(This->texstages[i].texture->colorkey[0].key.dwColorSpaceLowValue,
					This->texstages[i].texture->texture->colorsizes, This->texstages[i].texture->texture->colororder,
					prog->uniforms[142 + i], This->texstages[i].texture->texture->colorbits, This->ext);
				This->ext->glUniform4i(prog->uniforms[153+i], This->texstages[i].texture->texture->colorsizes[0], 
					This->texstages[i].texture->texture->colorsizes[1],
					This->texstages[i].texture->texture->colorsizes[2],
					This->texstages[i].texture->texture->colorsizes[3]);
			}
		}
	}
	if(prog->uniforms[137]!= -1) This->ext->glUniform1f(prog->uniforms[137],This->viewport.dwWidth);
	if(prog->uniforms[138]!= -1) This->ext->glUniform1f(prog->uniforms[138],This->viewport.dwHeight);
	if(prog->uniforms[139]!= -1) This->ext->glUniform1f(prog->uniforms[139],This->viewport.dwX);
	if(prog->uniforms[140]!= -1) This->ext->glUniform1f(prog->uniforms[140],This->viewport.dwY);
	if(prog->uniforms[141]!= -1) This->ext->glUniform1i(prog->uniforms[141],This->renderstate[D3DRENDERSTATE_ALPHAREF]);
	if(prog->uniforms[150]!= -1) This->ext->glUniform4iv(prog->uniforms[150],1,(GLint*)device->glDDS7->texture->colorbits);
	do
	{
		if (This->util->SetFBO(device->glDDS7) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!device->glDDS7->texture->internalformats[1]) break;
		TextureManager_FixTexture(This->texman, device->glDDS7->texture,
			(device->glDDS7->bigbuffer ? device->glDDS7->bigbuffer : device->glDDS7->buffer), &device->glDDS7->dirty, device->glDDS7->miplevel);
		This->util->SetFBO((FBO*)NULL);
		device->glDDS7->fbo.fbcolor = NULL;
		device->glDDS7->fbo.fbz = NULL;
	} while (1);
	This->util->SetViewport((int)((float)This->viewport.dwX*device->glDDS7->mulx),
		(int)((float)This->viewport.dwY*device->glDDS7->muly),
		(int)((float)This->viewport.dwWidth*device->glDDS7->mulx),
		(int)((float)This->viewport.dwHeight*device->glDDS7->muly));
	This->util->SetDepthRange(This->viewport.dvMinZ,This->viewport.dvMaxZ);
	if(This->renderstate[D3DRENDERSTATE_ALPHABLENDENABLE]) This->util->BlendEnable(true);
	else This->util->BlendEnable(false);
	glRenderer__SetBlend(This,This->renderstate[D3DRENDERSTATE_SRCBLEND],This->renderstate[D3DRENDERSTATE_DESTBLEND]);
	This->util->SetCull((D3DCULL)This->renderstate[D3DRENDERSTATE_CULLMODE]);
	glRenderer__SetFogColor(This,This->renderstate[D3DRENDERSTATE_FOGCOLOR]);
	glRenderer__SetFogStart(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGSTART]));
	glRenderer__SetFogEnd(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGEND]));
	glRenderer__SetFogDensity(This,*(GLfloat*)(&This->renderstate[D3DRENDERSTATE_FOGDENSITY]));
	This->util->SetPolyMode((D3DFILLMODE)This->renderstate[D3DRENDERSTATE_FILLMODE]);
	This->util->SetShadeMode((D3DSHADEMODE)This->renderstate[D3DRENDERSTATE_SHADEMODE]);
	if(indices) glDrawElements(mode,indexcount,GL_UNSIGNED_SHORT,indices);
	else glDrawArrays(mode,0,count);
	if(device->glDDS7->zbuffer) device->glDDS7->zbuffer->dirty |= 2;
	device->glDDS7->dirty |= 2;
	if(flags & D3DDP_WAIT) glFlush();
	return;
}

void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo)
{
	This->util->DeleteFBO(fbo);
}

void glRenderer__UpdateClipper(glRenderer *This, glDirectDrawSurface7 *surface)
{
	GLfloat view[4];
	if (!surface->stencil)
	{
		surface->stencil = (TEXTURE*)malloc(sizeof(TEXTURE));
		ZeroMemory(surface->stencil, sizeof(TEXTURE));
		surface->stencil->minfilter = surface->stencil->magfilter = GL_NEAREST;
		surface->stencil->wraps = surface->stencil->wrapt = GL_CLAMP_TO_EDGE;
		surface->stencil->pixelformat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		surface->stencil->pixelformat.dwBBitMask = 0xF;
		surface->stencil->pixelformat.dwGBitMask = 0xF0;
		surface->stencil->pixelformat.dwRBitMask = 0xF00;
		surface->stencil->pixelformat.dwZBitMask = 0xF000;
		surface->stencil->pixelformat.dwRGBBitCount = 16;
		TextureManager__CreateTexture(This->texman, surface->stencil, surface->ddsd.dwWidth, surface->ddsd.dwHeight);
	}
	if ((surface->ddsd.dwWidth != surface->stencil->width) ||
		(surface->ddsd.dwHeight != surface->stencil->height))
		TextureManager__UploadTexture(This->texman, surface->stencil, 0, NULL,
			surface->ddsd.dwWidth, surface->ddsd.dwHeight, FALSE, TRUE);
	This->util->SetFBO(&surface->stencilfbo, surface->stencil, 0, false);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)surface->ddsd.dwWidth;
	view[3] = (GLfloat)surface->ddsd.dwHeight;
	This->util->SetViewport(0,0,surface->ddsd.dwWidth,surface->ddsd.dwHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	ShaderManager_SetShader(This->shaders,PROG_CLIPSTENCIL,NULL,0);
	This->ext->glUniform4f(This->shaders->shaders[PROG_CLIPSTENCIL].view,view[0],view[1],view[2],view[3]);
	This->util->EnableArray(This->shaders->shaders[PROG_CLIPSTENCIL].pos,true);
	This->ext->glVertexAttribPointer(This->shaders->shaders[PROG_CLIPSTENCIL].pos,
		2,GL_FLOAT,false,sizeof(BltVertex),&surface->clipper->vertices[0].x);
	This->util->SetCull(D3DCULL_NONE);
	This->util->SetPolyMode(D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLES, 0, (6 * surface->clipper->clipsize) - 1,
		6 * surface->clipper->clipsize, GL_UNSIGNED_SHORT, surface->clipper->indices);
	This->util->SetFBO((FBO*)NULL);
}

void glRenderer__DepthFill(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *dest, LPDDBLTFX lpDDBltFx)
{
	RECT destrect;
	DDSURFACEDESC2 ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	dest->GetSurfaceDesc(&ddsd);
	if (!lpDestRect)
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else destrect = *lpDestRect;
	if (dest->attachparent)
	{
		do
		{
			if (This->util->SetFBO(dest->attachparent) == GL_FRAMEBUFFER_COMPLETE) break;
			if (!dest->attachparent->texture->internalformats[1]) break;
			TextureManager_FixTexture(This->texman, dest->attachparent->texture, 
				(dest->attachparent->bigbuffer ? dest->attachparent->bigbuffer : dest->attachparent->buffer),
				&dest->attachparent->dirty, dest->attachparent->miplevel);
			This->util->SetFBO((FBO*)NULL);
			dest->attachparent->fbo.fbcolor = NULL;
			dest->attachparent->fbo.fbz = NULL;
		} while (1);
	}
	else
	{
		if (!dest->dummycolor)
		{
			dest->dummycolor = (TEXTURE*)malloc(sizeof(TEXTURE));
			ZeroMemory(dest->dummycolor, sizeof(TEXTURE));
			dest->dummycolor->minfilter = dest->dummycolor->magfilter = GL_NEAREST;
			dest->dummycolor->wraps = dest->dummycolor->wrapt = GL_CLAMP_TO_EDGE;
			dest->dummycolor->pixelformat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
			dest->dummycolor->pixelformat.dwBBitMask = 0xF;
			dest->dummycolor->pixelformat.dwGBitMask = 0xF0;
			dest->dummycolor->pixelformat.dwRBitMask = 0xF00;
			dest->dummycolor->pixelformat.dwZBitMask = 0xF000;
			dest->dummycolor->pixelformat.dwRGBBitCount = 16;
			TextureManager__CreateTexture(This->texman, dest->dummycolor, dest->ddsd.dwWidth, dest->ddsd.dwHeight);
		}
		if ((dest->ddsd.dwWidth != dest->dummycolor->width) ||
			(dest->ddsd.dwHeight != dest->dummycolor->height))
			TextureManager__UploadTexture(This->texman, dest->dummycolor, 0, NULL,
			dest->ddsd.dwWidth, dest->ddsd.dwHeight, FALSE, TRUE);
		This->util->SetFBO(&dest->zfbo, dest->dummycolor, dest->texture, false);
	}
	This->util->SetViewport(0, 0, dest->ddsd.dwWidth, dest->ddsd.dwHeight);
	if (lpDestRect) This->util->SetScissor(true, lpDestRect->left, lpDestRect->top,
		lpDestRect->right, lpDestRect->bottom);
	This->util->DepthWrite(true);
	This->util->ClearDepth(lpDDBltFx->dwFillDepth / (double)0xFFFF);
	glClear(GL_DEPTH_BUFFER_BIT);
	if (lpDestRect)This->util->SetScissor(false, 0, 0, 0, 0);
}

void glRenderer__SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState)
{
	if (dwRendStateType > 152) return;
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

void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glDirectDrawSurface7 *Texture)
{
	if (This->texstages[dwStage].texture == Texture) return;
	This->texstages[dwStage].texture = Texture;
	if (Texture)
	{
		This->shaderstate3d.texstageid[dwStage] |= 1i64 << 59;
		if (Texture->ddsd.dwFlags & DDSD_CKSRCBLT) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 60;
		else This->shaderstate3d.texstageid[dwStage] &= 0xEFFFFFFFFFFFFFFFi64;
	}
	else This->shaderstate3d.texstageid[dwStage] &= 0xE7FFFFFFFFFFFFFFi64;
}

void glRenderer__SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue)
{
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
		This->texstages[dwStage].bumpenv00 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT01:
		This->texstages[dwStage].bumpenv01 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT10:
		This->texstages[dwStage].bumpenv10 = dwValue;
		break;
	case D3DTSS_BUMPENVMAT11:
		This->texstages[dwStage].bumpenv11 = dwValue;
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
	if (dtstTransformStateType > 23) return;
	memcpy(&This->transform[dtstTransformStateType], lpD3DMatrix, sizeof(D3DMATRIX));
}

void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	memcpy(&This->material, lpMaterial, sizeof(D3DMATERIAL));
}

void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove)
{
	int numlights = 0;
	int lightindex = 0;
	if(!remove)memcpy(&This->lights[index], light, sizeof(D3DLIGHT7));
	else ZeroMemory(&This->lights[index], sizeof(D3DLIGHT7));
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

void glRenderer__SetViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	memcpy(&This->viewport, lpViewport, sizeof(D3DVIEWPORT7));
}

void glRenderer__SetFogColor(glRenderer *This, DWORD color)
{
	if (color == This->fogcolor) return;
	This->fogcolor = color;
	GLfloat colors[4];
	colors[0] = (GLfloat)((color >> 16) & 255) / 255.0f;
	colors[1] = (GLfloat)((color >> 8) & 255) / 255.0f;
	colors[2] = (GLfloat)(color & 255) / 255.0f;
	colors[3] = (GLfloat)((color >> 24) & 255) / 255.0f;
	glFogfv(GL_FOG_COLOR, colors);
}

void glRenderer__SetFogStart(glRenderer *This, GLfloat start)
{
	if (start == This->fogstart) return;
	This->fogstart = start;
	glFogf(GL_FOG_START, start);
}

void glRenderer__SetFogEnd(glRenderer *This, GLfloat end)
{
	if (end == This->fogend) return;
	This->fogend = end;
	glFogf(GL_FOG_END, end);
}

void glRenderer__SetFogDensity(glRenderer *This, GLfloat density)
{
	if (density == This->fogdensity) return;
	This->fogdensity = density;
	glFogf(GL_FOG_DENSITY, density);
}

void glRenderer__SetDepthComp(glRenderer *This)
{
	switch (This->renderstate[D3DRENDERSTATE_ZFUNC])
	{
	case D3DCMP_NEVER:
		This->util->SetDepthComp(GL_NEVER);
		break;
	case D3DCMP_LESS:
		This->util->SetDepthComp(GL_LESS);
		break;
	case D3DCMP_EQUAL:
		This->util->SetDepthComp(GL_EQUAL);
		break;
	case D3DCMP_LESSEQUAL:
		This->util->SetDepthComp(GL_LEQUAL);
		break;
	case D3DCMP_GREATER:
		This->util->SetDepthComp(GL_GREATER);
		break;
	case D3DCMP_NOTEQUAL:
		This->util->SetDepthComp(GL_NOTEQUAL);
		break;
	case D3DCMP_GREATEREQUAL:
		This->util->SetDepthComp(GL_GEQUAL);
		break;
	case D3DCMP_ALWAYS:
	default:
		This->util->SetDepthComp(GL_ALWAYS);
		break;
	}
}

}