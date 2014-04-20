// DXGL
// Copyright (C) 2012-2014 William Feely

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
#include "TextureManager.h"
#include "glUtil.h"
#include "glDirectDrawSurface.h"

glUtil::glUtil(glExtensions *glext)
{
	ext = glext;
	depthwrite = true;
	depthtest = false;
	depthcomp = 0;
	alphacomp = 0;
	currentfbo = NULL;
	scissorx = 0;
	scissory = 0;
	scissorwidth = 0;
	scissorheight = 0;
	viewportx = 0;
	viewporty = 0;
	viewportwidth = 0;
	viewportheight = 0;
	depthnear = 0.0;
	depthfar = 1.0;
	matrixmode = GL_MODELVIEW;
	ZeroMemory(materialambient, 4 * sizeof(GLfloat));
	ZeroMemory(materialdiffuse, 4 * sizeof(GLfloat));
	ZeroMemory(materialspecular, 4 * sizeof(GLfloat));
	ZeroMemory(materialemission, 4 * sizeof(GLfloat));
	materialshininess = 0;
	scissorenabled = false;
	texwrap[16];
	clearr = 0.0;
	clearg = 0.0;
	clearb = 0.0;
	cleara = 0.0;
	cleardepth = 1.0;
	clearstencil = 0;
	blendsrc = GL_ONE;
	blenddest = GL_ZERO;
	blendenabled = false;
	arrays[42];
	cullmode = D3DCULL_NONE;
	cullenabled = false;
	polymode = D3DFILL_SOLID;
	shademode = D3DSHADE_GOURAUD;
}
void glUtil::InitFBO(FBO *fbo)
{
	if(!fbo->fbo)
	{
		ZeroMemory(fbo,sizeof(FBO));
		if(ext->GLEXT_ARB_framebuffer_object) ext->glGenFramebuffers(1,&fbo->fbo);
		else if(ext->GLEXT_EXT_framebuffer_object) ext->glGenFramebuffersEXT(1,&fbo->fbo);
	}
}

void glUtil::DeleteFBO(FBO *fbo)
{
	if(fbo->fbo)
	{
		if(ext->GLEXT_ARB_framebuffer_object)
		{
			if(currentfbo == fbo) ext->glBindFramebuffer(GL_FRAMEBUFFER,0);
			ext->glDeleteFramebuffers(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
		else if(ext->GLEXT_EXT_framebuffer_object)
		{
			if(currentfbo == fbo) ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
			ext->glDeleteFramebuffersEXT(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
	}
}

void glUtil::SetFBOTexture(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil)
{
	if(!color) return;
	if(!fbo->fbo) return;
	if(ext->GLEXT_ARB_framebuffer_object)
	{
		if(currentfbo != fbo) ext->glBindFramebuffer(GL_FRAMEBUFFER,fbo->fbo);
		currentfbo = fbo;
		ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color->id,0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(!fbo->stencil) ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		else
		{
			if(fbo->stencil) ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z) ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = ext->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	else if(ext->GLEXT_EXT_framebuffer_object)
	{
		if(currentfbo != fbo) ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo->fbo);
		currentfbo = fbo;
		ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,color->id,0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(z)
			{
				ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
				ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
			}
			else
			{
				ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
				ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
			}
		}
		else
		{
			ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_STENCIL_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
			if(z)ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,z->id,0);
			else ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D,0,0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = ext->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}
}

void glUtil::SetFBO(glDirectDrawSurface7 *surface)
{
	if(!surface) SetFBO((FBO*)NULL);
	if(surface->zbuffer) SetFBO(&surface->fbo,surface->texture,surface->zbuffer->texture,surface->zbuffer->hasstencil);
	else SetFBO(&surface->fbo,surface->texture,NULL,false);
}

void glUtil::SetFBO(FBO *fbo)
{
	if(fbo == currentfbo) return;
	if(!fbo)
	{
		if(ext->GLEXT_ARB_framebuffer_object) ext->glBindFramebuffer(GL_FRAMEBUFFER,0);
		else if(ext->GLEXT_EXT_framebuffer_object) ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	}
	else
	{
		if(ext->GLEXT_ARB_framebuffer_object) ext->glBindFramebuffer(GL_FRAMEBUFFER,fbo->fbo);
		else if(ext->GLEXT_EXT_framebuffer_object) ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fbo->fbo);
	}
	currentfbo = fbo;
}

void glUtil::SetFBO(FBO *fbo, TEXTURE *color, TEXTURE *z, bool stencil)
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

void glUtil::SetWrap(int level, DWORD coord, DWORD address, TextureManager *texman)
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
		if(ext->GLEXT_ARB_sampler_objects)
		{
			if(coord)
			{
				if(texman->samplers[level].wrapt != wrapmode)
				{
					ext->glSamplerParameteri(texman->samplers[level].id,GL_TEXTURE_WRAP_T,wrapmode);
					texman->samplers[level].wrapt = wrapmode;
				}
			}
			else
			{
				if(texman->samplers[level].wraps != wrapmode)
				{
					ext->glSamplerParameteri(texman->samplers[level].id,GL_TEXTURE_WRAP_S,wrapmode);
					texman->samplers[level].wraps = wrapmode;
				}
			}
		}
		else
		{
			texman->SetActiveTexture(level);
			if(coord) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapmode);
			else glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapmode);
		}
		//SetActiveTexture(currtexture);
	}
}

void glUtil::DepthWrite(bool enabled)
{
	if(enabled != depthwrite)
	{
		depthwrite = enabled;
		if(depthwrite) glDepthMask(GL_TRUE);
		else glDepthMask(GL_FALSE);
	}
}
void glUtil::DepthTest(bool enabled)
{
	if(enabled != depthtest)
	{
		depthtest = enabled;
		if(depthtest) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}
}
void glUtil::SetDepthComp(GLenum comp)
{
	if(comp != depthcomp)
	{
		depthcomp = comp;
		glDepthFunc(comp);
	}
}

void glUtil::SetScissor(bool enabled, GLint x, GLint y, GLsizei width, GLsizei height)
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

void glUtil::MatrixMode(GLenum mode)
{
	if(mode != matrixmode)
	{
		matrixmode = mode;
		glMatrixMode(mode);
	}
}

void glUtil::SetMatrix(GLenum mode, GLfloat *mat1, GLfloat *mat2, bool *dirty)
{
	if(ext->GLEXT_EXT_direct_state_access)
	{
		ext->glMatrixLoadfEXT(mode,mat1);
		if(mode == GL_MODELVIEW) ext->glMatrixMultfEXT(mode,mat2);
	}
	else
	{
		MatrixMode(mode);
		glLoadMatrixf(mat1);
		if(mode == GL_MODELVIEW) glMultMatrixf(mat2);
	}
	if(dirty) *dirty = false;
}

void glUtil::SetMaterial(GLfloat ambient[4],GLfloat diffuse[4],GLfloat specular[4],GLfloat emission[4],GLfloat shininess)
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

void glUtil::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
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

void glUtil::SetDepthRange(GLclampd rangenear, GLclampd rangefar)
{
	if((rangenear != depthnear) || (rangefar != depthfar))
	{
		depthnear = rangenear;
		depthfar = rangefar;
		glDepthRange(rangenear,rangefar);
	}
}

void glUtil::ClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
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

void glUtil::ClearDepth(GLclampd depth)
{
	if(cleardepth != depth)
	{
		cleardepth = depth;
		glClearDepth(depth);
	}
}

void glUtil::ClearStencil(GLint stencil)
{
	if(clearstencil != stencil)
	{
		clearstencil = stencil;
		glClearStencil(stencil);
	}
}

void glUtil::EnableArray(int index, bool enabled)
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
		if(enabled) ext->glEnableVertexAttribArray(index);
		else ext->glDisableVertexAttribArray(index);
	}
}

void glUtil::BlendFunc(GLenum src, GLenum dest)
{
	if((blendsrc != src) || (blenddest != dest))
	{
		blendsrc = src;
		blenddest = dest;
		glBlendFunc(src,dest);
	}
}

void glUtil::BlendEnable(bool enabled)
{
	if(enabled != blendenabled)
	{
		blendenabled = enabled;
		if(enabled) glEnable(GL_BLEND);
		else glDisable(GL_BLEND);
	}
}

void glUtil::EnableCull(bool enabled)
{
	if(cullenabled != enabled)
	{
		cullenabled = enabled;
		if(enabled) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
	}
}
void glUtil::SetCull(D3DCULL mode)
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
void glUtil::SetPolyMode(D3DFILLMODE mode)
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

void glUtil::SetShadeMode(D3DSHADEMODE mode)
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