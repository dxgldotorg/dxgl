// DXGL
// Copyright (C) 2011-2016 William Feely

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


void glExtensions_Init(glExtensions *ext)
{
	const GLubyte *glversion;
	const GLubyte *glextensions;
	BOOL broken_fbo;
	ZeroMemory(ext, sizeof(glExtensions));
	ext->atimem = FALSE;
	glversion = glGetString(GL_VERSION);
	ext->glver_minor = 0;
	if(!sscanf((char*)glversion,"%d.%d",&ext->glver_major,&ext->glver_minor)) ext->glver_major = 0;
	if((ext->glver_major >= 2) || ((ext->glver_major >= 1) && (ext->glver_minor >= 2)))
		ext->glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)wglGetProcAddress("glDrawRangeElements");
	if((ext->glver_major >= 2) || ((ext->glver_major >= 1) && (ext->glver_minor >= 3)))
	{
		ext->glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		ext->glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
	}
	if((ext->glver_major >= 2) || ((ext->glver_major >= 1) && (ext->glver_minor >= 5)))
	{
		ext->glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		ext->glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
		ext->glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		ext->glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		ext->glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
		ext->glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
	}
	if(ext->glver_major >= 2)
	{
		ext->glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		ext->glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		ext->glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		ext->glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		ext->glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		ext->glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		ext->glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		ext->glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		ext->glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		ext->glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		ext->glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		ext->glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		ext->glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		ext->glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		ext->glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		ext->glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		ext->glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
		ext->glUniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i");
		ext->glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
		ext->glUniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv");
		ext->glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");
		ext->glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		ext->glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
		ext->glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
		ext->glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
		ext->glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
		ext->glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
		ext->glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
		ext->glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
		ext->glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		ext->glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
		ext->glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
		ext->glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	}
	else
	{
		MessageBox(NULL,_T("DXGL requires an OpenGL 2.0 or higher compatible graphics card to function.  \
Please contact your graphics card manufacturer for an updated driver.  This program will now exit."),_T("Fatal error"),
			MB_OK|MB_ICONERROR);
		ExitProcess(-1);
	}
	glextensions = glGetString(GL_EXTENSIONS);
	if(strstr((char*)glextensions,"GL_ARB_framebuffer_object") || (ext->glver_major >= 3)) ext->GLEXT_ARB_framebuffer_object = 1;
	else ext->GLEXT_ARB_framebuffer_object = 0;
	if(strstr((char*)glextensions,"GL_EXT_framebuffer_object")) ext->GLEXT_EXT_framebuffer_object = 1;
	else ext->GLEXT_EXT_framebuffer_object = 0;
	if(strstr((char*)glextensions,"GL_NV_packed_depth_stencil")) ext->GLEXT_NV_packed_depth_stencil = 1;
	else ext->GLEXT_NV_packed_depth_stencil = 0;
	if(strstr((char*)glextensions,"GL_EXT_packed_depth_stencil")) ext->GLEXT_EXT_packed_depth_stencil = 1;
	else ext->GLEXT_EXT_packed_depth_stencil = 0;
	if(strstr((char*)glextensions,"GL_ARB_depth_buffer_float") || (ext->glver_major >= 3)) ext->GLEXT_ARB_depth_buffer_float = 1;
	else ext->GLEXT_ARB_depth_buffer_float = 0;
	if(strstr((char*)glextensions,"GL_ARB_depth_texture") || (ext->glver_major >= 2)
		|| ((ext->glver_major >= 1) && (ext->glver_minor >= 4))) ext->GLEXT_ARB_depth_texture = 1;
	else ext->GLEXT_ARB_depth_texture = 0;
	if(strstr((char*)glextensions,"GL_NVX_gpu_memory_info")) ext->GLEXT_NVX_gpu_memory_info = 1;
	else ext->GLEXT_NVX_gpu_memory_info = 0;
	if(strstr((char*)glextensions,"GL_ATI_meminfo")) ext->GLEXT_ATI_meminfo = 1;
	else ext->GLEXT_ATI_meminfo = 0;
	if(strstr((char*)glextensions,"GL_ARB_ES2_compatibility") || (ext->glver_major >= 5)
		|| ((ext->glver_major >= 4) && (ext->glver_minor >= 1))) ext->GLEXT_ARB_ES2_compatibility = 1;
	else ext->GLEXT_ARB_ES2_compatibility = 0;
	if(strstr((char*)glextensions,"GL_EXT_direct_state_access")) ext->GLEXT_EXT_direct_state_access = 1;
	else ext->GLEXT_EXT_direct_state_access = 0;
	if (strstr((char*)glextensions, "GL_ARB_direct_state_access") || (ext->glver_major >= 5)
		|| ((ext->glver_major >= 4) && (ext->glver_minor >= 5))) ext->GLEXT_ARB_direct_state_access = 1;
	else ext->GLEXT_ARB_direct_state_access = 0;
	if (strstr((char*)glextensions, "GL_ARB_sampler_objects") || (ext->glver_major >= 4)
		|| ((ext->glver_major >= 3) && (ext->glver_minor >= 3))) ext->GLEXT_ARB_sampler_objects = 1;
	else ext->GLEXT_ARB_sampler_objects = 0;
	if(strstr((char*)glextensions,"GL_EXT_gpu_shader4")) ext->GLEXT_EXT_gpu_shader4 = 1;
	else ext->GLEXT_EXT_gpu_shader4 = 0;
	if(strstr((char*)glextensions,"GL_GREMEDY_frame_terminator")) ext->GLEXT_GREMEDY_frame_terminator = 1;
	else ext->GLEXT_GREMEDY_frame_terminator = 0;
	broken_fbo = TRUE;
	if(ext->GLEXT_ARB_framebuffer_object)
	{
		ext->glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		if(!ext->glGenFramebuffers) ext->GLEXT_ARB_framebuffer_object = 0;
		ext->glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		ext->glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		ext->glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		ext->glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		ext->glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		ext->glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		broken_fbo = FALSE;
	}
	if(ext->GLEXT_EXT_framebuffer_object)
	{
		ext->glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
		if(!ext->glGenFramebuffersEXT) ext->GLEXT_EXT_framebuffer_object = 0;
		ext->glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
		ext->glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
		ext->glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
		ext->glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
		ext->glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
		ext->glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
		broken_fbo = FALSE;
	}
	if(ext->GLEXT_EXT_direct_state_access)
	{
		ext->glTextureParameterfEXT = (PFNGLTEXTUREPARAMETERFEXTPROC)wglGetProcAddress("glTextureParameterfEXT");
		ext->glTextureParameterfvEXT = (PFNGLTEXTUREPARAMETERFVEXTPROC)wglGetProcAddress("glTextureParameterfvEXT");
		ext->glTextureParameteriEXT = (PFNGLTEXTUREPARAMETERIEXTPROC)wglGetProcAddress("glTextureParameteriEXT");
		ext->glTextureParameterivEXT = (PFNGLTEXTUREPARAMETERIVEXTPROC)wglGetProcAddress("glTextureParameterivEXT");
		ext->glTextureImage2DEXT = (PFNGLTEXTUREIMAGE2DEXTPROC)wglGetProcAddress("glTextureImage2DEXT");
		ext->glTextureSubImage2DEXT = (PFNGLTEXTURESUBIMAGE2DEXTPROC)wglGetProcAddress("glTextureSubImage2DEXT");
		ext->glGetTextureImageEXT = (PFNGLGETTEXTUREIMAGEEXTPROC)wglGetProcAddress("glGetTextureImageEXT");
		ext->glMatrixLoadfEXT = (PFNGLMATRIXLOADFEXTPROC)wglGetProcAddress("glMatrixLoadfEXT");
		ext->glMatrixMultfEXT = (PFNGLMATRIXMULTFEXTPROC)wglGetProcAddress("glMatrixMultfEXT");
		ext->glNamedBufferDataEXT = (PFNGLNAMEDBUFFERDATAEXTPROC)wglGetProcAddress("glNamedBufferDataEXT");
		ext->glMapNamedBufferEXT = (PFNGLMAPNAMEDBUFFEREXTPROC)wglGetProcAddress("glMapNamedBufferEXT");
		ext->glUnmapNamedBufferEXT = (PFNGLUNMAPNAMEDBUFFEREXTPROC)wglGetProcAddress("glUnmapNamedBufferEXT");
	}
	if (ext->GLEXT_ARB_direct_state_access)
	{
		ext->glTextureParameterf = (PFNGLTEXTUREPARAMETERFPROC)wglGetProcAddress("glTextureParameterf");
		ext->glTextureParameterfv = (PFNGLTEXTUREPARAMETERFVPROC)wglGetProcAddress("glTextureParameterfv");
		ext->glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)wglGetProcAddress("glTextureParameteri");
		ext->glTextureParameteriv = (PFNGLTEXTUREPARAMETERIVPROC)wglGetProcAddress("glTextureParameteriv");
		ext->glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)wglGetProcAddress("glTextureSubImage2D");
		ext->glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)wglGetProcAddress("glGetTextureImage");
		ext->glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)wglGetProcAddress("glNamedBufferData");
		ext->glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)wglGetProcAddress("glMapNamedBuffer");
		ext->glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)wglGetProcAddress("glUnmapNamedBuffer");
	}
	if (ext->GLEXT_ARB_sampler_objects)
	{
		ext->glBindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("glBindSampler");
		ext->glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)wglGetProcAddress("glDeleteSamplers");
		ext->glGenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("glGenSamplers");
		ext->glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)wglGetProcAddress("glSamplerParameterf");
		ext->glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("glSamplerParameteri");
		ext->glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)wglGetProcAddress("glSamplerParameterfv");
		ext->glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)wglGetProcAddress("glSamplerParameteriv");
	}
	if (ext->GLEXT_GREMEDY_frame_terminator)
	{
		ext->glFrameTerminatorGREMEDY = (PFNGLFRAMETERMINATORGREMEDYPROC)wglGetProcAddress("glFrameTerminatorGREMEDY");
	}
	if(broken_fbo)
	{
		MessageBox(NULL,_T("DXGL requires support for OpenGL Framebuffer Objects to function.  \
Please contact your graphics card manufacturer for an updated driver.  This program will now exit."),_T("Fatal error"),
			MB_OK|MB_ICONERROR);
		ExitProcess(-1);
	}
	ext->wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	ext->wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
}
