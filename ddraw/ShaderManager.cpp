// DXGL
// Copyright (C) 2011-2015 William Feely

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
#include "timer.h"
#include "glUtil.h"
#include "glRenderer.h"
#include "string.h"
#include "ShaderManager.h"
#include "ShaderGen3D.h"
#include "ShaderGen2D.h"

extern "C" {

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
	vec2 index = vec2(((myindex.x*(255.0/256.0))+(0.5/256.0)),0.5);\n\
	vec4 texel = texture2D(pal, index); \n\
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

const char frag_clipstencil[] = "\
#version 110\n\
void main (void)\n\
{\n\
 gl_FragColor = vec4(1.0,0.0,0.0,0.0);\n\
} ";

const char vert_ortho[] = "\
#version 110\n\
uniform vec4 view;\n\
attribute vec2 xy;\n\
attribute vec2 st;\n\
void main()\n\
{\n\
	vec4 xyzw = vec4(xy[0],xy[1],0,1);\n\
	mat4 proj = mat4(\n\
    vec4(2.0 / (view[1] - view[0]), 0, 0, 0),\n\
    vec4(0, 2.0 / (view[2] - view[3]), 0, 0),\n\
    vec4(0, 0, -2.0, 0),\n\
    vec4(-(view[1] + view[0]) / (view[1] - view[0]),\n\
 -(view[2] + view[3]) / (view[2] - view[3]), -1 , 1));\n\
	gl_Position    = proj * xyzw;\n\
	gl_TexCoord[0] = vec4(st,0.0,1.0);\n\
} ";



// Use EXACTLY one line per entry.  Don't change layout of the list.
const int SHADER_START = __LINE__;
const SHADER shader_template[] = 
{
	{0,0,	vert_ortho,			frag_Texture,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_Pal256,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_ColorKey,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_ColorKeyMask,	0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_2ColorKey,		0,-1,-1,-1},
	{0,0,	vert_ortho,			frag_clipstencil,	0,-1,-1,-1}
};
const int SHADER_END = __LINE__ - 4;
const int NumberOfShaders = SHADER_END - SHADER_START;


void ShaderManager_Init(glExtensions *glext, ShaderManager *shaderman)
{
	shaderman->ext = glext;
	shaderman->shaders = (SHADER*)malloc(sizeof(SHADER)*NumberOfShaders);
	memcpy(shaderman->shaders, shader_template, sizeof(SHADER)*NumberOfShaders);
	const GLchar *src;
	GLint srclen;
	for(int i = 0; i < NumberOfShaders; i++)
	{
		shaderman->shaders[i].prog = shaderman->ext->glCreateProgram();
		if(shaderman->shaders[i].vsrc)
		{
			shaderman->shaders[i].vs = shaderman->ext->glCreateShader(GL_VERTEX_SHADER);
			src = shaderman->shaders[i].vsrc;
			srclen = strlen(shaderman->shaders[i].vsrc);
			shaderman->ext->glShaderSource(shaderman->shaders[i].vs,1,&src,&srclen);
			shaderman->ext->glCompileShader(shaderman->shaders[i].vs);
			shaderman->ext->glAttachShader(shaderman->shaders[i].prog,shaderman->shaders[i].vs);
		}
		if(shaderman->shaders[i].fsrc)
		{
			shaderman->shaders[i].fs = shaderman->ext->glCreateShader(GL_FRAGMENT_SHADER);
			src = shaderman->shaders[i].fsrc;
			srclen = strlen(shaderman->shaders[i].fsrc);
			shaderman->ext->glShaderSource(shaderman->shaders[i].fs,1,&src,&srclen);
			shaderman->ext->glCompileShader(shaderman->shaders[i].fs);
			shaderman->ext->glAttachShader(shaderman->shaders[i].prog,shaderman->shaders[i].fs);
		}
		shaderman->ext->glLinkProgram(shaderman->shaders[i].prog);
		shaderman->shaders[i].pos = shaderman->ext->glGetAttribLocation(shaderman->shaders[i].prog,"xy");
		shaderman->shaders[i].texcoord = shaderman->ext->glGetAttribLocation(shaderman->shaders[i].prog,"st");
		shaderman->shaders[i].tex0 = shaderman->ext->glGetUniformLocation(shaderman->shaders[i].prog,"tex0");
		shaderman->shaders[i].tex1 = shaderman->ext->glGetUniformLocation(shaderman->shaders[i].prog,"tex1");
		shaderman->shaders[i].ckey = shaderman->ext->glGetUniformLocation(shaderman->shaders[i].prog,"ckey");
		shaderman->shaders[i].pal = shaderman->ext->glGetUniformLocation(shaderman->shaders[i].prog,"pal");
		shaderman->shaders[i].view = shaderman->ext->glGetUniformLocation(shaderman->shaders[i].prog,"view");
	}
	shaderman->gen3d = (ShaderGen3D*)malloc(sizeof(ShaderGen3D));
	ShaderGen3D_Init(shaderman->ext, shaderman, shaderman->gen3d);
	shaderman->gen2d = (ShaderGen2D*)malloc(sizeof(ShaderGen2D));
	ZeroMemory(shaderman->gen2d, sizeof(ShaderGen2D));
	ShaderGen2D_Init(shaderman->gen2d, shaderman->ext, shaderman);
}

void ShaderManager_Delete(ShaderManager *This)
{
	This->ext->glUseProgram(0);
	for(int i = 0; i < NumberOfShaders; i++)
	{
		if(This->shaders[i].prog)
		{
			This->ext->glDeleteProgram(This->shaders[i].prog);
			This->shaders[i].prog = 0;
		}
		if(This->shaders[i].vs)
		{
			This->ext->glDeleteShader(This->shaders[i].vs);
			This->shaders[i].vs = 0;
		}
		if(This->shaders[i].fs)
		{
			This->ext->glDeleteShader(This->shaders[i].fs);
			This->shaders[i].fs = 0;
		}
	}
	free(This->shaders);
	ShaderGen2D_Delete(This->gen2d);
	free(This->gen2d);
	ShaderGen3D_Delete(This->gen3d);
	free(This->gen3d);
}

void ShaderManager_SetShader(ShaderManager *This, __int64 id, __int64 *texstate, int *texcoords, int type)
{
	ShaderGen3D_SetShader(This->gen3d, id, texstate, texcoords, type, This->gen2d);
}

}