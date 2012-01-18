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

struct GenShader
{
	SHADER shader;
	__int64 id;
};
GenShader genshaders[256];
static __int64 current_shader = 0;
static int shadercount = 0;
static bool initialized = false;
static bool isbuiltin = true;

void SetShader(__int64 id, bool builtin)
{
	if(builtin)
	{
		glUseProgram(shaders[id].prog);
		current_shader = shaders[id].prog;
	}
	else
	{
	}
}

GLuint GetProgram()
{
	if(isbuiltin) return current_shader & 0xFFFFFFFF;
	else
	{
		return 0;
	}
}

#define REVISION 1
static const char header[] =
	"//REV" STR(REVISION) "\n\
#version 110\n";
static const char vertexshader[] = "//Vertex Shader\n";
static const char fragshader[] = "//Fragment Shader\n";
static const char idheader[] = "//ID: 0x";
static const char linefeed[] = "\n";
static const char mainstart[] = "void main()\n{\n";
static const char mainend[] = "} ";
static const char attr_xy[] = "attribute vec2 xy;\n";
static const char conv_xy[] = "vec4 xyzw = vec4(xy[0],xy[1],0,1);\n";
static const char attr_xyz[] = "attribute vec3 xyz;\n";
static const char conv_xyz[] = "vec4 xyzw = vec4(xyz[0],xyz[1],xyz[2],1);\n";
static const char attr_xyzw[] = "attribute vec4 xyzw;\n";
static const char attr_rgb[] = "attrib vec3 rgb;\n";
static const char conv_rgb[] = "vec4 rgba = vec4(rgb[0],rgb[1],rgb[2],1);\n";
static const char attr_rgba[] = "attrib vec4 rgba;\n";
static const char attr_s[] = "attrib float sX;\n";
static const char conv_s[] = "vec4 strqX = vec4(sX,0,0,1);\n";
static const char attr_st[] = "attrib vec2 stX;\n";
static const char conv_st[] = "vec4 strqX = vec4(stX[0],stX[1],0,1);\n";
static const char attr_str[] = "attrib vec3 strX;\n";
static const char conv_str[] = "vec4 strqX = vec4(strX[0],strX[1],strX[2],1);\n";
static const char attr_strq[] = "attrib vec4 strqX;\n";