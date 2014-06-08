// DXGL
// Copyright (C) 2013 William Feely

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

class ShaderManager;

typedef struct
{
	GLint vs;
	GLint fs;
	STRING vsrc;
	STRING fsrc;
	GLint prog;
	GLint attribs[8];
	GLint uniforms[16];
} _GENSHADER2D;

struct GenShader2D
{
	_GENSHADER2D shader;
	DWORD id;
};

typedef struct ShaderGen2D
{
	int current_genshader2D;
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

void ShaderGen2D_Init(ShaderGen2D *gen, glExtensions *ext, ShaderManager *shaderman);
void ShaderGen2D_Delete(ShaderGen2D *gen);
void ShaderGen2D_CreateShader2D(ShaderGen2D *gen, int index, DWORD id);

#endif //_SHADERGEN2D_H