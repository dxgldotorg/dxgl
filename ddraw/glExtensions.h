// DXGL
// Copyright (C) 2011 William Feely

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
#ifndef _GLEXTENSIONS_H
#define _GLEXTENSIONS_H

extern int GLEXT_ARB_framebuffer_object;
extern int GLEXT_EXT_framebuffer_object;
#ifdef __GNUC__
#undef GLAPI
#define GLAPI extern
#endif
GLAPI GLuint (APIENTRY *glCreateShader) (GLenum type);
GLAPI void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar** string, const GLint* length);
GLAPI void (APIENTRY *glCompileShader) (GLuint shader);
GLAPI GLuint (APIENTRY *glCreateProgram) ();
GLAPI void (APIENTRY *glDeleteProgram) (GLuint program);
GLAPI void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint* params);
GLAPI void (APIENTRY *glAttachShader) (GLuint program, GLuint shader);
GLAPI void (APIENTRY *glDetachShader) (GLuint program, GLuint shader);
GLAPI void (APIENTRY *glLinkProgram) (GLuint program);
GLAPI void (APIENTRY *glUseProgram) (GLuint program);

GLAPI void (APIENTRY *glGenFramebuffers) (GLsizei n, GLuint* ids);
GLAPI void (APIENTRY *glBindFramebuffer) (GLenum target, GLuint framebuffer);
GLAPI void (APIENTRY *glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
GLAPI void (APIENTRY *glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
GLAPI void (APIENTRY *glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI GLenum (APIENTRY *glCheckFramebufferStatus) (GLenum target);

GLAPI void (APIENTRY *glGenFramebuffersEXT) (GLsizei n, GLuint* ids);
GLAPI void (APIENTRY *glBindFramebufferEXT) (GLenum target, GLuint framebuffer);
GLAPI void (APIENTRY *glGenRenderbuffersEXT) (GLsizei n, GLuint* renderbuffers);
GLAPI void (APIENTRY *glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
GLAPI void (APIENTRY *glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI GLenum (APIENTRY *glCheckFramebufferStatusEXT) (GLenum target);

GLAPI GLint (APIENTRY *glGetUniformLocation) (GLuint program, const GLchar* name);
GLAPI void (APIENTRY *glUniform1i) (GLint location, GLint v0);
GLAPI void (APIENTRY *glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);

GLAPI void (APIENTRY *glActiveTexture)(GLenum texture);

GLAPI BOOL (APIENTRY *wglSwapIntervalEXT)(int interval);

void InitGLExt();

#endif //_GLEXTENSIONS_H
