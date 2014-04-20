// DXGL
// Copyright (C) 2011-2014 William Feely

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

#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_RGB565 0x8D62

class glExtensions
{
public:
	GLuint(APIENTRY *glCreateShader) (GLenum type);
	void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
	void (APIENTRY *glCompileShader) (GLuint shader);
	void (APIENTRY *glDeleteShader) (GLuint shader);
	GLuint(APIENTRY *glCreateProgram) ();
	void (APIENTRY *glDeleteProgram) (GLuint program);
	void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint* params);
	void (APIENTRY *glAttachShader) (GLuint program, GLuint shader);
	void (APIENTRY *glDetachShader) (GLuint program, GLuint shader);
	void (APIENTRY *glLinkProgram) (GLuint program);
	void (APIENTRY *glUseProgram) (GLuint program);
	void (APIENTRY *glGetShaderiv) (GLuint shader, GLenum pname, GLint* params);
	void (APIENTRY *glGetShaderInfoLog) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
	void (APIENTRY *glGetProgramInfoLog) (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infolog);

	GLint(APIENTRY *glGetAttribLocation) (GLuint program, const GLchar* name);
	void (APIENTRY *glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
	void (APIENTRY *glEnableVertexAttribArray) (GLuint index);
	void (APIENTRY *glDisableVertexAttribArray) (GLuint index);

	void (APIENTRY *glGenFramebuffers) (GLsizei n, GLuint* ids);
	void (APIENTRY *glBindFramebuffer) (GLenum target, GLuint framebuffer);
	void (APIENTRY *glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
	void (APIENTRY *glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
	void (APIENTRY *glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	GLenum(APIENTRY *glCheckFramebufferStatus) (GLenum target);
	void (APIENTRY *glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);

	void (APIENTRY *glGenFramebuffersEXT) (GLsizei n, GLuint* ids);
	void (APIENTRY *glBindFramebufferEXT) (GLenum target, GLuint framebuffer);
	void (APIENTRY *glGenRenderbuffersEXT) (GLsizei n, GLuint* renderbuffers);
	void (APIENTRY *glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
	void (APIENTRY *glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	GLenum(APIENTRY *glCheckFramebufferStatusEXT) (GLenum target);
	void (APIENTRY *glDeleteFramebuffersEXT) (GLsizei n, const GLuint *framebuffers);

	GLint(APIENTRY *glGetUniformLocation) (GLuint program, const GLchar* name);
	void (APIENTRY *glUniform1i) (GLint location, GLint v0);
	void (APIENTRY *glUniform2i) (GLint location, GLint v0, GLint v1);
	void (APIENTRY *glUniform3i) (GLint location, GLint v0, GLint v1, GLint v2);
	void (APIENTRY *glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void (APIENTRY *glUniform4iv) (GLint location, GLsizei count, const GLint* value);
	void (APIENTRY *glUniform1f) (GLint location, GLfloat v0);
	void (APIENTRY *glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
	void (APIENTRY *glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void (APIENTRY *glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void (APIENTRY *glUniform3fv) (GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY *glUniform4fv) (GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY *glUniformMatrix3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void (APIENTRY *glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

	void (APIENTRY *glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

	void (APIENTRY *glActiveTexture)(GLenum texture);
	void (APIENTRY *glClientActiveTexture) (GLenum texture);

	void (APIENTRY *glGenBuffers)(GLsizei n, GLuint* buffers);
	void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint* buffers);
	void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
	void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	void* (APIENTRY *glMapBuffer)(GLenum target, GLenum access);
	GLboolean(APIENTRY *glUnmapBuffer)(GLenum target);

	BOOL(APIENTRY *wglSwapIntervalEXT)(int interval);
	int (APIENTRY *wglGetSwapIntervalEXT)();

	void (APIENTRY *glTextureParameterfEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTextureParameteriEXT)(GLuint texture, GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
	void (APIENTRY *glTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat,
		GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
		GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glGetTextureImageEXT)(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void (APIENTRY *glMatrixLoadfEXT)(GLenum mode, const GLfloat *m);
	void (APIENTRY *glMatrixMultfEXT)(GLenum mode, const GLfloat *m);

	void (APIENTRY *glBindSampler)(GLuint unit, GLuint sampler);
	void (APIENTRY *glDeleteSamplers)(GLsizei n, const GLuint *samplers);
	void (APIENTRY *glGenSamplers)(GLsizei n, GLuint *samplers);
	void (APIENTRY *glSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
	void (APIENTRY *glSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
	void (APIENTRY *glSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *params);
	void (APIENTRY *glSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *params);

	int GLEXT_ARB_framebuffer_object;
	int GLEXT_EXT_framebuffer_object;
	int GLEXT_NV_packed_depth_stencil;
	int GLEXT_EXT_packed_depth_stencil;
	int GLEXT_ARB_depth_buffer_float;
	int GLEXT_ARB_depth_texture;
	int GLEXT_NVX_gpu_memory_info;
	int GLEXT_ATI_meminfo;
	int GLEXT_ARB_ES2_compatibility;
	int GLEXT_EXT_direct_state_access;
	int GLEXT_ARB_sampler_objects;
	int glver_major;
	int glver_minor;

	glExtensions();

private:
	bool atimem;

};


#endif //_GLEXTENSIONS_H
