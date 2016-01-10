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
#include "glTexture.h"
#include "glUtil.h"
#include "BufferObject.h"
#include "glDirectDrawSurface.h"

extern "C" {

void glUtil_Create(glExtensions *glext, glUtil **out)
{
	glUtil *util;
	util = (glUtil *)malloc(sizeof(glUtil));
	if (!util) return;
	ZeroMemory(util, sizeof(glUtil));
	util->ext = glext;
	util->depthwrite = TRUE;
	util->depthtest = FALSE;
	util->depthcomp = 0;
	util->alphacomp = 0;
	util->currentfbo = NULL;
	util->scissorx = 0;
	util->scissory = 0;
	util->scissorwidth = 0;
	util->scissorheight = 0;
	util->viewportx = 0;
	util->viewporty = 0;
	util->viewportwidth = 0;
	util->viewportheight = 0;
	util->depthnear = 0.0;
	util->depthfar = 1.0;
	util->matrixmode = GL_MODELVIEW;
	ZeroMemory(util->materialambient, 4 * sizeof(GLfloat));
	ZeroMemory(util->materialdiffuse, 4 * sizeof(GLfloat));
	ZeroMemory(util->materialspecular, 4 * sizeof(GLfloat));
	ZeroMemory(util->materialemission, 4 * sizeof(GLfloat));
	util->materialshininess = 0;
	util->scissorenabled = FALSE;
	util->texwrap[16];
	util->clearr = 0.0;
	util->clearg = 0.0;
	util->clearb = 0.0;
	util->cleara = 0.0;
	util->cleardepth = 1.0;
	util->clearstencil = 0;
	util->blendsrc = GL_ONE;
	util->blenddest = GL_ZERO;
	util->blendenabled = FALSE;
	util->arrays[42];
	util->cullmode = D3DCULL_NONE;
	util->cullenabled = FALSE;
	util->polymode = D3DFILL_SOLID;
	util->shademode = D3DSHADE_GOURAUD;
	util->pboPackBinding = util->pboUnpackBinding = util->vboArrayBinding =
		util->vboElementArrayBinding = util->uboUniformBufferBinding = util->LastBoundBuffer = NULL;
	util->refcount = 1;
	*out = util;
	int i;
	if (glext->GLEXT_ARB_sampler_objects)
	{
		memset(util->samplers, 0, 8 * sizeof(SAMPLER));
		for (i = 0; i < 8; i++)
		{
			glext->glGenSamplers(1, &util->samplers[i].id);
			glext->glBindSampler(i, util->samplers[i].id);
			glext->glSamplerParameteri(util->samplers[i].id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glext->glSamplerParameteri(util->samplers[i].id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glext->glSamplerParameteri(util->samplers[i].id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glext->glSamplerParameteri(util->samplers[i].id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	util->texlevel = 0;
	ZeroMemory(util->textures, 16 * sizeof(GLuint));
}

void glUtil_AddRef(glUtil *This)
{
	InterlockedIncrement(&This->refcount);
}

void glUtil_Release(glUtil *This)
{
	InterlockedDecrement(&This->refcount);
	if (!This->refcount)
	{
		int i;
		if (This->ext->GLEXT_ARB_sampler_objects)
		{
			for (i = 0; i < 8; i++)
			{
				This->ext->glBindSampler(i, 0);
				This->ext->glDeleteSamplers(1, &This->samplers[i].id);
				This->samplers[i].id = 0;
			}
		}
		free(This);
	}
}

void glUtil_InitFBO(glUtil *This, FBO *fbo)
{
	if(!fbo->fbo)
	{
		ZeroMemory(fbo,sizeof(FBO));
		if(This->ext->GLEXT_ARB_framebuffer_object) This->ext->glGenFramebuffers(1,&fbo->fbo);
		else if(This->ext->GLEXT_EXT_framebuffer_object) This->ext->glGenFramebuffersEXT(1,&fbo->fbo);
	}
}

void glUtil_DeleteFBO(glUtil *This, FBO *fbo)
{
	if(fbo->fbo)
	{
		if(This->ext->GLEXT_ARB_framebuffer_object)
		{
			if(This->currentfbo == fbo) This->ext->glBindFramebuffer(GL_FRAMEBUFFER,0);
			This->ext->glDeleteFramebuffers(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
		else if(This->ext->GLEXT_EXT_framebuffer_object)
		{
			if(This->currentfbo == fbo) This->ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
			This->ext->glDeleteFramebuffersEXT(1,&fbo->fbo);
			ZeroMemory(fbo,sizeof(FBO));
		}
	}
}

void glUtil_SetFBOTexture(glUtil *This, FBO *fbo, glTexture *color, glTexture *z, BOOL stencil)
{
	if(!color) return;
	if(!fbo->fbo) return;
	if(This->ext->GLEXT_ARB_framebuffer_object)
	{
		if(This->currentfbo != fbo) This->ext->glBindFramebuffer(GL_FRAMEBUFFER,fbo->fbo);
		This->currentfbo = fbo;
		This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color->id,0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(!fbo->stencil) This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z)This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		else
		{
			if(fbo->stencil) This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);
			if(z) This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,z->id,0);
			else This->ext->glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = This->ext->glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}
	else if(This->ext->GLEXT_EXT_framebuffer_object)
	{
		if (This->currentfbo != fbo) This->ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fbo);
		This->currentfbo = fbo;
		This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color->id, 0);
		fbo->fbcolor = color;
		if(stencil)
		{
			if(z)
			{
				This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, z->id, 0);
				This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, z->id, 0);
			}
			else
			{
				This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
				This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
			}
		}
		else
		{
			This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
			if (z)This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, z->id, 0);
			else This->ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0);
		}
		fbo->stencil = stencil;
		fbo->fbz = z;
		fbo->status = This->ext->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}
}

GLenum glUtil_SetFBOSurface(glUtil *This, glDirectDrawSurface7 *surface)
{
	if (!surface) return glUtil_SetFBO(This, (FBO*)NULL);
	if (surface->zbuffer) return glUtil_SetFBOTextures(This, &surface->fbo, surface->texture, surface->zbuffer->texture, surface->zbuffer->hasstencil);
	else return glUtil_SetFBOTextures(This, &surface->fbo, surface->texture, NULL, FALSE);
}

GLenum glUtil_SetFBO(glUtil *This, FBO *fbo)
{
	if (fbo == This->currentfbo)
	{
		if (fbo) return fbo->status;
		else return GL_FRAMEBUFFER_COMPLETE;
	}
	if(!fbo)
	{
		if (This->ext->GLEXT_ARB_framebuffer_object) This->ext->glBindFramebuffer(GL_FRAMEBUFFER, 0);
		else if (This->ext->GLEXT_EXT_framebuffer_object) This->ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	else
	{
		if (This->ext->GLEXT_ARB_framebuffer_object)
		{
			This->ext->glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);
			fbo->status = This->ext->glCheckFramebufferStatus(GL_FRAMEBUFFER);
		}
		else if (This->ext->GLEXT_EXT_framebuffer_object)
		{
			This->ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fbo);
			fbo->status = This->ext->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		}
	}
	This->currentfbo = fbo;
	if (fbo) return fbo->status;
	else
	{
		if (This->ext->GLEXT_ARB_framebuffer_object) return This->ext->glCheckFramebufferStatus(GL_FRAMEBUFFER);
		else if (This->ext->GLEXT_EXT_framebuffer_object) return This->ext->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		else return 0;
	}
}

GLenum glUtil_SetFBOTextures(glUtil *This, FBO *fbo, glTexture *color, glTexture *z, BOOL stencil)
{
	if(!fbo)
	{
		return glUtil_SetFBO(This, (FBO*)NULL);
	}
	if(!fbo->fbo) glUtil_InitFBO(This, fbo);
	if (!color) return GL_INVALID_ENUM;
	if((color != fbo->fbcolor) || (z != fbo->fbz) || (stencil != fbo->stencil))
		glUtil_SetFBOTexture(This, fbo,color,z,stencil);
	if(fbo != This->currentfbo) return glUtil_SetFBO(This, fbo);
	else return fbo->status;
}

void glUtil_SetWrap(glUtil *This, int level, DWORD coord, DWORD address)
{
	if(level == -1)
	{
		for(int i = 0; i < 16; i++)
			This->texwrap[i] = GL_REPEAT;
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
		This->texwrap[level * 2 + coord] = wrapmode;
		//int currtexture = texlevel;
		if (This->ext->GLEXT_ARB_sampler_objects)
		{
			if(coord)
			{
				if(This->samplers[level].wrapt != wrapmode)
				{
					This->ext->glSamplerParameteri(This->samplers[level].id, GL_TEXTURE_WRAP_T, wrapmode);
					This->samplers[level].wrapt = wrapmode;
				}
			}
			else
			{
				if(This->samplers[level].wraps != wrapmode)
				{
					This->ext->glSamplerParameteri(This->samplers[level].id, GL_TEXTURE_WRAP_S, wrapmode);
					This->samplers[level].wraps = wrapmode;
				}
			}
		}
		else
		{
			glUtil_SetActiveTexture(This,level);
			if(coord) glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapmode);
			else glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wrapmode);
		}
		//TextureManager_SetActiveTexture(texman,currtexture);
	}
}

void glUtil_DepthWrite(glUtil *This, BOOL enabled)
{
	if (enabled != This->depthwrite)
	{
		This->depthwrite = enabled;
		if (This->depthwrite) glDepthMask(GL_TRUE);
		else glDepthMask(GL_FALSE);
	}
}
void glUtil_DepthTest(glUtil *This, BOOL enabled)
{
	if (enabled != This->depthtest)
	{
		This->depthtest = enabled;
		if (This->depthtest) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}
}
void glUtil_SetDepthComp(glUtil *This, GLenum comp)
{
	if (comp != This->depthcomp)
	{
		This->depthcomp = comp;
		glDepthFunc(comp);
	}
}

void glUtil_SetScissor(glUtil *This, BOOL enabled, GLint x, GLint y, GLsizei width, GLsizei height)
{
	if (enabled != This->scissorenabled)
	{
		This->scissorenabled = enabled;
		if(enabled) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}
	if ((x != This->scissorx) || (y != This->scissory) ||
		(width != This->scissorwidth) || (height != This->scissorheight))
	{
		This->scissorx = x;
		This->scissory = y;
		This->scissorwidth = width;
		This->scissorheight = height;
		glScissor(x,y,width,height);
	}
}

void glUtil_MatrixMode(glUtil *This, GLenum mode)
{
	if (mode != This->matrixmode)
	{
		This->matrixmode = mode;
		glMatrixMode(mode);
	}
}

void glUtil_SetMatrix(glUtil *This, GLenum mode, GLfloat *mat1, GLfloat *mat2, BOOL *dirty)
{
	if(This->ext->GLEXT_EXT_direct_state_access)
	{
		This->ext->glMatrixLoadfEXT(mode, mat1);
		if (mode == GL_MODELVIEW) This->ext->glMatrixMultfEXT(mode, mat2);
	}
	else
	{
		glUtil_MatrixMode(This, mode);
		glLoadMatrixf(mat1);
		if(mode == GL_MODELVIEW) glMultMatrixf(mat2);
	}
	if(dirty) *dirty = FALSE;
}

void glUtil_SetMaterial(glUtil *This, GLfloat ambient[4],GLfloat diffuse[4],GLfloat specular[4],GLfloat emission[4],GLfloat shininess)
{
	if(memcmp(ambient,This->materialambient,4*sizeof(GLfloat)))
	{
		memcpy(This->materialambient, ambient, 4 * sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
	}
	if (memcmp(diffuse, This->materialdiffuse, 4 * sizeof(GLfloat)))
	{
		memcpy(This->materialdiffuse, diffuse, 4 * sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
	}
	if (memcmp(specular, This->materialspecular, 4 * sizeof(GLfloat)))
	{
		memcpy(This->materialspecular, specular, 4 * sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	}
	if (memcmp(emission, This->materialemission, 4 * sizeof(GLfloat)))
	{
		memcpy(This->materialemission, emission, 4 * sizeof(GLfloat));
		glMaterialfv(GL_FRONT,GL_EMISSION,emission);
	}
	if (shininess != This->materialshininess)
	{
		This->materialshininess = shininess;
		glMaterialf(GL_FRONT,GL_SHININESS,shininess);
	}
}

void glUtil_SetViewport(glUtil *This, GLint x, GLint y, GLsizei width, GLsizei height)
{
	if ((x != This->viewportx) || (y != This->viewporty) ||
		(width != This->viewportwidth) || (height != This->viewportheight))
	{
		This->viewportx = x;
		This->viewporty = y;
		This->viewportwidth = width;
		This->viewportheight = height;
		glViewport(x,y,width,height);
	}
}

void glUtil_SetDepthRange(glUtil *This, GLclampd rangenear, GLclampd rangefar)
{
	if ((rangenear != This->depthnear) || (rangefar != This->depthfar))
	{
		This->depthnear = rangenear;
		This->depthfar = rangefar;
		glDepthRange(rangenear,rangefar);
	}
}

void glUtil_ClearColor(glUtil *This, GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
	if ((This->clearr != r) || (This->clearg != g) ||
		(This->clearb != b) || (This->cleara != a))
	{
		This->clearr = r;
		This->clearg = g;
		This->clearb = b;
		This->cleara = a;
		glClearColor(r,g,b,a);
	}
}

void glUtil_ClearDepth(glUtil *This, GLclampd depth)
{
	if (This->cleardepth != depth)
	{
		This->cleardepth = depth;
		glClearDepth(depth);
	}
}

void glUtil_ClearStencil(glUtil *This, GLint stencil)
{
	if (This->clearstencil != stencil)
	{
		This->clearstencil = stencil;
		glClearStencil(stencil);
	}
}

void glUtil_EnableArray(glUtil *This, int index, BOOL enabled)
{
	if(index == -1)
	{
		for(int i = 0; i < 42; i++)
			This->arrays[i] = FALSE;
		return;
	}
	if(index >= 42) return;
	if (This->arrays[index] != enabled)
	{
		This->arrays[index] = enabled;
		if (enabled) This->ext->glEnableVertexAttribArray(index);
		else This->ext->glDisableVertexAttribArray(index);
	}
}

void glUtil_BlendFunc(glUtil *This, GLenum src, GLenum dest)
{
	if ((This->blendsrc != src) || (This->blenddest != dest))
	{
		This->blendsrc = src;
		This->blenddest = dest;
		glBlendFunc(src,dest);
	}
}

void glUtil_BlendEnable(glUtil *This, BOOL enabled)
{
	if(enabled != This->blendenabled)
	{
		This->blendenabled = enabled;
		if(enabled) glEnable(GL_BLEND);
		else glDisable(GL_BLEND);
	}
}

void glUtil_EnableCull(glUtil *This, BOOL enabled)
{
	if (This->cullenabled != enabled)
	{
		This->cullenabled = enabled;
		if(enabled) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
	}
}
void glUtil_SetCull(glUtil *This, D3DCULL mode)
{
	if (This->cullmode != mode)
	{
		This->cullmode = mode;
		switch(mode)
		{
		case D3DCULL_CCW:
			glUtil_EnableCull(This, TRUE);
			glFrontFace(GL_CCW);
			break;
		case D3DCULL_CW:
			glUtil_EnableCull(This, TRUE);
			glFrontFace(GL_CW);
			break;
		case D3DCULL_NONE:
			glUtil_EnableCull(This, FALSE);
			break;
		}
	}
}
void glUtil_SetPolyMode(glUtil *This, D3DFILLMODE mode)
{
	if (This->polymode != mode)
	{
		This->polymode = mode;
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

void glUtil_SetShadeMode(glUtil *This, D3DSHADEMODE mode)
{
	if (This->shademode != mode)
	{
		This->shademode = mode;
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

void glUtil_BindBuffer(glUtil *This, BufferObject *buffer, GLenum target)
{
	switch (target)
	{
	case GL_PIXEL_PACK_BUFFER:
		This->LastBoundBuffer = This->pboPackBinding;
		This->pboPackBinding = buffer;
		break;
	case GL_PIXEL_UNPACK_BUFFER:
		This->LastBoundBuffer = This->pboUnpackBinding;
		This->pboUnpackBinding = buffer;
		break;
	case GL_ARRAY_BUFFER:
		This->LastBoundBuffer = This->vboArrayBinding;
		This->vboArrayBinding = buffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		This->LastBoundBuffer = This->vboElementArrayBinding;
		This->vboElementArrayBinding = buffer;
		break;
	case GL_UNIFORM_BUFFER:
		This->LastBoundBuffer = This->uboUniformBufferBinding;
		This->uboUniformBufferBinding = buffer;
		break;
	default:
		This->LastBoundBuffer = NULL;
	}
	if (buffer) This->ext->glBindBuffer(target, buffer->buffer);
	else This->ext->glBindBuffer(target, 0);
}

void glUtil_UndoBindBuffer(glUtil *This, GLenum target)
{
	if (This->LastBoundBuffer) This->ext->glBindBuffer(target, This->LastBoundBuffer->buffer);
	else This->ext->glBindBuffer(target, 0);
	This->LastBoundBuffer = NULL;
}

void glUtil_SetActiveTexture(glUtil *This, int level)
{
	if (level != This->texlevel)
	{
		This->texlevel = level;
		This->ext->glActiveTexture(GL_TEXTURE0 + level);
	}
}


void glUtil_SetTexture(glUtil *This, unsigned int level, glTexture *texture)
{
	GLuint texname;
	if (level >= 16) return;
	if (!texture) texname = 0;
	else texname = texture->id;
	if (texname != This->textures[level])
	{
		glUtil_SetActiveTexture(This, level);
		glBindTexture(GL_TEXTURE_2D, texname);
	}
}

};