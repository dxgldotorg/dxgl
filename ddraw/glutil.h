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

#pragma once
#ifndef _GLUTIL_H
#define _GLUTIL_H

extern GLuint fbcolor,fbz;
extern GLuint fbo;
extern bool stencil;

void InitFBO();
void DeleteFBO();
void SetTexture(int level,GLuint texture);
void SetActiveTexture(int level);
void SetWrap(int level, DWORD coord, DWORD address);
GLenum SetFBO(GLint color, GLint z, bool stencil);
void SetDepthComp(GLenum comp);
void DepthWrite(bool enabled);
void DepthTest(bool enabled);
void SetScissor(bool enabled, GLint x, GLint y, GLsizei width, GLsizei height);
void SetMatrix(GLenum mode, GLfloat *mat1, GLfloat *mat2, bool *dirty);
void MatrixMode(GLenum mode);
void SetMaterial(GLfloat ambient[4],GLfloat diffuse[4],GLfloat specular[4],GLfloat emission[4],GLfloat shininess);
void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void SetDepthRange(GLclampd rangenear, GLclampd rangefar);
void ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void ClearDepth(GLclampd depth);
void ClearStencil(GLint stencil);
void EnableArray(int index, bool enabled);

#endif //_GLUTIL_H