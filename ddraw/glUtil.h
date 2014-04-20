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

#pragma once
#ifndef _GLUTIL_H
#define _GLUTIL_H

typedef struct
{
	GLuint fbo;
	TEXTURE *fbcolor;
	TEXTURE *fbz;
	bool stencil;
	GLenum status;
} FBO;

typedef struct
{
	GLfloat x, y;
	GLubyte r, g, b, a;
	GLfloat s, t;
	GLfloat dests, destt;
	GLfloat padding;
} BltVertex;

class glDirectDrawSurface7;

class glUtil
{
public:
	glUtil(glExtensions *glext);
	void InitFBO(FBO *fbo);
	void DeleteFBO(FBO *fbo);
	void SetFBOTexture(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil);
	void SetWrap(int level, DWORD coord, DWORD address, TextureManager *texman);
	void SetFBO(glDirectDrawSurface7 *surface);
	void SetFBO(FBO *fbo);
	void SetFBO(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil);
	void SetDepthComp(GLenum comp);
	void DepthWrite(bool enabled);
	void DepthTest(bool enabled);
	void SetScissor(bool enabled, GLint x, GLint y, GLsizei width, GLsizei height);
	void SetMatrix(GLenum mode, GLfloat *mat1, GLfloat *mat2, bool *dirty);
	void MatrixMode(GLenum mode);
	void SetMaterial(GLfloat ambient[4], GLfloat diffuse[4], GLfloat specular[4], GLfloat emission[4], GLfloat shininess);
	void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void SetDepthRange(GLclampd rangenear, GLclampd rangefar);
	void ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
	void ClearDepth(GLclampd depth);
	void ClearStencil(GLint stencil);
	void EnableArray(int index, bool enabled);
	void BlendFunc(GLenum src, GLenum dest);
	void BlendEnable(bool enabled);
	void EnableCull(bool enabled);
	void SetCull(D3DCULL mode);
	void SetPolyMode(D3DFILLMODE mode);
	void SetShadeMode(D3DSHADEMODE mode);
	FBO *currentfbo;
private:
	glExtensions *ext;
	bool depthwrite;
	bool depthtest;
	GLuint depthcomp;
	GLuint alphacomp;
	GLint scissorx;
	GLint scissory;
	GLsizei scissorwidth;
	GLsizei scissorheight;
	GLint viewportx;
	GLint viewporty;
	GLsizei viewportwidth;
	GLsizei viewportheight;
	GLclampd depthnear;
	GLclampd depthfar;
	GLenum matrixmode;
	GLfloat materialambient[4];
	GLfloat materialdiffuse[4];
	GLfloat materialspecular[4];
	GLfloat materialemission[4];
	GLfloat materialshininess;
	bool scissorenabled;
	GLint texwrap[16];
	GLclampf clearr;
	GLclampf clearg;
	GLclampf clearb;
	GLclampf cleara;
	GLclampd cleardepth;
	GLint clearstencil;
	GLenum blendsrc;
	GLenum blenddest;
	bool blendenabled;
	bool arrays[42];
	D3DCULL cullmode;
	bool cullenabled;
	D3DFILLMODE polymode;
	D3DSHADEMODE shademode;
};

#endif //_GLUTIL_H