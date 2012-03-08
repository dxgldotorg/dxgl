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
#include <string>
using namespace std;

typedef struct
{
	GLint vs;
	GLint fs;
	string *vsrc;
	string *fsrc;
	GLint prog;
} _GENSHADER;

struct GenShader
{
	_GENSHADER shader;
	__int64 id;
	__int64 texids[8];
};
GenShader genshaders[256];
static __int64 current_shader = 0;
static __int64 current_texid[8];
static int shadercount = 0;
static int genindex = 0;
static bool initialized = false;
static bool isbuiltin = true;
GLuint current_prog;

/* Bits in Shader ID:
Bits 0-1 - Shading mode:  00=flat 01=gouraud 11=phong 10=flat per-pixel
Bit 2 - Alpha test enable
Bits 3-5 - Alpha test function:
000=never  001=less  010=equal  011=lessequal
100=greater  101=notequal  110=lessequal  111=always
Bits 6-7 - Table fog:
00 = none  01=exp  10=exp2  11=linear
Bits 8-9 - Vertex fog: same as table
Bit 10 - Range based fog
Bit 11 - Specular highlights
Bit 12 - Stippled alpha
Bit 13 - Color key transparency
Bit 14-17 - Z bias
Bits 18-20 - Number of lights
Bit 21 - Camera relative specular highlights
Bit 22 - Alpha blended color key
Bits 23-24 - Diffuse material source
Bits 25-26 - Specular material source
Bits 27-28 - Ambient material source
Bits 29-30 - Emissive material source
Bits 31-33 - Number of textures
Bit 34 - Use transformed vertices
Bit 35 - Use diffuse color
Bit 36 - Use specular color
Bit 37 - Enable normals
Bits 38-45 - Light types
Bits 46-48 - Number of blending weights
*/

/* Bits in Texture Stage ID:
Bits 0-4: Texture color operation
Bits 5-10: Texture color argument 1
Bits 11-16: Texture color argument 2
Bits 17-20: Texture alpha operation
Bits 21-26: Texture alpha argument 1
Bits 27-32: Texture alpha argument 2
Bits 33-35: Texture coordinate index
Bits 36-37: Texture coordinate flags
Bits 38-39: U Texture address
Bits 40-41: V Texture address
Bits 42-44: Texture magnification filter
Bits 45-46: Texture minification filter
Bit 47: Enable texture coordinate transform
Bits 48-49: Number of texcoord dimensions
Bit 50: Projected texcoord
Bits 51-52: Texutre coordinate format:
00=2dim  01=3dim 10=4dim 11=1dim
*/
void ZeroShaderArray()
{
	ZeroMemory(genshaders,256*sizeof(GenShader));
	current_shader = 0;
	isbuiltin = true;
}

void ClearShaders()
{
	for(int i = 0; i < shadercount; i++)
	{
		genshaders[i].id = 0;
		ZeroMemory(genshaders[i].texids,8*sizeof(__int64));
		if(genshaders[i].shader.prog) glDeleteProgram(genshaders[i].shader.prog);
		if(genshaders[i].shader.fs) glDeleteShader(genshaders[i].shader.fs);
		if(genshaders[i].shader.vs) glDeleteShader(genshaders[i].shader.vs);
		if(genshaders[i].shader.fsrc) delete genshaders[i].shader.fsrc;
		if(genshaders[i].shader.vsrc) delete genshaders[i].shader.vsrc;
		ZeroMemory(&genshaders[i].shader,sizeof(_GENSHADER));
	}
	shadercount = 0;
	genindex = 0;
}

void SetShader(__int64 id, TexState *texstate, bool builtin)
{
	int shaderindex = -1;
	if(builtin)
	{
		if(isbuiltin && (shaders[id].prog == current_shader)) return;
		glUseProgram(shaders[id].prog);
		current_shader = shaders[id].prog;
		isbuiltin=true;
	}
	else
	{
		if(!isbuiltin && (id == current_shader))
		{
			if(!memcmp(current_texid,texstate,8*sizeof(__int64))) return;
		}
		current_shader = id;
		isbuiltin=false;
		for(int i = 0; i < shadercount; i++)
		{
			if(genshaders[i].id == id)
			{
				if(!memcmp(genshaders[i].texids,texstate,8*sizeof(__int64)))
				{
					shaderindex = i;
					break;
				}
			}
		}
		if(shaderindex == -1)
		{
			shadercount++;
			if(shadercount > 256) shadercount = 256;
			if(genshaders[genindex].shader.prog)
			{
				glUseProgram(0);
				glDeleteProgram(genshaders[shaderindex].shader.prog);
				glDeleteShader(genshaders[shaderindex].shader.vs);
				glDeleteShader(genshaders[shaderindex].shader.fs);
				delete genshaders[shaderindex].shader.vsrc;
				delete genshaders[shaderindex].shader.fsrc;
				ZeroMemory(&genshaders[shaderindex],sizeof(GenShader));
			}
			CreateShader(genindex,id,texstate);
			shaderindex = genindex;
			genindex++;
			if(genindex == 256) genindex = 0;
		}
		genshaders[shaderindex].id = id;
		glUseProgram(genshaders[shaderindex].shader.prog);
		current_prog = genshaders[shaderindex].shader.prog;
	}
}

GLuint GetProgram()
{
	if(isbuiltin) return current_shader & 0xFFFFFFFF;
	else return current_prog;
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
// Attributes
static const char attr_xyz[] = "attribute vec3 xyz;\n";
static const char attr_rhw[] = "attribute float rhw;\n";
static const char attr_nxyz[] = "attribute vec3 nxyz;\n";
static const char attr_blend[] = "attribute float blendX;\n";
static const char attr_rgba[] = "attribute vec4 rgbaX;\n";
static const char attr_s[] = "attribute float sX;\n";
static const char conv_s[] = "vec4 strqX = vec4(sX,0,0,1);\n";
static const char attr_st[] = "attribute vec2 stX;\n";
static const char conv_st[] = "vec4 strqX = vec4(stX[0],stX[1],0,1);\n";
static const char attr_str[] = "attribute vec3 strX;\n";
static const char conv_str[] = "vec4 strqX = vec4(strX[0],strX[1],strX[2],1);\n";
static const char attr_strq[] = "attribute vec4 strqX;\n";
// Uniforms
static const char unif_matrices[] = "uniform mat4 world;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
uniform mat4 normalmat;\n";
static const char unif_material[] = "struct Material\n\
{\n\
vec4 diffuse;\n\
vec4 ambient;\n\
vec4 specular;\n\
vec4 emissive;\n\
float power;\n\
};\n\
uniform Material material;\n";
static const char lightstruct[] = "struct Light\n\
{\n\
vec4 diffuse;\n\
vec4 specular;\n\
vec4 ambient;\n\
vec3 position;\n\
vec3 direction;\n\
float range;\n\
float falloff;\n\
float constant;\n\
float linear;\n\
float quad;\n\
float theta;\n\
float phi;\n\
};\n";
static const char unif_light[] = "uniform Light lightX;\n";
static const char unif_ambient[] = "uniform vec4 ambientcolor;\n";
// Variables
static const char var_colors[] = "vec4 diffuse;\n\
vec4 specular;\n\
vec4 ambient;\n";
static const char var_color[] = "vec4 color;\n";
static const char var_xyzw[] = "vec4 xyzw;\n";
// Operations
static const char op_transform[] = "xyzw = vec4(xyz,1);\n\
gl_Position = (projection*(view*world))*xyzw;\n";
static const char op_passthru[] = "gl_Position = xyzw;\n";
static const char op_resetcolor[] = "diffuse = specular = vec4(0.0);\n\
ambient = ambientcolor / 255.0;\n";
static const char op_dirlight[] = "DirLight(lightX);\n";
static const char op_spotlight[] = "SpotLight(lightX);\n";
static const char op_colorout[] = "vec4 color = (material.diffuse * diffuse) + (material.ambient * ambient) + \n\
(material.specular * specular) + material.emissive;\n\
gl_FrontColor = color;\n";
static const char op_colorfragout[] = "gl_FragColor = color;\n";
static const char op_fragpassthru[] = "color = gl_Color;\n";

// Functions
static const char func_dirlight[] = "void DirLight(in Light light)\n\
{\n\
float NdotHV = 0.0;\n\
vec3 N = normalize(vec3(normalmat*vec4(nxyz,1.0)));\n\
vec3 dir = normalize(light.direction);\n\
ambient += light.ambient;\n\
float NdotL = max(dot(N,dir),0.0);\n\
diffuse += light.diffuse*NdotL;\n\
if(NdotL > 0.0)\n\
{\n\
vec3 eye = (-view[3].xyz / view[3].w);\n\
vec3 P = vec3((view*world)*xyzw);\n\
vec3 L = normalize(light.direction.xyz - P);\n\
vec3 V = normalize(eye - P);\n\
NdotHV = max(dot(N,L+V),0.0);\n\
specular += (pow(NdotHV,float(material.power))*light.specular);\n\
ambient += light.ambient;\n\
}\n\
}\n";

void CreateShader(int index, __int64 id, TexState *texstate)
{
	string tmp;
	int i;
	bool hasdir = false;
	bool hasspot = false;
	int count;
	int numlights;
	char idstring[22];
	_snprintf(idstring,21,"%0.16I64X\n",id);
	idstring[21] = 0;
	genshaders[index].shader.vsrc = new string;
	genshaders[index].shader.fsrc = new string;
	// Create vertex shader
	//Header
	string *vsrc = genshaders[index].shader.vsrc;
	vsrc->append(header);
	vsrc->append(vertexshader);
	vsrc->append(idheader);
	vsrc->append(idstring);
	// Attributes
	vsrc->append(attr_xyz);
	if((id>>34)&1) vsrc->append(attr_rhw);
	tmp = attr_rgba;
	if((id>>35)&1)
	{
		tmp.replace(19,1,"0");
		vsrc->append(tmp);
	}
	if((id>>36)&1)
	{
		tmp.replace(19,1,"1");
		vsrc->append(tmp);
	}
	if((id>>37)&1) vsrc->append(attr_nxyz);
	count = (id>>46)&7;
	if(count)
	{
		tmp = attr_blend;
		for(i = 0; i < count; i++)
		{
			tmp.replace(21,1,_itoa(i,idstring,10));
			vsrc->append(tmp);
		}
	}

	// Uniforms
	vsrc->append(unif_matrices); // Material
	vsrc->append(unif_material);
	vsrc->append(unif_ambient);
	numlights = (id>>18)&7;
	if(numlights) // Lighting
	{
		vsrc->append(lightstruct);
		tmp = unif_light;
		for(i = 0; i < numlights; i++)
		{
			tmp.replace(19,1,_itoa(i,idstring,10));
			vsrc->append(tmp);
		}
	}
	
	// Variables
	vsrc->append(var_colors);
	if(!((id>>34)&1)) vsrc->append(var_xyzw);

	// Functions
	if(numlights)
	{
		for(i = 0; i < numlights; i++)
		{
			if(id>>(38+i)&1) hasspot = true;
			else hasdir = true;
		}
	}
	if(hasspot) FIXME("Add spot lights");
	if(hasdir) vsrc->append(func_dirlight);
	//Main
	vsrc->append(mainstart);
	if((id>>34)&1) vsrc->append(op_passthru);
	else vsrc->append(op_transform);
	vsrc->append(op_resetcolor);
	if(numlights)
	{
		for(i = 0; i < numlights; i++)
		{
			if(id>>(38+i)&1)
			{
				tmp = op_spotlight;
				tmp.replace(15,1,_itoa(i,idstring,10));
				vsrc->append(tmp);
			}
			else
			{
				tmp = op_dirlight;
				tmp.replace(14,1,_itoa(i,idstring,10));
				vsrc->append(tmp);
			}
		}
	}
	vsrc->append(op_colorout);
	vsrc->append(mainend);
#ifdef _DEBUG
	OutputDebugStringA("Vertex shader:\n");
	OutputDebugStringA(vsrc->c_str());
	OutputDebugStringA("\nCompiling vertex shader:\n");
#endif
	genshaders[index].shader.vs = glCreateShader(GL_VERTEX_SHADER);
	const char *src = vsrc->c_str();
	GLint srclen = strlen(src);
	glShaderSource(genshaders[index].shader.vs,1,&src,&srclen);
	glCompileShader(genshaders[index].shader.vs);
	GLint loglen,result;
	char *infolog = NULL;
	glGetShaderiv(genshaders[index].shader.vs,GL_COMPILE_STATUS,&result);
#ifdef _DEBUG
	if(!result)
	{
		glGetShaderiv(genshaders[index].shader.vs,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		glGetShaderInfoLog(genshaders[index].shader.vs,loglen,&result,infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		free(infolog);
	}
#endif
	// Create fragment shader
	string *fsrc = genshaders[index].shader.fsrc;
	fsrc->append(header);
	fsrc->append(fragshader);
	_snprintf(idstring,21,"%0.16I64X\n",id);
	idstring[21] = 0;
	fsrc->append(idheader);
	fsrc->append(idstring);
	// Attributs
	// Uniforms
	// Variables
	fsrc->append(var_color);
	// Functions
	// Main
	fsrc->append(mainstart);
	fsrc->append(op_fragpassthru);
	fsrc->append(op_colorfragout);
	fsrc->append(mainend);
#ifdef _DEBUG
	OutputDebugStringA("Fragment shader:\n");
	OutputDebugStringA(fsrc->c_str());
	OutputDebugStringA("\nCompiling fragment shader:\n");
#endif
	genshaders[index].shader.fs = glCreateShader(GL_FRAGMENT_SHADER);
	src = fsrc->c_str();
	srclen = strlen(src);
	glShaderSource(genshaders[index].shader.fs,1,&src,&srclen);
	glCompileShader(genshaders[index].shader.fs);
	glGetShaderiv(genshaders[index].shader.fs,GL_COMPILE_STATUS,&result);
#ifdef _DEBUG
	if(!result)
	{
		glGetShaderiv(genshaders[index].shader.fs,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		glGetShaderInfoLog(genshaders[index].shader.fs,loglen,&result,infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		free(infolog);
	}
	OutputDebugStringA("\nLinking program:\n");
#endif
	genshaders[index].shader.prog = glCreateProgram();
	glAttachShader(genshaders[index].shader.prog,genshaders[index].shader.vs);
	glAttachShader(genshaders[index].shader.prog,genshaders[index].shader.fs);
	glLinkProgram(genshaders[index].shader.prog);
	glGetProgramiv(genshaders[index].shader.prog,GL_LINK_STATUS,&result);
#ifdef _DEBUG
	if(!result)
	{
		glGetProgramiv(genshaders[index].shader.prog,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		glGetProgramInfoLog(genshaders[index].shader.prog,loglen,&result,infolog);
		OutputDebugStringA("Program link failed. Error messages:\n");
		OutputDebugStringA(infolog);
		free(infolog);
	}
#endif
}
