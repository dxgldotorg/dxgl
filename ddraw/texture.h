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

#pragma once
#ifndef _TEXTURE_H
#define _TEXTURE_H

typedef struct
{
	GLuint id;
	GLsizei width;
	GLsizei height;
	GLint minfilter;
	GLint magfilter;
	GLint wraps;
	GLint wrapt;
	GLint miplevel;
	DWORD bordercolor;
	GLint internalformat;
	GLenum format;
	GLenum type;
	GLuint pbo;
	DDPIXELFORMAT pixelformat;
} TEXTURE;

void InitTexture(DXGLCFG *cfg);
void SetActiveTexture(int level);
void SetTexture(unsigned int level, TEXTURE *texture);

extern void (*_CreateTexture)(TEXTURE *texture, int width, int height);
extern void (*_DeleteTexture)(TEXTURE *texture);
extern void (*_UploadTexture)(TEXTURE *texture, int level, const void *data, int width, int height);
extern void (*_DownloadTexture)(TEXTURE *texture, int level, void *data);
extern const DDPIXELFORMAT texformats[];
extern const int numtexformats;

#endif //_TEXTURE_H