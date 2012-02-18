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
	__int64 TEX0;
	__int64 TEX1;
	__int64 TEX2;
	__int64 TEX3;
	__int64 TEX4;
	__int64 TEX5;
	__int64 TEX6;
	__int64 TEX7;
} TexState;

void SetShader(__int64 id, TexState *texstate, bool builtin);
GLuint GetProgram();
void ZeroShaderArray();
void CreateShader(int index, __int64 id, TexState *texstate);

#endif