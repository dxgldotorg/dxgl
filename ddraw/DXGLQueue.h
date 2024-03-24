// DXGL
// Copyright (C) 2023 William Feely

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
#ifndef _DXGLQUEUE_H
#define _DXGLQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DXGLQueue
{
	char *commands;
	ULONG_PTR commandsize;
	ULONG_PTR commandread, commandwrite;
	ULONG_PTR pixelbuffer;
	char *pixelbufferptr;
	ULONG_PTR pixelbuffersize;
	ULONG_PTR pixelbufferread, pixelbufferwrite;
	ULONG_PTR vertexbuffer;
	char *vertexbufferptr;
	ULONG_PTR vertexbuffersize;
	ULONG_PTR vertexbufferread, vertexbufferwrite;
	ULONG_PTR indexbuffer;
	char *indexbufferptr;
	ULONG_PTR indexbuffersize;
	ULONG_PTR indexbufferread, indexbufferwrite;
	BOOL filled;
	BOOL busy;
	BOOL inscene;
} DXGLQueue;

// A queue command that doesn't take arguments
typedef struct DXGLQueueCmd
{
	LONG command;
	DWORD size;
} DXGLQueueCmd;

typedef struct DXGLQueueCmdCreateTexture
{
	LONG command;
	DWORD size;
	DDSURFACEDESC2 desc;
	DXGLTexture **out;
} DXGLQueueCmdCreateTexture;

typedef struct DXGLQueueCmdDeleteTexture
{
	LONG command;
	DWORD size;
	DWORD_PTR count;
	DXGLTexture *texture;
} DXGLQueueCmdDeleteTexture;

typedef struct DXGLQueueCmdFreePointer
{
	LONG command;
	DWORD size;
	DWORD_PTR count;
	void *ptr;
} DXGLQueueCmdFreePointer;

typedef struct DXGLQueueCmdSetCooperativeLevel
{
	LONG command;
	DWORD size;
	HWND hWnd;
	DWORD dwFlags;
} DXGLQueueCmdSetCooperativeLevel;

typedef struct DXGLQueueCmdExpandBuffers
{
	LONG command;
	DWORD size;
	DWORD_PTR cmdsize;
	DWORD_PTR pixelsize;
	DWORD_PTR vertexsize;
	DWORD_PTR indexsize;
} DXGLQueueCmdExpandBuffers;

typedef union DXGLQueueCmdDecoder
{
	DXGLQueueCmd cmd;
	DXGLQueueCmdCreateTexture createtexture;
	DXGLQueueCmdDeleteTexture deletetexture;
	DXGLQueueCmdFreePointer freepointer;
	DXGLQueueCmdSetCooperativeLevel setcooplevel;
	DXGLQueueCmdExpandBuffers expandbuffers;
} DXGLQueueCmdDecoder;

typedef struct DXGLPostQueueCmd
{
	DXGLQueueCmd *data; // Required - contains command and non-vertex data
	char *pixelbuffer;
	DWORD pixelsize;
	char *vertexbuffer;
	DWORD vertexsize;
	char *indexbuffer;
	DWORD indexsize;
} DXGLPostQueueCmd;

#define QUEUEOP_QUIT -1
#define QUEUEOP_NULL 0
#define QUEUEOP_RESET 1
#define QUEUEOP_CREATETEXTURE 2
#define QUEUEOP_DELETETEXTURE 3
#define QUEUEOP_BREAK 4
#define QUEUEOP_FREEPOINTER 5
#define QUEUEOP_EXPANDBUFFERS 6

#ifdef __cplusplus
}
#endif


#endif //_DXGLQUEUE_H