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

#include "common.h"
#include "shadergen.h"
#include "shaders.h"

SHADER genshaders[256];
static __int64 current_shader = 0;
static int shadercount = 0;
static bool initialized = false;

void SetShader(__int64 id, bool builtin)
{
	if(builtin)
		{
			glUseProgram(shaders[id].prog);
			current_shader = shaders[id].prog;
		}
}

__int64 GetProgram()
{
	return current_shader;
}
