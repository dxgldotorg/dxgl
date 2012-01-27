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
	int TextureMaxX;
	int TextureMaxY;
	bool NonPowerOfTwo;
	int RenderToTexture; // 0 - no support, 1 -
	int MultiTextureExt;
	int PalettedTextures;
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

struct BltVertex
{
	GLfloat x,y;
	GLubyte r,g,b,a;
	GLfloat s,t;
	GLfloat padding[3];
};

extern BltVertex bltvertices[4];

#define GLEVENT_NULL 0
#define GLEVENT_DELETE 1
#define GLEVENT_CREATE 2
#define GLEVENT_UPLOAD 3
#define GLEVENT_DOWNLOAD 4
#define GLEVENT_DELETETEX 5
#define GLEVENT_BLT 6
#define GLEVENT_DRAWSCREEN 7

extern int swapinterval;
extern inline void SetSwap(int swap);

class glRenderer
{
public:
	glRenderer(int width, int height, int bpp, bool fullscreen, HWND hwnd, glDirectDraw7 *glDD7);
	~glRenderer();
	static DWORD WINAPI ThreadEntry(void *entry);
	int UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3);
	int DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2);
	HRESULT Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
	GLuint MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3);
	void DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src);
	void DeleteTexture(GLuint texture);
	HGLRC hRC;
private:
	// In-thread APIs
	DWORD _Entry();
	BOOL _InitGL(int width, int height, int bpp, int fullscreen, HWND hWnd, glDirectDraw7 *glDD7);
	int _UploadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2, int texformat3);
	int _DownloadTexture(char *buffer, char *bigbuffer, GLuint texture, int x, int y, int bigx, int bigy, int pitch, int bigpitch, int bpp, int texformat, int texformat2);
	HRESULT _Blt(LPRECT lpDestRect, glDirectDrawSurface7 *src,
		glDirectDrawSurface7 *dest, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx, RECT *viewrect);
	GLuint _MakeTexture(GLint min, GLint mag, GLint wraps, GLint wrapt, DWORD width, DWORD height, GLint texformat1, GLint texformat2, GLint texformat3);
	void _DrawScreen(GLuint texture, GLuint paltex, glDirectDrawSurface7 *dest, glDirectDrawSurface7 *src, RECT *viewrect);
	void _DeleteTexture(GLuint texture);
	glDirectDraw7 *ddInterface;
	int eventnum;
	void* inputs[32];
	void* outputs[32];
	HANDLE hThread;
	HANDLE EventSend;
	HANDLE EventWait;
	HDC hDC;
	HWND hWnd;
	HWND hRenderWnd;
	bool hasHWnd;
	GLCAPS gl_caps;
	DIB dib;
	GLuint PBO;
};

#endif //_GLRENDERER_H