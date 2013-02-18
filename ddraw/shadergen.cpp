// DXGL
// Copyright (C) 2012-2013 William Feely

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
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include <string>
using namespace std;
#include "shadergen.h"
#include "shaders.h"

GenShader genshaders[256];
static __int64 current_shader = 0;
static __int64 current_texid[8];
static int shadercount = 0;
static int genindex = 0;
static bool initialized = false;
static bool isbuiltin = true;
GLuint current_prog;
int current_genshader;

/* Bits in Shader ID:
Bits 0-1 - Shading mode:  00=flat 01=gouraud 11=phong 10=flat per-pixel GL/VS/FS
Bit 2 - Alpha test enable  FS
Bits 3-5 - Alpha test function:  FS
000=never  001=less  010=equal  011=lessequal
100=greater  101=notequal  110=greaterequal  111=always
Bits 6-7 - Table fog:  FS
00 = none  01=exp  10=exp2  11=linear
Bits 8-9 - Vertex fog: same as table  VS/FS
Bit 10 - Range based fog  VS/FS
Bit 11 - Specular highlights  VS/FS
Bit 12 - Stippled alpha  FS
Bit 13 - Color key transparency  FS
Bit 14-17 - Z bias  FS
Bits 18-20 - Number of lights  VS/FS
Bit 21 - Camera relative specular highlights  VS/FS
Bit 22 - Alpha blended color key  FS
Bits 23-24 - Diffuse material source  VS
Bits 25-26 - Specular material source  VS
Bits 27-28 - Ambient material source  VS
Bits 29-30 - Emissive material source  VS
Bits 31-33 - Number of textures  VS/FS
Bit 35 - Use diffuse color  VS
Bit 36 - Use specular color  VS
Bit 37 - Enable normals  VS
Bits 38-45 - Directional or point/spot light  VS/FS
Bits 46-48 - Number of blending weights  VS
Bit 49 - Normalize normals  VS
Bit 50 - Use transformed vertices  VS
Bits 51-58 - Point or spot light  VS/FS
Bit 59 - Enable lights  VS/FS
Bit 60 - Use vertex colors  VS
Bit 61 - Enable fog  VS/FS
*/

/* Bits in Texture Stage ID:
Bits 0-4: Texture color operation  FS
Bits 5-10: Texture color argument 1  FS
Bits 11-16: Texture color argument 2  FS
Bits 17-21: Texture alpha operation  FS
Bits 22-27: Texture alpha argument 1  FS
Bits 28-33: Texture alpha argument 2  FS
Bits 34-36: Texture coordinate index  VS
Bits 37-38: Texture coordinate flags  VS
Bits 39-40: U Texture address  GL
Bits 41-42: V Texture address  GL
Bits 43-45: Texture magnification filter  GL/FS?
Bits 46-47: Texture minification filter  GL/FS?
Bits 48-49: Texture mip filter  GL/FS?
Bit 50: Enable texture coordinate transform  VS
Bits 51-52: Number of texcoord dimensions  VS
Bit 53: Projected texcoord  VS
Bits in texcoord ID:
00=2dim  01=3dim 10=4dim 11=1dim
Bits 54-56: Texture coordinate index  VS
Bits 57-58: Texture coordinate flags  VS
Bits in flags:
00=passthru 01=cameraspacenormal
10=cameraspaceposition 11=cameraspacereflectionvector
Bit 59: Texture image enabled
Bit 60: Texture has color key
*/

/**
  * Clears the array of shaders.
  */
void ZeroShaderArray()
{
	ZeroMemory(genshaders,256*sizeof(GenShader));
	current_shader = 0;
	isbuiltin = true;
}

/**
  * Deletes all shader programs in the array.
  */
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
	current_genshader = -1;
	shadercount = 0;
	genindex = 0;
}

/**
  * Sets a shader by render state.  If the shader does not exist, generates it.
  * @param id
  *  64-bit value containing current render states
  * @param texstate
  *  Pointer to the texture stage state array, containing 8 64-bit state values
  * @param texcoords
  *  Pointer to number of texture coordinates in each texture stage
  * @param builtin
  *  If true, the id parameter is an index to a built-in shader for 2D blitting
  */
void SetShader(__int64 id, TEXTURESTAGE *texstate, int *texcoords, bool builtin)
{
	int shaderindex = -1;
	if(builtin)
	{
		if(isbuiltin && (shaders[id].prog == current_shader)) return;
		glUseProgram(shaders[id].prog);
		current_shader = shaders[id].prog;
		isbuiltin=true;
		current_genshader = -1;
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
				bool texidmatch = true;
				for(int j = 0; j < 8; j++)
					if(genshaders[i].texids[j] != texstate[j].shaderid) texidmatch = false;
				if(texidmatch)
				{
					if(!memcmp(genshaders[i].texcoords,texcoords,8*sizeof(int)))
					{
						shaderindex = i;
						break;
					}
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
				glDeleteProgram(genshaders[genindex].shader.prog);
				glDeleteShader(genshaders[genindex].shader.vs);
				glDeleteShader(genshaders[genindex].shader.fs);
				delete genshaders[genindex].shader.vsrc;
				delete genshaders[genindex].shader.fsrc;
				ZeroMemory(&genshaders[genindex],sizeof(GenShader));
			}
			CreateShader(genindex,id,texstate,texcoords);
			shaderindex = genindex;
			genindex++;
			if(genindex >= 256) genindex = 0;
		}
		genshaders[shaderindex].id = id;
		for(int i = 0; i < 8; i++)
			genshaders[shaderindex].texids[i] = texstate[i].shaderid;
		memcpy(genshaders[shaderindex].texcoords,texcoords,8*sizeof(int));
		glUseProgram(genshaders[shaderindex].shader.prog);
		current_prog = genshaders[shaderindex].shader.prog;
		current_genshader = shaderindex;
	}
}

/**
  * Retrieves the GLSL program currently in use
  * @return
  *  Number of the current GLSL program, or if using built-in shaders, the ID of
  *  the shader
  */
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
static const char const_nxyz[] = "const vec3 nxyz = vec3(0,0,0);\n";
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
static const char unif_tex[] = "uniform sampler2D texX;\n";
static const char unif_size[] = "uniform float width;\n\
uniform float height;\n";
static const char unif_alpharef[] = "uniform int alpharef;\n";
static const char unif_key[] = "uniform ivec4 keyX;\n";
static const char unif_world[] = "uniform mat4 matWorld;\n";
// Variables
static const char var_common[] = "vec4 diffuse;\n\
vec4 specular;\n\
vec4 ambient;\n\
vec3 N;\n";
static const char var_color[] = "vec4 color;\n";
static const char var_xyzw[] = "vec4 xyzw;\n";
static const char var_fogfactorvertex[] = "varying float fogfactor;\n";
static const char var_fogfactorpixel[] = "float fogfactor;\n";
// Operations
static const char op_transform[] = "xyzw = vec4(xyz,1.0);\n\
vec4 pos = gl_ModelViewProjectionMatrix*xyzw;\n\
gl_Position = vec4(pos.x,-pos.y,pos.z,pos.w);\n";
static const char op_normalize[] = "N = normalize(gl_NormalMatrix*nxyz);\n";
static const char op_normalpassthru[] = "N = gl_NormalMatrix*nxyz;\n";
static const char op_passthru[] = "gl_Position = vec4(((xyz.x)/(width/2.0))-1.0,((xyz.y)/(height/2.0))-1.0,(xyz.z),1.0);\n";
static const char op_resetcolor[] = "diffuse = specular = vec4(0.0);\n\
ambient = ambientcolor / 255.0;\n";
static const char op_dirlight[] = "DirLight(lightX);\n";
static const char op_pointlight[] = "PointLight(lightX);\n";
static const char op_spotlight[] = "SpotLight(lightX);\n";
static const char op_colorout[] = "gl_FrontColor = (gl_FrontMaterial.diffuse * diffuse) + (gl_FrontMaterial.ambient * ambient)\n\
+ (gl_FrontMaterial.specular * specular) + gl_FrontMaterial.emission;\n\
gl_FrontSecondaryColor = (gl_FrontMaterial.specular * specular);\n";
static const char op_colorvert[] = "gl_FrontColor = rgba0.bgra;\n";
static const char op_color2vert[] = "gl_FrontSecondaryColor = rgba1.bgra;\n";
static const char op_colorwhite[] = "gl_FrontColor = vec4(1.0,1.0,1.0,1.0);\n";
static const char op_colorfragout[] = "gl_FragColor = color;\n";
static const char op_colorfragin[] = "color = gl_Color;\n";
static const char op_colorkey[] = "if(ivec4(texture2DProj(texX,gl_TexCoord[Y])*255.5) == keyZ) discard;\n";
static const char op_texpassthru1[] = "gl_TexCoord[x] = ";
static const char op_texpassthru2s[] = "vec4(sX,0,0,1);\n";
static const char op_texpassthru2st[] = "vec4(stX,0,1);\n";
static const char op_texpassthru2str[] = "vec4(strX,1);\n";
static const char op_texpassthru2strq[] = "strqX;\n";
static const char op_texpassthru2null[] = "vec4(0,0,0,1);\n";
static const char op_fogcoordstandardpixel[] = "float fogcoord = gl_FragCoord.z / gl_FragCoord.w;\n";
static const char op_fogcoordstandard[] = "gl_FogFragCoord = abs(gl_ModelViewMatrix*xyzw).z;\n";
static const char op_fogcoordrange[] = "vec4 eyepos = gl_ModelViewMatrix*xyzw;\n\
vec3 eyepos3 = eyepos.xyz / eyepos.w;\n\
gl_FogFragCoord = sqrt((eyepos3.x * eyepos3.x) + (eyepos3.y * eyepos3.y) + (eyepos3.z * eyepos3.z));\n";
static const char op_foglinear[] = "fogfactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);\n";
static const char op_fogexp[] = "fogfactor = 1.0 / exp(gl_FogFragCoord * gl_Fog.density);\n";
static const char op_fogexp2[] = "fogfactor = 1.0 / exp(gl_FogFragCoord * gl_FogFragCoord *\n\
gl_Fog.density * gl_Fog.density);\n";
static const char op_foglinearpixel[] = "fogfactor = (gl_Fog.end - fogcoord) / (gl_Fog.end - gl_Fog.start);\n";
static const char op_fogexppixel[] = "fogfactor = 1.0 / exp(fogcoord * gl_Fog.density);\n";
static const char op_fogexp2pixel[] = "fogfactor = 1.0 / exp(fogcoord * fogcoord *\n\
gl_Fog.density * gl_Fog.density);\n";
static const char op_fogclamp[] = "fogfactor = clamp(fogfactor,0.0,1.0);\n";
static const char op_fogblend[] = "color = mix(gl_Fog.color,color,fogfactor);\n";
static const char op_fogassign[] = "color = gl_Fog.color;\n";

// Functions
static const char func_dirlight[] = "void DirLight(in Light light)\n\
{\n\
float NdotHV = 0.0;\n\
vec3 dir = normalize(-light.direction);\n\
ambient += light.ambient;\n\
float NdotL = max(dot(N,dir),0.0);\n\
diffuse += light.diffuse*NdotL;\n\
if((NdotL > 0.0) && (gl_FrontMaterial.shininess != 0.0))\n\
{\n\
vec3 eye = vec3(0.0,0.0,1.0);\n\
vec3 P = vec3(gl_ModelViewMatrix*xyzw);\n\
vec3 L = normalize(-light.direction.xyz - P);\n\
vec3 V = normalize(eye - P);\n\
NdotHV = max(dot(N,L+V),0.0);\n\
specular += (pow(NdotHV,float(gl_FrontMaterial.shininess))*light.specular);\n\
ambient += light.ambient;\n\
}\n\
}\n";
static const char func_pointlight[] = "void PointLight(in Light light)\n\
{\n\
ambient += light.ambient;\n\
vec4 pos = matWorld*xyzw;\n\
vec3 pos3 = pos.xyz / pos.w;\n\
vec3 V = light.position - pos3;\n\
float d = length(V);\n\
if(d > light.range) return;\n\
V = normalize(V);\n\
float attenuation = 1.0/(light.constant+(d*light.linear)+((d*d)*light.quad));\n\
float NdotV = max(0.0,dot(N,V));\n\
float NdotHV = max(0.0,dot(N,normalize(V+vec3(0.0,0.0,1.0))));\n\
float pf;\n\
if(NdotV == 0.0) pf = 0.0;\n\
else if(gl_FrontMaterial.shininess > 0.0) pf = pow(NdotHV,gl_FrontMaterial.shininess);\n\
else pf = 0.0;\n\
diffuse += light.diffuse*NdotV*attenuation;\n\
specular += light.specular*pf*attenuation;\n\
}\n";
static const char func_spotlight[] = "void SpotLight(in Light light)\n\
{\n\
vec4 pos = (matWorld*xyzw);\n\
vec3 pos3 = pos.xyz / pos.w;\n\
vec3 V = light.position - pos3;\n\
float d = length(V);\n\
V = normalize(V);\n\
float attenuation = 1.0/(light.constant+(d*light.linear)+((d*d)*light.quad));\n\
float NdotV = max(0.0,dot(N,V));\n\
float NdotHV = max(0.0,dot(N,normalize(V+vec3(0.0,0.0,1.0))));\n\
float pf;\n\
if(NdotV == 0.0) pf = 0.0;\n\
else if(gl_FrontMaterial.shininess > 0.0) pf = pow(NdotHV,gl_FrontMaterial.shininess);\n\
else pf = 0.0;\n\
float spotangle = dot(-V,normalize(light.direction));\n\
if(spotangle < cos(light.phi * (180.0/3.14159265)))\n\
attenuation = 0.0;\n\
diffuse += light.diffuse*NdotV*attenuation;\n\
ambient += light.ambient;\n\
specular += light.specular*pf*attenuation;\n\
}\n";



/**
  * Creates an OpenGL shader program
  * @param index
  *  Index of the shader in the array to generate
  * @param id
  *  64-bit value containing current render states
  * @param texstate
  *  Pointer to the texture stage state array, containing 8 64-bit state values
  * @param texcoords
  *  Pointer to number of texture coordinates in each texture stage
  */
void CreateShader(int index, __int64 id, TEXTURESTAGE *texstate, int *texcoords)
{
	string tmp;
	int i;
	bool hasdir = false;
	bool haspoint = false;
	bool hasspot = false;
	int count;
	int numlights;
	int vertexfog,pixelfog;
	vertexfog = pixelfog = 0;
	if((id>>61)&1)
	{
		vertexfog = (id>>8)&3;
		pixelfog = (id>>6)&3;
	}
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
	if((id>>50)&1) vsrc->append(attr_rhw);
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
	else vsrc->append(const_nxyz);
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
	for(i = 0; i < 8; i++)
	{
		switch(texcoords[i])
		{
		case -1:
			continue;
		case 3:
			tmp = attr_s;
			tmp.replace(16,1,_itoa(i,idstring,10));
			break;
		case 0:
			tmp = attr_st;
			tmp.replace(17,1,_itoa(i,idstring,10));
			break;
		case 1:
			tmp = attr_str;
			tmp.replace(18,1,_itoa(i,idstring,10));
			break;
		case 2:
			tmp = attr_strq;
			tmp.replace(19,1,_itoa(i,idstring,10));
			break;
		}
		vsrc->append(tmp);
	}

	// Uniforms
	vsrc->append(unif_ambient);
	if((id>>50)&1) vsrc->append(unif_size);
	if((id>>59)&1) numlights = (id>>18)&7;
	else numlights = 0;
	if((id>>50)&1) numlights = 0;
	if(numlights) // Lighting
	{
		vsrc->append(lightstruct);
		vsrc->append(unif_world);
		tmp = unif_light;
		for(i = 0; i < numlights; i++)
		{
			tmp.replace(19,1,_itoa(i,idstring,10));
			vsrc->append(tmp);
		}
	}
	
	// Variables
	vsrc->append(var_common);
	if(!((id>>50)&1)) vsrc->append(var_xyzw);
	if(vertexfog && !pixelfog) vsrc->append(var_fogfactorvertex);

	// Functions
	if(numlights)
	{
		for(i = 0; i < numlights; i++)
		{
			if(id>>(38+i)&1)
			{
				if(id>>(51+i)&1) hasspot = true;
				else haspoint = true;
			}
			else hasdir = true;
		}
	}
	bool hasspecular = (id >> 11) & 1;
	if(hasspot) vsrc->append(func_spotlight);
	if(haspoint) vsrc->append(func_pointlight);
	if(hasdir) vsrc->append(func_dirlight);
	//Main
	vsrc->append(mainstart);
	if((id>>50)&1) vsrc->append(op_passthru);
	else vsrc->append(op_transform);
	if((id>>49)&1) vsrc->append(op_normalize);
	else vsrc->append(op_normalpassthru);
	const string colorargs[] = {"gl_FrontMaterial.diffuse","gl_FrontMaterial.ambient","gl_FrontMaterial.specular",
		"gl_FrontMaterial.emission","rgba0.bgra","rgba1.bgra"};
	if(numlights)
	{
		vsrc->append(op_resetcolor);
		for(i = 0; i < numlights; i++)
		{
			if(id>>(38+i)&1)
			{
				if(id>>(51+i)&1)
				{
					tmp = op_spotlight;
					tmp.replace(15,1,_itoa(i,idstring,10));
					vsrc->append(tmp);
				}
				else
				{
					tmp = op_pointlight;
					tmp.replace(16,1,_itoa(i,idstring,10));
					vsrc->append(tmp);
				}
			}
			else
			{
				tmp = op_dirlight;
				tmp.replace(14,1,_itoa(i,idstring,10));
				vsrc->append(tmp);
			}
		}
		if((id>>60)&1)
		{
			bool hascolor1 = false;
			if((id>>35)&1) hascolor1 = true;
			bool hascolor2 = false;
			if((id>>36)&1) hascolor2 = true;
			int matcolor;
			vsrc->append("gl_FrontColor = (");
			matcolor = ((id>>23)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) vsrc->append(colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) vsrc->append(colorargs[5]);
			else vsrc->append(colorargs[0]);
			vsrc->append(" * diffuse) + (");
			matcolor = ((id>>27)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) vsrc->append(colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) vsrc->append(colorargs[5]);
			else vsrc->append(colorargs[1]);
			vsrc->append(" * ambient)\n+ (");
			matcolor = ((id>>25)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) vsrc->append(colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) vsrc->append(colorargs[5]);
			else vsrc->append(colorargs[2]);
			vsrc->append(" * specular) + ");
			matcolor = ((id>>29)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) vsrc->append(colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) vsrc->append(colorargs[5]);
			else vsrc->append(colorargs[3]);
			vsrc->append(";\n");
		}
		else vsrc->append(op_colorout);
	}
	else
	{
		if((id>>35)&1) vsrc->append(op_colorvert);
		else vsrc->append(op_colorwhite);
		if((id>>36)&1) vsrc->append(op_color2vert);
	}
	int texindex;
	for(i = 0; i < 8; i++)
	{
		if((texstate[i].shaderid>>50)&1)
		{
			FIXME("Support texture coordinate transform");
		}
		else
		{
			tmp = op_texpassthru1;
			tmp.replace(12,1,_itoa(i,idstring,10));
			vsrc->append(tmp);
			texindex = (texstate[i].shaderid>>54)&3;
			switch(texcoords[texindex])
			{
			case -1: // No texcoords
				vsrc->append(op_texpassthru2null);
				break;
			case 0: // st
				tmp = op_texpassthru2st;
				tmp.replace(7,1,_itoa(texindex,idstring,10));
				vsrc->append(tmp);
			default:
				break;
			case 1: // str
				tmp = op_texpassthru2str;
				tmp.replace(8,1,_itoa(texindex,idstring,10));
				vsrc->append(tmp);
				break;
			case 2: // strq
				tmp = op_texpassthru2strq;
				tmp.replace(4,1,_itoa(texindex,idstring,10));
				vsrc->append(tmp);
				break;
			case 3: // s
				tmp = op_texpassthru2s;
				tmp.replace(6,1,_itoa(texindex,idstring,10));
				vsrc->append(tmp);
				break;
			}
		}
	}
	if(vertexfog && !pixelfog)
	{
		if((id>>10)&1) vsrc->append(op_fogcoordrange);
		else vsrc->append(op_fogcoordstandard);
		switch(vertexfog)
		{
		case D3DFOG_LINEAR:
			vsrc->append(op_foglinear);
			break;
		case D3DFOG_EXP:
			vsrc->append(op_fogexp);
			break;
		case D3DFOG_EXP2:
			vsrc->append(op_fogexp2);
			break;
		}
		vsrc->append(op_fogclamp);
	}
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
	GLint result;
	char *infolog = NULL;
	glGetShaderiv(genshaders[index].shader.vs,GL_COMPILE_STATUS,&result);
#ifdef _DEBUG
	GLint loglen;
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
	// Uniforms
	for(i = 0; i < 8; i++)
	{
		if((texstate[i].shaderid & 31) == D3DTOP_DISABLE)break;
		tmp = unif_tex;
		tmp.replace(21,1,_itoa(i,idstring,10));
		fsrc->append(tmp);
	}
	if((id>>13)&1)
	{
		for(i = 0; i < 8; i++)
		{
			if((texstate[i].shaderid>>60)&1)
			{
				tmp = unif_key;
				tmp.replace(17,1,_itoa(i,idstring,10));
				fsrc->append(tmp);
			}
		}
	}
	if((id>>2)&1) fsrc->append(unif_alpharef);
	// Variables
	fsrc->append(var_color);
	if(vertexfog && !pixelfog) fsrc->append(var_fogfactorvertex);
	if(pixelfog) fsrc->append(var_fogfactorpixel);
	// Functions
	// Main
	fsrc->append(mainstart);
	fsrc->append(op_colorfragin);
	string arg1,arg2;
	string texarg;
	int args[4];
	bool texfail;
	bool alphadisabled = false;
	const string blendargs[] = {"color","gl_Color","texture2DProj(texX,gl_TexCoord[Y])",
		"","texfactor","gl_SecondaryColor","vec3(1,1,1)","1",".rgb",".a",".aaa"};
	bool usecolorkey = false;
	if((id>>13)&1) usecolorkey = true;
	for(i = 0; i < 8; i++)
	{
		if((texstate[i].shaderid & 31) == D3DTOP_DISABLE)break;
		args[0] = (texstate[i].shaderid>>5)&63;
		args[1] = (texstate[i].shaderid>>11)&63;
		args[2] = (texstate[i].shaderid>>22)&63;
		args[3] = (texstate[i].shaderid>>28)&63;
		// Color key
		if(usecolorkey)
		{
			if((texstate[i].shaderid>>60)&1)
			{
				arg1.assign(op_colorkey);
				arg1.replace(26,1,_itoa(i,idstring,10));
				arg1.replace(40,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
				arg1.replace(57,1,_itoa(i,idstring,10));
				fsrc->append(arg1);
			}
		}
		// Color stage
		texfail = false;
		switch(args[0]&7) //arg1
		{
			case D3DTA_CURRENT:
			default:
				arg1 = blendargs[0];
				break;
			case D3DTA_DIFFUSE:
				arg1 = blendargs[1];
				break;
			case D3DTA_TEXTURE:
				if((texstate[i].shaderid >> 59)&1)
				{
					arg1 = blendargs[2];
					arg1.replace(17,1,_itoa(i,idstring,10));
					arg1.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				arg1 = blendargs[4];
				break;
			case D3DTA_SPECULAR:
				arg1 = blendargs[5];
				break;
		}
		if(args[0] & D3DTA_COMPLEMENT) arg1 = "(1.0 - " + arg1 + ")";
		if(args[0] & D3DTA_ALPHAREPLICATE) arg1.append(blendargs[10]);
		else arg1.append(blendargs[8]);
		switch(args[1]&7) //arg2
		{
			case D3DTA_CURRENT:
			default:
				arg2 = blendargs[0];
				break;
			case D3DTA_DIFFUSE:
				arg2 = blendargs[1];
				break;
			case D3DTA_TEXTURE:
				if((texstate[i].shaderid >> 59)&1)
				{
					arg2 = blendargs[2];
					arg2.replace(17,1,_itoa(i,idstring,10));
					arg2.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				arg2 = blendargs[4];
				break;
			case D3DTA_SPECULAR:
				arg2 = blendargs[5];
				break;
		}
		if(args[1] & D3DTA_COMPLEMENT) arg2 = "(1.0 - " + arg2 + ")";
		if(args[1] & D3DTA_ALPHAREPLICATE) arg2.append(blendargs[10]);
		else arg2.append(blendargs[8]);
		if(!texfail) switch(texstate[i].shaderid & 31)
		{
		case D3DTOP_DISABLE:
		default:
			break;
		case D3DTOP_SELECTARG1:
			fsrc->append("color.rgb = " + arg1 + ";\n");
			break;
		case D3DTOP_SELECTARG2:
			fsrc->append("color.rgb = " + arg2 + ";\n");
			break;
		case D3DTOP_MODULATE:
			fsrc->append("color.rgb = " + arg1 + " * " + arg2 + ";\n");
			break;
		case D3DTOP_MODULATE2X:
			fsrc->append("color.rgb = (" + arg1 + " * " + arg2 + ") * 2.0;\n");
			break;
		case D3DTOP_MODULATE4X:
			fsrc->append("color.rgb = (" + arg1 + " * " + arg2 + ") * 4.0;\n");
			break;
		case D3DTOP_ADD:
			fsrc->append("color.rgb = " + arg1 + " + " + arg2 + ";\n");
			break;
		case D3DTOP_ADDSIGNED:
			fsrc->append("color.rgb = " + arg1 + " + " + arg2 + " - .5;\n");
			break;
		case D3DTOP_ADDSIGNED2X:
			fsrc->append("color.rgb = (" + arg1 + " + " + arg2 + " - .5) * 2.0;\n");
			break;
		case D3DTOP_SUBTRACT:
			fsrc->append("color.rgb = " + arg1 + " - " + arg2 + ";\n");
			break;
		case D3DTOP_ADDSMOOTH:
			fsrc->append("color.rgb = " + arg1 + " + " + arg2 + " - " + arg1 + " * " + arg2 + ";\n");
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			fsrc->append("color.rgb = " + arg1 + " * gl_Color.a + " + arg2 + " * (1.0-gl_Color.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			texarg = blendargs[2];
			texarg.replace(17,1,_itoa(i,idstring,10));
			texarg.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
			fsrc->append("color.rgb = " + arg1 + " * " + texarg + ".a + " + arg2 + " * (1.0-"
				+ texarg + ".a);\n");
			break;
		case D3DTOP_BLENDFACTORALPHA:
			fsrc->append("color.rgb = " + arg1 + " * texfactor.a + " + arg2 + " * (1.0-texfactor.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			texarg = blendargs[2];
			texarg.replace(17,1,_itoa(i,idstring,10));
			texarg.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
			fsrc->append("color.rgb = " + arg1 + " + " + arg2 + " * (1.0-" + texarg + ".a);\n");
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			fsrc->append("color.rgb = " + arg1 + " * color.a + " + arg2 + " * (1.0-color.a);\n");
			break;
		}
		if(((texstate[i].shaderid>>17) & 31) == D3DTOP_DISABLE)alphadisabled = true;
		if(alphadisabled) continue;
		// Alpha stage
		texfail = false;
		switch(args[2]&7) //arg1
		{
			case D3DTA_CURRENT:
			default:
				arg1 = blendargs[0]+blendargs[9];
				break;
			case D3DTA_DIFFUSE:
				arg1 = blendargs[1]+blendargs[9];
				break;
			case D3DTA_TEXTURE:
				if((texstate[i].shaderid >> 59)&1)
				{
					arg1 = blendargs[2]+blendargs[9];
					arg1.replace(17,1,_itoa(i,idstring,10));
					arg1.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				arg1 = blendargs[4]+blendargs[9];
				break;
			case D3DTA_SPECULAR:
				arg1 = blendargs[5]+blendargs[9];
				break;
		}
		if(args[2] & D3DTA_COMPLEMENT)
			arg1 = "(1.0 - " + arg1 + ")";
		switch(args[3]&7) //arg2
		{
			case D3DTA_CURRENT:
			default:
				arg2 = blendargs[1]+blendargs[9];
				break;
			case D3DTA_DIFFUSE:
				arg2 = blendargs[1]+blendargs[9];
				break;
			case D3DTA_TEXTURE:
				if((texstate[i].shaderid >> 59)&1)
				{
					arg2 = blendargs[2]+blendargs[9];
					arg2.replace(17,1,_itoa(i,idstring,10));
					arg2.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				arg2 = blendargs[4]+blendargs[9];
				break;
			case D3DTA_SPECULAR:
				arg2 = blendargs[5]+blendargs[9];
				break;
		}
		if(args[3] & D3DTA_COMPLEMENT)
			arg2 = "(1.0 - " + arg2 + ")";
		if(!texfail) switch((texstate[i].shaderid>>17) & 31)
		{
		case D3DTOP_DISABLE:
		default:
			break;
		case D3DTOP_SELECTARG1:
			fsrc->append("color.a = " + arg1 + ";\n");
			break;
		case D3DTOP_SELECTARG2:
			fsrc->append("color.a = " + arg2 + ";\n");
			break;
		case D3DTOP_MODULATE:
			fsrc->append("color.a = " + arg1 + " * " + arg2 + ";\n");
			break;
		case D3DTOP_MODULATE2X:
			fsrc->append("color.a = (" + arg1 + " * " + arg2 + ") * 2.0;\n");
			break;
		case D3DTOP_MODULATE4X:
			fsrc->append("color.a = (" + arg1 + " * " + arg2 + ") * 4.0;\n");
			break;
		case D3DTOP_ADD:
			fsrc->append("color.a = " + arg1 + " + " + arg2 + ";\n");
			break;
		case D3DTOP_ADDSIGNED:
			fsrc->append("color.a = " + arg1 + " + " + arg2 + " - .5;\n");
			break;
		case D3DTOP_ADDSIGNED2X:
			fsrc->append("color.a = (" + arg1 + " + " + arg2 + " - .5) * 2.0;\n");
			break;
		case D3DTOP_SUBTRACT:
			fsrc->append("color.a = " + arg1 + " - " + arg2 + ";\n");
			break;
		case D3DTOP_ADDSMOOTH:
			fsrc->append("color.a = " + arg1 + " + " + arg2 + " - " + arg1 + " * " + arg2 + ";\n");
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			fsrc->append("color.a = " + arg1 + " * gl_Color.a + " + arg2 + " * (1.0-gl_Color.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			texarg = blendargs[2];
			texarg.replace(17,1,_itoa(i,idstring,10));
			texarg.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
			fsrc->append("color.a = " + arg1 + " * " + texarg + ".a + " + arg2 + " * (1.0-"
				+ texarg + ".a);\n");
			break;
		case D3DTOP_BLENDFACTORALPHA:
			fsrc->append("color.a = " + arg1 + " * texfactor.a + " + arg2 + " * (1.0-texfactor.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			texarg = blendargs[2];
			texarg.replace(17,1,_itoa(i,idstring,10));
			texarg.replace(31,1,_itoa((texstate[i].shaderid>>54)&7,idstring,10));
			fsrc->append("color.a = " + arg1 + " + " + arg2 + " * (1.0-" + texarg + ".a);\n");
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			fsrc->append("color.a = " + arg1 + " * color.a + " + arg2 + " * (1.0-color.a);\n");
			break;
		}
	}
	if((id>>2)&1)
	{
		switch((id>>3)&7)
		{
		case 0:
			fsrc->append("discard;\n");
			break;
		case 1:
			fsrc->append("if(int(color.a * 255.5) >= alpharef) discard;");
			break;
		case 2:
			fsrc->append("if(int(color.a * 255.5) != alpharef) discard;");
			break;
		case 3:
			fsrc->append("if(int(color.a * 255.5) > alpharef) discard;");
			break;
		case 4:
			fsrc->append("if(int(color.a * 255.5) <= alpharef) discard;");
			break;
		case 5:
			fsrc->append("if(int(color.a * 255.5) == alpharef) discard;");
			break;
		case 6:
			fsrc->append("if(int(color.a * 255.5) < alpharef) discard;");
			break;
		case 7:
		default:
			break;
		}
	}
	if(vertexfog && !pixelfog) fsrc->append(op_fogblend);
	if(pixelfog)
	{
		fsrc->append(op_fogcoordstandardpixel);
		switch(pixelfog)
		{
		case D3DFOG_LINEAR:
			fsrc->append(op_foglinearpixel);
			break;
		case D3DFOG_EXP:
			fsrc->append(op_fogexppixel);
			break;
		case D3DFOG_EXP2:
			fsrc->append(op_fogexp2pixel);
			break;
		}
		fsrc->append(op_fogclamp);
		fsrc->append(op_fogblend);
	}
	if(((id>>61)&1) && !vertexfog && !pixelfog) fsrc->append(op_fogassign);
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
	// Attributes
	genshaders[index].shader.attribs[0] = glGetAttribLocation(genshaders[index].shader.prog,"xyz");
	genshaders[index].shader.attribs[1] = glGetAttribLocation(genshaders[index].shader.prog,"rhw");
	genshaders[index].shader.attribs[2] = glGetAttribLocation(genshaders[index].shader.prog,"blend0");
	genshaders[index].shader.attribs[3] = glGetAttribLocation(genshaders[index].shader.prog,"blend1");
	genshaders[index].shader.attribs[4] = glGetAttribLocation(genshaders[index].shader.prog,"blend2");
	genshaders[index].shader.attribs[5] = glGetAttribLocation(genshaders[index].shader.prog,"blend3");
	genshaders[index].shader.attribs[6] = glGetAttribLocation(genshaders[index].shader.prog,"blend4");
	genshaders[index].shader.attribs[7] = glGetAttribLocation(genshaders[index].shader.prog,"nxyz");
	genshaders[index].shader.attribs[8] = glGetAttribLocation(genshaders[index].shader.prog,"rgba0");
	genshaders[index].shader.attribs[9] = glGetAttribLocation(genshaders[index].shader.prog,"rgba1");
	char attrS[] = "sX";
	for(int i = 0; i < 8; i++)
	{
		attrS[1] = i + '0';
		genshaders[index].shader.attribs[i+10] = glGetAttribLocation(genshaders[index].shader.prog,attrS);
	}
	char attrST[] = "stX";
	for(int i = 0; i < 8; i++)
	{
		attrST[2] = i + '0';
		genshaders[index].shader.attribs[i+18] = glGetAttribLocation(genshaders[index].shader.prog,attrST);
	}
	char attrSTR[] = "strX";
	for(int i = 0; i < 8; i++)
	{
		attrSTR[3] = i + '0';
		genshaders[index].shader.attribs[i+26] = glGetAttribLocation(genshaders[index].shader.prog,attrSTR);
	}
	char attrSTRQ[] = "strqX";
	for(int i = 0; i < 8; i++)
	{
		attrSTRQ[4] = i + '0';
		genshaders[index].shader.attribs[i+34] = glGetAttribLocation(genshaders[index].shader.prog,attrSTRQ);
	}
	genshaders[index].shader.uniforms[0] = glGetUniformLocation(genshaders[index].shader.prog,"matWorld");
	// Uniforms
	// TODO: 4-14 world1-3 and texture0-7
	char uniflight[] = "lightX.            ";
	for(int i = 0; i < 8; i++)
	{
		uniflight[5] = i + '0';
		strcpy(uniflight+7,"diffuse");
		genshaders[index].shader.uniforms[20+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"specular");
		genshaders[index].shader.uniforms[21+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"ambient");
		genshaders[index].shader.uniforms[22+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"position");
		genshaders[index].shader.uniforms[23+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"direction");
		genshaders[index].shader.uniforms[24+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"range");
		genshaders[index].shader.uniforms[25+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"falloff");
		genshaders[index].shader.uniforms[26+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"constant");
		genshaders[index].shader.uniforms[27+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"linear");
		genshaders[index].shader.uniforms[28+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"quad");
		genshaders[index].shader.uniforms[29+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"theta");
		genshaders[index].shader.uniforms[30+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
		strcpy(uniflight+7,"phi");
		genshaders[index].shader.uniforms[31+(i*12)] = glGetUniformLocation(genshaders[index].shader.prog,uniflight);
	}
	char uniftex[] = "texX";
	for(int i = 0; i < 8; i++)
	{
		uniftex[3] = i + '0';
		genshaders[index].shader.uniforms[128+i] = glGetUniformLocation(genshaders[index].shader.prog,uniftex);
	}
	genshaders[index].shader.uniforms[136] = glGetUniformLocation(genshaders[index].shader.prog,"ambientcolor");
	genshaders[index].shader.uniforms[137] = glGetUniformLocation(genshaders[index].shader.prog,"width");
	genshaders[index].shader.uniforms[138] = glGetUniformLocation(genshaders[index].shader.prog,"height");
	genshaders[index].shader.uniforms[139] = glGetUniformLocation(genshaders[index].shader.prog,"alpharef");
	char unifkey[] = "keyX";
	for(int i = 0; i < 8; i++)
	{
		unifkey[3] = i + '0';
		genshaders[index].shader.uniforms[140+i] = glGetUniformLocation(genshaders[index].shader.prog,unifkey);
	}
}
