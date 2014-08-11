// DXGL
// Copyright (C) 2012-2014 William Feely

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
	bool enabled;
	int width;
	int height;
	int pitch;
	HDC hdc;
	HBITMAP hbitmap;
	BITMAPINFO info;
	BYTE *pixels;
} DIB;

typedef struct
{
	GLuint buffer;
	GLsizei size;
	void *pointer;
	bool mapped;
	bool busy;
} PBO;

typedef struct
{
	void *data;
	int stride;
} GLVERTEX;

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
#define OP_DELETEFBO				13
#define OP_UPDATECLIPPER            14

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
	GLuint PBO;
	CRITICAL_SECTION cs;
	HANDLE busy;
	HANDLE start;
	unsigned int frequency;
	DXGLTimer timer;
	TEXTURE *backbuffer;
	int backx;
	int backy;
	DWORD fogcolor;
	GLfloat fogstart;
	GLfloat fogend;
	GLfloat fogdensity;
	BltVertex bltvertices[4];
	int oldswap;
	TextureManager *texman;
	glUtil *util;
	ShaderManager *shaders;
} glRenderer;

void glRenderer_Init(glRenderer *This, int width, int height, int bpp, bool fullscreen, unsigned int frequency, HWND hwnd, glDirectDraw7 *glDD7);
void glRenderer_Delete(glRenderer *This);
static DWORD WINAPI glRenderer_ThreadEntry(void *entry);
void glRenderer_UploadTexture(glRenderer *This, char *buffer, char *bigbuffer, TEXTURE *texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp);
void glRenderer_DownloadTexture(glRenderer *This, char *buffer, char *bigbuffer, TEXTURE *texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp);
HRESULT glRenderer_Blt(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *src,
	glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
void glRenderer_MakeTexture(glRenderer *This, TEXTURE *texture, DWORD width, DWORD height);
void glRenderer_DrawScreen(glRenderer *This, TEXTURE *texture, TEXTURE *paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, GLint vsync);
void glRenderer_DeleteTexture(glRenderer *This, TEXTURE *texture);
void glRenderer_InitD3D(glRenderer *This, int zbuffer);
void glRenderer_Flush(glRenderer *This);
void glRenderer_SetWnd(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND newwnd);
HRESULT glRenderer_Clear(glRenderer *This, glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
HRESULT glRenderer_DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer_DeleteFBO(glRenderer *This, FBO *fbo);
void glRenderer_UpdateClipper(glRenderer *This, glDirectDrawSurface7 *surface);
unsigned int glRenderer_GetScanLine(glRenderer *This);
// In-thread APIs
DWORD glRenderer__Entry(glRenderer *This);
BOOL glRenderer__InitGL(glRenderer *This, int width, int height, int bpp, int fullscreen, unsigned int frequency, HWND hWnd, glDirectDraw7 *glDD7);
void glRenderer__UploadTexture(glRenderer *This, char *buffer, char *bigbuffer, TEXTURE *texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp);
void glRenderer__DownloadTexture(glRenderer *This, char *buffer, char *bigbuffer, TEXTURE *texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp);
void glRenderer__Blt(glRenderer *This, LPRECT lpDestRect, glDirectDrawSurface7 *src,
	glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
void glRenderer__MakeTexture(glRenderer *This, TEXTURE *texture, DWORD width, DWORD height);
void glRenderer__DrawScreen(glRenderer *This, TEXTURE *texture, TEXTURE *paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, GLint vsync, bool setsync);
void glRenderer__DeleteTexture(glRenderer *This, TEXTURE *texture);
void glRenderer__DrawBackbuffer(glRenderer *This, TEXTURE **texture, int x, int y, int progtype);
void glRenderer__DrawBackbufferRect(glRenderer *This, TEXTURE *texture, RECT srcrect, int progtype);
void glRenderer__InitD3D(glRenderer *This, int zbuffer);
void glRenderer__Clear(glRenderer *This, glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
void glRenderer__DrawPrimitives(glRenderer *This, glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texcormats, DWORD count, LPWORD indices,
	DWORD indexcount, DWORD flags);
void glRenderer__Flush(glRenderer *This);
void glRenderer__SetWnd(glRenderer *This, int width, int height, int fullscreen, int bpp, unsigned int frequency, HWND newwnd);
void glRenderer__DeleteFBO(glRenderer *This, FBO *fbo);
void glRenderer__UpdateClipper(glRenderer *This, glDirectDrawSurface7 *surface);
void glRenderer__SetFogColor(glRenderer *This, DWORD color);
void glRenderer__SetFogStart(glRenderer *This, GLfloat start);
void glRenderer__SetFogEnd(glRenderer *This, GLfloat end);
void glRenderer__SetFogDensity(glRenderer *This, GLfloat density);
inline void glRenderer__SetSwap(glRenderer *This, int swap);
void glRenderer__SetBlend(glRenderer *This, DWORD src, DWORD dest);

#ifdef __cplusplus
}
#endif

#endif //_GLRENDERER_H