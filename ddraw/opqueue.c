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

#include "common.h"
#include "opqueue.h"
#include "spinlock.h"

BOOL opqueue_init(OPQUEUE *queue)
{
	if (!queue) return FALSE;
	if (queue->data_size) return TRUE;
	else queue->data_size = 1048576;
	queue->writeptr = queue->readptr = 0;
	queue->waitfull = FALSE;
	queue->data = malloc(queue->data_size);
	if (!queue->data)
	{
		queue->data_size = 0;
		return FALSE;
	}
	queue->writelock = queue->readlock = 0;
	queue->waitevent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!queue->waitevent)
	{
		free(queue->data);
		queue->data_size = 0;
		return FALSE;
	}
	return TRUE;
}

void opqueue_delete(OPQUEUE *queue)
{
	if (!queue->data_size) return;
	CloseHandle(queue->waitevent);
	free(queue->data);
	ZeroMemory(queue, sizeof(OPQUEUE));
}

BOOL opqueue_alloc(OPQUEUE *queue, DWORD size)
{
	BYTE *ptr;
	DWORD newsize;
	if ((size * 3) > queue->data_size)
	{
		_enterspinlock(&queue->readlock);
		_enterspinlock(&queue->writelock);
		newsize = NextMultipleOf64K(size * 3);
		ptr = realloc(queue->data, newsize);
		if (ptr)
		{
			queue->data = ptr;
			queue->data_size = newsize;
			_exitspinlock(&queue->writelock);
			_exitspinlock(&queue->readlock);
			return TRUE;
		}
		else
		{
			_exitspinlock(&queue->writelock);
			_exitspinlock(&queue->readlock);
			return FALSE;
		}
	}
	return TRUE;
}

BYTE *opqueue_putlock(OPQUEUE *queue, DWORD size)
{
	BYTE *ptr;
	BOOL wraparound = FALSE;
	BOOL waitfull = FALSE;
	if (!queue->data_size) return NULL;
	if (size > queue->data_size) return NULL;  // Large items should use opqueue_alloc before attempting to do opqueue_put
	_enterspinlock(&queue->writelock);
	if (queue->writeptr + size > queue->data_size)
	{
		wraparound = TRUE;
		while(queue->readptr)
		{
			waitfull = TRUE;
			_exitspinlock(&queue->writelock);
			_enterspinlock(&queue->readlock);
			queue->waitfull = TRUE;
			_exitspinlock(&queue->readlock);
			_enterspinlock(&queue->writelock);
			WaitForSingleObject(queue->waitevent, 10);
		}
		if (waitfull) ResetEvent(queue->waitevent);
		queue->waitfull = FALSE;
	}
	if (wraparound)
	{
		queue->writeptr = 0;
	}
	ptr = queue->data + queue->writeptr;
	return ptr;
}

void opqueue_putunlock(OPQUEUE *queue, DWORD size)
{
	queue->writeptr += size;
	if (queue->waitempty) SetEvent(queue->waitevent);
	_exitspinlock(&queue->writelock);
}


BYTE *opqueue_getlock(OPQUEUE *queue)
{
	_enterspinlock(&queue->readlock);
	_enterspinlock(&queue->writelock);
	if (queue->readptr == queue->writeptr)
	{
		_exitspinlock(&queue->readlock);
		queue->waitempty = TRUE;
		ResetEvent(queue->waitevent);
		_exitspinlock(&queue->writelock);
		WaitForSingleObject(queue->waitevent, INFINITE);
		_enterspinlock(&queue->readlock);
	}
	_exitspinlock(&queue->writelock);
	return queue->data + queue->readptr;
}
void opqueue_getunlock(OPQUEUE *queue, DWORD size)
{
	BOOL rewound = FALSE;
	queue->readptr += size;
	_enterspinlock(&queue->writelock);
	if (queue->readptr >= queue->writeptr)
	{
		queue->readptr = 0;
		queue->writeptr = 0;
		rewound = TRUE;
	}
	_exitspinlock(&queue->writelock);
	if (queue->waitfull)
	{
		if (rewound) SetEvent(queue->waitevent);
	}
	_exitspinlock(&queue->readlock);
}
