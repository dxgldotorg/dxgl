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
#include <Windows.h>

void *malloc(size_t size)
{
	void *ptr;
	ptr = HeapAlloc(GetProcessHeap(),0,size);
	if(!ptr)
	{
		_set_errno(ENOMEM);
		return NULL;
	}
	return ptr;
}

void *realloc(void *memblock, size_t size)
{
	void *ptr;
	if(!size)
	{
		free(memblock);
		return NULL;
	}
	if(!memblock) ptr = HeapAlloc(GetProcessHeap(),0,size);
	else ptr = HeapReAlloc(GetProcessHeap(),0,memblock,size);
	if(!ptr)
	{
		_set_errno(ENOMEM);
		return NULL;
	}
	return ptr;
}

void free(void *memblock)
{
	if(!memblock) return;
	if(HeapFree(GetProcessHeap(),0,memblock)) errno = EINVAL;
}