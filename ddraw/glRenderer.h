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

#ifndef _GLRENDERER_H
#define _GLRENDERER_H

typedef struct
{
	float Version;
	float ShaderVer;
	GLint TextureMax;
} GLCAPS;

typedef struct
{
	void *data;
	int stride;
} GLVERTEX;

typedef struct
{
	__int64 stateid;
	__int64 texstageid[8];
} SHADERSTATE;

typedef struct TEXTURESTAGE
{
	D3DTEXTUREOP colorop;
	DWORD colorarg1;
	DWORD colorarg2;
	D3DTEXTUREOP alphaop;
	DWORD alphaarg1;
	DWORD alphaarg2;
	D3DVALUE bumpenv00;
	D3DVALUE bumpenv01;
	D3DVALUE bumpenv10;
	D3DVALUE bumpenv11;
	DWORD texcoordindex;
	D3DTEXTUREADDRESS addressu;
	D3DTEXTUREADDRESS addressv;
	DWORD bordercolor;
	D3DTEXTUREMAGFILTER magfilter;
	D3DTEXTUREMINFILTER minfilter;
	D3DTEXTUREMIPFILTER mipfilter;
	D3DVALUE lodbias;
	DWORD miplevel;
	DWORD anisotropy;
	D3DVALUE bumpenvlscale;
	D3DVALUE bumpenvloffset;
	D3DTEXTURETRANSFORMFLAGS textransform;
	glTexture *texture;
	GLint glmagfilter;
	GLint glminfilter;
} TEXTURESTAGE;

#define OP_NULL						0
#define OP_SETWND					1
#define OP_DELETE					2
#define OP_CREATE					3
#define OP_DELETETEX				4
#define OP_BLT						5
#define OP_DRAWSCREEN				6
#define OP_INITD3D					7
#define OP_CLEAR					8
#define OP_FLUSH					9
#define OP_DRAWPRIMITIVES			10
#define OP_DELETEFBO				11
#define OP_UPDATECLIPPER			12
#define OP_DEPTHFILL				13
#define OP_SETRENDERSTATE			14
#define OP_SETTEXTURE				15
#define OP_SETTEXTURESTAGESTATE		16
#define OP_SETTRANSFORM				17
#define OP_SETMATERIAL				18
#define OP_SETLIGHT					19
#define OP_SETVIEWPORT				20
#define OP_ATTACHZ					21
#define OP_DETACHZ					22
#define OP_FLIPTEXTURE				23
#define OP_SETTEXTUREWRAP			24
#define OP_SETTEXTUREFILTER			25
#define OP_SETTEXTURECOLORKEY		26
#define OP_SETTEXTUREPALETTE		27
#define OP_SETTEXTURESTENCIL		28
#define OP_LOCKTEXTURE				29
#define OP_UNLOCKTEXTURE			30
#define OP_GETTEXTUREDC				31
#define OP_RELEASETEXTUREDC			32
#define OP_RESTORETEXTURE			33
#define OP_SETRENDERTARGET			34
#define OP_SETBPP					35

extern const DWORD renderstate_default[153];
extern const TEXTURESTAGE texstagedefault0;
extern const TEXTURESTAGE texstagedefault1;

#ifdef __cplusplus
class glDirectDraw7;
class glDirect3DDevice7;
class glRenderWindow;
struct glDirectDrawClipper;
extern "C" {
#else
typedef int glDirectDraw7;
typedef int glDirect3DDevice7;
typedef int glRenderWindow;
#endif

/** @brief glRenderer struct
  * OpenGL renderer structure for DXGL.
  */
typedef struct glRenderer
{
	HGLRC hRC;
	GLCAPS gl_caps;
	glExtensions *ext;
	glDirectDraw7 *ddInterface;
	int opcode;
	void* inputs[32];
	void* outputs[32];
	HANDLE hThread;
	HDC hDC;
	HWND hWnd;
	glRenderWindow *RenderWnd;
	DIB dib;
	FBO fbo;
	BufferObject *pbo;
	CRITICAL_SECTION cs;
	HANDLE busy;
	HANDLE start;
	unsigned int frequency;
	DXGLTimer timer;
	glTexture *backbuffer;
	int backx;
	int backy;
	int bpp;
	DWORD fogcolor;
	GLfloat fogstart;
	GLfloat fogend;
	GLfloat fogdensity;
	BltVertex bltvertices[4];
	int oldswap;
	glUtil *util;
	ShaderManager *shaders;
	DWORD renderstate[153];
	SHADERSTATE shaderstate3d;
	TEXTURESTAGE texstages[8];
	D3DMATERIAL7 material;
	D3DLIGHT7 lights[8];
	D3DMATRIX transform[24];
	D3DVIEWPORT7 viewport;
	glTexture *rendertarget;
	glTexture *primary;
} glRenderer;

void glRenderer_Init(glRenderer *This, int width, int height, int bpp, BOOL fullscreen, unsigned int frequency, HWND hwnd, glDirectDraw7 *glDD7, BOOL devwnd);
void glRenderer_Delete(glRenderer *This);
static DWORD WINAPI glRenderer_ThreadEntry(void *entry);
HRESULT glRenderer_Blt(glRenderer *This, BltCommand *cmd);
void glRenderer_MakeTexture(glRenderer *This, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei fakex, GLsizei fakey);
void glRenderer_DrawScreen(glRenderer *This, glTexture *texture, glTexture *src, GLint vsync);
void glRenderer_DeleteTexture(glRenderer *This, glTexture *texture);
void glRenderer_InitD3D(glRenderer *This, int zbuffer, int x, int y);
void glRenderer_Flush(glRenderer *This);
void glRenderer_SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd);
HRESULT glRenderer_Clear(glRenderer *This, glTexture *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
HRESULT glRenderer_DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer_DeleteFBO(glRenderer *This, FBO *fbo);
void glRenderer_UpdateClipper(glRenderer *This, glTexture *stencil, size_t size, BltVertex *vertices, GLshort *indices);
unsigned int glRenderer_GetScanLine(glRenderer *This);
HRESULT glRenderer_DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent);
void glRenderer_SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
void glRenderer_SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture);
void glRenderer_SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
void glRenderer_SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
void glRenderer_SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial);
void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove);
void glRenderer_SetViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport);
void glRenderer_AttachZ(glRenderer *This, glTexture *parent, glTexture *attach);
void glRenderer_DetachZ(glRenderer *This, glTexture *parent);
void glRenderer_FlipTexture(glRenderer *This, glTexture **fliplist, DWORD count, BOOL framebuffer, DWORD flags, DWORD flips);
void glRenderer_SetTextureWrap(glRenderer *This, glTexture *texture, GLint s, GLint t);
void glRenderer_SetTextureFilter(glRenderer *This, glTexture *texture, GLint mag, GLint min);
void glRenderer_SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
void glRenderer_SetTexturePalette(glRenderer *This, glTexture *texture, glTexture *palette);
void glRenderer_SetTextureStencil(glRenderer *This, glTexture *texture, glTexture *stencil);
HRESULT glRenderer_LockTexture(glRenderer *This, glTexture *texture, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int miplevel);
HRESULT glRenderer_UnlockTexture(glRenderer *This, glTexture *texture, RECT *r, int miplevel, BOOL primary, int vsync);
HDC glRenderer_GetTextureDC(glRenderer *This, glTexture *texture, int miplevel);
void glRenderer_ReleaseTextureDC(glRenderer *This, glTexture *texture, int miplevel, BOOL primary, int vsync);
void glRenderer_RestoreTexture(glRenderer *This, glTexture *texture);
void glRenderer_SetRenderTarget(glRenderer *This, glTexture *texture);
void glRenderer_SetBPP(glRenderer *This, int bpp);
// In-thread APIs
DWORD glRenderer__Entry(glRenderer *This);
BOOL glRenderer__InitGL(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7);
void glRenderer__Blt(glRenderer *This, BltCommand *cmd);
void glRenderer__MakeTexture(glRenderer *This, glTexture **texture, const DDSURFACEDESC2 *ddsd, GLsizei fakex, GLsizei fakey);
void glRenderer__DrawScreen(glRenderer *This, glTexture *src, GLint vsync, BOOL setsync);
void glRenderer__DeleteTexture(glRenderer *This, glTexture *texture);
void glRenderer__DrawBackbuffer(glRenderer *This, glTexture **texture, int x, int y, int progtype);
void glRenderer__DrawBackbufferRect(glRenderer *This, glTexture *texture, RECT srcrect, int progtype);
void glRenderer__InitD3D(glRenderer *This, int zbuffer, int x, int y);
void glRenderer__Clear(glRenderer *This, glTexture *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
void glRenderer__DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texcormats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer__Flush(glRenderer *This);
void glRenderer__SetWnd(glRenderer *This, int width, int height, int fullscreen, int bpp, unsigned int frequency, HWND newwnd, BOOL devwnd);
void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo);
void glRenderer__UpdateClipper(glRenderer *This, glTexture *stencil, size_t size, BltVertex *vertices, GLshort *indices);
void glRenderer__DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent);
void glRenderer__SetFogColor(glRenderer *This, DWORD color);
void glRenderer__SetFogStart(glRenderer *This, GLfloat start);
void glRenderer__SetFogEnd(glRenderer *This, GLfloat end);
void glRenderer__SetFogDensity(glRenderer *This, GLfloat density);
void glRenderer__SetSwap(glRenderer *This, int swap);
void glRenderer__SetBlend(glRenderer *This, DWORD src, DWORD dest);
void glRenderer__SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture);
void glRenderer__SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
void glRenderer__SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial);
void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light, BOOL remove);
void glRenderer__SetViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport);
void glRenderer__SetDepthComp(glRenderer *This);
void glRenderer__AttachZ(glRenderer *This, glTexture *parent, glTexture *attach);
void glRenderer__DetachZ(glRenderer *This, glTexture *parent);
void glRenderer__FlipTexture(glRenderer *This, glTexture **fliplist, DWORD count, BOOL framebuffer, DWORD flags, DWORD flips);
void glRenderer__SetTextureWrap(glRenderer *This, glTexture *texture, GLint s, GLint t);
void glRenderer__SetTextureFilter(glRenderer *This, glTexture *texture, GLint mag, GLint min);
void glRenderer__SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
void glRenderer__SetTexturePalette(glRenderer *This, glTexture *texture, glTexture *palette);
void glRenderer__SetTextureStencil(glRenderer *This, glTexture *texture, glTexture *stencil);
void glRenderer__LockTexture(glRenderer *This, glTexture *texture, RECT *r, DDSURFACEDESC2 *ddsd, DWORD flags, int miplevel);
void glRenderer__UnlockTexture(glRenderer *This, glTexture *texture, RECT *r, int miplevel, BOOL primary, int vsync);
void glRenderer__GetTextureDC(glRenderer *This, glTexture *texture, int miplevel);
void glRenderer__ReleaseTextureDC(glRenderer *This, glTexture *texture, int miplevel, BOOL primary, int vsync);
void glRenderer__RestoreTexture(glRenderer *This, glTexture *texture, BOOL setsync);
void glRenderer__SetRenderTarget(glRenderer *This, glTexture *texture);
void glRenderer__SetBPP(glRenderer *This, int bpp);

#ifdef __cplusplus
}
#endif

#endif //_GLRENDERER_H
