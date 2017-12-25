// DXGL
// Copyright (C) 2015-2016 William Feely

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

#pragma once
#ifndef __STRUCT_H
#define __STRUCT_H

// Vertex format used by Blt calls
typedef struct
{
	GLfloat x, y;
	GLfloat s, t;
	GLfloat dests, destt;
	GLfloat stencils, stencilt;
} BltVertex;

// Sampler object state information
typedef struct SAMPLER
{
	GLuint id;
	GLint wraps;
	GLint wrapt;
	GLint minfilter;
	GLint magfilter;
} SAMPLER;

// Timer used to simulate display timing
typedef struct DXGLTimer
{
	int timertype;
	unsigned int lines;
	unsigned int vsync_lines;
	double monitor_period;
	double timer_frequency;
	LARGE_INTEGER timer_base;
} DXGLTimer;

// OpenGL Extensions structure
typedef struct glExtensions
{
	GLuint(APIENTRY *glCreateShader) (GLenum type);
	void (APIENTRY *glShaderSource) (GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
	void (APIENTRY *glCompileShader) (GLuint shader);
	void (APIENTRY *glDeleteShader) (GLuint shader);
	GLuint(APIENTRY *glCreateProgram) ();
	void (APIENTRY *glDeleteProgram) (GLuint program);
	void (APIENTRY *glGetProgramiv) (GLuint program, GLenum pname, GLint* params);
	void (APIENTRY *glAttachShader) (GLuint program, GLuint shader);
	void (APIENTRY *glDetachShader) (GLuint program, GLuint shader);
	void (APIENTRY *glLinkProgram) (GLuint program);
	void (APIENTRY *glUseProgram) (GLuint program);
	void (APIENTRY *glGetShaderiv) (GLuint shader, GLenum pname, GLint* params);
	void (APIENTRY *glGetShaderInfoLog) (GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
	void (APIENTRY *glGetProgramInfoLog) (GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infolog);

	GLint(APIENTRY *glGetAttribLocation) (GLuint program, const GLchar* name);
	void (APIENTRY *glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer);
	void (APIENTRY *glEnableVertexAttribArray) (GLuint index);
	void (APIENTRY *glDisableVertexAttribArray) (GLuint index);

	void (APIENTRY *glGenFramebuffers) (GLsizei n, GLuint* ids);
	void (APIENTRY *glBindFramebuffer) (GLenum target, GLuint framebuffer);
	void (APIENTRY *glGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
	void (APIENTRY *glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
	void (APIENTRY *glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	GLenum(APIENTRY *glCheckFramebufferStatus) (GLenum target);
	void (APIENTRY *glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);

	void (APIENTRY *glGenFramebuffersEXT) (GLsizei n, GLuint* ids);
	void (APIENTRY *glBindFramebufferEXT) (GLenum target, GLuint framebuffer);
	void (APIENTRY *glGenRenderbuffersEXT) (GLsizei n, GLuint* renderbuffers);
	void (APIENTRY *glBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
	void (APIENTRY *glFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	GLenum(APIENTRY *glCheckFramebufferStatusEXT) (GLenum target);
	void (APIENTRY *glDeleteFramebuffersEXT) (GLsizei n, const GLuint *framebuffers);

	GLint(APIENTRY *glGetUniformLocation) (GLuint program, const GLchar* name);
	void (APIENTRY *glUniform1i) (GLint location, GLint v0);
	void (APIENTRY *glUniform2i) (GLint location, GLint v0, GLint v1);
	void (APIENTRY *glUniform3i) (GLint location, GLint v0, GLint v1, GLint v2);
	void (APIENTRY *glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	void (APIENTRY *glUniform3iv) (GLint location, GLsizei count, const GLint* value);
	void (APIENTRY *glUniform4iv) (GLint location, GLsizei count, const GLint* value);
	void (APIENTRY *glUniform1f) (GLint location, GLfloat v0);
	void (APIENTRY *glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
	void (APIENTRY *glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	void (APIENTRY *glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	void (APIENTRY *glUniform3fv) (GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY *glUniform4fv) (GLint location, GLsizei count, const GLfloat* value);
	void (APIENTRY *glUniformMatrix3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
	void (APIENTRY *glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);

	void (APIENTRY *glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

	void (APIENTRY *glActiveTexture)(GLenum texture);
	void (APIENTRY *glClientActiveTexture) (GLenum texture);

	void (APIENTRY *glGenBuffers)(GLsizei n, GLuint* buffers);
	void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint* buffers);
	void (APIENTRY *glBindBuffer)(GLenum target, GLuint buffer);
	void (APIENTRY *glBufferData)(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
	void* (APIENTRY *glMapBuffer)(GLenum target, GLenum access);
	GLboolean(APIENTRY *glUnmapBuffer)(GLenum target);

	BOOL(APIENTRY *wglSwapIntervalEXT)(int interval);
	int (APIENTRY *wglGetSwapIntervalEXT)();

	void (APIENTRY *glTextureParameterfEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
	void (APIENTRY *glTextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTextureParameteriEXT)(GLuint texture, GLenum target, GLenum pname, GLint param);
	void (APIENTRY *glTextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
	void (APIENTRY *glTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint internalformat,
		GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
	void (APIENTRY *glTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
		GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glGetTextureImageEXT)(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void (APIENTRY *glMatrixLoadfEXT)(GLenum mode, const GLfloat *m);
	void (APIENTRY *glMatrixMultfEXT)(GLenum mode, const GLfloat *m);
	void (APIENTRY *glNamedBufferDataEXT)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
	void* (APIENTRY *glMapNamedBufferEXT)(GLuint buffer, GLenum access);
	GLboolean(APIENTRY *glUnmapNamedBufferEXT)(GLuint buffer);

	void (APIENTRY *glTextureParameterf)(GLuint texture, GLenum pname, GLfloat param);
	void (APIENTRY *glTextureParameterfv)(GLuint texture, GLenum pname, const GLfloat *params);
	void (APIENTRY *glTextureParameteri)(GLuint texture, GLenum pname, GLint param);
	void (APIENTRY *glTextureParameteriv)(GLuint texture, GLenum pname, const GLint *params);
	void (APIENTRY *glTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
		GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void (APIENTRY *glGetTextureImage)(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, GLvoid *pixels);
	void (APIENTRY *glNamedBufferData)(GLuint buffer, GLsizei size, const void *data, GLenum usage);
	void* (APIENTRY *glMapNamedBuffer)(GLuint buffer, GLenum access);
	GLboolean(APIENTRY *glUnmapNamedBuffer)(GLuint buffer);

	void (APIENTRY *glBindSampler)(GLuint unit, GLuint sampler);
	void (APIENTRY *glDeleteSamplers)(GLsizei n, const GLuint *samplers);
	void (APIENTRY *glGenSamplers)(GLsizei n, GLuint *samplers);
	void (APIENTRY *glSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
	void (APIENTRY *glSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
	void (APIENTRY *glSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *params);
	void (APIENTRY *glSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *params);

	void (APIENTRY *glFrameTerminatorGREMEDY)();

	int GLEXT_ARB_framebuffer_object;
	int GLEXT_EXT_framebuffer_object;
	int GLEXT_NV_packed_depth_stencil;
	int GLEXT_EXT_packed_depth_stencil;
	int GLEXT_ARB_depth_buffer_float;
	int GLEXT_ARB_depth_texture;
	int GLEXT_NVX_gpu_memory_info;
	int GLEXT_ATI_meminfo;
	int GLEXT_ARB_ES2_compatibility;
	int GLEXT_EXT_direct_state_access;
	int GLEXT_ARB_direct_state_access;
	int GLEXT_ARB_sampler_objects;
	int GLEXT_EXT_gpu_shader4;
	int GLEXT_GREMEDY_frame_terminator;
	int glver_major;
	int glver_minor;
	BOOL atimem;
} glExtensions;

// Buffer object (such as PBO or VBO)
typedef struct BufferObject
{
	ULONG refcount;
	ULONG busy;
	GLuint buffer;
	GLsizei size;
	GLbyte *pointer;
	BOOL mapped;
	BOOL bound;
	BOOL target;
	glExtensions *ext;
	struct glUtil *util;
} BufferObject;

struct glTexture;

// Framebuffer object state
typedef struct FBO
{
	GLuint fbo;
	struct glTexture *fbcolor;
	struct glTexture *fbz;
	BOOL stencil;
	GLenum status;
} FBO;

// Various OpenGL state items
typedef struct glUtil
{
	ULONG refcount;
	FBO *currentfbo;
	BufferObject *pboPackBinding;
	BufferObject *pboUnpackBinding;
	BufferObject *vboArrayBinding;
	BufferObject *vboElementArrayBinding;
	BufferObject *uboUniformBufferBinding;
	glExtensions *ext;
	BOOL depthwrite;
	BOOL depthtest;
	GLuint depthcomp;
	GLuint alphacomp;
	GLint scissorx;
	GLint scissory;
	GLsizei scissorwidth;
	GLsizei scissorheight;
	GLint viewportx;
	GLint viewporty;
	GLsizei viewportwidth;
	GLsizei viewportheight;
	GLclampd depthnear;
	GLclampd depthfar;
	GLenum matrixmode;
	GLfloat materialambient[4];
	GLfloat materialdiffuse[4];
	GLfloat materialspecular[4];
	GLfloat materialemission[4];
	GLfloat materialshininess;
	BOOL scissorenabled;
	GLint texwrap[16];
	GLclampf clearr;
	GLclampf clearg;
	GLclampf clearb;
	GLclampf cleara;
	GLclampd cleardepth;
	GLint clearstencil;
	GLenum blendsrc;
	GLenum blenddest;
	BOOL blendenabled;
	BOOL arrays[42];
	D3DCULL cullmode;
	BOOL cullenabled;
	D3DFILLMODE polymode;
	D3DSHADEMODE shademode;
	BufferObject *LastBoundBuffer;
	SAMPLER samplers[16];
	GLint texlevel;
	GLuint textures[16];
} glUtil;

// Storage for DIB info
typedef struct DIB
{
	BOOL enabled;
	int width;
	int height;
	int pitch;
	HDC hdc;
	HBITMAP hbitmap;
	BITMAPINFO *info;
	BYTE *pixels;
} DIB;

// Texture object mipmap level
typedef struct MIPLEVEL
{
	DDSURFACEDESC2 ddsd;
	char *buffer;
	char *bigbuffer;
	char *gdibuffer;
	HDC hdc;
	HBITMAP hbitmap;
	BITMAPINFO *bitmapinfo;
	BufferObject *pboPack;
	BufferObject *pboUnpack;
	DWORD dirty;
	// dirty bits:
	// 1 - Surface buffer was locked and may have been written to by CPU
	// 2 - Texture was written to by GPU
	DWORD locked;
	FBO fbo;
} MIPLEVEL;

// Surface texture object
typedef struct glTexture
{
	UINT refcount;
	GLuint id;
	MIPLEVEL levels[17];  // Future proof to 64k
	GLint bigwidth;
	GLint bigheight;
	BOOL bigprimary;
	GLint minfilter;
	GLint magfilter;
	GLint wraps;
	GLint wrapt;
	GLint miplevel;
	DWORD bordercolor;
	GLint internalformats[8];
	DWORD colorsizes[4];
	DWORD colorbits[4];
	int colororder;
	GLenum format;
	GLenum type;
	BOOL zhasstencil;
	struct glTexture *palette;
	struct glTexture *stencil;
	struct glTexture *dummycolor;
	struct glRenderer *renderer;
} glTexture;

// Color orders:
// 0 - ABGR
// 1 - ARGB
// 2 - BGRA
// 3 - RGBA
// 4 - R or Indexed
// 5 - Luminance
// 6 - Alpha
// 7 - Luminance Alpha

typedef struct SHADER
{
	GLint vs;
	GLint fs;
	const char *vsrc;
	const char *fsrc;
	GLint prog;
	GLint pos;
	GLint texcoord;
	GLint tex0;
	GLint tex1;
	GLint ckey;
	GLint colorsize;
	GLint pal;
	GLint view;
} SHADER;

struct ShaderGen3D;
struct ShaderGen2D;

typedef struct ShaderManager
{
	SHADER *shaders;
	struct ShaderGen3D *gen3d;
	struct ShaderGen2D *gen2d;
	glExtensions *ext;
} ShaderManager;

typedef struct SetWndCommand
{
	int width;
	int height;
	int bpp;
	int fullscreen;
	unsigned int frequency;
	HWND newwnd;
	BOOL devwnd;
} SetWndCommand;

typedef struct BltCommand
{
	//DWORD opcode;
	//DWORD size;
	RECT destrect;
	RECT srcrect;
	glTexture *dest;
	glTexture *src;
	GLint destlevel;
	GLint srclevel;
	DWORD flags;
	DDBLTFX bltfx;
	glTexture *zdest;
	glTexture *zsrc;
	glTexture *alphadest;
	glTexture *alphasrc;
	glTexture *pattern;
	GLint zdestlevel;
	GLint zsrclevel;
	GLint alphadestlevel;
	GLint alphasrclevel;
	GLint patternlevel;
	DDCOLORKEY srckey;
	DDCOLORKEY destkey;
}BltCommand;

typedef struct ClearCommand
{
	DWORD dwCount;
	LPD3DRECT lpRects;
	DWORD dwFlags;
	DWORD dwColor;
	D3DVALUE dvZ;
	DWORD dwStencil;
	glTexture *target;
	glTexture *zbuffer;
	GLint targetlevel;
	GLint zlevel;
}ClearCommand;

struct glDirectDrawPaletteVtbl;

// Structure for glDirectDrawPalette, emulates IDirectDrawPalette
typedef struct glDirectDrawPalette
{
	struct glDirectDrawPaletteVtbl *lpVtbl;

	ULONG refcount;
	PALETTEENTRY palette[256];
	glTexture *texture;
	DWORD flags;
	IUnknown *creator;
} glDirectDrawPalette;

// Function pointer table for glDirectDrawPalette
typedef struct glDirectDrawPaletteVtbl
{
	// DirectDraw (all versions) API
	HRESULT(WINAPI *QueryInterface)(glDirectDrawPalette *This, REFIID riid, void** ppvObj);
	ULONG(WINAPI *AddRef)(glDirectDrawPalette *This);
	ULONG(WINAPI *Release)(glDirectDrawPalette *This);

	HRESULT(WINAPI *GetCaps)(glDirectDrawPalette *This, LPDWORD lpdwCaps);
	HRESULT(WINAPI *GetEntries)(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries);
	HRESULT(WINAPI *Initialize)(glDirectDrawPalette *This, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable);
	HRESULT(WINAPI *SetEntries)(glDirectDrawPalette *This, DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries);
} glDirectDrawPaletteVtbl;

typedef struct CommandBuffer
{
	BYTE *cmdbuffer;
	size_t cmdsize;
	BYTE *uploadbuffer;
	size_t uploadsize;
	BufferObject *vertices;
	BufferObject *indices;
	size_t write_ptr_cmd;
	size_t write_ptr_cmd_modify;
	size_t read_ptr_cmd;
	size_t write_ptr_upload;
	size_t read_ptr_upload;
	size_t write_ptr_vertex;
	size_t read_ptr_vertex;
	size_t write_ptr_index;
	size_t read_ptr_index;
} CommandBuffer;

typedef struct RenderTarget
{
	glTexture *target;
	glTexture *zbuffer;
	GLint level;
	GLint zlevel;
	GLfloat mulx;
	GLfloat muly;
} RenderTarget;

typedef struct GLCAPS
{
	float Version;
	float ShaderVer;
	GLint TextureMax;
} GLCAPS;

typedef struct GLVERTEX
{
	void *data;
	int stride;
} GLVERTEX;

typedef struct SHADERSTATE
{
	__int64 stateid;
	__int64 texstageid[8];
} SHADERSTATE;

typedef struct VIEWPORT
{
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei hieght;
} VIEWPORT;

#endif //__STRUCT_H