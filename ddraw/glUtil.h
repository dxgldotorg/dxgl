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

#pragma once
#ifndef _GLUTIL_H
#define _GLUTIL_H

#ifdef __cplusplus
extern "C" {
#endif
struct glDirectDrawSurface7;



void glUtil_Create(glExtensions *glext, glUtil **out);
void glUtil_AddRef(glUtil *This);
void glUtil_Release(glUtil *This);
void glUtil_InitFBO(glUtil *This, FBO *fbo);
void glUtil_DeleteFBO(glUtil *This, FBO *fbo);
void glUtil_SetFBOTexture(glUtil *This, FBO *fbo, glTexture *color, glTexture *z, GLint level, GLint zlevel, BOOL stencil);
void glUtil_SetWrap(glUtil *This, int level, DWORD coord, DWORD address);
GLenum glUtil_SetFBOSurface(glUtil *This, glTexture *surface);
GLenum glUtil_SetFBO(glUtil *This, FBO *fbo);
GLenum glUtil_SetFBOTextures(glUtil *This, FBO *fbo, glTexture *color, glTexture *z, GLint level, GLint zlevel, BOOL stencil);
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
void glUtil_SetActiveTexture(glUtil *This, int level);
void glUtil_SetTexture(glUtil *This, unsigned int level, glTexture *texture);

#ifdef __cplusplus
}
#endif

#endif //_GLUTIL_H
