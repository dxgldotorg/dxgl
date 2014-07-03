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

#ifndef _SHADERGEN_H
#define _SHADERGEN_H

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
	int texcoords[8];
} GenShader;

#define D3DTOP_DXGL_DECALMASK 0x101;
#define D3DTOP_DXGL_MODULATEMASK 0x102;

struct ShaderGen2D;

class ShaderGen3D
{
public:
	ShaderGen3D(glExtensions *glext, ShaderManager *shaderman);
	~ShaderGen3D();
	void ClearShaders();
	void SetShader(__int64 id, TEXTURESTAGE *texstate, int *texcoords, int type, ShaderGen2D *gen2d);
	GLuint GetProgram();
	void ZeroShaderArray();
	void CreateShader(int index, __int64 id, TEXTURESTAGE *texstate, int *texcoords);
	GenShader *genshaders;
	int current_genshader;
private:
	__int64 current_shader;
	__int64 current_texid[8];
	int current_shadertype;
	int shadercount;
	int maxshaders;
	int genindex;
	GLuint current_prog;
	glExtensions *ext;
	ShaderManager *shaders;
};

#endif