// DXGL
// Copyright (C) 2016 William Feely

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
#ifndef __STRUCT_COMMAND_H
#define __STRUCT_COMMAND_H

typedef struct GenericCmd
{
	DWORD opcode;
	DWORD size;
} GenericCmd;
typedef struct MakeTextureCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
} MakeTextureCmd;
typedef struct UploadTextureCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
	GLint level;
	DWORD texturesize;
	BYTE *content;
	DWORD offset;
} UploadTextureCmd;
typedef struct DownloadTextureCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
	GLint level;
} DownloadTextureCmd;
typedef struct DeleteTextureCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
} DeleteTextureCmd;
typedef struct BltCmd
{
	DWORD opcode;
	DWORD size;
	BltCommand cmd;
} BltCmd;
typedef struct DrawScreenCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
	glTexture *paltex;
	GLint vsync;
	glTexture *previous;
} DrawScreenCmd;
typedef struct InitD3DCmd
{
	DWORD opcode;
	DWORD size;
	int zbuffer;
	int x;
	int y;
} InitD3DCmd;
typedef struct ClearCmd
{
	DWORD opcode;
	DWORD size;
	ClearCommand cmd;
} ClearCmd;
typedef struct FlushCmd
{
	DWORD opcode;
	DWORD size;
} FlushCmd;
typedef struct DrawPrimitivesCmd
{
	DWORD opcode;
	DWORD size;
	RenderTarget *target;
	GLenum mode;
	GLVERTEX *vertices;
	int *texformats;
	DWORD count;
	LPWORD indices;
} DrawPrimitivesCmd;
typedef struct UpdateClipperCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *stencil;
	GLushort *indices;
	BltVertex *vertices;
	GLsizei count;
	GLsizei width;
	GLsizei height;
} UpdateClipperCmd;
typedef struct DepthFillCmd
{
	DWORD opcode;
	DWORD size;
	BltCommand cmd;
	glTexture *parent;
	GLint parentlevel;
} DepthFillCmd;
typedef struct SetRenderStateCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	struct
	{
		D3DRENDERSTATETYPE dwRendStateType;
		DWORD dwRenderState;
	} state[1];
	// For count over 1, use additional pairs to store state changes.
} SetRenderStateCmd;
typedef struct SetTextureCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	struct
	{
		DWORD stage;
		glTexture *texture;
	} texstage[1];
	// For count over 1, use additional pairs to store texture changes.
} SetTextureCmd;
typedef struct SetTextureStageStateCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	DWORD dwStage;
	struct
	{
		D3DTEXTURESTAGESTATETYPE dwState;
		DWORD dwValue;
	} state[1];
	// For count over 1, use additional sets to store texture state changes.
} SetTextureStageStateCmd;
typedef struct SetTransformCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	struct
	{
		D3DTRANSFORMSTATETYPE dtstTransformStateType;
		D3DMATRIX matrix;
	} transform[1];
} SetTransformCmd;
typedef struct SetMaterialCmd
{
	DWORD opcode;
	DWORD size;
	D3DMATERIAL7 material;
} SetMaterialCmd;
typedef struct SetLightCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	DWORD index;
	D3DLIGHT7 light[1];
	// For count over 1, use additional pairs to store lights
} SetLightCmd;
typedef struct RemoveLightCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	DWORD index[1];
	// For count over 1, use additional indices
} RemoveLightCmd;
typedef struct SetD3DViewportCmd
{
	DWORD opcode;
	DWORD size;
	D3DVIEWPORT7 viewport;
} SetD3DViewportCmd;
typedef struct SetTextureColorKeyCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
	DWORD flags;
	DDCOLORKEY colorkey;
	GLint level;
} SetTextureColorKeyCmd;
typedef struct MakeTexturePrimaryCmd
{
	DWORD opcode;
	DWORD size;
	glTexture *texture;
	glTexture *parent;
	BOOL primary;
} MakeTexturePrimaryCmd;
typedef struct DXGLBreakCmd
{
	DWORD opcode;
	DWORD size;
} DXGLBreakCmd;
typedef struct InitTextureStageCmd
{
	DWORD opcode;
	DWORD size;
	DWORD count;
	DWORD stage[1];
} InitTextureStageCmd;
typedef struct SetTextureSurfaceDescCmd
{
	DWORD opcode;
	DWORD size;
	GLint level;
	DDSURFACEDESC2 desc;
} SetTextureSurfaceDescCmd;
typedef struct SetShader2DCmd
{
	DWORD opcode;
	DWORD size;
	int type; // 0 for builtin, 1 for 2D, 2 for 3D (INVALID, use full command)
	__int64 id;
};
typedef struct SetShaderCmd
{
	DWORD opcode;
	DWORD size;
	int type; // 0 for builtin, 1 for 2D, 2 for 3D
	__int64 id;
	__int64 texstate[8];
};

typedef struct MIN_STORAGE_Cmd
{
	BYTE data[256];
} MIN_STORAGE_CMD;

typedef union QueueCmd
{
	GenericCmd Generic;
	MakeTextureCmd MakeTexture;
	UploadTextureCmd UploadTexture;
	DownloadTextureCmd DownloadTexture;
	DeleteTextureCmd DeleteTexture;
	BltCmd Blt;
	DrawScreenCmd DrawScreen;
	InitD3DCmd InitD3D;
	ClearCmd Clear;
	FlushCmd Flush;
	DrawPrimitivesCmd DrawPrimitives;
	UpdateClipperCmd UpdateClipper;
	DepthFillCmd DepthFill;
	SetRenderStateCmd SetRenderState;
	SetTextureCmd SetTexture;
	SetTextureStageStateCmd SetTextureStageState;
	SetTransformCmd SetTransform;
	SetMaterialCmd SetMaterial;
	SetLightCmd SetLight;
	RemoveLightCmd RemoveLight;
	SetD3DViewportCmd SetD3DViewport;
	SetTextureColorKeyCmd SetTextureColorKey;
	MakeTexturePrimaryCmd MakeTexturePrimary;
	DXGLBreakCmd DXGLBreak;
	InitTextureStageCmd InitTextureStage;
	SetTextureSurfaceDescCmd SetTextureSurfaceDesc;
	SetShader2DCmd SetShader2D;
	SetShaderCmd SetShader;
	MIN_STORAGE_CMD MIN_STORAGE;
} QueueCmd;

typedef struct RenderState
{
	CommandBuffer *cmd;
	QueueCmd last_cmd;
	BYTE *last_cmd_start;
} RenderState;

#endif //__STRUCT_COMMAND_H