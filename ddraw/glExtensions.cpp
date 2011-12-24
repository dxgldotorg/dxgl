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

#include "common.h"
#include "glExtensions.h"

GLuint (APIENTRY *glCreateShader) (GLenum type) = NULL;
void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar** string, const GLint* length) = NULL;
void (APIENTRY *glCompileShader) (GLuint shader) = NULL;
GLuint (APIENTRY *glCreateProgram) () = NULL;
void (APIENTRY *glDeleteProgram) (GLuint program) = NULL;
void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint* params) = NULL;
void (APIENTRY *glAttachShader) (GLuint program, GLuint shader) = NULL;
void (APIENTRY *glDetachShader) (GLuint program, GLuint shader) = NULL;
void (APIENTRY *glLinkProgram) (GLuint program) = NULL;
void (APIENTRY *glUseProgram) (GLuint program) = NULL;

void (APIENTRY *glGenFramebuffers) (GLsizei n, GLuint* ids) = NULL;
void (APIENTRY *glBindFramebuffer) (GLenum target, GLuint framebuffer) = NULL;
void (APIENTRY *glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers) = NULL;
void (APIENTRY *glBindRenderbuffer) (GLenum target, GLuint renderbuffer) = NULL;
void (APIENTRY *glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
GLenum (APIENTRY *glCheckFramebufferStatus) (GLenum target) = NULL;

void (APIENTRY *glGenFramebuffersEXT) (GLsizei n, GLuint* ids) = NULL;
void (APIENTRY *glBindFramebufferEXT) (GLenum target, GLuint framebuffer) = NULL;
void (APIENTRY *glGenRenderbuffersEXT) (GLsizei n, GLuint* renderbuffers) = NULL;
void (APIENTRY *glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer) = NULL;
void (APIENTRY *glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = NULL;
GLenum (APIENTRY *glCheckFramebufferStatusEXT) (GLenum target) = NULL;

GLint (APIENTRY *glGetUniformLocation) (GLuint program, const GLchar* name) = NULL;
void (APIENTRY *glUniform1i) (GLint location, GLint v0) = NULL;
void (APIENTRY *glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3) = NULL;

void (APIENTRY *glActiveTexture)(GLenum texture) = NULL;

BOOL (APIENTRY *wglSwapIntervalEXT)(int interval) = NULL;
int (APIENTRY *wglGetSwapIntervalEXT)() = NULL;

int GLEXT_ARB_framebuffer_object = 0;
int GLEXT_EXT_framebuffer_object = 0;
int glver_major, glver_minor = 0;

void InitGLExt()
{
	const GLubyte *glversion = glGetString(GL_VERSION);
	sscanf((char*)glversion,"%d.%d",&glver_major,&glver_minor);
	if((glver_major >= 2) || ((glver_major >= 1) && (glver_minor >= 3)))
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	if(glver_major >= 2)
	{
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
	}
	const GLubyte *glextensions = glGetString(GL_EXTENSIONS);
	if(strstr((char*)glextensions,"GL_ARB_framebuffer_object")) GLEXT_ARB_framebuffer_object = 1;
	if(strstr((char*)glextensions,"GL_EXT_framebuffer_object")) GLEXT_EXT_framebuffer_object = 1;
	if(GLEXT_ARB_framebuffer_object)
	{
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
	}
	if(GLEXT_EXT_framebuffer_object)
	{
		glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
		glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
		glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
		glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
		glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2D");
		glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
	}
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
}
