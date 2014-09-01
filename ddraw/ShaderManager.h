// DXGL
// Copyright (C) 2011-2014 William Feely

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
#ifndef __SHADERS_H
#define __SHADERS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	GLint vs;
	GLint fs;
	const char *vsrc;
	const char *fsrc;
	GLint prog;
	GLint pos;
	GLint texcoord;
	GLint tex0;
	GLint tex1;
	GLint ckey;
	GLint pal;
	GLint view;
} SHADER;

extern const SHADER shader_template[];

#define PROG_TEXTURE 0
#define PROG_PAL256 1
#define PROG_CKEY 2
#define PROG_CKEYMASK 3
#define PROG_2CKEY 4
#define PROG_CLIPSTENCIL 5

struct TEXTURESTAGE;
struct ShaderGen3D;

typedef struct ShaderManager
{
	SHADER *shaders;
	ShaderGen3D *gen3d;
	ShaderGen2D *gen2d;
	glExtensions *ext;
} ShaderManager;

void ShaderManager_Init(glExtensions *glext, ShaderManager *shaderman);
void ShaderManager_Delete(ShaderManager *This);
void ShaderManager_SetShader(ShaderManager *This, __int64 id, TEXTURESTAGE *texstate, int *texcoords, int type);

#ifdef __cplusplus
}
#endif

#endif //__SHADERS_H