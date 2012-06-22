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

struct GLVERTEX
{
	void *data;
	int stride;
};

struct BltVertex
{
	GLfloat x,y;
	GLubyte r,g,b,a;
	GLfloat s,t;
	GLfloat padding[3];
};

extern BltVertex bltvertices[4];

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


extern int swapinterval;
extern inline void SetSwap(int swap);

class glDirectDraw7;
class glDirect3DDevice7;
class glDirectDrawSurface7;
class glRenderWindow;

/** @brief glRenderer class
  * OpenGL renderer class for DXGL.
  */
class glRenderer
{
public:
	glRenderer(int width, int height, int bpp, bool fullscreen, HWND hwnd, glDirectDraw7 *glDD7);
	~glRenderer();
	static DWORD WINAPI ThreadEntry(void *entry);
	void UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3);
	void DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2);
	HRESULT Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	GLuint MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3);
	void DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src);
	void DeleteTexture(GLuint texture);
	void InitD3D(int zbuffer);
	void Flush();
	void SetWnd(int width, int height, int fullscreen, int bpp, HWND newwnd);
	HRESULT Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	HRESULT DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texformats, DWORD count, LPWORD indices,
		DWORD indexcount, DWORD flags);
	HGLRC hRC;
	GLCAPS gl_caps;
private:
	// In-thread APIs
	DWORD _Entry();
	BOOL _InitGL(int width, int height, int bpp, int fullscreen, HWND hWnd, glDirectDraw7 *glDD7);
	void _UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3);
	void _DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2);
	HRESULT _Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	GLuint _MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3, bool setsync);
	void _DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, bool setsync);
	void _DeleteTexture(GLuint texture);
	void _DrawBackbuffer(GLuint *texture, int x, int y);
	void _InitD3D(int zbuffer);
	void _Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	void glRenderer::_DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texcormats, DWORD count, LPWORD indices,
		DWORD indexcount, DWORD flags);
	glDirectDraw7 *ddInterface;
	void _Flush();
	void _SetWnd(int width, int height, int fullscreen, int bpp, HWND newwnd);
	int opcode;
	void* inputs[32];
	void* outputs[32];
	HANDLE hThread;
	HDC hDC;
	HWND hWnd;
	glRenderWindow *RenderWnd;
	DIB dib;
	GLuint PBO;
	CRITICAL_SECTION cs;
	HANDLE busy;
	HANDLE start;
};

#endif //_GLRENDERER_H