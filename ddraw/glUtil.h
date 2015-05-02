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

#ifdef __cplusplus
class glDirectDrawSurface7;
extern "C" {
#else
typedef int glDirectDrawSurface7;
#endif

struct TEXTURE;
struct BufferObject;
struct TextureManager;

typedef struct SAMPLER
{
	GLuint id;
	GLint wraps;
	GLint wrapt;
	GLint minfilter;
	GLint magfilter;
} SAMPLER;

typedef struct FBO
{
	GLuint fbo;
	TEXTURE *fbcolor;
	TEXTURE *fbz;
	BOOL stencil;
	GLenum status;
} FBO;

typedef struct
{
	GLfloat x, y;
	GLfloat s, t;
	GLfloat dests, destt;
	GLfloat stencils, stencilt;
} BltVertex;

typedef struct glUtil
{
	ULONG refcount;
	FBO *currentfbo;
	BufferObject *pboPackBinding;
	BufferObject *pboUnpackBinding;
	BufferObject *vboArrayBinding;
	BufferObject *vboElementArrayBinding;
	BufferObject *uboUniformBufferBinding;
	glExtensions *ext;
	BOOL depthwrite;
	BOOL depthtest;
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
	BOOL scissorenabled;
	GLint texwrap[16];
	GLclampf clearr;
	GLclampf clearg;
	GLclampf clearb;
	GLclampf cleara;
	GLclampd cleardepth;
	GLint clearstencil;
	GLenum blendsrc;
	GLenum blenddest;
	BOOL blendenabled;
	BOOL arrays[42];
	D3DCULL cullmode;
	BOOL cullenabled;
	D3DFILLMODE polymode;
	D3DSHADEMODE shademode;
	BufferObject *LastBoundBuffer;
	SAMPLER samplers[8];
} glUtil;

void glUtil_Create(glExtensions *glext, glUtil **out);
void glUtil_AddRef(glUtil *This);
void glUtil_Release(glUtil *This);
void glUtil_InitFBO(glUtil *This, FBO *fbo);
void glUtil_DeleteFBO(glUtil *This, FBO *fbo);
void glUtil_SetFBOTexture(glUtil *This, FBO *fbo, TEXTURE *color, TEXTURE *z, BOOL stencil);
void glUtil_SetWrap(glUtil *This, int level, DWORD coord, DWORD address, TextureManager *texman);
GLenum glUtil_SetFBOSurface(glUtil *This, glDirectDrawSurface7 *surface);
GLenum glUtil_SetFBO(glUtil *This, FBO *fbo);
GLenum glUtil_SetFBOTextures(glUtil *This, FBO *fbo, TEXTURE *color, TEXTURE *z, BOOL stencil);
void glUtil_SetDepthComp(glUtil *This, GLenum comp);
void glUtil_DepthWrite(glUtil *This, BOOL enabled);
void glUtil_DepthTest(glUtil *This, BOOL enabled);
void glUtil_SetScissor(glUtil *This, BOOL enabled, GLint x, GLint y, GLsizei width, GLsizei height);
void glUtil_SetMatrix(glUtil *This, GLenum mode, GLfloat *mat1, GLfloat *mat2, BOOL *dirty);
void glUtil_MatrixMode(glUtil *This, GLenum mode);
void glUtil_SetMaterial(glUtil *This, GLfloat ambient[4], GLfloat diffuse[4], GLfloat specular[4], GLfloat emission[4], GLfloat shininess);
void glUtil_SetViewport(glUtil *This, GLint x, GLint y, GLsizei width, GLsizei height);
void glUtil_SetDepthRange(glUtil *This, GLclampd rangenear, GLclampd rangefar);
void glUtil_ClearColor(glUtil *This, GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glUtil_ClearDepth(glUtil *This, GLclampd depth);
void glUtil_ClearStencil(glUtil *This, GLint stencil);
void glUtil_EnableArray(glUtil *This, int index, BOOL enabled);
void glUtil_BlendFunc(glUtil *This, GLenum src, GLenum dest);
void glUtil_BlendEnable(glUtil *This, BOOL enabled);
void glUtil_EnableCull(glUtil *This, BOOL enabled);
void glUtil_SetCull(glUtil *This, D3DCULL mode);
void glUtil_SetPolyMode(glUtil *This, D3DFILLMODE mode);
void glUtil_SetShadeMode(glUtil *This, D3DSHADEMODE mode);
void glUtil_BindBuffer(glUtil *This, BufferObject *buffer, GLenum target);
void glUtil_UndoBindBuffer(glUtil *This, GLenum target);

#ifdef __cplusplus
}
#endif

#endif //_GLUTIL_H
