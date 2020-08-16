// DXGL
// Copyright (C) 2012-2020 William Feely

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

#ifdef __cplusplus
class glDirectDraw7;
class glDirect3DDevice7;
class glDirectDrawSurface7;
class glRenderWindow;
extern "C" {
#else
typedef int glDirectDraw7;
typedef int glDirect3DDevice7;
typedef int glDirectDrawSurface7;
typedef int glRenderWindow;
#endif

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
	union {
		glTexture *texture;
		glDirectDrawSurface7 *surface;
	} DUMMYUNIONNAME1;
	GLint glmagfilter;
	GLint glminfilter;
} TEXTURESTAGE;

#define OP_NULL						0
#define OP_SETWND					1
#define OP_DELETE					2
#define OP_CREATE					3
#define OP_UPLOAD					4
#define OP_DOWNLOAD					5
#define OP_DELETETEX				6
#define OP_BLT						7
#define OP_DRAWSCREEN				8
#define OP_INITD3D					9
#define OP_CLEAR					10
#define OP_FLUSH					11
#define OP_DRAWPRIMITIVES			12
#define OP_UPDATECLIPPER			13
#define OP_DEPTHFILL				14
#define OP_SETRENDERSTATE			15
#define OP_SETTEXTURE				16
#define OP_SETTEXTURESTAGESTATE		17
#define OP_SETTRANSFORM				18
#define OP_SETMATERIAL				19
#define OP_SETLIGHT					20
#define OP_REMOVELIGHT				21
#define OP_SETD3DVIEWPORT			22
#define OP_DXGLBREAK                23
#define OP_SETTEXTURECOLORKEY		24
#define OP_MAKETEXTUREPRIMARY       25
#define OP_ENDCOMMAND				26
#define OP_INITTEXTURESTAGE			27
#define OP_SETTEXTURESURFACEDESC    28
#define OP_SETSHADER2D				29
#define OP_SETSHADER				30
#define OP_SETRENDERTARGET			31
#define OP_SETVIEWPORT				32
#define OP_VERTEX2D					33
#define OP_SETD3DDEPTHMODE			34
#define OP_BLENDENABLE				35
#define OP_SETDEPTHTEST				36
#define OP_SETFRONTBUFFERBITS       37
#define OP_SETSWAP                  38
#define OP_SWAPBUFFERS				39
#define OP_SETUNIFORM				40
#define OP_SETATTRIB				41
#define OP_SETMODE3D				42

extern const DWORD renderstate_default[RENDERSTATE_COUNT];
extern const TEXTURESTAGE texstagedefault0;
extern const TEXTURESTAGE texstagedefault1;

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
	glTexture *backbuffers[16];
	DWORD fogcolor;
	GLfloat fogstart;
	GLfloat fogend;
	GLfloat fogdensity;
	BltVertex bltvertices[4];
	int oldswap;
	glUtil *util;
	ShaderManager *shaders;
	DWORD renderstate[RENDERSTATE_COUNT];
	SHADERSTATE shaderstate3d;
	TEXTURESTAGE texstages[12];
	D3DMATERIAL7 material;
	D3DLIGHT7 lights[8];
	D3DMATRIX transform[24];
	D3DVIEWPORT7 viewport;
	CommandBuffer cmd1, cmd2;
	float mulx, muly;
	RenderState state;
	size_t scenesize, scenesizevertex, scenesizeindex;
	DWORD last_fvf;
	GLsizei fvf_stride;
	BOOL mode_3d;
	float postsizex, postsizey;
	int xoffset, yoffset;
	OVERLAY *overlays;
	int overlaycount;
} glRenderer;

HRESULT glRenderer_AddCommand(glRenderer *This, QueueCmd *cmd, BOOL inner, BOOL wait);
void glRenderer_Init(glRenderer *This, int width, int height, int bpp, BOOL fullscreen, unsigned int frequency, HWND hwnd, glDirectDraw7 *glDD7, BOOL devwnd);
void glRenderer_Delete(glRenderer *This);
DWORD glRenderer_GetBPP(glRenderer *This);
static DWORD WINAPI glRenderer_ThreadEntry(void *entry);
void glRenderer_UploadTexture(glRenderer *This, glTexture *texture, GLint level);
void glRenderer_DownloadTexture(glRenderer *This, glTexture *texture, GLint level);
HRESULT glRenderer_Blt(glRenderer *This, BltCommand *cmd);
void glRenderer_MakeTexture(glRenderer *This, glTexture *texture);
void glRenderer_DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous, BOOL settime, OVERLAY *overlays, int overlaycount);
void glRenderer_DeleteTexture(glRenderer *This, glTexture *texture);
void glRenderer_InitD3D(glRenderer *This, int zbuffer, int x, int y);
void glRenderer_Flush(glRenderer *This);
void glRenderer_SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd, BOOL devwnd);
HRESULT glRenderer_Clear(glRenderer *This, ClearCommand *cmd);
HRESULT glRenderer_DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer_UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height);
unsigned int glRenderer_GetScanLine(glRenderer *This);
HRESULT glRenderer_DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel);
void glRenderer_SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
void glRenderer_SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture);
void glRenderer_SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
void glRenderer_SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
void glRenderer_SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial);
void glRenderer_SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light);
void glRenderer_RemoveLight(glRenderer *This, DWORD index);
void glRenderer_SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport);
void glRenderer_SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level);
void glRenderer_MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary);
void glRenderer_DXGLBreak(glRenderer *This);
void glRenderer_EndCommand(glRenderer *This, BOOL wait, BOOL in_cs);
// In-thread APIs
DWORD glRenderer__Entry(glRenderer *This);
BOOL glRenderer__InitGL(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7);
void glRenderer__InitCommandBuffer(glRenderer *This, CommandBuffer *cmd, size_t framesize);
void glRenderer__DeleteCommandBuffer(CommandBuffer *cmd);
void glRenderer__UploadTexture(glRenderer *This, glTexture *texture, GLint level);
void glRenderer__DownloadTexture(glRenderer *This, glTexture *texture, GLint level);
void glRenderer__Blt(glRenderer *This, BltCommand *cmd, BOOL backend);
void glRenderer__MakeTexture(glRenderer *This, glTexture *texture);
void glRenderer__DrawScreen(glRenderer *This, glTexture *texture, glTexture *paltex, GLint vsync, glTexture *previous, BOOL setsync, BOOL settime, OVERLAY *overlays, int overlaycount);
void glRenderer__DeleteTexture(glRenderer *This, glTexture *texture);
void glRenderer__DrawBackbuffer(glRenderer *This, glTexture **texture, int x, int y, int progtype, BOOL paletted, BOOL firstpass, int index);
void glRenderer__DrawBackbufferRect(glRenderer *This, glTexture *texture, RECT srcrect, RECT destrect, int progtype, int index);
void glRenderer__InitD3D(glRenderer *This, int zbuffer, int x, int y);
void glRenderer__Clear(glRenderer *This, ClearCommand *cmd);
void glRenderer__UpdateFVF(glRenderer *This, DWORD fvf);
void glRenderer__DrawPrimitives(glRenderer *This, RenderTarget *target, GLenum mode, DWORD fvf,
	BYTE *vertices, BOOL strided, DWORD count, LPWORD indices, DWORD indexcount, DWORD flags);
void glRenderer__DrawPrimitivesOld(glRenderer *This, RenderTarget *target, GLenum mode, GLVERTEX *vertices, int *texcormats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer__Flush(glRenderer *This);
void glRenderer__SetWnd(glRenderer *This, int width, int height, int fullscreen, int bpp, unsigned int frequency, HWND newwnd, BOOL devwnd);
void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo);
void glRenderer__UpdateClipper(glRenderer *This, glTexture *stencil, GLushort *indices, BltVertex *vertices,
	GLsizei count, GLsizei width, GLsizei height);
void glRenderer__DepthFill(glRenderer *This, BltCommand *cmd, glTexture *parent, GLint parentlevel);
void glRenderer__SetFogColor(glRenderer *This, DWORD color);
void glRenderer__SetFogStart(glRenderer *This, GLfloat start);
void glRenderer__SetFogEnd(glRenderer *This, GLfloat end);
void glRenderer__SetFogDensity(glRenderer *This, GLfloat density);
void glRenderer__SetSwap(glRenderer *This, int swap);
void glRenderer__SetBlend(glRenderer *This, DWORD src, DWORD dest);
void glRenderer__SetRenderState(glRenderer *This, D3DRENDERSTATETYPE dwRendStateType, DWORD dwRenderState);
void glRenderer__RemoveTextureFromD3D(glRenderer *This, glTexture *texture);
void glRenderer__SetTexture(glRenderer *This, DWORD dwStage, glTexture *Texture);
void glRenderer__SetTextureStageState(glRenderer *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE dwState, DWORD dwValue);
void glRenderer__SetTransform(glRenderer *This, D3DTRANSFORMSTATETYPE dtstTransformStateType, LPD3DMATRIX lpD3DMatrix);
void glRenderer__SetMaterial(glRenderer *This, LPD3DMATERIAL7 lpMaterial);
void glRenderer__SetLight(glRenderer *This, DWORD index, LPD3DLIGHT7 light);
void glRenderer__RemoveLight(glRenderer *This, DWORD index);
void glRenderer__SetD3DViewport(glRenderer *This, LPD3DVIEWPORT7 lpViewport);
void glRenderer__SetTextureColorKey(glRenderer *This, glTexture *texture, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey, GLint level);
void glRenderer__MakeTexturePrimary(glRenderer *This, glTexture *texture, glTexture *parent, BOOL primary);
void glRenderer__SetDepthComp(glRenderer *This);
void glRenderer__DXGLBreak(glRenderer *This, BOOL setbusy);
void glRenderer__EndCommand(glRenderer *This, BOOL wait);
void glRenderer__SetMode3D(glRenderer *This, BOOL enabled);

void BltFlipLR(BltVertex *vertices);
void BltFlipUD(BltVertex *vertices);
void RotateBlt90(BltVertex *vertices, int times);

#ifdef __cplusplus
}
#endif

#endif //_GLRENDERER_H
