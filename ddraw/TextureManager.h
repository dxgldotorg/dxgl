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

typedef struct
{
	GLuint id;
	GLint wraps;
	GLint wrapt;
	GLint minfilter;
	GLint magfilter;
} SAMPLER;

extern const DDPIXELFORMAT texformats[];
extern const int numtexformats;

class TextureManager
{
public:
	TextureManager(glExtensions *glext);
	void InitSamplers();
	void DeleteSamplers();
	void SetActiveTexture(int level);
	void SetTexture(unsigned int level, TEXTURE *texture);

	void _CreateTexture(TEXTURE *texture, int width, int height);
	void _DeleteTexture(TEXTURE *texture);
	void _UploadTexture(TEXTURE *texture, int level, const void *data, int width, int height);
	void _DownloadTexture(TEXTURE *texture, int level, void *data);

	SAMPLER samplers[8];

private:
	void CreateTextureClassic(TEXTURE *texture, int width, int height);
	void DeleteTexture(TEXTURE *texture);
	void UploadTextureClassic(TEXTURE *texture, int level, const void *data, int width, int height);
	void DownloadTextureClassic(TEXTURE *texture, int level, void *data);
	glExtensions *ext;
	GLint texlevel;
	GLuint textures[16];
};

#endif //_TEXTURE_H