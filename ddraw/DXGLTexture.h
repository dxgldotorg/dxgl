// DXGL
// Copyright (C) 2022 William Feely

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
#ifndef _DXGLTEXTURE_H
#define _DXGLTEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gltexformat
{
	GLint internalformat;
	GLenum format;
	GLenum type;
} gltexformat;

typedef union texformat
{
	gltexformat glformat;
} texformat;

typedef struct gltexparam
{
	GLint magfilter;
	GLint minfilter;
	GLint wraps;
	GLint wrapt;
	GLint wrapr;
	GLint baselevel;
	GLint maxlevel;
	GLfloat minlod;
	GLfloat maxlod;
	GLfloat border[4];
} gltexparam;

typedef union texparam
{
	gltexparam glparam;
} texparam;

typedef struct DXGLTexture
{
	DWORD_PTR size;  // Size of strucure plus data storage
	UINT refcount;  // Reference count
	DWORD api;  // API type for texure
	union
	{
		GLuint glhandle;  // OpenGL texture name
	} DUMMYUNIONNAME1;
	struct
	{
		GLsizei width;
		GLsizei height;
		GLsizei depth;
	} levels[17];  // Dimensions of texture levels, future proofed to 65536
	union
	{
		GLenum gltarget;  // Texure target - for cubemap GL_TEXTURE_CUBE_MAP
	} DUMMYUNIONNAME2;
	texformat format;  // Texture format description
	texparam params;  // Texture parameters, mostly for OpenGL
} DXGLTexture;



#ifdef __cplusplus
}
#endif
#endif //_DXGLTEXTURE_H