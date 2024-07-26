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
	GLint internalformats[4];
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
	DWORD_PTR size;  // Size of strucure
	UINT refcount;  // Reference count
	DWORD api;  // API type for texure
	union
	{
		DWORD_PTR handle; // Generic handle
		GLuint glhandle;  // OpenGL texture name
	} DUMMYUNIONNAME1;
	DWORD mipcount;
	struct
	{
		GLsizei width;
		GLsizei height;
		GLsizei depth;  // Future: for depth textures and to pad to 4 DWORDs
		GLsizei pitch;
	} levels[17];  // Dimensions of texture levels, future proofed to 65536
	union
	{
		GLenum gltarget;  // Texure target - for cubemap GL_TEXTURE_CUBE_MAP
	} DUMMYUNIONNAME2;
	union
	{
		DWORD_PTR readbufferhandle;
		GLuint readPBO;
	} DUMMYUNIONNAME3;
	union
	{
		DWORD_PTR writebufferhandle;
		GLuint writePBO;
	} DUMMYUNIONNAME4;
	BYTE *readbuffer;
	BYTE *writebuffer;
	DWORD colororder;
	DWORD colorsizes[4];
	DWORD colorbits[4];
	BOOL intcoords;  // Use integer coords if TRUE, float 0 to 1 if FALSE
	texformat format;  // Texture format description
	texparam params;  // Texture parameters, mostly for OpenGL
	DDPIXELFORMAT ddformat; // DDraw pixel format
	BOOL zhasstencil;
	BOOL useconv;
	int convfunctionupload;
	int convfunctiondownload;
	int internalsize;
	int packsize;
	unsigned char blttype;
	BOOL sysmem;
	BYTE *buffer;
} DXGLTexture;



#ifdef __cplusplus
}
#endif
#endif //_DXGLTEXTURE_H