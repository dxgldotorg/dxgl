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

// Use EXACTLY one line per entry.  Don't change layout of the list.
static const int START_TEXFORMATS = __LINE__;
const DDPIXELFORMAT texformats[] = 
{ // Size					Flags							FOURCC	bits	R/Ymask		G/U/Zmask	B/V/STmask	A/Zmask
	{sizeof(DDPIXELFORMAT),	DDPF_PALETTEINDEXED8,			0,		8,		0,			0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		8,		0xE0,		0x1C,		0x3,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0x7C00,		0x3E0,		0x1F,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		16,		0xF800,		0x7E0,		0x1F,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		24,		0xFF0000,	0xFF00,		0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF0000,	0xFF00,		0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB,						0,		32,		0xFF,		0xFF00,		0xFF0000,	0},
	{sizeof(DDPIXELFORMAT),	DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xE0,		0x1C,		0x3,		0xFF00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0xF00,		0xF0,		0xF,		0xF000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		16,		0x7c00,		0x3E0,		0x1F,		0x8000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_ALPHAPIXELS,		0,		32,		0xFF0000,	0xFF00,		0xFF,		0xFF000000},
	{sizeof(DDPIXELFORMAT), DDPF_LUMINANCE,					0,		8,		0xFF,		0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ALPHA,						0,		8,		0,			0,			0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_LUMINANCE|DDPF_ALPHAPIXELS,0,		16,		0xFF,		0,			0,			0xFF00},
	{sizeof(DDPIXELFORMAT), DDPF_ZBUFFER,					0,		16,		0,			0xFFFF,		0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		24,		0,			0xFFFFFF00,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFF00,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		0,			0xFFFFFFFF,	0,			0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFFFFFF00,	0xFF,		0},
	{sizeof(DDPIXELFORMAT),	DDPF_ZBUFFER,					0,		32,		8,			0xFF,		0xFFFFFF00,	0}
};
static const int END_TEXFORMATS = __LINE__ - 4;
int numtexformats;

void ClearError()
{
	do
	{
		if (glGetError() == GL_NO_ERROR) break;
	} while (1);
}

TextureManager *TextureManager_Create(glExtensions *glext)
{
	TextureManager *newtex;
	numtexformats = END_TEXFORMATS - START_TEXFORMATS;
	newtex = (TextureManager*)malloc(sizeof(TextureManager));
	if (!newtex) return 0;
	ZeroMemory(newtex, sizeof(TextureManager));
	newtex->ext = glext;
	newtex->texlevel = 0;
	ZeroMemory(newtex->textures, 16 * sizeof(GLuint));
	return newtex;
}

void TextureManager__CreateTexture(TextureManager *This, TEXTURE *texture, int width, int height)
{
	TextureManager_CreateTextureClassic(This, texture, width, height);
}
void TextureManager__DeleteTexture(TextureManager *This, TEXTURE *texture)
{
	TextureManager_DeleteTexture(This, texture);
}
void TextureManager__UploadTexture(TextureManager *This, TEXTURE *texture, int level, const void *data, int width, int height, BOOL checkerror)
{
	TextureManager_UploadTextureClassic(This, texture, level, data, width, height, checkerror);
}
void TextureManager__DownloadTexture(TextureManager *This, TEXTURE *texture, int level, void *data)
{
	TextureManager_DownloadTextureClassic(This, texture, level, data);
}

void TextureManager_InitSamplers(TextureManager *This)
{
	int i;
	if(This->ext->GLEXT_ARB_sampler_objects)
	{
		memset(This->samplers,0,8*sizeof(SAMPLER));
		for(i = 0; i < 8; i++)
		{
			This->ext->glGenSamplers(1,&This->samplers[i].id);
			This->ext->glBindSampler(i,This->samplers[i].id);
			This->ext->glSamplerParameteri(This->samplers[i].id,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			This->ext->glSamplerParameteri(This->samplers[i].id,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			This->ext->glSamplerParameteri(This->samplers[i].id,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			This->ext->glSamplerParameteri(This->samplers[i].id,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
	}
}
void TextureManager_DeleteSamplers(TextureManager *This)
{
	int i;
	if(This->ext->GLEXT_ARB_sampler_objects)
	{
		for(i = 0; i < 8; i++)
		{
			This->ext->glBindSampler(i,0);
			This->ext->glDeleteSamplers(1,&This->samplers[i].id);
			This->samplers[i].id = 0;
		}
	}
}

void TextureManager_CreateTextureClassic(TextureManager *This, TEXTURE *texture, int width, int height)
{
	int texformat = -1;
	int i;
	GLenum error;
	texture->pixelformat.dwSize = sizeof(DDPIXELFORMAT);
	for(i = 0; i < numtexformats; i++)
	{
		if(!memcmp(&texformats[i],&texture->pixelformat,sizeof(DDPIXELFORMAT)))
		{
			texformat = i;
			break;
		}
	}
	ZeroMemory(texture->internalformats, 8 * sizeof(GLint));
	switch(texformat)
	{
	case -1:
	case 0: // 8-bit palette
		if(This->ext->glver_major >= 3)
		{
			texture->internalformats[0] = GL_R8;
			texture->format = GL_RED;
		}
		else
		{
			texture->internalformats[0] = GL_RGBA8;
			texture->format = GL_LUMINANCE;
		}
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 1: // 8-bit RGB332
		texture->internalformats[0] = GL_R3_G3_B2;
		texture->internalformats[1] = GL_RGB8;
		texture->internalformats[2] = GL_RGBA8;
		texture->format = GL_RGB;
		texture->type = GL_UNSIGNED_BYTE_3_3_2;
		break;
	case 2: // 16-bit RGB555
		texture->internalformats[0] = GL_RGB5_A1;
		texture->internalformats[1] = GL_RGB8;
		texture->internalformats[2] = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case 3: // 16-bit RGB565
		texture->internalformats[0] = GL_RGB565;
		texture->internalformats[1] = GL_RGB8;
		texture->internalformats[2] = GL_RGBA8;
		texture->format = GL_RGB;
		texture->type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case 4: // 24-bit RGB888
		texture->internalformats[0] = GL_RGB8;
		texture->internalformats[1] = GL_RGBA8;
		texture->format = GL_BGR;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 5: // 32-bit RGB888
		texture->internalformats[0] = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 6: // 32-bit BGR888
		texture->internalformats[0] = GL_RGBA8;
		texture->format = GL_RGBA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 7: // 16-bit RGBA8332
		FIXME("Unusual texture format RGBA8332 not supported");
		break;
	case 8: // 16-bit RGBA4444
		texture->internalformats[0] = GL_RGBA4;
		texture->internalformats[1] = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		break;
	case 9: // 16-bit RGBA1555
		texture->internalformats[0] = GL_RGB5_A1;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case 10: // 32-bit RGBA8888
		texture->internalformats[0] = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 11: // 8-bit Luminance
		texture->internalformats[0] = GL_LUMINANCE8;
		texture->internalformats[1] = GL_RGB8;
		texture->internalformats[2] = GL_RGBA8;
		texture->format = GL_LUMINANCE;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 12: // 8-bit Alpha
		texture->internalformats[0] = GL_ALPHA8;
		texture->format = GL_ALPHA;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 13: // 16-bit Luminance Alpha
		texture->internalformats[0] = GL_LUMINANCE8_ALPHA8;
		texture->internalformats[1] = GL_RGBA8;
		texture->format = GL_LUMINANCE_ALPHA;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 14: // 16-bit Z buffer
		texture->internalformats[0] = GL_DEPTH_COMPONENT16;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_SHORT;
		break;
	case 15: // 24-bit Z buffer
		texture->internalformats[0] = GL_DEPTH_COMPONENT24;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 16: // 32/24 bit Z buffer
		texture->internalformats[0] = GL_DEPTH_COMPONENT24;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 17: // 32-bit Z buffer
		texture->internalformats[0] = GL_DEPTH_COMPONENT32;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 18: // 32-bit Z/Stencil buffer, depth LSB
		texture->internalformats[0] = GL_DEPTH24_STENCIL8;
		texture->format = GL_DEPTH_STENCIL;
		texture->type = GL_UNSIGNED_INT_24_8;
		break;
	case 19: // 32-bit Z/Stencil buffer, depth MSB
		texture->internalformats[0] = GL_DEPTH24_STENCIL8;
		texture->format = GL_DEPTH_STENCIL;
		texture->type = GL_UNSIGNED_INT_24_8;
		break;
	}
	texture->width = width;
	texture->height = height;
	glGenTextures(1,&texture->id);
	TextureManager_SetTexture(This,0,texture);
	do
	{
		ClearError();
		glTexImage2D(GL_TEXTURE_2D, 0, texture->internalformats[0], texture->width, texture->height, 0, texture->format, texture->type, NULL);
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			if (texture->internalformats[1] == 0)
			{
				FIXME("Failed to create texture, cannot find internal format");
				break;
			}
			memmove(&texture->internalformats[0], &texture->internalformats[1], 7 * sizeof(GLint));
			texture->internalformats[7] = 0;
		}
		else break;
	} while (1);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texture->minfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texture->magfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,texture->wraps);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,texture->wrapt);
}

void TextureManager_DeleteTexture(TextureManager *This, TEXTURE *texture)
{
	glDeleteTextures(1,&texture->id);
	texture->bordercolor = texture->format = texture->type = texture->width =
		texture->height = texture->magfilter = texture->minfilter =
		texture->miplevel = texture->wraps = texture->wrapt =
		texture->pbo = texture->id = 0;
	ZeroMemory(texture->internalformats, 8 * sizeof(GLint));
}

void TextureManager_UploadTextureClassic(TextureManager *This, TEXTURE *texture, int level, const void *data, int width, int height, BOOL checkerror)
{
	GLenum error;
	texture->width = width;
	texture->height = height;
	if (checkerror)
	{
		do
		{
			ClearError();
			if (This->ext->GLEXT_EXT_direct_state_access) This->ext->glTextureImage2DEXT(texture->id, GL_TEXTURE_2D, level, texture->internalformats[0],
				width, height, 0, texture->format, texture->type, data);
			else
			{
				TextureManager_SetActiveTexture(This, 0);
				TextureManager_SetTexture(This, 0, texture);
				glTexImage2D(GL_TEXTURE_2D, level, texture->internalformats[0], width, height, 0, texture->format, texture->type, data);
			}
			error = glGetError();
			if (error != GL_NO_ERROR)
			{
				if (texture->internalformats[1] == 0)
				{
					FIXME("Failed to update texture, cannot find internal format");
					break;
				}
				memmove(&texture->internalformats[0], &texture->internalformats[1], 7 * sizeof(GLint));
				texture->internalformats[7] = 0;
			}
			else break;
		} while (1);
	}
	else
	{
		if (This->ext->GLEXT_EXT_direct_state_access) This->ext->glTextureImage2DEXT(texture->id, GL_TEXTURE_2D, level, texture->internalformats[0],
			width, height, 0, texture->format, texture->type, data);
		else
		{
			TextureManager_SetActiveTexture(This, 0);
			TextureManager_SetTexture(This, 0, texture);
			glTexImage2D(GL_TEXTURE_2D, level, texture->internalformats[0], width, height, 0, texture->format, texture->type, data);
		}
	}
}

void TextureManager_DownloadTextureClassic(TextureManager *This, TEXTURE *texture, int level, void *data)
{
	if(This->ext->GLEXT_EXT_direct_state_access) This->ext->glGetTextureImageEXT(texture->id,GL_TEXTURE_2D,level,texture->format,texture->type,data);
	else
	{
		TextureManager_SetActiveTexture(This, 0);
		TextureManager_SetTexture(This, 0,texture);
		glGetTexImage(GL_TEXTURE_2D,level,texture->format,texture->type,data);
	}
}

void TextureManager_SetActiveTexture(TextureManager *This, int level)
{
	if(level != This->texlevel)
	{
		This->texlevel = level;
		This->ext->glActiveTexture(GL_TEXTURE0+level);
	}
}


void TextureManager_SetTexture(TextureManager *This, unsigned int level, TEXTURE *texture)
{
	GLuint texname;
	if (level >= 16) return;
	if(!texture) texname = 0;
	else texname=texture->id;
	if(texname != This->textures[level])
	{
		TextureManager_SetActiveTexture(This, level);
		glBindTexture(GL_TEXTURE_2D,texname);
	}
}

BOOL TextureManager_FixTexture(TextureManager *This, TEXTURE *texture, void *data, DWORD *dirty)
{
	// data should be null to create uninitialized texture or be pointer to top-level
	// buffer to retain texture data
	TEXTURE newtexture;
	GLenum error;
	memcpy(&newtexture, texture, sizeof(TEXTURE));
	if (texture->miplevel > 0) return FALSE;
	if (texture->internalformats[1] == 0) return FALSE;
	glGenTextures(1, &newtexture.id);
	TextureManager_SetActiveTexture(This, 0);
	if (data)
	{
		TextureManager_SetTexture(This, 0, texture);
		glGetTexImage(GL_TEXTURE_2D, 0, texture->format, texture->type, data);
		if (dirty) *dirty |= 2;
	}
	TextureManager_SetTexture(This, 0, &newtexture);
	do
	{
		memmove(&newtexture.internalformats[0], &newtexture.internalformats[1], 7 * sizeof(GLint));
		newtexture.internalformats[7] = 0;
		ClearError();
		glTexImage2D(GL_TEXTURE_2D, 0, newtexture.internalformats[0], newtexture.width, newtexture.height,
			0, newtexture.format, newtexture.type, NULL);
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			if (newtexture.internalformats[1] == 0)
			{
				FIXME("Failed to repair texture, cannot find internal format");
				break;
			}
		}
		else break;
	} while (1);
	TextureManager__DeleteTexture(This, texture);
	memcpy(texture, &newtexture, sizeof(TEXTURE));
	return TRUE;
}