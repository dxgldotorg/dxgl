// DXGL
// Copyright (C) 2011-2012 William Feely

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
#include "texture.h"
#include "glutil.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include <string>
using namespace std;
#include "shaders.h"
#include "shadergen.h"

const char frag_Color[] = "\
#version 110\n\
void main() \n\
{ \n\
    gl_FragColor = gl_Color; \n\
} ";

const char frag_Texture[] = "\
#version 110\n\
uniform sampler2D tex0;\n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( tex0, gl_TexCoord[0].st ); \n\
} ";

const char frag_Pal256[] =  "\
#version 110\n\
uniform sampler2D pal; \n\
uniform sampler2D tex0; \n\
void main() \n\
{ \n\
	vec4 myindex = texture2D(tex0, gl_TexCoord[0].xy); \n\
	vec4 texel = texture2D(pal, myindex.xy); \n\
	gl_FragColor = texel; \n\
} ";

const char frag_ColorKey[] = "\
#version 110\n\
uniform sampler2D tex0;\n\
uniform ivec3 ckey;\n\
void main (void)\n\
{\n\
 vec4 value = texture2D(tex0, vec2(gl_TexCoord[0]));\n\
 ivec3 comp = ivec3(texture2D(tex0, vec2(gl_TexCoord[0]))*255.5);\n\
 if (comp == ckey)\n\
  discard;\n\
 gl_FragColor = value;\n\
} ";

const char frag_ColorKeyMask[] = "\
#version 110\n\
uniform sampler2D tex0;\n\
uniform ivec4 ckey;\n\
void main (void)\n\
{\n\
 vec4 value = texture2D(tex0, vec2(gl_TexCoord[0]));\n\
 ivec4 comp = ivec4(texture2D(tex0, vec2(gl_TexCoord[0]))*255.5);\n\
 if (comp == ckey)\n\
  gl_FragColor[0] = 1.0;\n\
 else gl_FragColor[0] = 0.0;\n\
} ";

const char frag_2ColorKey[] = "\
#version 110\n\
uniform sampler2D tex0;\n\
uniform sampler2D tex1;\n\
uniform ivec4 ckey;\n\
void main (void)\n\
{\n\
 vec4 value = texture2D(tex0, vec2(gl_TexCoord[0]));\n\
 ivec4 comp = ivec4(texture2D(tex0, vec2(gl_TexCoord[0]))*255.5);\n\
 if (comp == ckey)\n\
  discard;\n\
 ivec4 comp2 = ivec4(texture2D(tex1,vec2(gl_TexCoord[1]))*255.5);\n\
 if(comp2[0] == 0)\n\
  discard;\n\
 gl_FragColor = value;\n\
} ";

const char vert_ortho[] = "\
#version 110\n\
uniform vec4 view;\n\
attribute vec2 xy;\n\
attribute vec3 rgb;\n\
attribute vec2 st;\n\
void main()\n\
{\n\
	vec4 xyzw = vec4(xy[0],xy[1],0,1);\n\
	vec4 rgba = vec4(rgb[0],rgb[1],rgb[2],1);\n\
	vec4 strq = vec4(st[0],st[1],0,1);\n\
	mat4 proj = mat4(\n\
    vec4(2.0 / (view[1] - view[0]), 0, 0, 0),\n\
    vec4(0, 2.0 / (view[2] - view[3]), 0, 0),\n\
    vec4(0, 0, -2.0, 0),\n\
    vec4(-(view[1] + view[0]) / (view[1] - view[0]),\n\
 -(view[2] + view[3]) / (view[2] - view[3]), -1 , 1));\n\
	gl_Position    = proj * xyzw;\n\
	gl_FrontColor  = rgba;\n\
	gl_TexCoord[0] = strq;\n\
} ";



// Use EXACTLY one line per entry.  Don't change layout of the list.
const int SHADER_START = __LINE__;
SHADER shaders[] = 
{
	{0,0,	vert_ortho,			frag_Color,			0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_Texture,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_Pal256,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_ColorKey,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_ColorKeyMask,	0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_2ColorKey,		0,-1,-1,-1}
};
const int SHADER_END = __LINE__ - 4;
const int NumberOfShaders = SHADER_END - SHADER_START;

void CompileShaders()
{
	ZeroShaderArray();
	const GLchar *src;
	GLint srclen;
	for(int i = 0; i < NumberOfShaders; i++)
	{
		shaders[i].prog = glCreateProgram();
		if(shaders[i].vsrc)
		{
			shaders[i].vs = glCreateShader(GL_VERTEX_SHADER);
			src = shaders[i].vsrc;
			srclen = strlen(shaders[i].vsrc);
			glShaderSource(shaders[i].vs,1,&src,&srclen);
			glCompileShader(shaders[i].vs);
			glAttachShader(shaders[i].prog,shaders[i].vs);
		}
		if(shaders[i].fsrc)
		{
			shaders[i].fs = glCreateShader(GL_FRAGMENT_SHADER);
			src = shaders[i].fsrc;
			srclen = strlen(shaders[i].fsrc);
			glShaderSource(shaders[i].fs,1,&src,&srclen);
			glCompileShader(shaders[i].fs);
			glAttachShader(shaders[i].prog,shaders[i].fs);
		}
		glLinkProgram(shaders[i].prog);
		shaders[i].pos = glGetAttribLocation(shaders[i].prog,"xy");
		shaders[i].rgb = glGetAttribLocation(shaders[i].prog,"rgb");
		shaders[i].texcoord = glGetAttribLocation(shaders[i].prog,"st");
		shaders[i].tex0 = glGetUniformLocation(shaders[i].prog,"tex0");
		shaders[i].tex1 = glGetUniformLocation(shaders[i].prog,"tex1");
		shaders[i].ckey = glGetUniformLocation(shaders[i].prog,"ckey");
		shaders[i].pal = glGetUniformLocation(shaders[i].prog,"pal");
		shaders[i].view = glGetUniformLocation(shaders[i].prog,"view");
	}
}

void DeleteShaders()
{
	glUseProgram(0);
	for(int i = 0; i < NumberOfShaders; i++)
	{
		if(shaders[i].prog)
		{
			glDeleteProgram(shaders[i].prog);
			shaders[i].prog = 0;
		}
		if(shaders[i].vs)
		{
			glDeleteShader(shaders[i].vs);
			shaders[i].vs = 0;
		}
		if(shaders[i].fs)
		{
			glDeleteShader(shaders[i].fs);
			shaders[i].fs = 0;
		}
	}
	ClearShaders();
}