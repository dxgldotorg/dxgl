// DXGL
// Copyright (C) 2013-2021 William Feely

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
#ifndef _SHADERGEN2D_H
#define _SHADERGEN2D_H

#ifdef __cplusplus
extern "C" {
#endif

struct ShaderManager;

typedef struct _GENSHADER2D
{
	GLint vs;
	GLint fs;
	STRING vsrc;
	STRING fsrc;
	GLint prog;
	GLint attribs[6];
	GLint uniforms[16];
} _GENSHADER2D;

typedef struct GenShader2D
{
	_GENSHADER2D shader;
	__int64 id;
} GenShader2D;

typedef struct ShaderGen2D
{
	GenShader2D *genshaders2D;
	int shadercount;
	int maxshaders;
	int genindex;
	glExtensions *ext;
	ShaderManager *shaders;
} ShaderGen2D;

extern const DWORD valid_rop_codes[256];
extern const DWORD rop_texture_usage[256];
extern const DWORD supported_rops[8];
extern const DWORD supported_rops_gl2[8];

__int64 PackROPBits(DWORD rop, __int64 flags);

void ShaderGen2D_Init(ShaderGen2D *gen, glExtensions *ext, ShaderManager *shaderman);
void ShaderGen2D_Delete(ShaderGen2D *gen);
void ShaderGen2D_CreateShader2D(ShaderGen2D *gen, int index, __int64 id);

#ifdef __cplusplus
}
#endif

#endif //_SHADERGEN2D_H
