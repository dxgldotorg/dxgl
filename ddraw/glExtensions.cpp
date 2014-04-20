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

#include "common.h"
#include "glExtensions.h"


glExtensions::glExtensions()
{
	atimem = false;
	const GLubyte *glversion = glGetString(GL_VERSION);
	glver_minor = 0;
	if(!sscanf((char*)glversion,"%d.%d",&glver_major,&glver_minor)) glver_major = 0;
	if((glver_major >= 2) || ((glver_major >= 1) && (glver_minor >= 2)))
		glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)wglGetProcAddress("glDrawRangeElements");
	if((glver_major >= 2) || ((glver_major >= 1) && (glver_minor >= 3)))
	{
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
	}
	if((glver_major >= 2) || ((glver_major >= 1) && (glver_minor >= 5)))
	{
		glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
		glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
		glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
	}
	if(glver_major >= 2)
	{
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
		glUniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i");
		glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
		glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");
		glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
		glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
		glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
		glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
		glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
		glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
		glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
		glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
		glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
		glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	}
	else
	{
		MessageBox(NULL,_T("DXGL requires an OpenGL 2.0 or higher compatible graphics card to function.  \
Please contact your graphics card manufacturer for an updated driver.  This program will now exit."),_T("Fatal error"),
			MB_OK|MB_ICONERROR);
		ExitProcess(-1);
	}
	const GLubyte *glextensions = glGetString(GL_EXTENSIONS);
	if(strstr((char*)glextensions,"GL_ARB_framebuffer_object") || (glver_major >= 3)) GLEXT_ARB_framebuffer_object = 1;
	else GLEXT_ARB_framebuffer_object = 0;
	if(strstr((char*)glextensions,"GL_EXT_framebuffer_object")) GLEXT_EXT_framebuffer_object = 1;
	else GLEXT_EXT_framebuffer_object = 0;
	if(strstr((char*)glextensions,"GL_NV_packed_depth_stencil")) GLEXT_NV_packed_depth_stencil = 1;
	else GLEXT_NV_packed_depth_stencil = 0;
	if(strstr((char*)glextensions,"GL_EXT_packed_depth_stencil")) GLEXT_EXT_packed_depth_stencil = 1;
	else GLEXT_EXT_packed_depth_stencil = 0;
	if(strstr((char*)glextensions,"GL_ARB_depth_buffer_float") || (glver_major >= 3)) GLEXT_ARB_depth_buffer_float = 1;
	else GLEXT_ARB_depth_buffer_float = 0;
	if(strstr((char*)glextensions,"GL_ARB_depth_texture") || (glver_major >= 2)
		|| ((glver_major >= 1) && (glver_minor >= 4))) GLEXT_ARB_depth_texture = 1;
	else GLEXT_ARB_depth_texture = 0;
	if(strstr((char*)glextensions,"GL_NVX_gpu_memory_info")) GLEXT_NVX_gpu_memory_info = 1;
	else GLEXT_NVX_gpu_memory_info = 0;
	if(strstr((char*)glextensions,"GL_ATI_meminfo")) GLEXT_ATI_meminfo = 1;
	else GLEXT_ATI_meminfo = 0;
	if(strstr((char*)glextensions,"GL_ARB_ES2_compatibility") || (glver_major >= 5)
		|| ((glver_major >= 4) && (glver_minor >= 1))) GLEXT_ARB_ES2_compatibility = 1;
	else GLEXT_ARB_ES2_compatibility = 0;
	if(strstr((char*)glextensions,"GL_EXT_direct_state_access")) GLEXT_EXT_direct_state_access = 1;
	else GLEXT_EXT_direct_state_access = 0;
	if(strstr((char*)glextensions,"GL_ARB_sampler_objects") || (glver_major >= 4)
		|| ((glver_major >= 3) && (glver_minor >= 3))) GLEXT_ARB_sampler_objects = 1;
	else GLEXT_ARB_sampler_objects = 0;
	bool broken_fbo = true;
	if(GLEXT_ARB_framebuffer_object)
	{
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		if(!glGenFramebuffers) GLEXT_ARB_framebuffer_object = 0;
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		broken_fbo = false;
	}
	if(GLEXT_EXT_framebuffer_object)
	{
		glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
		if(!glGenFramebuffersEXT) GLEXT_EXT_framebuffer_object = 0;
		glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
		glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
		glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
		glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
		glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
		glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
		broken_fbo = false;
	}
	if(GLEXT_EXT_direct_state_access)
	{
		glTextureParameterfEXT = (PFNGLTEXTUREPARAMETERFEXTPROC)wglGetProcAddress("glTextureParameterfEXT");
		glTextureParameterfvEXT = (PFNGLTEXTUREPARAMETERFVEXTPROC)wglGetProcAddress("glTextureParameterfvEXT");
		glTextureParameteriEXT = (PFNGLTEXTUREPARAMETERIEXTPROC)wglGetProcAddress("glTextureParameteriEXT");
		glTextureParameterivEXT = (PFNGLTEXTUREPARAMETERIVEXTPROC)wglGetProcAddress("glTextureParameterivEXT");
		glTextureImage2DEXT = (PFNGLTEXTUREIMAGE2DEXTPROC)wglGetProcAddress("glTextureImage2DEXT");
		glTextureSubImage2DEXT = (PFNGLTEXTURESUBIMAGE2DEXTPROC)wglGetProcAddress("glTextureSubImage2DEXT");
		glGetTextureImageEXT = (PFNGLGETTEXTUREIMAGEEXTPROC)wglGetProcAddress("glGetTextureImageEXT");
		glMatrixLoadfEXT = (PFNGLMATRIXLOADFEXTPROC)wglGetProcAddress("glMatrixLoadfEXT");
		glMatrixMultfEXT = (PFNGLMATRIXMULTFEXTPROC)wglGetProcAddress("glMatrixMultfEXT");
	}
	if(GLEXT_ARB_sampler_objects)
	{
		glBindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("glBindSampler");
		glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)wglGetProcAddress("glDeleteSamplers");
		glGenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("glGenSamplers");
		glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)wglGetProcAddress("glSamplerParameterf");
		glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("glSamplerParameteri");
		glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)wglGetProcAddress("glSamplerParameterfv");
		glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)wglGetProcAddress("glSamplerParameteriv");
	}
	if(broken_fbo)
	{
		MessageBox(NULL,_T("DXGL requires support for OpenGL Framebuffer Objects to function.  \
Please contact your graphics card manufacturer for an updated driver.  This program will now exit."),_T("Fatal error"),
			MB_OK|MB_ICONERROR);
		ExitProcess(-1);
	}
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
}
