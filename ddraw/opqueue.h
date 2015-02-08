// DXGL
// Copyright (C) 2015 William Feely

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
#ifndef _OPQUEUE_H
#define _OPQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

struct OPQUEUE;

typedef struct OPQUEUE
{
	ULONG_PTR data_size;
	ULONG_PTR writeptr;
	ULONG_PTR readptr;
	BYTE *data;
	HANDLE waitevent;
	BOOL waitfull;
	BOOL waitempty;
	DWORD writelock;
	DWORD readlock;
} OPQUEUE;

BOOL opqueue_init(OPQUEUE *queue);
void opqueue_delete(OPQUEUE *queue);
BOOL opqueue_alloc(OPQUEUE *queue, DWORD size);
BYTE *opqueue_putlock(OPQUEUE *queue, DWORD size);
void opqueue_putunlock(OPQUEUE *queue, DWORD size);
BYTE *opqueue_getlock(OPQUEUE *queue);
void opqueue_getunlock(OPQUEUE *queue, DWORD size);

#ifdef __cplusplus
}
#endif

#endif //_OPQUEUE_H