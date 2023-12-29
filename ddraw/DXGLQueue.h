// DXGL
// Copyright (C) 2022 William Feely

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
	ULONG_PTR commandpos;
	ULONG_PTR pixelbuffer;
	char *pixelbufferptr;
	ULONG_PTR pixelbuffersize;
	ULONG_PTR pixelbufferpos;
	ULONG_PTR vertexbuffer;
	char *vertexbufferptr;
	ULONG_PTR vertexbuffersize;
	ULONG_PTR vertexbufferpos;
	ULONG_PTR indexbuffer;
	char *indexbufferptr;
	ULONG_PTR indexbuffersize;
	ULONG_PTR indexbufferpos;
	BOOL filled;
	BOOL busy;
} DXGLQueue;

typedef struct DXGLPostQueueCmd
{
	char *data; // Required - contains command and non-vertex data
	char *pixelbuffer;
	char *pixelsize;
	char *vertexbuffer;
	char *vertexsize;
	char *indexbuffer;
	char *indexsize;
} DXGLPostQueueCmd;

typedef struct DXGLQueueCmd
{
	LONG command;
	DWORD size;
} DXGLQueueCmd;

#define QUEUEOP_QUIT -1
#define QUEUEOP_NULL 0
#define QUEUEOP_RESET 1
#define QUEUEOP_MAKETEXTURE 2
#define QUEUEOP_DELETETEXTURE 3

#ifdef __cplusplus
}
#endif


#endif //_DXGLQUEUE_H