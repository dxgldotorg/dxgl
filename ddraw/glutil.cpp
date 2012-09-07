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
#include "texture.h"
#include "glutil.h"

bool depthwrite = true;
bool depthtest = false;
GLuint depthcomp = 0;
GLuint alphacomp = 0;
TEXTURE *fbcolor = NULL;
TEXTURE *fbz = NULL;
GLuint fbo = 0;
GLint scissorx = 0;
GLint scissory = 0;
GLsizei scissorwidth = 0;
GLsizei scissorheight = 0;
GLint viewportx = 0;
GLint viewporty = 0;
GLsizei viewportwidth = 0;
GLsizei viewportheight = 0;
GLclampd depthnear = 0.0;
GLclampd depthfar = 1.0;
GLenum matrixmode = GL_MODELVIEW;
GLfloat materialambient[4] = {0,0,0,0};
GLfloat materialdiffuse[4] = {0,0,0,0};
GLfloat materialspecular[4] = {0,0,0,0};
GLfloat materialemission[4] = {0,0,0,0};
GLfloat materialshininess = 0;
bool scissorenabled = false;
bool stencil = false;
GLint texwrap[16];
GLclampf clearr = 0.0;
GLclampf clearg = 0.0;
GLclampf clearb = 0.0;
GLclampf cleara = 0.0;
GLclampd cleardepth = 1.0;
GLint clearstencil = 0;
GLenum blendsrc = GL_ONE;
GLenum blenddest = GL_ZERO;
bool blendenabled = false;
bool arrays[42];
D3DCULL cullmode = D3DCULL_NONE;
bool cullenabled = false;

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

GLenum SetFBO(TEXTURE *color, TEXTURE *z, bool stencil)
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
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color->id,0);
		if(stencil)
		{
			if(!::stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		else
		{
			if(::stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
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
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color->id,0);
		if(stencil)
		{
			if(!::stencil) glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		else
		{
			if(::stencil) glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2DEXT(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		error = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
	}
	::stencil = stencil;
	return error;
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
		//int currtexture = texlevel;
		SetActiveTexture(level);
		if(coord) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapmode);
		else glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapmode);
		//SetActiveTexture(currtexture);
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

void SetScissor(bool enabled, GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(enabled != scissorenabled)
	{
		scissorenabled = enabled;
		if(enabled) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}
	if((x != scissorx) || (y != scissory) || (width != scissorwidth) || (height != scissorheight))
	{
		scissorx = x;
		scissory = y;
		scissorwidth = width;
		scissorheight = height;
		glScissor(x,y,width,height);
	}
}

void MatrixMode(GLenum mode)
{
	if(mode != matrixmode)
	{
		matrixmode = mode;
		glMatrixMode(mode);
	}
}

void SetMatrix(GLenum mode, GLfloat *mat1, GLfloat *mat2, bool *dirty)
{
	MatrixMode(mode);
	glLoadMatrixf(mat1);
	if(mode == GL_MODELVIEW) glMultMatrixf(mat2);
	if(dirty) *dirty = false;
}

void SetMaterial(GLfloat ambient[4],GLfloat diffuse[4],GLfloat specular[4],GLfloat emission[4],GLfloat shininess)
{
	if(memcmp(ambient,materialambient,4*sizeof(GLfloat)))
	{
		memcpy(materialambient,ambient,4*sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
	}
	if(memcmp(diffuse,materialdiffuse,4*sizeof(GLfloat)))
	{
		memcpy(materialdiffuse,diffuse,4*sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
	}
	if(memcmp(specular,materialspecular,4*sizeof(GLfloat)))
	{
		memcpy(materialspecular,specular,4*sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	}
	if(memcmp(emission,materialemission,4*sizeof(GLfloat)))
	{
		memcpy(materialemission,emission,4*sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_EMISSION,emission);
	}
	if(shininess != materialshininess)
	{
		materialshininess = shininess;
		glMaterialf(GL_FRONT,GL_SHININESS,shininess);
	}
}

void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	if((x != viewportx) || (y != viewporty) || (width != viewportwidth) || (height != viewportheight))
	{
		viewportx = x;
		viewporty = y;
		viewportwidth = width;
		viewportheight = height;
		glViewport(x,y,width,height);
	}
}

void SetDepthRange(GLclampd rangenear, GLclampd rangefar)
{
	if((rangenear != depthnear) || (rangefar != depthfar))
	{
		depthnear = rangenear;
		depthfar = rangefar;
		glDepthRange(rangenear,rangefar);
	}
}

void ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
	if((clearr != r) || (clearg != g) || (clearb != b) || (cleara != a))
	{
		clearr = r;
		clearg = g;
		clearb = b;
		cleara = a;
		glClearColor(r,g,b,a);
	}
}

void ClearDepth(GLclampd depth)
{
	if(cleardepth != depth)
	{
		cleardepth = depth;
		glClearDepth(depth);
	}
}

void ClearStencil(GLint stencil)
{
	if(clearstencil != stencil)
	{
		clearstencil = stencil;
		glClearStencil(stencil);
	}
}

void EnableArray(int index, bool enabled)
{
	if(index == -1)
	{
		for(int i = 0; i < 42; i++)
			arrays[i] = false;
		return;
	}
	if(index >= 42) return;
	if(arrays[index] != enabled)
	{
		arrays[index] = enabled;
		if(enabled) glEnableVertexAttribArray(index);
		else glDisableVertexAttribArray(index);
	}
}

void BlendFunc(GLenum src, GLenum dest)
{
	if((blendsrc != src) || (blenddest != dest))
	{
		blendsrc = src;
		blenddest = dest;
		glBlendFunc(src,dest);
	}
}

void BlendEnable(bool enabled)
{
	if(enabled != blendenabled)
	{
		blendenabled = enabled;
		if(enabled) glEnable(GL_BLEND);
		else glDisable(GL_BLEND);
	}
}

void EnableCull(bool enabled)
{
	if(cullenabled != enabled)
	{
		cullenabled = enabled;
		if(enabled) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
	}
}
void SetCull(D3DCULL mode)
{
	if(cullmode != mode)
	{
		cullmode = mode;
		switch(mode)
		{
		case D3DCULL_CCW:
			EnableCull(true);
			glFrontFace(GL_CCW);
			break;
		case D3DCULL_CW:
			EnableCull(true);
			glFrontFace(GL_CW);
			break;
		case D3DCULL_NONE:
			EnableCull(false);
			break;
		}
	}
}