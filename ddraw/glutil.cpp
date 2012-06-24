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

#include "common.h"
#include "glutil.h"

bool depthwrite = true;
bool depthtest = false;
GLuint depthcomp = 0;
GLuint alphacomp = 0;
GLuint fbcolor = 0;
GLuint fbz = 0;
GLuint fbo = 0;
bool stencil = false;
GLint texlevel = 0;
GLint texwrap[16];

void InitFBO()
{
	if(GLEXT_ARB_framebuffer_object)
	{
		glGenFramebuffers(1,&fbo);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
}
	else if(GLEXT_EXT_framebuffer_object)
	{
		glGenFramebuffersEXT(1,&fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}
}

void DeleteFBO()
{
	if(GLEXT_ARB_framebuffer_object)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glDeleteFramebuffers(1,&fbo);
	}
	else if(GLEXT_EXT_framebuffer_object)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
		glDeleteFramebuffersEXT(1,&fbo);
	}
}

GLenum SetFBO(GLint color, GLint z, bool stencil)
{
	GLenum error;
	if((fbcolor == color) && (fbz == z)) return 0;
	fbcolor = color;
	fbz = z;
	if(GLEXT_ARB_framebuffer_object)
	{
		if(!color)
		{
			glBindFramebuffer(GL_FRAMEBUFFER,0);
			return 0;
		}
		else glBindFramebuffer(GL_FRAMEBUFFER,fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
		if(stencil)
		{
			if(!::stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z,0);
		}
		else
		{
			if(::stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z,0);
		}
		error = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	else if(GLEXT_EXT_framebuffer_object)
	{
		if(!color)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER,0);
			return 0;
		}
		else glBindFramebufferEXT(GL_FRAMEBUFFER,fbo);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
		if(stencil)
		{
			if(!::stencil) glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z,0);
		}
		else
		{
			if(::stencil) glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z,0);
		}
		error = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
	}
	::stencil = stencil;
	return error;
}

void SetActiveTexture(int level)
{
	if(level != texlevel)
	{
		texlevel = level;
		glActiveTexture(GL_TEXTURE0+level);
	}
}

void SetTexture(int level,GLuint texture)
{
	SetActiveTexture(level);
	glBindTexture(GL_TEXTURE_2D,texture);
}

void SetWrap(int level, DWORD coord, DWORD address)
{
	if(level == -1)
	{
		for(int i = 0; i < 16; i++)
			texwrap[i] = GL_REPEAT;
	}
	if(coord > 1) return;
	if(level > 8) return;
	if(level < 0) return;
	GLint wrapmode;
	switch(address)
	{
	case D3DTADDRESS_WRAP:
		wrapmode = GL_REPEAT;
		break;
	case D3DTADDRESS_MIRROR:
		wrapmode = GL_MIRRORED_REPEAT;
		break;
	case D3DTADDRESS_CLAMP:
		wrapmode = GL_CLAMP_TO_EDGE;
		break;
	case D3DTADDRESS_BORDER:
		wrapmode = GL_CLAMP_TO_BORDER;
		break;
	default:
		return;
	}
	if(texwrap[level*2+coord] == wrapmode) return;
	else
	{
		int currtexture = texlevel;
		SetActiveTexture(level);
		if(coord) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapmode);
		else glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapmode);
		SetActiveTexture(currtexture);
	}
}

void DepthWrite(bool enabled)
{
	if(enabled != depthwrite)
	{
		depthwrite = enabled;
		if(depthwrite) glDepthMask(GL_TRUE);
		else glDepthMask(GL_FALSE);
	}
}
void DepthTest(bool enabled)
{
	if(enabled != depthtest)
	{
		depthtest = enabled;
		if(depthtest) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}
}
void SetDepthComp(GLenum comp)
{
	if(comp != depthcomp)
	{
		depthcomp = comp;
		glDepthFunc(comp);
	}
}