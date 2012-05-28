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

#define OP_NULL 0
#define OP_DELETE 1
#define OP_CREATE 2
#define OP_UPLOAD 3
#define OP_DOWNLOAD 4
#define OP_DELETETEX 5
#define OP_BLT 6
#define OP_DRAWSCREEN 7
#define OP_INITD3D 8
#define OP_CLEAR 9
#define OP_FLUSH 10
#define OP_DRAWPRIMITIVES 11
#define OP_SETRENDERSTATE 12
#define OP_SETMATRIX 13

#define OP_RESETQUEUE 0xFFFFFFFF


extern int swapinterval;
extern inline void SetSwap(int swap);

class glDirectDraw7;
class glDirect3DDevice7;
class glDirectDrawSurface7;

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
	void SetRenderState(DWORD index, DWORD count, DWORD *data);
	void SetMatrix(DWORD index, DWORD count, D3DMATRIX *data);
	void Sync(int size);
	int AddQueue(DWORD opcode, int mode, DWORD size, int paramcount, ...);
	HRESULT Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	HRESULT DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, bool packed, DWORD *texformats, DWORD count, LPWORD indices,
		DWORD indexcount, DWORD flags);
	HGLRC hRC;
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	GLCAPS gl_caps;
private:
	// In-thread APIs
	DWORD _Entry();
	BOOL _InitGL(int width, int height, int bpp, int fullscreen, HWND hWnd, glDirectDraw7 *glDD7);
	void _UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3);
	void _DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2);
	void _Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	GLuint _MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3);
	void _DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src);
	void _DeleteTexture(GLuint texture);
	void _DrawBackbuffer(GLuint *texture, int x, int y);
	void _InitD3D(int zbuffer);
	void _Clear(glDirectDrawSurface7 *target, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags, DWORD dwColor, D3DVALUE dvZ, DWORD dwStencil);
	void _DrawPrimitives(glDirect3DDevice7 *device, GLenum mode, GLVERTEX *vertices, int *texcormats, DWORD count, LPWORD indices,
		DWORD indexcount, DWORD flags);
	void _SetRenderState(DWORD index, DWORD count, DWORD *data);
	void _SetMatrix(DWORD index, DWORD count, D3DMATRIX *data);
	void _UpdateNormalMatrix();
	glDirectDraw7 *ddInterface;
	void _Flush();
	void* inputs[7];
	void* output;
	HANDLE hThread;
	HDC hDC;
	HWND hWnd;
	HWND hRenderWnd;
	bool hasHWnd;
	DIB dib;
	GLuint PBO;
	HANDLE busy;
	HANDLE start;
	HANDLE sync;
	bool running;
	CRITICAL_SECTION commandcs;
	CRITICAL_SECTION queuecs;
	LPDWORD queue;
	int queuesize;
	int queuelength;
	int queue_read;
	int queue_write;
	int syncsize;
	bool dead;
	DWORD renderstate[153];
	D3DMATRIX matrices[24];
	bool normal_dirty;
};

#endif //_GLRENDERER_H