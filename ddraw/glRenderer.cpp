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
#include "BufferObject.h"
#include "glTexture.h"
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
static const DDPIXELFORMAT ddpf888_32 =
{
	sizeof(DDPIXELFORMAT),
	DDPF_RGB,
	0,
	32,
	0xFF0000,
	0xFF00,
	0xFF,
	0
};
static const DDSURFACEDESC2 ddsdBackbuffer =
{
	sizeof(DDSURFACEDESC2),
	DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT,
	1,
	256,
	256 * 4,
	0,
	0,
	0,
	0,
	NULL,
	nullckey,
	nullckey,
	nullckey,
	nullckey,
	ddpf888_32,
	ddscaps_offscreen,
	0,
};

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
	if(swap != This->oldswap)
	{
		This->ext->wglSwapIntervalEXT(swap);
		This->oldswap = This->ext->wglGetSwapIntervalEXT();
		This->oldswap = swap;
	}
}

/**
  * Initializes a glRenderer object
  * @param This
  *  Pointer to glRenderer object to initialize
  * @param width,height,bpp
  *  Width, height, and BPP of the rendering window
  * @param fullscreen
  *  TRUE if fullscreen mode is required, FALSE for windowed
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
	LONG_PTR winstyle, winstyleex;
	This->oldswap = 0;
	This->fogcolor = 0;
	This->fogstart = 0.0f;
	This->fogend = 1.0f;
	This->fogdensity = 1.0f;
	This->backbuffer = NULL;
	This->hDC = NULL;
	This->hRC = NULL;
	This->pbo = NULL;
	This->dib.enabled = false;
	This->hWnd = hwnd;
	This->rendertarget = NULL;
	InitializeCriticalSection(&This->cs);
	This->busy = CreateEvent(NULL,FALSE,FALSE,NULL);
	This->start = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(fullscreen)
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
	SetWindowPos(This->hWnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	This->RenderWnd = new glRenderWindow(width,height,fullscreen,This->hWnd,glDD7,devwnd);
	This->inputs[0] = (void*)width;
	This->inputs[1] = (void*)height;
	This->inputs[2] = (void*)bpp;
	This->inputs[3] = (void*)fullscreen;
	This->inputs[4] = (void*)frequency;
	This->inputs[5] = (void*)This->hWnd;
	This->inputs[6] = glDD7;
	This->inputs[7] = This;
	This->inputs[8] = (void*)devwnd;
	This->hThread = CreateThread(NULL, 0, glRenderer_ThreadEntry, This->inputs, 0, NULL);
	WaitForSingleObject(This->busy,INFINITE);
}

/**
  * Deletes a glRenderer object
  * @param This
  *  Pointer to glRenderer object
  */
void glRenderer_Delete(glRenderer *This)
{
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
void glRenderer_MakeTexture(glRenderer *This, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei fakex, GLsizei fakey)
{
	EnterCriticalSection(&This->cs);
	DDSURFACEDESC2 ddsd2 = *ddsd;
	This->inputs[0] = texture;
	This->inputs[1] = &ddsd2;
	This->inputs[2] = (void*)fakex;
	This->inputs[3] = (void*)fakey;
	This->opcode = OP_CREATE;
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
	WaitForSingleObject(This->busy, INFINITE);
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
	EnterCriticalSection(&This->cs);
	RECT r,r2;
	if(((cmd->dest->ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(cmd->dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((cmd->dest->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
		!(cmd->dest->ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
	{
		GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(This->RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
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
  * @param src
  *  Source surface to be updated
  * @param vsync
  *  Vertical sync count
  */
void glRenderer_DrawScreen(glRenderer *This, glTexture *texture, glTexture *src, GLint vsync)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = src;
	This->inputs[2] = (void*)vsync;
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
  * @param target
  *  Texture to be cleared
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
HRESULT glRenderer_Clear(glRenderer *This, glTexture *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = target;
	This->inputs[1] = (void*)dwCount;
	This->inputs[2] = lpRects;
	This->inputs[3] = (void*)dwFlags;
	This->inputs[4] = (void*)dwColor;
	memcpy(&This->inputs[5],&dvZ,4);
	This->inputs[6] = (void*)dwStencil;
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
	EnterCriticalSection(&This->cs);
	if(fullscreen && newwnd)
	{
		SetWindowLongPtrA(newwnd,GWL_EXSTYLE,WS_EX_APPWINDOW);
		SetWindowLongPtrA(newwnd,GWL_STYLE,WS_OVERLAPPED);
		ShowWindow(newwnd,SW_MAXIMIZE);
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
HRESULT glRenderer_DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = device;
	This->inputs[1] = (void*)mode;
	This->inputs[2] = vertices;
	This->inputs[3] = texformats;
	This->inputs[4] = (void*)count;
	This->inputs[5] = indices;
	This->inputs[6] = (void*)indexcount;
	This->inputs[7] = (void*)flags;
	This->opcode = OP_DRAWPRIMITIVES;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
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
	EnterCriticalSection(&This->cs);
	This->inputs[0] = fbo;
	This->opcode = OP_DELETEFBO;
	SetEvent(This->start);
	WaitForSingleObject(This->busy,INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
  * Updates the clipper stencil for a surface.
  * @param This
  *  Pointer to glRenderer object
  * @param stencil
  *  Stencil texture to update
  * @param size
  *  Size of clip list
  * @param vertices
  *  Vertices in BltVertex format for clip list
  * @param indices
  *  Vertex index list for clip list
  */
void glRenderer_UpdateClipper(glRenderer *This, glTexture *stencil, size_t size, BltVertex *vertices, GLshort *indices)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = stencil;
	This->inputs[1] = (void*)size;
	This->inputs[2] = vertices;
	This->inputs[3] = indices;
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
* @return
*  DD_OK if the depth fill succeeded.
*/
HRESULT glRenderer_DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = cmd;
	This->inputs[1] = parent;
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
*  Texture stage to bind
* @param Texture
*  Texture to bind to the stage; old texture will be released; NULL to unbind
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
* @param remove
*  TRUE to clear a light from the renderer.
*/

void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)index;
	This->inputs[1] = light;
	This->inputs[2] = (void*)remove;
	This->opcode = OP_SETLIGHT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
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
	EnterCriticalSection(&This->cs);
	This->inputs[0] = lpViewport;
	This->opcode = OP_SETVIEWPORT;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Attaches a Z-buffer to a texture.
* @param This
*  Pointer to glRenderer object
* @param parent
*  Parent texture to attach Z buffer
* @param attach
*  Z-buffer texture to attach
*/
void glRenderer_AttachZ(glRenderer *This, glTexture *parent, glTexture *attach)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = parent;
	This->inputs[1] = attach;
	This->opcode = OP_ATTACHZ;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Detaches a Z-buffer from a texture.
* @param This
*  Pointer to glRenderer object
* @param parent
*  Parent texture to attach Z buffer
*/
void glRenderer_DetachZ(glRenderer *This, glTexture *parent)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = parent;
	This->opcode = OP_DETACHZ;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Flips a stack of textures.
* @param This
*  Pointer to glRenderer object
* @param fliplist
*  Pointer to list of texture pointers in a flip list
* @param count
*  Number of textures in a flip list
* @param framebuffer
*  True if the first texture represents the framebuffer
* @param flags
*  DDraw flags for Vsync options
* @param flips
*  Number of flips to perform in this command
*/
void glRenderer_FlipTexture(glRenderer *This, glTexture **fliplist, DWORD count, BOOL framebuffer, DWORD flags, DWORD flips)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = fliplist;
	This->inputs[1] = (void*)count;
	This->inputs[2] = (void*)framebuffer;
	This->inputs[3] = (void*)flags;
	This->inputs[4] = (void*)flips;
	This->opcode = OP_FLIPTEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets the wrap mode for a texture
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set wrap mode on
* @param s
*  S-coordinate wrap mode
* @param t
*  T-coordinate wrap mode
*/
void glRenderer_SetTextureWrap(glRenderer *This, glTexture *texture, GLint s, GLint t)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)s;
	This->inputs[2] = (void*)t;
	This->opcode = OP_SETTEXTUREWRAP;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets the texture filter on a texture object.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set texture filter on
* @param mag
*  Magnification filter for texture
* @param min
*  Minification filter for texture
*/
void glRenderer_SetTextureFilter(glRenderer *This, glTexture *texture, GLint mag, GLint min)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)mag;
	This->inputs[2] = (void*)min;
	This->opcode = OP_SETTEXTUREFILTER;
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
*/
void glRenderer_SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)dwFlags;
	This->inputs[2] = lpDDColorKey;
	This->opcode = OP_SETTEXTURECOLORKEY;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Attaches or detaches a palette to a texture object
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set palette on
* @param palette
*  Texture representing a color palette
*/
void glRenderer_SetTexturePalette(glRenderer *This, glTexture *texture, glTexture *palette)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = palette;
	This->opcode = OP_SETTEXTUREPALETTE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}


/**
* Attaches or detaches a clipping stencil to a texture object
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to set palette on
* @param stencil
*  Texture representing a clipping stencil
*/
void glRenderer_SetTextureStencil(glRenderer *This, glTexture *texture, glTexture *stencil)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = stencil;
	This->opcode = OP_SETTEXTURESTENCIL;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Retrieves a pointer to a buffer representing texture data
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to lock
* @param r
*  Region of texture to lock; pointer will be to upper left of region; lock whole surface if NULL
* @param ddsd
*  DDSURFACEDESC2 structure to receive surface parameters and buffer pointer.
* @param flags
*  DDraw flags for surface locking
* @param miplevel
*  Mipmap level of texture to lock
* @return
*  DD_OK if call succeeded
*/
HRESULT glRenderer_LockTexture(glRenderer *This, glTexture *texture, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int miplevel)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = r;
	This->inputs[2] = ddsd;
	This->inputs[3] = (void*)flags;
	This->inputs[4] = (void*)miplevel;
	This->opcode = OP_LOCKTEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
* Releases a pointer to a locked texture.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to unlock
* @param r
*  Pointer to region previously used to lock the texture, or NULL to unlock whole texture
* @param miplevel
*  Mipmap level of texture to unlock
* @param primary
*  TRUE if the surface is the primary surface
* @param vsync
*  Wait for vertical blank if unlocking the primary surface
* @return
*  DD_OK if call succeeded
*/
HRESULT glRenderer_UnlockTexture(glRenderer *This, glTexture *texture, RECT *r, int miplevel, BOOL primary, int vsync)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = r;
	This->inputs[2] = (void*)miplevel;
	This->inputs[3] = (void*)primary;
	This->inputs[4] = (void*)vsync;
	This->opcode = OP_UNLOCKTEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HRESULT)This->outputs[0];
}

/**
* Retrieves a GDI Device Context representing a texture surface.  This function locks the entire texture.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to lock and retrieve Device Context from
* @param miplevel
*  Mipmap level of texture to retrieve Device Context for
* @return
*  Returns a GDI device context ready for drawing onto if the function succeeds.
*/
HDC glRenderer_GetTextureDC(glRenderer *This, glTexture *texture, int miplevel)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)miplevel;
	This->opcode = OP_GETTEXTUREDC;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
	return (HDC)This->outputs[0];
}

/**
* Releases a GDI Device Context from a texture surface and unlocks it.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to release GDI Device Context and unlock.
* @param miplevel
*  Mipmap level of texture to release Device Context from
* @param primary
*  TRUE if the surface is the primary surface
* @param vsync
*  Wait for vertical blank if unlocking the primary surface
*/
void glRenderer_ReleaseTextureDC(glRenderer *This, glTexture *texture, int miplevel, BOOL primary, int vsync)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->inputs[1] = (void*)miplevel;
	This->inputs[2] = (void*)primary;
	This->inputs[3] = (void*)vsync;
	This->opcode = OP_RELEASETEXTUREDC;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Recovers a texture that was created in a previous Rendering Context.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to restore
*/
void glRenderer_RestoreTexture(glRenderer *This, glTexture *texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_RESTORETEXTURE;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}


/**
* Sets the render target for D3D commands.
* @param This
*  Pointer to glRenderer object
* @param texture
*  Texture to use as a render target.
*/

void glRenderer_SetRenderTarget(glRenderer *This, glTexture *texture)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = texture;
	This->opcode = OP_SETRENDERTARGET;
	SetEvent(This->start);
	WaitForSingleObject(This->busy, INFINITE);
	LeaveCriticalSection(&This->cs);
}

/**
* Sets the default color depth for surfaces.
* @param This
*  Pointer to glRenderer object
* @param bpp
*  Default BPP to set for surfaces
*/
void glRenderer_SetBPP(glRenderer *This, int bpp)
{
	EnterCriticalSection(&This->cs);
	This->inputs[0] = (void*)bpp;
	This->opcode = OP_SETBPP;
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
	float tmpfloats[16];
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
					if (This->dib.hdc)
					{
						DeleteDC(This->dib.hdc);
						free(This->dib.info);
					}
					ZeroMemory(&This->dib,sizeof(DIB));
				}
				glUtil_DeleteFBO(This->util, &This->fbo);
				if(This->pbo)
				{
					BufferObject_Release(This->pbo);
					This->pbo = NULL;
				}
				if(This->backbuffer)
				{
					glTexture_Release(This->backbuffer, TRUE, NULL);
					This->backbuffer = NULL;
					This->backx = 0;
					This->backy = 0;
				}
				ShaderManager_Delete(This->shaders);
				glUtil_Release(This->util);
				free(This->shaders);
				free(This->ext);
				This->ext = NULL;
				wglMakeCurrent(NULL,NULL);
				wglDeleteContext(This->hRC);
				This->hRC = NULL;
			};
			if(This->hDC) ReleaseDC(This->RenderWnd->GetHWnd(),This->hDC);
			This->hDC = NULL;
			delete This->RenderWnd;
			This->RenderWnd = NULL;
			SetEvent(This->busy);
			return 0;
			break;
		case OP_SETWND:
			glRenderer__SetWnd(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2],
				(int)This->inputs[3],(unsigned int)This->inputs[4],(HWND)This->inputs[5],(BOOL)This->inputs[6]);
			break;
		case OP_CREATE:
			glRenderer__MakeTexture(This, (glTexture**)This->inputs[0], (const DDSURFACEDESC2*)This->inputs[1], (GLsizei)This->inputs[2],
				(GLsizei)This->inputs[3]);
			SetEvent(This->busy);
			break;
		case OP_DELETETEX:
			glRenderer__DeleteTexture(This,(glTexture*)This->inputs[0]);
			break;
		case OP_BLT:
			glRenderer__Blt(This, (BltCommand*)This->inputs[0]);
			break;
		case OP_DRAWSCREEN:
			glRenderer__DrawScreen(This,(glTexture*)This->inputs[0], (GLint)This->inputs[1],TRUE);
			break;
		case OP_INITD3D:
			glRenderer__InitD3D(This,(int)This->inputs[0],(int)This->inputs[1],(int)This->inputs[2]);
			break;
		case OP_CLEAR:
			memcpy(&tmpfloats[0],&This->inputs[5],4);
			glRenderer__Clear(This,(glTexture*)This->inputs[0],(DWORD)This->inputs[1],
				(LPD3DRECT)This->inputs[2],(DWORD)This->inputs[3],(DWORD)This->inputs[4],tmpfloats[0],(DWORD)This->inputs[6]);
			break;
		case OP_FLUSH:
			glRenderer__Flush(This);
			break;
		case OP_DRAWPRIMITIVES:
			glRenderer__DrawPrimitives(This,(glDirect3DDevice7*)This->inputs[0],(GLenum)This->inputs[1],
				(GLVERTEX*)This->inputs[2],(int*)This->inputs[3],(DWORD)This->inputs[4],(LPWORD)This->inputs[5],
				(DWORD)This->inputs[6],(DWORD)This->inputs[7]);
			break;
		case OP_DELETEFBO:
			glRenderer__DeleteFBO(This,(FBO*)This->inputs[0]);
			break;
		case OP_UPDATECLIPPER:
			glRenderer__UpdateClipper(This, (glTexture*)This->inputs[0], (size_t)This->inputs[1],
				(BltVertex*)This->inputs[2], (GLshort*)This->inputs[3]);
			break;
		case OP_DEPTHFILL:
			glRenderer__DepthFill(This, (BltCommand*)This->inputs[0], (glTexture*)This->inputs[1]);
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
			glRenderer__SetLight(This, (DWORD)This->inputs[0], (LPD3DLIGHT7)This->inputs[1], (BOOL)This->inputs[2]);
			break;
		case OP_SETVIEWPORT:
			glRenderer__SetViewport(This, (LPD3DVIEWPORT7)This->inputs[0]);
			break;
		case OP_ATTACHZ:
			glRenderer__AttachZ(This, (glTexture*)This->inputs[0], (glTexture*)This->inputs[1]);
			break;
		case OP_DETACHZ:
			glRenderer__DetachZ(This, (glTexture*)This->inputs[0]);
			break;
		case OP_FLIPTEXTURE:
			glRenderer__FlipTexture(This, (glTexture**)This->inputs[0], (DWORD)This->inputs[1], (BOOL)This->inputs[2],
				(DWORD)This->inputs[3], (DWORD)This->inputs[4]);
			break;
		case OP_SETTEXTUREWRAP:
			glRenderer__SetTextureWrap(This, (glTexture*)This->inputs[0], (GLint)This->inputs[1], (GLint)This->inputs[2]);
			break;
		case OP_SETTEXTUREFILTER:
			glRenderer__SetTextureFilter(This, (glTexture*)This->inputs[0], (GLint)This->inputs[1], (GLint)This->inputs[2]);
			break;
		case OP_SETTEXTURECOLORKEY:
			glRenderer__SetTextureColorKey(This, (glTexture*)This->inputs[0], (DWORD)This->inputs[1], (LPDDCOLORKEY)This->inputs[2]);
			break;
		case OP_SETTEXTUREPALETTE:
			glRenderer__SetTexturePalette(This, (glTexture*)This->inputs[0], (glTexture*)This->inputs[1]);
			break;
		case OP_SETTEXTURESTENCIL:
			glRenderer__SetTextureStencil(This, (glTexture*)This->inputs[0], (glTexture*)This->inputs[1]);
			break;
		case OP_LOCKTEXTURE:
			glRenderer__LockTexture(This, (glTexture*)This->inputs[0], (RECT*)This->inputs[1],
				(DDSURFACEDESC2*)This->inputs[2], (DWORD)This->inputs[3], (int)This->inputs[4]);
			break;
		case OP_UNLOCKTEXTURE:
			glRenderer__UnlockTexture(This, (glTexture*)This->inputs[0], (RECT*)This->inputs[1], (int)This->inputs[2],
				(BOOL)This->inputs[3], (int)This->inputs[4]);
			break;
		case OP_GETTEXTUREDC:
			glRenderer__GetTextureDC(This, (glTexture*)This->inputs[0], (int)This->inputs[1]);
			break;
		case OP_RELEASETEXTUREDC:
			glRenderer__ReleaseTextureDC(This, (glTexture*)This->inputs[0], (int)This->inputs[1],
				(BOOL)This->inputs[2], (int)This->inputs[3]);
			break;
		case OP_RESTORETEXTURE:
			glRenderer__RestoreTexture(This, (glTexture*)This->inputs[0], TRUE);
			break;
		case OP_SETRENDERTARGET:
			glRenderer__SetRenderTarget(This, (glTexture*)This->inputs[0]);
			break;
		case OP_SETBPP:
			glRenderer__SetBPP(This, (int)This->inputs[0]);
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
	glUtil_Create(This->ext, &This->util);
	glRenderer__SetSwap(This,1);
	glFinish();
	DXGLTimer_Init(&This->timer);
	DXGLTimer_Calibrate(&This->timer, height, frequency);
	glRenderer__SetSwap(This,0);
	glUtil_SetViewport(This->util,0,0,width,height);
	glViewport(0,0,width,height);
	glUtil_SetDepthRange(This->util,0.0,1.0);
	glUtil_DepthWrite(This->util,TRUE);
	glUtil_DepthTest(This->util,FALSE);
	glUtil_MatrixMode(This->util,GL_MODELVIEW);
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

void glRenderer__Blt(glRenderer *This, BltCommand *cmd)
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
	ddsd = cmd->dest->ddsd;
	ddsd.dwWidth = cmd->dest->mipmaps[cmd->destlevel].width;
	ddsd.dwHeight = cmd->dest->mipmaps[cmd->destlevel].height;
	if (!memcmp(&cmd->destrect, &nullrect, sizeof(RECT)))
	{
		destrect.left = 0;
		destrect.top = 0;
		destrect.right = ddsd.dwWidth;
		destrect.bottom = ddsd.dwHeight;
	}
	else destrect = cmd->destrect;
	if ((cmd->bltfx.dwSize == sizeof(DDBLTFX)) && (cmd->flags & DDBLT_ROP))
	{
		shaderid = PackROPBits(cmd->bltfx.dwROP, cmd->flags);
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 2) usedest = TRUE;
		if (rop_texture_usage[(cmd->bltfx.dwROP >> 16) & 0xFF] & 4) usepattern = TRUE;
	}
	else shaderid = cmd->flags & 0xF2FAADFF;
	if (cmd->flags & DDBLT_KEYDEST) usedest = TRUE;
	if (usedest)
	{
		ShaderManager_SetShader(This->shaders, PROG_TEXTURE, NULL, 0);
		glRenderer__DrawBackbufferRect(This, cmd->dest, destrect, PROG_TEXTURE);
		This->bltvertices[1].dests = This->bltvertices[3].dests = 0.;
		This->bltvertices[0].dests = This->bltvertices[2].dests = (GLfloat)(destrect.right - destrect.left) / (GLfloat)This->backx;
		This->bltvertices[0].destt = This->bltvertices[1].destt = 1.;
		This->bltvertices[2].destt = This->bltvertices[3].destt = 1.0-((GLfloat)(destrect.bottom - destrect.top) / (GLfloat)This->backy);
	}
	ShaderManager_SetShader(This->shaders, shaderid, NULL, 1);
	GenShader2D *shader = &This->shaders->gen2d->genshaders2D[This->shaders->gen3d->current_genshader];
	glUtil_BlendEnable(This->util, FALSE);
	do
	{
		if (glUtil_SetFBOTextures(This->util, &cmd->dest->mipmaps[cmd->destlevel].fbo, cmd->dest, NULL, cmd->destlevel, 0, NULL) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!cmd->dest->internalformats[1]) break;
		glTexture__Repair(cmd->dest);
		glUtil_SetFBO(This->util, NULL);
		cmd->dest->mipmaps[cmd->destlevel].fbo.fbcolor = NULL;
		cmd->dest->mipmaps[cmd->destlevel].fbo.fbz = NULL;
	} while (1);
	glUtil_SetViewport(This->util,0,0,cmd->dest->mipmaps[cmd->destlevel].bigx,
		cmd->dest->mipmaps[cmd->destlevel].bigy);
	glUtil_DepthTest(This->util, FALSE);
	DDSURFACEDESC2 ddsdSrc;
	ddsdSrc.dwSize = sizeof(DDSURFACEDESC2);
	if (cmd->src)
	{
		ddsdSrc = cmd->src->ddsd;
		ddsdSrc.dwWidth = cmd->src->mipmaps[cmd->srclevel].width;
		ddsdSrc.dwHeight = cmd->src->mipmaps[cmd->srclevel].height;
		if (cmd->src->mipmaps[cmd->srclevel].dirty & 1)
			glTexture__Upload(cmd->src, cmd->srclevel, FALSE, FALSE);
	}
	
	if (!memcmp(&cmd->srcrect, &nullrect, sizeof(RECT)))
	{
		srcrect.left = 0;
		srcrect.top = 0;
		srcrect.right = ddsdSrc.dwWidth;
		srcrect.bottom = ddsdSrc.dwHeight;
	}
	else srcrect = cmd->srcrect;
	This->bltvertices[1].x = This->bltvertices[3].x =
		(GLfloat)destrect.left * ((GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigx/(GLfloat)ddsd.dwWidth);
	This->bltvertices[0].x = This->bltvertices[2].x =
		(GLfloat)destrect.right * ((GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigx/(GLfloat)ddsd.dwWidth);
	This->bltvertices[0].y = This->bltvertices[1].y =
		(GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy-((GLfloat)destrect.top * ((GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy/(GLfloat)ddsd.dwHeight));
	This->bltvertices[2].y = This->bltvertices[3].y =
		(GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy-((GLfloat)destrect.bottom * ((GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy/(GLfloat)ddsd.dwHeight));
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)ddsdSrc.dwWidth;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)ddsdSrc.dwHeight;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)ddsdSrc.dwHeight;
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
		This->bltvertices[1].stencils = This->bltvertices[3].stencils = This->bltvertices[1].x / (GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigx;
		This->bltvertices[0].stencils = This->bltvertices[2].stencils = This->bltvertices[0].x / (GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigx;
		This->bltvertices[0].stencilt = This->bltvertices[1].stencilt = This->bltvertices[0].y / (GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy;
		This->bltvertices[2].stencilt = This->bltvertices[3].stencilt = This->bltvertices[2].y / (GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy;
	}
	if(cmd->dest->mipmaps[cmd->destlevel].fbo.fbz) glClear(GL_DEPTH_BUFFER_BIT);
	if (cmd->flags & DDBLT_COLORFILL) SetColorFillUniform(cmd->bltfx.dwFillColor, cmd->dest->colorsizes,
		cmd->dest->colororder, cmd->dest->colorbits, shader->shader.uniforms[12], This->ext);
	if ((cmd->flags & DDBLT_KEYSRC) && (cmd->src && (cmd->src->ddsd.dwFlags & DDSD_CKSRCBLT)) && !(cmd->flags & DDBLT_COLORFILL))
	{
		SetColorKeyUniform(cmd->src->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue, cmd->src->colorsizes,
			cmd->src->colororder, shader->shader.uniforms[5], cmd->src->colorbits, This->ext);
		if (cmd->flags & 0x20000000) SetColorKeyUniform(cmd->src->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue, cmd->src->colorsizes,
			cmd->src->colororder, shader->shader.uniforms[7], cmd->src->colorbits, This->ext);
	}
	if (!(cmd->flags & DDBLT_COLORFILL)) This->ext->glUniform1i(shader->shader.uniforms[1], 0);
	if ((cmd->flags & DDBLT_KEYDEST) && (This && (cmd->dest->ddsd.dwFlags & DDSD_CKDESTBLT)))
	{
		SetColorKeyUniform(cmd->dest->ddsd.ddckCKDestBlt.dwColorSpaceLowValue, cmd->dest->colorsizes,
			cmd->dest->colororder, shader->shader.uniforms[6], cmd->dest->colorbits, This->ext);
		if(cmd->flags & 0x40000000) SetColorKeyUniform(cmd->dest->ddsd.ddckCKDestBlt.dwColorSpaceHighValue, cmd->dest->colorsizes,
			cmd->dest->colororder, shader->shader.uniforms[8], cmd->dest->colorbits, This->ext);
	}
	if (usedest && (shader->shader.uniforms[2] != -1))
	{
		glUtil_SetTexture(This->util, 1, This->backbuffer);
		This->ext->glUniform1i(shader->shader.uniforms[2], 1);
	}
	if (usepattern && (shader->shader.uniforms[3] != -1))
	{
		glTexture *pattern = cmd->pattern;
		glUtil_SetTexture(This->util, 2, pattern);
		This->ext->glUniform1i(shader->shader.uniforms[3], 2);
		This->ext->glUniform2i(shader->shader.uniforms[9], pattern->ddsd.dwWidth, pattern->ddsd.dwHeight);
	}
	if (cmd->flags & 0x10000000)  // Use clipper
	{
		glUtil_SetTexture(This->util, 3, cmd->dest->stencil);
		This->ext->glUniform1i(shader->shader.uniforms[4],3);
		glUtil_EnableArray(This->util, shader->shader.attribs[5], TRUE);
		This->ext->glVertexAttribPointer(shader->shader.attribs[5], 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].stencils);
	}
	if(cmd->src)
	{
		glUtil_SetTexture(This->util,0,cmd->src);
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			if((dxglcfg.scalingfilter == 0) || (This->ddInterface->GetBPP() == 8))\
				glTexture__SetFilter(cmd->src, 0, GL_NEAREST, GL_NEAREST, This->ext, This->util);
			else glTexture__SetFilter(cmd->src, 0, GL_LINEAR, GL_LINEAR, This->ext, This->util);
		}
	}
	else glUtil_SetTexture(This->util,0,NULL);
	This->ext->glUniform4f(shader->shader.uniforms[0],0,
		(GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigx,0,(GLfloat)cmd->dest->mipmaps[cmd->destlevel].bigy);
	if(cmd->src) This->ext->glUniform4i(shader->shader.uniforms[10], cmd->src->colorsizes[0], cmd->src->colorsizes[1],
		cmd->src->colorsizes[2], cmd->src->colorsizes[3]);
	if(cmd->dest) This->ext->glUniform4i(shader->shader.uniforms[11], cmd->dest->colorsizes[0], cmd->dest->colorsizes[1],
		cmd->dest->colorsizes[2], cmd->dest->colorsizes[3]);
	cmd->dest->mipmaps[cmd->destlevel].dirty |= 2;
	glUtil_EnableArray(This->util, shader->shader.attribs[0], TRUE);
	This->ext->glVertexAttribPointer(shader->shader.attribs[0],2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	if(!(cmd->flags & DDBLT_COLORFILL))
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
	if (((ddsd.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER)) &&
		(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) ||
		((ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
			!(ddsd.ddsCaps.dwCaps & DDSCAPS_FLIP)))
		glRenderer__DrawScreen(This, cmd->dest, 0, FALSE);
	This->outputs[0] = DD_OK;
	SetEvent(This->busy);
}

void glRenderer__MakeTexture(glRenderer *This, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei fakex, GLsizei fakey)
{
	glTexture_Create(This->ext, This->util, texture, ddsd, fakex, fakey, This->hRC, This->bpp);
	// dirty hack; glDirectDraw interface enforces only one primary.
	if (ddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) This->primary = *texture;
}

void glRenderer__DrawBackbuffer(glRenderer *This, glTexture **texture, int x, int y, int progtype)
{
	DDSURFACEDESC2 ddsd = ddsdBackbuffer;
	GLfloat view[4];
	glUtil_SetActiveTexture(This->util,0);
	if(!This->backbuffer)
	{
		ddsd.dwWidth = x;
		ddsd.dwHeight = y;
		glTexture_Create(This->ext, This->util, &This->backbuffer, &ddsd, x, y, This->hRC, This->bpp);
		This->backx = x;
		This->backy = y;
	}
	if((This->backx != x) || (This->backy != y))
	{
		glTexture__Upload(This->backbuffer, 0, FALSE, TRUE);
		This->backx = x;
		This->backy = y;
	}
	glUtil_SetFBOTextures(This->util,&This->fbo,This->backbuffer,0,0,0,FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)x;
	view[3] = (GLfloat)y;
	glUtil_SetViewport(This->util,0,0,x,y);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glUtil_SetTexture(This->util,0,*texture);
	*texture = This->backbuffer;
	if (This->ext->GLEXT_ARB_sampler_objects)
		glTexture__SetFilter(NULL, 0, GL_LINEAR, GL_LINEAR, This->ext, This->util);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
	This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y;
	glUtil_EnableArray(This->util,This->shaders->shaders[progtype].pos,TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	glUtil_EnableArray(This->util,This->shaders->shaders[progtype].texcoord,TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	glUtil_SetCull(This->util,D3DCULL_NONE);
	glUtil_SetPolyMode(This->util,D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glUtil_SetFBO(This->util, NULL);
}

void glRenderer__DrawBackbufferRect(glRenderer *This, glTexture *texture, RECT srcrect, int progtype)
{
	DDSURFACEDESC2 ddsd = ddsdBackbuffer;
	GLfloat view[4];
	int x = srcrect.right - srcrect.left;
	int y = srcrect.bottom - srcrect.top;
	glUtil_SetActiveTexture(This->util, 0);
	if (!This->backbuffer)
	{
		ddsd.dwWidth = x;
		ddsd.dwHeight = y;
		glTexture_Create(This->ext, This->util, &This->backbuffer, &ddsd, x, y, This->hRC, This->bpp);
		This->backx = x;
		This->backy = y;
	}
	if ((This->backx < x) || (This->backy < y))
	{
		if (This->backx > x) x = This->backx;
		if (This->backx > y) y = This->backx;
		glTexture__Upload(This->backbuffer, 0, FALSE, TRUE);
		This->backx = x;
		This->backy = y;
	}
	glUtil_SetFBOTextures(This->util, &This->fbo, This->backbuffer, 0, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)This->backx;
	view[3] = (GLfloat)This->backy;
	glUtil_SetViewport(This->util, 0, 0, This->backx, This->backy);
	glUtil_SetScissor(This->util, TRUE, 0, 0, This->backx, This->backy);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUtil_SetScissor(This->util, FALSE, 0, 0, 0, 0);
	glUtil_SetTexture(This->util, 0, texture);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view, view[0], view[1], view[2], view[3]);
	This->bltvertices[1].s = This->bltvertices[3].s = (GLfloat)srcrect.left / (GLfloat)texture->ddsd.dwWidth;
	This->bltvertices[0].s = This->bltvertices[2].s = (GLfloat)srcrect.right / (GLfloat)texture->ddsd.dwWidth;
	This->bltvertices[0].t = This->bltvertices[1].t = (GLfloat)srcrect.top / (GLfloat)texture->ddsd.dwHeight;
	This->bltvertices[2].t = This->bltvertices[3].t = (GLfloat)srcrect.bottom / (GLfloat)texture->ddsd.dwHeight;
	This->bltvertices[1].x = This->bltvertices[3].x = 0.;
	This->bltvertices[0].x = This->bltvertices[2].x = (float)x;
	This->bltvertices[0].y = This->bltvertices[1].y = 0.;
	This->bltvertices[2].y = This->bltvertices[3].y = (float)y;
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(BltVertex), &This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP, 0, 3, 4, GL_UNSIGNED_SHORT, bltindices);
	glUtil_SetFBO(This->util, NULL);
}

void glRenderer__DrawScreen(glRenderer *This, glTexture *src, GLint vsync, BOOL setsync)
{
	int progtype;
	RECT r,r2;
	glUtil_BlendEnable(This->util, FALSE);
	if((src->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	{
		GetClientRect(This->hWnd,&r);
		GetClientRect(This->RenderWnd->GetHWnd(),&r2);
		if(memcmp(&r2,&r,sizeof(RECT)))
		SetWindowPos(This->RenderWnd->GetHWnd(),NULL,0,0,r.right,r.bottom,SWP_SHOWWINDOW);
	}
	glUtil_DepthTest(This->util, FALSE);
	RECT *viewrect = &r2;
	glRenderer__SetSwap(This,vsync);
	LONG sizes[6];
	GLfloat view[4];
	GLint viewport[4];
	if(src->mipmaps[0].dirty & 1)
	{
		glTexture__Upload(src, 0, TRUE, FALSE);
		src->mipmaps[0].dirty &= ~1;
	}
	if(src->ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
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
			view[2] = (GLfloat)src->mipmaps[0].bigy-(GLfloat)viewrect->top;
			view[3] = (GLfloat)src->mipmaps[0].bigy-(GLfloat)viewrect->bottom;
		}
	}
	else
	{
		view[0] = 0;
		view[1] = (GLfloat)src->mipmaps[0].bigx;
		view[2] = 0;
		view[3] = (GLfloat)src->mipmaps[0].bigy;
	}
	glUtil_SetFBO(This->util, NULL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if(This->ddInterface->GetBPP() == 8)
	{
		ShaderManager_SetShader(This->shaders,PROG_PAL256,NULL,0);
		progtype = PROG_PAL256;
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
		This->ext->glUniform1i(This->shaders->shaders[progtype].pal,1);
		glUtil_SetTexture(This->util,0,src);
		glUtil_SetTexture(This->util,1,src->palette);
		if(dxglcfg.scalingfilter)
		{
			glRenderer__DrawBackbuffer(This, &src, src->mipmaps[0].bigx, src->mipmaps[0].bigy, progtype);
			ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
			progtype = PROG_TEXTURE;
			glUtil_SetTexture(This->util,0,src);
			This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
		}
		if(This->ext->GLEXT_ARB_sampler_objects)
		{
			glTexture__SetFilter(NULL, 0, GL_NEAREST, GL_NEAREST, This->ext, This->util);
			glTexture__SetFilter(NULL, 1, GL_NEAREST, GL_NEAREST, This->ext, This->util);
		}
	}
	else
	{
		ShaderManager_SetShader(This->shaders,PROG_TEXTURE,NULL,0);
		progtype = PROG_TEXTURE;
		glUtil_SetTexture(This->util,0,src);
		This->ext->glUniform1i(This->shaders->shaders[progtype].tex0,0);
	}
	if(dxglcfg.scalingfilter && This->ext->GLEXT_ARB_sampler_objects)
		glTexture__SetFilter(NULL, 0, GL_LINEAR, GL_LINEAR, This->ext, This->util);
	else if(This->ext->GLEXT_ARB_sampler_objects)
		glTexture__SetFilter(NULL, 0, GL_NEAREST, GL_NEAREST, This->ext, This->util);
	glUtil_SetViewport(This->util,viewport[0],viewport[1],viewport[2],viewport[3]);
	This->ext->glUniform4f(This->shaders->shaders[progtype].view,view[0],view[1],view[2],view[3]);
	if(This->ddInterface->GetFullscreen())
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)sizes[0];
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)sizes[1];
	}
	else
	{
		This->bltvertices[0].x = This->bltvertices[2].x = (float)src->mipmaps[0].bigx;
		This->bltvertices[0].y = This->bltvertices[1].y = This->bltvertices[1].x = This->bltvertices[3].x = 0.;
		This->bltvertices[2].y = This->bltvertices[3].y = (float)src->mipmaps[0].bigy;
	}
	This->bltvertices[0].s = This->bltvertices[0].t = This->bltvertices[1].t = This->bltvertices[2].s = 1.;
	This->bltvertices[1].s = This->bltvertices[2].t = This->bltvertices[3].s = This->bltvertices[3].t = 0.;
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].pos,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].x);
	glUtil_EnableArray(This->util, This->shaders->shaders[progtype].texcoord, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[progtype].texcoord,2,GL_FLOAT,GL_FALSE,sizeof(BltVertex),&This->bltvertices[0].s);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLE_STRIP,0,3,4,GL_UNSIGNED_SHORT,bltindices);
	glFlush();
	if(This->hWnd) SwapBuffers(This->hDC);
	else
	{
		glReadBuffer(GL_FRONT);
		BufferObject_Bind(This->pbo, GL_PIXEL_PACK_BUFFER);
		GLint packalign;
		glGetIntegerv(GL_PACK_ALIGNMENT,&packalign);
		glPixelStorei(GL_PACK_ALIGNMENT,1);
		This->ddInterface->GetSizes(sizes);
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
	if(setsync) SetEvent(This->busy);

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
	SetEvent(This->busy);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	GLfloat ambient[] = {0.0,0.0,0.0,0.0};
	if (zbuffer) glUtil_DepthTest(This->util, TRUE);
	glUtil_SetDepthComp(This->util, GL_LEQUAL);
	GLfloat identity[16];
	__gluMakeIdentityf(identity);
	glUtil_SetMatrix(This->util, GL_MODELVIEW, identity, identity, NULL);
	glUtil_SetMatrix(This->util, GL_PROJECTION, identity, NULL, NULL);
	for (int i = 0; i < 24; i++)
		memcpy(&This->transform[i], identity, sizeof(D3DMATRIX));
	GLfloat one[4] = {1,1,1,1};
	GLfloat zero[4] = {0,0,0,1};
	glUtil_SetMaterial(This->util, one, one, zero, zero, 0);
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

void glRenderer__Clear(glRenderer *This, glTexture *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil)
{
	This->outputs[0] = (void*)D3D_OK;
	GLfloat color[4];
	dwordto4float(dwColor,color);
	do
	{
		if (glUtil_SetFBOSurface(This->util, target) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!target->internalformats[1]) break;
		glTexture__Repair(target);
		glUtil_SetFBO(This->util, NULL);
		target->mipmaps[0].fbo.fbcolor = NULL;
		target->mipmaps[0].fbo.fbz = NULL;
	} while (1);
	int clearbits = 0;
	if(dwFlags & D3DCLEAR_TARGET)
	{
		clearbits |= GL_COLOR_BUFFER_BIT;
		glUtil_ClearColor(This->util, color[0], color[1], color[2], color[3]);
	}
	if(dwFlags & D3DCLEAR_ZBUFFER)
	{
		clearbits |= GL_DEPTH_BUFFER_BIT;
		glUtil_ClearDepth(This->util, dvZ);
		glUtil_DepthWrite(This->util, TRUE);
	}
	if(dwFlags & D3DCLEAR_STENCIL)
	{
		clearbits |= GL_STENCIL_BUFFER_BIT;
		glUtil_ClearStencil(This->util, dwStencil);
	}
	if(dwCount)
	{
		for(DWORD i = 0; i < dwCount; i++)
		{
			glUtil_SetScissor(This->util, TRUE, lpRects[i].x1, lpRects[i].y1, lpRects[i].x2, lpRects[i].y2);
			glClear(clearbits);
		}
		glUtil_SetScissor(This->util, false, 0, 0, 0, 0);
	}
	else glClear(clearbits);
	if(target->zbuffer) target->zbuffer->mipmaps[0].dirty |= 2;
	target->mipmaps[0].dirty |= 2;
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
		glUtil_SetViewport(This->util, 0, 0, width, height);
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

void glRenderer__DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
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
	if(This->renderstate[D3DRENDERSTATE_ZENABLE]) glUtil_DepthTest(This->util, TRUE);
	else glUtil_DepthTest(This->util, FALSE);
	if (This->renderstate[D3DRENDERSTATE_ZWRITEENABLE]) glUtil_DepthWrite(This->util, TRUE);
	else glUtil_DepthWrite(This->util, FALSE);
	_GENSHADER *prog = &This->shaders->gen3d->genshaders[This->shaders->gen3d->current_genshader].shader;
	glUtil_EnableArray(This->util, prog->attribs[0], TRUE);
	This->ext->glVertexAttribPointer(prog->attribs[0],3,GL_FLOAT,GL_FALSE,vertices[0].stride,vertices[0].data);
	if(transformed)
	{
		if(prog->attribs[1] != -1)
		{
			glUtil_EnableArray(This->util, prog->attribs[1], TRUE);
			This->ext->glVertexAttribPointer(prog->attribs[1],4,GL_FLOAT,GL_FALSE,vertices[1].stride,vertices[1].data);
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
	glUtil_SetMatrix(This->util, GL_MODELVIEW, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_VIEW],
		(GLfloat*)&This->transform[D3DTRANSFORMSTATE_WORLD],NULL);
	glUtil_SetMatrix(This->util, GL_PROJECTION, (GLfloat*)&This->transform[D3DTRANSFORMSTATE_PROJECTION], NULL, NULL);

	glUtil_SetMaterial(This->util, (GLfloat*)&This->material.ambient, (GLfloat*)&This->material.diffuse, (GLfloat*)&This->material.specular,
		(GLfloat*)&This->material.emissive, This->material.power);

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
			if(This->texstages[i].texture->mipmaps[0].dirty & 1)
			{
				glTexture__Upload(This->texstages[i].texture, 0, FALSE, FALSE);
				This->texstages[i].texture->mipmaps[0].dirty &= ~1;
			}
			if(This->texstages[i].texture)
				glTexture__SetFilter(This->texstages[i].texture, i, This->texstages[i].glmagfilter, This->texstages[i].glminfilter, NULL, NULL);
			glUtil_SetTexture(This->util,i,This->texstages[i].texture);
			glUtil_SetWrap(This->util, i, 0, This->texstages[i].addressu);
			glUtil_SetWrap(This->util, i, 1, This->texstages[i].addressv);
		}
		glUtil_SetTexture(This->util,i,0);
		This->ext->glUniform1i(prog->uniforms[128+i],i);
		if(This->renderstate[D3DRENDERSTATE_COLORKEYENABLE] && This->texstages[i].texture && (prog->uniforms[142+i] != -1))
		{
			if(This->texstages[i].texture->ddsd.dwFlags & DDSD_CKSRCBLT)
			{
				SetColorKeyUniform(This->texstages[i].texture->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue,
					This->texstages[i].texture->colorsizes, This->texstages[i].texture->colororder,
					prog->uniforms[142 + i], This->texstages[i].texture->colorbits, This->ext);
				This->ext->glUniform4i(prog->uniforms[153+i], This->texstages[i].texture->colorsizes[0], 
					This->texstages[i].texture->colorsizes[1],
					This->texstages[i].texture->colorsizes[2],
					This->texstages[i].texture->colorsizes[3]);
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
		if (glUtil_SetFBOSurface(This->util, This->rendertarget) == GL_FRAMEBUFFER_COMPLETE) break;
		if (!This->rendertarget->internalformats[1]) break;
		glTexture__Repair(This->rendertarget);
		glUtil_SetFBO(This->util, NULL);
		This->rendertarget->mipmaps[0].fbo.fbcolor = NULL;
		This->rendertarget->mipmaps[0].fbo.fbz = NULL;
	} while (1);
	glUtil_SetViewport(This->util, (int)((float)This->viewport.dwX*device->glDDS7->mulx),
		(int)((float)This->viewport.dwY*device->glDDS7->muly),
		(int)((float)This->viewport.dwWidth*device->glDDS7->mulx),
		(int)((float)This->viewport.dwHeight*device->glDDS7->muly));
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
	if(This->rendertarget->zbuffer) This->rendertarget->zbuffer->mipmaps[0].dirty |= 2;
	This->rendertarget->mipmaps[0].dirty |= 2;
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

void glRenderer__UpdateClipper(glRenderer *This, glTexture *stencil, size_t size, BltVertex *vertices, GLshort *indices)
{
	GLfloat view[4];
	glUtil_SetFBOTextures(This->util, &stencil->mipmaps[0].fbo, stencil, NULL, 0, 0, FALSE);
	view[0] = view[2] = 0;
	view[1] = (GLfloat)stencil->ddsd.dwWidth;
	view[3] = (GLfloat)stencil->ddsd.dwHeight;
	glUtil_SetViewport(This->util, 0, 0, stencil->ddsd.dwWidth, stencil->ddsd.dwHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	ShaderManager_SetShader(This->shaders,PROG_CLIPSTENCIL,NULL,0);
	This->ext->glUniform4f(This->shaders->shaders[PROG_CLIPSTENCIL].view,view[0],view[1],view[2],view[3]);
	glUtil_EnableArray(This->util, This->shaders->shaders[PROG_CLIPSTENCIL].pos, TRUE);
	This->ext->glVertexAttribPointer(This->shaders->shaders[PROG_CLIPSTENCIL].pos,
		2,GL_FLOAT,false,sizeof(BltVertex),&vertices[0].x);
	glUtil_SetCull(This->util, D3DCULL_NONE);
	glUtil_SetPolyMode(This->util, D3DFILL_SOLID);
	This->ext->glDrawRangeElements(GL_TRIANGLES, 0, (6 * size) - 1,
		6 * size, GL_UNSIGNED_SHORT, indices);
	glUtil_SetFBO(This->util, NULL);
	SetEvent(This->busy);
}

void glRenderer__DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent)
{
	RECT destrect;
	DDSURFACEDESC2 ddsd;
	DDSURFACEDESC2 tmpddsd;
	BOOL usedestrect = FALSE;
	ddsd.dwSize = sizeof(DDSURFACEDESC2);
	ddsd = cmd->dest->ddsd;
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
			if (glUtil_SetFBOSurface(This->util, parent) == GL_FRAMEBUFFER_COMPLETE) break;
			if (!parent->internalformats[1]) break;
			glTexture__Repair(parent);
			glUtil_SetFBO(This->util, NULL);
			parent->mipmaps[0].fbo.fbcolor = NULL;
			parent->mipmaps[0].fbo.fbz = NULL;
		} while (1);
	}
	else
	{
		if (!cmd->dest->dummycolor)
		{
			tmpddsd = ddsdBackbuffer;
			tmpddsd.dwWidth = cmd->dest->ddsd.dwWidth;
			tmpddsd.dwHeight = cmd->dest->ddsd.dwHeight;
			glTexture_Create(This->ext, This->util, &cmd->dest->dummycolor, &tmpddsd,
				cmd->dest->mipmaps[0].bigx, cmd->dest->mipmaps[0].bigy, This->hRC, This->bpp);
		}
		if ((cmd->dest->ddsd.dwWidth != cmd->dest->dummycolor->ddsd.dwWidth) ||
			(cmd->dest->ddsd.dwHeight != cmd->dest->dummycolor->ddsd.dwHeight))
		{
			tmpddsd = cmd->dest->ddsd;
			tmpddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
			tmpddsd.dwWidth = cmd->dest->ddsd.dwWidth;
			tmpddsd.dwHeight = cmd->dest->ddsd.dwHeight;
			glTexture__Modify(cmd->dest->dummycolor, &tmpddsd, cmd->dest->mipmaps[0].bigx, cmd->dest->mipmaps[0].bigy, FALSE);
		}
		glUtil_SetFBOTextures(This->util, &cmd->dest->mipmaps[cmd->destlevel].fbo, cmd->dest->dummycolor, cmd->dest, cmd->destlevel, 0, FALSE);
	}
	glUtil_SetViewport(This->util, 0, 0, cmd->dest->mipmaps[cmd->destlevel].width, cmd->dest->mipmaps[cmd->destlevel].height);
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

void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture)
{
	if (This->texstages[dwStage].texture == Texture)
	{
		SetEvent(This->busy);
		return;
	}
	if (This->texstages[dwStage].texture) glTexture_Release(This->texstages[dwStage].texture, TRUE, NULL);
	This->texstages[dwStage].texture = Texture;
	if (Texture)
	{
		glTexture_AddRef(Texture);
		This->shaderstate3d.texstageid[dwStage] |= 1i64 << 59;
		if (Texture->ddsd.dwFlags & DDSD_CKSRCBLT) This->shaderstate3d.texstageid[dwStage] |= 1i64 << 60;
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
	if (dtstTransformStateType > 23)
	{
		SetEvent(This->busy);
		return;
	}
	memcpy(&This->transform[dtstTransformStateType], lpD3DMatrix, sizeof(D3DMATRIX));
	SetEvent(This->busy);
}

void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial)
{
	memcpy(&This->material, lpMaterial, sizeof(D3DMATERIAL));
	SetEvent(This->busy);
}

void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove)
{
	int numlights = 0;
	int lightindex = 0;
	if(!remove)memcpy(&This->lights[index], light, sizeof(D3DLIGHT7));
	else ZeroMemory(&This->lights[index], sizeof(D3DLIGHT7));
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

void glRenderer__SetViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport)
{
	memcpy(&This->viewport, lpViewport, sizeof(D3DVIEWPORT7));
	SetEvent(This->busy);
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

void glRenderer__AttachZ(glRenderer *This, glTexture *parent, glTexture *attach)
{
	if (parent->zbuffer) glTexture_Release(parent->zbuffer, TRUE, This);
	parent->zbuffer = attach;
	if (parent->zbuffer) glTexture_AddRef(parent->zbuffer);
	SetEvent(This->busy);
}

void glRenderer__DetachZ(glRenderer *This, glTexture *parent)
{
	if (parent->zbuffer) glTexture_Release(parent->zbuffer, TRUE, This);
	parent->zbuffer = NULL;
	SetEvent(This->busy);
}

void glRenderer__FlipTexture(glRenderer *This, glTexture **fliplist, DWORD count, BOOL framebuffer, DWORD flags, DWORD flips)
{
	int i, j, k;
	int vsync = 0;
	GLuint tmp = fliplist[0]->id;
	MIPMAP tmp2[15];
	int mipcount = fliplist[0]->ddsd.dwMipMapCount;
	memcpy(&tmp2, &fliplist[0]->mipmaps, mipcount*sizeof(MIPMAP));
	if (!mipcount) mipcount = 1;
	for (k = 0; k < mipcount; k++)
	{
		if (fliplist[0]->mipmaps[k].dirty & 1)
			glTexture__Upload(fliplist[0], k, FALSE, FALSE);
		fliplist[0]->mipmaps[k].dirty |= 2;
	}
	for (i = 0; i < flips; i++)
	{
		for (j = 0; j < count-1; j++)
		{
			for (k = 0; k < mipcount; k++)
			{
				if (fliplist[j]->mipmaps[k].dirty & 1)
					glTexture__Upload(fliplist[j], k, FALSE, FALSE);
				fliplist[j]->mipmaps[k].dirty |= 2;
			}
			//memcpy(&fliplist[j]->mipmaps, &fliplist[j + 1]->mipmaps, mipcount*sizeof(MIPMAP));
			fliplist[j]->id = fliplist[j + 1]->id;
		}
		fliplist[count - 1]->id = tmp;
		//memcpy(&fliplist[count - 1]->mipmaps, &tmp2, mipcount*sizeof(MIPMAP));
	}
	if (framebuffer)
	{
		if (!(flags & DDCAPS2_FLIPNOVSYNC))
		{
			if ((flags & 0x0F000000) == DDFLIP_INTERVAL2) vsync = 2;
			else if ((flags & 0x0F000000) == DDFLIP_INTERVAL3) vsync = 3;
			else if ((flags & 0x0F000000) == DDFLIP_INTERVAL4) vsync = 4;
			else vsync = 1;
		}
		if (!vsync)
		{
			glRenderer__DrawScreen(This, fliplist[0], 0, FALSE);
		}
		else
		{
			for (i = 0; i < vsync; i++)
			{
				glRenderer__DrawScreen(This, fliplist[0], 1, FALSE);
			}
		}
	}
	SetEvent(This->busy);
}

void glRenderer__SetTextureWrap(glRenderer *This, glTexture *texture, GLint s, GLint t)
{
	texture->wraps = s;
	texture->wrapt = t;
	SetEvent(This->busy);
}

void glRenderer__SetTextureFilter(glRenderer *This, glTexture *texture, GLint mag, GLint min)
{
	glTexture__SetFilter(texture, 0, mag, min, This->ext, This->util);
	SetEvent(This->busy);
}

void glRenderer__SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if (dwFlags & DDCKEY_SRCBLT)
	{
		texture->ddsd.dwFlags |= DDSD_CKSRCBLT;
		texture->ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		else texture->ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
	}
	if (dwFlags & DDCKEY_DESTBLT)
	{
		texture->ddsd.dwFlags |= DDSD_CKDESTBLT;
		texture->ddsd.ddckCKDestBlt.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		else texture->ddsd.ddckCKDestBlt.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
	}
	if (dwFlags & DDCKEY_SRCOVERLAY)
	{
		texture->ddsd.dwFlags |= DDSD_CKSRCOVERLAY;
		texture->ddsd.ddckCKSrcOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		else texture->ddsd.ddckCKSrcOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
	}
	if (dwFlags & DDCKEY_DESTOVERLAY)
	{
		texture->ddsd.dwFlags |= DDSD_CKDESTOVERLAY;
		texture->ddsd.ddckCKDestOverlay.dwColorSpaceLowValue = lpDDColorKey->dwColorSpaceLowValue;
		if (DDCKEY_COLORSPACE) texture->ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceLowValue;
		else texture->ddsd.ddckCKDestOverlay.dwColorSpaceHighValue = lpDDColorKey->dwColorSpaceHighValue;
	}
	SetEvent(This->busy);
}

void glRenderer__SetTexturePalette(glRenderer *This, glTexture *texture, glTexture *palette)
{
	if (texture->palette) glTexture_Release(texture->palette, TRUE, This);
	texture->palette = palette;
	if (texture->palette) glTexture_AddRef(texture->palette);
	SetEvent(This->busy);
}

void glRenderer__SetTextureStencil(glRenderer *This, glTexture *texture, glTexture *stencil)
{
	if (texture->stencil) glTexture_Release(texture->stencil, TRUE, This);
	texture->stencil = stencil;
	if (texture->stencil) glTexture_AddRef(texture->stencil);
	SetEvent(This->busy);
}

void glRenderer__LockTexture(glRenderer *This, glTexture *texture, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int miplevel)
{
	This->outputs[0] = (void*)glTexture__Lock(texture, r, ddsd, flags, miplevel);
	SetEvent(This->busy);
}

void glRenderer__UnlockTexture(glRenderer *This, glTexture *texture, RECT *r, int miplevel, BOOL primary, int vsync)
{
	glTexture__Unlock(texture, r, miplevel);
	This->outputs[0] = DD_OK;
	SetEvent(This->busy);
	if (primary) glRenderer__DrawScreen(This, texture, vsync, FALSE);
}
void glRenderer__GetTextureDC(glRenderer *This, glTexture *texture, int miplevel)
{
	This->outputs[0] = glTexture__GetDC(texture, This, miplevel);
	SetEvent(This->busy);
}

void glRenderer__ReleaseTextureDC(glRenderer *This, glTexture *texture, int miplevel, BOOL primary, int vsync)
{
	glTexture__ReleaseDC(texture, miplevel);
	SetEvent(This->busy);
	if (primary) glRenderer__DrawScreen(This, texture, vsync, FALSE);
}

void glRenderer__RestoreTexture(glRenderer *This, glTexture *texture, BOOL setsync)
{
	if (texture->hrc == This->hRC)
	{
		if(setsync) SetEvent(This->busy);
		return;
	}
	glTexture__SetPixelFormat(texture);
	glTexture__CreateSimple(texture);
	if (texture->zbuffer) glRenderer__RestoreTexture(This, texture->zbuffer, FALSE);
	if (texture->palette) glRenderer__RestoreTexture(This, texture->palette, FALSE);
	if (texture->stencil) glRenderer__RestoreTexture(This, texture->stencil, FALSE);
	if (texture->dummycolor) glRenderer__RestoreTexture(This, texture->dummycolor, FALSE);
	if(setsync) SetEvent(This->busy);
}

void glRenderer__SetRenderTarget(glRenderer *This, glTexture *texture)
{
	if (This->rendertarget) glTexture_Release(This->rendertarget, TRUE, This);
	This->rendertarget = texture;
	if (This->rendertarget) glTexture_AddRef(This->rendertarget);
	SetEvent(This->busy);
}

void glRenderer__SetBPP(glRenderer *This, int bpp)
{
	This->bpp = bpp;
	SetEvent(This->busy);
}

}