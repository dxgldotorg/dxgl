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

#ifdef __cplusplus
extern "C" {
#endif

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
	GLint internalformats[8];
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
extern int numtexformats;

typedef struct TextureManager
{
	SAMPLER samplers[8];
	glExtensions *ext;
	GLint texlevel;
	GLuint textures[16];
} TextureManager;

TextureManager *TextureManager_Create(glExtensions *glext);
void TextureManager_InitSamplers(TextureManager *This);
void TextureManager_DeleteSamplers(TextureManager *This);
void TextureManager_SetActiveTexture(TextureManager *This, int level);
void TextureManager_SetTexture(TextureManager *This, unsigned int level, TEXTURE *texture);
void TextureManager__CreateTexture(TextureManager *This, TEXTURE *texture, int width, int height);
void TextureManager__DeleteTexture(TextureManager *This, TEXTURE *texture);
void TextureManager__UploadTexture(TextureManager *This, TEXTURE *texture, int level, const void *data, int width, int height, BOOL checkerror);
void TextureManager__DownloadTexture(TextureManager *This, TEXTURE *texture, int level, void *data);
void TextureManager_CreateTextureClassic(TextureManager *This, TEXTURE *texture, int width, int height);
void TextureManager_DeleteTexture(TextureManager *This, TEXTURE *texture);
void TextureManager_UploadTextureClassic(TextureManager *This, TEXTURE *texture, int level, const void *data, int width, int height, BOOL checkerror);
void TextureManager_DownloadTextureClassic(TextureManager *This, TEXTURE *texture, int level, void *data);
BOOL TextureManager_FixTexture(TextureManager *This, TEXTURE *texture, void *data, DWORD *dirty);

#ifdef __cplusplus
}
#endif

#endif //_TEXTURE_H