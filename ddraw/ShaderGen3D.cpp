// DXGL
// Copyright (C) 2012-2020 William Feely

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
#include "glTexture.h"
#include "glUtil.h"
#include "timer.h"
#include "glRenderer.h"
#include "glDirect3DDevice.h"
#include "string.h"
#include "ShaderGen3D.h"
#include "ShaderGen2D.h"
#include "ShaderManager.h"
#include "../common/version.h"
#include "ddraw.h"


extern "C" {

void ShaderGen3D_Init(glExtensions *glext, ShaderManager *shaderman, ShaderGen3D *gen)
{
	gen->ext = glext;
	gen->shaders = shaderman;
	ZeroMemory(gen->current_texid,8*sizeof(__int64));
	gen->shadercount = 0;
	gen->genindex = 0;
	gen->maxshaders = 256;
	gen->genshaders = (GenShader*)malloc(256 * sizeof(GenShader));
	ZeroMemory(gen->genshaders, 256 * sizeof(GenShader));
	gen->current_shader = 0;
	gen->current_shadertype = 0;
}

void ShaderGen3D_Delete(ShaderGen3D *This)
{
	ShaderGen3D_ClearShaders(This);
	free(This->genshaders);
}
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
Bit 14-17 - Reserved
Bits 18-20 - Number of lights  VS/FS
Bit 21 - Camera relative specular highlights  VS/FS
Bit 22 - Alpha blended color key  FS
Bits 23-24 - Diffuse material source  VS
Bits 25-26 - Specular material source  VS
Bits 27-28 - Ambient material source  VS
Bits 29-30 - Emissive material source  VS
Bits 31-33 - Number of textures  VS/FS
Bit 34 - More than 0 textures  VS/FS
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
Bit 62 - Enable dithering  FS
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
	Bits in flags:
	00=passthru 01=cameraspacenormal
	10=cameraspaceposition 11=cameraspacereflectionvector
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
Bits 54-58: Reserved
Bit 59: Texture image enabled
Bit 60: Texture has color key
*/


/**
  * Deletes all shader programs in the array.
  * @param This
  *  Pointer to ShaderGen3D structure
  */
void ShaderGen3D_ClearShaders(ShaderGen3D *This)
{
	if(!This->genshaders) return;
	for(int i = 0; i < This->shadercount; i++)
	{
		This->genshaders[i].id = 0;
		ZeroMemory(This->genshaders[i].texids,8*sizeof(__int64));
		if(This->genshaders[i].shader.prog) This->ext->glDeleteProgram(This->genshaders[i].shader.prog);
		if(This->genshaders[i].shader.fs) This->ext->glDeleteShader(This->genshaders[i].shader.fs);
		if(This->genshaders[i].shader.vs) This->ext->glDeleteShader(This->genshaders[i].shader.vs);
		if(This->genshaders[i].shader.fsrc.ptr) String_Free(&This->genshaders[i].shader.fsrc);
		if(This->genshaders[i].shader.vsrc.ptr) String_Free(&This->genshaders[i].shader.vsrc);
	}
	if(This->genshaders) free(This->genshaders);
	This->genshaders = NULL;
	This->current_genshader = NULL;
	This->shadercount = 0;
	This->genindex = 0;
}

/**
  * Sort 2D shaders callback, for qsort and bsearch functions
  * @param elem1
  *  First item to compare
  * @param elem2
  *  Second item to compare
  * @return
  *  negative if elem1 is less than elem2, positive if greater, zero if same
  */
int __cdecl compshader2D(const GenShader2D *elem1, const  GenShader2D *elem2)
{
	return memcmp(&elem1->id, &elem2->id, sizeof(__int64));
}

/**
* Sort 2D shaders callback, for qsort and bsearch functions
* @param elem1
*  First item to compare
* @param elem2
*  Second item to compare
* @return
*  negative if elem1 is less than elem2, positive if greater, zero if same
*/
int __cdecl compshader3D(const GenShader *elem1, const  GenShader *elem2)
{
	return memcmp(&elem1->id, &elem2->id, 9 * sizeof(__int64));
}

/**
  * Sets a shader by render state.  If the shader does not exist, generates it.
  * @param This
  *  Pointer to ShaderGen3D structure
  * @param id
  *  64-bit value containing current render states
  * @param texstate
  *  Pointer to the texture state ID.
  * @param texcoords
  *  Pointer to number of texture coordinates in each texture stage
  * @param type
  *  Type of shader:
  *  0 for builtin
  *  1 for generated 2D
  *  2 for generated 3D
  */
void ShaderGen3D_SetShader(ShaderGen3D *This, __int64 id, __int64 *texstate, int type, ShaderGen2D *gen2d)
{
	//int shaderindex = -1;
	switch(type)
	{
	case 0:  // Static built-in shader
		if((This->current_shadertype == 0) && (This->shaders->shaders[id].prog == This->current_shader)) return;
		This->ext->glUseProgram(This->shaders->shaders[id].prog);
		This->current_shader = This->shaders->shaders[id].prog;
		This->current_shadertype = 0;
		This->current_genshader = NULL;
		break;
	case 1:  // 2D generated shader
		if ((This->current_shadertype == 1) && (id == This->current_shader)) return;
		This->current_shader = id;
		This->current_shadertype = 1;
		GenShader2D key2d;
		GenShader2D *shader2d;
		key2d.id = id;
		shader2d = (GenShader2D*)bsearch(&key2d, gen2d->genshaders2D, gen2d->genindex, sizeof(GenShader2D),
			(int(__cdecl *) (const void *, const void *))compshader2D);
		if (!shader2d)
		{
			gen2d->shadercount++;
			if (gen2d->genshaders2D[gen2d->genindex].shader.prog)
			{
				This->ext->glUseProgram(0);
				This->ext->glDeleteProgram(gen2d->genshaders2D[gen2d->genindex].shader.prog);
				This->ext->glDeleteShader(gen2d->genshaders2D[gen2d->genindex].shader.vs);
				This->ext->glDeleteShader(gen2d->genshaders2D[gen2d->genindex].shader.fs);
				String_Free(&gen2d->genshaders2D[gen2d->genindex].shader.vsrc);
				String_Free(&gen2d->genshaders2D[gen2d->genindex].shader.fsrc);
				ZeroMemory(&gen2d->genshaders2D[gen2d->genindex], sizeof(GenShader2D));
			}
			ShaderGen2D_CreateShader2D(gen2d, gen2d->genindex, id);
			gen2d->genindex++;
			if (gen2d->genindex >= gen2d->maxshaders)
			{
				GenShader2D *tmp2dgen = (GenShader2D*)realloc(gen2d->genshaders2D,
					gen2d->maxshaders * 2 * sizeof(GenShader2D));
				if (!tmp2dgen) gen2d->genindex = 0;
				else
				{
					ZeroMemory(&tmp2dgen[gen2d->maxshaders], gen2d->maxshaders * sizeof(GenShader2D));
					gen2d->genshaders2D = tmp2dgen;
					gen2d->maxshaders *= 2;
				}
			}
			qsort(gen2d->genshaders2D, gen2d->genindex, sizeof(GenShader2D), 
				(int(__cdecl *) (const void *, const void *))compshader2D);
			shader2d = (GenShader2D*)bsearch(&key2d, gen2d->genshaders2D, gen2d->genindex, sizeof(GenShader2D),
				(int(__cdecl *) (const void *, const void *))compshader2D);
		}
		if (!shader2d) return;  // Out of memory condition
		This->ext->glUseProgram(shader2d->shader.prog);
		This->current_prog = shader2d->shader.prog;
		This->current_genshader = (GenShader*)shader2d;
		break;
	case 2:  // 3D generated shader
		if((This->current_shadertype == 2) && (id == This->current_shader))
		{
			if(!memcmp(This->current_texid,texstate,8*sizeof(__int64))) return;
		}
		This->current_shader = id;
		This->current_shadertype = 2;
		GenShader key3d;
		GenShader *shader3d;
		key3d.id = id;
		memcpy(&key3d.texids, texstate, 8 * sizeof(__int64));
		shader3d = (GenShader*)bsearch(&key3d, This->genshaders, This->genindex, sizeof(GenShader),
			(int(__cdecl *) (const void *, const void *))compshader3D);
		if(!shader3d)
		{
			This->shadercount++;
			if(This->genshaders[This->genindex].shader.prog)
			{
				This->ext->glUseProgram(0);
				This->ext->glDeleteProgram(This->genshaders[This->genindex].shader.prog);
				This->ext->glDeleteShader(This->genshaders[This->genindex].shader.vs);
				This->ext->glDeleteShader(This->genshaders[This->genindex].shader.fs);
				String_Free(&This->genshaders[This->genindex].shader.vsrc);
				String_Free(&This->genshaders[This->genindex].shader.fsrc);
				ZeroMemory(&This->genshaders[This->genindex],sizeof(GenShader));
			}
			ShaderGen3D_CreateShader(This, This->genindex,id,texstate);
			This->genindex++;
			if (This->genindex >= This->maxshaders)
			{
				GenShader *tmp3dgen = (GenShader*)realloc(This->genshaders,
					This->maxshaders * 2 * sizeof(GenShader));
				if (!tmp3dgen) This->genindex = 0;
				else
				{
					ZeroMemory(&tmp3dgen[This->maxshaders], This->maxshaders * sizeof(GenShader));
					This->genshaders = tmp3dgen;
					This->maxshaders *= 2;
				}
			}
			qsort(This->genshaders, This->genindex, sizeof(GenShader),
				(int(__cdecl *) (const void *, const void *))compshader3D);
			shader3d = (GenShader*)bsearch(&key3d, This->genshaders, This->genindex, sizeof(GenShader),
				(int(__cdecl *) (const void *, const void *))compshader3D);
		}
		if (!shader3d) return; // Out of memory condition
		This->ext->glUseProgram(shader3d->shader.prog);
		This->current_prog = shader3d->shader.prog;
		This->current_genshader = shader3d;
	}
}

/**
  * Retrieves the GLSL program currently in use
  * @param This
  *  Pointer to ShaderGen3D structure
  * @return
  *  Number of the current GLSL program, or if using built-in shaders, the ID of
  *  the shader
  */
GLuint ShaderGen3D_GetProgram(ShaderGen3D *This)
{
	if (This->current_shadertype == 0) return This->current_shader & 0xFFFFFFFF;
	else return This->current_prog;
}


static const char header[] =
	"//REV" STR(SHADER3DVERSION) "\n";
static const char ver110[] = "#version 110\n";
static const char ver120[] = "#version 120\n";
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
static const char unif_viewport[] = "uniform float width;\n\
uniform float height;\n\
uniform float xoffset;\n\
uniform float yoffset;\n\
uniform float minz;\n\
uniform float maxz;\n";
static const char unif_alpharef[] = "uniform int alpharef;\n";
static const char unif_key[] = "uniform ivec3 keyX;\n";
static const char unif_keybits[] = "uniform ivec4 keybitsX;\n";

static const char unif_world[] = "uniform mat4 matWorld;\n";
static const char unif_modelview[] = "uniform mat4 matModelView;\n";
static const char unif_projection[] = "uniform mat4 matProjection;\n";
static const char unif_normal[] = "uniform mat3 matNormal;\n";
static const char unif_ditherbits[] = "uniform ivec4 ditherbits;\n";

static const char unif_material[] = "uniform vec4 mtlambient;\n\
uniform vec4 mtldiffuse;\n\
uniform vec4 mtlspecular;\n\
uniform vec4 mtlemission;\n\
uniform float mtlshininess;\n";

static const char unif_fogcolor[] = "uniform vec4 fogcolor;\n";
static const char unif_fogstart[] = "uniform float fogstart;\n";
static const char unif_fogend[] = "uniform float fogend;\n";
static const char unif_fogdensity[] = "uniform float fogdensity;\n";

// Variables
static const char var_common[] = "vec4 diffuse;\n\
vec4 specular;\n\
vec4 ambient;\n\
vec3 N;\n";
static const char var_color[] = "vec4 color;\n";
static const char var_xyzw[] = "vec4 xyzw;\n";
static const char var_fogfactorvertex[] = "varying float fogfactor;\n";
static const char var_fogfactorpixel[] = "float fogfactor;\n";
static const char var_keycomp[] = "ivec4 keycomp;\n";
// Constants
static const char const_nxyz[] = "const vec3 nxyz = vec3(0,0,0);\n";
static const char const_threshold[] = "mat4 threshold = mat4(\n\
1.0, 9.0, 3.0, 11.0,\n\
13.0, 5.0, 15.0, 7.0,\n\
4.0, 12.0, 2.0, 10.0,\n\
16.0, 8.0, 14.0, 6.0);\n";

// Operations
static const char op_transform[] = "xyzw = vec4(xyz,1.0);\n\
vec4 pos = matProjection*matModelView*xyzw;\n\
gl_Position = vec4(pos.x,-pos.y,pos.z,pos.w);\n";
static const char op_normalize[] = "N = normalize(matNormal*nxyz);\n";
static const char op_normalpassthru[] = "N = matNormal*nxyz;\n";
static const char op_tlvertex[] = "gl_Position = vec4(((xyz.x-xoffset)/(width/2.0)-1.0)/rhw,\
((xyz.y-yoffset)/(height/2.0)-1.0)/rhw,xyz.z/rhw,1.0/rhw);\n";
static const char op_resetcolor[] = "diffuse = specular = vec4(0.0);\n\
ambient = ambientcolor / 255.0;\n";
static const char op_dirlight[] = "DirLight(lightX);\n";
static const char op_pointlight[] = "PointLight(lightX);\n";
static const char op_spotlight[] = "SpotLight(lightX);\n";
static const char op_colorout[] = "gl_FrontColor = (mtldiffuse * diffuse) + (mtlambient * ambient)\n\
+ (mtlspecular * specular) + mtlemission;\n\
gl_FrontSecondaryColor = (mtlspecular * specular);\n";
static const char op_colorvert[] = "gl_FrontColor = rgba0.bgra;\n";
static const char op_color2vert[] = "gl_FrontSecondaryColor = rgba1.bgra;\n";
static const char op_colorwhite[] = "gl_FrontColor = vec4(1.0,1.0,1.0,1.0);\n";
static const char op_colorfragout[] = "gl_FragColor = color;\n";
static const char op_dither[] = "color = dither(color);\n";
static const char op_colorfragin[] = "color = gl_Color;\n";
static const char op_colorkeyin[] = "keycomp = ivec4(texture2DProj(texX,gl_TexCoord[Y])*vec4(keybitsZ)+.5);\n";
static const char op_colorkey[] = "if(keycomp.rgb == keyX) discard;\n";
static const char op_texpassthru1[] = "gl_TexCoord[x] = ";
static const char op_texpassthru2s[] = "vec4(sX,0,0,1);\n";
static const char op_texpassthru2st[] = "vec4(stX,0,1);\n";
static const char op_texpassthru2str[] = "vec4(strX,1);\n";
static const char op_texpassthru2strq[] = "strqX;\n";
static const char op_texpassthru2null[] = "vec4(0,0,0,1);\n";
static const char op_fogcoordstandardpixel[] = "float fogcoord = gl_FragCoord.z / gl_FragCoord.w;\n";
static const char op_fogcoordstandard[] = "gl_FogFragCoord = abs(matModelView*xyzw).z;\n";
static const char op_fogcoordrange[] = "vec4 eyepos = matModelView*xyzw;\n\
vec3 eyepos3 = eyepos.xyz / eyepos.w;\n\
gl_FogFragCoord = sqrt((eyepos3.x * eyepos3.x) + (eyepos3.y * eyepos3.y) + (eyepos3.z * eyepos3.z));\n";
static const char op_foglinear[] = "fogfactor = (fogend - gl_FogFragCoord) / (fogend - fogstart);\n";
static const char op_fogexp[] = "fogfactor = 1.0 / exp(gl_FogFragCoord * fogdensity);\n";
static const char op_fogexp2[] = "fogfactor = 1.0 / exp(gl_FogFragCoord * gl_FogFragCoord *\n\
fogdensity * fogdensity);\n";
static const char op_foglinearpixel[] = "fogfactor = (fogend - fogcoord) / (fogend - fogstart);\n";
static const char op_fogexppixel[] = "fogfactor = 1.0 / exp(fogcoord * fogdensity);\n";
static const char op_fogexp2pixel[] = "fogfactor = 1.0 / exp(fogcoord * fogcoord *\n\
fogdensity * fogdensity);\n";
static const char op_fogclamp[] = "fogfactor = clamp(fogfactor,0.0,1.0);\n";
static const char op_fogblend[] = "color = mix(fogcolor,color,fogfactor);\n";
static const char op_fogassign[] = "color = fogcolor;\n";

// Functions
static const char func_dirlight[] = "void DirLight(in Light light)\n\
{\n\
float NdotHV = 0.0;\n\
vec3 dir = normalize(-light.direction);\n\
ambient += light.ambient;\n\
float NdotL = max(dot(N,dir),0.0);\n\
diffuse += light.diffuse*NdotL;\n\
if((NdotL > 0.0) && (mtlshininess != 0.0))\n\
{\n\
vec3 eye = vec3(0.0,0.0,1.0);\n\
vec3 P = vec3(matModelView*xyzw);\n\
vec3 L = normalize(-light.direction.xyz - P);\n\
vec3 V = normalize(eye - P);\n\
NdotHV = max(dot(N,L+V),0.0);\n\
specular += (pow(NdotHV,float(mtlshininess))*light.specular);\n\
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
else if(mtlshininess > 0.0) pf = pow(NdotHV,mtlshininess);\n\
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
else if(mtlshininess > 0.0) pf = pow(NdotHV,mtlshininess);\n\
else pf = 0.0;\n\
float spotangle = dot(-V,normalize(light.direction));\n\
if(spotangle < cos(light.phi * (180.0/3.14159265)))\n\
attenuation = 0.0;\n\
diffuse += light.diffuse*NdotV*attenuation;\n\
ambient += light.ambient;\n\
specular += light.specular*pf*attenuation;\n\
}\n";
static const char func_dither[] = "vec4 dither(vec4 color2)\n\
{\n\
	vec4 color = color2;\n\
	int x = int(mod(gl_FragCoord.x, 4.0));\n\
	int y = int(mod(gl_FragCoord.y, 4.0));\n\
	vec4 limit;\n\
	limit.r = (threshold[x][y]) / ((pow(2.0, float(ditherbits.r)) - 1.0)*16.0);\n\
	limit.g = (threshold[x][y]) / ((pow(2.0, float(ditherbits.g)) - 1.0)*16.0);\n\
	limit.b = (threshold[x][y]) / ((pow(2.0, float(ditherbits.b)) - 1.0)*16.0);\n\
	color.r += limit.r;\n\
	color.g += limit.g;\n\
	color.b += limit.b;\n\
	color.r *= pow(2.0, float(ditherbits.r)) - 1.0;\n\
	color.r = floor(color.r);\n\
	color.g *= pow(2.0, float(ditherbits.g)) - 1.0;\n\
	color.g = floor(color.g);\n\
	color.b *= pow(2.0, float(ditherbits.b)) - 1.0;\n\
	color.b = floor(color.b);\n\
	color.r /= pow(2.0, float(ditherbits.r)) - 1.0;\n\
	color.g /= pow(2.0, float(ditherbits.g)) - 1.0;\n\
	color.b /= pow(2.0, float(ditherbits.b)) - 1.0;\n\
	return color;\n\
}\n";


/**
  * Creates an OpenGL shader program
  * @param This
  *  Pointer to ShaderGen3D structure
  * @param index
  *  Index of the shader in the array to generate
  * @param id
  *  64-bit value containing current render states
  * @param texstate
  *  Pointer to the texture stage state array, containing 8 64-bit state values
  * @param texcoords
  *  Pointer to number of texture coordinates in each texture stage
  */
void ShaderGen3D_CreateShader(ShaderGen3D *This, int index, __int64 id, __int64 *texstate)
{
	STRING tmp;
	ZeroMemory(&tmp, sizeof(STRING));
	int i;
	bool hasdir = false;
	bool haspoint = false;
	bool hasspot = false;
	bool dither = false;
	BOOL haskey = FALSE;
	int count;
	int numlights;
	int numtex;
	int vertexfog,pixelfog;
	vertexfog = pixelfog = 0;
	if ((id>>34)&1) numtex = ((id >> 31) & 7) + 1;
	else numtex = 0;
	if((id>>61)&1)
	{
		vertexfog = (id>>8)&3;
		pixelfog = (id>>6)&3;
	}
	char idstring[22];
	_snprintf(idstring,21,"%0.16I64X\n",id);
	idstring[21] = 0;
	This->genshaders[index].shader.vsrc.ptr = NULL;
	This->genshaders[index].shader.fsrc.ptr = NULL;
	// Create vertex shader
	//Header
	STRING *vsrc = &This->genshaders[index].shader.vsrc;
	String_Append(vsrc, header);
	String_Append(vsrc, ver110);
	String_Append(vsrc, vertexshader);
	String_Append(vsrc, idheader);
	String_Append(vsrc, idstring);
	// Attributes
	String_Append(vsrc, attr_xyz);
	if((id>>50)&1) String_Append(vsrc, attr_rhw);
	String_Assign(&tmp, attr_rgba);
	if((id>>35)&1)
	{
		tmp.ptr[19] = '0';
		String_Append(vsrc, tmp.ptr);
	}
	if((id>>36)&1)
	{
		tmp.ptr[19] = '1';
		String_Append(vsrc, tmp.ptr);
	}
	if((id>>37)&1) String_Append(vsrc, attr_nxyz);
	else String_Append(vsrc, const_nxyz);
	count = (id>>46)&7;
	if(count)
	{
		String_Assign(&tmp,attr_blend);
		for(i = 0; i < count; i++)
		{
			tmp.ptr[21] = *(_itoa(i,idstring,10));
			String_Append(vsrc, tmp.ptr);
		}
	}
	for(i = 0; i < numtex; i++)
	{
		switch ((texstate[i] >> 51)&3)
		{
		case 0:
			String_Assign(&tmp,attr_s);
			tmp.ptr[16] = *(_itoa(i,idstring,10));
			break;
		case 1:
			String_Assign(&tmp, attr_st);
			tmp.ptr[17] = *(_itoa(i,idstring,10));
			break;
		case 2:
			String_Assign(&tmp, attr_str);
			tmp.ptr[18] = *(_itoa(i,idstring,10));
			break;
		case 3:
			String_Assign(&tmp, attr_strq);
			tmp.ptr[19] = *(_itoa(i,idstring,10));
			break;
		}
		String_Append(vsrc, tmp.ptr);
	}

	// Uniforms
	String_Append(vsrc, unif_ambient);
	if((id>>50)&1) String_Append(vsrc, unif_viewport);
	if((id>>59)&1) numlights = (id>>18)&7;
	else numlights = 0;
	if((id>>50)&1) numlights = 0;
	if(numlights) // Lighting
	{
		String_Append(vsrc, lightstruct);
		String_Append(vsrc, unif_world);
		String_Append(vsrc, unif_normal);
		String_Append(vsrc, unif_material);
		String_Assign(&tmp, unif_light);
		for(i = 0; i < numlights; i++)
		{
			tmp.ptr[19] = *(_itoa(i,idstring,10));
			String_Append(vsrc, tmp.ptr);
		}
	}
	else if (!((id >> 49) & 1)) String_Append(vsrc, unif_normal);
	if (numlights || !((id >> 50) & 1) || vertexfog)
	{
		String_Append(vsrc, unif_modelview);
		String_Append(vsrc, unif_projection);
	}
	if (vertexfog || pixelfog)
	{
		String_Append(vsrc, unif_fogcolor);
		String_Append(vsrc, unif_fogstart);
		String_Append(vsrc, unif_fogend);
		String_Append(vsrc, unif_fogdensity);
	}
	// Variables
	String_Append(vsrc, var_common);
	String_Append(vsrc, var_xyzw);
	if(vertexfog && !pixelfog) String_Append(vsrc, var_fogfactorvertex);

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
	if(hasspot) String_Append(vsrc, func_spotlight);
	if(haspoint) String_Append(vsrc, func_pointlight);
	if(hasdir) String_Append(vsrc, func_dirlight);
	//Main
	String_Append(vsrc, mainstart);
	if((id>>50)&1) String_Append(vsrc, op_tlvertex);
	else String_Append(vsrc, op_transform);
	if((id>>49)&1) String_Append(vsrc, op_normalize);
	else String_Append(vsrc, op_normalpassthru);
	const char *colorargs[] = {"mtldiffuse","mtlambient","mtlspecular",
		"mtlemission","rgba0.bgra","rgba1.bgra"};
	if(numlights)
	{
		String_Append(vsrc, op_resetcolor);
		for(i = 0; i < numlights; i++)
		{
			if(id>>(38+i)&1)
			{
				if(id>>(51+i)&1)
				{
					String_Assign(&tmp, op_spotlight);
					tmp.ptr[15] = *(_itoa(i,idstring,10));
					String_Append(vsrc, tmp.ptr);
				}
				else
				{
					String_Assign(&tmp, op_pointlight);
					tmp.ptr[16] = *(_itoa(i,idstring,10));
					String_Append(vsrc, tmp.ptr);
				}
			}
			else
			{
				String_Assign(&tmp, op_dirlight);
				tmp.ptr[14] = *(_itoa(i,idstring,10));
				String_Append(vsrc, tmp.ptr);
			}
		}
		if((id>>60)&1)
		{
			bool hascolor1 = false;
			if((id>>35)&1) hascolor1 = true;
			bool hascolor2 = false;
			if((id>>36)&1) hascolor2 = true;
			int matcolor;
			String_Append(vsrc, "gl_FrontColor = (");
			matcolor = ((id>>23)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) String_Append(vsrc, colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) String_Append(vsrc, colorargs[5]);
			else String_Append(vsrc, colorargs[0]);
			String_Append(vsrc, " * diffuse) + (");
			matcolor = ((id>>27)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) String_Append(vsrc, colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) String_Append(vsrc, colorargs[5]);
			else String_Append(vsrc, colorargs[1]);
			String_Append(vsrc, " * ambient)\n+ (");
			matcolor = ((id>>25)&3);
			if((matcolor == D3DMCS_COLOR1) && hascolor1) String_Append(vsrc, colorargs[4]);
			else if((matcolor == D3DMCS_COLOR2) && hascolor2) String_Append(vsrc, colorargs[5]);
			else String_Append(vsrc, colorargs[2]);
			String_Append(vsrc, " * specular) + ");
			matcolor = ((id>>29)&3);
			if ((matcolor == D3DMCS_COLOR1) && hascolor1) String_Append(vsrc, colorargs[4]);
			else if ((matcolor == D3DMCS_COLOR2) && hascolor2) String_Append(vsrc, colorargs[5]);
			else String_Append(vsrc, colorargs[3]);
			String_Append(vsrc, ";\n");
		}
		else String_Append(vsrc, op_colorout);
	}
	else
	{
		if((id>>35)&1) String_Append(vsrc, op_colorvert);
		else String_Append(vsrc, op_colorwhite);
		if((id>>36)&1) String_Append(vsrc, op_color2vert);
	}
	int texindex;
	for(i = 0; i < 8; i++)
	{
		if((texstate[i]>>50)&1)
		{
			FIXME("Support texture coordinate transform");
		}
		else
		{
			String_Assign(&tmp,op_texpassthru1);
			tmp.ptr[12] = *(_itoa(i,idstring,10));
			String_Append(vsrc, tmp.ptr);
			texindex = (texstate[i]>>34)&3;
			switch ((texstate[texindex] >> 51) & 3)
			{
			case 0: // s
				String_Assign(&tmp, op_texpassthru2s);
				tmp.ptr[6] = *(_itoa(texindex, idstring, 10));
				String_Append(vsrc, tmp.ptr);
				break;
			case 1: // st
				String_Assign(&tmp, op_texpassthru2st);
				tmp.ptr[7] = *(_itoa(texindex,idstring,10));
				String_Append(vsrc, tmp.ptr);
				break;
			case 2: // str
				String_Assign(&tmp, op_texpassthru2str);
				tmp.ptr[8] = *(_itoa(texindex,idstring,10));
				String_Append(vsrc, tmp.ptr);
				break;
			case 3: // strq
				String_Assign(&tmp, op_texpassthru2strq);
				tmp.ptr[4] = *(_itoa(texindex,idstring,10));
				String_Append(vsrc, tmp.ptr);
				break;
			}
		}
	}
	if(vertexfog && !pixelfog)
	{
		if((id>>10)&1) String_Append(vsrc, op_fogcoordrange);
		else String_Append(vsrc, op_fogcoordstandard);
		switch(vertexfog)
		{
		case D3DFOG_LINEAR:
			String_Append(vsrc, op_foglinear);
			break;
		case D3DFOG_EXP:
			String_Append(vsrc, op_fogexp);
			break;
		case D3DFOG_EXP2:
			String_Append(vsrc, op_fogexp2);
			break;
		}
		String_Append(vsrc, op_fogclamp);
	}
	String_Append(vsrc, mainend);
#ifdef _DEBUG
	OutputDebugStringA("Vertex shader:\n");
	OutputDebugStringA(vsrc->ptr);
	OutputDebugStringA("\nCompiling vertex shader:\n");
	TRACE_STRING("Vertex shader:\n");
	TRACE_STRING(vsrc->ptr);
	TRACE_STRING("\nCompiling vertex shader:\n");
#endif
	This->genshaders[index].shader.vs = This->ext->glCreateShader(GL_VERTEX_SHADER);
	const char *src = vsrc->ptr;
	GLint srclen = strlen(src);
	This->ext->glShaderSource(This->genshaders[index].shader.vs,1,&src,&srclen);
	This->ext->glCompileShader(This->genshaders[index].shader.vs);
	GLint result;
	char *infolog = NULL;
	This->ext->glGetShaderiv(This->genshaders[index].shader.vs,GL_COMPILE_STATUS,&result);
#ifdef _DEBUG
	GLint loglen;
	if(!result)
	{
		This->ext->glGetShaderiv(This->genshaders[index].shader.vs,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		This->ext->glGetShaderInfoLog(This->genshaders[index].shader.vs,loglen,&result,infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Compilation failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
#endif
	// Create fragment shader
	if ((id>>62)&1)	dither = true;
	STRING *fsrc = &This->genshaders[index].shader.fsrc;
	String_Append(fsrc, header);
	String_Append(fsrc, ver110);
	String_Append(fsrc, fragshader);
	_snprintf(idstring,21,"%0.16I64X\n",id);
	idstring[21] = 0;
	String_Append(fsrc, idheader);
	String_Append(fsrc, idstring);
	// Uniforms
	for(i = 0; i < 8; i++)
	{
		if((texstate[i] & 31) == D3DTOP_DISABLE)break;
		String_Assign(&tmp, unif_tex);
		tmp.ptr[21] = *(_itoa(i,idstring,10));
		String_Append(fsrc, tmp.ptr);
	}
	if((id>>13)&1)
	{
		for(i = 0; i < 8; i++)
		{
			if((texstate[i]>>60)&1)
			{
				String_Assign(&tmp, unif_key);
				tmp.ptr[17] = *(_itoa(i,idstring,10));
				String_Append(fsrc, tmp.ptr);
				String_Assign(&tmp, unif_keybits);
				tmp.ptr[21] = *(_itoa(i, idstring, 10));
				String_Append(fsrc, tmp.ptr);
				haskey = TRUE;
			}
		}
	}
	if((id>>2)&1) String_Append(fsrc, unif_alpharef);
	if (dither) String_Append(fsrc, unif_ditherbits);
	if (vertexfog || pixelfog)
	{
		String_Append(fsrc, unif_fogcolor);
		String_Append(fsrc, unif_fogstart);
		String_Append(fsrc, unif_fogend);
		String_Append(fsrc, unif_fogdensity);
	}
	// Variables
	String_Append(fsrc, var_color);
	if(vertexfog && !pixelfog) String_Append(fsrc, var_fogfactorvertex);
	if(pixelfog) String_Append(fsrc, var_fogfactorpixel);
	if (dither) String_Append(fsrc, const_threshold);
	if (haskey) String_Append(fsrc, var_keycomp);
	// Functions
	if (dither) String_Append(fsrc, func_dither);
	// Main
	String_Append(fsrc, mainstart);
	String_Append(fsrc, op_colorfragin);
	STRING arg1,arg2;
	STRING texarg;
	ZeroMemory(&arg1, sizeof(STRING));
	ZeroMemory(&arg2, sizeof(STRING));
	ZeroMemory(&texarg, sizeof(STRING));
	int args[4];
	bool texfail;
	bool alphadisabled = false;
	const char *blendargs[] = {"color","gl_Color","texture2DProj(texX,gl_TexCoord[Y])",
		"","texfactor","gl_SecondaryColor","vec3(1,1,1)","1",".rgb",".a",".aaa"};
	bool usecolorkey = false;
	if((id>>13)&1) usecolorkey = true;
	for(i = 0; i < 8; i++)
	{
		if((texstate[i] & 31) == D3DTOP_DISABLE)break;
		args[0] = (texstate[i]>>5)&63;
		args[1] = (texstate[i]>>11)&63;
		args[2] = (texstate[i]>>22)&63;
		args[3] = (texstate[i]>>28)&63;
		// Color key
		if(usecolorkey)
		{
			if((texstate[i]>>60)&1)
			{
				String_Assign(&arg1, op_colorkeyin);
				arg1.ptr[33] = *(_itoa(i, idstring, 10));
				arg1.ptr[47] = *(_itoa((texstate[i] >> 34) & 7, idstring, 10));
				arg1.ptr[63] = *(_itoa(i, idstring, 10));
				String_Append(fsrc, arg1.ptr);
				String_Assign(&arg1, op_colorkey);
				arg1.ptr[21] = *(_itoa(i,idstring,10));
				String_Append(fsrc, arg1.ptr);
			}
		}
		// Color stage
		texfail = false;
		switch(args[0]&7) //arg1
		{
			case D3DTA_CURRENT:
			default:
				String_Assign(&arg1, blendargs[0]);
				break;
			case D3DTA_DIFFUSE:
				String_Assign(&arg1, blendargs[1]);
				break;
			case D3DTA_TEXTURE:
				if((texstate[i] >> 59)&1)
				{
					String_Assign(&arg1, blendargs[2]);
					arg1.ptr[17] = *(_itoa(i,idstring,10));
					arg1.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				String_Append(&arg1, blendargs[4]);
				break;
			case D3DTA_SPECULAR:
				String_Append(&arg1, blendargs[5]);
				break;
		}
		if (args[0] & D3DTA_COMPLEMENT)
		{
			String_Assign(&tmp, arg1.ptr);
			String_Append(&tmp, "(1.0 - ");
			String_Append(&tmp, arg1.ptr);
			String_Append(&tmp, ")");
			String_Assign(&arg1, tmp.ptr);
		}
		if (args[0] & D3DTA_ALPHAREPLICATE) String_Append(&arg1, blendargs[10]);
		else String_Append(&arg1, blendargs[8]);
		switch(args[1]&7) //arg2
		{
			case D3DTA_CURRENT:
			default:
				String_Assign(&arg2, blendargs[0]);
				break;
			case D3DTA_DIFFUSE:
				String_Assign(&arg2, blendargs[1]);
				break;
			case D3DTA_TEXTURE:
				if((texstate[i] >> 59)&1)
				{
					String_Assign(&arg2, blendargs[2]);
					arg2.ptr[17] = *(_itoa(i,idstring,10));
					arg2.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				String_Assign(&arg2, blendargs[4]);
				break;
			case D3DTA_SPECULAR:
				String_Assign(&arg2, blendargs[5]);
				break;
		}
		if(args[1] & D3DTA_COMPLEMENT)
		{
			String_Assign(&tmp, arg2.ptr);
			String_Append(&tmp, "(1.0 - ");
			String_Append(&tmp, arg2.ptr);
			String_Append(&tmp, ")");
			String_Assign(&arg2, tmp.ptr);
		}
		if (args[1] & D3DTA_ALPHAREPLICATE) String_Append(&arg2, blendargs[10]);
		else String_Append(&arg2, blendargs[8]);
		if(!texfail) switch(texstate[i] & 31)
		{
		case D3DTOP_DISABLE:
		default:
			break;
		case D3DTOP_SELECTARG1:
			String_Append(fsrc, "color.rgb = ");
				String_Append(fsrc, arg1.ptr);
				String_Append(fsrc, ";\n");
			break;
		case D3DTOP_SELECTARG2:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_MODULATE:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_MODULATE2X:
			String_Append(fsrc, "color.rgb = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ") * 2.0;\n");
			break;
		case D3DTOP_MODULATE4X:
			String_Append(fsrc, "color.rgb = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ") * 4.0;\n");
			break;
		case D3DTOP_ADD:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_ADDSIGNED:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - .5;\n");
			break;
		case D3DTOP_ADDSIGNED2X:
			String_Append(fsrc, "color.rgb = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - .5) * 2.0;\n");
			break;
		case D3DTOP_SUBTRACT:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " - ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_ADDSMOOTH:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * gl_Color.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-gl_Color.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			String_Assign(&texarg, blendargs[2]);
			texarg.ptr[17] = *(_itoa(i,idstring,10));
			texarg.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a);\n");
			break;
		case D3DTOP_BLENDFACTORALPHA:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * texfactor.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-texfactor.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			String_Assign(&texarg, blendargs[2]);
			texarg.ptr[17] = *(_itoa(i,idstring,10));
			texarg.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a);\n");
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			String_Append(fsrc, "color.rgb = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * color.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-color.a);\n");
			break;
		}
		if(((texstate[i]>>17) & 31) == D3DTOP_DISABLE)alphadisabled = true;
		if(alphadisabled) continue;
		// Alpha stage
		texfail = false;
		switch(args[2]&7) //arg1
		{
			case D3DTA_CURRENT:
			default:
				String_Assign(&arg1, blendargs[0]);
				String_Append(&arg1, blendargs[9]);
				break;
			case D3DTA_DIFFUSE:
				String_Assign(&arg1, blendargs[1]);
				String_Append(&arg1, blendargs[9]);
				break;
			case D3DTA_TEXTURE:
				if((texstate[i] >> 59)&1)
				{
					String_Assign(&arg1, blendargs[2]);
					String_Append(&arg1, blendargs[9]);
					arg1.ptr[17] = *(_itoa(i,idstring,10));
					arg1.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				String_Assign(&arg1, blendargs[4]);
				String_Append(&arg1, blendargs[9]);
				break;
			case D3DTA_SPECULAR:
				String_Assign(&arg1, blendargs[5]);
				String_Append(&arg1, blendargs[9]);
				break;
		}
		if(args[2] & D3DTA_COMPLEMENT)
		{
			String_Assign(&tmp, arg1.ptr);
			String_Append(&tmp, "(1.0 - ");
			String_Append(&tmp, arg1.ptr);
			String_Append(&tmp, ")");
			String_Assign(&arg1, tmp.ptr);
		}
		switch (args[3] & 7) //arg2
		{
			case D3DTA_CURRENT:
			default:
				String_Assign(&arg2, blendargs[0]);
				String_Append(&arg2, blendargs[9]);
				break;
			case D3DTA_DIFFUSE:
				String_Assign(&arg2, blendargs[1]);
				String_Append(&arg2, blendargs[9]);
				break;
			case D3DTA_TEXTURE:
				if((texstate[i] >> 59)&1)
				{
					String_Assign(&arg2, blendargs[2]);
					String_Append(&arg2, blendargs[9]);
					arg2.ptr[17] = *(_itoa(i,idstring,10));
					arg2.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
				}
				else texfail = true;
				break;
			case D3DTA_TFACTOR:
				FIXME("Support texture factor value");
				String_Assign(&arg2, blendargs[4]);
				String_Append(&arg2, blendargs[9]);
				break;
			case D3DTA_SPECULAR:
				String_Assign(&arg2, blendargs[5]);
				String_Append(&arg2, blendargs[9]);
				break;
		}
		if(args[3] & D3DTA_COMPLEMENT)
		{
			String_Assign(&tmp, arg2.ptr);
			String_Append(&tmp, "(1.0 - ");
			String_Append(&tmp, arg2.ptr);
			String_Append(&tmp, ")");
			String_Assign(&arg2, tmp.ptr);
		}
		if (!texfail) switch ((texstate[i] >> 17) & 31)
		{
		case D3DTOP_DISABLE:
		default:
			break;
		case D3DTOP_SELECTARG1:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_SELECTARG2:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_MODULATE:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_MODULATE2X:
			String_Append(fsrc, "color.a = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ") * 2.0;\n");
			break;
		case D3DTOP_MODULATE4X:
			String_Append(fsrc, "color.a = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ") * 4.0;\n");
			break;
		case D3DTOP_ADD:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_ADDSIGNED:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - .5;\n");
			break;
		case D3DTOP_ADDSIGNED2X:
			String_Append(fsrc, "color.a = (");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - .5) * 2.0;\n");
			break;
		case D3DTOP_SUBTRACT:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " - ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_ADDSMOOTH:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " - ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, ";\n");
			break;
		case D3DTOP_BLENDDIFFUSEALPHA:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * gl_Color.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-gl_Color.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHA:
			String_Assign(&texarg, blendargs[2]);
			texarg.ptr[17] = *(_itoa(i,idstring,10));
			texarg.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * ");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a);\n");
			break;
		case D3DTOP_BLENDFACTORALPHA:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * texfactor.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-texfactor.a);\n");
			break;
		case D3DTOP_BLENDTEXTUREALPHAPM:
			String_Assign(&texarg, blendargs[2]);
			texarg.ptr[17] = *(_itoa(i,idstring,10));
			texarg.ptr[31] = *(_itoa((texstate[i]>>34)&7,idstring,10));
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-");
			String_Append(fsrc, texarg.ptr);
			String_Append(fsrc, ".a);\n");
			break;
		case D3DTOP_BLENDCURRENTALPHA:
			String_Append(fsrc, "color.a = ");
			String_Append(fsrc, arg1.ptr);
			String_Append(fsrc, " * color.a + ");
			String_Append(fsrc, arg2.ptr);
			String_Append(fsrc, " * (1.0-color.a);\n");
			break;
		}
	}
	if((id>>2)&1)
	{
		switch((id>>3)&7)
		{
		case 0:
			String_Append(fsrc, "discard;\n");
			break;
		case 1:
			String_Append(fsrc, "if(int(color.a * 255.5) >= alpharef) discard;");
			break;
		case 2:
			String_Append(fsrc, "if(int(color.a * 255.5) != alpharef) discard;");
			break;
		case 3:
			String_Append(fsrc, "if(int(color.a * 255.5) > alpharef) discard;");
			break;
		case 4:
			String_Append(fsrc, "if(int(color.a * 255.5) <= alpharef) discard;");
			break;
		case 5:
			String_Append(fsrc, "if(int(color.a * 255.5) == alpharef) discard;");
			break;
		case 6:
			String_Append(fsrc, "if(int(color.a * 255.5) < alpharef) discard;");
			break;
		case 7:
		default:
			break;
		}
	}
	if(vertexfog && !pixelfog) String_Append(fsrc, op_fogblend);
	if(pixelfog)
	{
		String_Append(fsrc, op_fogcoordstandardpixel);
		switch(pixelfog)
		{
		case D3DFOG_LINEAR:
			String_Append(fsrc, op_foglinearpixel);
			break;
		case D3DFOG_EXP:
			String_Append(fsrc, op_fogexppixel);
			break;
		case D3DFOG_EXP2:
			String_Append(fsrc, op_fogexp2pixel);
			break;
		}
		String_Append(fsrc, op_fogclamp);
		String_Append(fsrc, op_fogblend);
	}
	//if(((id>>61)&1) && !vertexfog && !pixelfog) String_Append(fsrc, op_fogassign);
	if (dither) String_Append(fsrc,op_dither);
	String_Append(fsrc, op_colorfragout);
	String_Append(fsrc, mainend);
	String_Free(&tmp);
	String_Free(&arg1);
	String_Free(&arg2);
	String_Free(&texarg);
#ifdef _DEBUG
	OutputDebugStringA("Fragment shader:\n");
	OutputDebugStringA(fsrc->ptr);
	OutputDebugStringA("\nCompiling fragment shader:\n");
	TRACE_STRING("Fragment shader:\n");
	TRACE_STRING(fsrc->ptr);
	TRACE_STRING("\nCompiling fragment shader:\n"); 
#endif
	This->genshaders[index].shader.fs = This->ext->glCreateShader(GL_FRAGMENT_SHADER);
	src = fsrc->ptr;
	srclen = strlen(src);
	This->ext->glShaderSource(This->genshaders[index].shader.fs,1,&src,&srclen);
	This->ext->glCompileShader(This->genshaders[index].shader.fs);
	This->ext->glGetShaderiv(This->genshaders[index].shader.fs,GL_COMPILE_STATUS,&result);
#ifdef _DEBUG
	if(!result)
	{
		This->ext->glGetShaderiv(This->genshaders[index].shader.fs,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		This->ext->glGetShaderInfoLog(This->genshaders[index].shader.fs,loglen,&result,infolog);
		OutputDebugStringA("Compilation failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Compilation failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
	OutputDebugStringA("\nLinking program:\n");
#endif
	This->genshaders[index].shader.prog = This->ext->glCreateProgram();
	This->ext->glAttachShader(This->genshaders[index].shader.prog,This->genshaders[index].shader.vs);
	This->ext->glAttachShader(This->genshaders[index].shader.prog,This->genshaders[index].shader.fs);
	This->ext->glLinkProgram(This->genshaders[index].shader.prog);
	This->ext->glGetProgramiv(This->genshaders[index].shader.prog,GL_LINK_STATUS,&result);
#ifdef _DEBUG
	if(!result)
	{
		This->ext->glGetProgramiv(This->genshaders[index].shader.prog,GL_INFO_LOG_LENGTH,&loglen);
		infolog = (char*)malloc(loglen);
		This->ext->glGetProgramInfoLog(This->genshaders[index].shader.prog,loglen,&result,infolog);
		OutputDebugStringA("Program link failed. Error messages:\n");
		OutputDebugStringA(infolog);
		TRACE_STRING("Program link failed. Error messages:\n");
		TRACE_STRING(infolog);
		free(infolog);
	}
#endif
	// Attributes
	This->genshaders[index].shader.attribs[0] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"xyz");
	This->genshaders[index].shader.attribs[1] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"rhw");
	This->genshaders[index].shader.attribs[2] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"blend0");
	This->genshaders[index].shader.attribs[3] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"blend1");
	This->genshaders[index].shader.attribs[4] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"blend2");
	This->genshaders[index].shader.attribs[5] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"blend3");
	This->genshaders[index].shader.attribs[6] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"blend4");
	This->genshaders[index].shader.attribs[7] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"nxyz");
	This->genshaders[index].shader.attribs[8] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"rgba0");
	This->genshaders[index].shader.attribs[9] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog,"rgba1");
	char attrS[] = "sX";
	for(int i = 0; i < 8; i++)
	{
		attrS[1] = i + '0';
		This->genshaders[index].shader.attribs[i + 10] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog, attrS);
	}
	char attrST[] = "stX";
	for(int i = 0; i < 8; i++)
	{
		attrST[2] = i + '0';
		This->genshaders[index].shader.attribs[i + 18] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog, attrST);
	}
	char attrSTR[] = "strX";
	for(int i = 0; i < 8; i++)
	{
		attrSTR[3] = i + '0';
		This->genshaders[index].shader.attribs[i + 26] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog, attrSTR);
	}
	char attrSTRQ[] = "strqX";
	for(int i = 0; i < 8; i++)
	{
		attrSTRQ[4] = i + '0';
		This->genshaders[index].shader.attribs[i + 34] = This->ext->glGetAttribLocation(This->genshaders[index].shader.prog, attrSTRQ);
	}
	// Uniforms
	This->genshaders[index].shader.uniforms[0] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "matWorld");
	This->genshaders[index].shader.uniforms[1] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "matModelView");
	This->genshaders[index].shader.uniforms[2] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "matProjection");
	This->genshaders[index].shader.uniforms[3] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "matNormal");
	// TODO: 4-14 world1-3 and texture0-7
	char uniflight[] = "lightX.            ";
	for(int i = 0; i < 8; i++)
	{
		uniflight[5] = i + '0';
		strcpy(uniflight+7,"diffuse");
		This->genshaders[index].shader.uniforms[20 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"specular");
		This->genshaders[index].shader.uniforms[21 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"ambient");
		This->genshaders[index].shader.uniforms[22 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"position");
		This->genshaders[index].shader.uniforms[23 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"direction");
		This->genshaders[index].shader.uniforms[24 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"range");
		This->genshaders[index].shader.uniforms[25 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"falloff");
		This->genshaders[index].shader.uniforms[26 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"constant");
		This->genshaders[index].shader.uniforms[27 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"linear");
		This->genshaders[index].shader.uniforms[28 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"quad");
		This->genshaders[index].shader.uniforms[29 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"theta");
		This->genshaders[index].shader.uniforms[30 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
		strcpy(uniflight+7,"phi");
		This->genshaders[index].shader.uniforms[31 + (i * 12)] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniflight);
	}
	char uniftex[] = "texX";
	for(int i = 0; i < 8; i++)
	{
		uniftex[3] = i + '0';
		This->genshaders[index].shader.uniforms[128 + i] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, uniftex);
	}
	This->genshaders[index].shader.uniforms[136] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"ambientcolor");
	This->genshaders[index].shader.uniforms[137] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"width");
	This->genshaders[index].shader.uniforms[138] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"height");
	This->genshaders[index].shader.uniforms[139] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"xoffset");
	This->genshaders[index].shader.uniforms[140] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"yoffset");
	This->genshaders[index].shader.uniforms[141] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"alpharef");
	char unifkey[] = "keyX";
	for(int i = 0; i < 8; i++)
	{
		unifkey[3] = i + '0';
		This->genshaders[index].shader.uniforms[142 + i] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, unifkey);
	}
	This->genshaders[index].shader.uniforms[150] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog,"ditherbits");
	This->genshaders[index].shader.uniforms[151] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "minz");
	This->genshaders[index].shader.uniforms[152] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "maxz");
	char unifkeybits[] = "keybitsX";
	for (int i = 0; i < 8; i++)
	{
		unifkeybits[7] = i + '0';
		This->genshaders[index].shader.uniforms[153 + i] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, unifkeybits);
	}
	This->genshaders[index].shader.uniforms[161] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "mtlambient");
	This->genshaders[index].shader.uniforms[162] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "mtldiffuse");
	This->genshaders[index].shader.uniforms[163] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "mtlspecular");
	This->genshaders[index].shader.uniforms[164] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "mtlemission");
	This->genshaders[index].shader.uniforms[165] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "mtlshininess");

	This->genshaders[index].shader.uniforms[166] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "fogcolor");
	This->genshaders[index].shader.uniforms[167] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "fogstart");
	This->genshaders[index].shader.uniforms[168] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "fogend");
	This->genshaders[index].shader.uniforms[169] = This->ext->glGetUniformLocation(This->genshaders[index].shader.prog, "fogdensity");

	This->genshaders[index].id = id;
	for (int i = 0; i < 8; i++)
		This->genshaders[index].texids[i] = texstate[i];
}

}