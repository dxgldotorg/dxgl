// DXGL
// Copyright (C) 2011-2012 William Feely

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

#ifndef GL_DEPTH_BUFFER
#define GL_DEPTH_BUFFER 0x8223
#endif
#ifndef GL_STENCIL_BUFFER
#define GL_STENCIL_BUFFER 0x8224
#endif

extern int GLEXT_ARB_framebuffer_object;
extern int GLEXT_EXT_framebuffer_object;
extern int GLEXT_NV_packed_depth_stencil;
extern int GLEXT_EXT_packed_depth_stencil;
extern int GLEXT_ARB_depth_buffer_float;
extern int GLEXT_ARB_depth_texture;
extern int GLEXT_NVX_gpu_memory_info;
extern int GLEXT_ATI_meminfo;
extern int GLEXT_ARB_ES2_compatibility;
extern int glver_major;
extern int glver_minor;
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_RGB565 0x8D62

#ifdef __GNUC__
#undef GLAPI
#define GLAPI extern
#endif

GLAPI GLuint (APIENTRY *glCreateShader) (GLenum type);
GLAPI void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
GLAPI void (APIENTRY *glCompileShader) (GLuint shader);
GLAPI void (APIENTRY *glDeleteShader) (GLuint shader);
GLAPI GLuint (APIENTRY *glCreateProgram) ();
GLAPI void (APIENTRY *glDeleteProgram) (GLuint program);
GLAPI void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint* params);
GLAPI void (APIENTRY *glAttachShader) (GLuint program, GLuint shader);
GLAPI void (APIENTRY *glDetachShader) (GLuint program, GLuint shader);
GLAPI void (APIENTRY *glLinkProgram) (GLuint program);
GLAPI void (APIENTRY *glUseProgram) (GLuint program);
GLAPI void (APIENTRY *glGetShaderiv) (GLuint shader, GLenum pname, GLint* params);
GLAPI void (APIENTRY *glGetShaderInfoLog) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
GLAPI void (APIENTRY *glGetProgramInfoLog) (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infolog);

GLAPI GLint (APIENTRY *glGetAttribLocation) (GLuint program, const GLchar* name);
GLAPI void (APIENTRY *glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
GLAPI void (APIENTRY *glEnableVertexAttribArray) (GLuint index);
GLAPI void (APIENTRY *glDisableVertexAttribArray) (GLuint index);

GLAPI void (APIENTRY *glGenFramebuffers) (GLsizei n, GLuint* ids);
GLAPI void (APIENTRY *glBindFramebuffer) (GLenum target, GLuint framebuffer);
GLAPI void (APIENTRY *glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
GLAPI void (APIENTRY *glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
GLAPI void (APIENTRY *glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI GLenum (APIENTRY *glCheckFramebufferStatus) (GLenum target);
GLAPI void (APIENTRY *glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);

GLAPI void (APIENTRY *glGenFramebuffersEXT) (GLsizei n, GLuint* ids);
GLAPI void (APIENTRY *glBindFramebufferEXT) (GLenum target, GLuint framebuffer);
GLAPI void (APIENTRY *glGenRenderbuffersEXT) (GLsizei n, GLuint* renderbuffers);
GLAPI void (APIENTRY *glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
GLAPI void (APIENTRY *glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLAPI GLenum (APIENTRY *glCheckFramebufferStatusEXT) (GLenum target);
GLAPI void (APIENTRY *glDeleteFramebuffersEXT) (GLsizei n, const GLuint *framebuffers);

GLAPI GLint (APIENTRY *glGetUniformLocation) (GLuint program, const GLchar* name);
GLAPI void (APIENTRY *glUniform1i) (GLint location, GLint v0);
GLAPI void (APIENTRY *glUniform2i) (GLint location, GLint v0, GLint v1);
GLAPI void (APIENTRY *glUniform3i) (GLint location, GLint v0, GLint v1, GLint v2);
GLAPI void (APIENTRY *glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GLAPI void (APIENTRY *glUniform1f) (GLint location, GLfloat v0);
GLAPI void (APIENTRY *glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
GLAPI void (APIENTRY *glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLAPI void (APIENTRY *glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLAPI void (APIENTRY *glUniform3fv) (GLint location, GLsizei count, const GLfloat* value);
GLAPI void (APIENTRY *glUniform4fv) (GLint location, GLsizei count, const GLfloat* value);
GLAPI void (APIENTRY *glUniformMatrix3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
GLAPI void (APIENTRY *glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

GLAPI void (APIENTRY *glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

GLAPI void (APIENTRY *glActiveTexture)(GLenum texture);
GLAPI void (APIENTRY *glClientActiveTexture) (GLenum texture);

GLAPI void (APIENTRY *glGenBuffers)(GLsizei n, GLuint* buffers);
GLAPI void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint* buffers);
GLAPI void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
GLAPI void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
GLAPI void* (APIENTRY *glMapBuffer)(GLenum target, GLenum access);
GLAPI GLboolean (APIENTRY *glUnmapBuffer)(GLenum target);

GLAPI BOOL (APIENTRY *wglSwapIntervalEXT)(int interval);
GLAPI int (APIENTRY *wglGetSwapIntervalEXT)();

void InitGLExt();

#endif //_GLEXTENSIONS_H
