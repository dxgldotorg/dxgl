// DXGL
// Copyright (C) 2012-2013 William Feely

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
#include "glDirectDrawSurface.h"

bool depthwrite = true;
bool depthtest = false;
GLuint depthcomp = 0;
GLuint alphacomp = 0;
FBO *currentfbo = NULL;
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
D3DFILLMODE polymode = D3DFILL_SOLID;
D3DSHADEMODE shademode = D3DSHADE_GOURAUD;

void InitFBO(FBO *fbo)
{
	if(!fbo->fbo)
	{
		ZeroMemory(fbo,sizeof(FBO));
		if(GLEXT_ARB_framebuffer_object) glGenFramebuffers(1,&fbo->fbo);
		else if(GLEXT_EXT_framebuffer_object) glGenFramebuffersEXT(1,&fbo->fbo);
	}
}

void DeleteFBO(FBO *fbo)
{
	if(fbo->fbo)
	{
		if(GLEXT_ARB_framebuffer_object)
		{
			if(currentfbo == fbo) glBindFramebuffer(GL_FRAMEBUFFER,0);
			glDeleteFramebuffers(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
		else if(GLEXT_EXT_framebuffer_object)
		{
			if(currentfbo == fbo) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
			glDeleteFramebuffersEXT(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
	}
}

void SetFBOTexture(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil)
{
	if(!color) return;
	if(!fbo->fbo) return;
	if(GLEXT_ARB_framebuffer_object)
	{
		if(currentfbo != fbo) glBindFramebuffer(GL_FRAMEBUFFER,fbo->fbo);
		currentfbo = fbo;
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color->id,0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(!fbo->stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		else
		{
			if(fbo->stencil) glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	else if(GLEXT_EXT_framebuffer_object)
	{
		if(currentfbo != fbo) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo->fbo);
		currentfbo = fbo;
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,color->id,0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(z)
			{
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
			}
			else
			{
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
			}
		}
		else
		{
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
			if(z)glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
			else glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}
}

void SetFBO(glDirectDrawSurface7 *surface)
{
	if(!surface) SetFBO((FBO*)NULL);
	if(surface->zbuffer) SetFBO(&surface->fbo,surface->texture,surface->zbuffer->texture,surface->zbuffer->hasstencil);
	else SetFBO(&surface->fbo,surface->texture,NULL,false);
}

void SetFBO(FBO *fbo)
{
	if(fbo == currentfbo) return;
	if(!fbo)
	{
		if(GLEXT_ARB_framebuffer_object) glBindFramebuffer(GL_FRAMEBUFFER,0);
		else if(GLEXT_EXT_framebuffer_object) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}
	else
	{
		if(GLEXT_ARB_framebuffer_object) glBindFramebuffer(GL_FRAMEBUFFER,fbo->fbo);
		else if(GLEXT_EXT_framebuffer_object) glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo->fbo);
	}
	currentfbo = fbo;
}

void SetFBO(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil)
{
	if(!fbo)
	{
		SetFBO((FBO*)NULL);
		return;
	}
	if(!fbo->fbo) InitFBO(fbo);
	if(!color) return;
	if((color != fbo->fbcolor) || (z != fbo->fbz) || (stencil != fbo->stencil))
		SetFBOTexture(fbo,color,z,stencil);
	if(fbo != currentfbo) SetFBO(fbo);
}

void SetWrap(int level, DWORD coord, DWORD address)
{
	if(level == -1)
	{
		for(int i = 0; i < 16; i++)
			texwrap[i] = GL_REPEAT;
	}
	if(coord > 1) return;
	if(level > 7) return;
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
	//if(texwrap[level*2+coord] == wrapmode) return;
	//else
	{
		texwrap[level*2+coord] = wrapmode;
		//int currtexture = texlevel;
		if(GLEXT_ARB_sampler_objects)
		{
			if(coord)
			{
				if(samplers[level].wrapt != wrapmode)
				{
					glSamplerParameteri(samplers[level].id,GL_TEXTURE_WRAP_T,wrapmode);
					samplers[level].wrapt = wrapmode;
				}
			}
			else
			{
				if(samplers[level].wraps != wrapmode)
				{
					glSamplerParameteri(samplers[level].id,GL_TEXTURE_WRAP_S,wrapmode);
					samplers[level].wraps = wrapmode;
				}
			}
		}
		else
		{
			SetActiveTexture(level);
			if(coord) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapmode);
			else glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapmode);
		}
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
	if(GLEXT_EXT_direct_state_access)
	{
		glMatrixLoadfEXT(mode,mat1);
		if(mode == GL_MODELVIEW) glMatrixMultfEXT(mode,mat2);
	}
	else
	{
		MatrixMode(mode);
		glLoadMatrixf(mat1);
		if(mode == GL_MODELVIEW) glMultMatrixf(mat2);
	}
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
void SetPolyMode(D3DFILLMODE mode)
{
	if(polymode != mode)
	{
		polymode = mode;
		switch(mode)
		{
		case D3DFILL_POINT:
			glPolygonMode(GL_FRONT_AND_BACK,GL_POINT);
			break;
		case D3DFILL_WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
			break;
		case D3DFILL_SOLID:
		default:
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			break;
		}
	}
}

void SetShadeMode(D3DSHADEMODE mode)
{
	if(shademode != mode)
	{
		shademode = mode;
		switch(mode)
		{
		case D3DSHADE_FLAT:
		case 4:
			glShadeModel(GL_FLAT);
			break;
		case D3DSHADE_GOURAUD:
		case D3DSHADE_PHONG:
		default:
			glShadeModel(GL_SMOOTH);
			break;
		}
	}
}