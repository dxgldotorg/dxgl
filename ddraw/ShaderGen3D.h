// DXGL
// Copyright (C) 2012-2016 William Feely

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

#ifndef _SHADERGEN3D_H
#define _SHADERGEN3D_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	GLint vs;
	GLint fs;
	STRING vsrc;
	STRING fsrc;
	GLint prog;
	GLint attribs[42];
	GLint uniforms[256];
} _GENSHADER;

typedef struct
{
	_GENSHADER shader;
	__int64 id;
	__int64 texids[8];
} GenShader;

#define D3DTOP_DXGL_DECALMASK 0x101;
#define D3DTOP_DXGL_MODULATEMASK 0x102;

struct ShaderGen2D;

typedef struct ShaderGen3D
{
	GenShader *genshaders;
	GenShader *current_genshader;
	__int64 current_shader;
	__int64 current_texid[8];
	int current_shadertype;
	int shadercount;
	int maxshaders;
	int genindex;
	GLuint current_prog;
	glExtensions *ext;
	ShaderManager *shaders;
} ShaderGen3D;

void ShaderGen3D_Init(glExtensions *glext, ShaderManager *shaderman, ShaderGen3D *gen);
void ShaderGen3D_Delete(ShaderGen3D *This);
void ShaderGen3D_ClearShaders(ShaderGen3D *This);
void ShaderGen3D_SetShader(ShaderGen3D *This, __int64 id, __int64 *texstate, int type, struct ShaderGen2D *gen2d);
GLuint ShaderGen3D_GetProgram(ShaderGen3D *This);
void ShaderGen3D_ZeroShaderArray(ShaderGen3D *This);
void ShaderGen3D_CreateShader(ShaderGen3D *This, int index, __int64 id, __int64 *texstate);

#ifdef __cplusplus
}
#endif

#endif //_SHADERGEN3D_H
