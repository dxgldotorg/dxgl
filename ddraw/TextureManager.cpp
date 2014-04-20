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
#include "timer.h"
#include "glRenderer.h"

// Use EXACTLY one line per entry.  Don't change layout of the list.
const int START_TEXFORMATS = __LINE__;
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
const int END_TEXFORMATS = __LINE__ - 4;
const int numtexformats = END_TEXFORMATS - START_TEXFORMATS;

TextureManager::TextureManager(glExtensions *glext)
{
	ext = glext;
	texlevel = 0;
	ZeroMemory(textures, 16 * sizeof(GLuint));
}

void TextureManager::_CreateTexture(TEXTURE *texture, int width, int height)
{
	CreateTextureClassic(texture, width, height);
}
void TextureManager::_DeleteTexture(TEXTURE *texture)
{
	DeleteTexture(texture);
}
void TextureManager::_UploadTexture(TEXTURE *texture, int level, const void *data, int width, int height)
{
	UploadTextureClassic(texture, level, data, width, height);
}
void TextureManager::_DownloadTexture(TEXTURE *texture, int level, void *data)
{
	DownloadTextureClassic(texture, level, data);
}

void TextureManager::InitSamplers()
{
	if(ext->GLEXT_ARB_sampler_objects)
	{
		memset(samplers,0,8*sizeof(SAMPLER));
		for(int i = 0; i < 8; i++)
		{
			ext->glGenSamplers(1,&samplers[i].id);
			ext->glBindSampler(i,samplers[i].id);
			ext->glSamplerParameteri(samplers[i].id,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
			ext->glSamplerParameteri(samplers[i].id,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
			ext->glSamplerParameteri(samplers[i].id,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			ext->glSamplerParameteri(samplers[i].id,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		}
	}
}
void TextureManager::DeleteSamplers()
{
	if(ext->GLEXT_ARB_sampler_objects)
	{
		for(int i = 0; i < 8; i++)
		{
			ext->glBindSampler(i,0);
			ext->glDeleteSamplers(1,&samplers[i].id);
			samplers[i].id = 0;
		}
	}
}

void TextureManager::CreateTextureClassic(TEXTURE *texture, int width, int height)
{
	int texformat = -1;
	texture->pixelformat.dwSize = sizeof(DDPIXELFORMAT);
	for(int i = 0; i < numtexformats; i++)
	{
		if(!memcmp(&texformats[i],&texture->pixelformat,sizeof(DDPIXELFORMAT)))
		{
			texformat = i;
			break;
		}
	}
	switch(texformat)
	{
	case -1:
	case 0: // 8-bit palette
		if(ext->glver_major >= 3)
		{
			texture->internalformat = GL_R8;
			texture->format = GL_RED;
		}
		else
		{
			texture->internalformat = GL_RGBA8;
			texture->format = GL_LUMINANCE;
		}
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 1: // 8-bit RGB332
		texture->internalformat = GL_R3_G3_B2;
		texture->format = GL_RGB;
		texture->type = GL_UNSIGNED_BYTE_3_3_2;
		break;
	case 2: // 16-bit RGB555
		texture->internalformat = GL_RGB5_A1;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case 3: // 16-bit RGB565
		/*if(GLEXT_ARB_ES2_compatibility) texture->internalformat = GL_RGB565;
		else */texture->internalformat = GL_RGBA8;
		texture->format = GL_RGB;
		texture->type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case 4: // 24-bit RGB888
		texture->internalformat = GL_RGB8;
		texture->format = GL_BGR;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 5: // 32-bit RGB888
		texture->internalformat = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 6: // 32-bit BGR888
		texture->internalformat = GL_RGBA8;
		texture->format = GL_RGBA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 7: // 16-bit RGBA8332
		FIXME("Unusual texture format RGBA8332 not supported");
		break;
	case 8: // 16-bit RGBA4444
		texture->internalformat = GL_RGBA4;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		break;
	case 9: // 16-bit RGBA1555
		texture->internalformat = GL_RGB5_A1;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		break;
	case 10: // 32-bit RGBA8888
		texture->internalformat = GL_RGBA8;
		texture->format = GL_BGRA;
		texture->type = GL_UNSIGNED_INT_8_8_8_8_REV;
		break;
	case 11: // 8-bit Luminance
		texture->internalformat = GL_LUMINANCE8;
		texture->format = GL_LUMINANCE;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 12: // 8-bit Alpha
		texture->internalformat = GL_ALPHA8;
		texture->format = GL_ALPHA;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 13: // 16-bit Luminance Alpha
		texture->internalformat = GL_LUMINANCE8_ALPHA8;
		texture->format = GL_LUMINANCE_ALPHA;
		texture->type = GL_UNSIGNED_BYTE;
		break;
	case 14: // 16-bit Z buffer
		texture->internalformat = GL_DEPTH_COMPONENT16;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_SHORT;
		break;
	case 15: // 24-bit Z buffer
		texture->internalformat = GL_DEPTH_COMPONENT24;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 16: // 32/24 bit Z buffer
		texture->internalformat = GL_DEPTH_COMPONENT24;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 17: // 32-bit Z buffer
		texture->internalformat = GL_DEPTH_COMPONENT32;
		texture->format = GL_DEPTH_COMPONENT;
		texture->type = GL_UNSIGNED_INT;
		break;
	case 18: // 32-bit Z/Stencil buffer, depth LSB
		texture->internalformat = GL_DEPTH24_STENCIL8;
		texture->format = GL_DEPTH_STENCIL;
		texture->type = GL_UNSIGNED_INT_24_8;
		break;
	case 19: // 32-bit Z/Stencil buffer, depth MSB
		texture->internalformat = GL_DEPTH24_STENCIL8;
		texture->format = GL_DEPTH_STENCIL;
		texture->type = GL_UNSIGNED_INT_24_8;
		break;
	}
	texture->width = width;
	texture->height = height;
	glGenTextures(1,&texture->id);
	SetTexture(0,texture);
	glTexImage2D(GL_TEXTURE_2D,0,texture->internalformat,texture->width,texture->height,0,texture->format,texture->type,NULL);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,texture->minfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,texture->magfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,texture->wraps);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,texture->wrapt);
}

void TextureManager::DeleteTexture(TEXTURE *texture)
{
	glDeleteTextures(1,&texture->id);
	texture->bordercolor = texture->format = texture->internalformat =
		texture->type = texture->width = texture->height = texture->magfilter =
		texture->minfilter = texture->miplevel = texture->wraps = texture->wrapt =
		texture->pbo = texture->id = 0;
}

void TextureManager::UploadTextureClassic(TEXTURE *texture, int level, const void *data, int width, int height)
{
	texture->width = width;
	texture->height = height;
	if(ext->GLEXT_EXT_direct_state_access) ext->glTextureImage2DEXT(texture->id,GL_TEXTURE_2D,level,texture->internalformat,
		width,height,0,texture->format,texture->type,data);
	else
	{
		SetActiveTexture(0);
		SetTexture(0,texture);
		glTexImage2D(GL_TEXTURE_2D,level,texture->internalformat,width,height,0,texture->format,texture->type,data);
	}
}

void TextureManager::DownloadTextureClassic(TEXTURE *texture, int level, void *data)
{
	if(ext->GLEXT_EXT_direct_state_access) ext->glGetTextureImageEXT(texture->id,GL_TEXTURE_2D,level,texture->format,texture->type,data);
	else
	{
		SetActiveTexture(0);
		SetTexture(0,texture);
		glGetTexImage(GL_TEXTURE_2D,level,texture->format,texture->type,data);
	}
}

void TextureManager::SetActiveTexture(int level)
{
	if(level != texlevel)
	{
		texlevel = level;
		ext->glActiveTexture(GL_TEXTURE0+level);
	}
}


void TextureManager::SetTexture(unsigned int level, TEXTURE *texture)
{
	if(level >= 16) return;
	GLuint texname;
	if(!texture) texname = 0;
	else texname=texture->id;
	if(texname != textures[level])
	{
		SetActiveTexture(level);
		glBindTexture(GL_TEXTURE_2D,texname);
	}
}



/*  old code
			if(ddsd.ddpfPixelFormat.dwFlags & DDPF_RGB)
			{
				switch(ddsd.ddpfPixelFormat.dwRGBBitCount)
				{
				case 8:
					if(ddsd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
					{
						texformat = GL_LUMINANCE;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_LUMINANCE8;
						else texformat3 = GL_RGBA8;
						if(!palettein) palette = new glDirectDrawPalette(DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_PRIMARYSURFACE,NULL,NULL);
						bitmapinfo->bmiHeader.biBitCount = 8;
					}
					else
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_BYTE_3_3_2;
						if(dxglcfg.texformat) texformat3 = GL_R3_G3_B2;
						else texformat3 = GL_RGBA8;
					}
					ddsd.ddpfPixelFormat.dwRBitMask = 0;
					ddsd.ddpfPixelFormat.dwGBitMask = 0;
					ddsd.ddpfPixelFormat.dwBBitMask = 0;
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth);
					break;
				case 16:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0x7C00) && (ddsd.ddpfPixelFormat.dwGBitMask == 0x3E0)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0x1F))
					{
						texformat = GL_BGRA;
						texformat2 = GL_UNSIGNED_SHORT_1_5_5_5_REV;
						if(dxglcfg.texformat) texformat3 = GL_RGB5_A1;
						else texformat3 = GL_RGBA8;
					}
					else // fixme:  support more formats
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_SHORT_5_6_5;
						if(dxglcfg.texformat) texformat3 = GL_RGB;
						else texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
					break;
				case 24:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000) && (ddsd.ddpfPixelFormat.dwGBitMask == 0xFF00)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0xFF))
					{
						texformat = GL_BGR;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_RGB8;
						else texformat3 = GL_RGBA8;
					}
					else // fixme:  support more formats
					{
						texformat = GL_RGB;
						texformat2 = GL_UNSIGNED_BYTE;
						if(dxglcfg.texformat) texformat3 = GL_RGB8;
						else texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*3);
					break;
				case 32:
				default:
					if((ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000) && (ddsd.ddpfPixelFormat.dwGBitMask == 0xFF00)
						&& (ddsd.ddpfPixelFormat.dwBBitMask == 0xFF))
					{
						texformat = GL_BGRA;
						texformat2 = GL_UNSIGNED_INT_8_8_8_8_REV;
						texformat3 = GL_RGBA8;
					}
					else // fixme: support more formats
					{
						texformat = GL_RGBA;
						texformat2 = GL_UNSIGNED_BYTE;
						texformat3 = GL_RGBA8;
					}
					ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*4);
				}
			}
			else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)
			{
				switch(ddsd.ddpfPixelFormat.dwZBufferBitDepth)
				{
				case 16:
				default:
					texformat = GL_DEPTH_COMPONENT;
					texformat2 = GL_UNSIGNED_BYTE;
					texformat3 = GL_DEPTH_COMPONENT16;
					break;
				case 24:
					texformat = GL_DEPTH_COMPONENT;
					texformat2 = GL_UNSIGNED_BYTE;
					texformat3 = GL_DEPTH_COMPONENT24;
					break;
				case 32:
					if((ddsd.ddpfPixelFormat.dwRGBZBitMask == 0x00ffffff) &&
						!(ddsd.ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER))
					{
						texformat = GL_DEPTH_COMPONENT;
						texformat2 = GL_UNSIGNED_INT;
						texformat3 = GL_DEPTH_COMPONENT24;
						break;
					}
					else if(ddsd.ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER)
					{
						texformat = GL_DEPTH_STENCIL;
						texformat2 = GL_UNSIGNED_INT_24_8;
						texformat3 = GL_DEPTH24_STENCIL8;
						hasstencil = true;
						break;
					}
					else
					{
						texformat = GL_DEPTH_COMPONENT;
						texformat2 = GL_UNSIGNED_INT;
						texformat3 = GL_DEPTH_COMPONENT32;
						break;
					}
				}
			}
		}

*/

/*
		if(!(ddsd.dwFlags & DDSD_PIXELFORMAT))
		{
			ddsd.ddpfPixelFormat.dwRGBBitCount = ddInterface->GetBPP();
			switch(ddInterface->GetBPP())
			{
			case 8:
				texformat = GL_LUMINANCE;
				texformat2 = GL_UNSIGNED_BYTE;
				if(dxglcfg.texformat) texformat3 = GL_LUMINANCE8;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
				ddsd.ddpfPixelFormat.dwRBitMask = 0;
				ddsd.ddpfPixelFormat.dwGBitMask = 0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth);
				break;
			case 15:
				texformat = GL_BGRA;
				texformat2 = GL_UNSIGNED_SHORT_1_5_5_5_REV;
				if(dxglcfg.texformat) texformat3 = GL_RGB5_A1;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0x7C00;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x3E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				ddsd.ddpfPixelFormat.dwRGBBitCount = 16;
				break;
			case 16:
				texformat = GL_RGB;
				texformat2 = GL_UNSIGNED_SHORT_5_6_5;
				if(dxglcfg.texformat) texformat3 = GL_RGB;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xF800;
				ddsd.ddpfPixelFormat.dwGBitMask = 0x7E0;
				ddsd.ddpfPixelFormat.dwBBitMask = 0x1F;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*2);
				break;
			case 24:
				texformat = GL_BGR;
				texformat2 = GL_UNSIGNED_BYTE;
				if(dxglcfg.texformat) texformat3 = GL_RGB8;
				else texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*3);
				break;
			case 32:
				texformat = GL_BGRA;
				texformat2 = GL_UNSIGNED_BYTE;
				texformat3 = GL_RGBA8;
				ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
				ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
				ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
				ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
				ddsd.lPitch = NextMultipleOfWord(ddsd.dwWidth*4);
				break;
			default:
				*error = DDERR_INVALIDPIXELFORMAT;
				return;
			}
		}

*/