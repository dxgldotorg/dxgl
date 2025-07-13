// DXGL
// Copyright (C) 2023-2025 William Feely

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
	ULONG_PTR commandwrite;
	ULONG_PTR pixelbuffer;
	char *pixelbufferptr;
	ULONG_PTR pixelbuffersize;
	ULONG_PTR pixelbufferwrite;
	ULONG_PTR vertexbuffer;
	char *vertexbufferptr;
	ULONG_PTR vertexbuffersize;
	ULONG_PTR vertexbufferwrite;
	ULONG_PTR indexbuffer;
	char *indexbufferptr;
	ULONG_PTR indexbuffersize;
	ULONG_PTR indexbufferwrite;
	struct
	{
		LONG command;
		ULONG_PTR ptr;
	} lastcommand;
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
	DXGLTexture *out;
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

typedef struct DXGLQueueCmdLock
{
	LONG command;
	DWORD size;
	DXGLTexture *texture;
	GLuint miplevel;
	BYTE **ptr;
} DXGLQueueCmdLock;

typedef struct DXGLQueueCmdUnlock
{
	LONG command;
	DWORD size;
	DXGLTexture *texture;
	GLuint miplevel;
} DXGLQueueCmdUnlock;

typedef struct DXGLQueueCmdSetTexture
{
	LONG command;
	DWORD size;
	GLuint level;
	DXGLTexture *texture;
} DXGLQueueCmdSetTexture;

typedef struct DXGLQueCmdSetTarget
{
	LONG command;
	DWORD size;
	DXGLTexture *texture;
	GLuint miplevel;
} DXGLQueueCmdSetTarget;

typedef struct DXGLQueueCmdSetRenderState
{
	LONG command;
	DWORD size;
	DXGLRenderState state;
} DXGLQueueCmdSetRenderState;

typedef struct DXGLQueueCmdSetFVF
{
	LONG command;
	DWORD size;
	DWORD fvf;
} DXGLQueueCmdSetFVF;

typedef struct DXGLQueueCmdDrawPrimitives2D
{
	LONG command;
	DWORD size;
	D3DPRIMITIVETYPE type;
	DWORD vertexcount;
	BYTE *vertices;
} DXGLQueueCmdDrawPrimitives2D;

typedef struct DXGLQueueCmdDrawPrimitives
{
	LONG command;
	DWORD size;
	D3DPRIMITIVETYPE type;
	DWORD vertexcount;
	DWORD indexcount;
	BYTE *vertices;
	WORD *indices;
} DXGLQueueCmdDrawPrimitives;

typedef struct DXGLQueueCmdSwapBuffers
{
	LONG command;
	DWORD size;
	int interval;
} DXGLQueueCmdSwapBuffers;

typedef struct DXGLQueueCmdSetWindowSize
{
	LONG command;
	DWORD size;
	RECT r;
} DXGLQueueCmdSetWindowSize;

typedef union DXGLQueueCmdDecoder
{
	DXGLQueueCmd cmd;
	DXGLQueueCmdCreateTexture createtexture;
	DXGLQueueCmdDeleteTexture deletetexture;
	DXGLQueueCmdFreePointer freepointer;
	DXGLQueueCmdSetCooperativeLevel setcooplevel;
	DXGLQueueCmdExpandBuffers expandbuffers;
	DXGLQueueCmdLock lock;
	DXGLQueueCmdUnlock unlock;
	DXGLQueueCmdSetTexture settexture;
	DXGLQueueCmdSetTarget settarget;
	DXGLQueueCmdSetRenderState setrenderstate;
	DXGLQueueCmdSetFVF setfvf;
	DXGLQueueCmdDrawPrimitives2D drawprimitives2d;
	DXGLQueueCmdDrawPrimitives drawprimitives;
	DXGLQueueCmdSwapBuffers swapbuffers;
	DXGLQueueCmdSetWindowSize setwindowsize;
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
#define QUEUEOP_SETTEXTURE 4
#define QUEUEOP_SETTARGET 5
#define QUEUEOP_SETRENDERSTATE 6
#define QUEUEOP_SETFVF 7
#define QUEUEOP_DRAWPRIMITIVES2D 8
#define QUEUEOP_DRAWPRIMITIVES 9
#define QUEUEOP_BREAK 10
#define QUEUEOP_FREEPOINTER 11
#define QUEUEOP_EXPANDBUFFERS 12
#define QUEUEOP_LOCK 13
#define QUEUEOP_UNLOCK 14
#define QUEUEOP_SWAPBUFFERS 15
#define QUEUEOP_SETWINDOWSIZE 16

#ifdef __cplusplus
}
#endif


#endif //_DXGLQUEUE_H