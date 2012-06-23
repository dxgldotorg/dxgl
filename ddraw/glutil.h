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
GLenum SetFBO(GLint color, GLint z, bool stencil);
void SetDepthComp(GLenum comp);

#endif //_GLUTIL_H